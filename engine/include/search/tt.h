#pragma once

#include "move/move.h"
#include <cstdint>
#include <cstddef>
#include <vector>

namespace chess {
namespace search {

enum class Bound : uint8_t { None = 0, Exact = 1, Lower = 2, Upper = 3 };

struct TTEntry {
    uint64_t key{0};
    int16_t score{0};
    uint16_t move{0};
    int8_t depth{0};
    Bound bound{Bound::None};
};

class TranspositionTable {
public:
    explicit TranspositionTable(size_t size_mb = 16) { resize(size_mb); }

    void resize(size_t size_mb);
    void clear() noexcept;

    [[nodiscard]] const TTEntry* probe(uint64_t key) const noexcept {
        const TTEntry& e = m_table[key & m_mask];
        return (e.bound != Bound::None && e.key == key) ? &e : nullptr;
    }

    void store(uint64_t key, int depth, int score, Bound bound, Move move) noexcept;

    [[nodiscard]] size_t size() const noexcept { return m_table.size(); }
    [[nodiscard]] int hashfull() const noexcept;  // Occupancy in permille (UCI)

private:
    std::vector<TTEntry> m_table;
    uint64_t m_mask{0};
};

} // namespace search
} // namespace chess
