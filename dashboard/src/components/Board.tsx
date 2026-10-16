import { useState } from 'react';
import type { CSSProperties } from 'react';
import { Chessboard } from 'react-chessboard';
import type { Arrow, PieceDropHandlerArgs, SquareHandlerArgs } from 'react-chessboard';
import type { Color } from '../api.ts';
import { kingSquare, legalTargets, occupiedSquares, resolveMove } from '../chess-utils.ts';

interface BoardProps {
  fen: string;
  orientation: Color;
  /** UCI moves the user may play right now; an empty list makes the board read-only. */
  legalMoves: string[];
  onMove?: (uci: string) => void;
  lastMove?: string | null;
  inCheck?: boolean;
  arrows?: Arrow[];
}

const PROMOTIONS = [
  { piece: 'q', white: '♕', black: '♛', label: 'Queen' },
  { piece: 'r', white: '♖', black: '♜', label: 'Rook' },
  { piece: 'b', white: '♗', black: '♝', label: 'Bishop' },
  { piece: 'n', white: '♘', black: '♞', label: 'Knight' },
];

const HIGHLIGHT = 'rgba(255, 214, 10, 0.4)';
const SELECTED = 'rgba(255, 214, 10, 0.65)';

export function Board({ fen, orientation, legalMoves, onMove, lastMove, inCheck = false, arrows = [] }: BoardProps) {
  // Selection belongs to one position; a new FEN implicitly clears it
  const [selection, setSelection] = useState<{ fen: string; square: string } | null>(null);
  const [promotion, setPromotion] = useState<{ from: string; to: string } | null>(null);
  const selected = selection?.fen === fen ? selection.square : null;
  const targets = selected ? legalTargets(legalMoves, selected) : [];
  const occupied = occupiedSquares(fen);
  const turn: Color = fen.split(' ')[1] === 'b' ? 'black' : 'white';

  function attempt(from: string, to: string): boolean {
    const move = resolveMove(legalMoves, from, to);
    if (!move) return false;
    setSelection(null);
    if (move === 'promotion') {
      setPromotion({ from, to });
      return false;
    }
    onMove?.(move);
    return true;
  }

  function onSquareClick({ square }: SquareHandlerArgs) {
    if (selected && targets.includes(square)) attempt(selected, square);
    else if (square !== selected && legalTargets(legalMoves, square).length > 0) setSelection({ fen, square });
    else setSelection(null);
  }

  const squareStyles: Record<string, CSSProperties> = {};
  if (lastMove) {
    squareStyles[lastMove.slice(0, 2)] = { background: HIGHLIGHT };
    squareStyles[lastMove.slice(2, 4)] = { background: HIGHLIGHT };
  }
  const checkedKing = inCheck ? kingSquare(fen, turn) : null;
  if (checkedKing) {
    squareStyles[checkedKing] = { background: 'radial-gradient(circle, rgba(255,40,40,0.85) 25%, rgba(255,40,40,0) 75%)' };
  }
  if (selected) squareStyles[selected] = { background: SELECTED };
  for (const target of targets) {
    squareStyles[target] = occupied.has(target)
      ? { background: 'radial-gradient(circle, transparent 58%, rgba(0,0,0,0.3) 60%)' }
      : { background: 'radial-gradient(circle, rgba(0,0,0,0.28) 20%, transparent 22%)' };
  }

  return (
    <div className="board-wrap">
      <Chessboard
        options={{
          id: 'main-board',
          position: fen,
          boardOrientation: orientation,
          squareStyles,
          arrows,
          animationDurationInMs: 180,
          lightSquareStyle: { backgroundColor: 'var(--square-light)' },
          darkSquareStyle: { backgroundColor: 'var(--square-dark)' },
          canDragPiece: ({ square }) => square !== null && legalTargets(legalMoves, square).length > 0,
          onPieceDrag: ({ square }) => square && setSelection({ fen, square }),
          onPieceDrop: ({ sourceSquare, targetSquare }: PieceDropHandlerArgs) =>
            targetSquare !== null && attempt(sourceSquare, targetSquare),
          onSquareClick,
        }}
      />
      {promotion && (
        <div className="promotion" role="dialog" aria-label="Choose promotion piece">
          <span>Promote to</span>
          <div className="promotion-choices">
            {PROMOTIONS.map((p) => (
              <button
                key={p.piece}
                title={p.label}
                aria-label={p.label}
                onClick={() => {
                  onMove?.(promotion.from + promotion.to + p.piece);
                  setPromotion(null);
                }}
              >
                {turn === 'white' ? p.white : p.black}
              </button>
            ))}
          </div>
          <button className="link" onClick={() => setPromotion(null)}>
            Cancel
          </button>
        </div>
      )}
    </div>
  );
}
