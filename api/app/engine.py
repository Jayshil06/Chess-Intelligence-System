"""Shared UCI engine process for the API, with requests serialized through a lock."""
from __future__ import annotations

import asyncio
import os
from dataclasses import dataclass
from pathlib import Path

import chess
import chess.engine

REPO = Path(__file__).resolve().parents[2]

# Difficulty levels: shallow fixed depths first, then increasing think time
LEVELS: dict[int, chess.engine.Limit] = {
    1: chess.engine.Limit(depth=1),
    2: chess.engine.Limit(depth=2),
    3: chess.engine.Limit(depth=3),
    4: chess.engine.Limit(depth=5),
    5: chess.engine.Limit(time=0.1),
    6: chess.engine.Limit(time=0.25),
    7: chess.engine.Limit(time=0.5),
    8: chess.engine.Limit(time=1.0),
}


def find_engine() -> Path:
    """CHESS_ENGINE, or the first chess_engine binary in a local build directory."""
    if env := os.environ.get("CHESS_ENGINE"):
        return Path(env)
    for build in ("build_release", "build", "build_native"):
        for name in ("chess_engine.exe", "chess_engine"):
            if (path := REPO / build / "engine" / name).exists():
                return path
    raise FileNotFoundError("engine binary not found; build the engine or set CHESS_ENGINE")


@dataclass
class Score:
    """White's point of view: centipawns, or moves to mate (negative when White is mated)."""
    cp: int | None
    mate: int | None


def white_score(score: chess.engine.PovScore | None) -> Score | None:
    if score is None:
        return None
    white = score.white()
    return Score(cp=white.score(), mate=white.mate())


@dataclass
class Analysis:
    score: Score | None
    depth: int
    nodes: int
    pv: list[chess.Move]


class EngineService:
    def __init__(self, path: Path, eval_file: str | None = None):
        self.path = path
        self.eval_file = eval_file
        self.name = path.name
        self._engine: chess.engine.UciProtocol | None = None
        self._lock = asyncio.Lock()

    async def start(self) -> None:
        _, self._engine = await chess.engine.popen_uci(str(self.path))
        self.name = self._engine.id.get("name", self.path.name)
        if self.eval_file:
            await self._engine.configure({"EvalFile": self.eval_file})

    async def stop(self) -> None:
        if self._engine is not None:
            await self._engine.quit()
            self._engine = None

    @property
    def engine(self) -> chess.engine.UciProtocol:
        if self._engine is None:
            raise RuntimeError("engine is not running")
        return self._engine

    async def best_move(self, board: chess.Board, level: int) -> tuple[chess.Move, Score | None]:
        async with self._lock:
            result = await self.engine.play(board, LEVELS[level], info=chess.engine.INFO_SCORE)
        if result.move is None:
            raise RuntimeError("engine returned no move")
        return result.move, white_score(result.info.get("score"))

    async def analyse(self, board: chess.Board, limit: chess.engine.Limit) -> Analysis:
        async with self._lock:
            info = await self.engine.analyse(board, limit)
        return Analysis(
            score=white_score(info.get("score")),
            depth=info.get("depth", 0),
            nodes=info.get("nodes", 0),
            pv=list(info.get("pv", [])),
        )
