import type { Color, MoveInfo, Score } from './api.ts';

/** Destination squares of the legal moves that start on `from`. */
export function legalTargets(legalMoves: string[], from: string): string[] {
  return [...new Set(legalMoves.filter((m) => m.startsWith(from)).map((m) => m.slice(2, 4)))];
}

/** The UCI move for a from/to pair, "promotion" when a piece must be chosen, or null if illegal. */
export function resolveMove(legalMoves: string[], from: string, to: string): string | 'promotion' | null {
  const matches = legalMoves.filter((m) => m.slice(0, 4) === from + to);
  if (matches.length === 0) return null;
  return matches.length > 1 || matches[0].length === 5 ? 'promotion' : matches[0];
}

export function formatScore(score: Score | null): string {
  if (!score) return '—';
  if (score.mate !== null) return score.mate > 0 ? `M${score.mate}` : `-M${-score.mate}`;
  const pawns = (score.cp ?? 0) / 100;
  return `${pawns > 0 ? '+' : ''}${pawns.toFixed(2)}`;
}

/** White's share of the evaluation bar, 0-100, using the same logistic scale as the engine. */
export function whiteShare(score: Score | null): number {
  if (!score) return 50;
  if (score.mate !== null) return score.mate > 0 ? 100 : 0;
  return 100 / (1 + Math.exp(-(score.cp ?? 0) / 400));
}

const RESULT_TEXT: Record<string, string> = {
  checkmate: 'Checkmate',
  stalemate: 'Stalemate',
  insufficient_material: 'Draw by insufficient material',
  threefold_repetition: 'Draw by threefold repetition',
  fivefold_repetition: 'Draw by fivefold repetition',
  fifty_moves: 'Draw by the fifty-move rule',
  seventyfive_moves: 'Draw by the seventy-five-move rule',
  resigned: 'You resigned',
};

/** A sentence for the game status from the player's point of view. */
export function statusText(status: string, result: string | null, player: Color): string {
  if (status === 'active') return '';
  const reason = RESULT_TEXT[status] ?? status.replaceAll('_', ' ');
  if (result === '1/2-1/2' || !result) return reason;
  const playerWon = (result === '1-0') === (player === 'white');
  return status === 'resigned' ? reason : `${reason} — ${playerWon ? 'you win!' : 'the engine wins'}`;
}

/** Move list rows: [move number, white SAN, black SAN]. Handles games that start with Black to move. */
export function movePairs(moves: MoveInfo[], startFen: string): [number, string, string][] {
  const fields = startFen.split(' ');
  const blackFirst = fields[1] === 'b';
  let number = Number(fields[5] ?? 1) || 1;
  const sans = moves.map((m) => m.san);
  if (blackFirst) sans.unshift('…');
  const rows: [number, string, string][] = [];
  for (let i = 0; i < sans.length; i += 2) rows.push([number++, sans[i], sans[i + 1] ?? '']);
  return rows;
}

/** Square of the given king, read from the FEN placement field. */
export function kingSquare(fen: string, color: Color): string | null {
  const king = color === 'white' ? 'K' : 'k';
  const ranks = fen.split(' ')[0].split('/');
  for (let r = 0; r < 8; r++) {
    let file = 0;
    for (const ch of ranks[r] ?? '') {
      if (ch >= '1' && ch <= '8') file += Number(ch);
      else if (ch === king) return 'abcdefgh'[file] + String(8 - r);
      else file++;
    }
  }
  return null;
}

/** Squares that hold a piece, read from the FEN placement field. */
export function occupiedSquares(fen: string): Set<string> {
  const squares = new Set<string>();
  fen.split(' ')[0].split('/').forEach((rank, r) => {
    let file = 0;
    for (const ch of rank) {
      if (ch >= '1' && ch <= '8') file += Number(ch);
      else squares.add('abcdefgh'[file++] + String(8 - r));
    }
  });
  return squares;
}
