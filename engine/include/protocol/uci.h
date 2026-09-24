#pragma once

#include "board/position.h"
#include "search/search.h"
#include <atomic>
#include <cstdint>
#include <iosfwd>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>

namespace chess {
namespace uci {

constexpr std::string_view ENGINE_NAME = "Chess Intelligence Engine 0.2.0";
constexpr int BENCH_DEPTH = 7;

// Matches against legal moves so the result carries the correct move flags
std::optional<Move> parse_move(const Position& pos, std::string_view str);

std::string format_score(int score);

uint64_t bench(std::ostream& out, int depth = BENCH_DEPTH);

class Engine {
public:
    explicit Engine(std::ostream& out);
    ~Engine();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    void loop(std::istream& in);
    bool handle(const std::string& line);  // Returns false on "quit"
    void wait();                           // Also stops "go infinite"

    [[nodiscard]] const Position& position() const noexcept { return m_pos; }

private:
    void cmd_position(std::istream& is);
    void cmd_go(std::istream& is);
    void cmd_setoption(std::istream& is);
    void stop_search();
    void send(const std::string& msg);

    std::ostream& m_out;
    std::mutex m_out_mutex;
    Position m_pos{true};
    search::Searcher m_searcher;
    std::thread m_thread;
    std::atomic<bool> m_stop{false};
    bool m_infinite{false};
};

} // namespace uci
} // namespace chess
