#include "board/types.h"
#include <cctype>

namespace chess {

std::string square_to_string(Square sq) {
    if (!is_valid_square(sq)) {
        return "-";
    }
    File f = square_file(sq);
    Rank r = square_rank(sq);
    std::string s;
    s += static_cast<char>('a' + static_cast<uint8_t>(f));
    s += static_cast<char>('1' + static_cast<uint8_t>(r));
    return s;
}

std::optional<Square> string_to_square(std::string_view str) {
    if (str.length() != 2) {
        return std::nullopt;
    }
    char file_char = str[0];
    char rank_char = str[1];

    if (file_char >= 'A' && file_char <= 'H') {
        file_char = static_cast<char>(std::tolower(file_char));
    }

    if (file_char < 'a' || file_char > 'h' || rank_char < '1' || rank_char > '8') {
        return std::nullopt;
    }

    File f = static_cast<File>(file_char - 'a');
    Rank r = static_cast<Rank>(rank_char - '1');
    return make_square(f, r);
}

char piece_to_char(Piece piece) {
    switch (piece) {
        case Piece::WhitePawn:   return 'P';
        case Piece::WhiteKnight: return 'N';
        case Piece::WhiteBishop: return 'B';
        case Piece::WhiteRook:   return 'R';
        case Piece::WhiteQueen:  return 'Q';
        case Piece::WhiteKing:   return 'K';
        case Piece::BlackPawn:   return 'p';
        case Piece::BlackKnight: return 'n';
        case Piece::BlackBishop: return 'b';
        case Piece::BlackRook:   return 'r';
        case Piece::BlackQueen:  return 'q';
        case Piece::BlackKing:   return 'k';
        default:                 return '.';
    }
}

std::optional<Piece> char_to_piece(char c) {
    switch (c) {
        case 'P': return Piece::WhitePawn;
        case 'N': return Piece::WhiteKnight;
        case 'B': return Piece::WhiteBishop;
        case 'R': return Piece::WhiteRook;
        case 'Q': return Piece::WhiteQueen;
        case 'K': return Piece::WhiteKing;
        case 'p': return Piece::BlackPawn;
        case 'n': return Piece::BlackKnight;
        case 'b': return Piece::BlackBishop;
        case 'r': return Piece::BlackRook;
        case 'q': return Piece::BlackQueen;
        case 'k': return Piece::BlackKing;
        default:  return std::nullopt;
    }
}

char piece_type_to_char(PieceType type) {
    switch (type) {
        case PieceType::Pawn:   return 'p';
        case PieceType::Knight: return 'n';
        case PieceType::Bishop: return 'b';
        case PieceType::Rook:   return 'r';
        case PieceType::Queen:  return 'q';
        case PieceType::King:   return 'k';
        default:                return '.';
    }
}

std::optional<PieceType> char_to_piece_type(char c) {
    switch (std::tolower(static_cast<unsigned char>(c))) {
        case 'p': return PieceType::Pawn;
        case 'n': return PieceType::Knight;
        case 'b': return PieceType::Bishop;
        case 'r': return PieceType::Rook;
        case 'q': return PieceType::Queen;
        case 'k': return PieceType::King;
        default:  return std::nullopt;
    }
}

std::string color_to_string(Color c) {
    switch (c) {
        case Color::White: return "white";
        case Color::Black: return "black";
        default:           return "none";
    }
}

std::string piece_type_to_string(PieceType pt) {
    switch (pt) {
        case PieceType::Pawn:   return "pawn";
        case PieceType::Knight: return "knight";
        case PieceType::Bishop: return "bishop";
        case PieceType::Rook:   return "rook";
        case PieceType::Queen:  return "queen";
        case PieceType::King:   return "king";
        default:                return "none";
    }
}

} // namespace chess
