#include "protocol/uci.h"
#include <cstdlib>
#include <iostream>
#include <string_view>

int main(int argc, char* argv[]) {
    // "chess_engine bench [depth]" runs the benchmark and exits; otherwise speak UCI on stdin/stdout
    if (argc > 1 && std::string_view(argv[1]) == "bench") {
        chess::uci::bench(std::cout, argc > 2 ? std::atoi(argv[2]) : chess::uci::BENCH_DEPTH);
        return 0;
    }

    chess::uci::Engine engine(std::cout);
    engine.loop(std::cin);
    return 0;
}
