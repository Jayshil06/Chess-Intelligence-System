#include <gtest/gtest.h>
#include "evaluation/evaluate.h"
#include "board/position.h"
#include "board/fen.h"
#include "move/movegen.h"

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

TEST(EvaluateTest, EndgameKingPrefersCentre) {
    auto central = fen::parse("4k3/8/8/8/3K4/8/4P3/8 w - - 0 1");
    auto corner = fen::parse("4k3/8/8/8/8/8/4P3/K7 w - - 0 1");
    ASSERT_TRUE(central && corner);
    EXPECT_GT(evaluate(*central), evaluate(*corner) + 50);
}

TEST(EvaluateTest, IncrementalMatchesFromScratch) {
    Position pos = *fen::parse("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    uint64_t x = 12345;
    for (int ply = 0; ply < 200; ++ply) {
        MoveList moves = generate_legal_moves(pos);
        if (moves.empty()) break;
        x = x * 6364136223846793005ULL + 1442695040888963407ULL;
        pos.make_move(moves[static_cast<size_t>((x >> 33) % moves.size())]);

        ASSERT_TRUE(pos.validate_invariants()) << ply;
        int scratch = evaluate_material(pos, Color::White) - evaluate_material(pos, Color::Black) +
                      evaluate_pst(pos, Color::White) - evaluate_pst(pos, Color::Black);
        int white_view = pos.side_to_move() == Color::White ? evaluate(pos) : -evaluate(pos);
        ASSERT_NEAR(white_view, scratch, 2) << ply;  // Separate tapering rounds independently
    }
}
