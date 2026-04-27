// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).
//
// Talent object model.  This is intentionally state-first for now: stage logic
// still lives in the existing stage handlers, while per-talent counters and
// progress flags are owned by their corresponding talent classes.

#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include "talent.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

struct Player;
struct AreaCard;
struct ScoreResult;

struct TalentRoundContext
{
    bool is_selection_round = false;
    bool has_valuable_one = false;
};

struct TalentAcquireContext
{
    bool has_valuable_one = false;
    bool initial_test_mode = false;
};

struct TalentPlacementContext
{
    bool has_valuable_one = false;
};

struct TalentDiscardContext
{
    bool has_valuable_one = false;
    std::optional<Talent> source_talent = std::nullopt;
};

enum class TalentCardPlacementSource
{
    RegularRound,
    InitialDeal,
    Selection,
    ExtraCard,
};

struct TalentCardPlacedContext
{
    bool has_valuable_one = false;
    TalentCardPlacementSource source = TalentCardPlacementSource::Selection;
    std::optional<Talent> source_talent = std::nullopt;
    const AreaCard* previous_card = nullptr;
};

struct TalentDefeatContext
{
    bool has_valuable_one = false;
    bool is_mirror = false;
};

struct TalentVictoryContext
{
    bool is_mirror = false;
};

struct TalentPreBattleExtraContext
{
    bool has_valuable_one = false;
    SpecialEvent special_event = SpecialEvent::无;
    std::mt19937& rng;
    const AreaCard* offered_card = nullptr;
};

struct TalentDamageEffect
{
    int32_t delta = 0;
    std::string message;
};

class TalentBase
{
  public:
    explicit TalentBase(const TalentInfo info) : info_(info) {}
    virtual ~TalentBase() = default;

    Talent Id() const { return info_.id; }
    std::string Name() const { return info_.name; }
    std::string Description() const { return info_.description; }
    std::string_view Grade() const { return info_.grade; }
    const TalentInfo& Info() const { return info_; }

    // 棋盘上渲染玩家天赋列表时调用。
    // 返回该天赋的完整显示文本，可包含进度、临时分、失效状态、记录位置等彩色后缀。
    virtual std::string BoardDisplay(const Player& player) const;

    // 渲染玩家分数详情时调用。
    // 返回可选的彩色分数片段，通常以 "+" 或负分标记开头；主 UI 会拼接玩家全部天赋的片段。
    virtual std::string ScoreDetail(const Player& player) const;

    // 渲染玩家分数详情时调用。
    // 返回已经由该天赋的 ScoreDetail 自行展示的永久额外分，主 UI 会从通用永久分里扣除，避免重复显示。
    // 只影响显示，不改变玩家的实际分数。
    virtual int32_t ScoreDetailDisplayedPermanentExtra(const Player& player) const;

    // 查询对战分数时调用。
    // 返回参与对战比较的临时分修正值，但不会写入玩家的永久盘面分。
    virtual int32_t TempBattleScore(const Player& player) const;

    // 玩家获得该天赋后立即调用。
    // 适合处理立即生效效果、排入额外砖块、初始化计数器、全盘转换等逻辑。
    // 返回文本会追加到获得天赋的播报中。
    virtual std::string OnAcquire(Player& player, const TalentAcquireContext& context);

    // 砖块放置到棋盘前调用：玩家已经选择/获得该砖块，但 Fill/SeqFill 尚未修改棋盘。
    // 可直接修改 `card` 来实现“仅影响新放置砖块”的转换。
    // 返回文本会追加到放置结果中；`is_normal_round` 仅在常规共享砖块放置阶段为 true。
    virtual std::string OnBeforePlaceCard(Player& player, AreaCard& card, bool is_normal_round);

    // 每个新回合开始时，对已经拥有的天赋调用。
    // 适合处理回合维护逻辑，例如回血、临时砖块到期、刷新临时计数器等。
    // 返回文本会追加到回合开始提示中。
    virtual std::string OnRoundStart(Player& player, const TalentRoundContext& context);

    // 常规放置阶段结束后、进入下一个主阶段前调用。
    // 适合处理必须在所有玩家完成本轮放置/弃牌后结算的全盘转换。
    // 当前流程不会使用返回文本。
    virtual void OnPlacementStageEnd(Player& player, const TalentPlacementContext& context);

    // 砖块被弃掉而不是放置时调用。
    // 当弃掉的是天赋额外砖块时，`source_talent` 用于标识来源天赋。
    // 返回文本会追加到弃牌消息中。
    virtual std::string OnDiscard(Player& player, const AreaCard& card, const TalentDiscardContext& context);

