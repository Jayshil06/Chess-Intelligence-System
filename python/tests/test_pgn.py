import gzip
import io
import tempfile
import unittest
from pathlib import Path

import pyarrow.parquet as pq

from chess_data.ingest import ingest
from chess_data.pgn import IngestStats, iter_games, iter_positions

PGN = """
[Event "Valid"]
[White "A"]
[Black "B"]
[Result "1-0"]
[WhiteElo "2100"]
[BlackElo "2050"]
[ECO "C20"]

1. e4 e5 2. Qh5 Nc6 3. Bc4 Nf6 4. Qxf7# 1-0

[Event "Illegal move"]
[Result "0-1"]

1. e4 e5 2. Ke3 Ke6 3. Kxe5 0-1

[Event "Unfinished"]
[Result "*"]

1. d4 d5 *

[Event "Variant"]
[Variant "Atomic"]
[Result "1/2-1/2"]

1. e4 e5 1/2-1/2

[Event "Low rated draw"]
[Result "1/2-1/2"]
[WhiteElo "1200"]
[BlackElo "1250"]

1. d4 d5 2. c4 e6 1/2-1/2
"""


class TestPgnIngestion(unittest.TestCase):
    def test_filters_invalid_games(self):
        stats = IngestStats()
        games = list(iter_games(io.StringIO(PGN), stats))
        self.assertEqual(stats.games_read, 5)
        self.assertEqual([g.headers["Event"] for g in games], ["Valid", "Low rated draw"])
        self.assertEqual(dict(stats.skipped), {"parse_error": 1, "no_result": 1, "variant": 1})

    def test_min_elo(self):
        stats = IngestStats()
        games = list(iter_games(io.StringIO(PGN), stats, min_elo=2000))
        self.assertEqual([g.headers["Event"] for g in games], ["Valid"])
        self.assertEqual(stats.skipped["low_elo"], 1)

    def test_positions_are_before_each_move(self):
        game = next(iter_games(io.StringIO(PGN)))
        rows = list(iter_positions(game, game_id=7))
        self.assertEqual(len(rows), 7)
        self.assertEqual(rows[0]["fen"], "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")
        self.assertEqual(rows[0]["move"], "e2e4")
        self.assertEqual(rows[-1]["move"], "h5f7")
        self.assertEqual(rows[1]["side_to_move"], 1)
        self.assertTrue(all(r["result"] == 1.0 and r["game_id"] == 7 and r["eco"] == "C20" for r in rows))

    def test_sampling(self):
        game = next(iter_games(io.StringIO(PGN)))
        self.assertEqual([r["ply"] for r in iter_positions(game, 0, skip_plies=2, every=2)], [2, 4, 6])

    def test_ingest_gzip_to_parquet(self):
        with tempfile.TemporaryDirectory() as tmp:
            src, out = Path(tmp) / "games.pgn.gz", Path(tmp) / "positions.parquet"
            with gzip.open(src, "wt", encoding="utf-8") as f:
                f.write(PGN)
            stats = ingest(src, out, batch_rows=3)
            table = pq.read_table(out)
            self.assertEqual(stats.games_kept, 2)
            self.assertEqual(stats.positions, 7 + 4)
            self.assertEqual(table.num_rows, 11)
            self.assertEqual(pq.ParquetFile(out).num_row_groups, 2)  # Batches flush at game boundaries
            self.assertEqual(sorted(set(table.column("game_id").to_pylist())), [0, 1])

    def test_max_games(self):
        with tempfile.TemporaryDirectory() as tmp:
            src, out = Path(tmp) / "games.pgn", Path(tmp) / "positions.parquet"
            src.write_text(PGN, encoding="utf-8")
            stats = ingest(src, out, max_games=1)
            self.assertEqual((stats.games_kept, stats.positions), (1, 7))


if __name__ == "__main__":
    unittest.main()
