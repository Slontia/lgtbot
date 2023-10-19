// Copyright (c) 2023-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <optional>
#include <vector>
#include <string>
#include <algorithm>

#include "utility/coordinate.h"
#include "utility/html.h"

namespace lgtbot {

namespace game_util {

namespace unity_chess {

constexpr const uint32_t k_max_player = 3;

struct SettlementResult
{
    std::array<uint32_t, k_max_player> color_counts_{0};
    uint32_t available_area_count_ = 0;
    std::string html_;
};

class Board
{
    struct Area
    {
        bool HasChess(const uint32_t player_id) const { return chess_flags_ & (1 << player_id); }

        bool HasBackground(const uint32_t player_id) const { return background_flags_ & (1 << player_id); }

        int32_t ColorCount(const uint32_t player_id) const
        {
            return background_flags_ == (1 << k_max_player) - 1 ? 0 :
                   background_flags_ == (1 << player_id)        ? 2 :
                   HasBackground(player_id)                     ? 1 : 0;
        }

        bool CanBeSetChess() const { return !is_bonus_ && !chess_flags_; }

        void SetChess(const uint32_t player_id) { chess_flags_ |= (1 << player_id); }

        void SetBackground(const uint32_t player_id) {
            if (is_covering_) {
                background_flags_ |= (1 << player_id);
            } else {
                is_covering_ = true;
                background_flags_ = (1 << player_id);
            }
        }

        bool is_covering_ : 1 = false; // the background is being updated this round
        bool is_bonus_ : 1 = false;
        uint8_t chess_flags_ : k_max_player = 0;
        uint8_t background_flags_ : k_max_player = 0;
    };
    static_assert(sizeof(Area) == 1);

  public:
    Board(const uint32_t size, std::vector<Coordinate> bounus_coordinates, std::string image_path)
        : size_(size), board_(size * size), image_path_(std::move(image_path))
    {
        for (const auto& coordinate : bounus_coordinates) {
            GetArea_(coordinate).is_bonus_ = true;
        }
    }

    bool Set(const Coordinate coordinate, const uint32_t player_id)
    {
        assert(player_id < k_max_player);
        if (!IsValidCoordinate_(coordinate) || !GetArea_(coordinate).CanBeSetChess()) {
            return false;
        }
        player_selections_[player_id] = coordinate;
        return true;
    }

    void RandomSet(const uint32_t player_id)
    {
        auto available_count = rand() % std::ranges::count_if(board_, &Area::CanBeSetChess);
        for (int32_t row = 0; row < size_; ++row) {
            for (int32_t col = 0; col < size_; ++col) {
                const Coordinate coordinate(row, col);
                if (GetArea_(coordinate).CanBeSetChess() && available_count-- == 0) {
                    player_selections_[player_id] = coordinate;
                }
            }
        }
    }

    SettlementResult Settlement()
    {
        ClearMark_();
        for (uint32_t player_id = 0; player_id < player_selections_.size(); ++player_id) {
            SettlementPlayer_(player_id);
        }
        const auto result = MakeSettlementResult_();
        for (auto& selection : player_selections_) {
            selection = std::nullopt;
        }
        return result;
    }

  private:
    Area& GetArea_(const Coordinate coordinate) { return board_[coordinate.row_ * size_ + coordinate.col_]; }
    const Area& GetArea_(const Coordinate coordinate) const { return board_[coordinate.row_ * size_ + coordinate.col_]; }

    void ClearMark_()
    {
        for (auto& area : board_) {
            area.is_covering_ = false;
        }
    }

    void SettlementPlayer_(const uint32_t player_id)
    {
        if (!player_selections_[player_id].has_value()) {
            return;
        }
        const Coordinate coordinate = *player_selections_[player_id];
        GetArea_(coordinate).SetChess(player_id);
        for (const auto& direction : {Coordinate{0, 1}, Coordinate{1, 0}, Coordinate{1, 1}, Coordinate{-1, 1}}) {
            for (int32_t i = 0; i < k_consecutive_length_; ++i) {
                const Coordinate begin_coordinate = coordinate - direction * i;
                if (CanColor_(begin_coordinate, direction, player_id)) {
                    ColorAround_(begin_coordinate + direction, player_id);
                }
            }
        }
    }

