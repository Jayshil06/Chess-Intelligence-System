#include <gtest/gtest.h>
#include "board/types.h"

using namespace chess;

TEST(ChessTypesTest, SquareMappingCorners) {
    EXPECT_EQ(static_cast<uint8_t>(Square::A1), 0);
    EXPECT_EQ(static_cast<uint8_t>(Square::H1), 7);
    EXPECT_EQ(static_cast<uint8_t>(Square::A8), 56);
    EXPECT_EQ(static_cast<uint8_t>(Square::H8), 63);

    EXPECT_EQ(square_file(Square::A1), File::FileA);
    EXPECT_EQ(square_rank(Square::A1), Rank::Rank1);

    EXPECT_EQ(square_file(Square::H1), File::FileH);
    EXPECT_EQ(square_rank(Square::H1), Rank::Rank1);

    EXPECT_EQ(square_file(Square::A8), File::FileA);
    EXPECT_EQ(square_rank(Square::A8), Rank::Rank8);

    EXPECT_EQ(square_file(Square::H8), File::FileH);
    EXPECT_EQ(square_rank(Square::H8), Rank::Rank8);
}

TEST(ChessTypesTest, MakeSquareAllCombinations) {
    for (int r = 0; r < 8; ++r) {
        for (int f = 0; f < 8; ++f) {
            File file = static_cast<File>(f);
            Rank rank = static_cast<Rank>(r);
            Square sq = make_square(file, rank);
            
            EXPECT_TRUE(is_valid_square(sq));
            EXPECT_EQ(square_file(sq), file);
            EXPECT_EQ(square_rank(sq), rank);
            EXPECT_EQ(static_cast<int>(sq), r * 8 + f);
        }
    }
}

TEST(ChessTypesTest, InvalidSquaresAndBounds) {
    EXPECT_FALSE(is_valid_square(Square::None));
    EXPECT_FALSE(is_valid_square(static_cast<Square>(64)));
    EXPECT_FALSE(is_valid_square(static_cast<Square>(100)));

    EXPECT_EQ(make_square(File::None, Rank::Rank1), Square::None);
    EXPECT_EQ(make_square(File::FileA, Rank::None), Square::None);
    EXPECT_EQ(square_file(Square::None), File::None);
    EXPECT_EQ(square_rank(Square::None), Rank::None);
}

TEST(ChessTypesTest, SquareStringRoundTrip) {
    for (int i = 0; i < 64; ++i) {
        Square sq = static_cast<Square>(i);
        std::string str = square_to_string(sq);
        EXPECT_EQ(str.length(), 2u);

        auto parsed = string_to_square(str);
        ASSERT_TRUE(parsed.has_value());
        EXPECT_EQ(parsed.value(), sq);
    }

    // Uppercase support
    auto e4_upper = string_to_square("E4");
    ASSERT_TRUE(e4_upper.has_value());
    EXPECT_EQ(e4_upper.value(), Square::E4);

    // Invalid string parsing
    EXPECT_FALSE(string_to_square("").has_value());
    EXPECT_FALSE(string_to_square("e").has_value());
    EXPECT_FALSE(string_to_square("e44").has_value());
    EXPECT_FALSE(string_to_square("i1").has_value());
    EXPECT_FALSE(string_to_square("a0").has_value());
    EXPECT_FALSE(string_to_square("a9").has_value());
}

TEST(ChessTypesTest, PieceColorAndTypeExtraction) {
    for (int c = 0; c < 2; ++c) {
        Color color = static_cast<Color>(c);
        for (int pt = 0; pt < 6; ++pt) {
            PieceType type = static_cast<PieceType>(pt);
            Piece p = make_piece(color, type);
            
            EXPECT_NE(p, Piece::None);
            EXPECT_EQ(color_of(p), color);
            EXPECT_EQ(type_of(p), type);
        }
    }

    EXPECT_EQ(make_piece(Color::None, PieceType::Pawn), Piece::None);
    EXPECT_EQ(make_piece(Color::White, PieceType::None), Piece::None);
    EXPECT_EQ(color_of(Piece::None), Color::None);
    EXPECT_EQ(type_of(Piece::None), PieceType::None);
}

TEST(ChessTypesTest, PieceCharRoundTrip) {
    const char white_chars[] = {'P', 'N', 'B', 'R', 'Q', 'K'};
    const char black_chars[] = {'p', 'n', 'b', 'r', 'q', 'k'};

    for (int pt = 0; pt < 6; ++pt) {
        Piece wp = make_piece(Color::White, static_cast<PieceType>(pt));
        Piece bp = make_piece(Color::Black, static_cast<PieceType>(pt));

        EXPECT_EQ(piece_to_char(wp), white_chars[pt]);
        EXPECT_EQ(piece_to_char(bp), black_chars[pt]);

        auto parsed_wp = char_to_piece(white_chars[pt]);
        ASSERT_TRUE(parsed_wp.has_value());
        EXPECT_EQ(parsed_wp.value(), wp);

        auto parsed_bp = char_to_piece(black_chars[pt]);
        ASSERT_TRUE(parsed_bp.has_value());
        EXPECT_EQ(parsed_bp.value(), bp);
    }

    EXPECT_EQ(piece_to_char(Piece::None), '.');
    EXPECT_FALSE(char_to_piece('x').has_value());
    EXPECT_FALSE(char_to_piece('1').has_value());
}

TEST(ChessTypesTest, ColorFlippingAndRelativeRanks) {
    EXPECT_EQ(flip_color(Color::White), Color::Black);
    EXPECT_EQ(flip_color(Color::Black), Color::White);
    EXPECT_EQ(flip_color(Color::None), Color::None);

    EXPECT_EQ(relative_rank(Color::White, Rank::Rank1), Rank::Rank1);
    EXPECT_EQ(relative_rank(Color::White, Rank::Rank8), Rank::Rank8);
    EXPECT_EQ(relative_rank(Color::Black, Rank::Rank1), Rank::Rank8);
    EXPECT_EQ(relative_rank(Color::Black, Rank::Rank8), Rank::Rank1);

    EXPECT_EQ(relative_square(Color::White, Square::E4), Square::E4);
    EXPECT_EQ(relative_square(Color::Black, Square::E4), Square::E5);
    EXPECT_EQ(relative_square(Color::Black, Square::A1), Square::A8);
    EXPECT_EQ(relative_square(Color::Black, Square::H8), Square::H1);
}

TEST(ChessTypesTest, Distances) {
    EXPECT_EQ(chebyshev_distance(Square::A1, Square::A1), 0);
    EXPECT_EQ(chebyshev_distance(Square::A1, Square::H8), 7);
    EXPECT_EQ(chebyshev_distance(Square::E4, Square::F5), 1);
    EXPECT_EQ(chebyshev_distance(Square::D4, Square::G7), 3);

    EXPECT_EQ(manhattan_distance(Square::A1, Square::A1), 0);
    EXPECT_EQ(manhattan_distance(Square::A1, Square::H8), 14);
    EXPECT_EQ(manhattan_distance(Square::E4, Square::F5), 2);
}
