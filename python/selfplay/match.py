"""Engine-vs-engine UCI matches with paired openings, clock handling, Elo and SPRT.

Usage: python -m selfplay.match NEW_ENGINE BASE_ENGINE --games 200 --tc 10+0.1 --sprt 0 5
"""
from __future__ import annotations

import argparse
import shlex
import time
from collections.abc import Callable
from dataclasses import dataclass, field
from pathlib import Path

import chess
import chess.engine
import chess.pgn

from .elo import estimate_elo, sprt_bounds, sprt_decision, sprt_llr

DEFAULT_OPENINGS = [
    "e4 e5 Nf3 Nc6", "d4 d5 c4 e6", "e4 c5 Nf3 d6", "e4 e6 d4 d5",
    "d4 Nf6 c4 g6", "c4 e5 Nc3 Nf6", "e4 c6 d4 d5", "Nf3 d5 g3 Nf6",
]


def engine_command(cmd: str) -> list[str]:
    """An existing path is used verbatim (spaces, backslashes); anything else is shell-split."""
    return [cmd] if Path(cmd).exists() else shlex.split(cmd)


def opening_fen(san_line: str) -> str:
    board = chess.Board()
    for san in san_line.split():
        board.push_san(san)
    return board.fen()


@dataclass(frozen=True)
class TimeControl:
    base: float       # Seconds per side
    increment: float  # Seconds added after each move

    @classmethod
    def parse(cls, text: str) -> TimeControl:
        base, _, inc = text.partition("+")
        return cls(float(base), float(inc or 0))


@dataclass
class MatchResult:
    """Counts are from the first engine's point of view."""
    wins: int = 0
    draws: int = 0
    losses: int = 0
    games: list[chess.pgn.Game] = field(default_factory=list)

    @property
    def played(self) -> int:
        return self.wins + self.draws + self.losses


def play_game(white: chess.engine.SimpleEngine, black: chess.engine.SimpleEngine, start_fen: str, *,
              limit: chess.engine.Limit | None = None, tc: TimeControl | None = None,
              max_plies: int = 400, game_key: object = None) -> chess.pgn.Game:
    """Play one game. With a time control, running out of clock or moving illegally forfeits."""
    board = chess.Board(start_fen)
    engines = {chess.WHITE: white, chess.BLACK: black}
    clock = {chess.WHITE: tc.base, chess.BLACK: tc.base} if tc else None
    result, termination = None, None

    while not board.is_game_over(claim_draw=True) and len(board.move_stack) < max_plies:
        mover = board.turn
        if clock:
            limit = chess.engine.Limit(white_clock=clock[chess.WHITE], black_clock=clock[chess.BLACK],
                                       white_inc=tc.increment, black_inc=tc.increment)
        start = time.perf_counter()
        try:
            move = engines[mover].play(board, limit, game=game_key).move
        except chess.engine.EngineError:
            move = None
        if clock:
            clock[mover] -= time.perf_counter() - start
            if clock[mover] < 0:
                result, termination = ("0-1" if mover == chess.WHITE else "1-0"), "time forfeit"
                break
            clock[mover] += tc.increment
        if move is None or move not in board.legal_moves:
            result, termination = ("0-1" if mover == chess.WHITE else "1-0"), "illegal move"
            break
        board.push(move)

    if result is None:
        outcome = board.outcome(claim_draw=True)
        result = outcome.result() if outcome else "1/2-1/2"
        termination = outcome.termination.name.lower() if outcome else "max plies"
    game = chess.pgn.Game.from_board(board)
    game.headers["Result"] = result
    game.headers["Termination"] = termination
    return game


def run_match(engine1: str, engine2: str, games: int, *, openings: list[str] | None = None,
              limit: chess.engine.Limit | None = None, tc: TimeControl | None = None,
              sprt: tuple[float, float] | None = None,
              on_game: Callable[[MatchResult], None] | None = None) -> MatchResult:
    """Each opening is played twice with colours swapped. Stops early once SPRT decides."""
    if limit is None and tc is None:
        limit = chess.engine.Limit(time=0.1)
    fens = [opening_fen(line) for line in (openings or DEFAULT_OPENINGS)]
    result = MatchResult()
    first = chess.engine.SimpleEngine.popen_uci(engine_command(engine1))
    second = chess.engine.SimpleEngine.popen_uci(engine_command(engine2))
    try:
        for i in range(games):
            first_is_white = i % 2 == 0
            white, black = (first, second) if first_is_white else (second, first)
            game = play_game(white, black, fens[(i // 2) % len(fens)], limit=limit, tc=tc, game_key=i)
            game.headers["White"] = Path(engine1 if first_is_white else engine2).name
            game.headers["Black"] = Path(engine2 if first_is_white else engine1).name
            game.headers["Round"] = str(i + 1)

            score = {"1-0": 1.0, "0-1": 0.0}.get(game.headers["Result"], 0.5)
            score = score if first_is_white else 1.0 - score
            if score == 1.0:
                result.wins += 1
            elif score == 0.5:
                result.draws += 1
            else:
                result.losses += 1
            result.games.append(game)
            if on_game:
                on_game(result)
            if sprt and sprt_decision(result.wins, result.draws, result.losses, *sprt):
                break
    finally:
        first.quit()
        second.quit()
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description="Play a UCI engine match and report Elo / SPRT")
    parser.add_argument("engine1", help="engine under test (command line)")
    parser.add_argument("engine2", help="baseline engine (command line)")
    parser.add_argument("--games", type=int, default=100)
    parser.add_argument("--tc", help="clock as base+increment in seconds, e.g. 10+0.1")
    parser.add_argument("--movetime", type=float, help="fixed seconds per move (when --tc is not given)")
    parser.add_argument("--depth", type=int, help="fixed depth per move (when --tc is not given)")
    parser.add_argument("--sprt", nargs=2, type=float, metavar=("ELO0", "ELO1"), help="stop at an SPRT decision")
    parser.add_argument("--pgn", help="write all games to this PGN file")
    args = parser.parse_args()

    tc = TimeControl.parse(args.tc) if args.tc else None
    limit = None
    if not tc and (args.movetime or args.depth):
        limit = chess.engine.Limit(time=args.movetime, depth=args.depth)

    def report(r: MatchResult) -> None:
        line = f"game {r.played}: +{r.wins} ={r.draws} -{r.losses}  elo {estimate_elo(r.wins, r.draws, r.losses)}"
        if args.sprt:
            lo, hi = sprt_bounds()
            line += f"  LLR {sprt_llr(r.wins, r.draws, r.losses, *args.sprt):+.2f} [{lo:.2f}, {hi:.2f}]"
        print(line, flush=True)

    result = run_match(args.engine1, args.engine2, args.games, limit=limit, tc=tc,
                       sprt=tuple(args.sprt) if args.sprt else None, on_game=report)
    if args.sprt:
        print("SPRT:", sprt_decision(result.wins, result.draws, result.losses, *args.sprt) or "inconclusive")
    if args.pgn:
        with open(args.pgn, "w", encoding="utf-8") as f:
            for game in result.games:
                print(game, file=f, end="\n\n")


if __name__ == "__main__":
    main()
