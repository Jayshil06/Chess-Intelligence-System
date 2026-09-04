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

consteval std::array<Bitboard, NUM_SQUARES> init_white_pawn_attacks() {
    std::array<Bitboard, NUM_SQUARES> table{};
    for (int r = 0; r < 8; ++r) {
        for (int f = 0; f < 8; ++f) {
            Square sq = make_square(static_cast<File>(f), static_cast<Rank>(r));
            Bitboard b = 0ULL;
            if (r < 7) { // Rank 8 (index 7) pawns cannot attack further (promotion rank)
                if (f > 0) { // North-West capture
                    Square target = make_square(static_cast<File>(f - 1), static_cast<Rank>(r + 1));
                    b |= (1ULL << static_cast<uint8_t>(target));
                }
                if (f < 7) { // North-East capture
                    Square target = make_square(static_cast<File>(f + 1), static_cast<Rank>(r + 1));
                    b |= (1ULL << static_cast<uint8_t>(target));
                }
            }
            table[static_cast<size_t>(sq)] = b;
        }
    }
    return table;
}

consteval std::array<Bitboard, NUM_SQUARES> init_black_pawn_attacks() {
    std::array<Bitboard, NUM_SQUARES> table{};
    for (int r = 0; r < 8; ++r) {
        for (int f = 0; f < 8; ++f) {
            Square sq = make_square(static_cast<File>(f), static_cast<Rank>(r));
            Bitboard b = 0ULL;
            if (r > 0) { // Rank 1 (index 0) pawns cannot attack further (promotion rank)
                if (f > 0) { // South-West capture
                    Square target = make_square(static_cast<File>(f - 1), static_cast<Rank>(r - 1));
                    b |= (1ULL << static_cast<uint8_t>(target));
                }
                if (f < 7) { // South-East capture
                    Square target = make_square(static_cast<File>(f + 1), static_cast<Rank>(r - 1));
                    b |= (1ULL << static_cast<uint8_t>(target));
                }
            }
            table[static_cast<size_t>(sq)] = b;
        }
    }
    return table;
}

consteval std::array<std::array<Bitboard, NUM_SQUARES>, 2> init_pawn_attacks() {
    return { init_white_pawn_attacks(), init_black_pawn_attacks() };
}

} // namespace detail

// Global precomputed tables
inline constexpr auto KNIGHT_ATTACKS = detail::init_knight_attacks();
inline constexpr auto KING_ATTACKS = detail::init_king_attacks();
inline constexpr auto WHITE_PAWN_ATTACKS = detail::init_white_pawn_attacks();
inline constexpr auto BLACK_PAWN_ATTACKS = detail::init_black_pawn_attacks();
inline constexpr auto PAWN_ATTACKS = detail::init_pawn_attacks();

// Fast inline accessor functions
constexpr Bitboard knight_attacks(Square sq) noexcept {
    if (!is_valid_square(sq)) return bb::EMPTY;
    return KNIGHT_ATTACKS[static_cast<size_t>(sq)];
}

constexpr Bitboard king_attacks(Square sq) noexcept {
    if (!is_valid_square(sq)) return bb::EMPTY;
    return KING_ATTACKS[static_cast<size_t>(sq)];
}

constexpr Bitboard white_pawn_attacks(Square sq) noexcept {
    if (!is_valid_square(sq)) return bb::EMPTY;
    return WHITE_PAWN_ATTACKS[static_cast<size_t>(sq)];
}

constexpr Bitboard black_pawn_attacks(Square sq) noexcept {
    if (!is_valid_square(sq)) return bb::EMPTY;
    return BLACK_PAWN_ATTACKS[static_cast<size_t>(sq)];
}

constexpr Bitboard pawn_attacks(Color color, Square sq) noexcept {
    if (!is_valid_square(sq)) return bb::EMPTY;
    return PAWN_ATTACKS[static_cast<size_t>(color)][static_cast<size_t>(sq)];
}

