"""Locate the engine binary for tests that drive the real engine."""
import os
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]


def find_engine() -> str | None:
    """CHESS_ENGINE, or a chess_engine binary from a local build directory."""
    if os.environ.get("CHESS_ENGINE"):
        return os.environ["CHESS_ENGINE"]
    for build in ("build_release", "build", "build_native"):
        for name in ("chess_engine.exe", "chess_engine"):
            path = REPO / build / "engine" / name
            if path.exists():
                return str(path)
    return None


def require_engine() -> str:
    """The engine path, or skip the calling test when no engine is built."""
    engine = find_engine()
    if engine is None:
        raise unittest.SkipTest("build the engine or set CHESS_ENGINE to run engine tests")
    return engine
