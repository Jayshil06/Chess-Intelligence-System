#include "board/bitboard.h"
#include <sstream>
#include <iomanip>

namespace chess {
namespace bb {

std::string to_string(Bitboard b) {
    std::ostringstream ss;
    ss << "+---+---+---+---+---+---+---+---+\n";
    for (int r = 7; r >= 0; --r) {
        for (int f = 0; f < 8; ++f) {
            Square sq = make_square(static_cast<File>(f), static_cast<Rank>(r));
            ss << "| " << (test_bit(b, sq) ? "1 " : ". ");
        }
        ss << "| " << (r + 1) << "\n";
        ss << "+---+---+---+---+---+---+---+---+\n";
    }
    ss << "  a   b   c   d   e   f   g   h\n";
    ss << "Hex: 0x" << std::hex << std::setw(16) << std::setfill('0') << b << std::dec << "\n";
    return ss.str();
}

} // namespace bb
} // namespace chess
