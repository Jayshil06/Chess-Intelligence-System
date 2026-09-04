# ♟️ Chess Intelligence System

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg?style=flat-square&logo=c%2B%2B)](https://en.cppreference.com/w/cpp/23)
[![CMake](https://img.shields.io/badge/CMake-3.25%2B-064F8C.svg?style=flat-square&logo=cmake)](https://cmake.org/)
[![Python](https://img.shields.io/badge/Python-3.12%2B-3776AB.svg?style=flat-square&logo=python&logoColor=white)](https://www.python.org/)
[![Tests](https://img.shields.io/badge/Tests-28%2F28%20Passing-brightgreen.svg?style=flat-square)](./engine/tests)
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
  - Negamax with Alpha-Beta pruning, iterative deepening, quiescence search, move ordering, and transposition tables.
  - Standard Universal Chess Interface (UCI) protocol compliance.
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
│   │   ├── search/          # Alpha-Beta, Iterative Deepening, TT
│   │   ├── evaluation/      # Classical evaluator
│   │   ├── protocol/        # UCI protocol handler
│   │   └── nnue/            # C++ NNUE inference
│   ├── src/                 # C++ Implementations
│   │   ├── board/           # types.cpp, bitboard.cpp, position.cpp, fen.cpp
│   │   ├── move/            # attacks.cpp
│   │   ├── search/
│   │   ├── evaluation/
│   │   ├── protocol/
│   │   ├── nnue/
│   │   └── main.cpp         # Engine CLI executable entry point
│   └── tests/               # C++ GoogleTest suite
│       ├── unit/            # test_types, test_bitboard, test_position, test_fen, test_attacks
│       ├── perft/           # Perft validation suite
│       └── search/          # Search benchmarks and tactical tests
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

#### 2. Run GoogleTest Suite (48 Tests)
```bash
ctest --test-dir build_release --output-on-failure
```

#### 3. Run the Engine Binary
```bash
./build_release/engine/chess_engine
```

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

- [x] **Step 1 — Repository Foundation**: Multi-language workspace layout, manifests, and tooling.
- [x] **Step 2 — C++ Build/Test Infrastructure**: C++23, CMake, Ninja, and GoogleTest automation.
- [x] **Step 3 — Chess Types & Square Mapping**: 0–63 LERF mapping, Color/PieceType/Piece/Square models, conversion tests.
- [x] **Step 4 — Bitboard Foundation**: 64-bit word operations, rank/file masks, popcount, LSB extraction.
- [x] **Step 5 — Position Representation**: 12 piece bitboards, occupancy, castling, en-passant, invariant engine.
- [x] **Step 6 — FEN Parsing and Serialization**: Robust 6-field FEN parser and exact serializer.
- [x] **Step 7 — Attack Tables: Knight and King**: Compile-time precomputed 64-square non-sliding attack tables.
- [x] **Step 8 — Pawn Attack Generation**: White & Black pawn attack masks, pushes, and en-passant targets.
- [x] **Step 9 — Sliding Attack Generation**: Bishop, Rook, Queen ray attacks & blocker handling.
- [x] **Step 10 — Move Representation**: Compact 32-bit packed Move encoding, MoveList, UCI conversions, and ordering.
- [ ] **Step 11–16 — Move Generation, Make/Unmake & Perft Hard Gate**.
- [ ] **Step 17–25 — Search Algorithms (Alpha-Beta, TT, Move Ordering, UCI, Benchmarks)**.
- [ ] **Step 26–34 — Python Data Pipeline, Feature Engineering, ML Baselines & PyTorch NNUE**.
- [ ] **Step 35–38 — FastAPI Service, React Dashboard & Portfolio Release**.

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
