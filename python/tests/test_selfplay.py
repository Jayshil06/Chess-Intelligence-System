import math
import os
import unittest
from pathlib import Path

import chess
import chess.engine

from selfplay.elo import elo_from_score, estimate_elo, expected_score, sprt_bounds, sprt_decision, sprt_llr
from selfplay.match import DEFAULT_OPENINGS, TimeControl, opening_fen, run_match

REPO = Path(__file__).resolve().parents[2]


def find_engine() -> str | None:
    """CHESS_ENGINE, or a chess_engine binary from a local build directory."""
    if os.environ.get("CHESS_ENGINE"):
        return os.environ["CHESS_ENGINE"]
    for build in ("build_release", "build"):
        for name in ("chess_engine.exe", "chess_engine"):
            path = REPO / build / "engine" / name
            if path.exists():
                return str(path)
    return None


class TestElo(unittest.TestCase):
    def test_score_elo_roundtrip(self):
        self.assertEqual(elo_from_score(0.5), 0.0)
        self.assertAlmostEqual(elo_from_score(0.75), 190.85, places=2)
        self.assertAlmostEqual(expected_score(elo_from_score(0.64)), 0.64)
        self.assertEqual(elo_from_score(1.0), math.inf)

    def test_estimate_has_symmetric_sign_and_interval(self):
        up, down = estimate_elo(60, 20, 20), estimate_elo(20, 20, 60)
        self.assertAlmostEqual(up.elo, -down.elo)
        self.assertLess(up.low, up.elo)
        self.assertLess(up.elo, up.high)
        with self.assertRaises(ValueError):
            estimate_elo(0, 0, 0)

    def test_sprt(self):
        lower, upper = sprt_bounds(0.05, 0.05)
        self.assertAlmostEqual(upper, math.log(19))
        self.assertAlmostEqual(lower, -math.log(19))
        self.assertEqual(sprt_llr(0, 0, 0, 0, 5), 0.0)
        self.assertEqual(sprt_decision(600, 300, 300, 0, 5), "H1")
        self.assertEqual(sprt_decision(300, 300, 600, 0, 5), "H0")
        self.assertIsNone(sprt_decision(5, 5, 4, 0, 5))


class TestMatchSetup(unittest.TestCase):
    def test_default_openings_are_legal(self):
        for line in DEFAULT_OPENINGS:
            self.assertTrue(chess.Board(opening_fen(line)).is_valid(), line)

    def test_time_control_parse(self):
        self.assertEqual(TimeControl.parse("10+0.1"), TimeControl(10.0, 0.1))
        self.assertEqual(TimeControl.parse("60"), TimeControl(60.0, 0.0))


def require_engine() -> str:
    engine = find_engine()
    if engine is None:
        raise unittest.SkipTest("build the engine or set CHESS_ENGINE to run engine tests")
    return engine


class TestMatchWithEngine(unittest.TestCase):
    engine: str

    @classmethod
    def setUpClass(cls):
        cls.engine = require_engine()

    def test_fixed_depth_match_completes_legally(self):
        engine = self.engine
        result = run_match(engine, engine, games=2, limit=chess.engine.Limit(depth=2))
        self.assertEqual(result.played, 2)
        for game in result.games:
            self.assertNotIn(game.headers["Termination"], {"time forfeit", "illegal move"})
            board = game.board()
            for move in game.mainline_moves():
                self.assertIn(move, board.legal_moves)
                board.push(move)

    def test_unpaired_games_use_distinct_openings(self):
        engine = self.engine
        openings = ["e4 e5", "d4 d5"]
        paired = run_match(engine, engine, games=2, openings=openings, limit=chess.engine.Limit(depth=1))
        unpaired = run_match(engine, engine, games=2, openings=openings, paired=False,
                             limit=chess.engine.Limit(depth=1))
        self.assertEqual(paired.games[0].headers["FEN"], paired.games[1].headers["FEN"])
        self.assertNotEqual(unpaired.games[0].headers["FEN"], unpaired.games[1].headers["FEN"])

    def test_clock_match_has_no_forfeit(self):
        engine = self.engine
        result = run_match(engine, engine, games=2, tc=TimeControl(1.0, 0.02))
        self.assertEqual(result.played, 2)
        self.assertFalse(any(g.headers["Termination"] in ("time forfeit", "illegal move") for g in result.games))


if __name__ == "__main__":
    unittest.main()
