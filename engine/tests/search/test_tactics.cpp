#include <gtest/gtest.h>
#include "search/search.h"
#include "board/fen.h"
#include "move/movegen.h"
#include "protocol/uci.h"

using namespace chess;
using namespace chess::search;

namespace {

Position from_fen(const char* fen) {
    auto pos = fen::parse(fen);
    EXPECT_TRUE(pos.has_value()) << fen;
    return pos.value_or(Position(true));
}

SearchResult search_depth(Position& pos, int depth) {
    Searcher searcher;
    SearchLimits limits;
    limits.max_depth = depth;
    return searcher.search(pos, limits);
}

} // namespace

TEST(TacticsTest, FindsMateInTwo) {
    // 1. Nf6+ gxf6 2. Bxf7#
    Position pos = from_fen("r2qkb1r/pp2nppp/3p4/2pNN1B1/2BnP3/3P4/PPP2PPP/R2bK2R w KQkq - 1 1");
    SearchResult r = search_depth(pos, 5);
    EXPECT_EQ(r.best_move.to_uci(), "d5f6");
    EXPECT_EQ(r.score, MATE_SCORE - 3);
}

TEST(TacticsTest, PrincipalVariationIsLegal) {
    Position pos(true);
    SearchResult r = search_depth(pos, 5);
    ASSERT_FALSE(r.pv.empty());
    EXPECT_EQ(r.pv.front(), r.best_move);
    for (Move m : r.pv) {
        ASSERT_TRUE(generate_legal_moves(pos).contains(m)) << m.to_uci();
        ASSERT_TRUE(pos.make_move(m));
    }
}

TEST(TacticsTest, InsufficientMaterialIsDraw) {
    Position pos = from_fen("8/8/8/4k3/8/8/2N5/4K3 w - - 0 1");
    EXPECT_EQ(search_depth(pos, 4).score, 0);
}

TEST(TacticsTest, ThreefoldRepetitionFromGameHistoryIsDraw) {
    // White's only legal move is Ka2; Black is up a rook
    Position pos = from_fen("7r/8/8/7p/7P/8/2k5/K7 w - - 0 1");
    EXPECT_LT(search_depth(pos, 4).score, -300);

    auto play_cycle = [&pos] {
        for (const char* uci : {"a1a2", "c2c1", "a2a1", "c1c2"}) {
            auto m = uci::parse_move(pos, uci);
            ASSERT_TRUE(m.has_value()) << uci;
            ASSERT_TRUE(pos.make_move(*m));
        }
    };

    play_cycle();  // Ka2 would only be a twofold repetition: still lost
    EXPECT_LT(search_depth(pos, 4).score, -300);

    play_cycle();  // Ka2 now completes a threefold repetition
    EXPECT_EQ(search_depth(pos, 4).score, 0);
}

TEST(TacticsTest, RespectsNodeLimit) {
    Position pos(true);
    Searcher searcher;
    SearchLimits limits;
    limits.max_depth = 30;
    limits.max_nodes = 5000;
    SearchResult r = searcher.search(pos, limits);
    EXPECT_TRUE(r.best_move.is_valid());
    EXPECT_LE(r.nodes, 5001ULL);
}

TEST(TacticsTest, ExternalStopFlagAbortsSearch) {
    Position pos(true);
    Searcher searcher;
    std::atomic<bool> stop{true};
    SearchLimits limits;
    limits.max_depth = 30;
    limits.stop = &stop;
    SearchResult r = searcher.search(pos, limits);
    EXPECT_TRUE(r.best_move.is_valid());  // Falls back to a legal move
    EXPECT_EQ(r.depth, 0);
}

TEST(TacticsTest, TranspositionTableSpeedsUpRepeatSearch) {
    Position pos = from_fen("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");
    Searcher searcher;
    SearchLimits limits;
    limits.max_depth = 5;
    SearchResult first = searcher.search(pos, limits);
    SearchResult second = searcher.search(pos, limits);
    EXPECT_LT(second.nodes, first.nodes);
    EXPECT_EQ(second.best_move, first.best_move);
}

TEST(TacticsTest, CastlingThroughCheckRejectedByCheckedMakeMove) {
    // Black bishop on a6 covers f1: O-O is pseudo-legal but illegal
    Position pos = from_fen("4k3/8/b7/8/8/8/8/4K2R w K - 0 1");
    EXPECT_FALSE(pos.make_move(Move(Square::E1, Square::G1, MoveFlag::CastleKingside)));
    EXPECT_EQ(pos.piece_at(Square::E1), Piece::WhiteKing);
    EXPECT_TRUE(pos.history().empty());
}

TEST(TacticsTest, AbortedIterationKeepsConsistentLegalBestMove) {
    Position root = from_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    MoveList legal = generate_legal_moves(root);
    for (uint64_t nodes = 500; nodes <= 64000; nodes *= 2) {
        Position pos = root;
        Searcher searcher;
        SearchLimits limits;
        limits.max_nodes = nodes;
        SearchResult r = searcher.search(pos, limits);
        ASSERT_TRUE(legal.contains(r.best_move)) << nodes;
        if (!r.pv.empty()) EXPECT_EQ(r.pv.front(), r.best_move) << nodes;
    }
}
