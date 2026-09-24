// Engine benchmark and perft gate. Exits non-zero if any perft count mismatches.
#include "board/fen.h"
#include "move/movegen.h"
#include "move/perft.h"
#include "search/search.h"
#include <chrono>
#include <cstdio>

using namespace chess;

namespace {

struct GatePosition {
    const char* name;
    const char* fen;
    int depth;
    uint64_t expected;
};

constexpr GatePosition GATE[] = {
    {"startpos", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 5, 4865609},
    {"kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 4, 4085603},
    {"pos3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 6, 11030083},
    {"pos4", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 5, 15833292},
    {"pos5", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 4, 2103487},
};

double ms_since(std::chrono::steady_clock::time_point start) {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}

template <typename F>
double calls_per_sec(F&& fn, int calls) {
    auto start = std::chrono::steady_clock::now();
    size_t sink = 0;
    for (int i = 0; i < calls; ++i) sink += fn();
    double ms = ms_since(start);
    if (sink == 0) std::printf("#");  // Keep the loop observable
    return calls * 1000.0 / ms;
}

} // namespace

int main() {
    bool ok = true;

    for (const auto& g : GATE) {
        Position pos = *fen::parse(g.fen);
        auto start = std::chrono::steady_clock::now();
        uint64_t nodes = perft(pos, g.depth);
        double ms = ms_since(start);
        bool pass = nodes == g.expected;
        ok &= pass;
        std::printf("perft    %-9s depth %d nodes %10llu expected %10llu ms %8.1f mnps %6.1f %s\n", g.name, g.depth,
                    static_cast<unsigned long long>(nodes), static_cast<unsigned long long>(g.expected), ms,
                    nodes / ms / 1000.0, pass ? "OK" : "FAIL");
    }

    for (int i = 0; i < 2; ++i) {
        Position pos = *fen::parse(GATE[i].fen);
        double legal = calls_per_sec([&] { return generate_legal_moves(pos).size(); }, 200000);
        double pseudo = calls_per_sec([&] { return generate_pseudo_legal_moves(pos).size(); }, 200000);
        std::printf("movegen  %-9s legal/s %10.0f pseudo/s %10.0f\n", GATE[i].name, legal, pseudo);
    }

    // Same final position (startpos) reached through increasingly long knight-shuffle histories
    const char* shuffle[] = {"g1f3", "g8f6", "f3g1", "f6g8"};
    for (int plies : {0, 8, 40, 80}) {
        Position pos(true);
        for (int p = 0; p < plies; ++p) {
            for (Move m : generate_legal_moves(pos)) {
                if (m.to_uci() == shuffle[p % 4]) { pos.make_move(m); break; }
            }
        }
        double legal = calls_per_sec([&] { return generate_legal_moves(pos).size(); }, 200000);
        std::printf("history  plies %2d  legal/s %10.0f\n", plies, legal);
    }

    for (int d = 1; d <= 6; ++d) {
        Position pos(true);
        search::Searcher searcher;
        search::SearchLimits limits;
        limits.max_depth = d;
        auto start = std::chrono::steady_clock::now();
        search::SearchResult r = searcher.search(pos, limits);
        std::printf("search   startpos  depth %d nodes %10llu ms %8.1f best %s\n", d,
                    static_cast<unsigned long long>(r.nodes), ms_since(start), r.best_move.to_uci().c_str());
    }

    for (int i : {1, 3, 4}) {
        Position pos = *fen::parse(GATE[i].fen);
        search::Searcher searcher;
        search::SearchLimits limits;
        limits.max_time_ms = 100;
        auto start = std::chrono::steady_clock::now();
        search::SearchResult r = searcher.search(pos, limits);
        std::printf("timed    %-9s budget_ms 100 actual_ms %8.1f depth %d nodes %llu\n", GATE[i].name,
                    ms_since(start), r.depth, static_cast<unsigned long long>(r.nodes));
    }

    std::printf("perft gate: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
