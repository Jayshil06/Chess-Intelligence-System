#include <gtest/gtest.h>
#include "move/movegen.h"
#include "board/fen.h"

using namespace chess;

TEST(MoveGenTest, StartingPositionWhite) {
    Position pos(true);

    MoveList all_moves = generate_pseudo_legal_moves(pos, MoveGenType::All);
    EXPECT_EQ(all_moves.size(), 20);

    MoveList captures = generate_pseudo_legal_moves(pos, MoveGenType::Captures);
    EXPECT_EQ(captures.size(), 0);

    MoveList quiets = generate_pseudo_legal_moves(pos, MoveGenType::Quiets);
    EXPECT_EQ(quiets.size(), 20);

    MoveList pawn_moves;
    generate_pawn_moves(pos, pawn_moves);
    EXPECT_EQ(pawn_moves.size(), 16);

    MoveList knight_moves;
    generate_knight_moves(pos, knight_moves);
    EXPECT_EQ(knight_moves.size(), 4);

    EXPECT_TRUE(all_moves.contains(Move::make(Square::E2, Square::E4, MoveFlag::DoublePush)));
    EXPECT_TRUE(all_moves.contains(Move::make(Square::E2, Square::E3, MoveFlag::Quiet)));
    EXPECT_TRUE(all_moves.contains(Move::make(Square::G1, Square::F3, MoveFlag::Quiet)));
    EXPECT_TRUE(all_moves.contains(Move::make(Square::B1, Square::C3, MoveFlag::Quiet)));
}

TEST(MoveGenTest, StartingPositionBlack) {
    Position pos(true);
    pos.set_side_to_move(Color::Black);

    MoveList all_moves = generate_pseudo_legal_moves(pos, MoveGenType::All);
    EXPECT_EQ(all_moves.size(), 20);

    EXPECT_TRUE(all_moves.contains(Move::make(Square::E7, Square::E5, MoveFlag::DoublePush)));
    EXPECT_TRUE(all_moves.contains(Move::make(Square::E7, Square::E6, MoveFlag::Quiet)));
    EXPECT_TRUE(all_moves.contains(Move::make(Square::G8, Square::F6, MoveFlag::Quiet)));
    EXPECT_TRUE(all_moves.contains(Move::make(Square::B8, Square::C6, MoveFlag::Quiet)));
}

TEST(MoveGenTest, PawnPromotionsAndCaptures) {
    Position pos;
    pos.put_piece(Piece::WhiteKing, Square::A1);
    pos.put_piece(Piece::BlackKing, Square::H8);
    pos.put_piece(Piece::WhitePawn, Square::E7);
    pos.put_piece(Piece::BlackRook, Square::D8);
    pos.put_piece(Piece::BlackKnight, Square::F8);
    pos.set_side_to_move(Color::White);

    MoveList pawn_moves;
    generate_pawn_moves(pos, pawn_moves);

    // 4 quiet promotions (E7-E8) + 4 captures on D8 + 4 captures on F8 = 12 moves
    EXPECT_EQ(pawn_moves.size(), 12);

    EXPECT_TRUE(pawn_moves.contains(Move::make(Square::E7, Square::E8, MoveFlag::QueenPromotion)));
    EXPECT_TRUE(pawn_moves.contains(Move::make(Square::E7, Square::E8, MoveFlag::RookPromotion)));
    EXPECT_TRUE(pawn_moves.contains(Move::make(Square::E7, Square::E8, MoveFlag::BishopPromotion)));
    EXPECT_TRUE(pawn_moves.contains(Move::make(Square::E7, Square::E8, MoveFlag::KnightPromotion)));

    EXPECT_TRUE(pawn_moves.contains(Move::make(Square::E7, Square::D8, MoveFlag::QueenPromotionCapture)));
    EXPECT_TRUE(pawn_moves.contains(Move::make(Square::E7, Square::F8, MoveFlag::KnightPromotionCapture)));

    MoveList caps;
    generate_pawn_moves(pos, caps, MoveGenType::Captures);
    EXPECT_EQ(caps.size(), 9); // 8 promotion captures + 1 queen promotion

    MoveList quiets;
    generate_pawn_moves(pos, quiets, MoveGenType::Quiets);
    EXPECT_EQ(quiets.size(), 3); // 3 underpromotions
}

