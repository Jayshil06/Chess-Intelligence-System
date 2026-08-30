#include <gtest/gtest.h>
#include "move/attacks.h"

using namespace chess;
using namespace chess::attacks;

TEST(AttacksTest, KnightAttacksExhaustiveAll64Squares) {
    EXPECT_EQ(knight_attacks(Square::None), bb::EMPTY);

    for (int i = 0; i < 64; ++i) {
        Square from = static_cast<Square>(i);
        Bitboard attacks = knight_attacks(from);
        int count = bb::popcount(attacks);

        // Knight attack count must be 2, 3, 4, 6, or 8
        EXPECT_TRUE(count == 2 || count == 3 || count == 4 || count == 6 || count == 8);

        // Cannot attack own square
        EXPECT_FALSE(bb::test_bit(attacks, from));

        // Test every attacked square
        Bitboard remaining = attacks;
        while (remaining) {
            Square to = bb::pop_lsb(remaining);
            int df = std::abs(static_cast<int>(square_file(from)) - static_cast<int>(square_file(to)));
            int dr = std::abs(static_cast<int>(square_rank(from)) - static_cast<int>(square_rank(to)));

            // (df, dr) must be (1, 2) or (2, 1) -> df * dr == 2 and df + dr == 3
            EXPECT_TRUE((df == 1 && dr == 2) || (df == 2 && dr == 1));

            // Symmetry: Knight on 'to' must attack 'from'
            EXPECT_TRUE(bb::test_bit(knight_attacks(to), from));
        }
    }

    // Specific corner counts (A1, H1, A8, H8)
    EXPECT_EQ(bb::popcount(knight_attacks(Square::A1)), 2);
    EXPECT_EQ(bb::popcount(knight_attacks(Square::H1)), 2);
    EXPECT_EQ(bb::popcount(knight_attacks(Square::A8)), 2);
    EXPECT_EQ(bb::popcount(knight_attacks(Square::H8)), 2);

    // Exact A1 targets: B3, C2
    Bitboard expected_a1 = bb::square_mask(Square::B3) | bb::square_mask(Square::C2);
    EXPECT_EQ(knight_attacks(Square::A1), expected_a1);

    // Center square (E4): 8 attacks
    EXPECT_EQ(bb::popcount(knight_attacks(Square::E4)), 8);
}

TEST(AttacksTest, KingAttacksExhaustiveAll64Squares) {
    EXPECT_EQ(king_attacks(Square::None), bb::EMPTY);

    for (int i = 0; i < 64; ++i) {
        Square from = static_cast<Square>(i);
        Bitboard attacks = king_attacks(from);
        int count = bb::popcount(attacks);

        // King attack count must be 3 (corners), 5 (edges), or 8 (interior)
        EXPECT_TRUE(count == 3 || count == 5 || count == 8);

        // Cannot attack own square
        EXPECT_FALSE(bb::test_bit(attacks, from));

        // Test every attacked square
        Bitboard remaining = attacks;
        while (remaining) {
            Square to = bb::pop_lsb(remaining);
            int dist = chebyshev_distance(from, to);

            // Chebyshev distance must be exactly 1
            EXPECT_EQ(dist, 1);

            // Symmetry: King on 'to' must attack 'from'
            EXPECT_TRUE(bb::test_bit(king_attacks(to), from));
        }
    }

    // Specific corner counts (A1, H1, A8, H8)
    EXPECT_EQ(bb::popcount(king_attacks(Square::A1)), 3);
    EXPECT_EQ(bb::popcount(king_attacks(Square::H1)), 3);
    EXPECT_EQ(bb::popcount(king_attacks(Square::A8)), 3);
    EXPECT_EQ(bb::popcount(king_attacks(Square::H8)), 3);

    // Exact A1 targets: A2, B1, B2
    Bitboard expected_a1 = bb::square_mask(Square::A2) | bb::square_mask(Square::B1) | bb::square_mask(Square::B2);
    EXPECT_EQ(king_attacks(Square::A1), expected_a1);

    // Exact E1 edge targets: D1, F1, D2, E2, F2
    Bitboard expected_e1 = bb::square_mask(Square::D1) | bb::square_mask(Square::F1) |
                           bb::square_mask(Square::D2) | bb::square_mask(Square::E2) | bb::square_mask(Square::F2);
    EXPECT_EQ(king_attacks(Square::E1), expected_e1);

    // Center square (E4): 8 attacks
    EXPECT_EQ(bb::popcount(king_attacks(Square::E4)), 8);
}

TEST(AttacksTest, AggregatedAttacksFromBitboards) {
    Bitboard knights = bb::square_mask(Square::B1) | bb::square_mask(Square::G1);
    Bitboard expected = knight_attacks(Square::B1) | knight_attacks(Square::G1);
    EXPECT_EQ(knight_attacks_from_bb(knights), expected);
    EXPECT_EQ(knight_attacks_from_bb(bb::EMPTY), bb::EMPTY);

    Bitboard kings = bb::square_mask(Square::E1);
    EXPECT_EQ(king_attacks_from_bb(kings), king_attacks(Square::E1));
    EXPECT_EQ(king_attacks_from_bb(bb::EMPTY), bb::EMPTY);
}
