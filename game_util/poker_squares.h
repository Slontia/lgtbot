// Copyright (c) 2024-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#ifdef ENUM_BEGIN
#ifdef ENUM_MEMBER
#ifdef ENUM_END

ENUM_BEGIN(GridType)
ENUM_MEMBER(GridType, empty)
ENUM_MEMBER(GridType, black_bonus)
ENUM_MEMBER(GridType, red_bonus)
ENUM_MEMBER(GridType, double_score)
ENUM_MEMBER(GridType, skip_next)
ENUM_MEMBER(GridType, card)
ENUM_END(GridType)

#endif
#endif
#endif

#ifndef POKER_SQUARES_H_
#define POKER_SQUARES_H_

#include "game_util/poker.h"
#include "utility/coordinate.h"
#include "utility/util_func.h"

#include <array>
#include <variant>

namespace lgtbot {

namespace game_util {

namespace poker_squares {

#define ENUM_FILE "../game_util/poker_squares.h"
#include "../utility/extend_enum.h"

constexpr poker::CardType k_card_type = poker::CardType::POKER;
using Card = poker::Card<k_card_type>;
using OptionalDeck = poker::OptionalDeck<k_card_type>;
using Hand = poker::Hand<k_card_type>;
constexpr auto k_card_num = poker::k_card_num<k_card_type>;

constexpr int32_t k_square_size = 5;
constexpr int32_t k_group_num = k_square_size * 2 + 2;
constexpr int32_t k_grid_num = k_square_size * k_square_size;

struct Statistic
{
    int32_t pattern_score_{0};
    int32_t bonus_score_{0};
    poker::PatternType::BitSet pattern_type_bitset_;
    bool skip_next_{false};
};

inline int32_t GetScore(const Statistic& statistic)
{
    constexpr int32_t k_pattern_type_count_base = 5;
    int32_t score = statistic.pattern_score_ + statistic.bonus_score_;
    const auto pattern_type_count = statistic.pattern_type_bitset_.count() - statistic.pattern_type_bitset_[poker::PatternType::HIGH_CARD];
    if (pattern_type_count >= k_pattern_type_count_base) {
        score += 100 * (1 << (pattern_type_count - k_pattern_type_count_base));
    }
    return score;
}

static std::array<char, 3> GetCardImgName(const Card card)
{
    constexpr char k_suit_chars[] = {
        [poker::PokerSuit(poker::PokerSuit::CLUBS).ToUInt()] = 'c',
        [poker::PokerSuit(poker::PokerSuit::DIAMONDS).ToUInt()] = 'd',
        [poker::PokerSuit(poker::PokerSuit::HEARTS).ToUInt()] = 'h',
        [poker::PokerSuit(poker::PokerSuit::SPADES).ToUInt()] = 's',
    };
    return std::array<char, 3>{
        k_suit_chars[card.suit_.ToUInt()],
        card.number_.ToString()[1], // the first character is '_'
        '\0',
    };
}

static constexpr int32_t GetNumberPoint(const poker::PokerNumber number) {
    return 1 + (number > poker::PokerNumber::_6) + (number > poker::PokerNumber::_10) + (number == poker::PokerNumber::_A) * 2;
}

static constexpr int32_t GetPatternPoint(const poker::PatternType type) {
    constexpr int k_points[] = {
        [poker::PatternType(poker::PatternType::HIGH_CARD).ToUInt()] = 0,
        [poker::PatternType(poker::PatternType::ONE_PAIR).ToUInt()] = 1,
        [poker::PatternType(poker::PatternType::TWO_PAIRS).ToUInt()] = 3,
        [poker::PatternType(poker::PatternType::THREE_OF_A_KIND).ToUInt()] = 6,
        [poker::PatternType(poker::PatternType::STRAIGHT).ToUInt()] = 12,
        [poker::PatternType(poker::PatternType::FLUSH).ToUInt()] = 5,
        [poker::PatternType(poker::PatternType::FULL_HOUSE).ToUInt()] = 10,
        [poker::PatternType(poker::PatternType::FOUR_OF_A_KIND).ToUInt()] = 16,
        [poker::PatternType(poker::PatternType::STRAIGHT_FLUSH).ToUInt()] = 30,
    };
    static_assert(sizeof(k_points) / sizeof(k_points[0]) == poker::PatternType::Count());
    return k_points[type.ToUInt()];
}

class PokerSquare
{
    class Grid;

    struct Multiple
    {
        int value_{1};
    };

    struct Group
    {
      public:
        using Grids = std::array<std::reference_wrapper<Grid>, k_square_size>;

