#pragma once

#include "board/types.h"
#include "board/bitboard.h"
#include "move/move.h"
#include "evaluation/psqt.h"
#include <array>
#include <vector>
#include <string>
#include <optional>
#include <cstdint>

namespace chess {

struct UndoState {
    Move move{Move::null()};
    Piece captured_piece{Piece::None};
    uint8_t castling_rights{0};
    Square en_passant_square{Square::None};
    uint16_t halfmove_clock{0};
    uint64_t zobrist_hash{0};
};

namespace Castling {
    constexpr uint8_t None = 0;
    constexpr uint8_t WhiteOO  = 1 << 0;
    constexpr uint8_t WhiteOOO = 1 << 1;
    constexpr uint8_t BlackOO  = 1 << 2;
    constexpr uint8_t BlackOOO = 1 << 3;

    constexpr uint8_t WhiteAll = WhiteOO | WhiteOOO;
    constexpr uint8_t BlackAll = BlackOO | BlackOOO;
    constexpr uint8_t All = WhiteAll | BlackAll;

    inline constexpr auto CASTLING_SQUARE_MASKS = []() consteval {
        std::array<uint8_t, NUM_SQUARES> masks{};
        masks.fill(All);
        masks[static_cast<size_t>(Square::E1)] = ~WhiteAll & All;
        masks[static_cast<size_t>(Square::H1)] = ~WhiteOO & All;
        masks[static_cast<size_t>(Square::A1)] = ~WhiteOOO & All;
        masks[static_cast<size_t>(Square::E8)] = ~BlackAll & All;
        masks[static_cast<size_t>(Square::H8)] = ~BlackOO & All;
        masks[static_cast<size_t>(Square::A8)] = ~BlackOOO & All;
        return masks;
    }();

    constexpr uint8_t update_mask(Square from, Square to) noexcept {
        uint8_t m = All;
        if (is_valid_square(from)) m &= CASTLING_SQUARE_MASKS[static_cast<size_t>(from)];
        if (is_valid_square(to))   m &= CASTLING_SQUARE_MASKS[static_cast<size_t>(to)];
        return m;
    }
}

class Position {
public:
    Position();
    explicit Position(bool set_startpos);

    void clear();
    void reset_to_starting_position();

    [[nodiscard]] Bitboard piece_bb(Piece p) const noexcept;
    [[nodiscard]] Bitboard piece_bb(Color c, PieceType pt) const noexcept;
    [[nodiscard]] Bitboard color_occupancy(Color c) const noexcept;
    [[nodiscard]] Bitboard all_occupancy() const noexcept;
    [[nodiscard]] Bitboard empty_squares() const noexcept;

    [[nodiscard]] Color side_to_move() const noexcept { return m_side_to_move; }
    [[nodiscard]] uint8_t castling_rights() const noexcept { return m_castling_rights; }
    [[nodiscard]] Square en_passant_square() const noexcept { return m_en_passant_square; }
    [[nodiscard]] uint16_t halfmove_clock() const noexcept { return m_halfmove_clock; }
    [[nodiscard]] uint16_t fullmove_number() const noexcept { return m_fullmove_number; }
    [[nodiscard]] uint64_t hash() const noexcept { return m_zobrist_hash; }

    // Material + PST totals from White's view, updated on every piece change
    [[nodiscard]] int psq_mg() const noexcept { return m_psq_mg; }
    [[nodiscard]] int psq_eg() const noexcept { return m_psq_eg; }
    [[nodiscard]] int phase() const noexcept { return m_phase; }

    void set_side_to_move(Color c) noexcept { m_side_to_move = c; }
    void set_castling_rights(uint8_t cr) noexcept { m_castling_rights = cr; }
    void set_en_passant_square(Square sq) noexcept { m_en_passant_square = sq; }
    void set_halfmove_clock(uint16_t h) noexcept { m_halfmove_clock = h; }
    void set_fullmove_number(uint16_t f) noexcept { m_fullmove_number = f; }
    void recalculate_hash() noexcept;

    [[nodiscard]] bool can_castle(uint8_t right) const noexcept { return (m_castling_rights & right) == right; }
    [[nodiscard]] bool has_castling_rights(Color c) const noexcept {
        return (m_castling_rights & (c == Color::White ? Castling::WhiteAll : Castling::BlackAll)) != 0;
    }
    void update_castling_rights(Square from, Square to) noexcept {
        m_castling_rights &= Castling::update_mask(from, to);
    }
    void clear_en_passant() noexcept { m_en_passant_square = Square::None; }

    [[nodiscard]] Piece piece_at(Square sq) const noexcept;
    [[nodiscard]] Color color_at(Square sq) const noexcept;
    [[nodiscard]] PieceType type_at(Square sq) const noexcept;

    void put_piece(Piece p, Square sq) noexcept;
    void remove_piece(Square sq) noexcept;
    void move_piece(Square from, Square to) noexcept;

    void make_move(Move m, UndoState& undo) noexcept;
    void unmake_move(const UndoState& undo) noexcept;
    bool make_move(Move m) noexcept;  // Legality-checked, recorded in history()
    bool unmake_move() noexcept;
    [[nodiscard]] const std::vector<UndoState>& history() const noexcept { return m_history; }

    [[nodiscard]] bool validate_invariants() const noexcept;

    bool operator==(const Position& other) const noexcept;

private:
    void update_occupancies() noexcept;
    void update_psq(Piece p, Square sq, int sign) noexcept {
        const eval::Score& s = eval::PSQT[static_cast<size_t>(p)][static_cast<size_t>(sq)];
        m_psq_mg += sign * s.mg;
        m_psq_eg += sign * s.eg;
        m_phase += sign * eval::phase_weight(p);
    }

    std::array<Bitboard, NUM_PIECES> m_pieces{};
    std::array<Piece, NUM_SQUARES> m_board{};
    std::array<Bitboard, NUM_COLORS> m_occupancy_color{};
    Bitboard m_occupancy_all{bb::EMPTY};

    Color m_side_to_move{Color::White};
    uint8_t m_castling_rights{Castling::None};
    Square m_en_passant_square{Square::None};
    uint16_t m_halfmove_clock{0};
    uint16_t m_fullmove_number{1};
    uint64_t m_zobrist_hash{0};

    int m_psq_mg{0};
    int m_psq_eg{0};
    int m_phase{0};

    std::vector<UndoState> m_history{};
};

} // namespace chess
