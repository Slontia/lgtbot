#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include <span>
#include <algorithm>

#include "utility/html.h"

namespace lgtbot {

namespace game_util {

namespace othello {

enum class ChessType { BLACK = 0, WHITE = 1, CRASH = 2, NONE = 3 };

enum class VariationType { PLACE, REVERSE, NONE };

struct Box
{
    ChessType cur_type_ = ChessType::NONE;
    ChessType next_type_ = ChessType::NONE;
    VariationType variation_type_ = VariationType::NONE;
};

using Statistic = std::array<int, 4>;

struct Coor
{
    int32_t row_;
    int32_t col_;

    Coor& operator+=(const Coor& o)
    {
        row_ += o.row_;
        col_ += o.col_;
        return *this;
    }

    Coor operator+(const Coor& o) const
    {
        return Coor{*this} += o;
    }

    auto operator<=>(const Coor&) const = default;
};

static std::array<Coor, 8> g_directions
{
    Coor{-1, -1}, Coor{-1, 0}, Coor{-1, 1}, Coor{0, -1}, Coor{0, 1}, Coor{1, -1}, Coor{1, 0}, Coor{1, 1}
};

class Board
{
  public:
    Board(std::string image_path, const std::span<const std::pair<Coor, ChessType>> init_chesses = std::array {
                std::pair{Coor{k_size_ / 2    , k_size_ / 2    }, ChessType::BLACK},
                std::pair{Coor{k_size_ / 2 - 1, k_size_ / 2 - 1}, ChessType::BLACK},
                std::pair{Coor{k_size_ / 2 - 1, k_size_ / 2    }, ChessType::WHITE},
                std::pair{Coor{k_size_ / 2    , k_size_ / 2 - 1}, ChessType::WHITE},
            })
        : image_path_(std::move(image_path))
    {
        for (const auto& [coor, type] : init_chesses) {
            auto& box = Get_(coor);
            box.cur_type_ = box.next_type_ = type;
        }
    }

    bool Place(const Coor& coor, const ChessType type)
    {
        if (coor.row_ < 0 || coor.row_ >= k_size_ || coor.col_ < 0 || coor.col_ >= k_size_) {
            return false;
        }
        auto& box = Get_(coor);
        if (box.cur_type_ != ChessType::NONE) {
            return false; // there is already a chess
        }
        bool reversed = false;
        for (const auto& direction : g_directions) {
            if (CouldReverseOneDirection_(coor, direction, type)) {
                ReverseOneDirection_(coor, direction, type);
                reversed = true;
            }
        }
        if (reversed) {
            box.next_type_ = box.next_type_ == ChessType::NONE ? type : ChessType::CRASH;
        }
        return reversed;
    }

    std::vector<Coor> PlacablePositions(const ChessType type) const
    {
        std::vector<Coor> ret;
        for (int32_t row = 0; row < k_size_; ++row) {
            for (int32_t col = 0; col < k_size_; ++col) {
                const Coor coor{row, col};
                if (IsPlacable_(coor, type)) {
                    ret.emplace_back(coor);
                }
            }
        }
        return ret;
    }

    Statistic Settlement()
    {
        Statistic statistic{0};
        for (auto& row_boxes : boxes_) {
            for (auto& box : row_boxes) {
                RefreshBox_(box, statistic);
            }
        }
        return statistic;
    }

    std::string ToHtml() const
    {
        html::Table table(k_size_ + 2, k_size_ + 2);
        table.SetTableStyle(" align=\"center\" cellpadding=\"2\" cellspacing=\"0\"");
        for (int32_t i = 0; i < k_size_; ++i) {
            table.Get(0, i + 1).SetContent(std::string(1, 'A' + i));
            table.Get(k_size_ + 1, i + 1).SetContent(std::string(1, 'A' + i));
            table.Get(i + 1, 0).SetContent(std::to_string(i));
            table.Get(i + 1, k_size_ + 1).SetContent(std::to_string(i));
        }
        for (int32_t row = 0; row < k_size_; ++row) {
            for (int32_t col = 0; col < k_size_; ++col) {
                FillHtmlTableBox_(Coor{row, col}, table);
            }
        }
        return "<style>html,body{color:#8fd59c; background:#217844;}</style>\n" + table.ToString();
    }

