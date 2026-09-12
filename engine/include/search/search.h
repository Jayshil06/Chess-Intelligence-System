#pragma once

#include "board/position.h"
#include "move/move.h"
#include <cstdint>
#include <vector>
#include <chrono>

namespace chess {
namespace search {

constexpr int INFINITY_SCORE = 30000;
constexpr int MATE_SCORE     = 29000;

struct SearchLimits {
    int max_depth{64};
    uint64_t max_nodes{0};
    uint64_t max_time_ms{0};
};

struct SearchResult {
    Move best_move{Move::null()};
    int score{0};
    int depth{0};
    uint64_t nodes{0};
    uint64_t time_ms{0};
    std::vector<Move> pv{};
};

class Searcher {
public:
    Searcher() = default;

    int negamax(Position& pos, int depth, int ply = 0) noexcept;
    int alpha_beta(Position& pos, int alpha, int beta, int depth, int ply = 0) noexcept;
    int quiescence(Position& pos, int alpha, int beta) noexcept;
    int alpha_beta_with_q(Position& pos, int alpha, int beta, int depth, int ply = 0) noexcept;

    SearchResult search(Position& pos, const SearchLimits& limits = {}) noexcept;

    [[nodiscard]] uint64_t nodes() const noexcept { return m_nodes; }
    void reset_nodes() noexcept { m_nodes = 0; }

private:
    uint64_t m_nodes{0};
    std::chrono::steady_clock::time_point m_start_time{};
    uint64_t m_max_time_ms{0};
    bool m_stop{false};

    [[nodiscard]] bool should_stop() noexcept;
};

} // namespace search
} // namespace chess