TEST(MoveGenTest, PawnEnPassant) {
    Position pos;
    pos.put_piece(Piece::WhiteKing, Square::E1);
    pos.put_piece(Piece::BlackKing, Square::E8);
    pos.put_piece(Piece::WhitePawn, Square::E5);
    pos.put_piece(Piece::BlackPawn, Square::D5);
    pos.set_en_passant_square(Square::D6);
    pos.set_side_to_move(Color::White);

    MoveList list;
    generate_pawn_moves(pos, list);

    EXPECT_TRUE(list.contains(Move::make(Square::E5, Square::D6, MoveFlag::EnPassant)));
    EXPECT_TRUE(list.contains(Move::make(Square::E5, Square::E6, MoveFlag::Quiet)));
}

TEST(MoveGenTest, KnightMovesSeparation) {
    Position pos;
    pos.put_piece(Piece::WhiteKing, Square::A1);
    pos.put_piece(Piece::BlackKing, Square::H8);
    pos.put_piece(Piece::WhiteKnight, Square::D4);
    pos.put_piece(Piece::BlackPawn, Square::C6);
    pos.put_piece(Piece::WhitePawn, Square::E6);
    pos.set_side_to_move(Color::White);

    MoveList moves;
    generate_knight_moves(pos, moves);

    // Knight attacks 8 squares. E6 is friendly (blocked), C6 is enemy (capture), 6 are empty (quiets)
    EXPECT_EQ(moves.size(), 7);

    MoveList caps;
    generate_knight_moves(pos, caps, MoveGenType::Captures);
    EXPECT_EQ(caps.size(), 1);
    EXPECT_TRUE(caps.contains(Move::make(Square::D4, Square::C6, MoveFlag::Capture)));

    MoveList quiets;
    generate_knight_moves(pos, quiets, MoveGenType::Quiets);
    EXPECT_EQ(quiets.size(), 6);
}

TEST(MoveGenTest, SlidingPiecesMoveGen) {
    Position pos;
    pos.put_piece(Piece::WhiteKing, Square::A1);
    pos.put_piece(Piece::BlackKing, Square::H8);
    pos.put_piece(Piece::WhiteRook, Square::D4);
    pos.put_piece(Piece::BlackPawn, Square::D6);
    pos.put_piece(Piece::WhitePawn, Square::B4);
    pos.set_side_to_move(Color::White);

    MoveList rook_moves;
    generate_rook_moves(pos, rook_moves);

    // North: D5, D6 (enemy capture, stops) = 2
    // South: D3, D2, D1 = 3
    // East: E4, F4, G4, H4 = 4
    // West: C4 (B4 is friendly blocker, stops) = 1
    // Total = 10 moves
    EXPECT_EQ(rook_moves.size(), 10);
    EXPECT_TRUE(rook_moves.contains(Move::make(Square::D4, Square::D6, MoveFlag::Capture)));
    EXPECT_FALSE(rook_moves.contains(Move::make(Square::D4, Square::D7, MoveFlag::Quiet)));
    EXPECT_FALSE(rook_moves.contains(Move::make(Square::D4, Square::B4, MoveFlag::Quiet)));
}

