import { useEffect, useState } from 'react';
import { api } from '../api.ts';
import type { EngineInfo } from '../api.ts';
import { LEVEL_NAMES } from './Play.tsx';

const FEATURES = [
  ['Board', 'Bitboards with a mailbox; slider attacks from PEXT or magic lookup tables. Move generation is verified by perft.'],
  ['Search', 'Principal variation search with iterative deepening, a transposition table, quiescence search with delta pruning, and check extensions.'],
  ['Move ordering', 'Transposition-table move, MVV-LVA captures, killer moves, and the history heuristic.'],
  ['Evaluation', 'Tapered material and piece-square tables updated on every move, or an NNUE network loaded at runtime.'],
  ['Draws', 'Threefold repetition, the fifty-move rule, and insufficient material.'],
];

export function Engine() {
  const [info, setInfo] = useState<EngineInfo | null>(null);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    api.engine().then(setInfo, (e) => setError(e instanceof Error ? e.message : String(e)));
  }, []);

  return (
    <div className="stack">
      <section className="panel">
        <h1>Engine</h1>
        {error && <p className="error">{error}</p>}
        {info && (
          <dl className="facts">
            <dt>Name</dt>
            <dd>{info.name}</dd>
            <dt>Evaluation</dt>
            <dd>{info.evaluation === 'nnue' ? `NNUE (${info.eval_file})` : 'Classical (material + piece-square tables)'}</dd>
          </dl>
        )}
      </section>

      <section className="panel">
        <h2>How it plays</h2>
        <dl className="facts">
          {FEATURES.map(([term, text]) => (
            <div key={term} className="fact-row">
              <dt>{term}</dt>
              <dd>{text}</dd>
            </div>
          ))}
        </dl>
      </section>

      {info && (
        <section className="panel">
          <h2>Strength levels</h2>
          <table className="levels">
            <thead>
              <tr><th scope="col">Level</th><th scope="col">Name</th><th scope="col">Search limit</th></tr>
            </thead>
            <tbody>
              {Object.entries(info.levels).map(([level, limit]) => (
                <tr key={level}>
                  <td>{level}</td>
                  <td>{LEVEL_NAMES[Number(level) - 1]}</td>
                  <td><code>{limit}</code></td>
                </tr>
              ))}
            </tbody>
          </table>
        </section>
      )}
    </div>
  );
}
