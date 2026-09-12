#include "move/perft.h"
#include "move/movegen.h"

namespace chess {

uint64_t perft(Position& pos, int depth) noexcept {
    if (depth <= 0) return 1ULL;

    MoveList moves;
    generate_pseudo_legal_moves(pos, moves);

    uint64_t nodes = 0;
    for (const auto& m : moves) {
        if (m.is_castling() && !is_castling_legal(pos, m.flag())) {
            continue;
        }

        UndoState undo;
        pos.make_move(m, undo);

        if (!is_in_check(pos, ~pos.side_to_move())) {
            if (depth == 1) {
                nodes++;
            } else {
                nodes += perft(pos, depth - 1);
            }
        }

        pos.unmake_move(undo);
    }

    return nodes;
}

PerftResults perft_divide(Position& pos, int depth) noexcept {
    PerftResults results{};
    if (depth <= 0) {
        results.nodes = 1;
        return results;
    }

    MoveList moves;
    generate_pseudo_legal_moves(pos, moves);

    for (const auto& m : moves) {
        if (m.is_castling() && !is_castling_legal(pos, m.flag())) {
            continue;
        }

        UndoState undo;
        pos.make_move(m, undo);

        if (!is_in_check(pos, ~pos.side_to_move())) {
            uint64_t sub_nodes = (depth == 1) ? 1ULL : perft(pos, depth - 1);
            results.divide.emplace_back(m, sub_nodes);
            results.nodes += sub_nodes;
        }

        pos.unmake_move(undo);
    }

    return results;
}

} // namespace chess