TEST(MoveGenTest, CastlingPseudoLegalGeneration) {
    Position pos;
    pos.put_piece(Piece::WhiteKing, Square::E1);
    pos.put_piece(Piece::WhiteRook, Square::A1);
    pos.put_piece(Piece::WhiteRook, Square::H1);
    pos.put_piece(Piece::BlackKing, Square::E8);
    pos.set_castling_rights(Castling::WhiteOO | Castling::WhiteOOO);
    pos.set_side_to_move(Color::White);

    MoveList castling;
    generate_castling_moves(pos, castling);
    EXPECT_EQ(castling.size(), 2);
    EXPECT_TRUE(castling.contains(Move::make(Square::E1, Square::G1, MoveFlag::CastleKingside)));
    EXPECT_TRUE(castling.contains(Move::make(Square::E1, Square::C1, MoveFlag::CastleQueenside)));

    // Obstruction on F1 blocks Kingside castling
    pos.put_piece(Piece::WhiteBishop, Square::F1);
    castling.clear();
    generate_castling_moves(pos, castling);
    EXPECT_EQ(castling.size(), 1);
    EXPECT_TRUE(castling.contains(Move::make(Square::E1, Square::C1, MoveFlag::CastleQueenside)));

    // Obstruction on B1 blocks Queenside castling
    pos.remove_piece(Square::F1);
    pos.put_piece(Piece::WhiteKnight, Square::B1);
    castling.clear();
    generate_castling_moves(pos, castling);
    EXPECT_EQ(castling.size(), 1);
    EXPECT_TRUE(castling.contains(Move::make(Square::E1, Square::G1, MoveFlag::CastleKingside)));
}