        Group(const Multiple basic_multiple, html::Box& point_multiple_html_box, html::Box& pattern_score_html_box)
            : basic_multiple_{basic_multiple}, point_multiple_html_box_{point_multiple_html_box}, pattern_score_html_box_{pattern_score_html_box}
        {
            point_multiple_html_box_.SetContent(MakePointMultipleInfo_(0, 0, basic_multiple_, false));
        }

        void Fill(const Card card, const Multiple extra_multiple, Statistic& statistic)
        {
            if (!hand_.Add(card)) {
                // A repeated card is filled, which is unexpected.
                assert(false);
                return;
            }
            number_point_ += GetNumberPoint(card.number_);
            if (++filled_count_ < k_square_size) {
                point_multiple_html_box_.SetContent(MakePointMultipleInfo_(number_point_, 0, basic_multiple_, false));
                return;
            }
            // All grids are filled with cards.
            const OptionalDeck deck = hand_.BestDeck();
            assert(deck.has_value());
            const int32_t pattern_point = GetPatternPoint(deck->type_);
            const Multiple multiple{basic_multiple_.value_ + extra_multiple.value_};
            const int32_t score = number_point_ * pattern_point * (basic_multiple_.value_ + extra_multiple.value_);
            statistic.pattern_type_bitset_[deck->type_] = true;
            statistic.pattern_score_ += score;
            point_multiple_html_box_.SetContent(MakePointMultipleInfo_(number_point_, pattern_point, multiple, true));
            if (pattern_point == 0) {
                pattern_score_html_box_.SetContent(
                        std::string("<font size=\"3\"><span style=\"background-color:#d3d3d3; text-align:center;\">" HTML_ESCAPE_SPACE) +
                        GetPatternTypeName(deck->type_) +
                        HTML_ESCAPE_SPACE "</span></font><br>" HTML_COLOR_FONT_HEADER(grey) HTML_SIZE_FONT_HEADER(5) "-" HTML_FONT_TAIL HTML_FONT_TAIL);
            } else if (pattern_point < 10) {
                pattern_score_html_box_.SetContent(
                        std::string("<font size=\"3\"><span style=\"background-color:#f3b13d; text-align:center;\">" HTML_ESCAPE_SPACE) +
                        GetPatternTypeName(deck->type_) +
                        HTML_ESCAPE_SPACE "</span></font><br>" HTML_SIZE_FONT_HEADER(5) + std::to_string(score) + HTML_FONT_TAIL);
            } else {
                pattern_score_html_box_.SetContent(
                        std::string("<font size=\"3\"><span style=\"background-color:#c5b4e3; text-align:center;\">" HTML_ESCAPE_SPACE) +
                        GetPatternTypeName(deck->type_) +
                        HTML_ESCAPE_SPACE "</span></font><br>" HTML_SIZE_FONT_HEADER(5) + std::to_string(score) + HTML_FONT_TAIL);
            }
        }

        OptionalDeck GetOptionalDeck() const { return hand_.BestDeck(); }

      private:
        Multiple basic_multiple_{1};
        html::Box& point_multiple_html_box_;
        html::Box& pattern_score_html_box_;
        int32_t filled_count_{0};
        int32_t number_point_{0};
        Hand hand_;
    };

    class Grid
    {
      public:
        using Groups = std::array<Group*, 4>;

        Grid(const GridType type, const Groups& groups) : type_{type}, groups_{groups} {}

        bool Fill(const Card card, Statistic& statistic)
        {
            if (type_ == GridType::card) {
                return false;
            }
            const Multiple extra_multiple{type_ == GridType::double_score};
            statistic.skip_next_ = type_ == GridType::skip_next;
            std::ranges::for_each(groups_ | std::views::filter([](Group* const group) { return group != nullptr; }),
                    [&](Group* const group) { group->Fill(card, extra_multiple, statistic); });
            if (const bool is_red_card = card.suit_ == poker::PokerSuit::HEARTS || card.suit_ == poker::PokerSuit::DIAMONDS;
                    (type_ == GridType::red_bonus && is_red_card) || (type_ == GridType::black_bonus && !is_red_card)) {
                constexpr int32_t k_bonus_score_base = 20;
                statistic.bonus_score_ += GetNumberPoint(card.number_) * k_bonus_score_base;
            }
            type_ = GridType::card;
            return true;
        }

        bool IsFilled() const { return type_ == GridType::card; }

      private:
        GridType type_;
        Groups groups_; // the grid which maximum groups contain is the central grid
    };

  public:
    PokerSquare(std::string image_path, const std::array<GridType, k_grid_num>& grid_types)
        : PokerSquare(std::move(image_path), MakeArray<k_group_num>(&MakeGroupDesc_), grid_types)
    {
    }

