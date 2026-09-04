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

TEST(AttacksTest, WhitePawnAttacksExhaustiveAll64Squares) {
    EXPECT_EQ(white_pawn_attacks(Square::None), bb::EMPTY);

    for (int i = 0; i < 64; ++i) {
        Square from = static_cast<Square>(i);
        File f = square_file(from);
        Rank r = square_rank(from);
        Bitboard attacks = white_pawn_attacks(from);
        int count = bb::popcount(attacks);

        // Pawns on rank 8 cannot attack (promotion rank / off board)
        if (r == Rank::Rank8) {
            EXPECT_EQ(count, 0);
            EXPECT_EQ(attacks, bb::EMPTY);
            continue;
        }

        // Flank pawns (file A and H) have 1 attack target, interior files have 2
        if (f == File::FileA || f == File::FileH) {
            EXPECT_EQ(count, 1);
        } else {
            EXPECT_EQ(count, 2);
        }

        // Cannot attack own square
        EXPECT_FALSE(bb::test_bit(attacks, from));

        // Test every attacked square
        Bitboard remaining = attacks;
        while (remaining) {
            Square to = bb::pop_lsb(remaining);
            File to_f = square_file(to);
            Rank to_r = square_rank(to);

            // White pawn attacks advance rank by exactly +1
            EXPECT_EQ(static_cast<int>(to_r), static_cast<int>(r) + 1);

            // File changes by exactly 1
            int df = std::abs(static_cast<int>(to_f) - static_cast<int>(f));
            EXPECT_EQ(df, 1);
        }
    }

    // Specific square verifications
    EXPECT_EQ(white_pawn_attacks(Square::A2), bb::square_mask(Square::B3));
    EXPECT_EQ(white_pawn_attacks(Square::H2), bb::square_mask(Square::G3));
    EXPECT_EQ(white_pawn_attacks(Square::E4), bb::square_mask(Square::D5) | bb::square_mask(Square::F5));
    EXPECT_EQ(white_pawn_attacks(Square::E7), bb::square_mask(Square::D8) | bb::square_mask(Square::F8));
    EXPECT_EQ(white_pawn_attacks(Square::E8), bb::EMPTY);
}

TEST(AttacksTest, BlackPawnAttacksExhaustiveAll64Squares) {
    EXPECT_EQ(black_pawn_attacks(Square::None), bb::EMPTY);

    for (int i = 0; i < 64; ++i) {
        Square from = static_cast<Square>(i);
        File f = square_file(from);
        Rank r = square_rank(from);
        Bitboard attacks = black_pawn_attacks(from);
        int count = bb::popcount(attacks);

        // Pawns on rank 1 cannot attack (promotion rank / off board)
        if (r == Rank::Rank1) {
            EXPECT_EQ(count, 0);
            EXPECT_EQ(attacks, bb::EMPTY);
            continue;
        }

        // Flank pawns (file A and H) have 1 attack target, interior files have 2
        if (f == File::FileA || f == File::FileH) {
            EXPECT_EQ(count, 1);
        } else {
            EXPECT_EQ(count, 2);
        }

        // Cannot attack own square
        EXPECT_FALSE(bb::test_bit(attacks, from));

        // Test every attacked square
        Bitboard remaining = attacks;
        while (remaining) {
            Square to = bb::pop_lsb(remaining);
            File to_f = square_file(to);
            Rank to_r = square_rank(to);

            // Black pawn attacks decrease rank by exactly -1
            EXPECT_EQ(static_cast<int>(to_r), static_cast<int>(r) - 1);

            // File changes by exactly 1
            int df = std::abs(static_cast<int>(to_f) - static_cast<int>(f));
            EXPECT_EQ(df, 1);
        }
    }

    // Specific square verifications
    EXPECT_EQ(black_pawn_attacks(Square::A7), bb::square_mask(Square::B6));
    EXPECT_EQ(black_pawn_attacks(Square::H7), bb::square_mask(Square::G6));
    EXPECT_EQ(black_pawn_attacks(Square::E5), bb::square_mask(Square::D4) | bb::square_mask(Square::F4));
    EXPECT_EQ(black_pawn_attacks(Square::E2), bb::square_mask(Square::D1) | bb::square_mask(Square::F1));
    EXPECT_EQ(black_pawn_attacks(Square::E1), bb::EMPTY);
}

