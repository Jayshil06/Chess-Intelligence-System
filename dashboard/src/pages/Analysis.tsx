import { Chess, validateFen } from 'chess.js';
import { LoaderCircle, Microscope, Play, Repeat, RotateCcw, Undo2, Upload } from 'lucide-react';
import { useEffect, useMemo, useRef, useState } from 'react';
import type { ReactNode } from 'react';
import { useNavigate, useSearchParams } from 'react-router';
import { api, errorText } from '../api.ts';
import type { Analysis as AnalysisResult, Color, MoveInfo } from '../api.ts';
import { bestMoveArrow, formatScore } from '../chess-utils.ts';
import { Board } from '../components/Board.tsx';
import { EvalBar } from '../components/EvalBar.tsx';
import { MoveList } from '../components/MoveList.tsx';

const START_FEN = new Chess().fen();
const THINK_TIMES = [250, 500, 1000, 2000, 5000];

type Ply = MoveInfo & { fen: string };

export function Analysis() {
  const navigate = useNavigate();
  const [params] = useSearchParams();
  const initial = params.get('fen');
  const [startFen, setStartFen] = useState(initial && validateFen(initial).ok ? initial : START_FEN);
  const [plies, setPlies] = useState<Ply[]>([]);
  const [fenInput, setFenInput] = useState('');
  const [orientation, setOrientation] = useState<Color>('white');
  const [thinkMs, setThinkMs] = useState(1000);
  const [auto, setAuto] = useState(true);
  const [result, setResult] = useState<AnalysisResult | null>(null);
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const latest = useRef(0);

  const fen = plies.at(-1)?.fen ?? startFen;
  const { position, legalMoves } = useMemo(() => {
    const chess = new Chess(fen);
    return { position: chess, legalMoves: chess.moves({ verbose: true }).map((m) => m.lan) };
  }, [fen]);
  const shown = result?.fen === fen ? result : null;

  async function analyse() {
    // Responses can arrive out of order; only the latest request may update the panel
    const request = ++latest.current;
    setBusy(true);
    setError(null);
    try {
      const analysis = await api.analyze(fen, thinkMs);
      if (request === latest.current) setResult(analysis);
    } catch (e) {
      if (request === latest.current) setError(errorText(e));
    } finally {
      if (request === latest.current) setBusy(false);
    }
  }

  useEffect(() => {
    setResult(null);
    if (!auto) return;
    // Wait for a pause so quick moves do not queue up engine work on the server
    const timer = setTimeout(() => void analyse(), 300);
    return () => clearTimeout(timer);
  }, [fen, thinkMs, auto]);

  function onMove(uci: string) {
    const chess = new Chess(fen);
    const move = chess.move(uci);
    setPlies([...plies, { uci, san: move.san, fen: chess.fen() }]);
  }

  function load(newFen: string) {
    setStartFen(newFen);
    setPlies([]);
  }

  function loadInput() {
    const check = validateFen(fenInput.trim());
    if (!check.ok) {
      setError(`Invalid FEN: ${check.error}`);
      return;
    }
    load(fenInput.trim());
    setFenInput('');
  }

  async function playFromHere() {
    const game = await api.createGame({ player_color: position.turn() === 'w' ? 'white' : 'black', level: 4, fen });
    navigate(`/play/${game.id}`);
  }

  let resultView: ReactNode;
  if (shown && shown.status !== 'active') {
    resultView = <strong>Game over: {shown.status.replaceAll('_', ' ')}</strong>;
  } else if (shown) {
    resultView = (
      <>
        <div className="score">{formatScore(shown.score)}</div>
        <div>
          Best move <strong>{shown.best_move?.san ?? '—'}</strong>
          <span className="muted"> · depth {shown.depth} · {shown.nodes.toLocaleString()} nodes</span>
        </div>
        <div className="pv" title="Principal variation">
          {shown.pv.map((m) => m.san).join(' ')}
        </div>
      </>
    );
  } else if (busy) {
    resultView = (
      <span>
        <LoaderCircle className="spin" size={16} aria-hidden /> Analysing…
      </span>
    );
  } else {
    resultView = <span className="muted">Press Analyse to evaluate this position.</span>;
  }

  return (
    <div className="game-layout">
      <div className="board-column">
        <EvalBar score={shown?.score ?? null} orientation={orientation} />
        <Board
          fen={fen}
          orientation={orientation}
          legalMoves={legalMoves}
          onMove={onMove}
          lastMove={plies.at(-1)?.uci}
          inCheck={position.inCheck()}
          arrows={shown?.best_move ? [bestMoveArrow(shown.best_move.uci)] : []}
        />
      </div>

      <aside className="panel side">
        <h1>Analysis board</h1>
        <p className="muted">Move pieces for either side. The engine evaluates the position on the board.</p>

        <div className="analysis-result" aria-live="polite">{resultView}</div>

        <div className="button-row">
          <button className="primary" onClick={analyse} disabled={busy}>
            <Microscope size={16} aria-hidden /> Analyse
          </button>
          <label className="inline">
            Think time
            <select value={thinkMs} onChange={(e) => setThinkMs(Number(e.target.value))}>
              {THINK_TIMES.map((ms) => (
                <option key={ms} value={ms}>{ms / 1000} s</option>
              ))}
            </select>
          </label>
          <label className="inline">
            <input type="checkbox" checked={auto} onChange={(e) => setAuto(e.target.checked)} /> Auto
          </label>
        </div>

        {error && <p className="error">{error}</p>}

        <MoveList moves={plies} startFen={startFen} />

        <div className="button-row">
          <button onClick={() => setPlies(plies.slice(0, -1))} disabled={plies.length === 0}>
            <Undo2 size={16} aria-hidden /> Undo
          </button>
          <button onClick={() => load(START_FEN)}>
            <RotateCcw size={16} aria-hidden /> Reset
          </button>
          <button onClick={() => setOrientation(orientation === 'white' ? 'black' : 'white')}>
            <Repeat size={16} aria-hidden /> Flip
          </button>
          <button onClick={playFromHere} disabled={position.isGameOver()}>
            <Play size={16} aria-hidden /> Play from here
          </button>
        </div>

        <div className="fen-row">
          <label className="field">
            <span>Current FEN</span>
            <input readOnly value={fen} onFocus={(e) => e.target.select()} />
          </label>
          <label className="field">
            <span>Load a position (FEN)</span>
            <div className="input-row">
              <input
                value={fenInput}
                placeholder="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
                onChange={(e) => setFenInput(e.target.value)}
                onKeyDown={(e) => e.key === 'Enter' && loadInput()}
              />
              <button onClick={loadInput} disabled={!fenInput.trim()}>
                <Upload size={16} aria-hidden /> Load
              </button>
            </div>
          </label>
        </div>
      </aside>
    </div>
  );
}
