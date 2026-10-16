import { Chess, validateFen } from 'chess.js';
import { LoaderCircle, Microscope, Play, Repeat, RotateCcw, Undo2, Upload } from 'lucide-react';
import { useEffect, useRef, useState } from 'react';
import type { Arrow } from 'react-chessboard';
import { useNavigate, useSearchParams } from 'react-router';
import { api } from '../api.ts';
import type { Analysis as AnalysisResult, Color, MoveInfo } from '../api.ts';
import { formatScore } from '../chess-utils.ts';
import { Board } from '../components/Board.tsx';
import { EvalBar } from '../components/EvalBar.tsx';
import { MoveList } from '../components/MoveList.tsx';

const START_FEN = new Chess().fen();
const THINK_TIMES = [250, 500, 1000, 2000, 5000];

function legalUci(fen: string): string[] {
  return new Chess(fen).moves({ verbose: true }).map((m) => m.from + m.to + (m.promotion ?? ''));
}

export function Analysis() {
  const navigate = useNavigate();
  const [params] = useSearchParams();
  const initial = params.get('fen');
  const [startFen, setStartFen] = useState(initial && validateFen(initial).ok ? initial : START_FEN);
  const [moves, setMoves] = useState<MoveInfo[]>([]);
  const [fens, setFens] = useState<string[]>([startFen]);
  const [fenInput, setFenInput] = useState('');
  const [orientation, setOrientation] = useState<Color>('white');
  const [thinkMs, setThinkMs] = useState(1000);
  const [auto, setAuto] = useState(true);
  const [result, setResult] = useState<AnalysisResult | null>(null);
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const latest = useRef(0);

  const fen = fens.at(-1)!;
  const position = new Chess(fen);

  async function analyse(target = fen) {
    const request = ++latest.current;
    setBusy(true);
    setError(null);
    try {
      const analysis = await api.analyze(target, thinkMs);
      if (request === latest.current) setResult(analysis);
    } catch (e) {
      if (request === latest.current) setError(e instanceof Error ? e.message : String(e));
    } finally {
      if (request === latest.current) setBusy(false);
    }
  }

  useEffect(() => {
    setResult(null);
    if (auto) void analyse(fen);
    // Re-analyse when the position, think time or auto mode changes
  }, [fen, thinkMs, auto]);

  function onMove(uci: string) {
    const chess = new Chess(fen);
    const move = chess.move({ from: uci.slice(0, 2), to: uci.slice(2, 4), promotion: uci[4] });
    setMoves([...moves, { uci, san: move.san }]);
    setFens([...fens, chess.fen()]);
  }

  function load(newFen: string) {
    setStartFen(newFen);
    setMoves([]);
    setFens([newFen]);
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

  const best = result?.fen === fen ? result.best_move : null;
  const arrows: Arrow[] = best
    ? [{ startSquare: best.uci.slice(0, 2), endSquare: best.uci.slice(2, 4), color: 'rgba(64, 156, 255, 0.85)' }]
    : [];

  return (
    <div className="game-layout">
      <div className="board-column">
        <EvalBar score={result?.fen === fen ? result.score : null} orientation={orientation} />
        <Board
          fen={fen}
          orientation={orientation}
          legalMoves={legalUci(fen)}
          onMove={onMove}
          lastMove={moves.at(-1)?.uci}
          inCheck={position.inCheck()}
          arrows={arrows}
        />
      </div>

      <aside className="panel side">
        <h1>Analysis board</h1>
        <p className="muted">Move pieces for either side. The engine evaluates the position on the board.</p>

        <div className="analysis-result" aria-live="polite">
          {result?.fen === fen && result.status !== 'active' ? (
            <strong>Game over: {result.status.replaceAll('_', ' ')}</strong>
          ) : result?.fen === fen ? (
            <>
              <div className="score">{formatScore(result.score)}</div>
              <div>
                Best move <strong>{result.best_move?.san ?? '—'}</strong>
                <span className="muted"> · depth {result.depth} · {result.nodes.toLocaleString()} nodes</span>
              </div>
              <div className="pv" title="Principal variation">
                {result.pv.map((m) => m.san).join(' ')}
              </div>
            </>
          ) : busy ? (
            <span>
              <LoaderCircle className="spin" size={16} aria-hidden /> Analysing…
            </span>
          ) : (
            <span className="muted">Press Analyse to evaluate this position.</span>
          )}
        </div>

        <div className="button-row">
          <button className="primary" onClick={() => analyse()} disabled={busy}>
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

        <MoveList moves={moves} startFen={startFen} />

        <div className="button-row">
          <button onClick={() => { setMoves(moves.slice(0, -1)); setFens(fens.slice(0, -1)); }} disabled={moves.length === 0}>
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
