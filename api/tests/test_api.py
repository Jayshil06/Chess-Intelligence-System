import unittest

import chess
from fastapi.testclient import TestClient

from app.engine import find_engine
from app.main import app

MATE_IN_ONE = "6k1/5ppp/8/8/8/8/5PPP/3R2K1 w - - 0 1"  # Rd8#


class ApiTest(unittest.TestCase):
    client: TestClient

    @classmethod
    def setUpClass(cls):
        try:
            find_engine()
        except FileNotFoundError as exc:
            raise unittest.SkipTest("build the engine or set CHESS_ENGINE to run API tests") from exc
        cls.ctx = TestClient(app)
        cls.client = cls.ctx.__enter__()

    @classmethod
    def tearDownClass(cls):
        cls.ctx.__exit__(None, None, None)

    def new_game(self, **body) -> dict:
        response = self.client.post("/api/games", json={"level": 1, **body})
        self.assertEqual(response.status_code, 201, response.text)
        return response.json()

    def test_health_and_engine_info(self):
        self.assertEqual(self.client.get("/api/health").json()["status"], "ok")
        info = self.client.get("/api/engine").json()
        self.assertEqual(info["evaluation"], "classical")
        self.assertEqual(len(info["levels"]), 8)
        self.assertEqual((info["levels"]["1"], info["levels"]["8"]), ("depth 1", "1 s per move"))

    def test_play_as_white(self):
        game = self.new_game()
        self.assertEqual((game["moves"], game["turn"], len(game["legal_moves"])), ([], "white", 20))
        game = self.client.post(f"/api/games/{game['id']}/moves", json={"move": "e2e4"}).json()
        self.assertEqual(game["moves"][0], {"uci": "e2e4", "san": "e4"})
        self.assertEqual(len(game["moves"]), 2)  # The engine replied
        self.assertEqual(game["turn"], "white")
        self.assertIsNotNone(game["evaluation"])
        self.assertIn(chess.Move.from_uci(game["moves"][1]["uci"]),
                      chess.Board("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1").legal_moves)

    def test_engine_opens_when_player_is_black(self):
        game = self.new_game(player_color="black")
        self.assertEqual((len(game["moves"]), game["turn"]), (1, "black"))
        self.assertEqual(self.client.post(f"/api/games/{game['id']}/undo").status_code, 409)

    def test_rejects_bad_moves(self):
        game = self.new_game()
        url = f"/api/games/{game['id']}/moves"
        self.assertEqual(self.client.post(url, json={"move": "e2e5"}).status_code, 400)
        self.assertEqual(self.client.post(url, json={"move": "nonsense"}).status_code, 400)
        self.assertEqual(self.client.post("/api/games/9999/moves", json={"move": "e2e4"}).status_code, 404)
        self.assertEqual(self.client.post("/api/games", json={"level": 9}).status_code, 422)

    def test_checkmate_ends_the_game(self):
        game = self.new_game(fen=MATE_IN_ONE)
        game = self.client.post(f"/api/games/{game['id']}/moves", json={"move": "d1d8"}).json()
        self.assertEqual((game["status"], game["result"], game["legal_moves"]), ("checkmate", "1-0", []))
        self.assertEqual(self.client.post(f"/api/games/{game['id']}/moves", json={"move": "g1f1"}).status_code, 409)

    def test_undo_takes_back_move_and_reply(self):
        game = self.new_game()
        self.client.post(f"/api/games/{game['id']}/moves", json={"move": "d2d4"})
        game = self.client.post(f"/api/games/{game['id']}/undo").json()
        self.assertEqual((game["moves"], game["turn"]), ([], "white"))
        self.assertEqual(self.client.post(f"/api/games/{game['id']}/undo").status_code, 409)

    def test_resign_and_pgn(self):
        game = self.new_game()
        self.client.post(f"/api/games/{game['id']}/moves", json={"move": "e2e4"})
        game = self.client.post(f"/api/games/{game['id']}/resign").json()
        self.assertEqual((game["status"], game["result"]), ("resigned", "0-1"))
        pgn = self.client.get(f"/api/games/{game['id']}/pgn").text
        self.assertIn("1. e4", pgn)
        self.assertIn('[Result "0-1"]', pgn)
        self.assertIn(game["id"], [g["id"] for g in self.client.get("/api/games").json()])

    def test_analyze(self):
        result = self.client.post("/api/analyze", json={"fen": chess.STARTING_FEN, "depth": 5}).json()
        self.assertEqual(result["status"], "active")
        self.assertIn(chess.Move.from_uci(result["best_move"]["uci"]), chess.Board().legal_moves)
        self.assertGreaterEqual(result["depth"], 5)
        self.assertEqual(result["pv"][0], result["best_move"])

        mate = self.client.post("/api/analyze", json={"fen": MATE_IN_ONE, "depth": 4}).json()
        self.assertEqual((mate["best_move"]["uci"], mate["score"]["mate"]), ("d1d8", 1))

        over = self.client.post("/api/analyze", json={"fen": "3R2k1/5ppp/8/8/8/8/5PPP/6K1 b - - 1 1"}).json()
        self.assertEqual((over["status"], over["best_move"]), ("checkmate", None))

    def test_analyze_rejects_bad_positions(self):
        self.assertEqual(self.client.post("/api/analyze", json={"fen": "not a fen"}).status_code, 400)
        self.assertEqual(self.client.post("/api/analyze", json={"fen": "8/8/8/8/8/8/8/Kk6 w - - 0 1"}).status_code, 400)


if __name__ == "__main__":
    unittest.main()
