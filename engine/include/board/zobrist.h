#pragma once

#include "board/types.h"
#include "board/bitboard.h"
#include <array>
#include <cstdint>

namespace chess {

class Position;

namespace zobrist {

namespace detail {

constexpr uint64_t splitmix64_next(uint64_t& state) noexcept {
    uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

struct ZobristTables {
    std::array<std::array<uint64_t, NUM_SQUARES>, NUM_PIECES> piece_keys{};
    uint64_t side_key{0};
    std::array<uint64_t, 16> castling_keys{};
    std::array<uint64_t, NUM_FILES> en_passant_keys{};
};

consteval ZobristTables init_zobrist() noexcept {
    ZobristTables tables{};
    uint64_t state = 0x123456789ABCDEF0ULL;

    for (size_t p = 0; p < NUM_PIECES; ++p) {
        for (size_t s = 0; s < NUM_SQUARES; ++s) {
            tables.piece_keys[p][s] = splitmix64_next(state);
        }
    }

    tables.side_key = splitmix64_next(state);

    for (size_t i = 0; i < 16; ++i) {
        tables.castling_keys[i] = splitmix64_next(state);
    }

    for (size_t i = 0; i < NUM_FILES; ++i) {
        tables.en_passant_keys[i] = splitmix64_next(state);
    }

    return tables;
}

} // namespace detail

inline constexpr auto ZOBRIST = detail::init_zobrist();

constexpr uint64_t piece_key(Piece p, Square sq) noexcept {
    if (p == Piece::None || !is_valid_square(sq)) return 0ULL;
    return ZOBRIST.piece_keys[static_cast<size_t>(p)][static_cast<size_t>(sq)];
}

constexpr uint64_t side_key() noexcept {
    return ZOBRIST.side_key;
}

constexpr uint64_t castling_key(uint8_t cr) noexcept {
    return ZOBRIST.castling_keys[cr & 0xF];
}

constexpr uint64_t en_passant_key(File f) noexcept {
    if (!is_valid_file(f)) return 0ULL;
    return ZOBRIST.en_passant_keys[static_cast<size_t>(f)];
}

constexpr uint64_t en_passant_key(Square sq) noexcept {
    if (!is_valid_square(sq)) return 0ULL;
    return en_passant_key(square_file(sq));
}

uint64_t compute_hash(const Position& pos) noexcept;

} // namespace zobrist
} // namespace chess
