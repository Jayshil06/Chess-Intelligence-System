#include <gtest/gtest.h>
#include "search/tt.h"

using namespace chess;
using namespace chess::search;

TEST(TranspositionTableTest, SizeIsPowerOfTwo) {
    TranspositionTable tt(3);
    EXPECT_GT(tt.size(), 0ULL);
    EXPECT_EQ(tt.size() & (tt.size() - 1), 0ULL);
    EXPECT_LE(tt.size() * sizeof(TTEntry), 3ULL * 1024 * 1024);
}

TEST(TranspositionTableTest, StoreAndProbe) {
    TranspositionTable tt(1);
    const uint64_t key = 0x123456789ABCDEFULL;
    Move m(Square::E2, Square::E4, MoveFlag::DoublePush);

    EXPECT_EQ(tt.probe(key), nullptr);
    tt.store(key, 5, 42, Bound::Exact, m);

    const TTEntry* e = tt.probe(key);
    ASSERT_NE(e, nullptr);
    EXPECT_EQ(e->depth, 5);
    EXPECT_EQ(e->score, 42);
    EXPECT_EQ(e->bound, Bound::Exact);
    EXPECT_EQ(Move(e->move), m);

    // Same slot, different key must not match
    EXPECT_EQ(tt.probe(key ^ (static_cast<uint64_t>(tt.size()) << 1)), nullptr);
}

TEST(TranspositionTableTest, ReplacementKeepsDeeperEntries) {
    TranspositionTable tt(1);
    const uint64_t key = 0xABCDEFULL;
    Move m(Square::G1, Square::F3);

    tt.store(key, 8, 10, Bound::Lower, m);
    tt.store(key, 3, 20, Bound::Upper, Move::null());
    EXPECT_EQ(tt.probe(key)->depth, 8);  // Shallower bound ignored

    tt.store(key, 3, 30, Bound::Exact, Move::null());
    EXPECT_EQ(tt.probe(key)->score, 30);         // Exact always replaces
    EXPECT_EQ(Move(tt.probe(key)->move), m);     // Best move preserved
}

TEST(TranspositionTableTest, ClearAndHashfull) {
    TranspositionTable tt(1);
    EXPECT_EQ(tt.hashfull(), 0);
    for (uint64_t k = 0; k < tt.size(); ++k) tt.store(k, 1, 0, Bound::Exact, Move::null());
    EXPECT_EQ(tt.hashfull(), 1000);
    tt.clear();
    EXPECT_EQ(tt.hashfull(), 0);
    EXPECT_EQ(tt.probe(1), nullptr);
}
