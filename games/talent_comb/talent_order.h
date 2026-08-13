// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).
//
// 天赋结算顺序汇总：所有带固定顺序的 hook dispatch 在此声明 `inline constexpr` 数组。
// 单点定义原则——dispatcher（talent_impl.h / mygame.cc）和"查看顺序"规则指令（mygame.cc 中
// k_rule_commands）共用同一组数组，避免顺序在一处改了别处忘记跟进。
//
// 新增 / 调整顺序时，只改本文件即可同步给所有引用方与玩家可见的规则展示。

#pragma once

#include <cstddef>
#include <iterator>

#include "talent.h"

namespace lgtbot {
namespace game {
namespace GAME_MODULE_NAME {

// 触发时机：MainStage::TransformCardForPlacement_（砖块放进盘面前的牌面变换）
inline constexpr Talent k_transform_order[] = {
    Talent::三相之力,
    Talent::两极反转,
    Talent::九转玄机,
    Talent::零的力量,
    Talent::以退为进,
    Talent::完美块,
};

// 触发时机：MainStage::OnCardPlaced_ 在 UpdateScore 之前（直接影响 effective_result）
inline constexpr Talent k_card_placed_order[] = {
    Talent::临时用品,
    Talent::星河流转,
    Talent::张三来袭,
    Talent::贪婪宝藏,
    Talent::零的力量,
    Talent::虚空之心,
    Talent::致命节奏,
};

// 触发时机：MainStage::OnCardPlaced_ 在 UpdateScore 之后（依赖最新分数 / 永久额外分）
inline constexpr Talent k_after_score_order[] = {
    Talent::戴森球,
    Talent::冥想,
    Talent::天使轮,
    Talent::表演型人格,
    Talent::二环里,
};

// 触发时机：MainStage::HandleDiscard_
inline constexpr Talent k_discard_order[] = {
    Talent::垃圾回收,
    Talent::零号位,
    Talent::锻造,
    Talent::贪婪宝藏,
    Talent::致命节奏,
};

// 触发时机：MainStage::ApplyAttackTalents_（胜者对败者结算前的攻击侧修正）
inline constexpr Talent k_attack_order[] = {
    Talent::快攻,
    Talent::致命魔术,
    Talent::攻击形态,
    Talent::防御形态,
};

// 触发时机：MainStage::ApplyDefenseTalents_（败者收到伤害前的防御侧修正）
inline constexpr Talent k_defense_order[] = {
    Talent::钢铁之躯,
    Talent::防御形态,
    Talent::攻击形态,
};

// 触发时机：MainStage::ApplyDefeatTalents_
// 律动残余 排在最前：先把伤害封顶在 25，再交给后续 hook 进一步调整（如 事不过三 归零）。
inline constexpr Talent k_defeat_order[] = {
    Talent::律动残余,
    Talent::图灵测试,
    Talent::事不过三,
    Talent::有舍有得,
    Talent::败者之刃,
    Talent::贪婪宝藏,
    Talent::以战代练,
};

// 触发时机：MainStage::ApplyVictoryTalents_
inline constexpr Talent k_victory_order[] = {
    Talent::嗜血,
    Talent::败者之刃,
    Talent::以战代练,
};

// 触发时机：MainStage::CollectPreBattleExtras_（战前额外砖块阶段）
inline constexpr Talent k_pre_battle_extra_order[] = {
    Talent::锻造,
    Talent::有舍有得,
};

// 触发时机：ExtraCardStage::CheckForgeAfterPlace_（每一次额外砖块放置 / 弃牌后）
inline constexpr Talent k_extra_card_action_end_order[] = {
    Talent::锻造,
};

// 触发时机：MainStage::StartNewRound_（每回合开始时）
inline constexpr Talent k_round_start_order[] = {
    Talent::零号位,
    Talent::冥想,
    Talent::利滚利,
    Talent::临时用品,
};

// 触发时机：MainStage::NextStageFsm（放置 / 选牌 / 额外砖块阶段结束后的盘面变换）
inline constexpr Talent k_placement_end_order[] = {
    Talent::以退为进,
    Talent::完美块,
};

// 顺序展示用元表：把所有上述数组连同中文简称集中在一处，
// "查看天赋结算顺序"规则命令直接遍历此表生成文本。
struct TalentOrderEntry
{
    const char* label;           // 阶段中文简称（用于玩家可见的规则说明）
    const Talent* begin;
    std::size_t size;
};

template <std::size_t N>
constexpr TalentOrderEntry MakeOrderEntry(const char* label, const Talent (&arr)[N])
{
    return TalentOrderEntry{label, arr, N};
}

inline const TalentOrderEntry k_talent_order_table[] = {
    MakeOrderEntry("回合开始",       k_round_start_order),
    MakeOrderEntry("放置前砖块变换",   k_transform_order),
    MakeOrderEntry("放置后",         k_card_placed_order),
    MakeOrderEntry("放置后(更新分数后)", k_after_score_order),
    MakeOrderEntry("弃牌",           k_discard_order),
    MakeOrderEntry("放置/选牌阶段结束", k_placement_end_order),
    MakeOrderEntry("战前额外砖块",   k_pre_battle_extra_order),
    MakeOrderEntry("额外砖块结算",   k_extra_card_action_end_order),
    MakeOrderEntry("攻击修正",      k_attack_order),
    MakeOrderEntry("防御修正",      k_defense_order),
    MakeOrderEntry("战败",           k_defeat_order),
    MakeOrderEntry("胜利",           k_victory_order),
};

} // namespace GAME_MODULE_NAME
} // namespace game
} // namespace lgtbot
