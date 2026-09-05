#include "move/movegen.h"

namespace chess {

void generate_pawn_moves(const Position& pos, MoveList& list, MoveGenType type) {
    Color us = pos.side_to_move();
    Color them = ~us;
    Bitboard pawns = pos.piece_bb(us, PieceType::Pawn);
    Bitboard all_occ = pos.all_occupancy();
    Bitboard enemy_occ = pos.color_occupancy(them);
    Square ep_sq = pos.en_passant_square();

    if (us == Color::White) {
        while (pawns) {
            Square from = bb::pop_lsb(pawns);
            Square push1 = static_cast<Square>(static_cast<uint8_t>(from) + 8);

            if (!bb::test_bit(all_occ, push1)) {
                if (square_rank(push1) == Rank::Rank8) {
                    if (type != MoveGenType::Quiets) {
                        list.push_back(Move::make(from, push1, MoveFlag::QueenPromotion));
                    }
                    if (type != MoveGenType::Captures) {
                        list.push_back(Move::make(from, push1, MoveFlag::RookPromotion));
                        list.push_back(Move::make(from, push1, MoveFlag::BishopPromotion));
                        list.push_back(Move::make(from, push1, MoveFlag::KnightPromotion));
                    }
                } else {
                    if (type != MoveGenType::Captures) {
                        list.push_back(Move::make(from, push1, MoveFlag::Quiet));
                    }
                    if (square_rank(from) == Rank::Rank2) {
                        Square push2 = static_cast<Square>(static_cast<uint8_t>(from) + 16);
                        if (!bb::test_bit(all_occ, push2)) {
                            if (type != MoveGenType::Captures) {
                                list.push_back(Move::make(from, push2, MoveFlag::DoublePush));
                            }
                        }
                    }
                }
            }

            Bitboard attacks = attacks::white_pawn_attacks(from);
            Bitboard caps = attacks & enemy_occ;
            while (caps) {
                Square to = bb::pop_lsb(caps);
                if (square_rank(to) == Rank::Rank8) {
                    if (type != MoveGenType::Quiets) {
                        list.push_back(Move::make(from, to, MoveFlag::QueenPromotionCapture));
                        list.push_back(Move::make(from, to, MoveFlag::RookPromotionCapture));
                        list.push_back(Move::make(from, to, MoveFlag::BishopPromotionCapture));
                        list.push_back(Move::make(from, to, MoveFlag::KnightPromotionCapture));
                    }
                } else {
                    if (type != MoveGenType::Quiets) {
                        list.push_back(Move::make(from, to, MoveFlag::Capture));
                    }
                }
            }

            if (ep_sq != Square::None && type != MoveGenType::Quiets) {
                if (bb::test_bit(attacks, ep_sq)) {
                    list.push_back(Move::make(from, ep_sq, MoveFlag::EnPassant));
                }
            }
        }
    } else {
        while (pawns) {
            Square from = bb::pop_lsb(pawns);
            Square push1 = static_cast<Square>(static_cast<uint8_t>(from) - 8);

            if (!bb::test_bit(all_occ, push1)) {
                if (square_rank(push1) == Rank::Rank1) {
                    if (type != MoveGenType::Quiets) {
                        list.push_back(Move::make(from, push1, MoveFlag::QueenPromotion));
                    }
                    if (type != MoveGenType::Captures) {
                        list.push_back(Move::make(from, push1, MoveFlag::RookPromotion));
                        list.push_back(Move::make(from, push1, MoveFlag::BishopPromotion));
                        list.push_back(Move::make(from, push1, MoveFlag::KnightPromotion));
                    }
                } else {
                    if (type != MoveGenType::Captures) {
                        list.push_back(Move::make(from, push1, MoveFlag::Quiet));
                    }
                    if (square_rank(from) == Rank::Rank7) {
                        Square push2 = static_cast<Square>(static_cast<uint8_t>(from) - 16);
                        if (!bb::test_bit(all_occ, push2)) {
                            if (type != MoveGenType::Captures) {
                                list.push_back(Move::make(from, push2, MoveFlag::DoublePush));
                            }
                        }
                    }
                }
            }

            Bitboard attacks = attacks::black_pawn_attacks(from);
            Bitboard caps = attacks & enemy_occ;
            while (caps) {
                Square to = bb::pop_lsb(caps);
                if (square_rank(to) == Rank::Rank1) {
                    if (type != MoveGenType::Quiets) {
                        list.push_back(Move::make(from, to, MoveFlag::QueenPromotionCapture));
                        list.push_back(Move::make(from, to, MoveFlag::RookPromotionCapture));
                        list.push_back(Move::make(from, to, MoveFlag::BishopPromotionCapture));
                        list.push_back(Move::make(from, to, MoveFlag::KnightPromotionCapture));
                    }
                } else {
                    if (type != MoveGenType::Quiets) {
                        list.push_back(Move::make(from, to, MoveFlag::Capture));
                    }
                }
            }

            if (ep_sq != Square::None && type != MoveGenType::Quiets) {
                if (bb::test_bit(attacks, ep_sq)) {
                    list.push_back(Move::make(from, ep_sq, MoveFlag::EnPassant));
                }
            }
        }
    }
}

void generate_knight_moves(const Position& pos, MoveList& list, MoveGenType type) {
    Color us = pos.side_to_move();
    Bitboard knights = pos.piece_bb(us, PieceType::Knight);
    Bitboard friendly_occ = pos.color_occupancy(us);
    Bitboard enemy_occ = pos.color_occupancy(~us);

    while (knights) {
        Square from = bb::pop_lsb(knights);
        Bitboard attacks = attacks::knight_attacks(from) & ~friendly_occ;

        if (type != MoveGenType::Quiets) {
            Bitboard caps = attacks & enemy_occ;
            while (caps) {
                Square to = bb::pop_lsb(caps);
                list.push_back(Move::make(from, to, MoveFlag::Capture));
            }
        }
        if (type != MoveGenType::Captures) {
            Bitboard quiets = attacks & ~pos.all_occupancy();
            while (quiets) {
                Square to = bb::pop_lsb(quiets);
                list.push_back(Move::make(from, to, MoveFlag::Quiet));
            }
        }
    }
}

void generate_bishop_moves(const Position& pos, MoveList& list, MoveGenType type) {
    Color us = pos.side_to_move();
    Bitboard bishops = pos.piece_bb(us, PieceType::Bishop);
    Bitboard all_occ = pos.all_occupancy();
    Bitboard friendly_occ = pos.color_occupancy(us);
    Bitboard enemy_occ = pos.color_occupancy(~us);

    while (bishops) {
        Square from = bb::pop_lsb(bishops);
        Bitboard attacks = attacks::bishop_attacks(from, all_occ) & ~friendly_occ;

        if (type != MoveGenType::Quiets) {
            Bitboard caps = attacks & enemy_occ;
            while (caps) {
                Square to = bb::pop_lsb(caps);
                list.push_back(Move::make(from, to, MoveFlag::Capture));
            }
        }
        if (type != MoveGenType::Captures) {
            Bitboard quiets = attacks & ~all_occ;
            while (quiets) {
                Square to = bb::pop_lsb(quiets);
                list.push_back(Move::make(from, to, MoveFlag::Quiet));
            }
        }
    }
}

void generate_rook_moves(const Position& pos, MoveList& list, MoveGenType type) {
    Color us = pos.side_to_move();
    Bitboard rooks = pos.piece_bb(us, PieceType::Rook);
    Bitboard all_occ = pos.all_occupancy();
    Bitboard friendly_occ = pos.color_occupancy(us);
    Bitboard enemy_occ = pos.color_occupancy(~us);

    while (rooks) {
        Square from = bb::pop_lsb(rooks);
        Bitboard attacks = attacks::rook_attacks(from, all_occ) & ~friendly_occ;

        if (type != MoveGenType::Quiets) {
            Bitboard caps = attacks & enemy_occ;
            while (caps) {
                Square to = bb::pop_lsb(caps);
                list.push_back(Move::make(from, to, MoveFlag::Capture));
            }
        }
        if (type != MoveGenType::Captures) {
            Bitboard quiets = attacks & ~all_occ;
            while (quiets) {
                Square to = bb::pop_lsb(quiets);
                list.push_back(Move::make(from, to, MoveFlag::Quiet));
            }
        }
    }
}

void generate_queen_moves(const Position& pos, MoveList& list, MoveGenType type) {
    Color us = pos.side_to_move();
    Bitboard queens = pos.piece_bb(us, PieceType::Queen);
    Bitboard all_occ = pos.all_occupancy();
    Bitboard friendly_occ = pos.color_occupancy(us);
    Bitboard enemy_occ = pos.color_occupancy(~us);

    while (queens) {
        Square from = bb::pop_lsb(queens);
        Bitboard attacks = attacks::queen_attacks(from, all_occ) & ~friendly_occ;

        if (type != MoveGenType::Quiets) {
            Bitboard caps = attacks & enemy_occ;
            while (caps) {
                Square to = bb::pop_lsb(caps);
                list.push_back(Move::make(from, to, MoveFlag::Capture));
            }
        }
        if (type != MoveGenType::Captures) {
            Bitboard quiets = attacks & ~all_occ;
            while (quiets) {
                Square to = bb::pop_lsb(quiets);
                list.push_back(Move::make(from, to, MoveFlag::Quiet));
            }
        }
    }
}

void generate_king_moves(const Position& pos, MoveList& list, MoveGenType type) {
    Color us = pos.side_to_move();
    Bitboard king = pos.piece_bb(us, PieceType::King);
    Bitboard all_occ = pos.all_occupancy();
    Bitboard friendly_occ = pos.color_occupancy(us);
    Bitboard enemy_occ = pos.color_occupancy(~us);

    if (king) {
        Square from = bb::lsb(king);
        Bitboard attacks = attacks::king_attacks(from) & ~friendly_occ;

        if (type != MoveGenType::Quiets) {
            Bitboard caps = attacks & enemy_occ;
            while (caps) {
                Square to = bb::pop_lsb(caps);
                list.push_back(Move::make(from, to, MoveFlag::Capture));
            }
        }
        if (type != MoveGenType::Captures) {
            Bitboard quiets = attacks & ~all_occ;
            while (quiets) {
                Square to = bb::pop_lsb(quiets);
                list.push_back(Move::make(from, to, MoveFlag::Quiet));
            }
        }
    }
}

bool is_castling_path_clear(const Position& pos, MoveFlag flag) noexcept {
    Color us = pos.side_to_move();
    Bitboard all_occ = pos.all_occupancy();

    if (us == Color::White) {
        if (flag == MoveFlag::CastleKingside) {
            return pos.piece_at(Square::E1) == Piece::WhiteKing &&
                   pos.piece_at(Square::H1) == Piece::WhiteRook &&
                   !bb::test_bit(all_occ, Square::F1) &&
                   !bb::test_bit(all_occ, Square::G1);
        } else if (flag == MoveFlag::CastleQueenside) {
            return pos.piece_at(Square::E1) == Piece::WhiteKing &&
                   pos.piece_at(Square::A1) == Piece::WhiteRook &&
                   !bb::test_bit(all_occ, Square::B1) &&
                   !bb::test_bit(all_occ, Square::C1) &&
                   !bb::test_bit(all_occ, Square::D1);
        }
    } else {
        if (flag == MoveFlag::CastleKingside) {
            return pos.piece_at(Square::E8) == Piece::BlackKing &&
                   pos.piece_at(Square::H8) == Piece::BlackRook &&
                   !bb::test_bit(all_occ, Square::F8) &&
                   !bb::test_bit(all_occ, Square::G8);
        } else if (flag == MoveFlag::CastleQueenside) {
            return pos.piece_at(Square::E8) == Piece::BlackKing &&
                   pos.piece_at(Square::A8) == Piece::BlackRook &&
                   !bb::test_bit(all_occ, Square::B8) &&
                   !bb::test_bit(all_occ, Square::C8) &&
                   !bb::test_bit(all_occ, Square::D8);
        }
    }
    return false;
}

void generate_castling_moves(const Position& pos, MoveList& list) {
    Color us = pos.side_to_move();
    uint8_t cr = pos.castling_rights();

    if (us == Color::White) {
        if ((cr & Castling::WhiteOO) && is_castling_path_clear(pos, MoveFlag::CastleKingside)) {
            list.push_back(Move::make(Square::E1, Square::G1, MoveFlag::CastleKingside));
        }
        if ((cr & Castling::WhiteOOO) && is_castling_path_clear(pos, MoveFlag::CastleQueenside)) {
            list.push_back(Move::make(Square::E1, Square::C1, MoveFlag::CastleQueenside));
        }
    } else {
        if ((cr & Castling::BlackOO) && is_castling_path_clear(pos, MoveFlag::CastleKingside)) {
            list.push_back(Move::make(Square::E8, Square::G8, MoveFlag::CastleKingside));
        }
        if ((cr & Castling::BlackOOO) && is_castling_path_clear(pos, MoveFlag::CastleQueenside)) {
            list.push_back(Move::make(Square::E8, Square::C8, MoveFlag::CastleQueenside));
        }
    }
}

void generate_pseudo_legal_moves(const Position& pos, MoveList& list, MoveGenType type) {
    generate_pawn_moves(pos, list, type);
    generate_knight_moves(pos, list, type);
    generate_bishop_moves(pos, list, type);
    generate_rook_moves(pos, list, type);
    generate_queen_moves(pos, list, type);
    generate_king_moves(pos, list, type);
    if (type != MoveGenType::Captures) {
        generate_castling_moves(pos, list);
    }
}

Bitboard attacks_to_square(const Position& pos, Square sq, Color attacker_color) noexcept {
    if (!is_valid_square(sq)) return bb::EMPTY;
    Bitboard attackers = bb::EMPTY;
    Bitboard all_occ = pos.all_occupancy();

    attackers |= attacks::pawn_attacks(~attacker_color, sq) & pos.piece_bb(attacker_color, PieceType::Pawn);
    attackers |= attacks::knight_attacks(sq) & pos.piece_bb(attacker_color, PieceType::Knight);
    attackers |= attacks::king_attacks(sq) & pos.piece_bb(attacker_color, PieceType::King);

    Bitboard diag_sliders = pos.piece_bb(attacker_color, PieceType::Bishop) | pos.piece_bb(attacker_color, PieceType::Queen);
    if (diag_sliders) {
        attackers |= attacks::bishop_attacks(sq, all_occ) & diag_sliders;
    }

    Bitboard ortho_sliders = pos.piece_bb(attacker_color, PieceType::Rook) | pos.piece_bb(attacker_color, PieceType::Queen);
    if (ortho_sliders) {
        attackers |= attacks::rook_attacks(sq, all_occ) & ortho_sliders;
    }

    return attackers;
}

bool is_square_attacked(const Position& pos, Square sq, Color attacker_color) noexcept {
    return attacks_to_square(pos, sq, attacker_color) != bb::EMPTY;
}

bool is_in_check(const Position& pos, Color c) noexcept {
    Bitboard king_bb = pos.piece_bb(c, PieceType::King);
    if (!king_bb) return false;
    Square king_sq = bb::lsb(king_bb);
    return is_square_attacked(pos, king_sq, ~c);
}

bool is_castling_legal(const Position& pos, MoveFlag flag) noexcept {
    Color us = pos.side_to_move();
    Color them = ~us;

    if (!is_castling_path_clear(pos, flag)) return false;
    if (is_in_check(pos, us)) return false;

    if (us == Color::White) {
        if (flag == MoveFlag::CastleKingside) {
            return !is_square_attacked(pos, Square::F1, them) &&
                   !is_square_attacked(pos, Square::G1, them);
        } else if (flag == MoveFlag::CastleQueenside) {
            return !is_square_attacked(pos, Square::D1, them) &&
                   !is_square_attacked(pos, Square::C1, them);
        }
    } else {
        if (flag == MoveFlag::CastleKingside) {
            return !is_square_attacked(pos, Square::F8, them) &&
                   !is_square_attacked(pos, Square::G8, them);
        } else if (flag == MoveFlag::CastleQueenside) {
            return !is_square_attacked(pos, Square::D8, them) &&
                   !is_square_attacked(pos, Square::C8, them);
        }
    }
    return false;
}

bool is_legal_move(const Position& pos, Move m) noexcept {
    Color us = pos.side_to_move();

    if (m.is_castling()) {
        return is_castling_legal(pos, m.flag());
    }

    Position next = pos;
    if (m.is_en_passant()) {
        Square cap_sq = attacks::pawn_ep_captured_square(m.to(), us);
        next.remove_piece(cap_sq);
        next.move_piece(m.from(), m.to());
    } else {
        if (m.is_capture()) {
            next.remove_piece(m.to());
        }
        next.move_piece(m.from(), m.to());
        if (m.is_promotion()) {
            next.put_piece(make_piece(us, m.promotion_type()), m.to());
        }
    }

    return !is_in_check(next, us);
}

void generate_legal_moves(const Position& pos, MoveList& list, MoveGenType type) {
    MoveList pseudo_moves;
    generate_pseudo_legal_moves(pos, pseudo_moves, type);

    for (const auto& m : pseudo_moves) {
        if (is_legal_move(pos, m)) {
            list.push_back(m);
        }
    }
}

bool is_checkmate(const Position& pos) noexcept {
    if (!is_in_check(pos, pos.side_to_move())) return false;
    MoveList legal_moves;
    generate_legal_moves(pos, legal_moves);
    return legal_moves.empty();
}

bool is_stalemate(const Position& pos) noexcept {
    if (is_in_check(pos, pos.side_to_move())) return false;
    MoveList legal_moves;
    generate_legal_moves(pos, legal_moves);
    return legal_moves.empty();
}

} // namespace chess
