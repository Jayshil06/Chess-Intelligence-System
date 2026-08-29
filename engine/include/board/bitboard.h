#pragma once

#include "board/types.h"
#include <cstdint>
#include <bit>
#include <string>
#include <array>
#include <optional>

namespace chess {

// 64-bit integer representing chess board occupancy
using Bitboard = uint64_t;

namespace bb {

// Empty and Universal bitboards
constexpr Bitboard EMPTY = 0ULL;
constexpr Bitboard ALL_SQUARES = ~0ULL;

// File bitboards (A-H)
constexpr Bitboard FILE_A = 0x0101010101010101ULL;
constexpr Bitboard FILE_B = FILE_A << 1;
constexpr Bitboard FILE_C = FILE_A << 2;
constexpr Bitboard FILE_D = FILE_A << 3;
constexpr Bitboard FILE_E = FILE_A << 4;
constexpr Bitboard FILE_F = FILE_A << 5;
constexpr Bitboard FILE_G = FILE_A << 6;
constexpr Bitboard FILE_H = FILE_A << 7;

constexpr Bitboard NOT_FILE_A = ~FILE_A;
constexpr Bitboard NOT_FILE_H = ~FILE_H;
constexpr Bitboard NOT_FILE_AB = ~(FILE_A | FILE_B);
constexpr Bitboard NOT_FILE_GH = ~(FILE_G | FILE_H);

// Rank bitboards (1-8)
constexpr Bitboard RANK_1 = 0x00000000000000FFULL;
constexpr Bitboard RANK_2 = RANK_1 << (8 * 1);
constexpr Bitboard RANK_3 = RANK_1 << (8 * 2);
constexpr Bitboard RANK_4 = RANK_1 << (8 * 3);
constexpr Bitboard RANK_5 = RANK_1 << (8 * 4);
constexpr Bitboard RANK_6 = RANK_1 << (8 * 5);
constexpr Bitboard RANK_7 = RANK_1 << (8 * 6);
constexpr Bitboard RANK_8 = RANK_1 << (8 * 7);

constexpr Bitboard NOT_RANK_1 = ~RANK_1;
constexpr Bitboard NOT_RANK_8 = ~RANK_8;

// Board color masks
constexpr Bitboard LIGHT_SQUARES = 0x55AA55AA55AA55AAULL;
constexpr Bitboard DARK_SQUARES  = 0xAA55AA55AA55AA55ULL;

// Square Bitboard generator (1ULL << sq)
constexpr Bitboard square_mask(Square sq) {
    if (!is_valid_square(sq)) return 0ULL;
    return 1ULL << static_cast<uint8_t>(sq);
}

constexpr Bitboard file_mask(File f) {
    if (!is_valid_file(f)) return 0ULL;
    return FILE_A << static_cast<uint8_t>(f);
}

constexpr Bitboard rank_mask(Rank r) {
    if (!is_valid_rank(r)) return 0ULL;
    return RANK_1 << (8 * static_cast<uint8_t>(r));
}

// Precomputed arrays for rapid lookup
inline constexpr auto SQUARE_MASKS = []() consteval {
    std::array<Bitboard, NUM_SQUARES> masks{};
    for (size_t i = 0; i < NUM_SQUARES; ++i) {
        masks[i] = 1ULL << i;
    }
    return masks;
}();

inline constexpr auto FILE_MASKS = []() consteval {
    std::array<Bitboard, NUM_FILES> masks{};
    for (size_t i = 0; i < NUM_FILES; ++i) {
        masks[i] = FILE_A << i;
    }
    return masks;
}();

inline constexpr auto RANK_MASKS = []() consteval {
    std::array<Bitboard, NUM_RANKS> masks{};
    for (size_t i = 0; i < NUM_RANKS; ++i) {
        masks[i] = RANK_1 << (8 * i);
    }
    return masks;
}();

// Basic Bit Manipulation Operations

constexpr void set_bit(Bitboard& b, Square sq) {
    if (is_valid_square(sq)) {
        b |= (1ULL << static_cast<uint8_t>(sq));
    }
}

constexpr void clear_bit(Bitboard& b, Square sq) {
    if (is_valid_square(sq)) {
        b &= ~(1ULL << static_cast<uint8_t>(sq));
    }
}

constexpr void toggle_bit(Bitboard& b, Square sq) {
    if (is_valid_square(sq)) {
        b ^= (1ULL << static_cast<uint8_t>(sq));
    }
}

constexpr bool test_bit(Bitboard b, Square sq) {
    if (!is_valid_square(sq)) return false;
    return (b & (1ULL << static_cast<uint8_t>(sq))) != 0;
}

// Population Count (number of set bits) using C++23 std::popcount
constexpr int popcount(Bitboard b) noexcept {
    return std::popcount(b);
}

// Check if exactly one bit is set
constexpr bool has_single_bit(Bitboard b) noexcept {
    return std::has_single_bit(b);
}

// Least Significant Bit (LSB) extraction
constexpr Square lsb(Bitboard b) noexcept {
    if (b == 0ULL) return Square::None;
    return static_cast<Square>(std::countr_zero(b));
}

// Most Significant Bit (MSB) extraction
constexpr Square msb(Bitboard b) noexcept {
    if (b == 0ULL) return Square::None;
    return static_cast<Square>(63 - std::countl_zero(b));
}

// Pop and return the least significant bit
inline Square pop_lsb(Bitboard& b) noexcept {
    if (b == 0ULL) return Square::None;
    Square sq = static_cast<Square>(std::countr_zero(b));
    b &= b - 1ULL; // Clear lowest set bit
    return sq;
}

// Directional Shifts (safe bitboard shifts preventing file wrap-around)

constexpr Bitboard shift_north(Bitboard b) noexcept {
    return (b << 8);
}

constexpr Bitboard shift_south(Bitboard b) noexcept {
    return (b >> 8);
}

constexpr Bitboard shift_east(Bitboard b) noexcept {
    return (b & NOT_FILE_H) << 1;
}

constexpr Bitboard shift_west(Bitboard b) noexcept {
    return (b & NOT_FILE_A) >> 1;
}

constexpr Bitboard shift_north_east(Bitboard b) noexcept {
    return (b & NOT_FILE_H) << 9;
}

constexpr Bitboard shift_north_west(Bitboard b) noexcept {
    return (b & NOT_FILE_A) << 7;
}

constexpr Bitboard shift_south_east(Bitboard b) noexcept {
    return (b & NOT_FILE_H) >> 7;
}

constexpr Bitboard shift_south_west(Bitboard b) noexcept {
    return (b & NOT_FILE_A) >> 9;
}

// String / Visual Representation
std::string to_string(Bitboard b);

} // namespace bb
} // namespace chess
