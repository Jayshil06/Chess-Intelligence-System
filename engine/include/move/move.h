#pragma once

#include "board/types.h"
#include <cstdint>
#include <string>
#include <string_view>
#include <optional>
#include <compare>
#include <array>

namespace chess {

enum class MoveFlag : uint16_t {
    Quiet = 0,
    DoublePush = 1,
    CastleKingside = 2,
    CastleQueenside = 3,
    Capture = 4,
    EnPassant = 5,
    KnightPromotion = 8,
    BishopPromotion = 9,
    RookPromotion = 10,
    QueenPromotion = 11,
    KnightPromotionCapture = 12,
    BishopPromotionCapture = 13,
    RookPromotionCapture = 14,
    QueenPromotionCapture = 15
};

class Move {
public:
    constexpr Move() noexcept : data_(0) {}
    constexpr explicit Move(uint32_t raw_data) noexcept : data_(raw_data) {}

    constexpr Move(Square from, Square to, MoveFlag flag = MoveFlag::Quiet) noexcept
        : data_(static_cast<uint16_t>(static_cast<uint8_t>(from)) |
                (static_cast<uint16_t>(static_cast<uint8_t>(to)) << 6) |
                (static_cast<uint16_t>(flag) << 12)) {}

    constexpr Move(Square from, Square to, MoveFlag flag, PieceType promo) noexcept
        : Move(from, to, make_promo_flag(flag, promo)) {}

    static constexpr Move make(Square from, Square to, MoveFlag flag = MoveFlag::Quiet) noexcept {
        return Move(from, to, flag);
    }

    static constexpr Move make_promotion(Square from, Square to, PieceType promo, bool is_capture = false) noexcept {
        uint16_t flag_val = is_capture ? 12 : 8;
        switch (promo) {
            case PieceType::Knight: flag_val += 0; break;
            case PieceType::Bishop: flag_val += 1; break;
            case PieceType::Rook:   flag_val += 2; break;
            case PieceType::Queen:  flag_val += 3; break;
            default: break;
        }
        return Move(from, to, static_cast<MoveFlag>(flag_val));
    }

    static constexpr Move null() noexcept {
        return Move(0);
    }

    constexpr Square from() const noexcept {
        return static_cast<Square>(data_ & 0x3F);
    }

    constexpr Square to() const noexcept {
        return static_cast<Square>((data_ >> 6) & 0x3F);
    }

    constexpr MoveFlag flag() const noexcept {
        return static_cast<MoveFlag>((data_ >> 12) & 0xF);
    }

    constexpr uint16_t raw_move() const noexcept {
        return static_cast<uint16_t>(data_ & 0xFFFF);
    }

    constexpr uint32_t raw() const noexcept {
        return data_;
    }

    constexpr int16_t score() const noexcept {
        return static_cast<int16_t>(data_ >> 16);
    }

    constexpr void set_score(int16_t s) noexcept {
        data_ = (data_ & 0xFFFF) | (static_cast<uint32_t>(static_cast<uint16_t>(s)) << 16);
    }

    constexpr bool is_null() const noexcept {
        return raw_move() == 0;
    }

    constexpr bool is_valid() const noexcept {
        return is_valid_square(from()) && is_valid_square(to()) && from() != to();
    }

    constexpr bool is_quiet() const noexcept {
        return flag() == MoveFlag::Quiet;
    }

    constexpr bool is_capture() const noexcept {
        return (static_cast<uint16_t>(flag()) & 0x4) != 0;
    }

    constexpr bool is_en_passant() const noexcept {
        return flag() == MoveFlag::EnPassant;
    }

    constexpr bool is_double_push() const noexcept {
        return flag() == MoveFlag::DoublePush;
    }

    constexpr bool is_castling() const noexcept {
        return flag() == MoveFlag::CastleKingside || flag() == MoveFlag::CastleQueenside;
    }

    constexpr bool is_kingside_castling() const noexcept {
        return flag() == MoveFlag::CastleKingside;
    }

    constexpr bool is_queenside_castling() const noexcept {
        return flag() == MoveFlag::CastleQueenside;
    }

    constexpr bool is_promotion() const noexcept {
        return (static_cast<uint16_t>(flag()) & 0x8) != 0;
    }

    constexpr PieceType promotion_type() const noexcept {
        if (!is_promotion()) return PieceType::None;
        switch (static_cast<uint16_t>(flag()) & 0x3) {
            case 0: return PieceType::Knight;
            case 1: return PieceType::Bishop;
            case 2: return PieceType::Rook;
            case 3: return PieceType::Queen;
            default: return PieceType::None;
        }
    }

    constexpr bool operator==(const Move& other) const noexcept {
        return raw_move() == other.raw_move();
    }

    constexpr bool operator!=(const Move& other) const noexcept {
        return !(*this == other);
    }

    constexpr std::strong_ordering operator<=>(const Move& other) const noexcept {
        return raw_move() <=> other.raw_move();
    }

    constexpr explicit operator bool() const noexcept {
        return !is_null();
    }

    std::string to_uci() const;
    std::string to_string() const;
    static std::optional<Move> from_uci(std::string_view uci);

private:
    static constexpr MoveFlag make_promo_flag(MoveFlag base_flag, PieceType promo) noexcept {
        bool is_cap = (static_cast<uint16_t>(base_flag) & 0x4) != 0;
        uint16_t val = is_cap ? 12 : 8;
        switch (promo) {
            case PieceType::Knight: val += 0; break;
            case PieceType::Bishop: val += 1; break;
            case PieceType::Rook:   val += 2; break;
            case PieceType::Queen:  val += 3; break;
            default: return base_flag;
        }
        return static_cast<MoveFlag>(val);
    }

    uint32_t data_{0};
};

class MoveList {
public:
    static constexpr size_t MAX_MOVES = 256;

    constexpr MoveList() noexcept : size_(0) {}

    constexpr void push_back(Move move) noexcept {
        if (size_ < MAX_MOVES) {
            moves_[size_++] = move;
        }
    }

    constexpr Move& operator[](size_t idx) noexcept { return moves_[idx]; }
    constexpr const Move& operator[](size_t idx) const noexcept { return moves_[idx]; }

    constexpr size_t size() const noexcept { return size_; }
    constexpr bool empty() const noexcept { return size_ == 0; }
    constexpr void clear() noexcept { size_ = 0; }

    constexpr Move* begin() noexcept { return moves_.data(); }
    constexpr Move* end() noexcept { return moves_.data() + size_; }
    constexpr const Move* begin() const noexcept { return moves_.data(); }
    constexpr const Move* end() const noexcept { return moves_.data() + size_; }

    constexpr bool contains(Move move) const noexcept {
        for (size_t i = 0; i < size_; ++i) {
            if (moves_[i] == move) return true;
        }
        return false;
    }

private:
    std::array<Move, MAX_MOVES> moves_{};
    size_t size_{0};
};

} // namespace chess
