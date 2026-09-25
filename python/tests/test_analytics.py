import io
import tempfile
import unittest
from pathlib import Path

import chess.engine
import chess.pgn
from test_selfplay import require_engine

from analytics.blunders import MATE_CP, find_blunders
from analytics.report import elo_calibration, feature_correlations, opening_stats, player_stats
from chess_data.ingest import ingest
from features.extract import featurize


def game_pgn(white: str, black: str, result: str, eco: str, white_elo: int, black_elo: int) -> str:
    moves = {"1-0": "1. e4 e5 2. Qh5 Nc6 3. Bc4 Nf6 4. Qxf7# 1-0",
             "0-1": "1. f3 e5 2. g4 Qh4# 0-1",
             "1/2-1/2": "1. d4 d5 2. c4 e6 1/2-1/2"}[result]
    return (f'[White "{white}"]\n[Black "{black}"]\n[Result "{result}"]\n[ECO "{eco}"]\n'
            f'[WhiteElo "{white_elo}"]\n[BlackElo "{black_elo}"]\n\n{moves}\n\n')


class TestReport(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        pgn = Path(cls.tmp.name) / "games.pgn"
        pgn.write_text(
            game_pgn("ann", "bob", "1-0", "C20", 2000, 1800) * 3
            + game_pgn("bob", "ann", "0-1", "A00", 1800, 2000) * 2
            + game_pgn("ann", "cid", "1/2-1/2", "D30", 2100, 2100), encoding="utf-8")
        cls.positions = Path(cls.tmp.name) / "positions.parquet"
        ingest(pgn, cls.positions)

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def test_opening_stats(self):
        rows = {r["eco"]: r for r in opening_stats(self.positions, min_games=1).to_dicts()}
        self.assertEqual(rows["C20"]["games"], 3)
        self.assertEqual(rows["C20"]["white_score"], 1.0)
        self.assertEqual(rows["A00"]["white_score"], 0.0)
        self.assertEqual(rows["D30"]["draw_rate"], 1.0)
        self.assertEqual(opening_stats(self.positions, min_games=3)["eco"].to_list(), ["C20"])

    def test_elo_calibration(self):
        rows = {r["elo_diff"]: r for r in elo_calibration(self.positions).to_dicts()}
        self.assertEqual(rows[200]["games"], 3)
        self.assertEqual(rows[200]["actual_score"], 1.0)
        self.assertAlmostEqual(rows[200]["expected_score"], 0.76, places=2)
        self.assertEqual(rows[-200]["actual_score"], 0.0)

    def test_player_stats(self):
        rows = {r["player"]: r for r in player_stats(self.positions, min_games=1).to_dicts()}
        self.assertEqual(rows["ann"]["games"], 6)
        self.assertAlmostEqual(rows["ann"]["score"], 5.5 / 6, places=3)
        self.assertEqual(rows["bob"]["score"], 0.0)
        self.assertEqual(rows["ann"]["favourite_eco"], "C20")

    def test_feature_correlations(self):
        features = Path(self.tmp.name) / "features.parquet"
        featurize(self.positions, features)
        table = feature_correlations(features)
        self.assertEqual(len(table), 17)
        self.assertIn("material_cp", table["feature"].to_list())


class TestBlunders(unittest.TestCase):
    engine: str

    @classmethod
    def setUpClass(cls):
        cls.engine = require_engine()

    def test_flags_the_move_that_allows_mate(self):
        game = chess.pgn.read_game(io.StringIO("1. e4 e5 2. Qh5 Nc6 3. Bc4 Nf6 4. Qxf7# 1-0"))
        assert game is not None
        with chess.engine.SimpleEngine.popen_uci(self.engine) as engine:
            blunders = find_blunders(game, engine, chess.engine.Limit(depth=4))
        by_ply = {b.ply: b for b in blunders}
        self.assertIn(5, by_ply)  # 3...Nf6?? allows Qxf7#
        self.assertEqual((by_ply[5].color, by_ply[5].san), ("black", "Nf6"))
        self.assertLessEqual(by_ply[5].eval_after, -MATE_CP + 10)
        self.assertNotIn(6, by_ply)  # The mating move itself loses nothing


if __name__ == "__main__":
    unittest.main()
