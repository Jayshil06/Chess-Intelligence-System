#include "search/tt.h"
#include <algorithm>
#include <bit>

namespace chess {
namespace search {

void TranspositionTable::resize(size_t size_mb) {
    size_t entries = std::max<size_t>(size_mb, 1) * 1024 * 1024 / sizeof(TTEntry);
    m_table.assign(std::bit_floor(entries), TTEntry{});
    m_mask = m_table.size() - 1;
}

void TranspositionTable::clear() noexcept {
    std::fill(m_table.begin(), m_table.end(), TTEntry{});
}

void TranspositionTable::store(uint64_t key, int depth, int score, Bound bound, Move move) noexcept {
    TTEntry& e = m_table[key & m_mask];
    if (e.key == key) {
        // Keep deeper results for the same position unless the new one is exact
        if (depth < e.depth && bound != Bound::Exact) return;
        if (move.is_null()) move = Move(e.move);
    }
    e = TTEntry{key, static_cast<int16_t>(score), move.raw_move(),
                static_cast<int8_t>(std::min(depth, 127)), bound};
}

int TranspositionTable::hashfull() const noexcept {
    size_t sample = std::min<size_t>(1000, m_table.size());
    size_t used = std::count_if(m_table.begin(), m_table.begin() + static_cast<std::ptrdiff_t>(sample),
                                [](const TTEntry& e) { return e.bound != Bound::None; });
    return static_cast<int>(used * 1000 / sample);
}

} // namespace search
} // namespace chess
