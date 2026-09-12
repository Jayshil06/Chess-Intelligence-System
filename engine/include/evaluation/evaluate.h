#pragma once

#include "board/types.h"
#include "board/position.h"
#include <array>
#include <cstdint>

namespace chess {
namespace eval {

constexpr int SCORE_MATE = 30000;
constexpr int SCORE_DRAW = 0;

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

int evaluate(const Position& pos) noexcept;
int evaluate_material(const Position& pos, Color c) noexcept;
int evaluate_pst(const Position& pos, Color c) noexcept;

} // namespace eval
} // namespace chess
