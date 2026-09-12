#include <gtest/gtest.h>
#include "board/position.h"
#include "board/fen.h"
#include "move/perft.h"

using namespace chess;

TEST(PerftTest, StartingPosition) {
    Position pos(true);

    EXPECT_EQ(perft(pos, 1), 20ULL);
    EXPECT_EQ(perft(pos, 2), 400ULL);
    EXPECT_EQ(perft(pos, 3), 8902ULL);
    EXPECT_EQ(perft(pos, 4), 197281ULL);
    EXPECT_EQ(perft(pos, 5), 4865609ULL);
}

TEST(PerftTest, Position2Kiwipete) {
    auto parsed = fen::parse("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    ASSERT_TRUE(parsed.has_value());
    Position pos = *parsed;

    EXPECT_EQ(perft(pos, 1), 48ULL);
    EXPECT_EQ(perft(pos, 2), 2039ULL);
    EXPECT_EQ(perft(pos, 3), 97862ULL);
    EXPECT_EQ(perft(pos, 4), 4085603ULL);
}

TEST(PerftTest, Position3) {
    auto parsed = fen::parse("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
    ASSERT_TRUE(parsed.has_value());
    Position pos = *parsed;

    EXPECT_EQ(perft(pos, 1), 14ULL);
    EXPECT_EQ(perft(pos, 2), 191ULL);
    EXPECT_EQ(perft(pos, 3), 2812ULL);
    EXPECT_EQ(perft(pos, 4), 43238ULL);
    EXPECT_EQ(perft(pos, 5), 674624ULL);
}

TEST(PerftTest, Position4) {
    auto parsed = fen::parse("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    ASSERT_TRUE(parsed.has_value());
    Position pos = *parsed;

    EXPECT_EQ(perft(pos, 1), 6ULL);
    EXPECT_EQ(perft(pos, 2), 264ULL);
    EXPECT_EQ(perft(pos, 3), 9467ULL);
    EXPECT_EQ(perft(pos, 4), 422333ULL);
}

TEST(PerftTest, Position5) {
    auto parsed = fen::parse("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
    ASSERT_TRUE(parsed.has_value());
    Position pos = *parsed;

    EXPECT_EQ(perft(pos, 1), 44ULL);
    EXPECT_EQ(perft(pos, 2), 1486ULL);
    EXPECT_EQ(perft(pos, 3), 62379ULL);
    EXPECT_EQ(perft(pos, 4), 2103487ULL);
}

TEST(PerftTest, Position6) {
    auto parsed = fen::parse("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");
    ASSERT_TRUE(parsed.has_value());
    Position pos = *parsed;

    EXPECT_EQ(perft(pos, 1), 46ULL);
    EXPECT_EQ(perft(pos, 2), 2079ULL);
    EXPECT_EQ(perft(pos, 3), 89890ULL);
    EXPECT_EQ(perft(pos, 4), 3894594ULL);
}

TEST(PerftTest, PerftDivide) {
    Position pos(true);
    PerftResults res = perft_divide(pos, 1);
    EXPECT_EQ(res.nodes, 20ULL);
    EXPECT_EQ(res.divide.size(), 20ULL);
}
