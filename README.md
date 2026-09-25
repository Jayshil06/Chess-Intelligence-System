# ♟️ Chess Intelligence System

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg?style=flat-square&logo=c%2B%2B)](https://en.cppreference.com/w/cpp/23)
[![CMake](https://img.shields.io/badge/CMake-3.25%2B-064F8C.svg?style=flat-square&logo=cmake)](https://cmake.org/)
[![CI](https://github.com/Jayshil06/Chess-Intelligence-System/actions/workflows/ci.yml/badge.svg)](https://github.com/Jayshil06/Chess-Intelligence-System/actions/workflows/ci.yml)
[![Tests](https://img.shields.io/badge/Tests-122%2F122%20Passing-brightgreen.svg?style=flat-square)](./engine/tests)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg?style=flat-square)](./LICENSE)

A chess intelligence platform built around a **perft-verified C++23 bitboard engine** that speaks UCI.
The engine tier and the Python data pipeline (PGN ingestion, features, self-play/Elo/SPRT) are implemented
and tested. Analytics, machine learning, the API, and the dashboard are designed and scaffolded, and are being built next (see the [roadmap](#️-implementation-roadmap)).

---

## 🏛️ Architecture

```text
                               CHESS INTELLIGENCE SYSTEM
                                          |
                     +--------------------+--------------------+
                     |                                         |
                     v                                         v
         C++ ENGINE CORE  [built]              PYTHON PLATFORM  [in progress]
                     |                                         |
             +-------+--------+                    +-----------+-----------+
             |       |        |                    |           |           |
           Board   Search    UCI                 Data       Analytics      ML
             |       |        |                Pipeline       |           |
             |       |        |               [built]     [planned]   [planned]
         Bitboard   PVS      Engine               PGN        EDA        PyTorch
         Mailbox    TT       Thread             Features    Stats        NNUE
             |       |        |                    |           |           |
             +-------+--------+                    +-----------+-----------+
                     |                                         |
                     +------------------+----------------------+
                                        |
                                        v
                             SELF-PLAY / ELO  [built]
                                        |
                                        v
                             FASTAPI SERVICE  [planned]
                                        |
                                        v
                          REACT / TYPESCRIPT UI  [planned]
```

### ✅ Built: C++ Engine Core (`engine/`)
- 64-bit bitboards with a mailbox for O(1) piece lookup; copy-free legality checks.
- Slider attacks from PEXT-indexed tables (BMI2) or fancy magic bitboards, validated bit-for-bit against a ray-scan reference.
- Legal move generation verified by perft on the five standard positions (CI gate).
- Principal Variation Search with iterative deepening, quiescence search with delta pruning, check extension, and a Zobrist-keyed transposition table.
- Move ordering: TT move, MVV-LVA captures, killer moves, and history heuristic.
- Draw detection: threefold repetition, fifty-move rule, and insufficient material.
- Tapered material + piece-square evaluation, updated incrementally on every move.
- UCI protocol with a threaded, interruptible search, clock-based time management, and `bench`/`perft` commands.

### ✅ Built: Python Data Platform (`python/`)
- **`chess_data`**: streaming PGN ingestion (plain, `.gz`, `.bz2`, `.zst`) at constant memory. Rejects games with illegal moves, variants, missing results, or players below an Elo floor, and writes one Parquet row per position.
- **`features`**: deterministic features per position (material, bishop pair, mobility, centre control, doubled/isolated/passed pawns, king shield and king-zone attacks, phase), streamed Parquet to Parquet.
- **`selfplay`**: UCI engine matches with paired openings and real clocks (time forfeits and illegal moves are scored). Reports Elo with a 95% confidence interval and stops early on an SPRT decision.

### 🗓️ Planned (scaffolding only today)
- **Analytics and ML (`python/analytics`, `python/models`)**: opening and player statistics, classical ML baselines, and PyTorch NNUE training.
- **NNUE inference in the engine**: not started.
- **FastAPI service (`api/`)**: engine-backed REST endpoints for evaluation, best move, and blunder detection. Currently a package stub.
- **React dashboard (`dashboard/`)**: interactive board, evaluation graph, and PV stream. Currently `package.json` only.

---

## 📂 Repository Structure

```text
Chess-Intelligence-System/
├── CMakeLists.txt           # Root CMake configuration
├── .github/workflows/ci.yml # CI: Release build + tests + perft gate, ASan/UBSan job
│
├── engine/                  # C++23 chess engine  [built]
│   ├── CMakeLists.txt       # chess_core library, chess_engine, chess_bench, chess_tests
│   ├── include/             # board/, move/, search/, evaluation/, protocol/
│   ├── src/                 # Implementations; main.cpp is the UCI entry point
│   ├── bench/               # chess_bench: perft gate, movegen and search benchmarks
│   └── tests/               # GoogleTest suite: unit/, perft/, search/
│
├── python/                  # chess_data, features, selfplay [built]; analytics, models [planned]
├── api/                     # FastAPI service      [planned, package stub]
└── dashboard/               # React + TypeScript   [planned, package.json only]
```

---

## 🚀 Getting Started

### Prerequisites
- **C++ Compiler**: C++23 compliant (GCC 14+, Clang 17+, or MSVC 2022+)
- **Build System**: CMake 3.25+ and Ninja

### Building and Testing the C++ Engine

#### 1. Configure and Build
```bash
cmake -B build_release -G Ninja            # Release is the default build type
cmake --build build_release

# Optional: optimize for this machine (enables PEXT slider lookups on BMI2 CPUs)
cmake -B build_native -G Ninja -DCHESS_NATIVE=ON
```
GoogleTest is downloaded over verified TLS and pinned by SHA-256. Behind a TLS-intercepting proxy,
point CMake at your CA bundle with `-DCMAKE_CA_FILE=/path/to/ca.pem`; do not disable verification.

#### 2. Run the Test Suite (122 Tests)
```bash
ctest --test-dir build_release --output-on-failure
```

#### 3. Run the Engine (UCI)
The binary speaks UCI on stdin/stdout, so it can be loaded into any UCI GUI (Arena, Cute Chess, BanksiaGUI).
```bash
./build_release/engine/chess_engine
uci
position startpos moves e2e4 e7e5
go movetime 1000        # also: depth N, nodes N, wtime/btime/winc/binc/movestogo, infinite
```
Extra commands: `d` (print FEN), `perft <depth>` (move-by-move node counts), `bench [depth]`,
and `setoption name Hash value <MB>`.

#### 4. Benchmark and Perft Gate
```bash
./build_release/engine/chess_bench            # exits non-zero if any perft count is wrong
./build_release/engine/chess_engine bench     # deterministic search node signature
```
Measured before/after numbers are recorded in [`engine/bench/BASELINE.md`](engine/bench/BASELINE.md).

### Python Data Platform

```bash
python -m venv .venv && source .venv/bin/activate      # Windows: .venv\Scripts\activate
pip install -e "python[dev,zstd]"
python python/run_tests.py                              # match tests use the engine if it is built

chess-ingest games.pgn.zst positions.parquet --min-elo 2000 --skip-plies 8
chess-features positions.parquet features.parquet
chess-match ./new_engine ./base_engine --games 400 --tc 10+0.1 --sprt 0 5 --pgn match.pgn
```
Ingestion runs at roughly 220 games/s and feature extraction at roughly 11K positions/s on one core.

---

## 🗺️ Implementation Roadmap

- [x] **Steps 1–6 — Board & State Foundation**: Multi-language workspace, C++23 build, square mapping, bitboards, position state, and FEN parsing.
- [x] **Steps 7–10 — Attack Tables & Move Encoding**: King/Knight/Pawn/Sliding attack generation and compact 32-bit Move encoding.
- [x] **Steps 11–16 — Movegen, State Transitions & Perft Hard Gate**: Legal move filtering, castling/EP state, make/unmake, Zobrist hashing, and verified Perft test suite.
- [x] **Steps 17–20 — Evaluation & Search Core**: Classical material/PST evaluation, Negamax, Alpha-Beta pruning, Quiescence, and Iterative Deepening.
- [x] **Steps 21–25 — Search Optimizations & UCI Protocol**: Transposition Table, move ordering heuristics, UCI protocol handler, and engine benchmarks.
- [ ] **Steps 26–34 — Python Data Platform, ML Baselines & PyTorch NNUE**: PGN pipeline, feature engineering, analytics, ML baselines, and NNUE training.
  - [x] PGN ingestion pipeline, feature engineering, and self-play/Elo/SPRT harness (moved ahead of ML so every engine change can be measured)
  - [ ] Analytics, classical ML baselines, PyTorch NNUE training
- [ ] **Steps 35–38 — FastAPI Service, React Dashboard & Portfolio Release**: Web microservice, interactive UI, self-play ELO evaluation, and portfolio release.

---

## 🤝 Contributing

Open-source contributors are warmly welcomed! Whether it's optimization, bug fixes, algorithmic improvements, or documentation enhancements:

1. **Fork the Project**
2. **Create your Feature Branch** (`git checkout -b feat/AmazingFeature`)
3. **Commit your Changes** (`git commit -m 'feat: add some AmazingFeature'`)
4. **Push to the Branch** (`git push origin feat/AmazingFeature`)
5. **Open a Pull Request**

---

## 📜 License

This project is licensed under the [MIT License](LICENSE).

---

## 👨‍💻 Author

Developed by **Jayshil**.
