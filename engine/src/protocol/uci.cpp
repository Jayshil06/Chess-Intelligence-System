#include "protocol/uci.h"
#include "board/fen.h"
#include "move/movegen.h"
#include "move/perft.h"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <sstream>

namespace chess {
namespace uci {

namespace {

uint64_t elapsed_ms(std::chrono::steady_clock::time_point start) {
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
    return std::max<uint64_t>(static_cast<uint64_t>(ms), 1);
}

} // namespace

std::optional<Move> parse_move(const Position& pos, std::string_view str) {
    for (Move m : generate_legal_moves(pos)) {
        if (m.to_uci() == str) return m;
    }
    return std::nullopt;
}

std::string format_score(int score) {
    if (!search::is_mate_score(score)) return "cp " + std::to_string(score);
    int moves = (search::MATE_SCORE - std::abs(score) + 1) / 2;
    return "mate " + std::to_string(score > 0 ? moves : -moves);
}

uint64_t bench(std::ostream& out, int depth) {
    static constexpr std::string_view BENCH_FENS[] = {
        fen::START_FEN,
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
    };

    search::Searcher searcher;
    search::SearchLimits limits;
    limits.max_depth = depth;
    uint64_t nodes = 0;
    auto start = std::chrono::steady_clock::now();

    for (std::string_view f : BENCH_FENS) {
        Position pos = *fen::parse(f);
        searcher.clear();
        search::SearchResult r = searcher.search(pos, limits);
        nodes += r.nodes;
        out << "bestmove " << r.best_move.to_uci() << " score " << format_score(r.score)
            << " nodes " << r.nodes << "  [" << f << "]\n";
    }

    uint64_t ms = elapsed_ms(start);
    out << "\nNodes searched : " << nodes << "\nTime (ms)      : " << ms
        << "\nNodes/second   : " << nodes * 1000 / ms << std::endl;
    return nodes;
}

Engine::Engine(std::ostream& out) : m_out(out) {
    m_searcher.set_info_callback([this](const search::SearchResult& r) {
        std::string line = "info depth " + std::to_string(r.depth) + " score " + format_score(r.score) +
                           " nodes " + std::to_string(r.nodes) +
                           " nps " + std::to_string(r.nodes * 1000 / std::max<uint64_t>(r.time_ms, 1)) +
                           " time " + std::to_string(r.time_ms) +
                           " hashfull " + std::to_string(m_searcher.tt().hashfull()) + " pv";
        for (Move m : r.pv) line += ' ' + m.to_uci();
        send(line);
    });
}

Engine::~Engine() {
    stop_search();
}

void Engine::loop(std::istream& in) {
    std::string line;
    while (std::getline(in, line) && handle(line)) {}
}

bool Engine::handle(const std::string& line) {
    std::istringstream is(line);
    std::string cmd;
    is >> cmd;

    if (cmd == "uci") {
        send("id name " + std::string(ENGINE_NAME) + "\nid author Jayshil\n"
             "option name Hash type spin default 16 min 1 max 4096\nuciok");
    } else if (cmd == "isready") {
        send("readyok");
    } else if (cmd == "ucinewgame") {
        wait();
        m_searcher.clear();
        m_pos = Position(true);
    } else if (cmd == "position") {
        wait();
        cmd_position(is);
    } else if (cmd == "go") {
        wait();
        cmd_go(is);
    } else if (cmd == "stop") {
        stop_search();
    } else if (cmd == "setoption") {
        wait();
        cmd_setoption(is);
    } else if (cmd == "d") {
        send("Fen: " + fen::to_string(m_pos));
    } else if (cmd == "perft") {
        wait();
        int depth;
        if (!(is >> depth)) depth = 1;
        Position pos = m_pos;
        auto start = std::chrono::steady_clock::now();
        PerftResults r = perft_divide(pos, depth);
        std::string out;
        for (const auto& [m, n] : r.divide) out += m.to_uci() + ": " + std::to_string(n) + '\n';
        out += "\nNodes searched: " + std::to_string(r.nodes) + "\nTime (ms): " + std::to_string(elapsed_ms(start));
        send(out);
    } else if (cmd == "bench") {
        wait();
        int depth;
        if (!(is >> depth)) depth = BENCH_DEPTH;
        std::ostringstream out;
        bench(out, depth);
        send(out.str());
    } else if (cmd == "quit") {
        stop_search();
        return false;
    } else if (!cmd.empty()) {
        send("info string unknown command: " + cmd);
    }
    return true;
}

void Engine::cmd_position(std::istream& is) {
    std::string tok, fen_str;
    is >> tok;
    if (tok == "startpos") {
        fen_str = fen::START_FEN;
        tok.clear();
        is >> tok;
    } else if (tok == "fen") {
        while (is >> tok && tok != "moves") fen_str += tok + ' ';
    } else {
        send("info string invalid position command");
        return;
    }

    auto parsed = fen::parse(fen_str);
    if (!parsed) {
        send("info string invalid fen");
        return;
    }
    m_pos = *parsed;

    if (tok != "moves") return;
    while (is >> tok) {
        auto m = parse_move(m_pos, tok);
        if (!m) {
            send("info string illegal move: " + tok);
            return;
        }
        m_pos.make_move(*m);
    }
}

void Engine::cmd_go(std::istream& is) {
    search::SearchLimits limits;
    int64_t time[2] = {0, 0}, inc[2] = {0, 0}, movestogo = 0, movetime = 0;
    bool has_time[2] = {false, false};
    bool infinite = false;

    std::string tok;
    while (is >> tok) {
        if (tok == "depth") is >> limits.max_depth;
        else if (tok == "nodes") is >> limits.max_nodes;
        else if (tok == "movetime") is >> movetime;
        else if (tok == "wtime") has_time[0] = static_cast<bool>(is >> time[0]);
        else if (tok == "btime") has_time[1] = static_cast<bool>(is >> time[1]);
        else if (tok == "winc") is >> inc[0];
        else if (tok == "binc") is >> inc[1];
        else if (tok == "movestogo") is >> movestogo;
        else if (tok == "infinite") infinite = true;
    }

    size_t us = static_cast<size_t>(m_pos.side_to_move());
    if (movetime > 0) {
        limits.max_time_ms = static_cast<uint64_t>(movetime);
    } else if (has_time[us]) {
        // Even share of the clock plus most of the increment; GUIs may send a negative clock
        int64_t left = std::max<int64_t>(time[us], 0);
        int64_t alloc = left / (movestogo > 0 ? movestogo : 30) + inc[us] * 3 / 4;
        limits.max_time_ms = static_cast<uint64_t>(std::clamp<int64_t>(alloc, 1, std::max<int64_t>(left - 50, 1)));
    }
    limits.stop = &m_stop;

    m_stop = false;
    m_infinite = infinite;
    m_thread = std::thread([this, limits, infinite, pos = m_pos]() mutable {
        search::SearchResult r = m_searcher.search(pos, limits);
        // Under "go infinite", bestmove must wait for "stop"
        while (infinite && !m_stop) std::this_thread::sleep_for(std::chrono::milliseconds(1));
        std::string line = "bestmove " + r.best_move.to_uci();
        if (r.pv.size() > 1) line += " ponder " + r.pv[1].to_uci();
        send(line);
    });
}

void Engine::cmd_setoption(std::istream& is) {
    std::string tok, name, value;
    while (is >> tok && tok != "value") {
        if (tok != "name") name += (name.empty() ? "" : " ") + tok;
    }
    is >> value;

    if (name == "Hash") {
        m_searcher.tt().resize(static_cast<size_t>(std::clamp(std::atoi(value.c_str()), 1, 4096)));
    } else {
        send("info string unknown option: " + name);
    }
}

void Engine::stop_search() {
    m_stop = true;
    wait();
}

void Engine::wait() {
    if (m_infinite) m_stop = true;  // An infinite search never finishes on its own
    if (m_thread.joinable()) m_thread.join();
}

void Engine::send(const std::string& msg) {
    std::lock_guard lock(m_out_mutex);
    m_out << msg << std::endl;
}

} // namespace uci
} // namespace chess
