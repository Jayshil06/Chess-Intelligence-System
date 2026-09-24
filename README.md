# ♟️ Chess Intelligence System

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg?style=flat-square&logo=c%2B%2B)](https://en.cppreference.com/w/cpp/23)
[![CMake](https://img.shields.io/badge/CMake-3.25%2B-064F8C.svg?style=flat-square&logo=cmake)](https://cmake.org/)
[![Python](https://img.shields.io/badge/Python-3.12%2B-3776AB.svg?style=flat-square&logo=python&logoColor=white)](https://www.python.org/)
[![Tests](https://img.shields.io/badge/Tests-117%2F117%20Passing-brightgreen.svg?style=flat-square)](./engine/tests)
[![Contributions Welcome](https://img.shields.io/badge/Contributions-Welcome-brightgreen.svg?style=flat-square)](https://github.com/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg?style=flat-square)](./LICENSE)

A portfolio-grade, high-performance Chess Intelligence System integrating **C++23 systems programming**, **data engineering**, **machine learning baselines**, **NNUE neural evaluation**, **FastAPI microservices**, and an **interactive React dashboard**.

---

## 🏛️ Architecture

```text
                               CHESS INTELLIGENCE SYSTEM
                                          |
                     +--------------------+--------------------+
                     |                                         |
                     v                                         v
               C++ ENGINE CORE                           PYTHON PLATFORM
                     |                                         |
             +-------+--------+                    +-----------+-----------+
             |       |        |                    |           |           |
           Board   Search   UCI                 Data       Analytics      ML
             |       |        |                Pipeline       |           |
             |       |        |                    |           |           |
         Bitboard AlphaBeta   |                  PGN        EDA        PyTorch
             |       |        |                    |           |           |
             |      TT        |                  Features   Stats       NNUE
             |       |        |                    |           |           |
             +-------+--------+                    +-----------+-----------+
                     |                                         |
                     +------------------+----------------------+
                                        |
                                        v
                                 SELF-PLAY / ELO
                                        |
                                        v
                                  FASTAPI SERVICE
                                        |
                                        v
                               REACT / TYPESCRIPT UI
```

### 🧩 System Components

- **C++ Engine Core (`engine/`)**:
  - Pure C++23, zero-overhead 64-bit Bitboard representation.
  - Deterministic move generation, check detection, castling, en-passant, and state rollback stack.
  - Ray-table sliding attacks, mailbox + bitboard hybrid board, copy-free legality checks.
  - Principal Variation Search with iterative deepening, quiescence search, check extension, and a Zobrist-keyed transposition table.
  - Move ordering: TT move, MVV-LVA captures, killer moves, and history heuristic.
  - Draw detection: threefold repetition, fifty-move rule, and insufficient material.
  - Tapered king evaluation (middlegame/endgame piece-square tables).
  - Standard Universal Chess Interface (UCI) protocol with threaded search, time management, and `bench`/`perft` commands.
  - C++ native NNUE inference runtime.

- **Python Intelligence Platform (`python/`)**:
  - Streaming PGN ingestion and position extraction (scalable to millions of games).
  - Deterministic feature engineering (material balance, mobility, pawn structures, king safety, piece activity).
  - Statistical EDA with Polars and DuckDB.
  - Classical ML baselines (Linear Regression, Random Forest, XGBoost, MLP) and PyTorch NNUE training.
  - Self-play tournament runner, ELO calculation, and SPRT statistical testing.

- **FastAPI Service (`api/`)**:
  - High-performance asynchronous REST API bridging the engine process and client over UCI.
  - Endpoints for position evaluation, best move recommendation, blunder detection, and analytics.

- **React Dashboard (`dashboard/`)**:
  - Modern TypeScript frontend with interactive chessboard, evaluation graph, principal variation (PV) stream, and player tendencies.

---

## 📂 Repository Structure

```text
chess-intelligence/
├── CMakeLists.txt           # Root CMake configuration
├── README.md                # Project documentation & architecture overview
├── LICENSE                  # MIT License
├── .gitignore               # Clean git ignore configuration
│
├── engine/                  # C++23 Chess Engine
│   ├── CMakeLists.txt       # Engine & GoogleTest build target
│   ├── include/             # C++ Header files
│   │   ├── board/           # Square, Piece, Color, Bitboard, Position, FEN
│   │   ├── move/            # Move representation & attack tables
│   │   ├── search/          # PVS, iterative deepening, transposition table
│   │   ├── evaluation/      # Classical evaluator
│   │   └── protocol/        # UCI protocol handler
│   ├── src/                 # C++ Implementations
│   │   ├── board/           # types.cpp, bitboard.cpp, position.cpp, fen.cpp
│   │   ├── move/            # attacks, movegen, perft
│   │   ├── search/          # search.cpp, tt.cpp
│   │   ├── evaluation/
│   │   ├── protocol/        # uci.cpp
│   │   └── main.cpp         # UCI engine entry point (`chess_engine bench` for benchmarks)
│   └── tests/               # C++ GoogleTest suite
│       ├── unit/            # Board, movegen, FEN, TT, UCI unit tests
│       ├── perft/           # Perft validation suite
│       └── search/          # Tactical search tests (mates, draws, limits)
│
├── python/                  # Python Data Platform & ML
│   ├── pyproject.toml       # Python package configuration (Python 3.12+)
│   ├── run_tests.py         # Test discovery and execution runner
│   ├── chess_data/          # Streaming PGN ingestion & validation
│   ├── features/            # Material, mobility, king safety, pawn structure features
│   ├── analytics/           # Openings, player tendencies, blunder detection, statistics
│   ├── models/              # Baselines, XGBoost, PyTorch neural networks & NNUE
│   ├── experiments/         # Experiment tracking & reproducible reports
│   ├── selfplay/            # Automated engine self-play & ELO framework
│   └── tests/               # Python unit tests
│
├── api/                     # FastAPI Web Service
│   ├── pyproject.toml
│   └── app/
│
├── dashboard/               # React + TypeScript UI
│   ├── package.json
│   └── src/
│
├── data/                    # Datasets (raw, processed Parquet, features)
├── models/                  # Checkpoints, exported weights, NNUE binaries
├── benchmarks/              # Performance benchmarks and node-rate logs
└── experiments/             # Experiment logs and baseline comparison metrics
```

---

## 🚀 Getting Started

### Prerequisites
- **C++ Compiler**: C++23 compliant (GCC 14+, Clang 17+, or MSVC 2022+)
- **Build System**: CMake 3.25+ and Ninja
- **Python**: Python 3.12+
- **Node.js**: Node.js 18+ (for dashboard)

---

### Building and Testing the C++ Engine

#### 1. Configure and Build (Ninja + CMake)
```bash
# Debug Build
cmake -B build_debug -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
cmake --build build_debug

# Release Build (Optimized)
cmake -B build_release -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build_release
```

#### 2. Run GoogleTest Suite (117 Tests)
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

#### 4. Benchmark
```bash
./build_release/engine/chess_engine bench      # fixed-depth search over 6 standard positions
```
The bench node count is deterministic, so it doubles as a regression signature for search changes.

---

### Python Platform Setup & Testing

#### 1. Install Dependencies
```bash
cd python
pip install -e .
```

#### 2. Run Python Unit Tests
```bash
python python/run_tests.py
```

---

## 🗺️ Implementation Roadmap

- [x] **Steps 1–6 — Board & State Foundation**: Multi-language workspace, C++23 build, square mapping, bitboards, position state, and FEN parsing.
- [x] **Steps 7–10 — Attack Tables & Move Encoding**: King/Knight/Pawn/Sliding attack generation and compact 32-bit Move encoding.
- [x] **Steps 11–16 — Movegen, State Transitions & Perft Hard Gate**: Legal move filtering, castling/EP state, make/unmake, Zobrist hashing, and verified Perft test suite.
- [x] **Steps 17–20 — Evaluation & Search Core**: Classical material/PST evaluation, Negamax, Alpha-Beta pruning, Quiescence, and Iterative Deepening.
- [x] **Steps 21–25 — Search Optimizations & UCI Protocol**: Transposition Table, move ordering heuristics, UCI protocol handler, and engine benchmarks.
- [ ] **Steps 26–34 — Python Data Platform, ML Baselines & PyTorch NNUE**: PGN pipeline, feature engineering, analytics, ML baselines, and NNUE training.
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
