#include <gtest/gtest.h>
#include "board/position.h"
#include "board/fen.h"
#include "move/movegen.h"
#include <random>

using namespace chess;

TEST(MakeUnmakeTest, QuietMoveRoundTrip) {
    Position pos(true);
    Position initial = pos;

    Move m = Move::make(Square::E2, Square::E4, MoveFlag::DoublePush);
    UndoState undo;
    pos.make_move(m, undo);

    EXPECT_EQ(pos.piece_at(Square::E2), Piece::None);
    EXPECT_EQ(pos.piece_at(Square::E4), Piece::WhitePawn);
    EXPECT_EQ(pos.en_passant_square(), Square::E3);
    EXPECT_EQ(pos.side_to_move(), Color::Black);
    EXPECT_TRUE(pos.validate_invariants());

    pos.unmake_move(undo);
    EXPECT_EQ(pos, initial);
    EXPECT_TRUE(pos.validate_invariants());
}

TEST(MakeUnmakeTest, CaptureMoveRoundTrip) {
    auto parsed = fen::parse("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2");
    ASSERT_TRUE(parsed.has_value());
    Position pos = *parsed;
    Position initial = pos;

    Move m = Move::make(Square::E4, Square::D5, MoveFlag::Capture);
    UndoState undo;
    pos.make_move(m, undo);

    EXPECT_EQ(pos.piece_at(Square::E4), Piece::None);
    EXPECT_EQ(pos.piece_at(Square::D5), Piece::WhitePawn);
    EXPECT_EQ(undo.captured_piece, Piece::BlackPawn);
    EXPECT_EQ(pos.side_to_move(), Color::Black);
    EXPECT_TRUE(pos.validate_invariants());

    pos.unmake_move(undo);
    EXPECT_EQ(pos, initial);
    EXPECT_EQ(pos.piece_at(Square::D5), Piece::BlackPawn);
    EXPECT_TRUE(pos.validate_invariants());
}

TEST(MakeUnmakeTest, EnPassantRoundTrip) {
    auto parsed = fen::parse("rnbqkbnr/ppp2ppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3");
    ASSERT_TRUE(parsed.has_value());
    Position pos = *parsed;
    Position initial = pos;

    Move m = Move::make(Square::E5, Square::D6, MoveFlag::EnPassant);
    UndoState undo;
    pos.make_move(m, undo);

    EXPECT_EQ(pos.piece_at(Square::E5), Piece::None);
    EXPECT_EQ(pos.piece_at(Square::D6), Piece::WhitePawn);
    EXPECT_EQ(pos.piece_at(Square::D5), Piece::None);
    EXPECT_EQ(undo.captured_piece, Piece::BlackPawn);
    EXPECT_TRUE(pos.validate_invariants());

    pos.unmake_move(undo);
    EXPECT_EQ(pos, initial);
    EXPECT_EQ(pos.piece_at(Square::E5), Piece::WhitePawn);
    EXPECT_EQ(pos.piece_at(Square::D5), Piece::BlackPawn);
    EXPECT_EQ(pos.piece_at(Square::D6), Piece::None);
    EXPECT_TRUE(pos.validate_invariants());
}

