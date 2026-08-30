#include <gtest/gtest.h>
#include "board/fen.h"

using namespace chess;

TEST(FenTest, StartingPositionParseAndRoundTrip) {
    auto pos_opt = fen::parse(fen::START_FEN);
    ASSERT_TRUE(pos_opt.has_value());

    const Position& pos = pos_opt.value();
    Position start_pos(true);
    EXPECT_EQ(pos, start_pos);

    std::string round_trip = fen::to_string(pos);
    EXPECT_EQ(round_trip, fen::START_FEN);
}

TEST(FenTest, DiverseKnownPositionsRoundTrip) {
    const std::vector<std::string> test_fens = {
        // Kiwipete
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        // Position 3
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        // Position 4
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        // Position 5
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        // Position with En-Passant square on rank 6
        "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2",
        // Position with En-Passant square on rank 3
        "rnbqkbnr/pppp1ppp/8/8/4p3/5N2/PPPPPPPP/RNBQKB1R w KQkq - 0 3",
        // Endgame with no castling
        "8/8/4k3/8/8/4K3/8/8 w - - 45 100",
        // Black to move with partial castling
        "r1bqk2r/pppp1ppp/2n5/4p3/1b2P3/2NP1N2/PPP2PPP/R1BQK2R b Kkq - 1 6"
    };

    for (const auto& original_fen : test_fens) {
        auto pos_opt = fen::parse(original_fen);
        ASSERT_TRUE(pos_opt.has_value()) << "Failed to parse: " << original_fen;
        EXPECT_TRUE(pos_opt->validate_invariants()) << "Invariant failed for: " << original_fen;

        std::string serialized = fen::to_string(pos_opt.value());
        EXPECT_EQ(serialized, original_fen);
    }
}

TEST(FenTest, ShortFENDefaultMoveCounters) {
    std::string short_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";
    auto pos_opt = fen::parse(short_fen);
    ASSERT_TRUE(pos_opt.has_value());
    EXPECT_EQ(pos_opt->halfmove_clock(), 0);
    EXPECT_EQ(pos_opt->fullmove_number(), 1);
    EXPECT_EQ(fen::to_string(pos_opt.value()), fen::START_FEN);
}

TEST(FenTest, SafeRejectionOfInvalidFENs) {
    const std::vector<std::string> invalid_fens = {
        "",                                                            // Empty
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR",                 // Incomplete (< 4 tokens)
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w",               // Incomplete
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 extra", // Too many tokens
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR x KQkq - 0 1",    // Invalid side to move
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQx - 0 1",     // Invalid castling char
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq e4 0 1",    // Invalid EP rank (must be rank 3 or 6)
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq z9 0 1",    // Invalid EP square
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - -1 1",   // Negative halfmove
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 0",    // Fullmove must be >= 1
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 abc",  // Non-numeric fullmove
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP w KQkq - 0 1",             // Only 7 ranks
        "rnbqkbnr/pppppppp/8/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",  // 9 ranks
        "rnbqkbnr/ppppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",   // 9 files in a rank
        "rnbqkbnr/ppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",     // 7 files in a rank
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNX w KQkq - 0 1",    // Invalid piece character 'X'
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBN9 w KQkq - 0 1",    // Invalid empty count '9'
        "Pnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"     // White pawn on Rank 8 (illegal)
    };

    for (const auto& bad_fen : invalid_fens) {
        auto pos_opt = fen::parse(bad_fen);
        EXPECT_FALSE(pos_opt.has_value()) << "Should have rejected invalid FEN: " << bad_fen;
    }
}
