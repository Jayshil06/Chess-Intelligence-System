#include "evaluation/nnue.h"
#include "board/position.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <fstream>

namespace chess {
namespace nnue {

static_assert(std::endian::native == std::endian::little, "network files are little-endian");

namespace {

template <typename T>
bool read(std::istream& in, T* data, size_t count) {
    return static_cast<bool>(in.read(reinterpret_cast<char*>(data), static_cast<std::streamsize>(count * sizeof(T))));
}

} // namespace

bool load(const std::string& path) {
    if (path.empty()) {
        unload();
        return true;
    }
    std::ifstream in(path, std::ios::binary);
    char magic[4];
    uint32_t header[3];
    if (!in || !read(in, magic, 4) || std::memcmp(magic, "CINN", 4) != 0 || !read(in, header, 3) ||
        header[0] != 1 || header[1] == 0 || header[1] > MAX_HIDDEN || header[2] == 0 || header[2] > MAX_L1) {
        return false;
    }

    auto net = std::make_unique<Network>();
    const size_t hidden = net->hidden = header[1];
    const size_t l1 = net->l1 = header[2];
    std::vector<float> l1_file(l1 * 2 * hidden);  // File order: [l1][2*hidden]
    net->ft_weight.resize(NUM_FEATURES * hidden);
    net->ft_bias.resize(hidden);
    net->l1_bias.resize(l1);
    net->out_weight.resize(l1);
    bool ok = read(in, net->ft_weight.data(), net->ft_weight.size()) && read(in, net->ft_bias.data(), hidden) &&
              read(in, l1_file.data(), l1_file.size()) && read(in, net->l1_bias.data(), l1) &&
              read(in, net->out_weight.data(), l1) && read(in, &net->out_bias, 1);
    if (!ok || in.peek() != std::char_traits<char>::eof()) return false;

    net->l1_weight.resize(l1_file.size());
    for (size_t j = 0; j < l1; ++j) {
        for (size_t i = 0; i < 2 * hidden; ++i) net->l1_weight[i * l1 + j] = l1_file[j * 2 * hidden + i] / QA;
    }
    detail::g_network = std::move(net);
    return true;
}

void unload() noexcept {
    detail::g_network.reset();
}

int evaluate(const Position& pos) noexcept {
    const Network& net = *detail::g_network;
    std::array<float, MAX_L1> h{};
    std::copy(net.l1_bias.begin(), net.l1_bias.end(), h.begin());

    // Side to move's accumulator feeds the first half of the inputs, the opponent's the second
    const Accumulator* sides[2] = {&pos.accumulator(pos.side_to_move()), &pos.accumulator(~pos.side_to_move())};
    for (size_t s = 0; s < 2; ++s) {
        for (size_t i = 0; i < net.hidden; ++i) {
            const int a = std::clamp<int>((*sides[s])[i], 0, QA);
            if (a == 0) continue;  // Clipped ReLU zeros contribute nothing
            const float* w = net.l1_weight.data() + (s * net.hidden + i) * net.l1;
            for (size_t j = 0; j < net.l1; ++j) h[j] += w[j] * static_cast<float>(a);
        }
    }

    float out = net.out_bias;
    for (size_t j = 0; j < net.l1; ++j) out += net.out_weight[j] * std::clamp(h[j], 0.0f, 1.0f);
    // Stay well inside the search's mate-score range
    return std::clamp(static_cast<int>(std::lround(out * WDL_SCALE)), -10000, 10000);
}

} // namespace nnue
} // namespace chess
