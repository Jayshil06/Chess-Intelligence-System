#pragma once

#include "board/position.h"
#include "move/move.h"
#include <cstdint>
#include <vector>
#include <utility>

namespace chess {

struct PerftResults {
    uint64_t nodes{0};
    std::vector<std::pair<Move, uint64_t>> divide{};
};

uint64_t perft(Position& pos, int depth) noexcept;
PerftResults perft_divide(Position& pos, int depth) noexcept;

} // namespace chess
