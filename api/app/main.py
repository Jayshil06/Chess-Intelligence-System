"""FastAPI service: play games against the engine and analyse positions.

Run: uvicorn app.main:app --port 8000   (serves the built dashboard too, if dashboard/dist exists)
"""
from __future__ import annotations

import os
from collections.abc import AsyncIterator
from contextlib import asynccontextmanager
from pathlib import Path
from typing import Literal

import chess
import chess.engine
from fastapi import FastAPI, HTTPException, Request
from fastapi.responses import FileResponse, PlainTextResponse
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel, Field
from starlette.exceptions import HTTPException as StarletteHTTPException
from starlette.responses import Response
from starlette.types import Scope

from .engine import LEVELS, REPO, EngineService, Score, find_engine
from .games import Game, GameStore

DASHBOARD = REPO / "dashboard" / "dist"


class MoveOut(BaseModel):
    uci: str
    san: str


class GameOut(BaseModel):
    id: int
    created: float
    fen: str
    start_fen: str
    player_color: Literal["white", "black"]
    level: int
    turn: Literal["white", "black"]
    moves: list[MoveOut]
    status: str
    result: str | None
    in_check: bool
    legal_moves: list[str]
    evaluation: Score | None = None


class NewGame(BaseModel):
    player_color: Literal["white", "black"] = "white"
    level: int = Field(default=4, ge=1, le=max(LEVELS))
    fen: str | None = None


class PlayerMove(BaseModel):
    move: str = Field(description="UCI move, e.g. e2e4 or e7e8q")


class AnalyzeRequest(BaseModel):
    fen: str
    depth: int | None = Field(default=None, ge=1, le=20)
    movetime_ms: int | None = Field(default=None, ge=10, le=10_000)


class AnalysisOut(BaseModel):
    fen: str
    status: str
    score: Score | None
    best_move: MoveOut | None
    pv: list[MoveOut]
    depth: int
    nodes: int


def color_name(color: chess.Color) -> Literal["white", "black"]:
    return "white" if color == chess.WHITE else "black"


def san_moves(start: chess.Board, moves: list[chess.Move]) -> list[MoveOut]:
    board, out = start.copy(stack=False), []
    for move in moves:
        out.append(MoveOut(uci=move.uci(), san=board.san(move)))
        board.push(move)
    return out


def parse_board(fen: str) -> chess.Board:
    try:
        board = chess.Board(fen)
    except ValueError as exc:
        raise HTTPException(400, f"invalid FEN: {exc}") from exc
    if not board.is_valid():
        raise HTTPException(400, "illegal position")
    return board


@asynccontextmanager
async def lifespan(app: FastAPI) -> AsyncIterator[None]:
    service = EngineService(find_engine(), os.environ.get("CHESS_EVALFILE"))
    await service.start()
    app.state.engine = service
    app.state.games = GameStore()
    yield
    await service.stop()


app = FastAPI(title="Chess Intelligence API", version="0.1.0", lifespan=lifespan)


def engine(request: Request) -> EngineService:
    return request.app.state.engine


def get_game(request: Request, game_id: int) -> Game:
    game = request.app.state.games.get(game_id)
    if game is None:
        raise HTTPException(404, "game not found")
    return game


def game_out(game: Game) -> GameOut:
    status, result = game.status()
    return GameOut(
        id=game.id,
        created=game.created,
        fen=game.board.fen(),
        start_fen=game.start_fen,
        player_color=color_name(game.player_color),
        level=game.level,
        turn=color_name(game.board.turn),
        moves=san_moves(game.board.root(), game.board.move_stack),
        status=status,
        result=result,
        in_check=game.board.is_check(),
        legal_moves=[] if status != "active" else [m.uci() for m in game.board.legal_moves],
        evaluation=game.evaluation,
    )


async def engine_reply(request: Request, game: Game) -> None:
    if game.engine_to_move:
        move, game.evaluation = await engine(request).best_move(game.board, game.level)
        game.board.push(move)


def describe_limit(limit: chess.engine.Limit) -> str:
    return f"depth {limit.depth}" if limit.depth else f"{limit.time:g} s per move"