    bool Fill(const int32_t index, const Card card)
    {
        if (!grids_[index].Fill(card, statistic_)) {
            return false;
        }
        UpdateBoxHtml_(index, card);
        return true;
    }

    // requires: an empty grid exists
    int32_t FillInOrder(const Card card)
    {
        int32_t index = 0;
        std::ranges::any_of(std::views::iota(0, k_grid_num), [&](const int i) { return index = i, Fill(i, card); });
        return index;
    }

    // requires: an empty grid exists
    int32_t FillRandomly(const Card card)
    {
        std::vector<int32_t> empty_positions;
        for (int32_t i = 0; i < k_grid_num; ++i) {
            if (!grids_[i].IsFilled()) {
                empty_positions.emplace_back(i);
            }
        }
        const int32_t index = empty_positions[rand() % empty_positions.size()];
        if (!Fill(index, card)) {
            assert(false);
        }
        return index;
    }

    bool ResetSkipNext() { return std::exchange(statistic_.skip_next_, false); }

    std::string ToHtml() const
    {
        return "### " HTML_COLOR_FONT_HEADER(green) "当前积分：" + std::to_string(GetScore(statistic_)) +
            HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE "非高牌牌型：" +
            std::to_string(statistic_.pattern_type_bitset_.count() - statistic_.pattern_type_bitset_[poker::PatternType::HIGH_CARD]) +
            (statistic_.skip_next_ ? HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE HTML_COLOR_FONT_HEADER(red) "（过牌中）" : "") +
            "\n\n" + html_table_.ToString();
    }

    const Statistic& GetStatistic() const { return statistic_; }

    auto GetDecks() const
    {
        return groups_ | std::views::transform([](const Group& group) { return group.GetOptionalDeck(); })
                       | std::views::filter([](const OptionalDeck& optional_deck) { return optional_deck.has_value(); })
                       | std::views::transform([](const OptionalDeck& optional_deck) { return *optional_deck; });
    }

  private:
    struct GroupDesc
    {
        Multiple multiple_;
        Coordinate html_point_multiple_coor_;
        Coordinate html_pattern_score_coor_;
        std::array<Coordinate, k_square_size> grid_coors_;
    };

    static constexpr const char* k_background_image_names[] = {
        "b_2", "b_0", "b_0", "b_0", "b_1",
        "b_0", "b_2", "b_0", "b_1", "b_0",
        "b_0", "b_0", "b_3", "b_0", "b_0",
        "b_0", "b_1", "b_0", "b_2", "b_0",
        "b_1", "b_0", "b_0", "b_0", "b_2",
    };

    static size_t ToIndex_(const Coordinate coor) { return coor.row_ * k_square_size + coor.col_; }

    static std::string MakePointMultipleInfo_(const int32_t number_point, const int32_t pattern_point, const Multiple multiple,
            const bool ready)
    {
        std::string result = "<div align=\"left\" style=\"height:100\% text-valign:middle\">";
        result += !ready ? HTML_COLOR_FONT_HEADER(black) :
                  pattern_point == 0 ? HTML_COLOR_FONT_HEADER(grey) : HTML_COLOR_FONT_HEADER(blue);
        result += HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE "点 ";
        result += std::to_string(number_point);
        if (ready) {
            result += "<br>" HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE "型 ";
            result += std::to_string(pattern_point);
        }
        if (multiple.value_ > 1) {
            result += "<br>" HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE "倍 ";
            result += std::to_string(multiple.value_);
        }
        result += "</div>";
        return result;
    }

    static constexpr GroupDesc MakeGroupDesc_(int32_t group_index)
    {
        if (group_index < k_square_size) { // a row group
            return GroupDesc{
                .multiple_{1},
                .html_point_multiple_coor_{group_index + 1, 0},
                .html_pattern_score_coor_{group_index + 1, k_square_size + 1},
                .grid_coors_ = MakeArray<k_square_size>(
                        [=](const int n) constexpr { return Coordinate{static_cast<int32_t>(group_index), n}; }),
            };
        } else if (group_index < k_square_size * 2) { // a column group
            return GroupDesc{
                .multiple_{1},
                .html_point_multiple_coor_{0, group_index - k_square_size + 1},
                .html_pattern_score_coor_{k_square_size + 1, group_index - k_square_size + 1},
                .grid_coors_ = MakeArray<k_square_size>(
                        [=](const int n) constexpr { return Coordinate{n, static_cast<int32_t>(group_index - k_square_size)}; }),
            };
        } else if (group_index == k_square_size * 2) {
            return GroupDesc{
                .multiple_{2},
                .html_point_multiple_coor_{0, 0},
                .html_pattern_score_coor_{k_square_size + 1, k_square_size + 1},
                .grid_coors_ = MakeArray<k_square_size>([=](const int n) constexpr { return Coordinate{n, n}; }),
            };
        } else if (k_square_size * 2 + 1) {
            return GroupDesc{
                .multiple_{2},
                .html_point_multiple_coor_{k_square_size + 1, 0},
                .html_pattern_score_coor_{0, k_square_size + 1},
                .grid_coors_ = MakeArray<k_square_size>(
                        [=](const int n) constexpr { return Coordinate{n, static_cast<int32_t>(k_square_size - 1 - n)}; }),
            };
        } else {
            // unexpected
            return {};
        }
    }