constexpr Bitboard pawn_attacks(Square sq, Color color) noexcept {
    return pawn_attacks(color, sq);
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

constexpr Bitboard white_pawn_attacks_from_bb(Bitboard white_pawns) noexcept {
    return bb::shift_north_west(white_pawns) | bb::shift_north_east(white_pawns);
}

constexpr Bitboard black_pawn_attacks_from_bb(Bitboard black_pawns) noexcept {
    return bb::shift_south_west(black_pawns) | bb::shift_south_east(black_pawns);
}

constexpr Bitboard pawn_attacks_from_bb(Color color, Bitboard pawns) noexcept {
    return color == Color::White ? white_pawn_attacks_from_bb(pawns)
                                 : black_pawn_attacks_from_bb(pawns);
}

constexpr Bitboard white_pawn_north_west_attacks(Bitboard pawns) noexcept {
    return bb::shift_north_west(pawns);
}

constexpr Bitboard white_pawn_north_east_attacks(Bitboard pawns) noexcept {
    return bb::shift_north_east(pawns);
}

constexpr Bitboard black_pawn_south_west_attacks(Bitboard pawns) noexcept {
    return bb::shift_south_west(pawns);
}

constexpr Bitboard black_pawn_south_east_attacks(Bitboard pawns) noexcept {
    return bb::shift_south_east(pawns);
}

// Pawn pushes
constexpr Bitboard white_pawn_single_pushes(Bitboard white_pawns, Bitboard empty_squares) noexcept {
    return bb::shift_north(white_pawns) & empty_squares;
}

constexpr Bitboard black_pawn_single_pushes(Bitboard black_pawns, Bitboard empty_squares) noexcept {
    return bb::shift_south(black_pawns) & empty_squares;
}

constexpr Bitboard pawn_single_pushes(Color color, Bitboard pawns, Bitboard empty_squares) noexcept {
    return color == Color::White ? white_pawn_single_pushes(pawns, empty_squares)
                                 : black_pawn_single_pushes(pawns, empty_squares);
}

constexpr Bitboard white_pawn_double_pushes(Bitboard white_pawns, Bitboard empty_squares) noexcept {
    Bitboard single_pushes = bb::shift_north(white_pawns & bb::RANK_2) & empty_squares;
    return bb::shift_north(single_pushes) & empty_squares;
}

constexpr Bitboard black_pawn_double_pushes(Bitboard black_pawns, Bitboard empty_squares) noexcept {
    Bitboard single_pushes = bb::shift_south(black_pawns & bb::RANK_7) & empty_squares;
    return bb::shift_south(single_pushes) & empty_squares;
}

constexpr Bitboard pawn_double_pushes(Color color, Bitboard pawns, Bitboard empty_squares) noexcept {
    return color == Color::White ? white_pawn_double_pushes(pawns, empty_squares)
                                 : black_pawn_double_pushes(pawns, empty_squares);
}

// En-passant logic and helpers
constexpr Bitboard pawn_ep_attackers(Color side_to_move, Square ep_sq, Bitboard friendly_pawns) noexcept {
    if (!is_valid_square(ep_sq)) return bb::EMPTY;
    return pawn_attacks(~side_to_move, ep_sq) & friendly_pawns;
}

constexpr Square pawn_ep_captured_square(Square ep_sq, Color side_to_move) noexcept {
    if (!is_valid_square(ep_sq)) return Square::None;
    return side_to_move == Color::White
        ? static_cast<Square>(static_cast<uint8_t>(ep_sq) - 8)
        : static_cast<Square>(static_cast<uint8_t>(ep_sq) + 8);
}

constexpr Square ep_target_square(Square double_push_from, Color side_moved) noexcept {
    if (!is_valid_square(double_push_from)) return Square::None;
    return side_moved == Color::White
        ? static_cast<Square>(static_cast<uint8_t>(double_push_from) + 8)
        : static_cast<Square>(static_cast<uint8_t>(double_push_from) - 8);
}

} // namespace attacks
} // namespace chess
