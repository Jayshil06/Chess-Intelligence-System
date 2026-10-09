#include <gtest/gtest.h>
#include "board/fen.h"
#include "evaluation/evaluate.h"
#include "evaluation/nnue.h"
#include "move/movegen.h"
#include "protocol/uci.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>

using namespace chess;

namespace {

struct TestNet {
    int hidden = 8;
    int l1 = 4;
    std::vector<int16_t> ft_weight, ft_bias;
    std::vector<float> l1_weight, l1_bias, out_weight;
    float out_bias = 0.1f;
};

TestNet make_net() {
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> w(-100, 100), b(-50, 150);
    std::uniform_real_distribution<float> f(-0.5f, 0.5f);
    TestNet n;
    for (int i = 0; i < nnue::NUM_FEATURES * n.hidden; ++i) n.ft_weight.push_back(static_cast<int16_t>(w(rng)));
    for (int i = 0; i < n.hidden; ++i) n.ft_bias.push_back(static_cast<int16_t>(b(rng)));
    for (int i = 0; i < n.l1 * 2 * n.hidden; ++i) n.l1_weight.push_back(f(rng));
    for (int i = 0; i < n.l1; ++i) n.l1_bias.push_back(f(rng));
    for (int i = 0; i < n.l1; ++i) n.out_weight.push_back(f(rng) * 4);
    return n;
}

template <typename T>
void put(std::ofstream& out, const std::vector<T>& v) {
    out.write(reinterpret_cast<const char*>(v.data()), static_cast<std::streamsize>(v.size() * sizeof(T)));
}

std::string write_net(const TestNet& n, const char* magic = "CINN", int extra_bytes = 0, int drop_bytes = 0) {
    auto path = std::filesystem::temp_directory_path() /
                (std::string("cis_") + ::testing::UnitTest::GetInstance()->current_test_info()->name() + ".nnue");
    {
        std::ofstream out(path, std::ios::binary);
        out.write(magic, 4);
        uint32_t header[3] = {1, static_cast<uint32_t>(n.hidden), static_cast<uint32_t>(n.l1)};
        out.write(reinterpret_cast<const char*>(header), sizeof(header));
        put(out, n.ft_weight);
        put(out, n.ft_bias);
        put(out, n.l1_weight);
        put(out, n.l1_bias);
        put(out, n.out_weight);
        out.write(reinterpret_cast<const char*>(&n.out_bias), sizeof(float));
        for (int i = 0; i < extra_bytes; ++i) out.put('\0');
    }
    if (drop_bytes) std::filesystem::resize_file(path, std::filesystem::file_size(path) - static_cast<size_t>(drop_bytes));
    return path.string();
}

// Independent forward pass computed from the board, without incremental accumulators
int reference_eval(const TestNet& n, const Position& pos) {
    std::vector<float> x;
    for (Color persp : {pos.side_to_move(), ~pos.side_to_move()}) {
        std::vector<int> acc(n.ft_bias.begin(), n.ft_bias.end());
        for (int s = 0; s < NUM_SQUARES; ++s) {
            Piece p = pos.piece_at(static_cast<Square>(s));
            if (p == Piece::None) continue;
            size_t idx = nnue::feature_index(p, static_cast<Square>(s), persp);
            for (int i = 0; i < n.hidden; ++i) acc[static_cast<size_t>(i)] += n.ft_weight[idx * 8 + static_cast<size_t>(i)];
        }
        for (int a : acc) x.push_back(static_cast<float>(std::clamp(a, 0, nnue::QA)) / nnue::QA);
    }
    float out = n.out_bias;
    for (int j = 0; j < n.l1; ++j) {
        float h = n.l1_bias[static_cast<size_t>(j)];
        for (int i = 0; i < 2 * n.hidden; ++i) h += n.l1_weight[static_cast<size_t>(j * 2 * n.hidden + i)] * x[static_cast<size_t>(i)];
        out += n.out_weight[static_cast<size_t>(j)] * std::clamp(h, 0.0f, 1.0f);
    }
    return std::clamp(static_cast<int>(std::lround(out * nnue::WDL_SCALE)), -10000, 10000);
}

Position mirrored(const Position& pos) {
    Position m;
    for (int s = 0; s < NUM_SQUARES; ++s) {
        Piece p = pos.piece_at(static_cast<Square>(s));
        if (p != Piece::None) m.put_piece(make_piece(~color_of(p), type_of(p)), static_cast<Square>(s ^ 56));
    }
    m.set_side_to_move(~pos.side_to_move());
    m.recalculate_hash();
    return m;
}

class NnueTest : public ::testing::Test {
protected:
    void TearDown() override { nnue::unload(); }
    TestNet net = make_net();
};

} // namespace

TEST(NnueFeatureTest, BlackViewMirrorsAndSwapsColours) {
    EXPECT_EQ(nnue::feature_index(Piece::WhitePawn, Square::A2, Color::White), 8u);
    EXPECT_EQ(nnue::feature_index(Piece::BlackPawn, Square::A7, Color::Black), 8u);
    EXPECT_EQ(nnue::feature_index(Piece::WhitePawn, Square::A2, Color::Black), 6u * 64 + 48);
    EXPECT_EQ(nnue::feature_index(Piece::BlackKing, Square::H8, Color::White), 11u * 64 + 63);
}

TEST_F(NnueTest, RejectsMalformedFiles) {
    EXPECT_FALSE(nnue::load("does_not_exist.nnue"));
    EXPECT_FALSE(nnue::load(write_net(net, "XXXX")));
    EXPECT_FALSE(nnue::load(write_net(net, "CINN", 0, 4)));
    EXPECT_FALSE(nnue::load(write_net(net, "CINN", 3)));
    EXPECT_EQ(nnue::active(), nullptr);

    ASSERT_TRUE(nnue::load(write_net(net)));
    const nnue::Network* loaded = nnue::active();
    EXPECT_FALSE(nnue::load(write_net(net, "XXXX")));
    EXPECT_EQ(nnue::active(), loaded);  // A failed load keeps the current network
}

TEST_F(NnueTest, EvaluationMatchesReference) {
    ASSERT_TRUE(nnue::load(write_net(net)));
    for (const char* f : {fen::START_FEN.data(),
                          "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
                          "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 b - - 0 1"}) {
        Position pos = *fen::parse(f);
        EXPECT_EQ(eval::evaluate(pos), reference_eval(net, pos)) << f;
        Position flip = mirrored(pos);
        EXPECT_EQ(eval::evaluate(flip), eval::evaluate(pos)) << f;
    }
}

TEST_F(NnueTest, IncrementalAccumulatorsNeverDrift) {
    ASSERT_TRUE(nnue::load(write_net(net)));
    Position pos = *fen::parse("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    const Position start = pos;
    std::mt19937 rng(7);
    std::vector<UndoState> undos;
    for (int ply = 0; ply < 300; ++ply) {
        MoveList moves = generate_legal_moves(pos);
        if (moves.empty()) break;
        undos.emplace_back();
        pos.make_move(moves[rng() % moves.size()], undos.back());
        ASSERT_TRUE(pos.validate_invariants()) << ply;
        ASSERT_EQ(eval::evaluate(pos), reference_eval(net, pos)) << ply;
    }
    while (!undos.empty()) {
        pos.unmake_move(undos.back());
        undos.pop_back();
    }
    EXPECT_EQ(pos, start);
}

TEST_F(NnueTest, UnloadRestoresClassicalEvaluation) {
    Position pos(true);
    pos.make_move(*uci::parse_move(pos, "e2e4"));
    int classical = eval::evaluate(pos);
    ASSERT_TRUE(nnue::load(write_net(net)));
    pos.refresh();
    EXPECT_EQ(eval::evaluate(pos), reference_eval(net, pos));
    nnue::unload();
    EXPECT_EQ(eval::evaluate(pos), classical);
}

TEST_F(NnueTest, UciEvalFileOption) {
    std::ostringstream out;
    uci::Engine engine(out);
    engine.handle("position startpos moves e2e4 e7e5");
    engine.handle("eval");
    EXPECT_NE(out.str().find("classical"), std::string::npos);

    engine.handle("setoption name EvalFile value " + write_net(net));
    engine.handle("eval");
    Position expected = *fen::parse("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2");
    EXPECT_NE(out.str().find("Evaluation: " + std::to_string(reference_eval(net, expected)) + " (side to move, nnue)"),
              std::string::npos);

    engine.handle("setoption name EvalFile value missing file.nnue");
    EXPECT_NE(out.str().find("failed to load network: missing file.nnue"), std::string::npos);
    engine.handle("setoption name EvalFile value <empty>");
    EXPECT_EQ(nnue::active(), nullptr);
}
