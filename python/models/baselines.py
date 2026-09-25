"""Classical baselines predicting the game result (White's score) from position features.

Usage: python -m models.baselines features.parquet [--models linear random_forest xgboost]
"""
from __future__ import annotations

import argparse
import time
from pathlib import Path

import numpy as np
import pyarrow.parquet as pq
from sklearn.dummy import DummyRegressor
from sklearn.ensemble import RandomForestRegressor
from sklearn.linear_model import Ridge
from sklearn.metrics import mean_absolute_error, mean_squared_error, r2_score
from sklearn.model_selection import GroupShuffleSplit
from sklearn.neural_network import MLPRegressor
from sklearn.pipeline import make_pipeline
from sklearn.preprocessing import StandardScaler

from experiments.tracking import record_run
from features.extract import FEATURE_NAMES


def load_features(path: str | Path, max_rows: int | None = None) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    table = pq.read_table(path, columns=FEATURE_NAMES + ["result", "game_id"])
    if max_rows is not None:
        table = table.slice(0, max_rows)
    x = np.column_stack([table.column(n).to_numpy() for n in FEATURE_NAMES]).astype(np.float32)
    return x, table.column("result").to_numpy().astype(np.float32), table.column("game_id").to_numpy()


def split_by_game(groups: np.ndarray, test_size: float = 0.2, seed: int = 0) -> tuple[np.ndarray, np.ndarray]:
    """Positions from one game never land on both sides of the split (avoids leakage)."""
    splitter = GroupShuffleSplit(n_splits=1, test_size=test_size, random_state=seed)
    return next(splitter.split(groups, groups=groups))


def make_models(seed: int = 0) -> dict:
    models = {
        "constant": DummyRegressor(),
        "linear": make_pipeline(StandardScaler(), Ridge(alpha=1.0)),
        "random_forest": RandomForestRegressor(n_estimators=200, min_samples_leaf=20, n_jobs=-1, random_state=seed),
        "mlp": make_pipeline(StandardScaler(), MLPRegressor(hidden_layer_sizes=(64, 32), early_stopping=True,
                                                            max_iter=200, random_state=seed)),
    }
    try:
        from xgboost import XGBRegressor

        models["xgboost"] = XGBRegressor(n_estimators=400, max_depth=6, learning_rate=0.05,
                                         subsample=0.8, colsample_bytree=0.8, random_state=seed)
    except ImportError:
        pass
    return models


def evaluate(model, x: np.ndarray, y: np.ndarray) -> dict[str, float]:
    pred = np.clip(model.predict(x), 0.0, 1.0)
    decisive = y != 0.5
    return {
        "mse": float(mean_squared_error(y, pred)),
        "mae": float(mean_absolute_error(y, pred)),
        "r2": float(r2_score(y, pred)),
        # Of decisive games, how often the model picks the winner
        "winner_accuracy": float(np.mean((pred[decisive] > 0.5) == (y[decisive] == 1.0))) if decisive.any() else 0.0,
    }


def run_baselines(path: str | Path, names: list[str] | None = None, test_size: float = 0.2, seed: int = 0,
                  max_rows: int | None = None) -> list[dict]:
    x, y, groups = load_features(path, max_rows)
    train, test = split_by_game(groups, test_size, seed)
    models = make_models(seed)
    results = []
    for name in names or list(models):
        if name not in models:
            raise ValueError(f"unknown or unavailable model {name!r}; choose from {sorted(models)}")
        start = time.perf_counter()
        models[name].fit(x[train], y[train])
        metrics = evaluate(models[name], x[test], y[test])
        results.append({"model": name, "train_seconds": round(time.perf_counter() - start, 2), **metrics})
    return results


def main() -> None:
    parser = argparse.ArgumentParser(description="Train and compare classical result-prediction baselines")
    parser.add_argument("features")
    parser.add_argument("--models", nargs="+")
    parser.add_argument("--test-size", type=float, default=0.2)
    parser.add_argument("--seed", type=int, default=0)
    parser.add_argument("--max-rows", type=int)
    parser.add_argument("--runs-dir", default=str(Path("experiments") / "runs"))
    args = parser.parse_args()

    results = run_baselines(args.features, args.models, args.test_size, args.seed, args.max_rows)
    print(f"{'model':14} {'mse':>8} {'mae':>8} {'r2':>8} {'winner_acc':>10} {'train_s':>8}")
    for r in results:
        print(f"{r['model']:14} {r['mse']:8.4f} {r['mae']:8.4f} {r['r2']:8.4f} {r['winner_accuracy']:10.3f} "
              f"{r['train_seconds']:8.2f}")
    params = {"features": str(args.features), "test_size": args.test_size, "seed": args.seed,
              "max_rows": args.max_rows, "feature_names": FEATURE_NAMES}
    print("recorded", record_run("baselines", params, results, args.runs_dir))


if __name__ == "__main__":
    main()
