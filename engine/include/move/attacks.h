#pragma once

#include "board/types.h"
#include "board/bitboard.h"
#include <array>
#include <cstdint>

namespace chess {
namespace attacks {

// Compile-time precomputed attack tables
namespace detail {

consteval std::array<Bitboard, NUM_SQUARES> init_knight_attacks() {
    std::array<Bitboard, NUM_SQUARES> table{};
    for (int r = 0; r < 8; ++r) {
        for (int f = 0; f < 8; ++f) {
            Square sq = make_square(static_cast<File>(f), static_cast<Rank>(r));
            Bitboard b = 0ULL;
            const int dr[] = {2, 1, -1, -2, -2, -1, 1, 2};
            const int df[] = {1, 2, 2, 1, -1, -2, -2, -1};

            for (int i = 0; i < 8; ++i) {
                int nr = r + dr[i];
                int nf = f + df[i];
                if (nr >= 0 && nr < 8 && nf >= 0 && nf < 8) {
                    Square target = make_square(static_cast<File>(nf), static_cast<Rank>(nr));
                    b |= (1ULL << static_cast<uint8_t>(target));
                }
            }
            table[static_cast<size_t>(sq)] = b;
        }
    }
    return table;
}

consteval std::array<Bitboard, NUM_SQUARES> init_king_attacks() {
    std::array<Bitboard, NUM_SQUARES> table{};
    for (int r = 0; r < 8; ++r) {
        for (int f = 0; f < 8; ++f) {
            Square sq = make_square(static_cast<File>(f), static_cast<Rank>(r));
            Bitboard b = 0ULL;
            const int dr[] = {1, 1, 0, -1, -1, -1, 0, 1};
            const int df[] = {0, 1, 1, 1, 0, -1, -1, -1};

            for (int i = 0; i < 8; ++i) {
                int nr = r + dr[i];
                int nf = f + df[i];
                if (nr >= 0 && nr < 8 && nf >= 0 && nf < 8) {
                    Square target = make_square(static_cast<File>(nf), static_cast<Rank>(nr));
                    b |= (1ULL << static_cast<uint8_t>(target));
                }
            }
            table[static_cast<size_t>(sq)] = b;
        }
    }
    return table;
}

} // namespace detail

// Global precomputed tables
inline constexpr auto KNIGHT_ATTACKS = detail::init_knight_attacks();
inline constexpr auto KING_ATTACKS = detail::init_king_attacks();

// Fast inline accessor functions
constexpr Bitboard knight_attacks(Square sq) noexcept {
    if (!is_valid_square(sq)) return bb::EMPTY;
    return KNIGHT_ATTACKS[static_cast<size_t>(sq)];
}

constexpr Bitboard king_attacks(Square sq) noexcept {
    if (!is_valid_square(sq)) return bb::EMPTY;
    return KING_ATTACKS[static_cast<size_t>(sq)];
}

// Bitboard-wide aggregated attacks
inline Bitboard knight_attacks_from_bb(Bitboard knights) noexcept {
    Bitboard attacks = bb::EMPTY;
    while (knights) {
        Square sq = bb::pop_lsb(knights);
        attacks |= knight_attacks(sq);
    }
    return attacks;
}

inline Bitboard king_attacks_from_bb(Bitboard kings) noexcept {
    Bitboard attacks = bb::EMPTY;
    while (kings) {
        Square sq = bb::pop_lsb(kings);
        attacks |= king_attacks(sq);
    }
    return attacks;
}

} // namespace attacks
} // namespace chess
