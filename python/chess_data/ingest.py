"""PGN -> Parquet position dataset. Usage: python -m chess_data.ingest games.pgn positions.parquet"""
from __future__ import annotations

import argparse
import time
from itertools import islice
from pathlib import Path

import pyarrow as pa
import pyarrow.parquet as pq

from .pgn import IngestStats, iter_games, iter_positions, open_pgn

SCHEMA = pa.schema([
    ("game_id", pa.int64()),
    ("ply", pa.int32()),
    ("fen", pa.string()),
    ("side_to_move", pa.int8()),
    ("move", pa.string()),
    ("result", pa.float32()),
    ("white", pa.string()),
    ("black", pa.string()),
    ("white_elo", pa.int32()),
    ("black_elo", pa.int32()),
    ("eco", pa.string()),
])


def ingest(
    pgn_path: str | Path,
    out_path: str | Path,
    *,
    min_elo: int = 0,
    max_games: int | None = None,
    skip_plies: int = 0,
    every: int = 1,
    batch_rows: int = 100_000,
) -> IngestStats:
    """Stream games into a Parquet file, one row group per batch, at constant memory."""
    stats = IngestStats()
    rows: list[dict] = []
    with open_pgn(pgn_path) as stream, pq.ParquetWriter(out_path, SCHEMA) as writer:
        for game_id, game in enumerate(islice(iter_games(stream, stats, min_elo), max_games)):
            rows.extend(iter_positions(game, game_id, skip_plies, every))
            if len(rows) >= batch_rows:
                writer.write_table(pa.Table.from_pylist(rows, SCHEMA))
                stats.positions += len(rows)
                rows.clear()
        if rows:
            writer.write_table(pa.Table.from_pylist(rows, SCHEMA))
            stats.positions += len(rows)
    return stats


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pgn", help="input .pgn, .pgn.gz, .pgn.bz2 or .pgn.zst")
    parser.add_argument("out", help="output .parquet")
    parser.add_argument("--min-elo", type=int, default=0, help="drop games where either player is below this")
    parser.add_argument("--max-games", type=int)
    parser.add_argument("--skip-plies", type=int, default=0, help="ignore the first N plies of each game")
    parser.add_argument("--every", type=int, default=1, help="keep every Nth position after --skip-plies")
    args = parser.parse_args()

    start = time.perf_counter()
    stats = ingest(args.pgn, args.out, min_elo=args.min_elo, max_games=args.max_games,
                   skip_plies=args.skip_plies, every=args.every)
    secs = time.perf_counter() - start
    print(f"games read {stats.games_read}, kept {stats.games_kept}, positions {stats.positions} "
          f"in {secs:.1f}s ({stats.games_read / max(secs, 1e-9):.0f} games/s)")
    if stats.skipped:
        print("skipped:", dict(stats.skipped))


if __name__ == "__main__":
    main()
