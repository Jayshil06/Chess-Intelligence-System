"""In-memory game store for games against the engine."""
from __future__ import annotations

import itertools
import time
from dataclasses import dataclass, field

import chess
import chess.pgn

from .engine import Score


@dataclass
class Game:
    id: int
    player_color: chess.Color
    level: int
    board: chess.Board
    created: float = field(default_factory=time.time)
    resigned: bool = False
    evaluation: Score | None = None  # From the engine's last move

    @property
    def start_fen(self) -> str:
        return self.board.root().fen()

    def status(self) -> tuple[str, str | None]:
        """("active", None) or a finished status with its PGN result."""
        if self.resigned:
            return "resigned", "0-1" if self.player_color == chess.WHITE else "1-0"
        outcome = self.board.outcome(claim_draw=True)
        if outcome is None:
            return "active", None
        return outcome.termination.name.lower(), outcome.result()

    @property
    def is_over(self) -> bool:
        return self.status()[0] != "active"

    @property
    def engine_to_move(self) -> bool:
        return not self.is_over and self.board.turn != self.player_color

    def undo(self) -> None:
        """Take back the player's last move and any engine reply, so it is the player's turn again."""
        plies = 2 if self.board.turn == self.player_color else 1
        if len(self.board.move_stack) < plies:
            raise ValueError("no player move to take back")
        for _ in range(plies):
            self.board.pop()
        self.resigned = False
        self.evaluation = None

    def pgn(self, engine_name: str) -> str:
        game = chess.pgn.Game.from_board(self.board)
        game.headers["Event"] = "Chess Intelligence web game"
        game.headers["White"] = "Player" if self.player_color == chess.WHITE else engine_name
        game.headers["Black"] = engine_name if self.player_color == chess.WHITE else "Player"
        game.headers["Result"] = self.status()[1] or "*"
        return str(game)


class GameStore:
    def __init__(self) -> None:
        self._games: dict[int, Game] = {}
        self._ids = itertools.count(1)

    def create(self, player_color: chess.Color, level: int, board: chess.Board) -> Game:
        game = Game(next(self._ids), player_color, level, board)
        self._games[game.id] = game
        return game

    def get(self, game_id: int) -> Game | None:
        return self._games.get(game_id)

    def all(self) -> list[Game]:
        return sorted(self._games.values(), key=lambda g: g.created, reverse=True)
