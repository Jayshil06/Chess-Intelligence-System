"""Minimal experiment tracking: one JSON record per run with params, metrics, and provenance."""
from __future__ import annotations

import json
import platform
import subprocess
import time
from pathlib import Path
from typing import Any

DEFAULT_ROOT = Path("experiments") / "runs"


def _git_commit() -> str | None:
    try:
        out = subprocess.run(["git", "rev-parse", "--short", "HEAD"], capture_output=True, text=True, check=True)
        return out.stdout.strip()
    except (OSError, subprocess.CalledProcessError):
        return None


def record_run(name: str, params: dict[str, Any], metrics: Any, root: str | Path = DEFAULT_ROOT) -> Path:
    root = Path(root)
    root.mkdir(parents=True, exist_ok=True)
    stamp = time.strftime("%Y%m%d-%H%M%S")
    record = {
        "name": name,
        "timestamp": stamp,
        "git_commit": _git_commit(),
        "python": platform.python_version(),
        "params": params,
        "metrics": metrics,
    }
    path = root / f"{stamp}_{name}.json"
    suffix = 1
    while path.exists():
        path = root / f"{stamp}_{name}_{suffix}.json"
        suffix += 1
    path.write_text(json.dumps(record, indent=2), encoding="utf-8")
    return path


def load_runs(root: str | Path = DEFAULT_ROOT, name: str | None = None) -> list[dict[str, Any]]:
    runs = [json.loads(p.read_text(encoding="utf-8")) for p in sorted(Path(root).glob("*.json"))]
    return [r for r in runs if name is None or r["name"] == name]
