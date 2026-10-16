import { Chess } from 'chess.js';
import { Check, Copy, Flag, Lightbulb, LoaderCircle, Plus, Repeat, Shuffle, Undo2 } from 'lucide-react';
import { useEffect, useState } from 'react';
import type { Arrow } from 'react-chessboard';
import { Link, useNavigate, useParams } from 'react-router';
import { api } from '../api.ts';
import type { Color, GameState } from '../api.ts';
import { statusText } from '../chess-utils.ts';
import { Board } from '../components/Board.tsx';
import { EvalBar } from '../components/EvalBar.tsx';
import { MoveList } from '../components/MoveList.tsx';

export const LEVEL_NAMES = ['Beginner', 'Novice', 'Casual', 'Club', 'Strong club', 'Expert', 'Master', 'Maximum'];
const HINT_COLOR = 'rgba(64, 156, 255, 0.85)';

const errorText = (e: unknown) => (e instanceof Error ? e.message : String(e));

export function Play() {
  const { gameId } = useParams();
  return gameId ? <GameView key={gameId} gameId={Number(gameId)} /> : <NewGamePanel />;
}

function NewGamePanel() {
  const navigate = useNavigate();
  const [color, setColor] = useState<Color | 'random'>('white');
  const [level, setLevel] = useState(4);
  const [starting, setStarting] = useState(false);
  const [error, setError] = useState<string | null>(null);

  async function start() {
    setStarting(true);
    setError(null);
    try {
      const playerColor = color === 'random' ? (Math.random() < 0.5 ? 'white' : 'black') : color;
      const game = await api.createGame({ player_color: playerColor, level });
      navigate(`/play/${game.id}`);
    } catch (e) {
      setError(errorText(e));
      setStarting(false);
    }
  }

  return (
    <section className="panel narrow">
      <h1>New game</h1>
      <p className="muted">Choose your side and how strong the engine should play.</p>

      <fieldset className="choice-group">
        <legend>Play as</legend>
        {(['white', 'black', 'random'] as const).map((c) => (
          <button key={c} className={`choice ${color === c ? 'selected' : ''}`} aria-pressed={color === c} onClick={() => setColor(c)}>
            {c === 'random' ? <Shuffle size={16} aria-hidden /> : <span className={`swatch ${c}`} aria-hidden />}
            {c[0].toUpperCase() + c.slice(1)}
          </button>
        ))}
      </fieldset>

      <label className="field">
        <span>
          Engine level: <strong>{level}</strong> — {LEVEL_NAMES[level - 1]}
        </span>
        <input type="range" min={1} max={8} value={level} onChange={(e) => setLevel(Number(e.target.value))} />
      </label>

      {error && <p className="error">{error}</p>}
      <button className="primary large" onClick={start} disabled={starting}>
        {starting ? <LoaderCircle className="spin" size={18} aria-hidden /> : <Plus size={18} aria-hidden />}
        Start game
      </button>
    </section>
  );
}

