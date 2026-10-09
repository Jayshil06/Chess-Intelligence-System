#pragma once

#include "board/types.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace chess {

class Position;

namespace nnue {

constexpr int NUM_FEATURES = 768;
constexpr int MAX_HIDDEN = 256;
constexpr int MAX_L1 = 64;
constexpr int QA = 255;         // Feature-transformer quantization scale
constexpr int WDL_SCALE = 400;  // Network output (win-probability logit) to centipawns

// File layout (little-endian): "CINN", u32 version, u32 hidden, u32 l1,
// i16 ft_weight[768][hidden], i16 ft_bias[hidden], f32 l1_weight[l1][2*hidden],
// f32 l1_bias[l1], f32 out_weight[l1], f32 out_bias
struct Network {
    int hidden{0};
    int l1{0};
    std::vector<int16_t> ft_weight;
    std::vector<int16_t> ft_bias;
    std::vector<float> l1_weight;
    std::vector<float> l1_bias;
    std::vector<float> out_weight;
    float out_bias{0.0f};
};

using Accumulator = std::array<int16_t, MAX_HIDDEN>;

namespace detail {
inline const Network* g_active = nullptr;
} // namespace detail

// An empty path unloads; a missing or malformed file returns false and keeps the current state
bool load(const std::string& path);
void unload() noexcept;
[[nodiscard]] inline const Network* active() noexcept { return detail::g_active; }

// Black's view mirrors the board and swaps colours, so both sides share one weight set
constexpr size_t feature_index(Piece p, Square sq, Color perspective) noexcept {
    size_t piece = static_cast<size_t>(p);
    size_t square = static_cast<size_t>(sq);
    if (perspective == Color::Black) {
        piece = (piece + 6) % 12;
        square ^= 56;
    }
    return piece * NUM_SQUARES + square;
}

// Side-to-move centipawns from the position's accumulators; requires an active network
int evaluate(const Position& pos) noexcept;

} // namespace nnue
} // namespace chess
