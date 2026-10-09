"""NNUE-style evaluation network: 768 piece-square inputs per side -> 2 x hidden -> l1 -> 1.

The first layer is exported as int16 (scale QA) so the engine can update it incrementally and
exactly; the small dense layers stay float32. See engine/include/evaluation/nnue.h for the format.

Usage: python -m models.nnue labelled.parquet net.nnue [--epochs 20] [--hidden 128]
"""
from __future__ import annotations

import argparse
import struct
from collections.abc import Callable
from pathlib import Path

import chess
import numpy as np
import pyarrow.parquet as pq
import torch
from torch import nn

from experiments.tracking import record_run
from features.extract import PIECE_VALUES
from models.baselines import split_by_game

NUM_FEATURES = 768
PAD = NUM_FEATURES  # Empty slots in a padded feature list
MAX_PIECES = 32
QA = 255
WDL_SCALE = 400.0
WEIGHT_CLIP = 2.0  # Bounds first-layer weights so int16 accumulators cannot overflow


def feature_index(piece: chess.Piece, square: chess.Square, perspective: chess.Color) -> int:
    """Same mapping as the engine: Black's view mirrors the board and swaps colours."""
    offset = 0 if piece.color == perspective else 6
    return (offset + piece.piece_type - 1) * 64 + (square if perspective == chess.WHITE else square ^ 56)


def encode(board: chess.Board) -> tuple[list[int], list[int]]:
    """Active features from the side to move's view and from the opponent's view."""
    pieces = board.piece_map().items()
    us, them = board.turn, not board.turn
    return [feature_index(p, sq, us) for sq, p in pieces], [feature_index(p, sq, them) for sq, p in pieces]


def encode_fens(fens: list[str]) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    """Padded (N, 32) index arrays for both views, plus side-to-move material in centipawns."""
    stm = np.full((len(fens), MAX_PIECES), PAD, dtype=np.int16)
    nstm = np.full((len(fens), MAX_PIECES), PAD, dtype=np.int16)
    material = np.zeros(len(fens), dtype=np.float32)
    for row, fen in enumerate(fens):
        board = chess.Board(fen)
        a, b = encode(board)
        stm[row, :len(a)], nstm[row, :len(b)] = a, b
        material[row] = sum(PIECE_VALUES.get(p.piece_type, 0) * (1 if p.color == board.turn else -1)
                            for p in board.piece_map().values())
    return stm, nstm, material


class NNUE(nn.Module):
    def __init__(self, hidden: int = 128, l1: int = 32):
        super().__init__()
        self.hidden, self.l1_size = hidden, l1
        self.ft = nn.EmbeddingBag(NUM_FEATURES + 1, hidden, mode="sum", padding_idx=PAD)
        nn.init.normal_(self.ft.weight, std=0.05)
        self.ft_bias = nn.Parameter(torch.full((hidden,), 0.25))
        self.l1 = nn.Linear(2 * hidden, l1)
        self.out = nn.Linear(l1, 1)

    def forward(self, stm: torch.Tensor, nstm: torch.Tensor) -> torch.Tensor:
        """Win-probability logit for the side to move; multiply by WDL_SCALE for centipawns."""
        us = torch.clamp(self.ft(stm) + self.ft_bias, 0.0, 1.0)
        them = torch.clamp(self.ft(nstm) + self.ft_bias, 0.0, 1.0)
        hidden = torch.clamp(self.l1(torch.cat([us, them], dim=1)), 0.0, 1.0)
        return self.out(hidden).squeeze(1)

    def clip_weights(self) -> None:
        with torch.no_grad():
            self.ft.weight.clamp_(-WEIGHT_CLIP, WEIGHT_CLIP)
            self.ft_bias.clamp_(-WEIGHT_CLIP, WEIGHT_CLIP)


def make_targets(eval_cp: np.ndarray, result: np.ndarray, side_to_move: np.ndarray, lam: float) -> np.ndarray:
    """Blend of the engine eval (as a win probability) and the game result, both for the side to move."""
    eval_prob = 1.0 / (1.0 + np.exp(-eval_cp / WDL_SCALE))
    result_stm = np.where(side_to_move == 0, result, 1.0 - result)
    return (lam * eval_prob + (1.0 - lam) * result_stm).astype(np.float32)


def predict_cp(model: NNUE, fens: list[str]) -> np.ndarray:
    stm, nstm, _ = encode_fens(fens)
    model.eval()
    with torch.no_grad():
        logits = model(torch.from_numpy(stm.astype(np.int64)), torch.from_numpy(nstm.astype(np.int64)))
    return logits.numpy() * WDL_SCALE


def export(model: NNUE, path: str | Path) -> None:
    def quantize(t: torch.Tensor) -> bytes:
        return torch.round(t.detach() * QA).numpy().astype("<i2").tobytes()

    def floats(t: torch.Tensor) -> bytes:
        return t.detach().numpy().astype("<f4").tobytes()

    with open(path, "wb") as f:
        f.write(b"CINN")
        f.write(struct.pack("<3I", 1, model.hidden, model.l1_size))
        f.write(quantize(model.ft.weight[:NUM_FEATURES]))
        f.write(quantize(model.ft_bias))
        f.write(floats(model.l1.weight))
        f.write(floats(model.l1.bias))
        f.write(floats(model.out.weight.reshape(-1)))
        f.write(floats(model.out.bias))


