# Engine Benchmark Baseline

Output of `chess_bench` for the audited commit `29ed6a8` (before) and the current tree (after).
Both were compiled with the same toolchain and flags (Clang 21 via `zig c++`, `-O3`, portable build
without `-march=native`) and run on the same machine: Intel Core i7-12700, single thread.
Absolute numbers depend on hardware; compare ratios.

Re-run with `./build_release/engine/chess_bench`. It exits non-zero if any perft count differs.

## Perft (correctness gate, unchanged)

| Position | Depth | Nodes | Before (Mnps) | After (Mnps) |
|---|---|---|---|---|
| startpos | 5 | 4,865,609 | 17.6 | 26.9 |
| kiwipete | 4 | 4,085,603 | 16.9 | 27.6 |
| pos3 | 6 | 11,030,083 | 14.2 | 22.2 |
| pos4 | 5 | 15,833,292 | 17.1 | 26.5 |
| pos5 | 4 | 2,103,487 | 16.6 | 27.4 |

## Legal move generation

| Case | Before (calls/s) | After (calls/s) |
|---|---|---|
| startpos | 1,151,027 | 1,709,507 |
| kiwipete | 520,974 | 953,292 |
| startpos after 0 plies of history | 900,242 | 1,744,616 |
| startpos after 8 plies of history | 556,558 | 1,722,637 |
| startpos after 40 plies of history | 560,212 | 1,705,252 |
| startpos after 80 plies of history | 525,486 | 1,685,799 |

The history rows reach the same position through knight shuffles. Before, throughput fell 42% once
history existed (legality checks copied the `Position` and its history vector); after, it is flat.

## Fixed-depth search from startpos

| Depth | Before nodes | Before ms | After nodes | After ms |
|---|---|---|---|---|
| 4 | 53,992 | 38.6 | 4,174 | 1.0 |
| 5 | 481,531 | 335.1 | 26,240 | 3.1 |
| 6 | 3,928,399 | 2,930.9 | 84,695 | 17.6 |
| 7 | 36,506,574 | ~21,300 (audit) | 496,404 | 77 (UCI `go depth 7`) |

## Time control (`max_time_ms = 100`)

| Position | Before actual ms | After actual ms |
|---|---|---|
| kiwipete | 28,551 (depth 2, 64.7M nodes) | 100.3 (depth 5) |
| pos4 | 7,019 (depth 2) | 93.6 (depth 6) |
| pos5 | 153 (depth 2) | 51.9 (depth 6) |

## Component measurements

| Measurement | Before | After |
|---|---|---|
| 10M `rook_attacks` calls | 195.5 ms (ray loop) | 6.0 ms (magic), 5.9 ms (PEXT) |
| 10M `bishop_attacks` calls | 162.5 ms | 4.6 ms |
| `evaluate()` per call | 76.9 ns (full rescan) | 1.2 ns (incremental) |
| `chess_engine bench 7` (6 positions) | not measurable (kiwipete did not finish depth 4 in 5 min) | 9,728,759 nodes, ~1,495 ms, 6.5M nps |

## UCI acceptance

| Check | Target | Result |
|---|---|---|
| `go movetime 1000` on kiwipete | within 1,100 ms | 1,001 ms |
| `stop` during `go infinite` | within 100 ms | 0.1 ms |
| Self-play game, 10 s + 0.1 s per side | no forfeit, no illegal move | 131 plies to checkmate, 2.2–2.9 s left, no errors |