TEST(AttacksTest, PawnAttackColorAccessorsAndTables) {
    for (int i = 0; i < 64; ++i) {
        Square sq = static_cast<Square>(i);
        EXPECT_EQ(pawn_attacks(Color::White, sq), white_pawn_attacks(sq));
        EXPECT_EQ(pawn_attacks(Color::Black, sq), black_pawn_attacks(sq));
        EXPECT_EQ(pawn_attacks(sq, Color::White), white_pawn_attacks(sq));
        EXPECT_EQ(pawn_attacks(sq, Color::Black), black_pawn_attacks(sq));
        EXPECT_EQ(PAWN_ATTACKS[0][i], WHITE_PAWN_ATTACKS[i]);
        EXPECT_EQ(PAWN_ATTACKS[1][i], BLACK_PAWN_ATTACKS[i]);
    }
}

TEST(AttacksTest, BitboardWidePawnAttacks) {
    // Full rank of white pawns on Rank 2 -> attacks on Rank 3 across files A-H
    Bitboard white_pawns = bb::RANK_2;
    Bitboard white_attacks = white_pawn_attacks_from_bb(white_pawns);
    EXPECT_EQ(white_attacks, bb::RANK_3);

    // Full rank of black pawns on Rank 7 -> attacks on Rank 6 across files A-H
    Bitboard black_pawns = bb::RANK_7;
    Bitboard black_attacks = black_pawn_attacks_from_bb(black_pawns);
    EXPECT_EQ(black_attacks, bb::RANK_6);

    // Consistency between bitboard-wide shift and individual square iteration
    Bitboard random_pawns = bb::square_mask(Square::A2) | bb::square_mask(Square::C3) |
                            bb::square_mask(Square::F4) | bb::square_mask(Square::H6);

    Bitboard expected_white = white_pawn_attacks(Square::A2) | white_pawn_attacks(Square::C3) |
                             white_pawn_attacks(Square::F4) | white_pawn_attacks(Square::H6);
    EXPECT_EQ(white_pawn_attacks_from_bb(random_pawns), expected_white);
    EXPECT_EQ(pawn_attacks_from_bb(Color::White, random_pawns), expected_white);

    Bitboard expected_black = black_pawn_attacks(Square::A2) | black_pawn_attacks(Square::C3) |
                             black_pawn_attacks(Square::F4) | black_pawn_attacks(Square::H6);
    EXPECT_EQ(black_pawn_attacks_from_bb(random_pawns), expected_black);
    EXPECT_EQ(pawn_attacks_from_bb(Color::Black, random_pawns), expected_black);

    // Directional attacks
    EXPECT_EQ(white_pawn_north_west_attacks(bb::square_mask(Square::E4)), bb::square_mask(Square::D5));
    EXPECT_EQ(white_pawn_north_east_attacks(bb::square_mask(Square::E4)), bb::square_mask(Square::F5));
    EXPECT_EQ(black_pawn_south_west_attacks(bb::square_mask(Square::E5)), bb::square_mask(Square::D4));
    EXPECT_EQ(black_pawn_south_east_attacks(bb::square_mask(Square::E5)), bb::square_mask(Square::F4));

    // Boundary directional shifts (A file cannot shift west, H file cannot shift east)
    EXPECT_EQ(white_pawn_north_west_attacks(bb::square_mask(Square::A2)), bb::EMPTY);
    EXPECT_EQ(white_pawn_north_east_attacks(bb::square_mask(Square::H2)), bb::EMPTY);
    EXPECT_EQ(black_pawn_south_west_attacks(bb::square_mask(Square::A7)), bb::EMPTY);
    EXPECT_EQ(black_pawn_south_east_attacks(bb::square_mask(Square::H7)), bb::EMPTY);
}

