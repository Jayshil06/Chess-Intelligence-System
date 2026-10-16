import { useEffect, useRef } from 'react';
import type { MoveInfo } from '../api.ts';
import { movePairs } from '../chess-utils.ts';

interface MoveListProps {
  moves: MoveInfo[];
  startFen: string;
  /** Number of moves played in the shown position; highlights the last of them. */
  activePly?: number;
  onSelectPly?: (ply: number) => void;
}

export function MoveList({ moves, startFen, activePly = moves.length, onSelectPly }: MoveListProps) {
  const listRef = useRef<HTMLOListElement>(null);
  const offset = startFen.split(' ')[1] === 'b' ? 1 : 0;

  useEffect(() => {
    listRef.current?.querySelector('.active')?.scrollIntoView({ block: 'nearest' });
  }, [activePly, moves.length]);

  if (moves.length === 0) return <p className="muted">No moves yet.</p>;

  const cell = (san: string, index: number) => {
    const ply = index - offset + 1;
    if (san === '…' || san === '') return <span className="move placeholder">{san}</span>;
    const className = `move ${ply === activePly ? 'active' : ''}`;
    return onSelectPly ? (
      <button className={className} onClick={() => onSelectPly(ply)}>
        {san}
      </button>
    ) : (
      <span className={className}>{san}</span>
    );
  };

  return (
    <ol className="move-list" ref={listRef}>
      {movePairs(moves, startFen).map(([number, white, black], row) => (
        <li key={number}>
          <span className="move-number">{number}.</span>
          {cell(white, row * 2)}
          {cell(black, row * 2 + 1)}
        </li>
      ))}
    </ol>
  );
}
