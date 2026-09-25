import tempfile
import unittest
from pathlib import Path

import numpy as np
import pyarrow as pa
import pyarrow.parquet as pq

from experiments.tracking import load_runs, record_run
from features.extract import FEATURE_NAMES
from models.baselines import load_features, make_models, run_baselines, split_by_game


def synthetic_features(path: Path, games: int = 300, plies: int = 10, seed: int = 0) -> None:
    """Material advantage drives the result, so learned models must beat the constant baseline."""
    rng = np.random.default_rng(seed)
    material = np.repeat(rng.normal(0, 300, games), plies) + rng.normal(0, 50, games * plies)
    win_prob = 1 / (1 + np.exp(-material / 150))
    outcome = np.repeat(rng.random(games), plies)
    result = np.where(outcome < win_prob, 1.0, 0.0)
    columns: dict[str, np.ndarray] = {name: np.zeros(games * plies, dtype=np.int16) for name in FEATURE_NAMES}
    columns["material_cp"] = material.astype(np.int16)
    columns["mobility"] = rng.integers(-10, 10, games * plies).astype(np.int16)
    columns["result"] = result.astype(np.float32)
    columns["game_id"] = np.repeat(np.arange(games), plies)
    pq.write_table(pa.table(columns), path)


class TestBaselines(unittest.TestCase):
    def test_group_split_keeps_games_whole(self):
        groups = np.repeat(np.arange(50), 4)
        train, test = split_by_game(groups, 0.2, seed=1)
        self.assertFalse(set(groups[train]) & set(groups[test]))
        self.assertEqual(len(train) + len(test), len(groups))

    def test_models_beat_constant(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "features.parquet"
            synthetic_features(path)
            x, y, groups = load_features(path)
            self.assertEqual(x.shape, (3000, len(FEATURE_NAMES)))
            results = {r["model"]: r for r in run_baselines(path, ["constant", "linear", "random_forest"])}
        self.assertLess(results["linear"]["mse"], results["constant"]["mse"] * 0.8)
        self.assertLess(results["random_forest"]["mse"], results["constant"]["mse"] * 0.8)
        self.assertGreater(results["linear"]["winner_accuracy"], 0.7)

    def test_unknown_model_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "features.parquet"
            synthetic_features(path, games=20)
            with self.assertRaises(ValueError):
                run_baselines(path, ["nope"])

    def test_registry(self):
        self.assertTrue({"constant", "linear", "random_forest", "mlp"} <= set(make_models()))


class TestTracking(unittest.TestCase):
    def test_record_and_load(self):
        with tempfile.TemporaryDirectory() as tmp:
            first = record_run("baselines", {"seed": 0}, [{"model": "linear", "mse": 0.2}], tmp)
            second = record_run("baselines", {"seed": 1}, [{"model": "linear", "mse": 0.1}], tmp)
            record_run("other", {}, {}, tmp)
            self.assertNotEqual(first, second)
            runs = load_runs(tmp, "baselines")
            self.assertEqual([r["params"]["seed"] for r in runs], [0, 1])
            self.assertIn("python", runs[0])


if __name__ == "__main__":
    unittest.main()
