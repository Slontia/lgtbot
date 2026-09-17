// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <algorithm>
#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#ifndef GAME_MODULE_NAME
#error GAME_MODULE_NAME is not defined
#endif

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

// ========== 常量 ==========

static constexpr int k_max_player_num = 3;      // 最大玩家数
static constexpr int k_color_num = 4;           // 卡牌颜色种类数
static constexpr int k_max_point = 12;          // 卡牌最大数字
static constexpr int k_copy_num = 2;            // 每种颜色每个数字的张数
static constexpr int k_deck_size = k_color_num * k_max_point * k_copy_num;   // 牌库总张数
static constexpr int k_init_hand_num = 10;      // 开局发放的手牌数
static constexpr int k_area_num = 2;            // 每位玩家的任务区数量
static constexpr int k_target_score = 3;        // 累计达到该分数直接获胜
static constexpr int k_task_type_num = 5;       // 任务类型种类数

// ========== 卡牌 ==========

// 卡牌颜色，枚举顺序即手牌排序时的颜色优先级
enum class Color : int { RED = 0, BLUE, YELLOW, GREY };

const char* const k_color_names[k_color_num] = { "红", "蓝", "黄", "灰" };
// 卡牌底色
const char* const k_color_backs[k_color_num] = { "#E06C63", "#5B9BD5", "#E5B33C", "#9AA5AD" };

struct Card
{
    Color color_;
    int point_;

    // 先按数字从小到大，数字相同时按颜色顺序（红蓝黄灰）
    bool operator<(const Card& c) const { return point_ != c.point_ ? point_ < c.point_ : color_ < c.color_; }
    bool operator==(const Card& c) const { return color_ == c.color_ && point_ == c.point_; }
};

// 卡牌的文字名，如「红5」
inline std::string CardName(const Card& card)
{
    return std::string(k_color_names[static_cast<int>(card.color_)]) + std::to_string(card.point_);
}

// 牌组的文字名，如「红5蓝6」
inline std::string CardsName(const std::vector<Card>& cards)
{
    std::string str;
    for (const auto& card : cards) {
        str += CardName(card);
    }
    return str;
}

// 解析无分隔符的牌名串，如「红2红4红10」。任一处不合法则返回空
inline std::optional<std::vector<Card>> ParseCards(const std::string& str)
{
    std::vector<Card> cards;
    size_t pos = 0;
    while (pos < str.size()) {
        int color = -1;
        for (int i = 0; i < k_color_num; ++i) {
            const std::string_view name{k_color_names[i]};
            if (str.size() - pos >= name.size() && str.compare(pos, name.size(), name.data(), name.size()) == 0) {
                color = i;
                pos += name.size();
                break;
            }
        }
        if (color == -1) {
            return std::nullopt;
        }
        const size_t digit_begin = pos;
        while (pos < str.size() && str[pos] >= '0' && str[pos] <= '9') {
            ++pos;
        }
        const size_t digit_num = pos - digit_begin;
        if (digit_num == 0 || digit_num > 2 || str[digit_begin] == '0') {
            return std::nullopt;
        }
        const int point = std::stoi(str.substr(digit_begin, digit_num));
        if (point < 1 || point > k_max_point) {
            return std::nullopt;
        }
        cards.emplace_back(Card{static_cast<Color>(color), point});
    }
    return cards.empty() ? std::nullopt : std::optional<std::vector<Card>>{cards};
}

// 生成一整套牌库
inline std::vector<Card> MakeDeck()
{
    std::vector<Card> deck;
    deck.reserve(k_deck_size);
    for (int color = 0; color < k_color_num; ++color) {
        for (int point = 1; point <= k_max_point; ++point) {
            for (int copy = 0; copy < k_copy_num; ++copy) {
                deck.emplace_back(Card{static_cast<Color>(color), point});
            }
        }
    }
    return deck;
}

// ========== 任务 ==========

// 任务类型
enum class TaskType : int { SAME_POINT = 0, CONSECUTIVE, SAME_COLOR, ODD, EVEN };

