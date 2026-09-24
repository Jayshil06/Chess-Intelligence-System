#include "evaluation/evaluate.h"
#include <algorithm>

namespace chess {
namespace eval {

namespace {

constexpr int taper(int mg, int eg, int phase) noexcept {
    phase = std::min(phase, MAX_PHASE);
    return (mg * phase + eg * (MAX_PHASE - phase)) / MAX_PHASE;
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
    int mg = 0, eg = 0;
    for (size_t pt = 0; pt < NUM_PIECE_TYPES; ++pt) {
        Bitboard b = pos.piece_bb(c, static_cast<PieceType>(pt));
        while (b) {
            size_t idx = static_cast<size_t>(relative_square(c, bb::pop_lsb(b)));
            mg += (*PST_MG[pt])[idx];
            eg += (*PST_EG[pt])[idx];
        }
    }
    return taper(mg, eg, pos.phase());
}

int evaluate(const Position& pos) noexcept {
    int eval = taper(pos.psq_mg(), pos.psq_eg(), pos.phase());
    return (pos.side_to_move() == Color::White) ? eval : -eval;
}

} // namespace eval
} // namespace chess
