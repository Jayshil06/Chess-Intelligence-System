#include <gtest/gtest.h>
#include "evaluation/evaluate.h"
#include "board/position.h"
#include "board/fen.h"

using namespace chess;
using namespace chess::eval;

TEST(EvaluateTest, StartingPositionSymmetry) {
    Position pos(true);

    EXPECT_EQ(evaluate_material(pos, Color::White), evaluate_material(pos, Color::Black));
    EXPECT_EQ(evaluate_pst(pos, Color::White), evaluate_pst(pos, Color::Black));
    EXPECT_EQ(evaluate(pos), 0);

    pos.set_side_to_move(Color::Black);
    EXPECT_EQ(evaluate(pos), 0);
}

TEST(EvaluateTest, MaterialAdvantage) {
    auto parsed = fen::parse("rnb1kbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    ASSERT_TRUE(parsed.has_value());
    Position white_up_queen = *parsed;

    EXPECT_GT(evaluate(white_up_queen), 800);

    white_up_queen.set_side_to_move(Color::Black);
    EXPECT_LT(evaluate(white_up_queen), -800);
}

TEST(EvaluateTest, PieceSquareTablesPositioning) {
    Position pos;
    pos.put_piece(Piece::WhiteKing, Square::G1);
    pos.put_piece(Piece::BlackKing, Square::G8);
    pos.put_piece(Piece::WhiteKnight, Square::E4); // Central knight
    pos.put_piece(Piece::BlackKnight, Square::A8); // Rim knight in corner
    pos.set_side_to_move(Color::White);

    // Both sides have King + Knight, but White knight has central PST bonus
    EXPECT_GT(evaluate(pos), 40);
}

TEST(EvaluateTest, ColorFlippingSymmetry) {
    auto pos_w = fen::parse("r1bqkb1r/pppp1ppp/2n2n2/4p3/4P3/2N2N2/PPPP1PPP/R1BQKB1R w KQkq - 4 4");
    auto pos_b = fen::parse("r1bqkb1r/pppp1ppp/2n2n2/4p3/4P3/2N2N2/PPPP1PPP/R1BQKB1R b KQkq - 4 4");
    ASSERT_TRUE(pos_w.has_value());
    ASSERT_TRUE(pos_b.has_value());

    EXPECT_EQ(evaluate(*pos_w), -evaluate(*pos_b));
}