    // 砖块已经实际放入棋盘后、Player::UpdateScore 前调用。
    // 可修改刚放置的砖块或棋盘，并更新 `effective_result`，让展示给玩家的分数变化匹配最终盘面。
    // `original_result` 是这些 hook 修改前的 Fill/SeqFill 结果。
    virtual std::string OnCardPlaced(Player& player, uint32_t idx, const ScoreResult& original_result, ScoreResult& effective_result,
                                     const TalentCardPlacedContext& context);

    // Player::UpdateScore 根据最终放置结果更新分数后调用。
    // 适合处理依赖最新分数/连线状态的效果，例如关闭临时分、修改百分比计数等。
    // `old_permanent_extra` 是本次放置更新前的永久额外分。
    virtual std::string AfterScoreUpdatedOnCardPlaced(Player& player, uint32_t idx, const ScoreResult& effective_result,
                                                      const TalentCardPlacedContext& context, int32_t old_permanent_extra);

    // 玩家在真实对战或镜像对战中战败后、伤害实际扣除前调用。
    // 适合处理战败计数和战败触发的盘面变化。
    // `damage` 是本次即将受到的伤害，天赋可将其改为 0 来免疫本次伤害。
    // 返回文本会追加到对战结果消息中。
    virtual std::string OnDefeat(Player& player, const TalentDefeatContext& context, int32_t& damage);

    // 玩家在真实对战或镜像对战中获胜后调用。
    // 适合处理胜利计数、回血、胜利后临时状态变更等不属于伤害变化的效果。
    // 返回文本会追加到对战结果消息中。
    virtual std::string OnVictory(Player& player, const TalentVictoryContext& context);

    // 战前额外砖块阶段收集额外砖块时调用。
    // 适合处理由战斗计数触发、但需要延后到战前额外阶段发放砖块的效果。
    // 返回文本会追加到战前额外砖块提示中。
    virtual std::string OnPreBattleExtraCards(Player& player, const TalentPreBattleExtraContext& context);

    // 额外砖块阶段中一次放置/弃牌操作结束后调用。
    // 适合处理由额外砖块操作推进的即时合成、追加额外砖块等效果。
    // 返回文本会立即播报。
    virtual std::string OnExtraCardActionEnd(Player& player);

    // 计算胜者造成的额外伤害时，对攻击方天赋调用。
    // 返回的 `delta` 会加到伤害上；正数增加伤害，负数降低伤害。
    // `message` 非空时会立即播报。
    virtual TalentDamageEffect AttackDamageDelta(Player& attacker, Player& defender, int32_t damage, std::mt19937& rng);

    // 计算败者受到的伤害修正时，对防御方天赋调用。
    // 返回值会加到受到的伤害上；负数减少伤害，正数增加伤害。
    virtual int32_t DefenseDamageDelta(Player& defender, int32_t damage);

  private:
    TalentInfo info_;
};

class PerfectBlockTalent;
class CounterattackTalent;
class SeizeTalent;
class ZeroRiskInvestmentTalent;
class IronBodyTalent;
class RetreatAdvanceTalent;
class DeadlyMagicTalent;
class TriForceTalent;
class EmergencyRescueTalent;
class WantAllTalent;
class PandoraBoxTalent;
class CompoundInterestTalent;
class DysonSphereTalent;
class DiscardScorerTalent;
class SincereTalent;
class GalaxyFlowTalent;
class MeditationTalent;
class LightInterferenceTalent;
class NineMysteryTalent;

class BloodlustTalent;
class StillUsefulTalent;
class SwiftAttackTalent;
class IndependentTalent;
class InPairsTalent;
class TrashRecycleTalent;
class SomethingRealTalent;
class OffensiveFormTalent;
class DefensiveFormTalent;
class LocalEnhanceTalent;
class GainAfterLossTalent;
class ThreeYearTalent;
class ForgeTalent;
class TailGoodsTalent;
class TuringTestTalent;
class NoMoreThanThreeTalent;
class SlotMachineTalent;
class DigitReverseTalent;
class LoserBladeTalent;
class TempWildTalent;
class BandageTalent;
class HerbalGrowthTalent;
class AngelRoundTalent;
class PlunderTalent;
class MultiChoiceTalent;
class ZhangSanTalent;
class GreedyTreasureTalent;
class ZeroPowerTalent;
class VoidHeartTalent;
class PerformancePersonalityTalent;

std::unique_ptr<TalentBase> CreateTalentState(Talent talent);

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot
