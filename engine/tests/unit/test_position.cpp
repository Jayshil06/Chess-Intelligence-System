#include <gtest/gtest.h>
#include "board/position.h"

using namespace chess;

TEST(PositionTest, ClearAndEmptyPosition) {
    Position pos;
    EXPECT_TRUE(pos.validate_invariants());
    EXPECT_EQ(pos.all_occupancy(), bb::EMPTY);
    EXPECT_EQ(pos.empty_squares(), bb::ALL_SQUARES);
    EXPECT_EQ(pos.color_occupancy(Color::White), bb::EMPTY);
    EXPECT_EQ(pos.color_occupancy(Color::Black), bb::EMPTY);
    EXPECT_EQ(pos.side_to_move(), Color::White);
    EXPECT_EQ(pos.castling_rights(), Castling::None);
    EXPECT_EQ(pos.en_passant_square(), Square::None);
    EXPECT_EQ(pos.halfmove_clock(), 0);
    EXPECT_EQ(pos.fullmove_number(), 1);

    for (int i = 0; i < 64; ++i) {
        Square sq = static_cast<Square>(i);
        EXPECT_EQ(pos.piece_at(sq), Piece::None);
        EXPECT_EQ(pos.color_at(sq), Color::None);
        EXPECT_EQ(pos.type_at(sq), PieceType::None);
    }
}

TEST(PositionTest, StartingPositionIntegrity) {
    Position pos(true);
    EXPECT_TRUE(pos.validate_invariants());

    EXPECT_EQ(bb::popcount(pos.color_occupancy(Color::White)), 16);
    EXPECT_EQ(bb::popcount(pos.color_occupancy(Color::Black)), 16);
    EXPECT_EQ(bb::popcount(pos.all_occupancy()), 32);

    // Specific piece placements
    EXPECT_EQ(pos.piece_at(Square::E1), Piece::WhiteKing);
    EXPECT_EQ(pos.piece_at(Square::D1), Piece::WhiteQueen);
    EXPECT_EQ(pos.piece_at(Square::A1), Piece::WhiteRook);
    EXPECT_EQ(pos.piece_at(Square::H1), Piece::WhiteRook);
    EXPECT_EQ(pos.piece_at(Square::B1), Piece::WhiteKnight);
    EXPECT_EQ(pos.piece_at(Square::G1), Piece::WhiteKnight);
    EXPECT_EQ(pos.piece_at(Square::C1), Piece::WhiteBishop);
    EXPECT_EQ(pos.piece_at(Square::F1), Piece::WhiteBishop);

    EXPECT_EQ(pos.piece_at(Square::E8), Piece::BlackKing);
    EXPECT_EQ(pos.piece_at(Square::D8), Piece::BlackQueen);
    EXPECT_EQ(pos.piece_at(Square::A8), Piece::BlackRook);
    EXPECT_EQ(pos.piece_at(Square::H8), Piece::BlackRook);
    EXPECT_EQ(pos.piece_at(Square::B8), Piece::BlackKnight);
    EXPECT_EQ(pos.piece_at(Square::G8), Piece::BlackKnight);
    EXPECT_EQ(pos.piece_at(Square::C8), Piece::BlackBishop);
    EXPECT_EQ(pos.piece_at(Square::F8), Piece::BlackBishop);

    // Pawn ranks
    EXPECT_EQ(pos.piece_bb(Piece::WhitePawn), bb::RANK_2);
    EXPECT_EQ(pos.piece_bb(Piece::BlackPawn), bb::RANK_7);

    // Metadata
    EXPECT_EQ(pos.side_to_move(), Color::White);
    EXPECT_EQ(pos.castling_rights(), Castling::All);
    EXPECT_EQ(pos.en_passant_square(), Square::None);
    EXPECT_EQ(pos.halfmove_clock(), 0);
    EXPECT_EQ(pos.fullmove_number(), 1);
}

TEST(PositionTest, PutRemoveAndMovePiece) {
    Position pos;

    pos.put_piece(Piece::WhiteKnight, Square::E4);
    EXPECT_TRUE(pos.validate_invariants());
    EXPECT_EQ(pos.piece_at(Square::E4), Piece::WhiteKnight);
    EXPECT_EQ(pos.color_at(Square::E4), Color::White);
    EXPECT_EQ(pos.type_at(Square::E4), PieceType::Knight);
    EXPECT_EQ(pos.all_occupancy(), bb::square_mask(Square::E4));

    // Overwriting piece at E4 with a Black Queen
    pos.put_piece(Piece::BlackQueen, Square::E4);
    EXPECT_TRUE(pos.validate_invariants());
    EXPECT_EQ(pos.piece_at(Square::E4), Piece::BlackQueen);
    EXPECT_EQ(pos.color_at(Square::E4), Color::Black);
    EXPECT_EQ(pos.type_at(Square::E4), PieceType::Queen);

    // Moving piece
    pos.move_piece(Square::E4, Square::D5);
    EXPECT_TRUE(pos.validate_invariants());
    EXPECT_EQ(pos.piece_at(Square::E4), Piece::None);
    EXPECT_EQ(pos.piece_at(Square::D5), Piece::BlackQueen);

    // Removing piece
    pos.remove_piece(Square::D5);
    EXPECT_TRUE(pos.validate_invariants());
    EXPECT_EQ(pos.piece_at(Square::D5), Piece::None);
    EXPECT_EQ(pos.all_occupancy(), bb::EMPTY);
}

TEST(PositionTest, EnPassantAndCastlingMetadata) {
    Position pos(true);

    pos.set_en_passant_square(Square::E3);
    EXPECT_TRUE(pos.validate_invariants());
    EXPECT_EQ(pos.en_passant_square(), Square::E3);

    // Invalid en-passant rank fails invariant
    pos.set_en_passant_square(Square::E4);
    EXPECT_FALSE(pos.validate_invariants());
    pos.set_en_passant_square(Square::None);
    EXPECT_TRUE(pos.validate_invariants());

    // Castling mutators
    pos.set_castling_rights(Castling::WhiteOO | Castling::BlackOOO);
    EXPECT_EQ(pos.castling_rights(), Castling::WhiteOO | Castling::BlackOOO);
}

TEST(PositionTest, EqualityOperator) {
    Position pos1(true);
    Position pos2(true);
    EXPECT_EQ(pos1, pos2);

    pos2.set_side_to_move(Color::Black);
    EXPECT_NE(pos1, pos2);

    pos2 = pos1;
    pos2.move_piece(Square::E2, Square::E4);
    EXPECT_NE(pos1, pos2);
}
