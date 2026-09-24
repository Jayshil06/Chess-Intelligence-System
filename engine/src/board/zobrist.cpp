#include "board/zobrist.h"
#include "board/position.h"

namespace chess {
namespace zobrist {

uint64_t compute_hash(const Position& pos) noexcept {
    uint64_t hash = 0ULL;

    for (size_t p = 0; p < NUM_PIECES; ++p) {
        Bitboard bb = pos.piece_bb(static_cast<Piece>(p));
        while (bb) {
            Square sq = bb::pop_lsb(bb);
            hash ^= ZOBRIST.piece_keys[p][static_cast<size_t>(sq)];
        }
    }

    if (pos.side_to_move() == Color::Black) {
        hash ^= ZOBRIST.side_key;
    }

    hash ^= ZOBRIST.castling_keys[pos.castling_rights() & 0xF];

    hash ^= en_passant_key(pos.en_passant_square(), pos.side_to_move(),
                           pos.piece_bb(pos.side_to_move(), PieceType::Pawn));

    return hash;
}

} // namespace zobrist
} // namespace chess
