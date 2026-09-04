#include <gtest/gtest.h>
#include "move/move.h"
#include <vector>
#include <algorithm>

using namespace chess;

TEST(MoveTest, DefaultAndNullMove) {
    Move m_default;
    Move m_null = Move::null();
    Move m_zero(0);

    EXPECT_TRUE(m_default.is_null());
    EXPECT_TRUE(m_null.is_null());
    EXPECT_TRUE(m_zero.is_null());
    EXPECT_FALSE(bool(m_default));

    EXPECT_EQ(m_default.to_uci(), "0000");
    EXPECT_EQ(m_default.to_string(), "0000");

    auto parsed_0000 = Move::from_uci("0000");
    ASSERT_TRUE(parsed_0000.has_value());
    EXPECT_TRUE(parsed_0000->is_null());

    auto parsed_none = Move::from_uci("none");
    ASSERT_TRUE(parsed_none.has_value());
    EXPECT_TRUE(parsed_none->is_null());
}

TEST(MoveTest, QuietMoveEncodingAndGetters) {
    Move m(Square::E2, Square::E4, MoveFlag::Quiet);

    EXPECT_EQ(m.from(), Square::E2);
    EXPECT_EQ(m.to(), Square::E4);
    EXPECT_EQ(m.flag(), MoveFlag::Quiet);
    EXPECT_TRUE(m.is_quiet());
    EXPECT_FALSE(m.is_capture());
    EXPECT_FALSE(m.is_promotion());
    EXPECT_FALSE(m.is_castling());
    EXPECT_FALSE(m.is_en_passant());
    EXPECT_FALSE(m.is_double_push());
    EXPECT_TRUE(m.is_valid());
    EXPECT_EQ(m.promotion_type(), PieceType::None);

    EXPECT_EQ(m.to_uci(), "e2e4");
}

TEST(MoveTest, DoublePushAndEnPassant) {
    Move double_push(Square::D2, Square::D4, MoveFlag::DoublePush);
    EXPECT_TRUE(double_push.is_double_push());
    EXPECT_FALSE(double_push.is_quiet());
    EXPECT_FALSE(double_push.is_capture());
    EXPECT_EQ(double_push.to_uci(), "d2d4");

    Move ep(Square::E5, Square::D6, MoveFlag::EnPassant);
    EXPECT_TRUE(ep.is_en_passant());
    EXPECT_TRUE(ep.is_capture());
    EXPECT_FALSE(ep.is_promotion());
    EXPECT_EQ(ep.to_uci(), "e5d6");
}

TEST(MoveTest, CastlingMoves) {
    Move w_ks(Square::E1, Square::G1, MoveFlag::CastleKingside);
    EXPECT_TRUE(w_ks.is_castling());
    EXPECT_TRUE(w_ks.is_kingside_castling());
    EXPECT_FALSE(w_ks.is_queenside_castling());
    EXPECT_FALSE(w_ks.is_capture());

    Move w_qs(Square::E1, Square::C1, MoveFlag::CastleQueenside);
    EXPECT_TRUE(w_qs.is_castling());
    EXPECT_TRUE(w_qs.is_queenside_castling());
    EXPECT_FALSE(w_qs.is_kingside_castling());

    Move b_ks(Square::E8, Square::G8, MoveFlag::CastleKingside);
    EXPECT_TRUE(b_ks.is_kingside_castling());

    Move b_qs(Square::E8, Square::C8, MoveFlag::CastleQueenside);
    EXPECT_TRUE(b_qs.is_queenside_castling());
}

