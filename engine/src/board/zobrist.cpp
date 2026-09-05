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

    Square ep_sq = pos.en_passant_square();
    if (ep_sq != Square::None) {
        File f = square_file(ep_sq);
        if (is_valid_file(f)) {
            hash ^= ZOBRIST.en_passant_keys[static_cast<size_t>(f)];
        }
    }

    return hash;
}

} // namespace zobrist
} // namespace chess
