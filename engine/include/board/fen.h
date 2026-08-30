#pragma once

#include "board/position.h"
#include <string>
#include <string_view>
#include <optional>

namespace chess {
namespace fen {

// Standard Chess Starting Position FEN
constexpr std::string_view START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

// Parse a FEN string into a Position object.
// Returns std::nullopt if the FEN string is malformed or invalid.
std::optional<Position> parse(std::string_view fen_str);

// Serialize a Position object back to standard FEN representation.
std::string to_string(const Position& pos);

} // namespace fen
} // namespace chess
