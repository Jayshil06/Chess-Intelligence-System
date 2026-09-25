"""Streaming PGN reading, game validation, and position extraction."""
from __future__ import annotations

import bz2
import gzip
import io
from collections import Counter
from collections.abc import Iterator
from dataclasses import dataclass, field
from pathlib import Path
from typing import TextIO

import chess
import chess.pgn

RESULT_SCORE = {"1-0": 1.0, "0-1": 0.0, "1/2-1/2": 0.5}


def open_pgn(path: str | Path) -> TextIO:
    """Open a PGN file as text, transparently decompressing .gz, .bz2 and .zst."""
    path = Path(path)
    suffix = path.suffix.lower()
    if suffix == ".gz":
        return gzip.open(path, "rt", encoding="utf-8", errors="replace")
    if suffix == ".bz2":
        return bz2.open(path, "rt", encoding="utf-8", errors="replace")
    if suffix == ".zst":
        import zstandard  # Optional dependency: pip install chess-platform[zstd]

        raw = zstandard.ZstdDecompressor().stream_reader(path.open("rb"))
        return io.TextIOWrapper(raw, encoding="utf-8", errors="replace")
    return path.open("r", encoding="utf-8", errors="replace")


class _QuietGameBuilder(chess.pgn.GameBuilder):
    """Collect parse errors on the game without logging them; skips are reported via IngestStats."""

    def handle_error(self, error: Exception) -> None:
        self.game.errors.append(error)


@dataclass
class IngestStats:
    games_read: int = 0
    games_kept: int = 0
    positions: int = 0
    skipped: Counter = field(default_factory=Counter)


def _elo(headers: chess.pgn.Headers, key: str) -> int | None:
    value = headers.get(key, "")
    return int(value) if value.isdigit() else None


def rejection_reason(game: chess.pgn.Game, min_elo: int = 0) -> str | None:
    """Why a game is unusable for training data, or None if it is fine."""
    if game.errors:
        return "parse_error"
    if game.headers.get("Variant", "Standard").lower() not in ("standard", "chess", "from position"):
        return "variant"
    if game.headers.get("Result") not in RESULT_SCORE:
        return "no_result"
    if game.next() is None:
        return "no_moves"
    if min_elo:
        white, black = _elo(game.headers, "WhiteElo"), _elo(game.headers, "BlackElo")
        if white is None or black is None or min(white, black) < min_elo:
            return "low_elo"
    return None


def iter_games(stream: TextIO, stats: IngestStats | None = None, min_elo: int = 0) -> Iterator[chess.pgn.Game]:
    """Yield valid games one at a time; memory use does not grow with file size."""
    stats = stats if stats is not None else IngestStats()
    while (game := chess.pgn.read_game(stream, Visitor=_QuietGameBuilder)) is not None:
        stats.games_read += 1
        reason = rejection_reason(game, min_elo)
        if reason:
            stats.skipped[reason] += 1
            continue
        stats.games_kept += 1
        yield game


def iter_positions(game: chess.pgn.Game, game_id: int, skip_plies: int = 0, every: int = 1) -> Iterator[dict]:
    """Yield one row per position before each mainline move (White-perspective result)."""
    headers = game.headers
    result = RESULT_SCORE[headers["Result"]]
    white_elo, black_elo = _elo(headers, "WhiteElo"), _elo(headers, "BlackElo")
    eco = headers.get("ECO")
    white, black = headers.get("White"), headers.get("Black")
    board = game.board()
    for ply, move in enumerate(game.mainline_moves()):
        if ply >= skip_plies and (ply - skip_plies) % every == 0:
            yield {
                "game_id": game_id,
                "ply": ply,
                "fen": board.fen(),
                "side_to_move": 0 if board.turn == chess.WHITE else 1,
                "move": move.uci(),
                "result": result,
                "white": white,
                "black": black,
                "white_elo": white_elo,
                "black_elo": black_elo,
                "eco": eco,
            }
        board.push(move)
