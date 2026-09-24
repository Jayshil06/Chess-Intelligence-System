#pragma once

#include "board/position.h"
#include "move/move.h"
#include "search/tt.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <vector>
#include <chrono>
#include <functional>

namespace chess {
namespace search {

constexpr int INFINITY_SCORE = 30000;
constexpr int MATE_SCORE     = 29000;
constexpr int MAX_PLY        = 128;

constexpr bool is_mate_score(int score) noexcept {
    return score >= MATE_SCORE - MAX_PLY || score <= -(MATE_SCORE - MAX_PLY);
}

struct SearchLimits {
    int max_depth{64};
    uint64_t max_nodes{0};
    uint64_t max_time_ms{0};
    const std::atomic<bool>* stop{nullptr};  // Optional external stop signal (UCI "stop")
};

struct SearchResult {
    Move best_move{Move::null()};
    int score{0};
    int depth{0};
    uint64_t nodes{0};
    uint64_t time_ms{0};
    std::vector<Move> pv{};
};

using InfoCallback = std::function<void(const SearchResult&)>;

class Searcher {
public:
    explicit Searcher(size_t tt_size_mb = 16) : m_tt(tt_size_mb) {}

    int negamax(Position& pos, int depth, int ply = 0) noexcept;
    int alpha_beta(Position& pos, int alpha, int beta, int depth, int ply = 0) noexcept;
    int quiescence(Position& pos, int alpha, int beta) noexcept;
    int alpha_beta_with_q(Position& pos, int alpha, int beta, int depth, int ply = 0) noexcept;

    SearchResult search(Position& pos, const SearchLimits& limits = {}) noexcept;

    [[nodiscard]] uint64_t nodes() const noexcept { return m_nodes; }
    void reset_nodes() noexcept { m_nodes = 0; }

    void set_info_callback(InfoCallback cb) { m_on_iteration = std::move(cb); }
    [[nodiscard]] TranspositionTable& tt() noexcept { return m_tt; }
    void clear() noexcept;  // Forget TT and heuristics (new game)

private:
    uint64_t m_nodes{0};
    std::chrono::steady_clock::time_point m_start_time{};
    uint64_t m_max_time_ms{0};
    uint64_t m_max_nodes{0};
    const std::atomic<bool>* m_external_stop{nullptr};
    bool m_stop{false};

    TranspositionTable m_tt;
    std::array<std::array<Move, 2>, MAX_PLY> m_killers{};
    std::array<std::array<std::array<int, NUM_SQUARES>, NUM_SQUARES>, NUM_COLORS> m_history{};
    std::array<std::array<Move, MAX_PLY>, MAX_PLY> m_pv{};
    std::array<int, MAX_PLY> m_pv_len{};
    std::vector<uint64_t> m_hashes{};  // Hashes of positions preceding the current one
    size_t m_root_index{0};            // Entries before this index come from game history
    InfoCallback m_on_iteration{};

    [[nodiscard]] bool should_stop() noexcept;
    [[nodiscard]] uint64_t elapsed_ms() const noexcept;
    [[nodiscard]] bool is_repetition(const Position& pos) const noexcept;
    void score_moves(const Position& pos, MoveList& moves, Move tt_move, int ply) const noexcept;
};

} // namespace search
} // namespace chess
