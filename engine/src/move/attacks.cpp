#include "move/attacks.h"

namespace chess {
namespace attacks {
namespace detail {

SliderEntry ROOK_ENTRIES[NUM_SQUARES];
SliderEntry BISHOP_ENTRIES[NUM_SQUARES];

namespace {

std::array<Bitboard, 102400> rook_table;
std::array<Bitboard, 5248> bishop_table;

void init_sliders(SliderEntry* entries, Bitboard* table, Bitboard (*reference)(Square, Bitboard)) {
    std::array<Bitboard, 4096> occupancy{}, reference_attacks{};
#if !defined(__BMI2__)
    std::array<int, 4096> epoch{};
    int attempt = 0;
    uint64_t seed = 0x2545F4914F6CDD1DULL;
    auto rand64 = [&seed] {
        seed ^= seed >> 12;
        seed ^= seed << 25;
        seed ^= seed >> 27;
        return seed * 0x2545F4914F6CDD1DULL;
    };
#endif

    for (size_t sq = 0; sq < NUM_SQUARES; ++sq) {
        Square s = static_cast<Square>(sq);
        // Edge squares cannot block anything beyond them
        Bitboard edges = ((bb::RANK_1 | bb::RANK_8) & ~bb::rank_mask(square_rank(s))) |
                         ((bb::FILE_A | bb::FILE_H) & ~bb::file_mask(square_file(s)));
        SliderEntry& e = entries[sq];
        e.mask = reference(s, bb::EMPTY) & ~edges;
        e.shift = static_cast<unsigned>(64 - bb::popcount(e.mask));
        e.attacks = table;

        // Carry-Rippler subset enumeration
        size_t size = 0;
        Bitboard b = 0;
        do {
            occupancy[size] = b;
            reference_attacks[size++] = reference(s, b);
            b = (b - e.mask) & e.mask;
        } while (b);

#if defined(__BMI2__)
        for (size_t i = 0; i < size; ++i) table[e.index(occupancy[i])] = reference_attacks[i];
#else
        // Retry random magics until every subset maps without a collision
        for (size_t i = 0; i < size;) {
            do {
                e.magic = rand64() & rand64() & rand64();
            } while (bb::popcount((e.mask * e.magic) >> 56) < 6);
            ++attempt;
            for (i = 0; i < size; ++i) {
                size_t idx = e.index(occupancy[i]);
                if (epoch[idx] < attempt) {
                    epoch[idx] = attempt;
                    table[idx] = reference_attacks[i];
                } else if (table[idx] != reference_attacks[i]) {
                    break;
                }
            }
        }
#endif
        table += size;
    }
}

// Built at static init; every slider lookup references the entry arrays, so this TU always links
[[maybe_unused]] const bool initialized = [] {
    init_sliders(ROOK_ENTRIES, rook_table.data(), ray_rook_attacks);
    init_sliders(BISHOP_ENTRIES, bishop_table.data(), ray_bishop_attacks);
    return true;
}();

} // namespace

} // namespace detail
} // namespace attacks
} // namespace chess
