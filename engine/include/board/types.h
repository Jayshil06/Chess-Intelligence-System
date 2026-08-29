#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <optional>
#include <algorithm>
#include <cmath>

namespace chess {

// Core Constants
constexpr int NUM_SQUARES = 64;
constexpr int NUM_COLORS = 2;
constexpr int NUM_PIECE_TYPES = 6;
constexpr int NUM_PIECES = 12;
constexpr int NUM_FILES = 8;
constexpr int NUM_RANKS = 8;

// Color representation
enum class Color : uint8_t {
    White = 0,
    Black = 1,
    None = 2
};

// Piece Type representation
enum class PieceType : uint8_t {
    Pawn = 0,
    Knight = 1,
    Bishop = 2,
    Rook = 3,
    Queen = 4,
    King = 5,
    None = 6
};

// Full Piece representation (combining Color and PieceType)
enum class Piece : uint8_t {
    WhitePawn = 0,
    WhiteKnight = 1,
    WhiteBishop = 2,
    WhiteRook = 3,
    WhiteQueen = 4,
    WhiteKing = 5,
    BlackPawn = 6,
    BlackKnight = 7,
    BlackBishop = 8,
    BlackRook = 9,
    BlackQueen = 10,
    BlackKing = 11,
    None = 12
};

// Board File (columns a-h, 0-7)
enum class File : uint8_t {
    FileA = 0,
    FileB = 1,
    FileC = 2,
    FileD = 3,
    FileE = 4,
    FileF = 5,
    FileG = 6,
    FileH = 7,
    None = 8
};

// Board Rank (rows 1-8, 0-7)
enum class Rank : uint8_t {
    Rank1 = 0,
    Rank2 = 1,
    Rank3 = 2,
    Rank4 = 3,
    Rank5 = 4,
    Rank6 = 5,
    Rank7 = 6,
    Rank8 = 7,
    None = 8
};

// Square mapping (0-63, Little-Endian Rank-File mapping)
// A1 = 0, B1 = 1, ..., H1 = 7, A2 = 8, ..., H8 = 63
enum class Square : uint8_t {
    A1 = 0,  B1 = 1,  C1 = 2,  D1 = 3,  E1 = 4,  F1 = 5,  G1 = 6,  H1 = 7,
    A2 = 8,  B2 = 9,  C2 = 10, D2 = 11, E2 = 12, F2 = 13, G2 = 14, H2 = 15,
    A3 = 16, B3 = 17, C3 = 18, D3 = 19, E3 = 20, F3 = 21, G3 = 22, H3 = 23,
    A4 = 24, B4 = 25, C4 = 26, D4 = 27, E4 = 28, F4 = 29, G4 = 30, H4 = 31,
    A5 = 32, B5 = 33, C5 = 34, D5 = 35, E5 = 36, F5 = 37, G5 = 38, H5 = 39,
    A6 = 40, B6 = 41, C6 = 42, D6 = 43, E6 = 44, F6 = 45, G6 = 46, H6 = 47,
    A7 = 48, B7 = 49, C7 = 50, D7 = 51, E7 = 52, F7 = 53, G7 = 54, H7 = 55,
    A8 = 56, B8 = 57, C8 = 58, D8 = 59, E8 = 60, F8 = 61, G8 = 62, H8 = 63,
    None = 64
};

// Compile-time / inline helper functions

constexpr bool is_valid_square(Square sq) {
    return static_cast<uint8_t>(sq) < NUM_SQUARES;
}

constexpr bool is_valid_file(File f) {
    return static_cast<uint8_t>(f) < NUM_FILES;
}

constexpr bool is_valid_rank(Rank r) {
    return static_cast<uint8_t>(r) < NUM_RANKS;
}

constexpr Square make_square(File file, Rank rank) {
    if (!is_valid_file(file) || !is_valid_rank(rank)) {
        return Square::None;
    }
    return static_cast<Square>(static_cast<uint8_t>(rank) * 8 + static_cast<uint8_t>(file));
}

constexpr File square_file(Square sq) {
    if (!is_valid_square(sq)) return File::None;
    return static_cast<File>(static_cast<uint8_t>(sq) & 7);
}

constexpr Rank square_rank(Square sq) {
    if (!is_valid_square(sq)) return Rank::None;
    return static_cast<Rank>(static_cast<uint8_t>(sq) >> 3);
}

constexpr Color flip_color(Color c) {
    if (c == Color::White) return Color::Black;
    if (c == Color::Black) return Color::White;
    return Color::None;
}

constexpr Piece make_piece(Color color, PieceType type) {
    if (color == Color::None || type == PieceType::None) {
        return Piece::None;
    }
    return static_cast<Piece>(static_cast<uint8_t>(color) * 6 + static_cast<uint8_t>(type));
}

constexpr Color color_of(Piece piece) {
    if (piece == Piece::None) return Color::None;
    return static_cast<Color>(static_cast<uint8_t>(piece) / 6);
}

constexpr PieceType type_of(Piece piece) {
    if (piece == Piece::None) return PieceType::None;
    return static_cast<PieceType>(static_cast<uint8_t>(piece) % 6);
}

constexpr Rank relative_rank(Color color, Rank rank) {
    if (!is_valid_rank(rank)) return Rank::None;
    if (color == Color::White) return rank;
    return static_cast<Rank>(7 - static_cast<uint8_t>(rank));
}

constexpr Square relative_square(Color color, Square sq) {
    if (!is_valid_square(sq)) return Square::None;
    if (color == Color::White) return sq;
    return static_cast<Square>(static_cast<uint8_t>(sq) ^ 56);
}

constexpr int square_index(Square sq) {
    return static_cast<int>(sq);
}

// Distance helpers
inline int chebyshev_distance(Square a, Square b) {
    int file_diff = std::abs(static_cast<int>(square_file(a)) - static_cast<int>(square_file(b)));
    int rank_diff = std::abs(static_cast<int>(square_rank(a)) - static_cast<int>(square_rank(b)));
    return std::max(file_diff, rank_diff);
}

inline int manhattan_distance(Square a, Square b) {
    int file_diff = std::abs(static_cast<int>(square_file(a)) - static_cast<int>(square_file(b)));
    int rank_diff = std::abs(static_cast<int>(square_rank(a)) - static_cast<int>(square_rank(b)));
    return file_diff + rank_diff;
}

// String and character conversions
std::string square_to_string(Square sq);
std::optional<Square> string_to_square(std::string_view str);

char piece_to_char(Piece piece);
std::optional<Piece> char_to_piece(char c);

char piece_type_to_char(PieceType type);
std::optional<PieceType> char_to_piece_type(char c);

std::string color_to_string(Color c);
std::string piece_type_to_string(PieceType pt);

} // namespace chess
