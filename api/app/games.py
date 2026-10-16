"""In-memory game store for games against the engine."""
from __future__ import annotations

import itertools
import time
from dataclasses import dataclass, field

import chess
import chess.pgn


@dataclass
class Game:
    id: int
    player_color: chess.Color
    level: int
    board: chess.Board
    created: float = field(default_factory=time.time)
    resigned: bool = False

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

    def player_moves(self) -> int:
        start_turn = self.board.root().turn
        movers = (start_turn if i % 2 == 0 else not start_turn for i in range(len(self.board.move_stack)))
        return sum(1 for mover in movers if mover == self.player_color)

    def undo(self) -> None:
        """Take back the player's last move and any engine reply, so it is the player's turn again."""
        if self.player_moves() == 0:
            raise ValueError("no player move to take back")
        undone_player_move = False
        while self.board.move_stack and not (undone_player_move and self.board.turn == self.player_color):
            undone_player_move |= (not self.board.turn) == self.player_color
            self.board.pop()
        self.resigned = False

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

    def create(self, player_color: chess.Color, level: int, fen: str | None = None) -> Game:
        board = chess.Board(fen) if fen else chess.Board()
        game = Game(next(self._ids), player_color, level, board)
        self._games[game.id] = game
        return game

    def get(self, game_id: int) -> Game | None:
        return self._games.get(game_id)

    def all(self) -> list[Game]:
        return sorted(self._games.values(), key=lambda g: g.created, reverse=True)