    bool CanColor_(Coordinate coordinate, const Coordinate direction, const uint32_t player_id) const
    {
        for (int32_t offset = 0; offset < k_consecutive_length_; ++offset) {
            const Coordinate current_coordinate = coordinate + direction * offset;
            if (!IsValidCoordinate_(current_coordinate) || !(GetArea_(current_coordinate).HasChess(player_id))) {
                return false;
            }
        }
        return true;
    }

    void ColorAround_(const Coordinate coordinate, const uint32_t player_id)
    {
        for (int32_t row = coordinate.row_ - 1; row <= coordinate.row_ + 1; ++row) {
            for (int32_t col = coordinate.col_ - 1; col <= coordinate.col_ + 1; ++col) {
                const Coordinate coordinate = Coordinate{row, col};
                if (IsValidCoordinate_(coordinate)) {
                    GetArea_(coordinate).SetBackground(player_id);
                }
            }
        }
    }

    bool IsValidCoordinate_(const Coordinate coordinate) const
    {
        return 0 <= coordinate.row_ && 0 <= coordinate.col_ && coordinate.row_ < size_ && coordinate.col_ < size_;
    }

    SettlementResult MakeSettlementResult_() const
    {
        SettlementResult result {
            .html_ = ToHtml_()
        };
        constexpr const int32_t k_bonus = 2;
        for (const auto area : board_) {
            result.available_area_count_ += area.CanBeSetChess();
            for (uint32_t player_id = 0; player_id < player_selections_.size(); ++player_id) {
                result.color_counts_[player_id] += area.ColorCount(player_id) * (1 + area.is_bonus_ * k_bonus);
            }
        }
        return result;
    }

    std::string AreaImage_(const Coordinate coordinate, const bool mark) const
    {
        const Area& area = GetArea_(coordinate);
        std::string s = "![](file:///" + image_path_ + "/" + std::to_string(mark) + "_";
        if (area.is_bonus_) {
            s += "3";
        } else {
            for (uint32_t player_id = 0; player_id < player_selections_.size(); ++player_id) {
                s += std::to_string(area.HasChess(player_id));
            }
        }
        s += "_";
        for (uint32_t player_id = 0; player_id < player_selections_.size(); ++player_id) {
            s += std::to_string(area.HasBackground(player_id));
        }
        s += ".png)";
        return s;
    }

    std::string ToHtml_() const
    {
        html::Table table(size_ + 2, size_ + 2);
        table.SetTableStyle(" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" ");
        for (uint32_t i = 0; i < size_; ++i) {
            table.Get(0, i + 1).SetContent(HTML_SIZE_FONT_HEADER(3) + std::to_string(i));
            table.Get(size_ + 1, i + 1).SetContent(HTML_SIZE_FONT_HEADER(3) + std::to_string(i));
            table.Get(i + 1, 0).SetContent(HTML_SIZE_FONT_HEADER(3) + std::string(1, 'A' + i));
            table.Get(i + 1, size_ + 1).SetContent(HTML_SIZE_FONT_HEADER(3) + std::string(1, 'A' + i));
        }
        for (uint32_t row = 0; row < size_; ++row) {
            for (uint32_t col = 0; col < size_; ++col) {
                table.Get(row + 1, col + 1).SetContent(AreaImage_(Coordinate(row, col), false));
            }
        }
        for (const auto& selection : player_selections_) {
            if (selection.has_value()) {
                table.Get(selection->row_ + 1, selection->col_ + 1).SetContent(AreaImage_(*selection, true));
            }
        }
        return table.ToString();
    }

    constexpr static const int32_t k_consecutive_length_ = 3;
    uint32_t size_;
    std::vector<Area> board_;
    std::array<std::optional<Coordinate>, k_max_player> player_selections_;
    std::string image_path_;
};

} // namespace unity_chess

} // namespace game_util

} // namespace lgtbot
