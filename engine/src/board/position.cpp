#include "board/position.h"
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

bool Position::operator==(const Position& other) const noexcept {
    return m_pieces == other.m_pieces &&
           m_occupancy_color == other.m_occupancy_color &&
           m_occupancy_all == other.m_occupancy_all &&
           m_side_to_move == other.m_side_to_move &&
           m_castling_rights == other.m_castling_rights &&
           m_en_passant_square == other.m_en_passant_square &&
           m_halfmove_clock == other.m_halfmove_clock &&
           m_fullmove_number == other.m_fullmove_number;
}

} // namespace chess
