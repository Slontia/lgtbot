// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include "utility/random.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <optional>
#include <random>
#include <string>
#include <vector>

#ifndef GAME_MODULE_NAME
#error GAME_MODULE_NAME is not defined
#endif

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

// ========== 常量 ==========

static constexpr int k_player_num = 2;                                      // 固定为两人对局
static constexpr int k_group_num = 3;                                       // 每次发牌的牌组数
static constexpr int k_number_per_group = 4;                                // 每组的数字牌张数
static constexpr int k_group_size = k_number_per_group + 1;                  // 每组张数，含一张无限牌
static constexpr int k_number_per_deal = k_group_num * k_number_per_group;   // 每次发牌的数字牌总张数
static constexpr int k_duel_num = 4;                                        // 每回合的小局数
static constexpr int k_min_point = 1;                                       // 数字牌的最小数字
static constexpr int k_max_point = 10;                                      // 数字牌的最大数字
static constexpr int k_infinite_point = 0;                                  // 无限牌在内部的数字表示
// 基础投入的三个档位：第 1~6 回合逐回合递增，第 7~12 回合固定为 8，第 13 回合起固定为 10
static constexpr int k_base_flat_round = 7;                                 // 基础投入停止递增的回合
static constexpr int k_base_high_round = 13;                                // 基础投入进入最高档的回合
static constexpr int k_base_first = 2;                                      // 第 1 回合的基础投入
static constexpr int k_base_flat = 8;                                       // 第 7~12 回合的基础投入
static constexpr int k_base_high = 10;                                      // 第 13 回合起的基础投入
// 整回合未使用无限牌的回合末奖励，随基础投入的档位提高
static constexpr int k_bonus_low = 10;
static constexpr int k_bonus_flat = 14;
static constexpr int k_bonus_high = 16;

// ========== 卡牌 ==========

struct Card
{
    int point_ = k_infinite_point;

    bool IsInfinite() const { return point_ == k_infinite_point; }

    bool operator==(const Card& c) const { return point_ == c.point_; }

    // 数字牌按数字从小到大排列，无限牌排在最后
    bool operator<(const Card& c) const
    {
        if (IsInfinite() != c.IsInfinite()) {
            return !IsInfinite();
        }
        return point_ < c.point_;
    }
};

// 无限牌可接受的输入写法，第一项同时用于显示。
// 「♾️」带变体选择符（U+FE0F），部分输入法只送出不带变体选择符的「♾」，两者都需接受
const char* const k_infinite_names[] = {
    "∞", "无限", "无", "无限牌", "inf", "INF", "Inf", "♾️", "♾",
};

// 卡牌的文字名，如「7」「∞」
inline std::string CardName(const Card& card)
{
    return card.IsInfinite() ? k_infinite_names[0] : std::to_string(card.point_);
}

// 牌组的文字名，以空格分隔，如「3 5 7 ∞」
inline std::string CardsName(const std::vector<Card>& cards)
{
    std::string str;
    for (const auto& card : cards) {
        if (!str.empty()) {
            str += ' ';
        }
        str += CardName(card);
    }
    return str;
}

// 解析单张卡牌，不合法则返回空
inline std::optional<Card> ParseCard(const std::string& str)
{
    for (const char* const name : k_infinite_names) {
        if (str == name) {
            return Card{k_infinite_point};
        }
    }
    if (str.empty() || str.size() > 2 || str[0] == '0') {
        return std::nullopt;
    }
    for (const char ch : str) {
        if (ch < '0' || ch > '9') {
            return std::nullopt;
        }
    }
    const int point = std::stoi(str);
    if (point < k_min_point || point > k_max_point) {
        return std::nullopt;
    }
    return Card{point};
}

// 小局的比较结果
enum class DuelResult : int { FIRST_WIN = 0, SECOND_WIN, DRAW };

// 比较两张牌：无限牌胜过任意数字牌，双方均为无限牌或数字相同均为平
inline DuelResult CompareCard(const Card& a, const Card& b)
{
    if (a.IsInfinite() && b.IsInfinite()) {
        return DuelResult::DRAW;
    }
    if (a.IsInfinite()) {
        return DuelResult::FIRST_WIN;
    }
    if (b.IsInfinite()) {
        return DuelResult::SECOND_WIN;
    }
    if (a.point_ == b.point_) {
        return DuelResult::DRAW;
    }
    return a.point_ > b.point_ ? DuelResult::FIRST_WIN : DuelResult::SECOND_WIN;
}