TEST(MoveTest, PromotionsAndPromotionCaptures) {
    Move q_promo = Move::make_promotion(Square::E7, Square::E8, PieceType::Queen);
    EXPECT_TRUE(q_promo.is_promotion());
    EXPECT_FALSE(q_promo.is_capture());
    EXPECT_EQ(q_promo.promotion_type(), PieceType::Queen);
    EXPECT_EQ(q_promo.to_uci(), "e7e8q");

    Move r_promo = Move::make_promotion(Square::E7, Square::E8, PieceType::Rook);
    EXPECT_TRUE(r_promo.is_promotion());
    EXPECT_EQ(r_promo.promotion_type(), PieceType::Rook);
    EXPECT_EQ(r_promo.to_uci(), "e7e8r");

    Move b_promo = Move::make_promotion(Square::E7, Square::E8, PieceType::Bishop);
    EXPECT_TRUE(b_promo.is_promotion());
    EXPECT_EQ(b_promo.promotion_type(), PieceType::Bishop);
    EXPECT_EQ(b_promo.to_uci(), "e7e8b");

    Move n_promo = Move::make_promotion(Square::E7, Square::E8, PieceType::Knight);
    EXPECT_TRUE(n_promo.is_promotion());
    EXPECT_EQ(n_promo.promotion_type(), PieceType::Knight);
    EXPECT_EQ(n_promo.to_uci(), "e7e8n");

    Move q_cap_promo = Move::make_promotion(Square::E7, Square::D8, PieceType::Queen, true);
    EXPECT_TRUE(q_cap_promo.is_promotion());
    EXPECT_TRUE(q_cap_promo.is_capture());
    EXPECT_EQ(q_cap_promo.promotion_type(), PieceType::Queen);
    EXPECT_EQ(q_cap_promo.to_uci(), "e7d8q");
}

TEST(MoveTest, FromUciParsing) {
    auto m1 = Move::from_uci("e2e4");
    ASSERT_TRUE(m1.has_value());
    EXPECT_EQ(m1->from(), Square::E2);
    EXPECT_EQ(m1->to(), Square::E4);
    EXPECT_EQ(m1->to_uci(), "e2e4");

    auto m2 = Move::from_uci("g1f3");
    ASSERT_TRUE(m2.has_value());
    EXPECT_EQ(m2->from(), Square::G1);
    EXPECT_EQ(m2->to(), Square::F3);

    auto m3 = Move::from_uci("e7e8q");
    ASSERT_TRUE(m3.has_value());
    EXPECT_TRUE(m3->is_promotion());
    EXPECT_EQ(m3->promotion_type(), PieceType::Queen);

    auto m4 = Move::from_uci("a7a8r");
    ASSERT_TRUE(m4.has_value());
    EXPECT_EQ(m4->promotion_type(), PieceType::Rook);

    EXPECT_FALSE(Move::from_uci("").has_value());
    EXPECT_FALSE(Move::from_uci("e2").has_value());
    EXPECT_FALSE(Move::from_uci("e2e9").has_value());
    EXPECT_FALSE(Move::from_uci("z1z2").has_value());
    EXPECT_FALSE(Move::from_uci("e7e8k").has_value());
    EXPECT_FALSE(Move::from_uci("e2e4qq").has_value());
}

TEST(MoveTest, ComparisonsAndScoring) {
    Move m1(Square::E2, Square::E4, MoveFlag::Quiet);
    Move m2(Square::E2, Square::E4, MoveFlag::Quiet);
    Move m3(Square::E2, Square::E4, MoveFlag::DoublePush);
    Move m4(Square::D2, Square::D4, MoveFlag::Quiet);

    EXPECT_EQ(m1, m2);
    EXPECT_NE(m1, m3);
    EXPECT_NE(m1, m4);

    m1.set_score(100);
    m2.set_score(200);
    EXPECT_EQ(m1, m2);
    EXPECT_EQ(m1.score(), 100);
    EXPECT_EQ(m2.score(), 200);

    std::vector<Move> moves = {m4, m3, m1};
    std::sort(moves.begin(), moves.end());
    EXPECT_TRUE(std::is_sorted(moves.begin(), moves.end()));
}

TEST(MoveTest, MoveListOperations) {
    MoveList list;
    EXPECT_TRUE(list.empty());
    EXPECT_EQ(list.size(), 0);

    Move m1(Square::E2, Square::E4);
    Move m2(Square::D2, Square::D4);

    list.push_back(m1);
    list.push_back(m2);

    EXPECT_FALSE(list.empty());
    EXPECT_EQ(list.size(), 2);
    EXPECT_EQ(list[0], m1);
    EXPECT_EQ(list[1], m2);
    EXPECT_TRUE(list.contains(m1));
    EXPECT_TRUE(list.contains(m2));
    EXPECT_FALSE(list.contains(Move(Square::A2, Square::A4)));

    size_t count = 0;
    for (const auto& m : list) {
        (void)m;
        count++;
    }
    EXPECT_EQ(count, 2);

    list.clear();
    EXPECT_TRUE(list.empty());
    EXPECT_EQ(list.size(), 0);
}