TEST(MoveGenTest, KiwipetePosition) {
    auto parsed = fen::parse("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    ASSERT_TRUE(parsed.has_value());
    const Position& pos = *parsed;

    MoveList moves = generate_pseudo_legal_moves(pos, MoveGenType::All);
    EXPECT_GT(moves.size(), 40);

    MoveList captures = generate_pseudo_legal_moves(pos, MoveGenType::Captures);
    MoveList quiets = generate_pseudo_legal_moves(pos, MoveGenType::Quiets);
    EXPECT_EQ(moves.size(), captures.size() + quiets.size());

    EXPECT_TRUE(moves.contains(Move::make(Square::E1, Square::G1, MoveFlag::CastleKingside)));
    EXPECT_TRUE(moves.contains(Move::make(Square::E1, Square::C1, MoveFlag::CastleQueenside)));
}

TEST(MoveGenTest, CastlingRightsUpdateMasks) {
    Position pos(true);
    EXPECT_EQ(pos.castling_rights(), Castling::All);
    EXPECT_TRUE(pos.can_castle(Castling::WhiteOO));
    EXPECT_TRUE(pos.can_castle(Castling::WhiteOOO));
    EXPECT_TRUE(pos.can_castle(Castling::BlackOO));
    EXPECT_TRUE(pos.can_castle(Castling::BlackOOO));

    // Non-king/rook move leaves castling rights intact
    pos.update_castling_rights(Square::E2, Square::E4);
    EXPECT_EQ(pos.castling_rights(), Castling::All);

    // White king move loses WhiteOO and WhiteOOO
    pos.update_castling_rights(Square::E1, Square::E2);
    EXPECT_EQ(pos.castling_rights(), Castling::BlackAll);
    EXPECT_FALSE(pos.has_castling_rights(Color::White));
    EXPECT_TRUE(pos.has_castling_rights(Color::Black));

    // Reset and test White H1 rook move
    pos.set_castling_rights(Castling::All);
    pos.update_castling_rights(Square::H1, Square::H3);
    EXPECT_EQ(pos.castling_rights(), Castling::WhiteOOO | Castling::BlackAll);

    // Test enemy piece capturing rook on A1
    pos.set_castling_rights(Castling::All);
    pos.update_castling_rights(Square::B3, Square::A1);
    EXPECT_EQ(pos.castling_rights(), Castling::WhiteOO | Castling::BlackAll);

    // Test Black king move loses BlackOO and BlackOOO
    pos.set_castling_rights(Castling::All);
    pos.update_castling_rights(Square::E8, Square::E7);
    EXPECT_EQ(pos.castling_rights(), Castling::WhiteAll);

    // Test Black H8 rook move
    pos.set_castling_rights(Castling::All);
    pos.update_castling_rights(Square::H8, Square::F8);
    EXPECT_EQ(pos.castling_rights(), Castling::WhiteAll | Castling::BlackOOO);

    // Test capture of Black A8 rook
    pos.set_castling_rights(Castling::All);
    pos.update_castling_rights(Square::B6, Square::A8);
    EXPECT_EQ(pos.castling_rights(), Castling::WhiteAll | Castling::BlackOO);
}

TEST(MoveGenTest, CastlingRookHelpers) {
    EXPECT_EQ(castling_rook_from(Square::G1), Square::H1);
    EXPECT_EQ(castling_rook_to(Square::G1), Square::F1);

    EXPECT_EQ(castling_rook_from(Square::C1), Square::A1);
    EXPECT_EQ(castling_rook_to(Square::C1), Square::D1);

    EXPECT_EQ(castling_rook_from(Square::G8), Square::H8);
    EXPECT_EQ(castling_rook_to(Square::G8), Square::F8);

    EXPECT_EQ(castling_rook_from(Square::C8), Square::A8);
    EXPECT_EQ(castling_rook_to(Square::C8), Square::D8);

    EXPECT_EQ(castling_rook_from(Square::E4), Square::None);
    EXPECT_EQ(castling_rook_to(Square::E4), Square::None);
}

TEST(MoveGenTest, CastlingBlackPathsAndRights) {
    Position pos;
    pos.put_piece(Piece::BlackKing, Square::E8);
    pos.put_piece(Piece::BlackRook, Square::A8);
    pos.put_piece(Piece::BlackRook, Square::H8);
    pos.put_piece(Piece::WhiteKing, Square::E1);
    pos.set_castling_rights(Castling::BlackOO | Castling::BlackOOO);
    pos.set_side_to_move(Color::Black);

    MoveList list;
    generate_castling_moves(pos, list);
    EXPECT_EQ(list.size(), 2);
    EXPECT_TRUE(list.contains(Move::make(Square::E8, Square::G8, MoveFlag::CastleKingside)));
    EXPECT_TRUE(list.contains(Move::make(Square::E8, Square::C8, MoveFlag::CastleQueenside)));

    // F8 occupied blocks Black Kingside castling
    pos.put_piece(Piece::BlackBishop, Square::F8);
    list.clear();
    generate_castling_moves(pos, list);
    EXPECT_EQ(list.size(), 1);
    EXPECT_TRUE(list.contains(Move::make(Square::E8, Square::C8, MoveFlag::CastleQueenside)));

    // D8 occupied blocks Black Queenside castling
    pos.remove_piece(Square::F8);
    pos.put_piece(Piece::BlackQueen, Square::D8);
    list.clear();
    generate_castling_moves(pos, list);
    EXPECT_EQ(list.size(), 1);
    EXPECT_TRUE(list.contains(Move::make(Square::E8, Square::G8, MoveFlag::CastleKingside)));
}

TEST(MoveGenTest, EnPassantStateAndDualAttackers) {
    // Test calculate_en_passant_square
    EXPECT_EQ(calculate_en_passant_square(Square::E2, Square::E4, PieceType::Pawn), Square::E3);
    EXPECT_EQ(calculate_en_passant_square(Square::D7, Square::D5, PieceType::Pawn), Square::D6);
    EXPECT_EQ(calculate_en_passant_square(Square::E2, Square::E3, PieceType::Pawn), Square::None);
    EXPECT_EQ(calculate_en_passant_square(Square::G1, Square::F3, PieceType::Knight), Square::None);

    // Two white pawns flanking an en-passant target square
    Position pos;
    pos.put_piece(Piece::WhiteKing, Square::E1);
    pos.put_piece(Piece::BlackKing, Square::E8);
    pos.put_piece(Piece::WhitePawn, Square::C5);
    pos.put_piece(Piece::WhitePawn, Square::E5);
    pos.put_piece(Piece::WhitePawn, Square::A5);
    pos.put_piece(Piece::BlackPawn, Square::D5);
    pos.set_en_passant_square(Square::D6);
    pos.set_side_to_move(Color::White);

    MoveList pawn_moves;
    generate_pawn_moves(pos, pawn_moves);

    // C5-D6 (EP) and E5-D6 (EP) are both generated
    EXPECT_TRUE(pawn_moves.contains(Move::make(Square::C5, Square::D6, MoveFlag::EnPassant)));
    EXPECT_TRUE(pawn_moves.contains(Move::make(Square::E5, Square::D6, MoveFlag::EnPassant)));
    // A5 pawn cannot capture D6 EP
    EXPECT_FALSE(pawn_moves.contains(Move::make(Square::A5, Square::D6, MoveFlag::EnPassant)));

    // Black dual EP attackers
    Position bpos;
    bpos.put_piece(Piece::WhiteKing, Square::E1);
    bpos.put_piece(Piece::BlackKing, Square::E8);
    bpos.put_piece(Piece::BlackPawn, Square::C4);
    bpos.put_piece(Piece::BlackPawn, Square::E4);
    bpos.put_piece(Piece::WhitePawn, Square::D4);
    bpos.set_en_passant_square(Square::D3);
    bpos.set_side_to_move(Color::Black);

    MoveList b_moves;
    generate_pawn_moves(bpos, b_moves);

    EXPECT_TRUE(b_moves.contains(Move::make(Square::C4, Square::D3, MoveFlag::EnPassant)));
    EXPECT_TRUE(b_moves.contains(Move::make(Square::E4, Square::D3, MoveFlag::EnPassant)));

    // Clear en-passant
    bpos.clear_en_passant();
    EXPECT_EQ(bpos.en_passant_square(), Square::None);
    b_moves.clear();
    generate_pawn_moves(bpos, b_moves);
    EXPECT_FALSE(b_moves.contains(Move::make(Square::C4, Square::D3, MoveFlag::EnPassant)));
}

TEST(MoveGenTest, AttackedSquareDetection) {
    Position pos(true);

    EXPECT_TRUE(is_square_attacked(pos, Square::D3, Color::White));
    EXPECT_TRUE(is_square_attacked(pos, Square::E3, Color::White));
    EXPECT_TRUE(is_square_attacked(pos, Square::F3, Color::White));
    EXPECT_TRUE(is_square_attacked(pos, Square::A3, Color::White));
    EXPECT_FALSE(is_square_attacked(pos, Square::E4, Color::White));

    EXPECT_TRUE(is_square_attacked(pos, Square::D6, Color::Black));
    EXPECT_TRUE(is_square_attacked(pos, Square::E6, Color::Black));
    EXPECT_FALSE(is_square_attacked(pos, Square::E5, Color::Black));

    Bitboard d3_attackers = attacks_to_square(pos, Square::D3, Color::White);
    EXPECT_TRUE(bb::test_bit(d3_attackers, Square::C2));
    EXPECT_TRUE(bb::test_bit(d3_attackers, Square::E2));
    EXPECT_EQ(bb::popcount(d3_attackers), 2);

    Bitboard c3_attackers = attacks_to_square(pos, Square::C3, Color::White);
    EXPECT_TRUE(bb::test_bit(c3_attackers, Square::B1));
    EXPECT_TRUE(bb::test_bit(c3_attackers, Square::B2));
    EXPECT_TRUE(bb::test_bit(c3_attackers, Square::D2));
    EXPECT_EQ(bb::popcount(c3_attackers), 3);
}

TEST(MoveGenTest, KingInCheckDetection) {
    Position pos(true);
    EXPECT_FALSE(is_in_check(pos, Color::White));
    EXPECT_FALSE(is_in_check(pos, Color::Black));

    Position check_pos;
    check_pos.put_piece(Piece::WhiteKing, Square::E1);
    check_pos.put_piece(Piece::BlackKing, Square::E8);
    check_pos.put_piece(Piece::BlackRook, Square::E4);
    EXPECT_TRUE(is_in_check(check_pos, Color::White));
    EXPECT_FALSE(is_in_check(check_pos, Color::Black));

    check_pos.put_piece(Piece::WhitePawn, Square::E2);
    EXPECT_FALSE(is_in_check(check_pos, Color::White));

    check_pos.put_piece(Piece::BlackKnight, Square::D3);
    EXPECT_TRUE(is_in_check(check_pos, Color::White));
}

TEST(MoveGenTest, PinnedPiecesAbsolutePin) {
    Position pos;
    pos.put_piece(Piece::WhiteKing, Square::E1);
    pos.put_piece(Piece::BlackKing, Square::A8);
    pos.put_piece(Piece::WhiteKnight, Square::E2);
    pos.put_piece(Piece::BlackRook, Square::E8);
    pos.set_side_to_move(Color::White);

    MoveList pseudo;
    generate_pseudo_legal_moves(pos, pseudo);
    EXPECT_TRUE(pseudo.contains(Move::make(Square::E2, Square::D4)));
    EXPECT_TRUE(pseudo.contains(Move::make(Square::E2, Square::F4)));

    MoveList legal;
    generate_legal_moves(pos, legal);
    EXPECT_FALSE(legal.contains(Move::make(Square::E2, Square::D4)));
    EXPECT_FALSE(legal.contains(Move::make(Square::E2, Square::F4)));

    Position pos2;
    pos2.put_piece(Piece::WhiteKing, Square::E1);
    pos2.put_piece(Piece::BlackKing, Square::A8);
    pos2.put_piece(Piece::WhiteRook, Square::E2);
    pos2.put_piece(Piece::BlackRook, Square::E8);
    pos2.set_side_to_move(Color::White);

    MoveList rook_legal;
    generate_legal_moves(pos2, rook_legal);
    EXPECT_TRUE(rook_legal.contains(Move::make(Square::E2, Square::E3, MoveFlag::Quiet)));
    EXPECT_TRUE(rook_legal.contains(Move::make(Square::E2, Square::E8, MoveFlag::Capture)));
    EXPECT_FALSE(rook_legal.contains(Move::make(Square::E2, Square::D2, MoveFlag::Quiet)));
    EXPECT_FALSE(rook_legal.contains(Move::make(Square::E2, Square::F2, MoveFlag::Quiet)));
}

TEST(MoveGenTest, CheckEvasions) {
    Position pos;
    pos.put_piece(Piece::WhiteKing, Square::E1);
    pos.put_piece(Piece::BlackKing, Square::A8);
    pos.put_piece(Piece::BlackRook, Square::E8);
    pos.put_piece(Piece::WhiteBishop, Square::D2);
    pos.put_piece(Piece::WhiteRook, Square::A7);
    pos.set_side_to_move(Color::White);

    ASSERT_TRUE(is_in_check(pos, Color::White));

    MoveList legal;
    generate_legal_moves(pos, legal);

    EXPECT_TRUE(legal.contains(Move::make(Square::E1, Square::D1, MoveFlag::Quiet)));
    EXPECT_TRUE(legal.contains(Move::make(Square::E1, Square::F1, MoveFlag::Quiet)));
    EXPECT_TRUE(legal.contains(Move::make(Square::E1, Square::F2, MoveFlag::Quiet)));
    EXPECT_TRUE(legal.contains(Move::make(Square::D2, Square::E3, MoveFlag::Quiet)));
    EXPECT_TRUE(legal.contains(Move::make(Square::A7, Square::E7, MoveFlag::Quiet)));

    for (const auto& m : legal) {
        Position simulated = pos;
        if (m.is_capture()) simulated.remove_piece(m.to());
        simulated.move_piece(m.from(), m.to());
        EXPECT_FALSE(is_in_check(simulated, Color::White));
    }
}

TEST(MoveGenTest, CastlingThroughCheckRejection) {
    Position pos;
    pos.put_piece(Piece::WhiteKing, Square::E1);
    pos.put_piece(Piece::WhiteRook, Square::A1);
    pos.put_piece(Piece::WhiteRook, Square::H1);
    pos.put_piece(Piece::BlackKing, Square::A8);
    pos.set_castling_rights(Castling::WhiteOO | Castling::WhiteOOO);
    pos.set_side_to_move(Color::White);

    EXPECT_TRUE(is_castling_legal(pos, MoveFlag::CastleKingside));
    EXPECT_TRUE(is_castling_legal(pos, MoveFlag::CastleQueenside));

    pos.put_piece(Piece::BlackRook, Square::E7);
    EXPECT_TRUE(is_in_check(pos, Color::White));
    EXPECT_FALSE(is_castling_legal(pos, MoveFlag::CastleKingside));
    EXPECT_FALSE(is_castling_legal(pos, MoveFlag::CastleQueenside));

    pos.remove_piece(Square::E7);
    pos.put_piece(Piece::BlackRook, Square::F7);
    EXPECT_FALSE(is_in_check(pos, Color::White));
    EXPECT_FALSE(is_castling_legal(pos, MoveFlag::CastleKingside));
    EXPECT_TRUE(is_castling_legal(pos, MoveFlag::CastleQueenside));

    pos.remove_piece(Square::F7);
    pos.put_piece(Piece::BlackRook, Square::G7);
    EXPECT_FALSE(is_castling_legal(pos, MoveFlag::CastleKingside));

    pos.remove_piece(Square::G7);
    pos.put_piece(Piece::BlackRook, Square::B7);
    EXPECT_TRUE(is_castling_legal(pos, MoveFlag::CastleQueenside));

    pos.remove_piece(Square::B7);
    pos.put_piece(Piece::BlackRook, Square::D7);
    EXPECT_FALSE(is_castling_legal(pos, MoveFlag::CastleQueenside));

    pos.remove_piece(Square::D7);
    pos.put_piece(Piece::BlackRook, Square::C7);
    EXPECT_FALSE(is_castling_legal(pos, MoveFlag::CastleQueenside));
}

TEST(MoveGenTest, CheckmateAndStalemate) {
    auto fools_mate = fen::parse("rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3");
    ASSERT_TRUE(fools_mate.has_value());
    EXPECT_TRUE(is_in_check(*fools_mate, Color::White));
    EXPECT_TRUE(is_checkmate(*fools_mate));
    EXPECT_FALSE(is_stalemate(*fools_mate));
    EXPECT_EQ(generate_legal_moves(*fools_mate).size(), 0);

    auto scholars_mate = fen::parse("r1bqkb1r/pppp1Qpp/2n2n2/4p3/2B1P3/8/PPPP1PPP/RNB1K1NR b KQkq - 0 4");
    ASSERT_TRUE(scholars_mate.has_value());
    EXPECT_TRUE(is_in_check(*scholars_mate, Color::Black));
    EXPECT_TRUE(is_checkmate(*scholars_mate));
    EXPECT_FALSE(is_stalemate(*scholars_mate));
    EXPECT_EQ(generate_legal_moves(*scholars_mate).size(), 0);

    auto stalemate_pos = fen::parse("k7/2Q5/1K6/8/8/8/8/8 b - - 0 1");
    ASSERT_TRUE(stalemate_pos.has_value());
    EXPECT_FALSE(is_in_check(*stalemate_pos, Color::Black));
    EXPECT_FALSE(is_checkmate(*stalemate_pos));
    EXPECT_TRUE(is_stalemate(*stalemate_pos));
    EXPECT_EQ(generate_legal_moves(*stalemate_pos).size(), 0);
}
