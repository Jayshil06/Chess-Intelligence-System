#include "search/search.h"
#include "evaluation/evaluate.h"
#include "move/movegen.h"
#include <algorithm>

namespace chess {
namespace search {

namespace {

constexpr int TT_MOVE_SCORE = 30000;
constexpr int CAPTURE_SCORE = 20000;
constexpr int KILLER_SCORE  = 15000;
constexpr int HISTORY_MAX   = 14000;

// Mate scores are stored relative to the node so they stay valid at any ply
int score_to_tt(int score, int ply) noexcept {
    if (score >= MATE_SCORE - MAX_PLY) return score + ply;
    if (score <= -(MATE_SCORE - MAX_PLY)) return score - ply;
    return score;
}

int score_from_tt(int score, int ply) noexcept {
    if (score >= MATE_SCORE - MAX_PLY) return score - ply;
    if (score <= -(MATE_SCORE - MAX_PLY)) return score + ply;
    return score;
}

bool is_insufficient_material(const Position& pos) noexcept {
    Bitboard heavy = pos.piece_bb(Piece::WhitePawn) | pos.piece_bb(Piece::BlackPawn) |
                     pos.piece_bb(Piece::WhiteRook) | pos.piece_bb(Piece::BlackRook) |
                     pos.piece_bb(Piece::WhiteQueen) | pos.piece_bb(Piece::BlackQueen);
    if (heavy) return false;
    Bitboard minors = pos.piece_bb(Piece::WhiteKnight) | pos.piece_bb(Piece::BlackKnight) |
                      pos.piece_bb(Piece::WhiteBishop) | pos.piece_bb(Piece::BlackBishop);
    return bb::popcount(minors) <= 1;
}

// Selection sort step: bring the highest-scored remaining move to index i
Move pick_next(MoveList& moves, size_t i) noexcept {
    size_t best = i;
    for (size_t j = i + 1; j < moves.size(); ++j) {
        if (moves[j].score() > moves[best].score()) best = j;
    }
    std::swap(moves[i], moves[best]);
    return moves[i];
}

} // namespace

void Searcher::clear() noexcept {
    m_tt.clear();
    m_killers = {};
    m_history = {};
}

uint64_t Searcher::elapsed_ms() const noexcept {
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - m_start_time).count());
}

bool Searcher::should_stop() noexcept {
    if (m_stop) return true;
    if ((m_external_stop && m_external_stop->load(std::memory_order_relaxed)) ||
        (m_max_nodes > 0 && m_nodes >= m_max_nodes) ||
        (m_max_time_ms > 0 && (m_nodes & 2047) == 0 && elapsed_ms() >= m_max_time_ms)) {
        m_stop = true;
    }
    return m_stop;
}

bool Searcher::is_repetition(const Position& pos) const noexcept {
    int n = static_cast<int>(m_hashes.size());
    int oldest = std::max(0, n - static_cast<int>(pos.halfmove_clock()));
    // A repeat inside the search tree is scored as a draw; game history needs threefold
    int game_matches = 0;
    for (int i = n - 2; i >= oldest; i -= 2) {
        if (m_hashes[static_cast<size_t>(i)] == pos.hash() &&
            (static_cast<size_t>(i) >= m_root_index || ++game_matches == 2)) {
            return true;
        }
    }
    return false;
}