    PokerSquare(std::string&& image_path, const std::array<GroupDesc, k_group_num>& group_descs, const std::array<GridType, k_grid_num>& grid_types)
        : image_path_{std::move(image_path)}
        , groups_(MakeGroups_(group_descs))
        , grids_(MakeGrids_(group_descs, grid_types))
    {
        // Initialize html table.
        html_table_.SetTableStyle("style=\"table-layout: fixed;\" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" width=448px ");
        html_table_.SetRowStyle("align=\"center\" valign=\"middle\" height=64px ");
        for (size_t i = 0; i < k_grid_num; ++i) {
            const auto grid_type = grid_types[i];
            const std::string html_head =
                grid_type == GridType::red_bonus    ? HTML_COLOR_FONT_HEADER(red)     :
                grid_type == GridType::double_score ? HTML_COLOR_FONT_HEADER(#b1a637) :
                grid_type == GridType::skip_next    ? HTML_COLOR_FONT_HEADER(#db3af7) :
                grid_type == GridType::empty        ? HTML_COLOR_FONT_HEADER(#4046a8) :
                                                      HTML_COLOR_FONT_HEADER(black);
            GetHtmlBox_(i).SetContent(Image_(k_background_image_names[i], grid_types[i].ToString(),
                        html_head + HTML_SIZE_FONT_HEADER(5) + static_cast<char>('a' + i) + HTML_FONT_TAIL HTML_FONT_TAIL));
        }
    }

    std::array<Group, k_group_num> MakeGroups_(const std::array<GroupDesc, k_group_num>& group_descs)
    {
        return MakeArray<k_group_num>([&](const int group_index)
                {
                    auto& group_desc = group_descs[group_index];
                    return Group{
                        group_desc.multiple_,
                        html_table_.Get(group_desc.html_point_multiple_coor_.row_, group_desc.html_point_multiple_coor_.col_),
                        html_table_.Get(group_desc.html_pattern_score_coor_.row_, group_desc.html_pattern_score_coor_.col_),
                    };
                });
    }

    std::array<Grid, k_grid_num> MakeGrids_(const std::array<GroupDesc, k_group_num>& group_descs, const std::array<GridType, k_grid_num>& grid_types)
    {
        class GroupPointersForGrid
        {
          public:
            void AppendGroup(Group& group) { group_ptrs_[group_num_++] = &group; }

            const Grid::Groups& GetGroupPointers() const { return group_ptrs_; }

          private:
            Grid::Groups group_ptrs_{nullptr};
            int group_num_{0};
        };

        const auto group_pointers_for_grids = [&]()
            {
                std::array<GroupPointersForGrid, k_grid_num> result;
                for (size_t i = 0; i < k_group_num; ++i) {
                    for (const Coordinate coor : group_descs[i].grid_coors_) {
                        result[ToIndex_(coor)].AppendGroup(groups_[i]);
                    }
                }
                return result;
            }();

        return MakeArray<k_grid_num>([&](const int grid_index)
                {
                    return Grid{grid_types[grid_index], group_pointers_for_grids[grid_index].GetGroupPointers()};
                });
    }

    void UpdateBoxHtml_(const int32_t index, const Card card)
    {
        GetHtmlBox_(index).SetContent(Image_(k_background_image_names[index], GetCardImgName(card).data()));
    }

    html::Box& GetHtmlBox_(const size_t index)
    {
        return html_table_.Get(index / k_square_size + 1, index % k_square_size + 1);
    }

    std::string Image_(const std::string_view background_name, const std::string_view name, const std::string_view text = "") const
    {
        return BackgroundImageDiv_(background_name) + BackgroundImageDiv_(name) + text.data() + "</div></div>";
    }

    std::string BackgroundImageDiv_(const std::string_view name) const
    {
        return "<div style=\"background:url('file:///" + image_path_ + name.data() + ".png') no-repeat; width:64px; height:64px; line-height:64px;\"> ";
    }

    std::string image_path_;
    html::Table html_table_{k_square_size + 2, k_square_size + 2};
    Statistic statistic_;
    std::array<Group, k_group_num> groups_;
    std::array<Grid, k_grid_num> grids_;
};

}

}

}

#endif