    std::string ToString() const
    {
        static const char* const k_chess_type_2_char = "102_";
        std::string ret(k_size_ * k_size_, 0);
        for (int32_t row = 0; row < k_size_; ++row) {
            for (int32_t col = 0; col < k_size_; ++col) {
                ret[row * k_size_ + col] = k_chess_type_2_char[static_cast<uint8_t>(Get_(Coor{row, col}).cur_type_)];
            }
        }
        return ret;
    }

  private:
    static void RefreshBox_(Box& box, Statistic& statistic)
    {
        if (box.cur_type_ == ChessType::NONE && box.next_type_ != ChessType::NONE) {
            box.variation_type_ = VariationType::PLACE;
        } else if (box.cur_type_ != box.next_type_) {
            box.variation_type_ = VariationType::REVERSE;
        } else {
            box.variation_type_ = VariationType::NONE;
        }
        box.cur_type_ = box.next_type_;
        ++statistic[static_cast<uint8_t>(box.cur_type_)];
    }

    bool IsValid_(const Coor& coor) const
    {
        return 0 <= coor.row_ && coor.row_ < k_size_ && 0 <= coor.col_ && coor.col_ < k_size_;
    }

    Box& Get_(const Coor& coor) { return boxes_[coor.row_][coor.col_]; }
    const Box& Get_(const Coor& coor) const { return boxes_[coor.row_][coor.col_]; }

    bool IsPlacable_(const Coor& coor, const ChessType type) const
    {
        return Get_(coor).cur_type_ == ChessType::NONE &&
            std::ranges::any_of(g_directions,
                    [&](const Coor& direction) { return CouldReverseOneDirection_(coor, direction, type); });
    }

    bool CouldReverseOneDirection_(const Coor& coor, const Coor& direction, const ChessType type) const
    {
        bool can_reverse = false;
        for (Coor cur_coor = coor + direction; IsValid_(cur_coor); cur_coor += direction) {
            auto& box = Get_(cur_coor);
            if (box.cur_type_ == ChessType::NONE) { // reach empty box
                return false;
            }
            if (box.cur_type_ == type) {
                return can_reverse;
            }
            can_reverse = true; // must reverse at least one chess
        }
        return false; // reach the wall
    }

    void ReverseOneDirection_(const Coor& coor, const Coor& direction, const ChessType type)
    {
        for (Coor cur_coor = coor + direction; IsValid_(cur_coor); cur_coor += direction) {
            auto& box = Get_(cur_coor);
            if (box.cur_type_ == ChessType::NONE || box.cur_type_ == type) {
                break;
            }
            box.next_type_ = box.cur_type_ == box.next_type_ ? type : ChessType::CRASH;
        }
    }

    void FillHtmlTableBox_(const Coor& coor, html::Table& table) const
    {
        auto& table_box = table.Get(coor.row_ + 1, coor.col_ + 1);
        auto& box = Get_(coor);
        const auto color = box.variation_type_ == VariationType::PLACE   ? "#a1c837" :
                           box.variation_type_ == VariationType::REVERSE ? "#37c871" : "";
        const auto image = box.cur_type_ == ChessType::BLACK ? "black" :
                           box.cur_type_ == ChessType::WHITE ? "white" :
                           box.cur_type_ == ChessType::CRASH ? "crash" : "none";
        table_box.SetColor(color);
        table_box.SetContent("![](file:///" + image_path_ + "/" + image + ".png)");
    }

    constexpr static int32_t k_size_ = 8;
    constexpr static int32_t k_box_width_ = 40;
    std::string image_path_;
    std::array<std::array<Box, k_size_>, k_size_> boxes_;
};

} // namespace othello

} // namespace game_util

} // namespace lgtbot
