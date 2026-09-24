#pragma once

#include "board/types.h"
#include "board/position.h"
#include "evaluation/psqt.h"
#include <array>
#include <cstdint>

namespace chess {
namespace eval {

constexpr int SCORE_DRAW = 0;

int evaluate(const Position& pos) noexcept;
int evaluate_material(const Position& pos, Color c) noexcept;
int evaluate_pst(const Position& pos, Color c) noexcept;

} // namespace eval
} // namespace chess
