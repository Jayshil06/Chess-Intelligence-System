#include <gtest/gtest.h>
#include "protocol/uci.h"
#include "board/fen.h"
#include <sstream>

using namespace chess;

TEST(UciTest, ParseMoveRestoresFlags) {
    auto pos = fen::parse("r3k2r/1P6/8/3pP3/8/8/8/R3K2R w KQkq d6 0 1");
    ASSERT_TRUE(pos.has_value());

    EXPECT_TRUE(uci::parse_move(*pos, "e1g1")->is_kingside_castling());
    EXPECT_TRUE(uci::parse_move(*pos, "e1c1")->is_queenside_castling());
    EXPECT_TRUE(uci::parse_move(*pos, "e5d6")->is_en_passant());
    EXPECT_EQ(uci::parse_move(*pos, "b7a8n")->flag(), MoveFlag::KnightPromotionCapture);
    EXPECT_EQ(uci::parse_move(*pos, "b7b8q")->flag(), MoveFlag::QueenPromotion);
    EXPECT_FALSE(uci::parse_move(*pos, "e1e3").has_value());
    EXPECT_FALSE(uci::parse_move(*pos, "b7b8").has_value());  // Promotion piece required
    EXPECT_FALSE(uci::parse_move(*pos, "garbage").has_value());
}

TEST(UciTest, FormatScore) {
    EXPECT_EQ(uci::format_score(35), "cp 35");
    EXPECT_EQ(uci::format_score(-120), "cp -120");
    EXPECT_EQ(uci::format_score(search::MATE_SCORE - 1), "mate 1");
    EXPECT_EQ(uci::format_score(search::MATE_SCORE - 3), "mate 2");
    EXPECT_EQ(uci::format_score(-search::MATE_SCORE + 2), "mate -1");
}

TEST(UciTest, HandshakeAndPosition) {
    std::ostringstream out;
    uci::Engine engine(out);

    engine.handle("uci");
    engine.handle("isready");
    EXPECT_NE(out.str().find("uciok"), std::string::npos);
    EXPECT_NE(out.str().find("readyok"), std::string::npos);

    engine.handle("position startpos moves e2e4 e7e5 g1f3");
    EXPECT_EQ(fen::to_string(engine.position()), "rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2");
    EXPECT_EQ(engine.position().history().size(), 3ULL);

    engine.handle("position fen 8/8/8/8/8/5k2/8/5K1R b - - 0 1 moves f3e3");
    EXPECT_EQ(fen::to_string(engine.position()), "8/8/8/8/8/4k3/8/5K1R w - - 1 2");
}

TEST(UciTest, RejectsBadInput) {
    std::ostringstream out;
    uci::Engine engine(out);

    engine.handle("position startpos moves e2e5");
    EXPECT_NE(out.str().find("illegal move: e2e5"), std::string::npos);
    EXPECT_EQ(fen::to_string(engine.position()), fen::START_FEN);

    engine.handle("position fen not a fen");
    EXPECT_NE(out.str().find("invalid fen"), std::string::npos);
    EXPECT_TRUE(engine.handle("foo"));
    EXPECT_FALSE(engine.handle("quit"));
}

TEST(UciTest, GoReportsInfoAndBestMove) {
    std::ostringstream out;
    uci::Engine engine(out);

    engine.handle("position fen r1bqkb1r/pppp1ppp/2n2n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 4 4");
    engine.handle("go depth 3");
    engine.wait();
    EXPECT_NE(out.str().find("info depth 1 score mate 1"), std::string::npos);
    EXPECT_NE(out.str().find("bestmove h5f7"), std::string::npos);
}

TEST(UciTest, StopEndsInfiniteSearch) {
    std::ostringstream out;
    uci::Engine engine(out);

    engine.handle("go infinite");
    engine.handle("stop");
    EXPECT_NE(out.str().find("bestmove "), std::string::npos);
}

TEST(UciTest, WaitingCommandEndsInfiniteSearch) {
    std::ostringstream out;
    uci::Engine engine(out);

    engine.handle("go infinite");
    engine.handle("position startpos");  // Must not hang without an explicit "stop"
    EXPECT_NE(out.str().find("bestmove "), std::string::npos);
}

TEST(UciTest, NegativeClockStillLimitsTime) {
    std::ostringstream out;
    uci::Engine engine(out);

    auto start = std::chrono::steady_clock::now();
    engine.handle("go wtime -20 btime 1000");
    engine.wait();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
    EXPECT_LT(ms, 500);
    EXPECT_NE(out.str().find("bestmove "), std::string::npos);
}

TEST(UciTest, BenchIsDeterministic) {
    std::ostringstream a, b;
    uint64_t n1 = uci::bench(a, 3);
    uint64_t n2 = uci::bench(b, 3);
    EXPECT_GT(n1, 0ULL);
    EXPECT_EQ(n1, n2);
}
