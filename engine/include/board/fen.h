#pragma once

#include "board/position.h"
#include <string>
#include <string_view>
#include <optional>

namespace chess {
namespace fen {

constexpr std::string_view START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

// Returns std::nullopt for malformed or illegal positions.
std::optional<Position> parse(std::string_view fen_str);

std::string to_string(const Position& pos);

} // namespace fen
} // namespace chess