// 指定回合的基础投入。第 1~6 回合为 2、3、4、5、6、7，第 7~12 回合为 8，第 13 回合起为 10
inline int BaseOfRound(const int round)
{
    if (round < k_base_flat_round) {
        return k_base_first + round - 1;
    }
    return round < k_base_high_round ? k_base_flat : k_base_high;
}

// 指定回合中「整回合未使用无限牌」的奖励金币，与基础投入的档位一致
inline int BonusOfRound(const int round)
{
    if (round < k_base_flat_round) {
        return k_bonus_low;
    }
    return round < k_base_high_round ? k_bonus_flat : k_bonus_high;
}

// ========== 发牌 ==========

// 一次发牌的结果：两位玩家各三组牌
using DealResult = std::array<std::array<std::vector<Card>, k_group_num>, k_player_num>;

// 随机生成 count 个位于 [k_min_point, k_max_point] 的数字
inline std::vector<int> RandNumbers_(std::mt19937& rng, const int count)
{
    std::vector<int> nums(static_cast<size_t>(count));
    for (int& num : nums) {
        num = static_cast<int>(RandInt(rng, k_min_point, k_max_point));
    }
    return nums;
}

// 把数字总和调整为 target，调整过程中每个数字始终保持在 [k_min_point, k_max_point] 内
inline void AdjustSum_(std::mt19937& rng, std::vector<int>& nums, const int target)
{
    const int size = static_cast<int>(nums.size());
    if (size == 0) {
        return;
    }
    int sum = std::accumulate(nums.begin(), nums.end(), 0);
    while (sum != target) {
        const int begin = static_cast<int>(RandInt(rng, 0, static_cast<uint32_t>(size) - 1));
        bool moved = false;
        for (int i = 0; i < size && !moved; ++i) {
            int& num = nums[static_cast<size_t>((begin + i) % size)];
            if (sum < target && num < k_max_point) {
                ++num;
                ++sum;
                moved = true;
            } else if (sum > target && num > k_min_point) {
                --num;
                --sum;
                moved = true;
            }
        }
        if (!moved) {   // 全部数字均已触及边界，无法继续靠近目标
            break;
        }
    }
}

// 在保持总和不变的前提下，随机把一个数字加一、另一个数字减一
inline void PerturbNumbers_(std::mt19937& rng, std::vector<int>& nums)
{
    const int size = static_cast<int>(nums.size());
    if (size < 2) {
        return;
    }
    const int begin = static_cast<int>(RandInt(rng, 0, static_cast<uint32_t>(size) - 1));
    for (int i = 0; i < size; ++i) {
        int& up = nums[static_cast<size_t>((begin + i) % size)];
        if (up >= k_max_point) {
            continue;
        }
        for (int j = 1; j < size; ++j) {
            int& down = nums[static_cast<size_t>((begin + i + j) % size)];
            if (down <= k_min_point) {
                continue;
            }
            ++up;
            --down;
            return;
        }
    }
}

// 取出第 group 组的数字并从小到大排序
inline std::vector<int> GroupNumbers_(const std::vector<int>& nums, const int group)
{
    const auto begin = nums.begin() + static_cast<std::ptrdiff_t>(group) * k_number_per_group;
    std::vector<int> sub(begin, begin + k_number_per_group);
    std::sort(sub.begin(), sub.end());
    return sub;
}

// 判定两位玩家的任意两组是否都不相同（忽略组内顺序）
inline bool NoSameGroup_(const std::vector<int>& first, const std::vector<int>& second)
{
    for (int a = 0; a < k_group_num; ++a) {
        const std::vector<int> group_a = GroupNumbers_(first, a);
        for (int b = 0; b < k_group_num; ++b) {
            if (group_a == GroupNumbers_(second, b)) {
                return false;
            }
        }
    }
    return true;
}

