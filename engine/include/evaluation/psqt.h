#pragma once

#include "board/types.h"
#include <array>

namespace chess {
namespace eval {

constexpr int PAWN_VALUE   = 100;
constexpr int KNIGHT_VALUE = 320;
constexpr int BISHOP_VALUE = 330;
constexpr int ROOK_VALUE   = 500;
constexpr int QUEEN_VALUE  = 900;
constexpr int KING_VALUE   = 20000;

constexpr int piece_value(PieceType pt) noexcept {
    switch (pt) {
        case PieceType::Pawn:   return PAWN_VALUE;
        case PieceType::Knight: return KNIGHT_VALUE;
        case PieceType::Bishop: return BISHOP_VALUE;
        case PieceType::Rook:   return ROOK_VALUE;
        case PieceType::Queen:  return QUEEN_VALUE;
        case PieceType::King:   return KING_VALUE;
        default: return 0;
    }
}

// Game phase from non-pawn material: 24 = opening, 0 = bare endgame
constexpr int MAX_PHASE = 24;
constexpr std::array<int, NUM_PIECE_TYPES> PHASE_WEIGHT = {0, 1, 1, 2, 4, 0};

using PieceTable = std::array<int, NUM_SQUARES>;

// Piece-square tables, White's perspective, A1 = index 0
constexpr PieceTable PAWN_MG = {
      0,   0,   0,   0,   0,   0,   0,   0,
      5,  10,  10, -20, -20,  10,  10,   5,
      5,  -5, -10,   0,   0, -10,  -5,   5,
      0,   0,   0,  20,  20,   0,   0,   0,
      5,   5,  10,  25,  25,  10,   5,   5,
     10,  10,  20,  30,  30,  20,  10,  10,
     50,  50,  50,  50,  50,  50,  50,  50,
      0,   0,   0,   0,   0,   0,   0,   0
};

constexpr PieceTable PAWN_EG = {
      0,   0,   0,   0,   0,   0,   0,   0,
      5,   5,   5,   5,   5,   5,   5,   5,
     10,  10,  10,  10,  10,  10,  10,  10,
     20,  20,  20,  20,  20,  20,  20,  20,
     35,  35,  35,  35,  35,  35,  35,  35,
     60,  60,  60,  60,  60,  60,  60,  60,
     90,  90,  90,  90,  90,  90,  90,  90,
      0,   0,   0,   0,   0,   0,   0,   0
};

constexpr PieceTable KNIGHT_PST = {
    -50, -40, -30, -30, -30, -30, -40, -50,
    -40, -20,   0,   5,   5,   0, -20, -40,
    -30,   5,  10,  15,  15,  10,   5, -30,
    -30,   0,  15,  20,  20,  15,   0, -30,
    -30,   5,  15,  20,  20,  15,   5, -30,
    -30,   0,  10,  15,  15,  10,   0, -30,
    -40, -20,   0,   0,   0,   0, -20, -40,
    -50, -40, -30, -30, -30, -30, -40, -50
};

constexpr PieceTable BISHOP_PST = {
    -20, -10, -10, -10, -10, -10, -10, -20,
    -10,   5,   0,   0,   0,   0,   5, -10,
    -10,  10,  10,  10,  10,  10,  10, -10,
    -10,   0,  10,  10,  10,  10,   0, -10,
    -10,   5,   5,  10,  10,   5,   5, -10,
    -10,   0,   5,  10,  10,   5,   0, -10,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -20, -10, -10, -10, -10, -10, -10, -20
};

constexpr PieceTable ROOK_PST = {
      0,   0,   0,   5,   5,   0,   0,   0,
     -5,   0,   0,   0,   0,   0,   0,  -5,
     -5,   0,   0,   0,   0,   0,   0,  -5,
     -5,   0,   0,   0,   0,   0,   0,  -5,
     -5,   0,   0,   0,   0,   0,   0,  -5,
     -5,   0,   0,   0,   0,   0,   0,  -5,
      5,  10,  10,  10,  10,  10,  10,   5,
      0,   0,   0,   0,   0,   0,   0,   0
};

constexpr PieceTable QUEEN_PST = {
    -20, -10, -10,  -5,  -5, -10, -10, -20,
    -10,   0,   5,   0,   0,   0,   0, -10,
    -10,   5,   5,   5,   5,   5,   0, -10,
      0,   0,   5,   5,   5,   5,   0,  -5,
     -5,   0,   5,   5,   5,   5,   0,  -5,
    -10,   0,   5,   5,   5,   5,   0, -10,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -20, -10, -10,  -5,  -5, -10, -10, -20
};

constexpr PieceTable KING_MG = {
     20,  30,  10,   0,   0,  10,  30,  20,
     20,  20,   0,   0,   0,   0,  20,  20,
    -10, -20, -20, -20, -20, -20, -20, -10,
    -20, -30, -30, -40, -40, -30, -30, -20,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30
};

constexpr PieceTable KING_EG = {
    -50, -30, -30, -30, -30, -30, -30, -50,
    -30, -30,   0,   0,   0,   0, -30, -30,
    -30, -10,  20,  30,  30,  20, -10, -30,
    -30, -10,  30,  40,  40,  30, -10, -30,
    -30, -10,  30,  40,  40,  30, -10, -30,
    -30, -10,  20,  30,  30,  20, -10, -30,
    -30, -20, -10,   0,   0, -10, -20, -30,
    -50, -40, -30, -20, -20, -30, -40, -50
};

// Minor and major pieces use the same table in both phases
constexpr std::array<const PieceTable*, NUM_PIECE_TYPES> PST_MG = {
    &PAWN_MG, &KNIGHT_PST, &BISHOP_PST, &ROOK_PST, &QUEEN_PST, &KING_MG};
constexpr std::array<const PieceTable*, NUM_PIECE_TYPES> PST_EG = {
    &PAWN_EG, &KNIGHT_PST, &BISHOP_PST, &ROOK_PST, &QUEEN_PST, &KING_EG};

struct Score {
    int mg{0};
    int eg{0};
};

// Material + PST per piece and square from White's view; Black entries are mirrored and negated
inline constexpr auto PSQT = [] {
    std::array<std::array<Score, NUM_SQUARES>, NUM_PIECES> table{};
    for (size_t pt = 0; pt < NUM_PIECE_TYPES; ++pt) {
        int material = pt == static_cast<size_t>(PieceType::King) ? 0 : piece_value(static_cast<PieceType>(pt));
        for (size_t sq = 0; sq < NUM_SQUARES; ++sq) {
            Score s{material + (*PST_MG[pt])[sq], material + (*PST_EG[pt])[sq]};
            table[pt][sq] = s;
            table[pt + NUM_PIECE_TYPES][sq ^ 56] = {-s.mg, -s.eg};
        }
    }
    return table;
}();

constexpr int phase_weight(Piece p) noexcept {
    return p == Piece::None ? 0 : PHASE_WEIGHT[static_cast<size_t>(type_of(p))];
}

} // namespace eval
} // namespace chess
