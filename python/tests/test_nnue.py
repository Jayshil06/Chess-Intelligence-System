import random
import re
import subprocess
import tempfile
import unittest
from pathlib import Path

import chess
import numpy as np
import pyarrow as pa
import pyarrow.parquet as pq

try:
    import torch
except ImportError as exc:  # The ml extra is optional
    raise unittest.SkipTest("torch is not installed") from exc

from test_selfplay import find_engine, require_engine

from chess_data.label import EVAL_CLIP, label
from models.nnue import NNUE, NUM_FEATURES, PAD, encode, encode_fens, export, feature_index, predict_cp, train


def random_positions(count: int, seed: int = 0) -> list[str]:
    """Positions from random play, so material imbalances are common."""
    rng, fens = random.Random(seed), []
    while len(fens) < count:
        board = chess.Board()
        for _ in range(rng.randint(10, 80)):
            moves = list(board.legal_moves)
            if not moves:
                break
            board.push(rng.choice(moves))
        if not board.is_game_over():
            fens.append(board.fen())
    return fens


class TestEncoding(unittest.TestCase):
    def test_feature_index_matches_engine_layout(self):
        self.assertEqual(feature_index(chess.Piece(chess.PAWN, chess.WHITE), chess.A2, chess.WHITE), 8)
        self.assertEqual(feature_index(chess.Piece(chess.PAWN, chess.BLACK), chess.A7, chess.BLACK), 8)
        self.assertEqual(feature_index(chess.Piece(chess.PAWN, chess.WHITE), chess.A2, chess.BLACK), 6 * 64 + 48)
        self.assertEqual(feature_index(chess.Piece(chess.KING, chess.BLACK), chess.H8, chess.WHITE), 11 * 64 + 63)

    def test_views_are_colour_symmetric(self):
        board = chess.Board("r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4")
        us, them = encode(board)
        mirror_us, mirror_them = encode(board.mirror())
        self.assertEqual(sorted(us), sorted(mirror_us))
        self.assertEqual(sorted(them), sorted(mirror_them))
        self.assertTrue(all(0 <= i < NUM_FEATURES for i in us + them))

    def test_padding_and_material(self):
        stm, nstm, material = encode_fens(["4k3/8/8/8/8/8/8/3QK3 b - - 0 1"])
        self.assertEqual(int((stm[0] != PAD).sum()), 3)
        self.assertEqual(material[0], -900)  # Black to move, White has the queen


class TestTraining(unittest.TestCase):
    def test_learns_material_and_beats_constant(self):
        fens = random_positions(3000)
        _, _, material = encode_fens(fens)
        boards = [chess.Board(f) for f in fens]
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "labelled.parquet"
            pq.write_table(pa.table({
                "fen": fens,
                "eval_cp": np.clip(material, -EVAL_CLIP, EVAL_CLIP).astype(np.int16),
                "result": np.full(len(fens), 0.5, dtype=np.float32),
                "side_to_move": np.array([0 if b.turn else 1 for b in boards], dtype=np.int8),
                "game_id": np.arange(len(fens)),
            }), path)
            _, metrics = train(path, hidden=32, l1=8, epochs=25, batch_size=256, lr=3e-3, lam=1.0,
                               log=lambda _: None)
        self.assertLess(metrics["val_loss"], metrics["constant_val_loss"] * 0.5)
        self.assertLess(metrics["val_mae_cp"], 150)


class TestExport(unittest.TestCase):
    def test_file_layout(self):
        model = NNUE(hidden=16, l1=4)
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "net.nnue"
            export(model, path)
            data = path.read_bytes()
        expected = 4 + 12 + (NUM_FEATURES * 16 + 16) * 2 + (4 * 32 + 4 + 4 + 1) * 4
        self.assertEqual(len(data), expected)
        self.assertEqual(data[:4], b"CINN")

    def test_engine_matches_python_forward_pass(self):
        engine = require_engine()
        torch.manual_seed(3)
        model = NNUE(hidden=32, l1=8)
        with torch.no_grad():  # Spread outputs so the comparison is meaningful
            model.out.weight.mul_(20)
        fens = random_positions(12, seed=5)
        expected = predict_cp(model, fens)
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "net with space.nnue"
            export(model, path)
            commands = [f"setoption name EvalFile value {path}"]
            for fen in fens:
                commands += [f"position fen {fen}", "eval"]
            out = subprocess.run([engine], input="\n".join(commands + ["quit"]) + "\n",
                                 capture_output=True, text=True, timeout=60).stdout
        self.assertIn("loaded network", out)
        got = [int(m) for m in re.findall(r"Evaluation: (-?\d+) \(side to move, nnue\)", out)]
        self.assertEqual(len(got), len(fens))
        self.assertGreater(float(np.std(expected)), 20)
        for fen, py, cpp in zip(fens, expected, got, strict=True):
            self.assertAlmostEqual(cpp, py, delta=4, msg=fen)  # int16 quantization of the first layer


@unittest.skipUnless(find_engine(), "build the engine or set CHESS_ENGINE to run labelling tests")
class TestLabel(unittest.TestCase):
    def test_labels_from_side_to_move_view(self):
        fens = [chess.STARTING_FEN,
                "rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3",  # White is checkmated
                "4k3/8/8/8/8/8/8/3QK3 w - - 0 1"]
        with tempfile.TemporaryDirectory() as tmp:
            src, out = Path(tmp) / "positions.parquet", Path(tmp) / "labelled.parquet"
            pq.write_table(pa.table({"fen": fens, "game_id": [0, 1, 2]}), src)
            self.assertEqual(label(src, out, require_engine(), depth=3, workers=2), 3)
            evals = pq.read_table(out).column("eval_cp").to_pylist()
        self.assertLess(abs(evals[0]), 100)
        self.assertEqual(evals[1], -EVAL_CLIP)
        self.assertGreater(evals[2], 500)


if __name__ == "__main__":
    unittest.main()
