// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <optional>
#include <array>
#include <ranges>
#include <algorithm>
#include <variant>
#include <iostream>

#include "../utility/html.h"

namespace lgtbot {

namespace game_util {

namespace alchemist {

enum class Color : char { RED = 'r', ORANGE = 'o', YELLOW = 'y', BLUE = 'b', GREY = 'g', PURPLE = 'p', };

enum class Point : char { ONE = '1', TWO = '2', THREE = '3', FOUR = '4', FIVE = '5', SIX = '6' };

class Card
{
  public:
    Card(const Color color, const Point point) : color_(color), point_(point) {}

    int Score() const { return static_cast<char>(point_) - '0'; }

    bool CanBeAdj(const Card card) const { return color_ == card.color_ || point_ == card.point_; }

    std::string ImageName() const
    {
        return std::string("card_") + static_cast<char>(color_) + static_cast<char>(point_);
    }

  private:
    Color color_;
    Point point_;
};

struct Stone
{
    int Score() const { return 10; }
    bool CanBeAdj(const Card card) const { return true; }
};

enum : int { FAIL_ALREADY_SET = -1, FAIL_ADJ_CARDS_MISMATCH = -2, FAIL_NON_ADJ_CARDS = -3, };

struct AreaStatistic
{
    int32_t card_count_ = 0;
    int32_t stone_count_ = 0;
};

class Board
{
  public:
    static constexpr const uint32_t k_size = 5;

    Board(std::string image_path, const int style = false) : image_path_(std::move(image_path)), style_(style), table_(k_size + 1, k_size + 1)
    {
        table_.SetTableStyle(" align=\"center\" cellpadding=\"1\" cellspacing=\"1\" ");
        table_.Get(0, 0).SetContent(Image_("coor_corner_" + std::to_string(style)));
        for (uint32_t i = 0; i < k_size; ++i) {
            table_.Get(0, i + 1).SetContent(Image_("coor_" + std::to_string(i + 1) + "_" + std::to_string(style)));
            table_.Get(i + 1, 0).SetContent(Image_(std::string("coor_") + static_cast<char>('a' + i) + "_" + std::to_string(style)));
            for (uint32_t j = 0; j < k_size; ++j) {
                table_.Get(i + 1, j + 1).SetContent(Image_("empty_" + std::to_string(style)));
            }
        }
    }

    void SetStone(const uint32_t row, const uint32_t col)
    {
        areas_[row][col] = Stone();
        table_.Get(row + 1, col + 1).SetContent(Image_("stone_" + std::to_string(style_)));
    }

    // If return -1, means the position is invalid
    // If return 0, means the card is set and does not get score
    // If return a positive number, means some areas are cleared and the score is returned
    int SetOrClearLine(const uint32_t row, const uint32_t col, const Card card, const bool cal_point = false)
    {
        if (areas_[row][col].has_value()) {
            return FAIL_ALREADY_SET;
        }
        if (const auto ret = IsAdjCardsOk_(row, col, card); ret != 0) {
            return ret;
        }
        const auto ret = TryClear_(row, col, cal_point ? card.Score() : 0);
        if (ret == 0) {
            areas_[row][col] = card;
            table_.Get(row + 1, col + 1).SetContent(Image_(card.ImageName()));
        }
        return ret;
    }

    bool Unset(const uint32_t row, const uint32_t col)
    {
        auto& area = areas_[row][col];
        if (!area.has_value()) {
            return false;
        }
        area.reset();
        table_.Get(row + 1, col + 1).SetContent(Image_("empty_" + std::to_string(style_)));
        return true;
    }

    AreaStatistic GetAreaStatistic() const
    {
        AreaStatistic result;
        for (const auto& area_line : areas_) {
            for (const auto& area : area_line) {
                if (!area.has_value()) {
                    std::visit([&] (const auto& arg)
                            {
                                using T = std::decay_t<decltype(arg)>;
                                if constexpr (std::is_same_v<T, Card>) {
                                    ++result.card_count_;
                                } else {
                                    ++result.stone_count_;
                                }
                            }, *area);
                }
            }
        }
        return result;
    }

    std::string ToHtml() const { return table_.ToString(); }

  private:
    std::string Image_(std::string name) { return "![](file://" + image_path_ + "/" + std::move(name) + ".png)"; }

    int IsAdjCardsOk_(const uint32_t row, const uint32_t col, const Card card) const
    {
        bool has_adj = false;
        const auto check = [&](const uint32_t adj_row, const uint32_t adj_col) -> bool
            {
                if (!(0 <= adj_row && adj_row < k_size && 0 <= adj_col && adj_col < k_size))
                {
                    return true; // is not valid position, skip check
                }
                const auto& adj_card = areas_[adj_row][adj_col];
                if (!adj_card.has_value()) {
                    return true; // there is not a card on this area, skip check
                }
                has_adj = true;
                return std::visit([&card](const auto& c) { return c.CanBeAdj(card); }, *adj_card);
            };
        if (!(check(row + 1, col) && check(row - 1, col) && check(row, col + 1) && check(row, col - 1))) {
            return FAIL_ADJ_CARDS_MISMATCH;
        } else if (!has_adj) {
            return FAIL_NON_ADJ_CARDS;
        } else {
            return 0;
        }
    }

    int TryClear_(const uint32_t row, const uint32_t col, const int junction_score)
    {
        const auto try_clear = [&](const auto skip, const auto& transform)
            {
                auto cards = std::views::iota(0)
                    | std::views::take(k_size)
                    | std::views::filter([&](const uint32_t n) { return n != skip; })
                    | std::views::transform(transform);
                if (std::ranges::all_of(cards, [&](const auto& c) { return areas_[c.first][c.second].has_value(); })) {
                    int score = 0;
                    std::ranges::for_each(cards, [&](const auto& c)
                            {
                                auto& area = areas_[c.first][c.second];
                                std::visit([&score](const auto& area) { score += area.Score(); }, *area);
                                area.reset();
                                table_.Get(c.first + 1, c.second + 1).SetContent(Image_("empty_" + std::to_string(style_)));
                            });
                    return junction_score == 0 ? 1 : score + junction_score;
                }
                return 0;
            };
        return (try_clear(col, [&](const uint32_t n) { return std::pair{row, n}; })) * 2
            + (try_clear(row, [&](const uint32_t n) { return std::pair{n, col}; })) * 2
            + (row != col ? 0 : try_clear(col, [&](const uint32_t n) { return std::pair{n, n}; }) * 3)
            + (row + col != k_size - 1 ? 0 : try_clear(col, [&](const uint32_t n) { return std::pair{k_size - 1 - n, n}; }) * 3);
    }

    const std::string image_path_;
    const int style_;
    std::array<std::array<std::optional<std::variant<Card, Stone>>, k_size>, k_size> areas_;
    html::Table table_;
};

} // namespace alchemist

} // namespace game_util

} // namespace lgtbot
