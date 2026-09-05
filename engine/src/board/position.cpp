#include "board/position.h"
#include "board/zobrist.h"
#include "move/attacks.h"
#include "move/movegen.h"
#include <algorithm>

namespace chess {

Position::Position() {
    clear();
}

Position::Position(bool set_startpos) {
    if (set_startpos) {
        reset_to_starting_position();
    } else {
        clear();
    }
}

void Position::clear() {
    m_pieces.fill(bb::EMPTY);
    m_occupancy_color.fill(bb::EMPTY);
    m_occupancy_all = bb::EMPTY;

    m_side_to_move = Color::White;
    m_castling_rights = Castling::None;
    m_en_passant_square = Square::None;
    m_halfmove_clock = 0;
    m_fullmove_number = 1;
    m_history.clear();
    m_zobrist_hash = 0ULL;
}

void Position::reset_to_starting_position() {
    clear();

    // White pieces
    m_pieces[static_cast<size_t>(Piece::WhitePawn)]   = bb::RANK_2;
    m_pieces[static_cast<size_t>(Piece::WhiteKnight)] = bb::square_mask(Square::B1) | bb::square_mask(Square::G1);
    m_pieces[static_cast<size_t>(Piece::WhiteBishop)] = bb::square_mask(Square::C1) | bb::square_mask(Square::F1);
    m_pieces[static_cast<size_t>(Piece::WhiteRook)]   = bb::square_mask(Square::A1) | bb::square_mask(Square::H1);
    m_pieces[static_cast<size_t>(Piece::WhiteQueen)]  = bb::square_mask(Square::D1);
    m_pieces[static_cast<size_t>(Piece::WhiteKing)]   = bb::square_mask(Square::E1);

    // Black pieces
    m_pieces[static_cast<size_t>(Piece::BlackPawn)]   = bb::RANK_7;
    m_pieces[static_cast<size_t>(Piece::BlackKnight)] = bb::square_mask(Square::B8) | bb::square_mask(Square::G8);
    m_pieces[static_cast<size_t>(Piece::BlackBishop)] = bb::square_mask(Square::C8) | bb::square_mask(Square::F8);
    m_pieces[static_cast<size_t>(Piece::BlackRook)]   = bb::square_mask(Square::A8) | bb::square_mask(Square::H8);
    m_pieces[static_cast<size_t>(Piece::BlackQueen)]  = bb::square_mask(Square::D8);
    m_pieces[static_cast<size_t>(Piece::BlackKing)]   = bb::square_mask(Square::E8);

    update_occupancies();

    m_side_to_move = Color::White;
    m_castling_rights = Castling::All;
    m_en_passant_square = Square::None;
    m_halfmove_clock = 0;
    m_fullmove_number = 1;
    recalculate_hash();
}

Bitboard Position::piece_bb(Piece p) const noexcept {
    if (p == Piece::None) return bb::EMPTY;
    return m_pieces[static_cast<size_t>(p)];
}

Bitboard Position::piece_bb(Color c, PieceType pt) const noexcept {
    Piece p = make_piece(c, pt);
    return piece_bb(p);
}

Bitboard Position::color_occupancy(Color c) const noexcept {
    if (c == Color::None) return bb::EMPTY;
    return m_occupancy_color[static_cast<size_t>(c)];
}

Bitboard Position::all_occupancy() const noexcept {
    return m_occupancy_all;
}

Bitboard Position::empty_squares() const noexcept {
    return ~m_occupancy_all;
}

Piece Position::piece_at(Square sq) const noexcept {
    if (!is_valid_square(sq) || !bb::test_bit(m_occupancy_all, sq)) {
        return Piece::None;
    }
    for (size_t i = 0; i < NUM_PIECES; ++i) {
        if (bb::test_bit(m_pieces[i], sq)) {
            return static_cast<Piece>(i);
        }
    }
    return Piece::None;
}

Color Position::color_at(Square sq) const noexcept {
    if (!is_valid_square(sq)) return Color::None;
    if (bb::test_bit(m_occupancy_color[static_cast<size_t>(Color::White)], sq)) return Color::White;
    if (bb::test_bit(m_occupancy_color[static_cast<size_t>(Color::Black)], sq)) return Color::Black;
    return Color::None;
}

PieceType Position::type_at(Square sq) const noexcept {
    Piece p = piece_at(sq);
    return type_of(p);
}

void Position::put_piece(Piece p, Square sq) noexcept {
    if (!is_valid_square(sq) || p == Piece::None) return;

    // Remove any piece currently occupying sq
    remove_piece(sq);

    size_t piece_idx = static_cast<size_t>(p);
    bb::set_bit(m_pieces[piece_idx], sq);

    Color c = color_of(p);
    bb::set_bit(m_occupancy_color[static_cast<size_t>(c)], sq);
    bb::set_bit(m_occupancy_all, sq);
}

void Position::remove_piece(Square sq) noexcept {
    if (!is_valid_square(sq) || !bb::test_bit(m_occupancy_all, sq)) return;

    for (size_t i = 0; i < NUM_PIECES; ++i) {
        if (bb::test_bit(m_pieces[i], sq)) {
            bb::clear_bit(m_pieces[i], sq);
            break;
        }
    }

    bb::clear_bit(m_occupancy_color[static_cast<size_t>(Color::White)], sq);
    bb::clear_bit(m_occupancy_color[static_cast<size_t>(Color::Black)], sq);
    bb::clear_bit(m_occupancy_all, sq);
}

void Position::move_piece(Square from, Square to) noexcept {
    if (!is_valid_square(from) || !is_valid_square(to) || from == to) return;

    Piece p = piece_at(from);
    if (p == Piece::None) return;

    remove_piece(from);
    put_piece(p, to);
}

void Position::update_occupancies() noexcept {
    m_occupancy_color[static_cast<size_t>(Color::White)] =
        m_pieces[static_cast<size_t>(Piece::WhitePawn)]   |
        m_pieces[static_cast<size_t>(Piece::WhiteKnight)] |
        m_pieces[static_cast<size_t>(Piece::WhiteBishop)] |
        m_pieces[static_cast<size_t>(Piece::WhiteRook)]   |
        m_pieces[static_cast<size_t>(Piece::WhiteQueen)]  |
        m_pieces[static_cast<size_t>(Piece::WhiteKing)];

    m_occupancy_color[static_cast<size_t>(Color::Black)] =
        m_pieces[static_cast<size_t>(Piece::BlackPawn)]   |
        m_pieces[static_cast<size_t>(Piece::BlackKnight)] |
        m_pieces[static_cast<size_t>(Piece::BlackBishop)] |
        m_pieces[static_cast<size_t>(Piece::BlackRook)]   |
        m_pieces[static_cast<size_t>(Piece::BlackQueen)]  |
        m_pieces[static_cast<size_t>(Piece::BlackKing)];

    m_occupancy_all = m_occupancy_color[static_cast<size_t>(Color::White)] |
                      m_occupancy_color[static_cast<size_t>(Color::Black)];
}

bool Position::validate_invariants() const noexcept {
    // 1. Bitboard disjointness: No two piece bitboards can overlap
    for (size_t i = 0; i < NUM_PIECES; ++i) {
        for (size_t j = i + 1; j < NUM_PIECES; ++j) {
            if ((m_pieces[i] & m_pieces[j]) != bb::EMPTY) {
                return false;
            }
        }
    }

    // 2. White occupancy must equal union of white pieces
    Bitboard expected_white =
        m_pieces[static_cast<size_t>(Piece::WhitePawn)]   |
        m_pieces[static_cast<size_t>(Piece::WhiteKnight)] |
        m_pieces[static_cast<size_t>(Piece::WhiteBishop)] |
        m_pieces[static_cast<size_t>(Piece::WhiteRook)]   |
        m_pieces[static_cast<size_t>(Piece::WhiteQueen)]  |
        m_pieces[static_cast<size_t>(Piece::WhiteKing)];

    if (m_occupancy_color[static_cast<size_t>(Color::White)] != expected_white) {
        return false;
    }

    // 3. Black occupancy must equal union of black pieces
    Bitboard expected_black =
        m_pieces[static_cast<size_t>(Piece::BlackPawn)]   |
        m_pieces[static_cast<size_t>(Piece::BlackKnight)] |
        m_pieces[static_cast<size_t>(Piece::BlackBishop)] |
        m_pieces[static_cast<size_t>(Piece::BlackRook)]   |
        m_pieces[static_cast<size_t>(Piece::BlackQueen)]  |
        m_pieces[static_cast<size_t>(Piece::BlackKing)];

    if (m_occupancy_color[static_cast<size_t>(Color::Black)] != expected_black) {
        return false;
    }

    // 4. White and Black occupancies must be disjoint
    if ((m_occupancy_color[static_cast<size_t>(Color::White)] &
         m_occupancy_color[static_cast<size_t>(Color::Black)]) != bb::EMPTY) {
        return false;
    }

    // 5. Total occupancy must equal White OR Black
    if (m_occupancy_all != (expected_white | expected_black)) {
        return false;
    }

    // 6. Pawns cannot be on Rank 1 or Rank 8
    if ((m_pieces[static_cast<size_t>(Piece::WhitePawn)] & (bb::RANK_1 | bb::RANK_8)) != bb::EMPTY) {
        return false;
    }
    if ((m_pieces[static_cast<size_t>(Piece::BlackPawn)] & (bb::RANK_1 | bb::RANK_8)) != bb::EMPTY) {
        return false;
    }

    // 7. En-passant square, if set, must be on Rank 3 or Rank 6
    if (m_en_passant_square != Square::None) {
        Rank ep_rank = square_rank(m_en_passant_square);
        if (ep_rank != Rank::Rank3 && ep_rank != Rank::Rank6) {
            return false;
        }
    }

    // 8. Individual square lookup must match bitboards
    for (int i = 0; i < NUM_SQUARES; ++i) {
        Square sq = static_cast<Square>(i);
        Piece p = piece_at(sq);
        if (p == Piece::None) {
            if (bb::test_bit(m_occupancy_all, sq)) return false;
        } else {
            if (!bb::test_bit(m_pieces[static_cast<size_t>(p)], sq)) return false;
        }
    }

    return true;
}

void Position::recalculate_hash() noexcept {
    m_zobrist_hash = zobrist::compute_hash(*this);
}

bool Position::operator==(const Position& other) const noexcept {
    return m_pieces == other.m_pieces &&
           m_occupancy_color == other.m_occupancy_color &&
           m_occupancy_all == other.m_occupancy_all &&
           m_side_to_move == other.m_side_to_move &&
           m_castling_rights == other.m_castling_rights &&
           m_en_passant_square == other.m_en_passant_square &&
           m_halfmove_clock == other.m_halfmove_clock &&
           m_fullmove_number == other.m_fullmove_number &&
           m_zobrist_hash == other.m_zobrist_hash;
}

void Position::make_move(Move m, UndoState& undo) noexcept {
    undo.move = m;
    undo.castling_rights = m_castling_rights;
    undo.en_passant_square = m_en_passant_square;
    undo.halfmove_clock = m_halfmove_clock;
    undo.zobrist_hash = m_zobrist_hash;

    Square from = m.from();
    Square to = m.to();
    Piece moving_piece = piece_at(from);
    PieceType moving_type = type_of(moving_piece);

    if (m.is_en_passant()) {
        undo.captured_piece = make_piece(~m_side_to_move, PieceType::Pawn);
    } else if (m.is_capture()) {
        undo.captured_piece = piece_at(to);
    } else {
        undo.captured_piece = Piece::None;
    }

    m_zobrist_hash ^= zobrist::castling_key(m_castling_rights);
    if (m_en_passant_square != Square::None) {
        m_zobrist_hash ^= zobrist::en_passant_key(m_en_passant_square);
    }

    if (moving_type == PieceType::Pawn || m.is_capture()) {
        m_halfmove_clock = 0;
    } else {
        m_halfmove_clock++;
    }

    if (m_side_to_move == Color::Black) {
        m_fullmove_number++;
    }

    m_en_passant_square = Square::None;
    if (m.is_double_push()) {
        m_en_passant_square = attacks::ep_target_square(from, m_side_to_move);
        m_zobrist_hash ^= zobrist::en_passant_key(m_en_passant_square);
    }

    update_castling_rights(from, to);
    m_zobrist_hash ^= zobrist::castling_key(m_castling_rights);

    if (m.is_en_passant()) {
        Square ep_cap_sq = attacks::pawn_ep_captured_square(to, m_side_to_move);
        remove_piece(ep_cap_sq);
        m_zobrist_hash ^= zobrist::piece_key(undo.captured_piece, ep_cap_sq);

        move_piece(from, to);
        m_zobrist_hash ^= zobrist::piece_key(moving_piece, from);
        m_zobrist_hash ^= zobrist::piece_key(moving_piece, to);
    } else if (m.is_castling()) {
        move_piece(from, to);
        m_zobrist_hash ^= zobrist::piece_key(moving_piece, from);
        m_zobrist_hash ^= zobrist::piece_key(moving_piece, to);

        Square rook_from = castling_rook_from(to);
        Square rook_to = castling_rook_to(to);
        Piece rook = piece_at(rook_from);
        move_piece(rook_from, rook_to);
        m_zobrist_hash ^= zobrist::piece_key(rook, rook_from);
        m_zobrist_hash ^= zobrist::piece_key(rook, rook_to);
    } else {
        if (m.is_capture()) {
            remove_piece(to);
            m_zobrist_hash ^= zobrist::piece_key(undo.captured_piece, to);
        }
        move_piece(from, to);
        m_zobrist_hash ^= zobrist::piece_key(moving_piece, from);
        m_zobrist_hash ^= zobrist::piece_key(moving_piece, to);

        if (m.is_promotion()) {
            Piece promo_piece = make_piece(m_side_to_move, m.promotion_type());
            put_piece(promo_piece, to);
            m_zobrist_hash ^= zobrist::piece_key(moving_piece, to);
            m_zobrist_hash ^= zobrist::piece_key(promo_piece, to);
        }
    }

    m_side_to_move = ~m_side_to_move;
    m_zobrist_hash ^= zobrist::side_key();
}

void Position::unmake_move(const UndoState& undo) noexcept {
    m_side_to_move = ~m_side_to_move;

    if (m_side_to_move == Color::Black) {
        m_fullmove_number--;
    }

    m_halfmove_clock = undo.halfmove_clock;
    m_castling_rights = undo.castling_rights;
    m_en_passant_square = undo.en_passant_square;
    m_zobrist_hash = undo.zobrist_hash;

    const Move& m = undo.move;
    Square from = m.from();
    Square to = m.to();

    if (m.is_en_passant()) {
        move_piece(to, from);
        Square ep_cap_sq = attacks::pawn_ep_captured_square(to, m_side_to_move);
        put_piece(undo.captured_piece, ep_cap_sq);
    } else if (m.is_castling()) {
        move_piece(to, from);
        Square rook_from = castling_rook_from(to);
        Square rook_to = castling_rook_to(to);
        move_piece(rook_to, rook_from);
    } else if (m.is_promotion()) {
        remove_piece(to);
        put_piece(make_piece(m_side_to_move, PieceType::Pawn), from);
        if (undo.captured_piece != Piece::None) {
            put_piece(undo.captured_piece, to);
        }
    } else {
        move_piece(to, from);
        if (undo.captured_piece != Piece::None) {
            put_piece(undo.captured_piece, to);
        }
    }
}

bool Position::make_move(Move m) noexcept {
    UndoState undo;
    make_move(m, undo);
    if (is_in_check(*this, ~m_side_to_move)) {
        unmake_move(undo);
        return false;
    }
    m_history.push_back(undo);
    return true;
}

bool Position::unmake_move() noexcept {
    if (m_history.empty()) return false;
    UndoState undo = m_history.back();
    m_history.pop_back();
    unmake_move(undo);
    return true;
}

} // namespace chess