// 生成一次发牌：两位玩家各三组，每组四张 1~10 的数字牌加一张无限牌。
// 两位玩家的十二张数字牌之和相同，且任一玩家的任一组都不与对手的任一组相同
inline DealResult MakeDeal(std::mt19937& rng)
{
    static constexpr int k_max_retry = 200;     // 组间去重的尝试次数上限

    std::vector<int> first = RandNumbers_(rng, k_number_per_deal);
    const int target = std::accumulate(first.begin(), first.end(), 0);
    std::vector<int> second = RandNumbers_(rng, k_number_per_deal);
    AdjustSum_(rng, second, target);
    for (int retry = 0; retry < k_max_retry && !NoSameGroup_(first, second); ++retry) {
        SeededShuffle(first.begin(), first.end(), rng);
        SeededShuffle(second.begin(), second.end(), rng);
        PerturbNumbers_(rng, second);   // 重新分组无法消除重复时，改变数字构成本身
    }

    DealResult result;
    const std::vector<int>* const nums[k_player_num] = { &first, &second };
    for (int pid = 0; pid < k_player_num; ++pid) {
        for (int group = 0; group < k_group_num; ++group) {
            std::vector<Card>& cards = result[static_cast<size_t>(pid)][static_cast<size_t>(group)];
            cards.clear();
            cards.reserve(k_group_size);
            for (const int num : GroupNumbers_(*nums[pid], group)) {
                cards.emplace_back(Card{num});
            }
            cards.emplace_back(Card{k_infinite_point});     // 每组固定附带一张无限牌
        }
    }
    return result;
}

// ========== 玩家 ==========

// 回合内的四个阶段
enum class Step : int { SELECT = 0, ORDER, PICK, BET };

const char* const k_step_names[4] = { "选组阶段", "排序阶段", "选牌阶段", "对局阶段" };

struct Player
{
    int coin_ = 0;                                          // 当前金币
    std::array<std::vector<Card>, k_group_num> groups_;     // 持有的三组牌，已使用的组为空
    int group_ = -1;                                        // 本回合选定的组编号，-1 表示尚未选定
    std::vector<Card> hand_;                                // 本回合选定组中尚未排出的牌
    std::vector<Card> order_;                               // 本回合已按小局顺序排出的牌
    bool used_infinite_ = false;                            // 本回合排出的牌中是否含无限牌
    int bet_ = 0;                                           // 本小局已投入的金币
    bool folded_ = false;                                   // 本小局是否已放弃
    bool eliminated_ = false;                               // 是否已退出本场游戏

    // 尚可选用的牌组数量
    int RemainGroupNum() const
    {
        int num = 0;
        for (const auto& group : groups_) {
            if (!group.empty()) {
                ++num;
            }
        }
        return num;
    }

    // 仅剩一组可选时返回该组编号，否则返回 -1
    int OnlyGroup() const
    {
        int found = -1;
        for (int i = 0; i < k_group_num; ++i) {
            if (groups_[static_cast<size_t>(i)].empty()) {
                continue;
            }
            if (found != -1) {
                return -1;
            }
            found = i;
        }
        return found;
    }

    // 三组牌全部用尽，需要重新发牌
    bool NeedDeal() const { return RemainGroupNum() == 0; }

    void ResetDuel()
    {
        bet_ = 0;
        folded_ = false;
    }

    void ResetRound()
    {
        group_ = -1;
        hand_.clear();
        order_.clear();
        used_infinite_ = false;
        ResetDuel();
    }

    void Deal(const std::array<std::vector<Card>, k_group_num>& groups) { groups_ = groups; }

    // 选定本回合使用的牌组，该组随即从持有的牌组中移出
    void SelectGroup(const int group)
    {
        group_ = group;
        hand_ = groups_[static_cast<size_t>(group)];
        groups_[static_cast<size_t>(group)].clear();
    }

    // 从尚未排出的牌中取出指定的牌，数量不足时返回 false 且不修改手牌
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

    // 把牌排入本回合的出牌顺序
    void PushOrder(const std::vector<Card>& cards)
    {
        for (const auto& card : cards) {
            order_.emplace_back(card);
            if (card.IsInfinite()) {
                used_infinite_ = true;
            }
        }
    }

    // 本小局使用的牌，尚未排出时返回空
    std::optional<Card> DuelCard(const int duel) const
    {
        if (duel < 0 || static_cast<size_t>(duel) >= order_.size()) {
            return std::nullopt;
        }
        return order_[static_cast<size_t>(duel)];
    }
};

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot
