#include "evaluation/nnue.h"
#include "board/position.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <fstream>
#include <memory>

namespace chess {
namespace nnue {

static_assert(std::endian::native == std::endian::little, "network files are little-endian");

namespace {

std::unique_ptr<Network> g_storage;

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
    net->hidden = static_cast<int>(header[1]);
    net->l1 = static_cast<int>(header[2]);
    const size_t hidden = header[1], l1 = header[2];
    net->ft_weight.resize(NUM_FEATURES * hidden);
    net->ft_bias.resize(hidden);
    net->l1_weight.resize(l1 * 2 * hidden);
    net->l1_bias.resize(l1);
    net->out_weight.resize(l1);
    bool ok = read(in, net->ft_weight.data(), net->ft_weight.size()) && read(in, net->ft_bias.data(), hidden) &&
              read(in, net->l1_weight.data(), net->l1_weight.size()) && read(in, net->l1_bias.data(), l1) &&
              read(in, net->out_weight.data(), l1) && read(in, &net->out_bias, 1);
    if (!ok || in.peek() != std::char_traits<char>::eof()) return false;

    g_storage = std::move(net);
    detail::g_active = g_storage.get();
    return true;
}

void unload() noexcept {
    detail::g_active = nullptr;
    g_storage.reset();
}

int evaluate(const Position& pos) noexcept {
    const Network& net = *detail::g_active;
    const int hidden = net.hidden;
    std::array<float, 2 * MAX_HIDDEN> x{};
    const Accumulator& us = pos.accumulator(pos.side_to_move());
    const Accumulator& them = pos.accumulator(~pos.side_to_move());
    for (int i = 0; i < hidden; ++i) {
        x[static_cast<size_t>(i)] = static_cast<float>(std::clamp<int>(us[static_cast<size_t>(i)], 0, QA)) / QA;
        x[static_cast<size_t>(hidden + i)] =
            static_cast<float>(std::clamp<int>(them[static_cast<size_t>(i)], 0, QA)) / QA;
    }

    float out = net.out_bias;
    for (int j = 0; j < net.l1; ++j) {
        const float* w = net.l1_weight.data() + static_cast<size_t>(j) * 2 * static_cast<size_t>(hidden);
        float h = net.l1_bias[static_cast<size_t>(j)];
        for (int i = 0; i < 2 * hidden; ++i) h += w[i] * x[static_cast<size_t>(i)];
        out += net.out_weight[static_cast<size_t>(j)] * std::clamp(h, 0.0f, 1.0f);
    }
    // Stay well inside the search's mate-score range
    return std::clamp(static_cast<int>(std::lround(out * WDL_SCALE)), -10000, 10000);
}

} // namespace nnue
} // namespace chess