TEST(MakeUnmakeTest, CastlingRoundTrip) {
    auto parsed = fen::parse("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    ASSERT_TRUE(parsed.has_value());
    Position pos = *parsed;
    Position initial = pos;

    // White Kingside
    Move m_w_ks = Move::make(Square::E1, Square::G1, MoveFlag::CastleKingside);
    UndoState undo;
    pos.make_move(m_w_ks, undo);
    EXPECT_EQ(pos.piece_at(Square::E1), Piece::None);
    EXPECT_EQ(pos.piece_at(Square::G1), Piece::WhiteKing);
    EXPECT_EQ(pos.piece_at(Square::H1), Piece::None);
    EXPECT_EQ(pos.piece_at(Square::F1), Piece::WhiteRook);
    EXPECT_FALSE(pos.has_castling_rights(Color::White));
    EXPECT_TRUE(pos.validate_invariants());

    pos.unmake_move(undo);
    EXPECT_EQ(pos, initial);
    EXPECT_TRUE(pos.validate_invariants());

    // White Queenside
    Move m_w_qs = Move::make(Square::E1, Square::C1, MoveFlag::CastleQueenside);
    pos.make_move(m_w_qs, undo);
    EXPECT_EQ(pos.piece_at(Square::E1), Piece::None);
    EXPECT_EQ(pos.piece_at(Square::C1), Piece::WhiteKing);
    EXPECT_EQ(pos.piece_at(Square::A1), Piece::None);
    EXPECT_EQ(pos.piece_at(Square::D1), Piece::WhiteRook);
    EXPECT_TRUE(pos.validate_invariants());

    pos.unmake_move(undo);
    EXPECT_EQ(pos, initial);
    EXPECT_TRUE(pos.validate_invariants());
}

TEST(MakeUnmakeTest, PromotionAndPromotionCaptureRoundTrip) {
    auto parsed = fen::parse("r1b1k2r/pppp1Ppp/8/8/8/8/PPPP2PP/RNBQKBNR w KQkq - 0 1");
    ASSERT_TRUE(parsed.has_value());
    Position pos = *parsed;
    Position initial = pos;

    // Quiet Queen promotion
    Move m_promo = Move::make_promotion(Square::F7, Square::F8, PieceType::Queen);
    UndoState undo;
    pos.make_move(m_promo, undo);
    EXPECT_EQ(pos.piece_at(Square::F7), Piece::None);
    EXPECT_EQ(pos.piece_at(Square::F8), Piece::WhiteQueen);
    EXPECT_TRUE(pos.validate_invariants());

    pos.unmake_move(undo);
    EXPECT_EQ(pos, initial);
    EXPECT_EQ(pos.piece_at(Square::F7), Piece::WhitePawn);
    EXPECT_TRUE(pos.validate_invariants());

    // Promotion capture
    auto parsed2 = fen::parse("r1bqk2r/pppp1Ppp/8/8/8/8/PPPP2PP/RNBQKBNR w KQkq - 0 1");
    ASSERT_TRUE(parsed2.has_value());
    pos = *parsed2;
    initial = pos;

    Move m_promo_cap = Move::make_promotion(Square::F7, Square::E8, PieceType::Knight, true);
    pos.make_move(m_promo_cap, undo);
    EXPECT_EQ(pos.piece_at(Square::F7), Piece::None);
    EXPECT_EQ(pos.piece_at(Square::E8), Piece::WhiteKnight);
    EXPECT_EQ(undo.captured_piece, Piece::BlackKing);
    EXPECT_TRUE(pos.validate_invariants());

    pos.unmake_move(undo);
    EXPECT_EQ(pos, initial);
    EXPECT_EQ(pos.piece_at(Square::F7), Piece::WhitePawn);
    EXPECT_EQ(pos.piece_at(Square::E8), Piece::BlackKing);
    EXPECT_TRUE(pos.validate_invariants());
}

TEST(MakeUnmakeTest, InternalHistoryStack) {
    Position pos(true);
    Position initial = pos;

    EXPECT_TRUE(pos.make_move(Move::make(Square::E2, Square::E4, MoveFlag::DoublePush)));
    EXPECT_EQ(pos.history().size(), 1);
    EXPECT_TRUE(pos.make_move(Move::make(Square::E7, Square::E5, MoveFlag::DoublePush)));
    EXPECT_EQ(pos.history().size(), 2);
    EXPECT_TRUE(pos.make_move(Move::make(Square::G1, Square::F3, MoveFlag::Quiet)));
    EXPECT_EQ(pos.history().size(), 3);

    EXPECT_TRUE(pos.unmake_move());
    EXPECT_EQ(pos.history().size(), 2);
    EXPECT_TRUE(pos.unmake_move());
    EXPECT_EQ(pos.history().size(), 1);
    EXPECT_TRUE(pos.unmake_move());
    EXPECT_EQ(pos.history().size(), 0);

    EXPECT_FALSE(pos.unmake_move());
    EXPECT_EQ(pos, initial);
}

TEST(MakeUnmakeTest, OneThousandRandomMakeUnmakeSequences) {
    std::mt19937 rng(42);

    const std::vector<std::string> test_fens = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"
    };

    for (const auto& fen_str : test_fens) {
        auto parsed = fen::parse(fen_str);
        ASSERT_TRUE(parsed.has_value());
        Position base_pos = *parsed;

        for (int iter = 0; iter < 200; ++iter) {
            Position current = base_pos;
            std::vector<UndoState> undo_stack;

            // Make up to 6 random legal moves
            int depth = std::uniform_int_distribution<int>(1, 6)(rng);
            for (int d = 0; d < depth; ++d) {
                MoveList legal_moves = generate_legal_moves(current);
                if (legal_moves.empty()) break;

                std::uniform_int_distribution<size_t> dist(0, legal_moves.size() - 1);
                Move chosen = legal_moves[dist(rng)];

                UndoState undo;
                current.make_move(chosen, undo);
                EXPECT_TRUE(current.validate_invariants());
                undo_stack.push_back(undo);
            }

            // Unmake all moves in reverse order
            while (!undo_stack.empty()) {
                current.unmake_move(undo_stack.back());
                undo_stack.pop_back();
                EXPECT_TRUE(current.validate_invariants());
            }

            // Must be identical to base position
            EXPECT_EQ(current, base_pos);
        }
    }
}
