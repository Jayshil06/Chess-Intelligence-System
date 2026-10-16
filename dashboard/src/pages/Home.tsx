import { Cpu, History, Microscope, Swords } from 'lucide-react';
import { useEffect, useState } from 'react';
import { Link } from 'react-router';
import { api } from '../api.ts';

const CARDS = [
  { to: '/play', icon: Swords, title: 'Play', text: 'Play a full game against the engine at one of eight strength levels.' },
  { to: '/analysis', icon: Microscope, title: 'Analysis', text: 'Set up any position and see the evaluation, best move and main line.' },
  { to: '/games', icon: History, title: 'Games', text: 'Replay your games move by move and download them as PGN.' },
  { to: '/engine', icon: Cpu, title: 'Engine', text: 'See how the C++ engine searches and evaluates positions.' },
];

export function Home() {
  const [engine, setEngine] = useState<string | null>(null);
  const [offline, setOffline] = useState(false);

  useEffect(() => {
    api.health().then((h) => setEngine(h.engine), () => setOffline(true));
  }, []);

  return (
    <>
      <section className="hero">
        <h1>Play chess against a hand-built engine</h1>
        <p>
          A C++23 bitboard engine with principal variation search, a transposition table and an optional NNUE
          evaluation, served over UCI.
        </p>
        <div className="button-row">
          <Link className="button primary large" to="/play">
            <Swords size={18} aria-hidden /> Start a game
          </Link>
          <Link className="button large" to="/analysis">
            <Microscope size={18} aria-hidden /> Analyse a position
          </Link>
        </div>
        <p className={`status-pill ${offline ? 'offline' : engine ? 'online' : ''}`} role="status">
          {offline ? 'Engine API offline: start it with uvicorn (see README)' : engine ? `Engine online: ${engine}` : 'Checking engine…'}
        </p>
      </section>

      <section className="cards">
        {CARDS.map(({ to, icon: Icon, title, text }) => (
          <Link key={to} to={to} className="card">
            <Icon size={26} aria-hidden />
            <h2>{title}</h2>
            <p>{text}</p>
          </Link>
        ))}
      </section>
    </>
  );
}
