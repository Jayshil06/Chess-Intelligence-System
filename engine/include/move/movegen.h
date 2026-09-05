#pragma once

#include "board/types.h"
#include "board/position.h"
#include "move/move.h"
#include "move/attacks.h"

namespace chess {

enum class MoveGenType : uint8_t {
    All = 0,
    Captures = 1,
    Quiets = 2
};

constexpr Square castling_rook_from(Square king_to) noexcept {
    switch (king_to) {
        case Square::G1: return Square::H1;
        case Square::C1: return Square::A1;
        case Square::G8: return Square::H8;
        case Square::C8: return Square::A8;
        default: return Square::None;
    }
}

constexpr Square castling_rook_to(Square king_to) noexcept {
    switch (king_to) {
        case Square::G1: return Square::F1;
        case Square::C1: return Square::D1;
        case Square::G8: return Square::F8;
        case Square::C8: return Square::D8;
        default: return Square::None;
    }
}

constexpr Square calculate_en_passant_square(Square from, Square to, PieceType pt) noexcept {
    if (pt != PieceType::Pawn) return Square::None;
    int diff = static_cast<int>(to) - static_cast<int>(from);
    if (diff == 16) {
        return static_cast<Square>(static_cast<uint8_t>(from) + 8);
    }
    if (diff == -16) {
        return static_cast<Square>(static_cast<uint8_t>(from) - 8);
    }
    return Square::None;
}

bool is_castling_path_clear(const Position& pos, MoveFlag flag) noexcept;

Bitboard attacks_to_square(const Position& pos, Square sq, Color attacker_color) noexcept;
bool is_square_attacked(const Position& pos, Square sq, Color attacker_color) noexcept;
bool is_in_check(const Position& pos, Color c) noexcept;
bool is_castling_legal(const Position& pos, MoveFlag flag) noexcept;
bool is_legal_move(const Position& pos, Move m) noexcept;

void generate_pawn_moves(const Position& pos, MoveList& list, MoveGenType type = MoveGenType::All);
void generate_knight_moves(const Position& pos, MoveList& list, MoveGenType type = MoveGenType::All);
void generate_bishop_moves(const Position& pos, MoveList& list, MoveGenType type = MoveGenType::All);
void generate_rook_moves(const Position& pos, MoveList& list, MoveGenType type = MoveGenType::All);
void generate_queen_moves(const Position& pos, MoveList& list, MoveGenType type = MoveGenType::All);
void generate_king_moves(const Position& pos, MoveList& list, MoveGenType type = MoveGenType::All);
void generate_castling_moves(const Position& pos, MoveList& list);

void generate_pseudo_legal_moves(const Position& pos, MoveList& list, MoveGenType type = MoveGenType::All);

inline MoveList generate_pseudo_legal_moves(const Position& pos, MoveGenType type = MoveGenType::All) {
    MoveList list;
    generate_pseudo_legal_moves(pos, list, type);
    return list;
}

void generate_legal_moves(const Position& pos, MoveList& list, MoveGenType type = MoveGenType::All);

inline MoveList generate_legal_moves(const Position& pos, MoveGenType type = MoveGenType::All) {
    MoveList list;
    generate_legal_moves(pos, list, type);
    return list;
}

bool is_checkmate(const Position& pos) noexcept;
bool is_stalemate(const Position& pos) noexcept;

} // namespace chess
