#include "search/search.h"
#include "evaluation/evaluate.h"
#include "move/movegen.h"
#include <algorithm>

namespace chess {
namespace search {

bool Searcher::should_stop() noexcept {
    if (m_stop) return true;
    if (m_max_time_ms > 0 && (m_nodes & 2047) == 0) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_start_time).count();
        if (static_cast<uint64_t>(elapsed) >= m_max_time_ms) {
            m_stop = true;
        }
    }
    return m_stop;
}

int Searcher::negamax(Position& pos, int depth, int ply) noexcept {
    m_nodes++;

    if (pos.halfmove_clock() >= 100) {
        return 0;
    }

    MoveList moves = generate_legal_moves(pos);
    if (moves.empty()) {
        if (is_in_check(pos, pos.side_to_move())) {
            return -MATE_SCORE + ply;
        }
        return 0;
    }

    if (depth <= 0) {
        return eval::evaluate(pos);
    }

    int best_score = -INFINITY_SCORE;
    for (const auto& m : moves) {
        UndoState undo;
        pos.make_move(m, undo);
        int score = -negamax(pos, depth - 1, ply + 1);
        pos.unmake_move(undo);

        if (score > best_score) {
            best_score = score;
        }
    }

    return best_score;
}

int Searcher::quiescence(Position& pos, int alpha, int beta) noexcept {
    m_nodes++;

    int stand_pat = eval::evaluate(pos);
    if (stand_pat >= beta) {
        return beta;
    }
    if (stand_pat > alpha) {
        alpha = stand_pat;
    }

    MoveList captures = generate_legal_moves(pos, MoveGenType::Captures);
    for (const auto& m : captures) {
        UndoState undo;
        pos.make_move(m, undo);
        int score = -quiescence(pos, -beta, -alpha);
        pos.unmake_move(undo);

        if (score >= beta) {
            return beta;
        }
        if (score > alpha) {
            alpha = score;
        }
    }

    return alpha;
}

int Searcher::alpha_beta(Position& pos, int alpha, int beta, int depth, int ply) noexcept {
    m_nodes++;

    if (should_stop()) {
        return 0;
    }

    if (pos.halfmove_clock() >= 100) {
        return 0;
    }

    MoveList moves = generate_legal_moves(pos);
    if (moves.empty()) {
        if (is_in_check(pos, pos.side_to_move())) {
            return -MATE_SCORE + ply;
        }
        return 0;
    }

    if (depth <= 0) {
        return eval::evaluate(pos);
    }

    for (const auto& m : moves) {
        UndoState undo;
        pos.make_move(m, undo);
        int score = -alpha_beta(pos, -beta, -alpha, depth - 1, ply + 1);
        pos.unmake_move(undo);

        if (should_stop()) {
            return 0;
        }

        if (score >= beta) {
            return beta;
        }
        if (score > alpha) {
            alpha = score;
        }
    }

    return alpha;
}

int Searcher::alpha_beta_with_q(Position& pos, int alpha, int beta, int depth, int ply) noexcept {
    m_nodes++;

    if (should_stop()) {
        return 0;
    }

    if (pos.halfmove_clock() >= 100) {
        return 0;
    }

    MoveList moves = generate_legal_moves(pos);
    if (moves.empty()) {
        if (is_in_check(pos, pos.side_to_move())) {
            return -MATE_SCORE + ply;
        }
        return 0;
    }

    if (depth <= 0) {
        return quiescence(pos, alpha, beta);
    }

    for (const auto& m : moves) {
        UndoState undo;
        pos.make_move(m, undo);
        int score = -alpha_beta_with_q(pos, -beta, -alpha, depth - 1, ply + 1);
        pos.unmake_move(undo);

        if (should_stop()) {
            return 0;
        }

        if (score >= beta) {
            return beta;
        }
        if (score > alpha) {
            alpha = score;
        }
    }

    return alpha;
}

SearchResult Searcher::search(Position& pos, const SearchLimits& limits) noexcept {
    SearchResult result{};
    m_nodes = 0;
    m_stop = false;
    m_max_time_ms = limits.max_time_ms;
    m_start_time = std::chrono::steady_clock::now();

    MoveList root_moves = generate_legal_moves(pos);
    if (root_moves.empty()) {
        if (is_in_check(pos, pos.side_to_move())) {
            result.score = -MATE_SCORE;
        } else {
            result.score = 0;
        }
        return result;
    }

    result.best_move = root_moves[0];
    int max_d = std::min(limits.max_depth, 64);

    for (int d = 1; d <= max_d; ++d) {
        int best_score = -INFINITY_SCORE;
        Move best_move_this_iteration = Move::null();

        for (const auto& m : root_moves) {
            UndoState undo;
            pos.make_move(m, undo);
            int score = -alpha_beta_with_q(pos, -INFINITY_SCORE, -best_score, d - 1, 1);
            pos.unmake_move(undo);

            if (should_stop()) break;

            if (score > best_score) {
                best_score = score;
                best_move_this_iteration = m;
            }
        }

        if (should_stop()) break;

        result.depth = d;
        result.score = best_score;
        if (!best_move_this_iteration.is_null()) {
            result.best_move = best_move_this_iteration;
        }

        if (best_score >= MATE_SCORE - 100) {
            break;
        }
    }

    auto end_time = std::chrono::steady_clock::now();
    result.nodes = m_nodes;
    result.time_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(end_time - m_start_time).count()
    );

    return result;
}

} // namespace search
} // namespace chess