def train(path: str | Path, *, hidden: int = 128, l1: int = 32, epochs: int = 20, batch_size: int = 4096,
          lr: float = 1e-3, lam: float = 0.9, test_size: float = 0.1, seed: int = 0, max_rows: int | None = None,
          log: Callable[[str], None] = print) -> tuple[NNUE, dict[str, float]]:
    torch.manual_seed(seed)
    rng = np.random.default_rng(seed)
    table = pq.read_table(path, columns=["fen", "eval_cp", "result", "side_to_move", "game_id"])
    if max_rows is not None:
        table = table.slice(0, max_rows)
    eval_cp = table.column("eval_cp").to_numpy().astype(np.float32)
    targets = make_targets(eval_cp, table.column("result").to_numpy(), table.column("side_to_move").to_numpy(), lam)
    stm_np, nstm_np, material = encode_fens(table.column("fen").to_pylist())
    train_idx, val_idx = split_by_game(table.column("game_id").to_numpy(), test_size, seed)

    stm, nstm = torch.from_numpy(stm_np.astype(np.int64)), torch.from_numpy(nstm_np.astype(np.int64))
    y = torch.from_numpy(targets)
    model = NNUE(hidden, l1)
    optimizer = torch.optim.Adam(model.parameters(), lr=lr)
    scheduler = torch.optim.lr_scheduler.CosineAnnealingLR(optimizer, T_max=epochs)
    val = torch.from_numpy(val_idx)

    def val_metrics() -> tuple[float, float]:
        model.eval()
        with torch.no_grad():
            logits = model(stm[val], nstm[val])
            loss = torch.mean((torch.sigmoid(logits) - y[val]) ** 2).item()
        mae = float(np.mean(np.abs(logits.numpy() * WDL_SCALE - eval_cp[val_idx])))
        return loss, mae

    train_loss = val_loss = val_mae = float("nan")
    for epoch in range(epochs):
        model.train()
        order = rng.permutation(train_idx)
        total = 0.0
        for start in range(0, len(order), batch_size):
            batch = torch.from_numpy(order[start:start + batch_size])
            loss = torch.mean((torch.sigmoid(model(stm[batch], nstm[batch])) - y[batch]) ** 2)
            optimizer.zero_grad()
            loss.backward()
            optimizer.step()
            model.clip_weights()
            total += loss.item() * len(batch)
        scheduler.step()
        train_loss = total / len(order)
        val_loss, val_mae = val_metrics()
        log(f"epoch {epoch + 1:3}/{epochs}  train {train_loss:.5f}  val {val_loss:.5f}  val MAE {val_mae:6.1f} cp")

    # References on the same validation positions: always 0.5, and material balance alone
    y_val = targets[val_idx]
    material_prob = 1.0 / (1.0 + np.exp(-material[val_idx] / WDL_SCALE))
    metrics = {
        "positions": int(len(targets)),
        "train_loss": train_loss,
        "val_loss": val_loss,
        "val_mae_cp": val_mae,
        "constant_val_loss": float(np.mean((0.5 - y_val) ** 2)),
        "material_val_loss": float(np.mean((material_prob - y_val) ** 2)),
        "material_mae_cp": float(np.mean(np.abs(material[val_idx] - eval_cp[val_idx]))),
    }
    return model, metrics


def main() -> None:
    parser = argparse.ArgumentParser(description="Train an NNUE evaluation network and export it for the engine")
    parser.add_argument("labelled", help="Parquet with fen, eval_cp, result, side_to_move, game_id")
    parser.add_argument("out", help="output network file, e.g. net.nnue")
    parser.add_argument("--hidden", type=int, default=128)
    parser.add_argument("--l1", type=int, default=32)
    parser.add_argument("--epochs", type=int, default=20)
    parser.add_argument("--batch-size", type=int, default=4096)
    parser.add_argument("--lr", type=float, default=1e-3)
    parser.add_argument("--lambda", dest="lam", type=float, default=0.9, help="weight of eval vs game result")
    parser.add_argument("--max-rows", type=int)
    parser.add_argument("--seed", type=int, default=0)
    parser.add_argument("--runs-dir", default=str(Path("experiments") / "runs"))
    args = parser.parse_args()

    model, metrics = train(args.labelled, hidden=args.hidden, l1=args.l1, epochs=args.epochs,
                           batch_size=args.batch_size, lr=args.lr, lam=args.lam, seed=args.seed,
                           max_rows=args.max_rows)
    export(model, args.out)
    print({k: round(v, 5) if isinstance(v, float) else v for k, v in metrics.items()})
    params = {k: v for k, v in vars(args).items() if k != "runs_dir"}
    print("exported", args.out, "| recorded", record_run("nnue", params, metrics, args.runs_dir))


if __name__ == "__main__":
    main()
