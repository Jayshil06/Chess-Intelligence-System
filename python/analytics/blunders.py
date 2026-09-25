"""Engine-based blunder detection: centipawn loss per move, flagged above a threshold.

Usage: python -m analytics.blunders games.pgn ENGINE [--depth 8] [--threshold 200]
"""
from __future__ import annotations

import argparse
from dataclasses import dataclass

import chess
import chess.engine
import chess.pgn

from chess_data.pgn import iter_games, open_pgn
from selfplay.match import engine_command

MATE_CP = 10_000


@dataclass(frozen=True)
class Blunder:
    ply: int
    san: str
    color: str
    eval_before: int  # Centipawns, mover's point of view
    eval_after: int
    loss: int


def position_eval(board: chess.Board, engine: chess.engine.SimpleEngine, limit: chess.engine.Limit) -> int:
    """Evaluation for the side to move; terminal positions are scored without the engine."""
    if board.is_checkmate():
        return -MATE_CP
    if board.is_game_over(claim_draw=True):
        return 0
    score = engine.analyse(board, limit).get("score")
    if score is None:
        raise RuntimeError("engine returned no evaluation")
    return score.pov(board.turn).score(mate_score=MATE_CP)


def find_blunders(game: chess.pgn.Game, engine: chess.engine.SimpleEngine, limit: chess.engine.Limit,
                  threshold: int = 200) -> list[Blunder]:
    """One analysis per position: the eval after a move is the negated eval of the next position."""
    board = game.board()
    evals = [position_eval(board, engine, limit)]
    moves = list(game.mainline_moves())
    sans = []
    for move in moves:
        sans.append(board.san(move))
        board.push(move)
        evals.append(position_eval(board, engine, limit))

    board = game.board()
    blunders = []
    for ply, move in enumerate(moves):
        before, after = evals[ply], -evals[ply + 1]
        loss = before - after
        if loss >= threshold:
            color = "white" if board.turn == chess.WHITE else "black"
            blunders.append(Blunder(ply, sans[ply], color, before, after, loss))
        board.push(move)
    return blunders


def main() -> None:
    parser = argparse.ArgumentParser(description="Flag moves that lose at least --threshold centipawns")
    parser.add_argument("pgn")
    parser.add_argument("engine")
    parser.add_argument("--depth", type=int, default=8)
    parser.add_argument("--threshold", type=int, default=200)
    parser.add_argument("--max-games", type=int, default=10)
    args = parser.parse_args()

    limit = chess.engine.Limit(depth=args.depth)
    with open_pgn(args.pgn) as stream, chess.engine.SimpleEngine.popen_uci(engine_command(args.engine)) as engine:
        for index, game in enumerate(iter_games(stream)):
            if index >= args.max_games:
                break
            print(f"{game.headers.get('White', '?')} - {game.headers.get('Black', '?')} {game.headers['Result']}")
            for b in find_blunders(game, engine, limit, args.threshold):
                print(f"  ply {b.ply:3} {b.color:5} {b.san:7} {b.eval_before:+6} -> {b.eval_after:+6}  (-{b.loss})")


if __name__ == "__main__":
    main()