TEST(AttacksTest, PawnSingleAndDoublePushes) {
    // Unblocked starting positions
    Bitboard white_pawns = bb::RANK_2;
    Bitboard empty_board = bb::ALL_SQUARES;

    Bitboard white_single = white_pawn_single_pushes(white_pawns, empty_board);
    Bitboard white_double = white_pawn_double_pushes(white_pawns, empty_board);
    EXPECT_EQ(white_single, bb::RANK_3);
    EXPECT_EQ(white_double, bb::RANK_4);

    Bitboard black_pawns = bb::RANK_7;
    Bitboard black_single = black_pawn_single_pushes(black_pawns, empty_board);
    Bitboard black_double = black_pawn_double_pushes(black_pawns, empty_board);
    EXPECT_EQ(black_single, bb::RANK_6);
    EXPECT_EQ(black_double, bb::RANK_5);

    // Generic color overloads
    EXPECT_EQ(pawn_single_pushes(Color::White, white_pawns, empty_board), bb::RANK_3);
    EXPECT_EQ(pawn_double_pushes(Color::White, white_pawns, empty_board), bb::RANK_4);
    EXPECT_EQ(pawn_single_pushes(Color::Black, black_pawns, empty_board), bb::RANK_6);
    EXPECT_EQ(pawn_double_pushes(Color::Black, black_pawns, empty_board), bb::RANK_5);

    // Blocked pawns: piece on E3 blocks E2 single push AND double push
    Bitboard occupied_e3 = bb::ALL_SQUARES ^ bb::square_mask(Square::E3);
    Bitboard single_blocked_e3 = white_pawn_single_pushes(white_pawns, occupied_e3);
    Bitboard double_blocked_e3 = white_pawn_double_pushes(white_pawns, occupied_e3);
    EXPECT_FALSE(bb::test_bit(single_blocked_e3, Square::E3));
    EXPECT_FALSE(bb::test_bit(double_blocked_e3, Square::E4));

    // Blocked only on double push square: piece on E4 blocks double push, but E3 single push remains legal
    Bitboard occupied_e4 = bb::ALL_SQUARES ^ bb::square_mask(Square::E4);
    Bitboard single_e4 = white_pawn_single_pushes(white_pawns, occupied_e4);
    Bitboard double_e4 = white_pawn_double_pushes(white_pawns, occupied_e4);
    EXPECT_TRUE(bb::test_bit(single_e4, Square::E3));
    EXPECT_FALSE(bb::test_bit(double_e4, Square::E4));

    // Pawns on non-starting ranks cannot double push
    Bitboard white_rank3_pawns = bb::RANK_3;
    EXPECT_EQ(white_pawn_double_pushes(white_rank3_pawns, empty_board), bb::EMPTY);
    Bitboard black_rank6_pawns = bb::RANK_6;
    EXPECT_EQ(black_pawn_double_pushes(black_rank6_pawns, empty_board), bb::EMPTY);
}

TEST(AttacksTest, EnPassantAttackersAndTargetSquares) {
    // White captures en-passant on E6 (Black just played e7-e5)
    Square ep_sq = Square::E6;
    Bitboard white_pawns = bb::square_mask(Square::D5) | bb::square_mask(Square::F5) | bb::square_mask(Square::A5);
    Bitboard attackers = pawn_ep_attackers(Color::White, ep_sq, white_pawns);

    EXPECT_TRUE(bb::test_bit(attackers, Square::D5));
    EXPECT_TRUE(bb::test_bit(attackers, Square::F5));
    EXPECT_FALSE(bb::test_bit(attackers, Square::A5));
    EXPECT_EQ(bb::popcount(attackers), 2);

    // Captured pawn square for White EP at E6 is E5
    EXPECT_EQ(pawn_ep_captured_square(ep_sq, Color::White), Square::E5);

    // Black captures en-passant on D3 (White just played d2-d4)
    Square ep_sq_black = Square::D3;
    Bitboard black_pawns = bb::square_mask(Square::C4) | bb::square_mask(Square::E4) | bb::square_mask(Square::H4);
    Bitboard black_attackers = pawn_ep_attackers(Color::Black, ep_sq_black, black_pawns);

    EXPECT_TRUE(bb::test_bit(black_attackers, Square::C4));
    EXPECT_TRUE(bb::test_bit(black_attackers, Square::E4));
    EXPECT_FALSE(bb::test_bit(black_attackers, Square::H4));
    EXPECT_EQ(bb::popcount(black_attackers), 2);

    // Captured pawn square for Black EP at D3 is D4
    EXPECT_EQ(pawn_ep_captured_square(ep_sq_black, Color::Black), Square::D4);

    // Double push creates EP target square behind moving pawn
    EXPECT_EQ(ep_target_square(Square::E2, Color::White), Square::E3);
    EXPECT_EQ(ep_target_square(Square::D7, Color::Black), Square::D6);
}

