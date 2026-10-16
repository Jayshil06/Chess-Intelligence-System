import assert from 'node:assert/strict';
import { test } from 'node:test';
import {
  formatScore,
  kingSquare,
  legalTargets,
  movePairs,
  occupiedSquares,
  resolveMove,
  statusText,
  whiteShare,
} from './chess-utils.ts';

const START = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1';

test('legal targets and move resolution', () => {
  const legal = ['e2e3', 'e2e4', 'g1f3', 'b7b8q', 'b7b8r', 'b7b8b', 'b7b8n'];
  assert.deepEqual(legalTargets(legal, 'e2'), ['e3', 'e4']);
  assert.equal(resolveMove(legal, 'e2', 'e4'), 'e2e4');
  assert.equal(resolveMove(legal, 'e2', 'e5'), null);
  assert.equal(resolveMove(legal, 'b7', 'b8'), 'promotion');
});

test('score formatting and evaluation bar', () => {
  assert.equal(formatScore({ cp: 35, mate: null }), '+0.35');
  assert.equal(formatScore({ cp: -120, mate: null }), '-1.20');
  assert.equal(formatScore({ cp: null, mate: 3 }), 'M3');
  assert.equal(formatScore({ cp: null, mate: -2 }), '-M2');
  assert.equal(formatScore(null), '—');
  assert.equal(whiteShare({ cp: 0, mate: null }), 50);
  assert.ok(whiteShare({ cp: 400, mate: null }) > 70);
  assert.equal(whiteShare({ cp: null, mate: -1 }), 0);
});

test('status text from the player view', () => {
  assert.equal(statusText('active', null, 'white'), '');
  assert.equal(statusText('checkmate', '1-0', 'white'), 'Checkmate — you win!');
  assert.equal(statusText('checkmate', '1-0', 'black'), 'Checkmate — the engine wins');
  assert.equal(statusText('stalemate', '1/2-1/2', 'white'), 'Stalemate');
  assert.equal(statusText('resigned', '0-1', 'white'), 'You resigned');
});

test('move pairs handle a black first move', () => {
  const moves = [
    { uci: 'e2e4', san: 'e4' },
    { uci: 'e7e5', san: 'e5' },
    { uci: 'g1f3', san: 'Nf3' },
  ];
  assert.deepEqual(movePairs(moves, START), [[1, 'e4', 'e5'], [2, 'Nf3', '']]);
  const fromBlack = movePairs([{ uci: 'e7e5', san: 'e5' }], '8/8/8/8/8/8/8/K6k b - - 0 12');
  assert.deepEqual(fromBlack, [[12, '…', 'e5']]);
});

test('king square from FEN', () => {
  assert.equal(kingSquare(START, 'white'), 'e1');
  assert.equal(kingSquare(START, 'black'), 'e8');
  assert.equal(kingSquare('6k1/8/8/8/8/8/8/K7 w - - 0 1', 'black'), 'g8');
});

test('occupied squares from FEN', () => {
  const occupied = occupiedSquares('6k1/8/8/8/8/8/8/K7 w - - 0 1');
  assert.deepEqual([...occupied].sort(), ['a1', 'g8']);
  assert.equal(occupiedSquares(START).size, 32);
});
