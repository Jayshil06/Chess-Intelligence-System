#include "evaluation/evaluate.h"

namespace chess {
namespace eval {

namespace {

// Piece-Square Tables (White perspective, A1=0 ... H8=63)
constexpr std::array<int, 64> PAWN_PST = {
      0,   0,   0,   0,   0,   0,   0,   0,
      5,  10,  10, -20, -20,  10,  10,   5,
      5,  -5, -10,   0,   0, -10,  -5,   5,
      0,   0,   0,  20,  20,   0,   0,   0,
      5,   5,  10,  25,  25,  10,   5,   5,
     10,  10,  20,  30,  30,  20,  10,  10,
     50,  50,  50,  50,  50,  50,  50,  50,
      0,   0,   0,   0,   0,   0,   0,   0
};

constexpr std::array<int, 64> KNIGHT_PST = {
    -50, -40, -30, -30, -30, -30, -40, -50,
    -40, -20,   0,   5,   5,   0, -20, -40,
    -30,   5,  10,  15,  15,  10,   5, -30,
    -30,   0,  15,  20,  20,  15,   0, -30,
    -30,   5,  15,  20,  20,  15,   5, -30,
    -30,   0,  10,  15,  15,  10,   0, -30,
    -40, -20,   0,   0,   0,   0, -20, -40,
    -50, -40, -30, -30, -30, -30, -40, -50
};

constexpr std::array<int, 64> BISHOP_PST = {
    -20, -10, -10, -10, -10, -10, -10, -20,
    -10,   5,   0,   0,   0,   0,   5, -10,
    -10,  10,  10,  10,  10,  10,  10, -10,
    -10,   0,  10,  10,  10,  10,   0, -10,
    -10,   5,   5,  10,  10,   5,   5, -10,
    -10,   0,   5,  10,  10,   5,   0, -10,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -20, -10, -10, -10, -10, -10, -10, -20
};

constexpr std::array<int, 64> ROOK_PST = {
      0,   0,   0,   5,   5,   0,   0,   0,
     -5,   0,   0,   0,   0,   0,   0,  -5,
     -5,   0,   0,   0,   0,   0,   0,  -5,
     -5,   0,   0,   0,   0,   0,   0,  -5,
     -5,   0,   0,   0,   0,   0,   0,  -5,
     -5,   0,   0,   0,   0,   0,   0,  -5,
      5,  10,  10,  10,  10,  10,  10,   5,
      0,   0,   0,   0,   0,   0,   0,   0
};

constexpr std::array<int, 64> QUEEN_PST = {
    -20, -10, -10,  -5,  -5, -10, -10, -20,
    -10,   0,   5,   0,   0,   0,   0, -10,
    -10,   5,   5,   5,   5,   5,   0, -10,
      0,   0,   5,   5,   5,   5,   0,  -5,
     -5,   0,   5,   5,   5,   5,   0,  -5,
    -10,   0,   5,   5,   5,   5,   0, -10,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -20, -10, -10,  -5,  -5, -10, -10, -20
};

constexpr std::array<int, 64> KING_PST = {
     20,  30,  10,   0,   0,  10,  30,  20,
     20,  20,   0,   0,   0,   0,  20,  20,
    -10, -20, -20, -20, -20, -20, -20, -10,
    -20, -30, -30, -40, -40, -30, -30, -20,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30
};

inline int pst_value(PieceType pt, Square sq, Color c) noexcept {
    Square rel_sq = relative_square(c, sq);
    size_t idx = static_cast<size_t>(rel_sq);
    switch (pt) {
        case PieceType::Pawn:   return PAWN_PST[idx];
        case PieceType::Knight: return KNIGHT_PST[idx];
        case PieceType::Bishop: return BISHOP_PST[idx];
        case PieceType::Rook:   return ROOK_PST[idx];
        case PieceType::Queen:  return QUEEN_PST[idx];
        case PieceType::King:   return KING_PST[idx];
        default: return 0;
    }
}

} // namespace

int evaluate_material(const Position& pos, Color c) noexcept {
    int score = 0;
    score += bb::popcount(pos.piece_bb(c, PieceType::Pawn)) * PAWN_VALUE;
    score += bb::popcount(pos.piece_bb(c, PieceType::Knight)) * KNIGHT_VALUE;
    score += bb::popcount(pos.piece_bb(c, PieceType::Bishop)) * BISHOP_VALUE;
    score += bb::popcount(pos.piece_bb(c, PieceType::Rook)) * ROOK_VALUE;
    score += bb::popcount(pos.piece_bb(c, PieceType::Queen)) * QUEEN_VALUE;
    return score;
}

int evaluate_pst(const Position& pos, Color c) noexcept {
    int score = 0;
    for (size_t pt = 0; pt < 6; ++pt) {
        PieceType type = static_cast<PieceType>(pt);
        Bitboard b = pos.piece_bb(c, type);
        while (b) {
            Square sq = bb::pop_lsb(b);
            score += pst_value(type, sq, c);
        }
    }
    return score;
}

int evaluate(const Position& pos) noexcept {
    int white_score = evaluate_material(pos, Color::White) + evaluate_pst(pos, Color::White);
    int black_score = evaluate_material(pos, Color::Black) + evaluate_pst(pos, Color::Black);

    int eval = white_score - black_score;
    return (pos.side_to_move() == Color::White) ? eval : -eval;
}

} // namespace eval
} // namespace chess