const char* const k_task_type_names[k_task_type_num] = { "同点", "连续", "同色", "奇数", "偶数" };
const char* const k_task_type_descs[k_task_type_num] = {
    "卡牌数字相同，颜色不限",
    "卡牌数字连续无间断且互不重复，颜色不限",
    "卡牌颜色相同，数字不限",
    "所有卡牌数字均为奇数，颜色不限",
    "所有卡牌数字均为偶数，颜色不限",
};

struct Task
{
    TaskType type_;
    int require_;   // 要求的最少张数

    // 任务的文字名，如「同色4」
    std::string Name() const { return std::string(k_task_type_names[static_cast<int>(type_)]) + std::to_string(require_); }
    const char* Desc() const { return k_task_type_descs[static_cast<int>(type_)]; }
};

// 判定牌组是否符合任务规则（不校验张数要求），空牌组视为符合
inline bool MatchRule(const TaskType type, const std::vector<Card>& cards)
{
    if (cards.empty()) {
        return true;
    }
    switch (type) {
    case TaskType::SAME_POINT:
        for (const auto& card : cards) {
            if (card.point_ != cards.front().point_) {
                return false;
            }
        }
        return true;
    case TaskType::SAME_COLOR:
        for (const auto& card : cards) {
            if (card.color_ != cards.front().color_) {
                return false;
            }
        }
        return true;
    case TaskType::ODD:
        for (const auto& card : cards) {
            if (card.point_ % 2 == 0) {
                return false;
            }
        }
        return true;
    case TaskType::EVEN:
        for (const auto& card : cards) {
            if (card.point_ % 2 != 0) {
                return false;
            }
        }
        return true;
    case TaskType::CONSECUTIVE: {
        std::vector<int> points;
        points.reserve(cards.size());
        for (const auto& card : cards) {
            points.emplace_back(card.point_);
        }
        std::sort(points.begin(), points.end());
        for (size_t i = 1; i < points.size(); ++i) {
            if (points[i] == points[i - 1]) {   // 数字重复即视为间断
                return false;
            }
        }
        return points.back() - points.front() + 1 == static_cast<int>(points.size());
    }
    }
    return false;
}

// 判定牌组是否达标：符合任务规则且张数不少于要求
inline bool IsQualified(const Task& task, const std::vector<Card>& cards)
{
    return static_cast<int>(cards.size()) >= task.require_ && MatchRule(task.type_, cards);
}

// ========== 玩家 ==========

// 回合内的四个步骤
enum class Step : int { DRAW = 0, TASK, CHAIN, DISCARD };

const char* const k_step_names[4] = { "抽牌阶段", "任务阶段", "连出阶段", "弃牌阶段" };

struct Player
{
    std::vector<Card> hand_;                            // 手牌，始终保持从小到大排序
    std::array<std::vector<Card>, k_area_num> areas_;   // 两个任务区，向所有玩家公开
    int score_ = 0;                                     // 累计分数
    bool task_done_ = false;                            // 本局是否已完成两个基础任务
    bool eliminated_ = false;                           // 是否已退出游戏

    // 本局开始时重置手牌与任务区，累计分数保留
    void ResetRound()
    {
        hand_.clear();
        for (auto& area : areas_) {
            area.clear();
        }
        task_done_ = false;
    }

    void SortHand() { std::sort(hand_.begin(), hand_.end()); }

    // 从手牌中取出指定的牌，手牌不足时返回 false 且不修改手牌
    bool TakeFromHand(const std::vector<Card>& cards)
    {
        std::vector<Card> rest = hand_;
        for (const auto& card : cards) {
            const auto it = std::find(rest.begin(), rest.end(), card);
            if (it == rest.end()) {
                return false;
            }
            rest.erase(it);
        }
        hand_ = std::move(rest);
        return true;
    }
};

// 连出指令的目标，如「1-1」表示 1 号玩家的第 1 个任务区
struct AreaArg
{
    int pid_;       // 从 0 开始
    int area_;      // 从 0 开始
};

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot
