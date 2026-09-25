import tempfile
import unittest
from pathlib import Path

import chess
import pyarrow as pa
import pyarrow.parquet as pq

from chess_data.ingest import ingest
from features.extract import FEATURE_NAMES, board_features, featurize


def features(fen: str) -> dict[str, int]:
    return board_features(chess.Board(fen))


class TestFeatures(unittest.TestCase):
    def test_start_position_is_balanced(self):
        f = board_features(chess.Board())
        self.assertEqual(set(f), set(FEATURE_NAMES))
        balanced = {k: v for k, v in f.items() if k not in ("phase", "side_to_move", "in_check")}
        self.assertTrue(all(v == 0 for v in balanced.values()), balanced)
        self.assertEqual(f["phase"], 24)

    def test_colour_mirror_negates_differential_features(self):
        board = chess.Board("r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4")
        f, g = board_features(board), board_features(board.mirror())
        for name in FEATURE_NAMES:
            if name == "side_to_move":
                self.assertEqual(f[name], 1 - g[name])
            elif name in ("phase", "in_check"):
                self.assertEqual(f[name], g[name])
            else:
                self.assertEqual(f[name], -g[name], name)

    def test_material(self):
        f = features("rnb1kbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")
        self.assertEqual((f["material_queen"], f["material_cp"], f["phase"]), (1, 900, 20))

    def test_bishop_pair(self):
        self.assertEqual(features("4k3/8/8/8/8/8/8/2B1KB2 w - - 0 1")["bishop_pair"], 1)

    def test_pawn_structure(self):
        # White: doubled, isolated c-pawns and an isolated a-pawn. Every pawn here is passed
        f = features("4k3/5ppp/8/P7/2P5/2P5/8/4K3 w - - 0 1")
        self.assertEqual(f["doubled_pawns"], 1)
        self.assertEqual(f["isolated_pawns"], 3)
        self.assertEqual(f["passed_pawns"], 0)

    def test_king_safety(self):
        # Castled white king with a full shield; the d4 queen hits g7 and h8 next to the bare black king
        f = features("6k1/8/8/8/3Q4/8/5PPP/6K1 w - - 0 1")
        self.assertEqual(f["king_shield"], 3)
        self.assertGreater(f["king_zone_attacks"], 0)

    def test_check_and_side_to_move(self):
        f = features("4k3/8/8/8/8/8/8/4K2r w - - 0 1")
        self.assertEqual((f["in_check"], f["side_to_move"]), (1, 0))

    def test_featurize_file(self):
        with tempfile.TemporaryDirectory() as tmp:
            src, out = Path(tmp) / "positions.parquet", Path(tmp) / "features.parquet"
            fens = [chess.Board().fen(), "rnb1kbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"]
            pq.write_table(pa.table({"fen": fens, "result": [0.5, 1.0]}), src)
            self.assertEqual(featurize(src, out, batch_rows=1), 2)
            table = pq.read_table(out)
            self.assertEqual(table.column_names, ["fen", "result"] + FEATURE_NAMES)
            self.assertEqual(table.column("material_cp").to_pylist(), [0, 900])

    def test_pipeline_from_ingested_pgn(self):
        with tempfile.TemporaryDirectory() as tmp:
            pgn, positions, out = (Path(tmp) / n for n in ("g.pgn", "positions.parquet", "features.parquet"))
            pgn.write_text('[Result "1-0"]\n\n1. e4 e5 2. Qh5 Nc6 3. Bc4 Nf6 4. Qxf7# 1-0\n', encoding="utf-8")
            ingest(pgn, positions)
            self.assertEqual(featurize(positions, out), 7)
            table = pq.read_table(out)
            self.assertEqual(table.column_names.count("side_to_move"), 1)
            self.assertTrue(set(FEATURE_NAMES) <= set(table.column_names))
            self.assertEqual(table.column("material_cp").to_pylist()[-1], 0)  # Before Qxf7#


if __name__ == "__main__":
    unittest.main()