function GameView({ gameId }: { gameId: number }) {
  const navigate = useNavigate();
  const [game, setGame] = useState<GameState | null>(null);
  const [pendingFen, setPendingFen] = useState<string | null>(null);
  const [pendingMove, setPendingMove] = useState<string | null>(null);
  const [thinking, setThinking] = useState(false);
  const [hinting, setHinting] = useState(false);
  const [flipped, setFlipped] = useState(false);
  const [hint, setHint] = useState<Arrow[]>([]);
  const [copied, setCopied] = useState(false);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    let ignore = false;
    api.getGame(gameId).then(
      (g) => !ignore && setGame(g),
      (e) => !ignore && setError(errorText(e)),
    );
    return () => {
      ignore = true;
    };
  }, [gameId]);

  if (!game) {
    return (
      <section className="panel narrow">
        {error ? (
          <>
            <p className="error">Could not load game #{gameId}: {error}</p>
            <Link to="/play" className="button primary">Start a new game</Link>
          </>
        ) : (
          <p className="muted">Loading game…</p>
        )}
      </section>
    );
  }

  const current = game;
  const active = current.status === 'active';
  const yourTurn = active && current.turn === current.player_color && !thinking;
  const orientation: Color = (current.player_color === 'white') !== flipped ? 'white' : 'black';

  async function run(action: () => Promise<GameState>) {
    setThinking(true);
    setError(null);
    setHint([]);
    try {
      setGame(await action());
    } catch (e) {
      setError(errorText(e));
    } finally {
      setThinking(false);
      setPendingFen(null);
      setPendingMove(null);
    }
  }

  function playMove(uci: string) {
    // Show the move at once; the engine's reply replaces this position
    const local = new Chess(current.fen);
    local.move({ from: uci.slice(0, 2), to: uci.slice(2, 4), promotion: uci[4] });
    setPendingFen(local.fen());
    setPendingMove(uci);
    void run(() => api.move(current.id, uci));
  }

  async function showHint() {
    setThinking(true);
    setHinting(true);
    try {
      const analysis = await api.analyze(current.fen, 500);
      const best = analysis.best_move?.uci;
      setHint(best ? [{ startSquare: best.slice(0, 2), endSquare: best.slice(2, 4), color: HINT_COLOR }] : []);
    } catch (e) {
      setError(errorText(e));
    } finally {
      setThinking(false);
      setHinting(false);
    }
  }

  async function copyPgn() {
    const pgn = await (await fetch(api.pgnUrl(current.id))).text();
    await navigator.clipboard.writeText(pgn);
    setCopied(true);
    setTimeout(() => setCopied(false), 1500);
  }

  const lastMove = pendingMove ?? current.moves.at(-1)?.uci ?? null;
  const status = statusText(current.status, current.result, current.player_color);

  return (
    <div className="game-layout">
      <div className="board-column">
        <EvalBar score={current.evaluation} orientation={orientation} />
        <Board
          fen={pendingFen ?? current.fen}
          orientation={orientation}
          legalMoves={yourTurn ? current.legal_moves : []}
          onMove={playMove}
          lastMove={lastMove}
          inCheck={!pendingFen && current.in_check}
          arrows={hint}
        />
      </div>

      <aside className="panel side">
        <div className="game-meta">
          <h1>Game #{current.id}</h1>
          <span className="badge">
            You play {current.player_color} · Level {current.level} ({LEVEL_NAMES[current.level - 1]})
          </span>
        </div>

        <p className="turn" aria-live="polite">
          {!active ? (
            <strong>{status}</strong>
          ) : thinking ? (
            <>
              <LoaderCircle className="spin" size={16} aria-hidden /> {hinting ? 'Finding a hint…' : 'Engine is thinking…'}
            </>
          ) : yourTurn ? (
            current.in_check ? 'Your move — you are in check!' : 'Your move'
          ) : (
            'Waiting for the engine'
          )}
        </p>

        {error && <p className="error">{error}</p>}

        <MoveList moves={current.moves} startFen={current.start_fen} />

        <div className="button-row">
          <button onClick={() => run(() => api.undo(current.id))} disabled={thinking} title="Take back your last move">
            <Undo2 size={16} aria-hidden /> Undo
          </button>
          <button onClick={showHint} disabled={!yourTurn} title="Show the engine's suggestion">
            <Lightbulb size={16} aria-hidden /> Hint
          </button>
          <button onClick={() => setFlipped(!flipped)} title="Flip the board">
            <Repeat size={16} aria-hidden /> Flip
          </button>
          <button onClick={copyPgn} title="Copy the game as PGN">
            {copied ? <Check size={16} aria-hidden /> : <Copy size={16} aria-hidden />} {copied ? 'Copied' : 'PGN'}
          </button>
          <button className="danger" onClick={() => run(() => api.resign(current.id))} disabled={!active || thinking}>
            <Flag size={16} aria-hidden /> Resign
          </button>
        </div>

        <div className="button-row">
          <button className="primary" onClick={() => navigate('/play')}>
            <Plus size={16} aria-hidden /> New game
          </button>
          <Link className="button" to={`/analysis?fen=${encodeURIComponent(current.fen)}`}>
            Analyse position
          </Link>
        </div>
      </aside>
    </div>
  );
}
