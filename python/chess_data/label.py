"""Add engine evaluations to a positions Parquet file as NNUE training targets.

Usage: python -m chess_data.label positions.parquet labelled.parquet ENGINE [--depth 6] [--workers 8]
"""
from __future__ import annotations

import argparse
import time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

import chess
import chess.engine
import pyarrow as pa
import pyarrow.parquet as pq

from analytics.blunders import position_eval
from selfplay.match import engine_command

EVAL_CLIP = 3000  # Mates and overwhelming advantages are capped so they do not dominate training


def _evaluate_all(engine: chess.engine.SimpleEngine, fens: list[str], limit: chess.engine.Limit) -> list[int]:
    return [max(-EVAL_CLIP, min(EVAL_CLIP, position_eval(chess.Board(fen), engine, limit))) for fen in fens]


def label(positions: str | Path, out: str | Path, engine_cmd: str, *, depth: int = 6, workers: int = 8,
          batch_rows: int = 20_000) -> int:
    """Stream batches through a pool of engine processes; eval_cp is from the side to move's view."""
    source = pq.ParquetFile(positions)
    schema = source.schema_arrow.append(pa.field("eval_cp", pa.int16()))
    limit = chess.engine.Limit(depth=depth)
    engines = [chess.engine.SimpleEngine.popen_uci(engine_command(engine_cmd)) for _ in range(workers)]
    rows = 0
    try:
        with ThreadPoolExecutor(workers) as pool, pq.ParquetWriter(out, schema) as writer:
            for batch in source.iter_batches(batch_size=batch_rows):
                fens = batch.column("fen").to_pylist()
                chunks = [fens[i::workers] for i in range(workers)]
                evals = [0] * len(fens)
                for i, chunk_evals in enumerate(pool.map(_evaluate_all, engines, chunks, [limit] * workers)):
                    evals[i::workers] = chunk_evals
                table = pa.Table.from_batches([batch]).append_column("eval_cp", pa.array(evals, pa.int16()))
                writer.write_table(table)
                rows += len(fens)
    finally:
        for engine in engines:
            engine.quit()
    return rows


def main() -> None:
    parser = argparse.ArgumentParser(description="Label positions with engine evaluations")
    parser.add_argument("positions")
    parser.add_argument("out")
    parser.add_argument("engine")
    parser.add_argument("--depth", type=int, default=6)
    parser.add_argument("--workers", type=int, default=8)
    args = parser.parse_args()
    start = time.perf_counter()
    rows = label(args.positions, args.out, args.engine, depth=args.depth, workers=args.workers)
    secs = time.perf_counter() - start
    print(f"labelled {rows} positions in {secs:.1f}s ({rows / max(secs, 1e-9):.0f} positions/s)")


if __name__ == "__main__":
    main()
