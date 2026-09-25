"""Deterministic hand-crafted position features. Differential features are White minus Black."""
from __future__ import annotations

import argparse
from pathlib import Path

import chess
import pyarrow as pa
import pyarrow.parquet as pq

PIECE_VALUES = {chess.PAWN: 100, chess.KNIGHT: 320, chess.BISHOP: 330, chess.ROOK: 500, chess.QUEEN: 900}
PHASE_WEIGHTS = {chess.KNIGHT: 1, chess.BISHOP: 1, chess.ROOK: 2, chess.QUEEN: 4}
CENTER = chess.BB_D4 | chess.BB_E4 | chess.BB_D5 | chess.BB_E5

FEATURE_NAMES = [
    "material_pawn", "material_knight", "material_bishop", "material_rook", "material_queen",
    "material_cp", "bishop_pair", "mobility", "center_control",
    "doubled_pawns", "isolated_pawns", "passed_pawns",
    "king_shield", "king_zone_attacks",
    "phase", "side_to_move", "in_check",
]


def _adjacent_files(f: int) -> int:
    return (chess.BB_FILES[f - 1] if f > 0 else 0) | (chess.BB_FILES[f + 1] if f < 7 else 0)


def _ranks_ahead(color: chess.Color, rank: int) -> int:
    ranks = chess.BB_RANKS[rank + 1:] if color == chess.WHITE else chess.BB_RANKS[:rank]
    mask = 0
    for r in ranks:
        mask |= r
    return mask


def _pawn_structure(board: chess.Board, color: chess.Color) -> tuple[int, int, int]:
    pawns = board.pieces_mask(chess.PAWN, color)
    enemy = board.pieces_mask(chess.PAWN, not color)
    doubled = isolated = passed = 0
    for f in range(8):
        count = (pawns & chess.BB_FILES[f]).bit_count()
        if count > 1:
            doubled += count - 1
        if count and not pawns & _adjacent_files(f):
            isolated += count
    for sq in chess.scan_forward(pawns):
        f = chess.square_file(sq)
        span = (chess.BB_FILES[f] | _adjacent_files(f)) & _ranks_ahead(color, chess.square_rank(sq))
        if not enemy & span:
            passed += 1
    return doubled, isolated, passed


def _king_safety(board: chess.Board, color: chess.Color) -> tuple[int, int]:
    """(own pawns shielding the king, enemy attacks landing on the king zone)."""
    king = board.king(color)
    if king is None:
        return 0, 0
    f, r = chess.square_file(king), chess.square_rank(king)
    near_ranks = 0
    for step in (1, 2):
        rank = r + step if color == chess.WHITE else r - step
        if 0 <= rank < 8:
            near_ranks |= chess.BB_RANKS[rank]
    shield = (board.pieces_mask(chess.PAWN, color) & (chess.BB_FILES[f] | _adjacent_files(f)) & near_ranks).bit_count()
    zone = chess.BB_KING_ATTACKS[king] | chess.BB_SQUARES[king]
    attacks = sum(board.attackers_mask(not color, sq).bit_count() for sq in chess.scan_forward(zone))
    return shield, attacks


def _mobility(board: chess.Board, color: chess.Color) -> int:
    own = board.occupied_co[color]
    total = 0
    for pt in (chess.KNIGHT, chess.BISHOP, chess.ROOK, chess.QUEEN):
        for sq in chess.scan_forward(board.pieces_mask(pt, color)):
            total += (board.attacks_mask(sq) & ~own).bit_count()
    return total


def board_features(board: chess.Board) -> dict[str, int]:
    f: dict[str, int] = {}
    counts = {pt: (len(board.pieces(pt, chess.WHITE)), len(board.pieces(pt, chess.BLACK))) for pt in PIECE_VALUES}
    for pt, name in zip(PIECE_VALUES, FEATURE_NAMES[:5], strict=True):
        f[name] = counts[pt][0] - counts[pt][1]
    f["material_cp"] = sum(PIECE_VALUES[pt] * (w - b) for pt, (w, b) in counts.items())
    f["bishop_pair"] = int(counts[chess.BISHOP][0] >= 2) - int(counts[chess.BISHOP][1] >= 2)
    f["mobility"] = _mobility(board, chess.WHITE) - _mobility(board, chess.BLACK)
    f["center_control"] = sum(
        board.attackers_mask(chess.WHITE, sq).bit_count() - board.attackers_mask(chess.BLACK, sq).bit_count()
        for sq in chess.scan_forward(CENTER))

    white_pawns, black_pawns = _pawn_structure(board, chess.WHITE), _pawn_structure(board, chess.BLACK)
    for name, w, b in zip(("doubled_pawns", "isolated_pawns", "passed_pawns"), white_pawns, black_pawns, strict=True):
        f[name] = w - b

    white_shield, white_danger = _king_safety(board, chess.WHITE)
    black_shield, black_danger = _king_safety(board, chess.BLACK)
    f["king_shield"] = white_shield - black_shield
    f["king_zone_attacks"] = black_danger - white_danger  # Positive when White attacks more

    f["phase"] = min(24, sum(PHASE_WEIGHTS[pt] * (w + b) for pt, (w, b) in counts.items() if pt in PHASE_WEIGHTS))
    f["side_to_move"] = 0 if board.turn == chess.WHITE else 1
    f["in_check"] = int(board.is_check())
    return f


def featurize(positions: str | Path, out: str | Path, batch_rows: int = 50_000) -> int:
    """Append feature columns to a positions Parquet file, streaming batch by batch.

    Columns the input already has (e.g. side_to_move from ingestion) are kept, not duplicated.
    """
    source = pq.ParquetFile(positions)
    names = [n for n in FEATURE_NAMES if n not in source.schema_arrow.names]
    schema = pa.unify_schemas([source.schema_arrow, pa.schema([(n, pa.int16()) for n in names])])
    rows = 0
    with pq.ParquetWriter(out, schema) as writer:
        for batch in source.iter_batches(batch_size=batch_rows):
            feats = [board_features(chess.Board(fen)) for fen in batch.column("fen").to_pylist()]
            table = pa.Table.from_batches([batch])
            for name in names:
                table = table.append_column(name, pa.array([row[name] for row in feats], pa.int16()))
            writer.write_table(table)
            rows += batch.num_rows
    return rows


def main() -> None:
    parser = argparse.ArgumentParser(description="Add feature columns to a positions Parquet file")
    parser.add_argument("positions")
    parser.add_argument("out")
    args = parser.parse_args()
    print(f"featurized {featurize(args.positions, args.out)} positions")


if __name__ == "__main__":
    main()
