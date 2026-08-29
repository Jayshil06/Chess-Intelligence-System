#include <gtest/gtest.h>
#include "board/bitboard.h"

using namespace chess;
using namespace chess::bb;

TEST(BitboardTest, ConstantsAndSquareMasks) {
    EXPECT_EQ(EMPTY, 0ULL);
    EXPECT_EQ(ALL_SQUARES, 0xFFFFFFFFFFFFFFFFULL);
    EXPECT_EQ(popcount(ALL_SQUARES), 64);
    EXPECT_EQ(popcount(EMPTY), 0);

    EXPECT_EQ(square_mask(Square::A1), 1ULL);
    EXPECT_EQ(square_mask(Square::B1), 2ULL);
    EXPECT_EQ(square_mask(Square::H8), 1ULL << 63);
    EXPECT_EQ(square_mask(Square::None), 0ULL);

    for (int i = 0; i < 64; ++i) {
        Square sq = static_cast<Square>(i);
        EXPECT_EQ(square_mask(sq), 1ULL << i);
        EXPECT_EQ(SQUARE_MASKS[i], 1ULL << i);
    }
}

TEST(BitboardTest, RankAndFileMasks) {
    for (int f = 0; f < 8; ++f) {
        File file = static_cast<File>(f);
        Bitboard mask = file_mask(file);
        EXPECT_EQ(popcount(mask), 8);
        EXPECT_EQ(FILE_MASKS[f], mask);
    }

    for (int r = 0; r < 8; ++r) {
        Rank rank = static_cast<Rank>(r);
        Bitboard mask = rank_mask(rank);
        EXPECT_EQ(popcount(mask), 8);
        EXPECT_EQ(RANK_MASKS[r], mask);
    }

    // Every file and rank must intersect at exactly one square
    for (int r = 0; r < 8; ++r) {
        for (int f = 0; f < 8; ++f) {
            Bitboard intersection = file_mask(static_cast<File>(f)) & rank_mask(static_cast<Rank>(r));
            Square expected_sq = make_square(static_cast<File>(f), static_cast<Rank>(r));
            EXPECT_EQ(intersection, square_mask(expected_sq));
            EXPECT_EQ(popcount(intersection), 1);
        }
    }
}

TEST(BitboardTest, BitOperationsSetClearToggleTest) {
    Bitboard b = EMPTY;

    for (int i = 0; i < 64; ++i) {
        Square sq = static_cast<Square>(i);
        EXPECT_FALSE(test_bit(b, sq));
        
        set_bit(b, sq);
        EXPECT_TRUE(test_bit(b, sq));
        EXPECT_EQ(popcount(b), i + 1);
    }

    EXPECT_EQ(b, ALL_SQUARES);

    for (int i = 0; i < 64; ++i) {
        Square sq = static_cast<Square>(i);
        clear_bit(b, sq);
        EXPECT_FALSE(test_bit(b, sq));
        EXPECT_EQ(popcount(b), 63 - i);
    }

    EXPECT_EQ(b, EMPTY);

    // Toggle test
    toggle_bit(b, Square::E4);
    EXPECT_TRUE(test_bit(b, Square::E4));
    toggle_bit(b, Square::E4);
    EXPECT_FALSE(test_bit(b, Square::E4));
    EXPECT_EQ(b, EMPTY);

    // Out of bounds safety
    set_bit(b, Square::None);
    clear_bit(b, Square::None);
    toggle_bit(b, Square::None);
    EXPECT_FALSE(test_bit(b, Square::None));
    EXPECT_EQ(b, EMPTY);
}

TEST(BitboardTest, SingleBitAndPopcount) {
    EXPECT_FALSE(has_single_bit(EMPTY));
    EXPECT_FALSE(has_single_bit(ALL_SQUARES));
    EXPECT_TRUE(has_single_bit(1ULL));
    EXPECT_TRUE(has_single_bit(1ULL << 45));
    EXPECT_FALSE(has_single_bit((1ULL << 45) | (1ULL << 2)));
}

TEST(BitboardTest, LsbMsbAndPopLsb) {
    EXPECT_EQ(lsb(EMPTY), Square::None);
    EXPECT_EQ(msb(EMPTY), Square::None);

    Bitboard b = 0ULL;
    set_bit(b, Square::C3);
    set_bit(b, Square::F6);
    set_bit(b, Square::A1);

    EXPECT_EQ(lsb(b), Square::A1);
    EXPECT_EQ(msb(b), Square::F6);

    // Pop LSB in order
    EXPECT_EQ(pop_lsb(b), Square::A1);
    EXPECT_EQ(pop_lsb(b), Square::C3);
    EXPECT_EQ(pop_lsb(b), Square::F6);
    EXPECT_EQ(pop_lsb(b), Square::None);
    EXPECT_EQ(b, EMPTY);
}

TEST(BitboardTest, DirectionalShiftsAndWrapAround) {
    // Single piece in center: E4
    Bitboard e4 = square_mask(Square::E4);

    EXPECT_EQ(shift_north(e4), square_mask(Square::E5));
    EXPECT_EQ(shift_south(e4), square_mask(Square::E3));
    EXPECT_EQ(shift_east(e4), square_mask(Square::F4));
    EXPECT_EQ(shift_west(e4), square_mask(Square::D4));
    EXPECT_EQ(shift_north_east(e4), square_mask(Square::F5));
    EXPECT_EQ(shift_north_west(e4), square_mask(Square::D5));
    EXPECT_EQ(shift_south_east(e4), square_mask(Square::F3));
    EXPECT_EQ(shift_south_west(e4), square_mask(Square::D3));

    // File H east shift must not wrap to File A
    Bitboard h4 = square_mask(Square::H4);
    EXPECT_EQ(shift_east(h4), EMPTY);
    EXPECT_EQ(shift_north_east(h4), EMPTY);
    EXPECT_EQ(shift_south_east(h4), EMPTY);

    // File A west shift must not wrap to File H
    Bitboard a4 = square_mask(Square::A4);
    EXPECT_EQ(shift_west(a4), EMPTY);
    EXPECT_EQ(shift_north_west(a4), EMPTY);
    EXPECT_EQ(shift_south_west(a4), EMPTY);

    // Rank 8 north shifts must disappear
    Bitboard e8 = square_mask(Square::E8);
    EXPECT_EQ(shift_north(e8), EMPTY);
    EXPECT_EQ(shift_north_east(e8), EMPTY);
    EXPECT_EQ(shift_north_west(e8), EMPTY);

    // Rank 1 south shifts must disappear
    Bitboard e1 = square_mask(Square::E1);
    EXPECT_EQ(shift_south(e1), EMPTY);
    EXPECT_EQ(shift_south_east(e1), EMPTY);
    EXPECT_EQ(shift_south_west(e1), EMPTY);
}

TEST(BitboardTest, ToStringFormatting) {
    Bitboard b = square_mask(Square::E4) | square_mask(Square::D5);
    std::string s = to_string(b);
    EXPECT_FALSE(s.empty());
    EXPECT_NE(s.find("1"), std::string::npos);
    EXPECT_NE(s.find("Hex:"), std::string::npos);
}
