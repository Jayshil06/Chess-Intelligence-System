#include "move/move.h"
#include <cctype>

namespace chess {

std::string Move::to_uci() const {
    if (is_null()) return "0000";
    std::string str = square_to_string(from()) + square_to_string(to());
    if (is_promotion()) {
        str += piece_type_to_char(promotion_type());
    }
    return str;
}

std::string Move::to_string() const {
    return to_uci();
}

std::optional<Move> Move::from_uci(std::string_view uci) {
    if (uci == "0000" || uci == "none") return Move::null();
    if (uci.length() != 4 && uci.length() != 5) return std::nullopt;

    auto from_sq = string_to_square(uci.substr(0, 2));
    auto to_sq = string_to_square(uci.substr(2, 2));
    if (!from_sq || !to_sq) return std::nullopt;

    if (uci.length() == 5) {
        char p = static_cast<char>(std::tolower(uci[4]));
        PieceType promo = PieceType::None;
        switch (p) {
            case 'q': promo = PieceType::Queen; break;
            case 'r': promo = PieceType::Rook; break;
            case 'b': promo = PieceType::Bishop; break;
            case 'n': promo = PieceType::Knight; break;
            default: return std::nullopt;
        }
        return make_promotion(*from_sq, *to_sq, promo);
    }

    return Move(*from_sq, *to_sq);
}

} // namespace chess