TEST(AttacksTest, RookAttacksUnblockedExhaustiveAll64Squares) {
    EXPECT_EQ(rook_attacks(Square::None), bb::EMPTY);

    for (int i = 0; i < 64; ++i) {
        Square from = static_cast<Square>(i);
        Bitboard attacks = rook_attacks(from, bb::EMPTY);
        int count = bb::popcount(attacks);

        EXPECT_EQ(count, 14);
        EXPECT_FALSE(bb::test_bit(attacks, from));
        EXPECT_EQ(ROOK_RAYS[i], attacks);

        Bitboard remaining = attacks;
        while (remaining) {
            Square to = bb::pop_lsb(remaining);
            bool same_rank = square_rank(from) == square_rank(to);
            bool same_file = square_file(from) == square_file(to);
            EXPECT_TRUE(same_rank ^ same_file);
            EXPECT_TRUE(bb::test_bit(rook_attacks(to, bb::EMPTY), from));
        }
    }

    Bitboard expected_a1 = (bb::FILE_A | bb::RANK_1) ^ bb::square_mask(Square::A1);
    EXPECT_EQ(rook_attacks(Square::A1, bb::EMPTY), expected_a1);

    Bitboard expected_d4 = (bb::FILE_D | bb::RANK_4) ^ bb::square_mask(Square::D4);
    EXPECT_EQ(rook_attacks(Square::D4, bb::EMPTY), expected_d4);
}

TEST(AttacksTest, RookAttacksWithBlockers) {
    Square sq = Square::D4;

    Bitboard blockers = bb::square_mask(Square::D6) | bb::square_mask(Square::D2) |
                        bb::square_mask(Square::F4) | bb::square_mask(Square::B4);

    Bitboard expected = bb::square_mask(Square::D5) | bb::square_mask(Square::D6) |
                        bb::square_mask(Square::D3) | bb::square_mask(Square::D2) |
                        bb::square_mask(Square::E4) | bb::square_mask(Square::F4) |
                        bb::square_mask(Square::C4) | bb::square_mask(Square::B4);

    Bitboard attacks = rook_attacks(sq, blockers);
    EXPECT_EQ(attacks, expected);
    EXPECT_EQ(bb::popcount(attacks), 8);

    Bitboard immediate_blockers = bb::square_mask(Square::D5) | bb::square_mask(Square::D3) |
                                  bb::square_mask(Square::E4) | bb::square_mask(Square::C4);
    Bitboard immediate_attacks = rook_attacks(sq, immediate_blockers);
    EXPECT_EQ(immediate_attacks, immediate_blockers);
    EXPECT_EQ(bb::popcount(immediate_attacks), 4);
}

TEST(AttacksTest, BishopAttacksUnblockedExhaustiveAll64Squares) {
    EXPECT_EQ(bishop_attacks(Square::None), bb::EMPTY);

    for (int i = 0; i < 64; ++i) {
        Square from = static_cast<Square>(i);
        Bitboard attacks = bishop_attacks(from, bb::EMPTY);
        int count = bb::popcount(attacks);

        EXPECT_GE(count, 7);
        EXPECT_LE(count, 13);
        EXPECT_FALSE(bb::test_bit(attacks, from));
        EXPECT_EQ(BISHOP_RAYS[i], attacks);

        Bitboard remaining = attacks;
        while (remaining) {
            Square to = bb::pop_lsb(remaining);
            int df = std::abs(static_cast<int>(square_file(from)) - static_cast<int>(square_file(to)));
            int dr = std::abs(static_cast<int>(square_rank(from)) - static_cast<int>(square_rank(to)));
            EXPECT_EQ(df, dr);
            EXPECT_TRUE(bb::test_bit(bishop_attacks(to, bb::EMPTY), from));
        }
    }

    EXPECT_EQ(bb::popcount(bishop_attacks(Square::A1, bb::EMPTY)), 7);
    EXPECT_EQ(bb::popcount(bishop_attacks(Square::H1, bb::EMPTY)), 7);
    EXPECT_EQ(bb::popcount(bishop_attacks(Square::A8, bb::EMPTY)), 7);
    EXPECT_EQ(bb::popcount(bishop_attacks(Square::H8, bb::EMPTY)), 7);
    EXPECT_EQ(bb::popcount(bishop_attacks(Square::D4, bb::EMPTY)), 13);
    EXPECT_EQ(bb::popcount(bishop_attacks(Square::E5, bb::EMPTY)), 13);
}

