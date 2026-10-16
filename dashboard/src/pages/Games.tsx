import { Chess } from 'chess.js';
import { ChevronLeft, ChevronRight, ChevronsLeft, ChevronsRight, Download, Play } from 'lucide-react';
import { useEffect, useState } from 'react';
import { Link, useNavigate, useParams } from 'react-router';
import { api } from '../api.ts';
import type { GameState } from '../api.ts';
import { statusText } from '../chess-utils.ts';
import { Board } from '../components/Board.tsx';
import { MoveList } from '../components/MoveList.tsx';
import { LEVEL_NAMES } from './Play.tsx';

export function Games() {
  const { gameId } = useParams();
  const navigate = useNavigate();
  const [games, setGames] = useState<GameState[] | null>(null);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    api.listGames().then(setGames, (e) => setError(e instanceof Error ? e.message : String(e)));
  }, []);

  const selected = games?.find((g) => g.id === Number(gameId)) ?? null;

  return (
    <div className="games-layout">
      <section className="panel">
        <h1>Games</h1>
        <p className="muted">Games played since the server started.</p>
        {error && <p className="error">{error}</p>}
        {games?.length === 0 && (
          <p>
            No games yet. <Link to="/play">Play one</Link>.
          </p>
        )}
        <ul className="game-list">
          {games?.map((g) => (
            <li key={g.id}>
              <button className={g.id === selected?.id ? 'selected' : ''} onClick={() => navigate(`/games/${g.id}`)}>
                <span className="game-title">Game #{g.id}</span>
                <span className="muted">
                  {new Date(g.created * 1000).toLocaleTimeString()} · you played {g.player_color} · level {g.level}
                </span>
                <span className={`result ${g.status === 'active' ? 'live' : ''}`}>
                  {g.status === 'active' ? 'In progress' : g.result} · {g.moves.length} plies
                </span>
              </button>
            </li>
          ))}
        </ul>
      </section>
      {selected ? <Replay key={selected.id} game={selected} /> : games && games.length > 0 && (
        <section className="panel muted">Select a game to replay it.</section>
      )}
    </div>
  );
}

function Replay({ game }: { game: GameState }) {
  const [ply, setPly] = useState(game.moves.length);
  const clamp = (p: number) => Math.max(0, Math.min(game.moves.length, p));

  useEffect(() => {
    const onKey = (e: KeyboardEvent) => {
      if (e.key === 'ArrowLeft') setPly((p) => clamp(p - 1));
      if (e.key === 'ArrowRight') setPly((p) => clamp(p + 1));
    };
    window.addEventListener('keydown', onKey);
    return () => window.removeEventListener('keydown', onKey);
  });

  const chess = new Chess(game.start_fen);
  for (const move of game.moves.slice(0, ply)) {
    chess.move({ from: move.uci.slice(0, 2), to: move.uci.slice(2, 4), promotion: move.uci[4] });
  }

  return (
    <div className="game-layout replay">
      <div className="board-column">
        <Board
          fen={chess.fen()}
          orientation={game.player_color}
          legalMoves={[]}
          lastMove={ply > 0 ? game.moves[ply - 1].uci : null}
          inCheck={chess.inCheck()}
        />
      </div>
      <aside className="panel side">
        <h2>Game #{game.id}</h2>
        <p className="muted">
          You played {game.player_color} against level {game.level} ({LEVEL_NAMES[game.level - 1]})
        </p>
        <p>
          <strong>{game.status === 'active' ? 'In progress' : statusText(game.status, game.result, game.player_color)}</strong>
        </p>
        <div className="button-row replay-controls" role="group" aria-label="Replay controls">
          <button onClick={() => setPly(0)} disabled={ply === 0} aria-label="First move"><ChevronsLeft size={18} /></button>
          <button onClick={() => setPly(clamp(ply - 1))} disabled={ply === 0} aria-label="Previous move"><ChevronLeft size={18} /></button>
          <span className="ply-counter">{ply} / {game.moves.length}</span>
          <button onClick={() => setPly(clamp(ply + 1))} disabled={ply === game.moves.length} aria-label="Next move"><ChevronRight size={18} /></button>
          <button onClick={() => setPly(game.moves.length)} disabled={ply === game.moves.length} aria-label="Last move"><ChevronsRight size={18} /></button>
        </div>
        <MoveList moves={game.moves} startFen={game.start_fen} activePly={ply} onSelectPly={setPly} />
        <div className="button-row">
          <a className="button" href={api.pgnUrl(game.id)} download={`game-${game.id}.pgn`}>
            <Download size={16} aria-hidden /> Download PGN
          </a>
          {game.status === 'active' && (
            <Link className="button primary" to={`/play/${game.id}`}>
              <Play size={16} aria-hidden /> Continue
            </Link>
          )}
          <Link className="button" to={`/analysis?fen=${encodeURIComponent(chess.fen())}`}>Analyse this position</Link>
        </div>
      </aside>
    </div>
  );
}
