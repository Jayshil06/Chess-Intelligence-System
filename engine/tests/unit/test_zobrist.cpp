#include <gtest/gtest.h>
#include "board/zobrist.h"
#include "board/position.h"
#include "board/fen.h"
#include "move/movegen.h"
#include <unordered_set>
#include <random>

using namespace chess;

TEST(ZobristTest, KeysIntegrityAndUniqueness) {
    std::unordered_set<uint64_t> all_keys;

    for (size_t p = 0; p < NUM_PIECES; ++p) {
        for (size_t s = 0; s < NUM_SQUARES; ++s) {
            uint64_t key = zobrist::ZOBRIST.piece_keys[p][s];
            EXPECT_NE(key, 0ULL);
            EXPECT_GT(bb::popcount(key), 16);
            EXPECT_LT(bb::popcount(key), 48);
            all_keys.insert(key);
        }
    }

    uint64_t side_k = zobrist::side_key();
    EXPECT_NE(side_k, 0ULL);
    all_keys.insert(side_k);

    for (size_t i = 0; i < 16; ++i) {
        uint64_t cr_k = zobrist::castling_key(static_cast<uint8_t>(i));
        EXPECT_NE(cr_k, 0ULL);
        all_keys.insert(cr_k);
    }

    for (size_t i = 0; i < NUM_FILES; ++i) {
        uint64_t ep_k = zobrist::en_passant_key(static_cast<File>(i));
        EXPECT_NE(ep_k, 0ULL);
        all_keys.insert(ep_k);
    }

    EXPECT_EQ(all_keys.size(), 12 * 64 + 1 + 16 + 8);
}

TEST(ZobristTest, StateSensitivity) {
    Position pos(true);
    uint64_t base_hash = pos.hash();
    EXPECT_EQ(base_hash, zobrist::compute_hash(pos));

    // Flipping side to move changes hash
    pos.set_side_to_move(Color::Black);
    pos.recalculate_hash();
    EXPECT_NE(pos.hash(), base_hash);
    EXPECT_EQ(pos.hash(), base_hash ^ zobrist::side_key());

    // Changing castling rights changes hash
    pos.reset_to_starting_position();
    pos.set_castling_rights(Castling::WhiteOO);
    pos.recalculate_hash();
    EXPECT_NE(pos.hash(), base_hash);

    // Setting en-passant square changes hash
    pos.reset_to_starting_position();
    pos.set_en_passant_square(Square::E3);
    pos.recalculate_hash();
    EXPECT_NE(pos.hash(), base_hash);
}

TEST(ZobristTest, IncrementalMatchesFullRecalculationAllMoves) {
    const std::vector<std::string> fens = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
        "rnbqkbnr/ppp2ppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3"
    };

    for (const auto& fen_str : fens) {
        auto parsed = fen::parse(fen_str);
        ASSERT_TRUE(parsed.has_value());
        Position pos = *parsed;

        MoveList moves = generate_legal_moves(pos);
        for (const auto& m : moves) {
            UndoState undo;
            pos.make_move(m, undo);

            EXPECT_EQ(pos.hash(), zobrist::compute_hash(pos));

            pos.unmake_move(undo);
            EXPECT_EQ(pos.hash(), zobrist::compute_hash(pos));
            EXPECT_EQ(pos.hash(), undo.zobrist_hash);
        }
    }
}

TEST(ZobristTest, CollisionResistanceAcrossMoves) {
    Position pos(true);
    std::unordered_set<uint64_t> hashes;
    hashes.insert(pos.hash());

    MoveList pl1 = generate_legal_moves(pos);
    for (const auto& m1 : pl1) {
        UndoState u1;
        pos.make_move(m1, u1);
        EXPECT_TRUE(hashes.insert(pos.hash()).second);

        MoveList pl2 = generate_legal_moves(pos);
        for (const auto& m2 : pl2) {
            UndoState u2;
            pos.make_move(m2, u2);
            EXPECT_TRUE(hashes.insert(pos.hash()).second);
            pos.unmake_move(u2);
        }

        pos.unmake_move(u1);
    }
}

TEST(ZobristTest, MakeUnmakeHashConsistency1000RandomSequences) {
    std::mt19937 rng(1337);

    const std::vector<std::string> test_fens = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"
    };

    for (const auto& fen_str : test_fens) {
        auto parsed = fen::parse(fen_str);
        ASSERT_TRUE(parsed.has_value());
        Position base_pos = *parsed;
        uint64_t base_hash = base_pos.hash();

        for (int iter = 0; iter < 250; ++iter) {
            Position current = base_pos;
            std::vector<UndoState> undo_stack;

            int depth = std::uniform_int_distribution<int>(1, 6)(rng);
            for (int d = 0; d < depth; ++d) {
                MoveList moves = generate_legal_moves(current);
                if (moves.empty()) break;

                std::uniform_int_distribution<size_t> dist(0, moves.size() - 1);
                Move m = moves[dist(rng)];

                UndoState undo;
                current.make_move(m, undo);
                EXPECT_EQ(current.hash(), zobrist::compute_hash(current));
                undo_stack.push_back(undo);
            }

            while (!undo_stack.empty()) {
                current.unmake_move(undo_stack.back());
                undo_stack.pop_back();
                EXPECT_EQ(current.hash(), zobrist::compute_hash(current));
            }

            EXPECT_EQ(current.hash(), base_hash);
            EXPECT_EQ(current, base_pos);
        }
    }
}
