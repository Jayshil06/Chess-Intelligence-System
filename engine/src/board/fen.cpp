#include "board/fen.h"
#include "move/movegen.h"
#include <sstream>
#include <vector>
#include <cctype>
#include <charconv>

namespace chess {
namespace fen {

namespace {

std::vector<std::string_view> split_tokens(std::string_view str) {
    std::vector<std::string_view> tokens;
    size_t start = 0;
    while (start < str.length()) {
        while (start < str.length() && std::isspace(static_cast<unsigned char>(str[start]))) {
            ++start;
        }
        if (start >= str.length()) break;
        size_t end = start;
        while (end < str.length() && !std::isspace(static_cast<unsigned char>(str[end]))) {
            ++end;
        }
        tokens.push_back(str.substr(start, end - start));
        start = end;
    }
    return tokens;
}

} // namespace

std::optional<Position> parse(std::string_view fen_str) {
    auto tokens = split_tokens(fen_str);
    if (tokens.size() < 4 || tokens.size() > 6) {
        return std::nullopt;
    }

    Position pos;
    pos.clear();

    std::string_view placement = tokens[0];
    int rank = 7;
    int file = 0;

    for (char c : placement) {
        if (c == '/') {
            if (file != 8) return std::nullopt;
            --rank;
            file = 0;
            if (rank < 0) return std::nullopt;
        } else if (c >= '1' && c <= '8') {
            int empty_count = c - '0';
            file += empty_count;
            if (file > 8) return std::nullopt;
        } else {
            auto piece_opt = char_to_piece(c);
            if (!piece_opt.has_value() || file >= 8 || rank < 0) {
                return std::nullopt;
            }
            Square sq = make_square(static_cast<File>(file), static_cast<Rank>(rank));
            pos.put_piece(piece_opt.value(), sq);
            ++file;
        }
    }

    if (rank != 0 || file != 8) {
        return std::nullopt;
    }

    std::string_view stm = tokens[1];
    if (stm == "w" || stm == "W") {
        pos.set_side_to_move(Color::White);
    } else if (stm == "b" || stm == "B") {
        pos.set_side_to_move(Color::Black);
    } else {
        return std::nullopt;
    }

    std::string_view castling = tokens[2];
    uint8_t cr = Castling::None;
    if (castling != "-") {
        for (char c : castling) {
            switch (c) {
                case 'K': cr |= Castling::WhiteOO;  break;
                case 'Q': cr |= Castling::WhiteOOO; break;
                case 'k': cr |= Castling::BlackOO;  break;
                case 'q': cr |= Castling::BlackOOO; break;
                default:  return std::nullopt;
            }
        }
    }
    pos.set_castling_rights(cr);

    std::string_view ep = tokens[3];
    if (ep == "-") {
        pos.set_en_passant_square(Square::None);
    } else {
        auto sq_opt = string_to_square(ep);
        if (!sq_opt.has_value()) {
            return std::nullopt;
        }
        Square ep_sq = sq_opt.value();
        Rank ep_rank = square_rank(ep_sq);
        // Must sit behind a pawn that just double-pushed, with both squares it crossed empty
        Color us = pos.side_to_move();
        Square origin = attacks::pawn_ep_captured_square(ep_sq, ~us);
        if (ep_rank != (us == Color::White ? Rank::Rank6 : Rank::Rank3) ||
            pos.piece_at(attacks::pawn_ep_captured_square(ep_sq, us)) != make_piece(~us, PieceType::Pawn) ||
            pos.piece_at(ep_sq) != Piece::None || pos.piece_at(origin) != Piece::None) {
            return std::nullopt;
        }
        pos.set_en_passant_square(ep_sq);
    }

    if (tokens.size() >= 5) {
        int halfmove = 0;
        auto res = std::from_chars(tokens[4].data(), tokens[4].data() + tokens[4].size(), halfmove);
        if (res.ec != std::errc() || halfmove < 0 || halfmove > UINT16_MAX || res.ptr != tokens[4].data() + tokens[4].size()) {
            return std::nullopt;
        }
        pos.set_halfmove_clock(static_cast<uint16_t>(halfmove));
    } else {
        pos.set_halfmove_clock(0);
    }

    if (tokens.size() >= 6) {
        int fullmove = 0;
        auto res = std::from_chars(tokens[5].data(), tokens[5].data() + tokens[5].size(), fullmove);
        if (res.ec != std::errc() || fullmove <= 0 || fullmove > UINT16_MAX || res.ptr != tokens[5].data() + tokens[5].size()) {
            return std::nullopt;
        }
        pos.set_fullmove_number(static_cast<uint16_t>(fullmove));
    } else {
        pos.set_fullmove_number(1);
    }

    // One king each, and the side that just moved cannot be in check
    if (bb::popcount(pos.piece_bb(Piece::WhiteKing)) != 1 || bb::popcount(pos.piece_bb(Piece::BlackKing)) != 1 ||
        is_in_check(pos, ~pos.side_to_move())) {
        return std::nullopt;
    }

    // Drop castling rights whose king or rook is not on its home square
    if (pos.piece_at(Square::E1) != Piece::WhiteKing) cr &= ~Castling::WhiteAll;
    if (pos.piece_at(Square::H1) != Piece::WhiteRook) cr &= ~Castling::WhiteOO;
    if (pos.piece_at(Square::A1) != Piece::WhiteRook) cr &= ~Castling::WhiteOOO;
    if (pos.piece_at(Square::E8) != Piece::BlackKing) cr &= ~Castling::BlackAll;
    if (pos.piece_at(Square::H8) != Piece::BlackRook) cr &= ~Castling::BlackOO;
    if (pos.piece_at(Square::A8) != Piece::BlackRook) cr &= ~Castling::BlackOOO;
    pos.set_castling_rights(cr);

    if (!pos.validate_invariants()) {
        return std::nullopt;
    }

    pos.recalculate_hash();
    return pos;
}

std::string to_string(const Position& pos) {
    std::string fen;

    for (int r = 7; r >= 0; --r) {
        int empty_count = 0;
        for (int f = 0; f < 8; ++f) {
            Square sq = make_square(static_cast<File>(f), static_cast<Rank>(r));
            Piece p = pos.piece_at(sq);
            if (p == Piece::None) {
                ++empty_count;
            } else {
                if (empty_count > 0) {
                    fen += static_cast<char>('0' + empty_count);
                    empty_count = 0;
                }
                fen += piece_to_char(p);
            }
        }
        if (empty_count > 0) {
            fen += static_cast<char>('0' + empty_count);
        }
        if (r > 0) {
            fen += '/';
        }
    }

    fen += (pos.side_to_move() == Color::White ? " w " : " b ");

    uint8_t cr = pos.castling_rights();
    if (cr == Castling::None) {
        fen += '-';
    } else {
        if (cr & Castling::WhiteOO)  fen += 'K';
        if (cr & Castling::WhiteOOO) fen += 'Q';
        if (cr & Castling::BlackOO)  fen += 'k';
        if (cr & Castling::BlackOOO) fen += 'q';
    }
    fen += ' ';

    Square ep = pos.en_passant_square();
    if (ep == Square::None) {
        fen += '-';
    } else {
        fen += square_to_string(ep);
    }

    fen += ' ';
    fen += std::to_string(pos.halfmove_clock());
    fen += ' ';
    fen += std::to_string(pos.fullmove_number());

    return fen;
}

} // namespace fen
} // namespace chess