void Searcher::score_moves(const Position& pos, MoveList& moves, Move tt_move, int ply) const noexcept {
    const auto& history = m_history[static_cast<size_t>(pos.side_to_move())];
    const auto& killers = m_killers[static_cast<size_t>(ply)];
    for (Move& m : moves) {
        int score;
        if (m == tt_move) {
            score = TT_MOVE_SCORE;
        } else if (m.is_capture() || m.promotion_type() == PieceType::Queen) {
            // MVV-LVA: most valuable victim first, cheapest attacker breaks ties
            score = CAPTURE_SCORE;
            if (m.is_capture()) {
                int victim = m.is_en_passant() ? 0 : static_cast<int>(pos.type_at(m.to()));
                score += victim * 100 - static_cast<int>(pos.type_at(m.from()));
            }
            if (m.promotion_type() == PieceType::Queen) score += 800;
        } else if (m == killers[0]) {
            score = KILLER_SCORE;
        } else if (m == killers[1]) {
            score = KILLER_SCORE - 1;
        } else {
            score = history[static_cast<size_t>(m.from())][static_cast<size_t>(m.to())];
        }
        m.set_score(static_cast<int16_t>(score));
    }
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

int Searcher::quiescence(Position& pos, int alpha, int beta) noexcept {
    m_nodes++;
    if (should_stop()) return 0;

    int stand_pat = eval::evaluate(pos);
    if (stand_pat >= beta) return stand_pat;
    alpha = std::max(alpha, stand_pat);

    MoveList captures = generate_legal_moves(pos, MoveGenType::Captures);
    score_moves(pos, captures, Move::null(), 0);

    int best = stand_pat;
    for (size_t i = 0; i < captures.size(); ++i) {
        Move m = pick_next(captures, i);
        UndoState undo;
        pos.make_move(m, undo);
        int score = -quiescence(pos, -beta, -alpha);
        pos.unmake_move(undo);

        if (m_stop) return 0;
        if (score > best) {
            best = score;
            if (score >= beta) return score;
            alpha = std::max(alpha, score);
        }
    }
    return best;
}

int Searcher::alpha_beta_with_q(Position& pos, int alpha, int beta, int depth, int ply) noexcept {
    m_pv_len[static_cast<size_t>(ply)] = ply;
    m_nodes++;
    if (should_stop()) return 0;

    const bool root = ply == 0;
    if (!root && (is_repetition(pos) || is_insufficient_material(pos))) return eval::SCORE_DRAW;
    if (ply >= MAX_PLY - 1) return eval::evaluate(pos);

    const Color us = pos.side_to_move();
    const bool in_check = is_in_check(pos, us);
    if (in_check) ++depth;  // Check extension
    if (depth <= 0) return quiescence(pos, alpha, beta);

    const uint64_t key = pos.hash();
    Move tt_move = Move::null();
    if (const TTEntry* e = m_tt.probe(key)) {
        tt_move = Move(e->move);
        int s = score_from_tt(e->score, ply);
        if (!root && e->depth >= depth &&
            (e->bound == Bound::Exact || (e->bound == Bound::Lower && s >= beta) ||
             (e->bound == Bound::Upper && s <= alpha))) {
            return s;
        }
    }

    MoveList moves = generate_legal_moves(pos);
    if (moves.empty()) return in_check ? -MATE_SCORE + ply : eval::SCORE_DRAW;
    if (pos.halfmove_clock() >= 100) return eval::SCORE_DRAW;

    score_moves(pos, moves, tt_move, ply);
    const int alpha_orig = alpha;
    int best = -INFINITY_SCORE;
    Move best_move = Move::null();
    m_hashes.push_back(key);

    for (size_t i = 0; i < moves.size(); ++i) {
        Move m = pick_next(moves, i);
        UndoState undo;
        pos.make_move(m, undo);
        int score;
        if (i == 0) {
            score = -alpha_beta_with_q(pos, -beta, -alpha, depth - 1, ply + 1);
        } else {
            // Principal variation search: null window first, re-search on fail-high
            score = -alpha_beta_with_q(pos, -alpha - 1, -alpha, depth - 1, ply + 1);
            if (score > alpha && score < beta) {
                score = -alpha_beta_with_q(pos, -beta, -alpha, depth - 1, ply + 1);
            }
        }
        pos.unmake_move(undo);

        if (m_stop) {
            m_hashes.pop_back();
            return 0;
        }
        if (score <= best) continue;
        best = score;
        best_move = m;
        if (score <= alpha) continue;
        alpha = score;

        auto& pv = m_pv[static_cast<size_t>(ply)];
        const auto& child = m_pv[static_cast<size_t>(ply + 1)];
        int child_len = m_pv_len[static_cast<size_t>(ply + 1)];
        pv[static_cast<size_t>(ply)] = m;
        std::copy(child.begin() + ply + 1, child.begin() + child_len, pv.begin() + ply + 1);
        m_pv_len[static_cast<size_t>(ply)] = child_len;

        if (score >= beta) {
            if (!m.is_capture()) {
                auto& killers = m_killers[static_cast<size_t>(ply)];
                if (killers[0] != m) {
                    killers[1] = killers[0];
                    killers[0] = m;
                }
                int& h = m_history[static_cast<size_t>(us)][static_cast<size_t>(m.from())][static_cast<size_t>(m.to())];
                h = std::min(h + depth * depth, HISTORY_MAX);
            }
            break;
        }
    }
    m_hashes.pop_back();

    Bound bound = best >= beta ? Bound::Lower : (best > alpha_orig ? Bound::Exact : Bound::Upper);
    m_tt.store(key, depth, score_to_tt(best, ply), bound, best_move);
    return best;
}

SearchResult Searcher::search(Position& pos, const SearchLimits& limits) noexcept {
    SearchResult result{};
    m_nodes = 0;
    m_stop = false;
    m_max_time_ms = limits.max_time_ms;
    m_max_nodes = limits.max_nodes;
    m_external_stop = limits.stop;
    m_start_time = std::chrono::steady_clock::now();
    m_killers = {};
    m_history = {};

    m_hashes.clear();
    m_hashes.reserve(pos.history().size() + MAX_PLY);
    for (const auto& undo : pos.history()) m_hashes.push_back(undo.zobrist_hash);
    m_root_index = m_hashes.size();

    MoveList root_moves = generate_legal_moves(pos);
    if (root_moves.empty()) {
        result.score = is_in_check(pos, pos.side_to_move()) ? -MATE_SCORE : eval::SCORE_DRAW;
        return result;
    }
    result.best_move = root_moves[0];

    int max_depth = std::clamp(limits.max_depth, 1, MAX_PLY - 1);
    for (int d = 1; d <= max_depth; ++d) {
        int score = alpha_beta_with_q(pos, -INFINITY_SCORE, INFINITY_SCORE, d, 0);
        if (m_stop) break;  // Incomplete iteration: keep the previous result

        result.depth = d;
        result.score = score;
        result.pv.assign(m_pv[0].begin(), m_pv[0].begin() + m_pv_len[0]);
        if (!result.pv.empty()) result.best_move = result.pv[0];
        result.nodes = m_nodes;
        result.time_ms = elapsed_ms();
        if (m_on_iteration) m_on_iteration(result);

        if (score >= MATE_SCORE - MAX_PLY) break;
        // Another iteration would likely not finish within the time budget
        if (m_max_time_ms > 0 && result.time_ms * 2 >= m_max_time_ms) break;
    }

    result.nodes = m_nodes;
    result.time_ms = elapsed_ms();
    return result;
}

} // namespace search
} // namespace chess
