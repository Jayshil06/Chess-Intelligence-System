"""Game-level analytics over ingested position Parquet files, computed with DuckDB.

Usage: python -m analytics.report positions.parquet [--features features.parquet]
"""
from __future__ import annotations

import argparse
from pathlib import Path

import duckdb
import polars as pl

from features.extract import FEATURE_NAMES


def _connect(positions: str | Path) -> duckdb.DuckDBPyConnection:
    """One row per game, rebuilt from position rows so sampled datasets still work."""
    con = duckdb.connect()
    con.read_parquet(str(positions)).create_view("positions")
    con.execute("""
        create temp view games as
        select game_id, any_value(eco) as eco, any_value(result) as result,
               any_value(white) as white, any_value(black) as black,
               any_value(white_elo) as white_elo, any_value(black_elo) as black_elo
        from positions group by game_id
    """)
    return con


def opening_stats(positions: str | Path, min_games: int = 20) -> pl.DataFrame:
    return _connect(positions).execute("""
        select eco, count(*) as games,
               round(avg(result), 3) as white_score,
               round(avg((result = 0.5)::int), 3) as draw_rate,
               round(avg((white_elo + black_elo) / 2), 0) as avg_elo
        from games where eco is not null
        group by eco having count(*) >= ?
        order by games desc, eco
    """, [min_games]).pl()


def elo_calibration(positions: str | Path, bucket: int = 100) -> pl.DataFrame:
    """Actual White score against the Elo-predicted score, by rating-difference bucket."""
    return _connect(positions).execute("""
        select floor((white_elo - black_elo) / ?) * ? as elo_diff, count(*) as games,
               round(avg(result), 3) as actual_score,
               round(avg(1 / (1 + pow(10, -(white_elo - black_elo) / 400))), 3) as expected_score
        from games where white_elo is not null and black_elo is not null
        group by 1 order by 1
    """, [bucket, bucket]).pl()


def player_stats(positions: str | Path, min_games: int = 10) -> pl.DataFrame:
    """Per-player results and most-played opening, combining games as White and as Black."""
    return _connect(positions).execute("""
        with seats as (
            select white as player, result as score, black_elo as opp_elo, eco from games
            union all
            select black, 1 - result, white_elo, eco from games
        )
        select player, count(*) as games, round(avg(score), 3) as score,
               round(avg((score = 0.5)::int), 3) as draw_rate,
               round(avg(opp_elo), 0) as avg_opponent_elo, mode(eco) as favourite_eco
        from seats where player is not null
        group by player having count(*) >= ?
        order by games desc, player
    """, [min_games]).pl()


def feature_correlations(features: str | Path) -> pl.DataFrame:
    """Pearson correlation of each feature with the game result (White's score)."""
    con = duckdb.connect()
    con.read_parquet(str(features)).create_view("features")
    selects = ", ".join(f"corr({name}, result) as {name}" for name in FEATURE_NAMES)
    row = con.execute(f"select {selects} from features").fetchone()
    return (pl.DataFrame({"feature": FEATURE_NAMES, "corr_with_result": row})
            .with_columns(pl.col("corr_with_result").round(3))
            .sort(pl.col("corr_with_result").abs(), descending=True, nulls_last=True))


def main() -> None:
    parser = argparse.ArgumentParser(description="Print opening, rating, player and feature analytics")
    parser.add_argument("positions")
    parser.add_argument("--features", help="features Parquet for result correlations")
    parser.add_argument("--min-games", type=int, default=20)
    args = parser.parse_args()

    pl.Config.set_tbl_rows(25)
    pl.Config.set_tbl_formatting("ASCII_MARKDOWN")
    print("## Openings\n", opening_stats(args.positions, args.min_games).head(25))
    print("\n## Elo calibration\n", elo_calibration(args.positions))
    print("\n## Players\n", player_stats(args.positions, args.min_games).head(25))
    if args.features:
        print("\n## Feature correlation with result\n", feature_correlations(args.features))


if __name__ == "__main__":
    main()
