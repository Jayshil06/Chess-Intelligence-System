#include <gtest/gtest.h>
#include "search/search.h"
#include "board/position.h"
#include "board/fen.h"

using namespace chess;
using namespace chess::search;

TEST(SearchTest, NegamaxMateInOne) {
    auto parsed = fen::parse("r1bqkb1r/pppp1ppp/2n2n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 4 4");
    ASSERT_TRUE(parsed.has_value());
    Position pos = *parsed;

    Searcher searcher;
    int score = searcher.negamax(pos, 1);
    EXPECT_GE(score, MATE_SCORE - 5);
}

TEST(SearchTest, AlphaBetaPrunesNodesComparedToNegamax) {
    auto parsed = fen::parse("r1bqkb1r/pppp1ppp/2n2n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 4 4");
    ASSERT_TRUE(parsed.has_value());
    Position pos = *parsed;

    Searcher negamax_searcher;
    int score_nm = negamax_searcher.negamax(pos, 2);
    uint64_t nm_nodes = negamax_searcher.nodes();

    Searcher ab_searcher;
    int score_ab = ab_searcher.alpha_beta(pos, -INFINITY_SCORE, INFINITY_SCORE, 2);
    uint64_t ab_nodes = ab_searcher.nodes();

    EXPECT_EQ(score_nm, score_ab);
    EXPECT_LT(ab_nodes, nm_nodes);
}

TEST(SearchTest, QuiescenceSearchSolvesTacticalHorizon) {
    auto parsed = fen::parse("rnb1kbnr/pppppppp/8/8/8/3q4/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    ASSERT_TRUE(parsed.has_value());
    Position pos = *parsed;

    Searcher searcher;
    int q_score = searcher.quiescence(pos, -INFINITY_SCORE, INFINITY_SCORE);
    EXPECT_GT(q_score, 500);
}

TEST(SearchTest, IterativeDeepeningFindsMate) {
    auto parsed = fen::parse("r1bqkb1r/pppp1ppp/2n2n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 4 4");
    ASSERT_TRUE(parsed.has_value());
    Position pos = *parsed;

    Searcher searcher;
    SearchLimits limits;
    limits.max_depth = 2;
    SearchResult result = searcher.search(pos, limits);

    EXPECT_EQ(result.best_move.from(), Square::H5);
    EXPECT_EQ(result.best_move.to(), Square::F7);
    EXPECT_GE(result.score, MATE_SCORE - 5);
}

TEST(SearchTest, SearchRespectsTimeLimit) {
    Position pos(true);
    Searcher searcher;
    SearchLimits limits;
    limits.max_depth = 10;
    limits.max_time_ms = 50;

    auto start = std::chrono::steady_clock::now();
    SearchResult res = searcher.search(pos, limits);
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start
    ).count();

    EXPECT_TRUE(res.best_move.is_valid());
    EXPECT_GT(res.nodes, 0ULL);
    EXPECT_LT(elapsed, 500);
}