TEST(AttacksTest, BishopAttacksWithBlockers) {
    Square sq = Square::D4;

    Bitboard blockers = bb::square_mask(Square::F6) | bb::square_mask(Square::B6) |
                        bb::square_mask(Square::F2) | bb::square_mask(Square::B2);

    Bitboard expected = bb::square_mask(Square::E5) | bb::square_mask(Square::F6) |
                        bb::square_mask(Square::C5) | bb::square_mask(Square::B6) |
                        bb::square_mask(Square::E3) | bb::square_mask(Square::F2) |
                        bb::square_mask(Square::C3) | bb::square_mask(Square::B2);

    Bitboard attacks = bishop_attacks(sq, blockers);
    EXPECT_EQ(attacks, expected);
    EXPECT_EQ(bb::popcount(attacks), 8);

    Bitboard immediate_blockers = bb::square_mask(Square::E5) | bb::square_mask(Square::C5) |
                                  bb::square_mask(Square::E3) | bb::square_mask(Square::C3);
    Bitboard immediate_attacks = bishop_attacks(sq, immediate_blockers);
    EXPECT_EQ(immediate_attacks, immediate_blockers);
    EXPECT_EQ(bb::popcount(immediate_attacks), 4);
}

TEST(AttacksTest, QueenAttacksUnblockedAndWithBlockers) {
    EXPECT_EQ(queen_attacks(Square::None), bb::EMPTY);

    for (int i = 0; i < 64; ++i) {
        Square sq = static_cast<Square>(i);
        Bitboard expected = bishop_attacks(sq, bb::EMPTY) | rook_attacks(sq, bb::EMPTY);
        EXPECT_EQ(queen_attacks(sq, bb::EMPTY), expected);
        EXPECT_EQ(QUEEN_RAYS[i], expected);
    }

    EXPECT_EQ(bb::popcount(queen_attacks(Square::A1, bb::EMPTY)), 21);
    EXPECT_EQ(bb::popcount(queen_attacks(Square::D4, bb::EMPTY)), 27);

    Square sq = Square::D4;
    Bitboard blockers = bb::square_mask(Square::D6) | bb::square_mask(Square::D2) |
                        bb::square_mask(Square::F4) | bb::square_mask(Square::B4) |
                        bb::square_mask(Square::F6) | bb::square_mask(Square::B6) |
                        bb::square_mask(Square::F2) | bb::square_mask(Square::B2);

    Bitboard expected_blocked = rook_attacks(sq, blockers) | bishop_attacks(sq, blockers);
    EXPECT_EQ(queen_attacks(sq, blockers), expected_blocked);
    EXPECT_EQ(bb::popcount(queen_attacks(sq, blockers)), 16);
}

TEST(AttacksTest, SlidingAttacksGenericDispatchAndBitboardAggregates) {
    Square sq = Square::D4;
    Bitboard blockers = bb::square_mask(Square::D6) | bb::square_mask(Square::F6);

    EXPECT_EQ(sliding_attacks(PieceType::Bishop, sq, blockers), bishop_attacks(sq, blockers));
    EXPECT_EQ(sliding_attacks(PieceType::Rook, sq, blockers), rook_attacks(sq, blockers));
    EXPECT_EQ(sliding_attacks(PieceType::Queen, sq, blockers), queen_attacks(sq, blockers));
    EXPECT_EQ(sliding_attacks(PieceType::Knight, sq, blockers), bb::EMPTY);

    EXPECT_EQ(attacks_by_type(PieceType::Knight, sq, blockers), knight_attacks(sq));
    EXPECT_EQ(attacks_by_type(PieceType::King, sq, blockers), king_attacks(sq));
    EXPECT_EQ(attacks_by_type(PieceType::Pawn, sq, blockers, Color::White), white_pawn_attacks(sq));
    EXPECT_EQ(attacks_by_type(PieceType::Bishop, sq, blockers), bishop_attacks(sq, blockers));
    EXPECT_EQ(attacks_by_type(PieceType::Rook, sq, blockers), rook_attacks(sq, blockers));
    EXPECT_EQ(attacks_by_type(PieceType::Queen, sq, blockers), queen_attacks(sq, blockers));

    Bitboard rooks = bb::square_mask(Square::A1) | bb::square_mask(Square::H8);
    EXPECT_EQ(rook_attacks_from_bb(rooks, blockers), rook_attacks(Square::A1, blockers) | rook_attacks(Square::H8, blockers));

    Bitboard bishops = bb::square_mask(Square::C1) | bb::square_mask(Square::F1);
    EXPECT_EQ(bishop_attacks_from_bb(bishops, blockers), bishop_attacks(Square::C1, blockers) | bishop_attacks(Square::F1, blockers));

    Bitboard queens = bb::square_mask(Square::D1);
    EXPECT_EQ(queen_attacks_from_bb(queens, blockers), queen_attacks(Square::D1, blockers));
}