@app.get("/api/health")
async def health(request: Request) -> dict[str, str]:
    return {"status": "ok", "engine": engine(request).name}


@app.get("/api/engine")
async def engine_info(request: Request) -> dict[str, object]:
    service = engine(request)
    return {
        "name": service.name,
        "evaluation": "nnue" if service.eval_file else "classical",
        "eval_file": service.eval_file,
        "levels": {level: describe_limit(limit) for level, limit in LEVELS.items()},
    }


@app.post("/api/games", status_code=201)
async def create_game(request: Request, body: NewGame) -> GameOut:
    board = parse_board(body.fen) if body.fen is not None else chess.Board()
    color = chess.WHITE if body.player_color == "white" else chess.BLACK
    game = request.app.state.games.create(color, body.level, board)
    await engine_reply(request, game)
    return game_out(game)


@app.get("/api/games")
async def list_games(request: Request) -> list[GameOut]:
    return [game_out(game) for game in request.app.state.games.all()]


@app.get("/api/games/{game_id}")
async def read_game(request: Request, game_id: int) -> GameOut:
    return game_out(get_game(request, game_id))


@app.post("/api/games/{game_id}/moves")
async def play_move(request: Request, game_id: int, body: PlayerMove) -> GameOut:
    game = get_game(request, game_id)
    if game.is_over:
        raise HTTPException(409, "game is over")
    if game.board.turn != game.player_color:
        raise HTTPException(409, "not your turn")
    try:
        move = game.board.parse_uci(body.move)
    except ValueError as exc:
        raise HTTPException(400, f"illegal move: {body.move}") from exc
    if not move:  # parse_uci accepts the null move "0000"
        raise HTTPException(400, f"illegal move: {body.move}")
    game.board.push(move)
    await engine_reply(request, game)
    return game_out(game)


@app.post("/api/games/{game_id}/undo")
async def undo_move(request: Request, game_id: int) -> GameOut:
    game = get_game(request, game_id)
    try:
        game.undo()
    except ValueError as exc:
        raise HTTPException(409, str(exc)) from exc
    return game_out(game)


@app.post("/api/games/{game_id}/resign")
async def resign(request: Request, game_id: int) -> GameOut:
    game = get_game(request, game_id)
    if game.is_over:
        raise HTTPException(409, "game is over")
    game.resigned = True
    return game_out(game)


@app.get("/api/games/{game_id}/pgn", response_class=PlainTextResponse)
async def game_pgn(request: Request, game_id: int) -> str:
    return get_game(request, game_id).pgn(engine(request).name)


@app.post("/api/analyze")
async def analyze(request: Request, body: AnalyzeRequest) -> AnalysisOut:
    board = parse_board(body.fen)
    outcome = board.outcome(claim_draw=False)
    if outcome is not None:
        return AnalysisOut(fen=board.fen(), status=outcome.termination.name.lower(), score=None,
                           best_move=None, pv=[], depth=0, nodes=0)
    if body.depth is not None:
        limit = chess.engine.Limit(depth=body.depth)
    else:  # Time-bounded by default: deep fixed-depth searches can take minutes
        limit = chess.engine.Limit(time=(body.movetime_ms or 1000) / 1000)
    result = await engine(request).analyse(board, limit)
    pv = san_moves(board, result.pv)
    return AnalysisOut(fen=board.fen(), status="active", score=result.score,
                       best_move=pv[0] if pv else None, pv=pv, depth=result.depth, nodes=result.nodes)


class SpaStaticFiles(StaticFiles):
    """Serve index.html for client-side routes such as /play."""

    async def get_response(self, path: str, scope: Scope) -> Response:
        try:
            return await super().get_response(path, scope)
        except StarletteHTTPException as exc:
            if exc.status_code != 404 or path.startswith("api"):
                raise
            return FileResponse(Path(str(self.directory)) / "index.html")


if DASHBOARD.exists():
    app.mount("/", SpaStaticFiles(directory=DASHBOARD, html=True), name="dashboard")
