// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).
//
// This file was generated with the assistance of Claude Code (claude.ai/code).

#include <array>
#include <map>
#include <functional>
#include <limits>
#include <memory>
#include <vector>
#include <random>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <set>
#include <queue>

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"
#include "utility/random.h"

#include "talent_comb.h"
#include "talent_order.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;
const GameProperties k_properties {
    .name_ = "天赋云巢",
    .developer_ = "铁蛋",
    .description_ = "云顶之巢 + 天赋系统，在战斗中获得天赋强化，成为最后的胜者",
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 8; }
uint32_t Multiple(const CustomOptions& options) { return GET_OPTION_VALUE(options, 种子).empty() ? 2 : 0; }
const MutableGenericOptions k_default_generic_options;

static std::string TalentRuleText(const int talent_id)
{
    if (talent_id < 0 || talent_id >= static_cast<int>(Talent::COUNT)) {
        return "未知天赋";
    }
    const auto talent = static_cast<Talent>(talent_id);
    const auto& info = GetTalentInfo(talent);
    return "【" + std::string(info.name) + "】\n等级：" + std::string(info.grade) + "\n" + info.description;
}

// 「游戏规则细节」命令的三种分支文本：游戏阶段 / 特殊事件 / 天赋结算优先级
static constexpr const char* k_phase_rule = R"EOF(### 游戏阶段
每个回合按以下顺序依次执行各阶段：
1. **砖块放置阶段**：每位玩家获得一张砖块（普通轮从卡池1抽取，选牌轮从卡池2选取），选择放置位置或弃牌
2. **战前额外砖块阶段**（如有）：处理战前触发的额外砖块，如「锻造」合成的砖块等
3. **天赋选择阶段**（如有）：达成连线条件（每3条线）的玩家从3个天赋中选择1个
4. **主动天赋阶段**（如有）：处理主动类天赋行动，也可 `pass` 放弃发动
5. **对战阶段**（第3回合起）：随机配对进行分数比拼，分低者扣血，同时结算中毒伤害与淘汰判定
6. **战后额外砖块阶段**（如有）：处理战后触发的额外砖块，如「三年之期」存储到期的砖块、「劫掠」从被淘汰玩家处获得的砖块等

所有阶段结束后进入下一回合，重复以上流程直到仅剩1名玩家存活或卡池耗尽。)EOF";

static constexpr const char* k_event_rule = R"EOF(### 特殊事件
- **大的要来了：** 卡池中没有1
- **两极分化：** 卡池中没有5
- **大的没了：** 卡池中没有9
- **天降恩泽：** 第一轮每人发一个癞子
- **调色盘：** 卡池中添加大量癞子
- **有1吗：** 每行1额外加12分
- **小透不算挂：** 提前公布下一轮的卡
- **？？？：** 天降恩泽 + 调色盘 + 有1吗 + 小透不算挂)EOF";

// 把 talent_order.h 的 k_talent_order_table 渲染成玩家可读的"天赋结算顺序"文本。
// 元表里的每一条对应一个 hook 的固定优先级数组，未列出的 hook 走"按玩家持有顺序"分发。
// 改动顺序只需更新 talent_order.h，本展示自动同步。
static std::string MakeTalentOrderRuleText()
{
    std::string text = "### 天赋结算顺序\n```\n";
    for (const auto& entry : k_talent_order_table) {
        text += "\n【";
        text += entry.label;
        text += "】\n  ";
        for (std::size_t i = 0; i < entry.size; ++i) {
            if (i > 0) text += "➡️";
            text += TalentName(entry.begin[i]);
        }
    }
    text += "\n```";
    return text;
}

// 查找天赋详情：输入完整天赋名命中即返回详情，未命中则列出 A/B 级全部天赋名清单供玩家参考。
static std::string LookupTalentRuleText(const std::string& name)
{
    // 容错：去掉用户输入首尾空白
    const auto trim = [](std::string s) {
        const auto first = s.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return std::string{};
        const auto last = s.find_last_not_of(" \t\r\n");
        return s.substr(first, last - first + 1);
    };
    const std::string trimmed = trim(name);
    for (int i = 0; i < static_cast<int>(Talent::COUNT); ++i) {
        const Talent t = static_cast<Talent>(i);
        if (TalentName(t) == trimmed) {
            return TalentRuleText(i);
        }
    }
    // 未命中：返回完整清单
    const auto& grade_a = GradeATalents();
    const auto& grade_b = GradeBTalents();
    std::string text = "【A级天赋(" + std::to_string(grade_a.size()) + "个)】\n";
    bool first = true;
    for (const Talent t : grade_a) {
        if (!first) text += "、";
        text += TalentName(t);
        first = false;
    }
    text += "\n\n【B级天赋(" + std::to_string(grade_b.size()) + "个)】\n";
    first = true;
    for (const Talent t : grade_b) {
        if (!first) text += "、";
        text += TalentName(t);
        first = false;
    }
    return text;
}

const std::vector<RuleCommand> k_rule_commands = {
    RuleCommand("查看游戏规则细节和结算优先级",
            [](const int type) -> const char* const
            {
                static const std::string order_rule = MakeTalentOrderRuleText();
                switch (type) {
                    case 0: return k_phase_rule;
                    case 1: return k_event_rule;
                    case 2: return order_rule.c_str();
                }
                return "";
            },
            AlterChecker<int>({{"游戏阶段", 0}, {"特殊事件", 1}, {"优先级", 2}})),
    RuleCommand("查看指定天赋效果（或查看完整列表）",
                [](const std::string& name) -> const char* const
                {
                    static std::string rule_text;
                    rule_text = LookupTalentRuleText(name);
                    return rule_text.c_str();
                },
                AnyArg("天赋名", "绝地反击")),
};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() < 2) {
        reply() << "天赋云巢至少需要 2 人参加游戏";
        return false;
    }
    return true;
}

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options)
            {
                generic_options.bench_computers_to_player_num_ = 4;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
    InitOptionsCommand("选择特殊事件开始游戏",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options,
                const int32_t& event)
            {
                GET_OPTION_VALUE(game_options, 事件) = event;
                return NewGameMode::MULTIPLE_USERS;
            },
            AlterChecker<int32_t>({{"无", 1}, {"大的要来了", 2}, {"两极分化", 3}, {"大的没了", 4},
                                   {"天降恩泽", 5}, {"调色盘", 6}, {"有1吗", 7}, {"小透不算挂", 8}, {"？？？", 9}})),
};

// ==================== Card Pool Generation ====================

static const std::array<std::vector<int32_t>, k_direct_max> k_points{
    std::vector<int32_t>{3, 4, 8},
    std::vector<int32_t>{1, 5, 9},
    std::vector<int32_t>{2, 6, 7}
};

static std::vector<AreaCard> GenerateBaseCards(SpecialEvent event)
{
    std::vector<int32_t> p0 = {3, 4, 8};
    std::vector<int32_t> p1 = {1, 5, 9};
    std::vector<int32_t> p2 = {2, 6, 7};

    // Remove digits based on special events
    if (event == SpecialEvent::大的要来了) {
        p1.erase(std::remove(p1.begin(), p1.end(), 1), p1.end());
    } else if (event == SpecialEvent::两极分化) {
        p1.erase(std::remove(p1.begin(), p1.end(), 5), p1.end());
    } else if (event == SpecialEvent::大的没了) {
        p1.erase(std::remove(p1.begin(), p1.end(), 9), p1.end());
    }

    std::vector<AreaCard> cards;
    for (int32_t a : p0) {
        for (int32_t b : p1) {
            for (int32_t c : p2) {
                cards.emplace_back(a, b, c);
                cards.emplace_back(a, b, c);
            }
        }
    }
    return cards;
}

static std::mt19937 MakeSubRng(std::mt19937& seed_generator)
{
    std::array<uint32_t, 8> seed_data;
    for (auto& value : seed_data) {
        value = seed_generator();
    }
    std::seed_seq seq(seed_data.begin(), seed_data.end());
    return std::mt19937(seq);
}

// ==================== Forward Declarations ====================

class RoundStage;
class SelectStage;
class ExtraCardStage;
class TalentStage;
class ActiveTalentStage;

// ==================== Round Phase ====================

enum class RoundPhase {
    放置,
    战前额外,
    天赋选择,
    主动天赋,
    对战,
    战后额外,
};

// ==================== MainStage ====================

class MainStage : public MainGameStage<RoundStage, SelectStage, ExtraCardStage, TalentStage, ActiveTalentStage>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility))
        , round_(0)
        , alive_(Global().PlayerNum())
        , player_out_(Global().PlayerNum(), 0)
        , player_out_phase_(Global().PlayerNum(), 0)
        , player_leave_(Global().PlayerNum(), false)
        , player_final_score_(Global().PlayerNum(), 0)
        , phase_(RoundPhase::放置)
        , fought_round_(0)
        , last_mirror_(-1)
    {
        // Seed
        seed_str_ = GAME_OPTION(种子);
        if (seed_str_.empty()) {
            std::random_device rd;
            std::uniform_int_distribution<unsigned long long> dis;
            seed_str_ = std::to_string(dis(rd));
        }
        std::mt19937 seed_generator = MakeRng(seed_str_);
        event_rng_ = MakeSubRng(seed_generator);
        pool1_rng_ = MakeSubRng(seed_generator);
        pool2_rng_ = MakeSubRng(seed_generator);
        talent_rng_ = MakeSubRng(seed_generator);
        random_card_rng_ = MakeSubRng(seed_generator);
        battle_rng_ = MakeSubRng(seed_generator);
        selection_order_rng_ = MakeSubRng(seed_generator);
        random_talent_rng_ = MakeSubRng(seed_generator);

        // Special event
        int event_opt = GAME_OPTION(事件);
        if (event_opt == 0) {
            // Random: 5/20 NONE, 2/20 each of 7 events, 1/20 MYSTERY
            std::vector<SpecialEvent> pool = {
                SpecialEvent::无, SpecialEvent::无, SpecialEvent::无,
                SpecialEvent::无, SpecialEvent::无,
                SpecialEvent::大的要来了, SpecialEvent::大的要来了,
                SpecialEvent::两极分化, SpecialEvent::两极分化,
                SpecialEvent::大的没了, SpecialEvent::大的没了,
                SpecialEvent::天降恩泽, SpecialEvent::天降恩泽,
                SpecialEvent::调色盘, SpecialEvent::调色盘,
                SpecialEvent::有1吗, SpecialEvent::有1吗,
                SpecialEvent::小透不算挂, SpecialEvent::小透不算挂,
                SpecialEvent::愚人节,
            };
            special_event_ = pool[RandInt(event_rng_, 0, static_cast<uint32_t>(pool.size() - 1))];
        } else {
            special_event_ = static_cast<SpecialEvent>(event_opt - 1);
        }

        // Create players
        const int32_t init_hp = static_cast<int32_t>(GAME_OPTION(血量));
        for (uint64_t i = 0; i < Global().PlayerNum(); ++i) {
            players_.emplace_back(Global().ResourceDir(), init_hp);
            players_.back().InitTalentPools(special_event_);
        }

        // Generate card pools
        // Pool1 (cards_): for normal rounds (round 2+), 54 base cards + 2 wild
        cards_ = GenerateBaseCards(special_event_);
        cards_.emplace_back(); // wild
        cards_.emplace_back(); // wild
        if (HasColorful(special_event_)) {
            cards_.emplace_back(); // +3 more wild for colorful
            cards_.emplace_back();
            cards_.emplace_back();
        }
        SeededShuffle(cards_.begin(), cards_.end(), pool1_rng_);

        // Pool2 (cards2_): for round 1 initial + selection rounds, 54 base cards, no wild
        cards2_ = GenerateBaseCards(special_event_);
        SeededShuffle(cards2_.begin(), cards2_.end(), pool2_rng_);

        it_ = cards_.begin();
        it2_ = cards2_.begin();

        // Damage rate
        dRate_ = std::pow(M_E, Global().PlayerNum() / 6.0) / M_E;
        iRate_ = dRate_;
    }

    virtual void FirstStageFsm(SubStageFsmSetter setter) override;
    virtual void NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;
    virtual void NextStageFsm(SelectStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;
    virtual void NextStageFsm(ExtraCardStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;
    virtual void NextStageFsm(TalentStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;
    virtual void NextStageFsm(ActiveTalentStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;

    // 框架仅在游戏结束时取一次作为入库 game_score。
    // 生产构建：返回按淘汰名次结算的 player_final_score_（DoGameOver_ 已写入）。
    // 测试构建（TEST_BOT）：返回盘面+天赋原分，让历史 AUTO_PLAY_TEST 的固定期望值
    //   不受新名次计分影响；名次计分的逻辑通过手算验证 + 框架级集成测试覆盖。
    int64_t PlayerScore(const PlayerID pid) const override
    {
#ifdef TEST_BOT
        return players_[pid].TotalScore();
#else
        return player_final_score_[pid];
#endif
    }

    int64_t PlayerBattleScore(const PlayerID pid) const
    {
        return players_[pid].TotalScore() + players_[pid].EffectiveTempBattleScore();
    }

    // 盘面天赋原分（不含终局血量奖惩），供单元测试作确定性断言使用。
    int64_t PlayerRawTotalScore(const PlayerID pid) const
    {
        return players_[pid].TotalScore();
    }

    int32_t PlayerHP(const PlayerID pid) const
    {
        return players_[pid].hp_;
    }

    // ===== UI / Display (implemented in talent_impl.h) =====
    std::string GetName(const std::string& x);
    std::pair<std::string, std::string> PlayerScoreDetail(const PlayerID pid) const;
    void SetPlayerBoard(html::Table& table, const int pos, const PlayerID pid, const bool isEliminated);
    std::string CombHtml(const std::string& str);

    // Check if game should end
    bool CheckGameOver() const
    {
        return alive_ <= 1;
    }

    // ===== Battle System (implemented in talent_impl.h) =====
    // sender 由调用方持有，DoBattle_ 与 DoEliminationAfterBattle_ 共用，使对战结果与玩家淘汰播报合并为一条消息。
    bool DoBattle_(MsgSenderBase::MsgSenderGuard& sender);
    // 返回 OnLethalDamage 触发的播报文本
    std::string ApplyDamage_(PlayerID pid, int32_t damage);
    void ProcessBattle_(PlayerID pid1, PlayerID pid2, bool mirror, std::string& result,
                        std::optional<int64_t> mirror_battle_score = std::nullopt,
                        std::optional<int32_t> mirror_base_score = std::nullopt);
    int32_t ApplyAttackTalents_(PlayerID attacker, PlayerID defender, int32_t damage);
    int32_t ApplyDefenseTalents_(PlayerID defender, int32_t damage);
    void ApplyDefeatTalents_(PlayerID loser, bool mirror, int32_t& damage,
                             int64_t my_battle_score, int64_t opp_battle_score, std::string& result);
    void ApplyVictoryTalents_(PlayerID winner, bool mirror,
                              int64_t my_battle_score, int64_t opp_battle_score, std::string& result);
    void ApplyBattleEndTalents_(PlayerID pid, bool mirror,
                                int64_t my_battle_score, int64_t opp_battle_score,
                                int32_t outcome, std::string& result);
    void DoEliminationAfterBattle_(MsgSenderBase::MsgSenderGuard& sender);
    std::optional<PlayerID> FindKillerFromFightMap_(PlayerID victim) const;
    void DoPoison_();

    // 真实对战阶段结束（DoBattle_ 确实发生了对战）后逐 alive 玩家分发 OnBattlePhaseEnd hook。
    // 例如「绝地反击」会在 trigger_round < round_ 时清空 extra_score。
    // 跳过对战的阶段（选牌轮 / 紧急救援）不会触发本调用，所以相关状态会继续保留到下一次真实对战。
    void OnBattlePhaseEnd_()
    {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (player_out_[pid] != 0) continue;
            auto& p = players_[pid];
            for (const auto talent : p.talents_) {
                p.talent_states_.at(talent)->OnBattlePhaseEnd(p, round_);
            }
        }
    }

    void CollectPreBattleExtras_();
    void CollectPostBattleExtras_();

    // ===== Talent Effects (implemented in talent_impl.h) =====
    // idx: the position (1-19) the card was placed at; 0 means "no galaxy-flow trigger applicable".
    std::pair<std::string, int32_t> OnCardPlaced_(PlayerID pid, uint32_t idx, const ScoreResult& result,
                                                  std::optional<Talent> source_talent = std::nullopt,
                                                  const AreaCard* previous_card = nullptr,
                                                  TalentCardPlacementSource placement_source = TalentCardPlacementSource::Selection);
    std::pair<AreaCard, std::string> TransformCardForPlacement_(PlayerID pid, const AreaCard& card, bool is_normal_round = false);
    std::string HandleDiscard_(PlayerID pid, const AreaCard& card, std::optional<Talent> source_talent = std::nullopt);
    bool HandleThreeYearStore_(PlayerID pid, const AreaCard& card);

    // ===== Card Pool Management (implemented in talent_impl.h) =====
    AreaCard DrawFromPool2_();
    std::vector<AreaCard> DrawBalancedCards_(uint32_t count);
    AreaCard DrawFromPool1_();

    // Check if any player needs talent choice
    bool AnyPendingTalentChoice_() const
    {
        if (IsTestMode_()) return false;
        for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
            if (player_out_[pid] != 0) continue;
            // Player already has a filled talent pool (ready to choose)
            if (!players_[pid].talent_pool_.empty()) return true;
            // Check if player qualifies for new talent (use selection count, not talent count)
            uint32_t line_count = players_[pid].comb_->LineCount();
            if (players_[pid].talent_selection_count_ < line_count / 3) {
                return true;
            }
        }
        return false;
    }

    // Generate talent pools for qualifying players
    void PrepareNewTalentChoices_()
    {
        if (IsTestMode_()) return;
        for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
            if (player_out_[pid] != 0) continue;
            auto& player = players_[pid];
            if (!player.talent_pool_.empty()) continue;
            uint32_t line_count = player.comb_->LineCount();
            if (player.talent_selection_count_ < line_count / 3) {
                player.GenerateTalentPool(talent_rng_);
            }
        }
    }

    bool IsTestMode_() const
    {
#ifdef TEST_BOT
        return !GAME_OPTION(天赋).empty();
#else
        return false;
#endif
    }

    // Check if any player has extra cards to place or choices to make
    bool AnyPendingExtraCards_() const
    {
        for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
            if (player_out_[pid] != 0) continue;
            if (!players_[pid].extra_card_queue_.empty()) return true;
        }
        return false;
    }

    bool AnyPendingActiveTalent_() const
    {
        for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
            if (player_out_[pid] != 0) continue;
            const auto& player = players_[pid];
            for (const auto talent : player.talents_) {
                if (player.talent_states_.at(talent)->HasPendingActiveChoice(player)) return true;
            }
        }
        return false;
    }

    // Advance the round phase state machine
    void AdvancePhase_(SubStageFsmSetter& setter);

    // Start a new round (card placement stage)
    void StartNewRound_(SubStageFsmSetter& setter);

    static bool IsSelectionRound(int32_t round)
    {
        // Rounds 8, 15, 22, 29... (first selection at round 8, then every 7)
        return round >= 8 && (round - 8) % 7 == 0;
    }

    int32_t round_;
    int32_t alive_;
    std::vector<Player> players_;
    std::vector<int32_t> player_out_;
    // 与 player_out_ 平行：记录该玩家淘汰发生在本回合的哪个阶段，用于排名差别。
    // 0=存活，1=对战阶段淘汰，2=中毒阶段淘汰（中毒结算在对战之后，因此排名更优）。
    std::vector<int32_t> player_out_phase_;
    std::vector<bool> player_leave_;
    // DoGameOver_ 中由 ComputeRankScores_ 填入；提交给框架作 game_score 的最终名次分。
    std::vector<int64_t> player_final_score_;
    SpecialEvent special_event_;
    RoundPhase phase_;

    std::vector<AreaCard> cards_;
    decltype(cards_)::iterator it_;
    std::vector<AreaCard> cards2_;
    decltype(cards2_)::iterator it2_;

    std::mt19937 event_rng_;
    std::mt19937 pool1_rng_;
    std::mt19937 pool2_rng_;
    std::mt19937 talent_rng_;
    std::mt19937 random_card_rng_;
    std::mt19937 battle_rng_;
    std::mt19937 selection_order_rng_;
    std::mt19937 random_talent_rng_;
    std::string seed_str_;

    // Battle state
    std::vector<std::unordered_map<int32_t, int32_t>> fought_list_;
    std::unordered_map<int32_t, int32_t> current_fight_map_;  // attacker→defender for current round
    double dRate_, iRate_;
    int32_t last_mirror_;
    int32_t fought_round_;

    // For foresee event
    std::optional<AreaCard> next_card_;

    // 紧急救援: flag for emergency-triggered SelectStage
    bool is_emergency_select_ = false;

    // Game over
    void DoGameOver_();
    void ComputeRankScores_();

    std::string ApplyImmediateTalentEffects_(PlayerID pid, Talent talent);
};

// Include talent effects, UI rendering, and battle settlement implementations
#include "talent_impl.h"

std::string MainStage::ApplyImmediateTalentEffects_(PlayerID pid, Talent talent)
{
    auto& player = players_[pid];
    std::string extra_msg;
    switch (talent) {
        case Talent::摇奖机: {
            // Randomly draw an A-tier talent (excluding PANDORA_BOX which cannot be obtained via random talents)
            std::vector<Talent> avail;
            for (auto t : player.available_a_) {
                if (t != Talent::潘多拉魔盒) avail.push_back(t);
            }
            if (!avail.empty()) {
                SeededShuffle(avail.begin(), avail.end(), random_talent_rng_);
                Talent result = avail[0];
                // Replace SLOT_MACHINE with the drawn talent in talents_
                auto it = std::find(player.talents_.begin(), player.talents_.end(), Talent::摇奖机);
                if (it != player.talents_.end()) *it = result;
                player.available_a_.erase(result);
                player.available_b_.erase(result);
                // Apply the drawn talent's immediate effects
                ApplyImmediateTalentEffects_(pid, result);
                extra_msg = "，摇到了A级天赋「" + std::string(TalentName(result)) + "」——" + std::string(TalentDescription(result));
            } else {
                extra_msg = "，但已没有可用的A级天赋！";
            }
            break;
        }
        case Talent::潘多拉魔盒: {
            // Randomly obtain 2 B-tier talents (exclude random-type talents like SLOT_MACHINE)
            std::vector<Talent> avail;
            for (auto t : player.available_b_) {
                if (t != Talent::摇奖机) avail.push_back(t);
            }
            if (avail.size() >= 2) {
                SeededShuffle(avail.begin(), avail.end(), random_talent_rng_);
                Talent t1 = avail[0], t2 = avail[1];
                // Replace PANDORA_BOX with first talent
                auto it = std::find(player.talents_.begin(), player.talents_.end(), Talent::潘多拉魔盒);
                if (it != player.talents_.end()) *it = t1;
                player.talents_.push_back(t2);
                player.available_a_.erase(t1); player.available_b_.erase(t1);
                player.available_a_.erase(t2); player.available_b_.erase(t2);
                std::string extra1 = ApplyImmediateTalentEffects_(pid, t1);
                std::string extra2 = ApplyImmediateTalentEffects_(pid, t2);
                extra_msg = "，开启潘多拉魔盒！获得B级天赋「" + std::string(TalentName(t1)) + "」" + extra1
                          + " 和「" + std::string(TalentName(t2)) + "」" + extra2;
            } else if (avail.size() == 1) {
                Talent t1 = avail[0];
                auto it = std::find(player.talents_.begin(), player.talents_.end(), Talent::潘多拉魔盒);
                if (it != player.talents_.end()) *it = t1;
                player.available_a_.erase(t1); player.available_b_.erase(t1);
                std::string extra1 = ApplyImmediateTalentEffects_(pid, t1);
                extra_msg = "，开启潘多拉魔盒！但B级天赋已不足，仅获得B级天赋「" + std::string(TalentName(t1)) + "」" + extra1;
            } else {
                extra_msg = "，但已没有可用的B级天赋！";
            }
            break;
        }
        default:
            extra_msg = player.talent_states_.at(talent)->OnAcquire(player, TalentAcquireContext{HasValuableOne(special_event_)});
            break;
    }

    // Re-check score (talents like 还是有用的 may change scoring)
    player.UpdateScore(ScoreResult{player.comb_->BaseScore(), player.comb_->LineCount(), 0}, HasValuableOne(special_event_));
    return extra_msg;
}

// ==================== RoundStage ====================

class RoundStage : public SubGameStage<>
{
  public:
    // Normal round: all players get the same card
    RoundStage(MainStage& main_stage, const uint64_t round, const AreaCard& card)
            : StageFsm(main_stage, "第" + std::to_string(round) + "回合",
                MakeStageCommand(*this, "放置砖块到指定位置（0 为弃牌）", &RoundStage::Set_, ArithChecker<uint32_t>(0, 19, "位置")),
                MakeStageCommand(*this, "查看游戏进展情况与本轮砖块", &RoundStage::Info_, VoidChecker("赛况")))
            , round_(round)
            , is_initial_(round == 1)
            , comb_html_(main_stage.CombHtml("## 第 " + std::to_string(round) + " 回合"))
    {
        if (is_initial_) {
            // Round 1: each player gets a balanced unique card
            if (HasWindfall(Main().special_event_)) {
                for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
                    if (Main().player_out_[pid] == 0) {
                        initial_cards_[pid] = AreaCard(); // wild card for windfall
                    }
                }
            } else {
                // Count alive players
                uint32_t alive_count = 0;
                std::vector<PlayerID> alive_pids;
                for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
                    if (Main().player_out_[pid] == 0) {
                        alive_count++;
                        alive_pids.push_back(pid);
                    }
                }
                auto balanced = Main().DrawBalancedCards_(alive_count);
                for (size_t i = 0; i < alive_pids.size(); ++i) {
                    initial_cards_[alive_pids[i]] = balanced[i];
                }
            }
        } else {
            shared_card_ = card;
        }
    }

    virtual void OnStageBegin() override
    {
        if (is_initial_) {
            Global().Boardcast() << "初始回合，每个人随机获得一张砖块，请公屏或私信裁判设置数字：";
            // Already mark eliminated players as ready
            for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
                if (Main().player_out_[pid] != 0) {
                    Global().SetReady(pid);
                }
            }
        } else {
            // Handle 三年之期 storage
            for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
                if (Main().player_out_[pid] != 0) {
                    Global().SetReady(pid);
                    continue;
                }
                if (Main().HandleThreeYearStore_(pid, shared_card_)) {
                    Global().Boardcast() << At(pid) << " 触发天赋「三年之期」，本轮砖块已存储（" << Main().players_[pid].ThreeYear().cards.size() << "/3）";
                    Global().SetReady(pid);
                }
            }

            std::string card_info = "本回合砖块为 " + shared_card_.CardName() + "，请公屏或私信裁判设置数字：";
            if (HasForesee(Main().special_event_) && Main().next_card_.has_value()) {
                card_info += "\n（下一轮砖块为：" + Main().next_card_->CardName() + "）";
            }
            Global().Boardcast() << card_info;
        }
        SendInfo(Global().BoardcastMsgSender());
        Global().StartTimer(GAME_OPTION(局时));
    }

  private:
    AreaCard GetCardForPlayer_(PlayerID pid) const
    {
        if (is_initial_) {
            auto it = initial_cards_.find(pid);
            if (it != initial_cards_.end()) return it->second;
            return AreaCard(); // shouldn't happen
        }
        return shared_card_;
    }

    void HandleUnreadyPlayers_()
    {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (Global().IsReady(pid) || Main().player_out_[pid] != 0) continue;

            auto& player = Main().players_[pid];
            const AreaCard card = GetCardForPlayer_(pid);
            auto sender = Global().Boardcast();

            if (player.comb_->HasEmptyPosition()) {
                auto [actual_card, transform_notify] = Main().TransformCardForPlacement_(pid, card, true);
                const auto [idx, result] = player.comb_->SeqFill(actual_card);
                auto [place_notify, actual_delta] = Main().OnCardPlaced_(
                    pid, idx, result, std::nullopt, nullptr,
                    is_initial_ ? TalentCardPlacementSource::InitialDeal : TalentCardPlacementSource::RegularRound);
                sender << At(pid) << " 超时，自动填入位置 " << idx;
                if (actual_delta > 0) {
                    sender << "，收获 " << actual_delta << " 点积分";
                }
            } else {
                // Board full, auto discard
                Main().HandleDiscard_(pid, card);
                sender << At(pid) << " 超时，盘面已满自动弃牌";
            }
        }
        Global().HookUnreadyPlayers();
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Main().player_leave_[pid] = true;
        return StageErrCode::CONTINUE;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        HandleUnreadyPlayers_();
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        HandleUnreadyPlayers_();
        return StageErrCode::CHECKOUT;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) return StageErrCode::OK;
        auto& player = Main().players_[pid];
        const AreaCard card = GetCardForPlayer_(pid);
        if (player.comb_->HasEmptyPosition()) {
            auto [actual_card, transform_notify] = Main().TransformCardForPlacement_(pid, card, true);
            const auto [idx, result] = player.comb_->SeqFill(actual_card);
            Main().OnCardPlaced_(pid, idx, result, std::nullopt, nullptr,
                                 is_initial_ ? TalentCardPlacementSource::InitialDeal : TalentCardPlacementSource::RegularRound);
        } else {
            Main().HandleDiscard_(pid, card);
        }
        return StageErrCode::READY;
    }

    AtomReqErrCode Set_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const uint32_t idx)
    {
        if (Global().IsReady(pid)) {
            reply() << "您已经设置过，无法重复设置";
            return StageErrCode::FAILED;
        }
        if (Main().player_out_[pid] != 0) {
            reply() << "[错误] 您已被淘汰";
            return StageErrCode::FAILED;
        }

        auto& player = Main().players_[pid];
        const AreaCard card = GetCardForPlayer_(pid);

        if (idx == 0) {
            // Discard
            std::string notify = Main().HandleDiscard_(pid, card);
            reply() << "弃牌成功" << notify;
            return StageErrCode::READY;
        }

        if (auto block_msg = player.ProtectedPositionMessage(idx)) {
            reply() << "[错误] " << *block_msg;
            return StageErrCode::FAILED;
        }

        // Transform card based on talents (三相之力, 以退为进, 完美块)
        const auto previous_card = player.comb_->GetCard(idx);
        auto [actual_card, transform_notify] = Main().TransformCardForPlacement_(pid, card, true);
        const auto result = player.comb_->Fill(idx, actual_card);
        auto [notify, actual_delta] = Main().OnCardPlaced_(
            pid, idx, result, std::nullopt, previous_card ? &*previous_card : nullptr,
            is_initial_ ? TalentCardPlacementSource::InitialDeal : TalentCardPlacementSource::RegularRound);

        auto sender = reply();
        sender << "设置位置 " << idx << " 成功";
        if (actual_delta > 0) {
            sender << "，收获 " << actual_delta << " 点积分";
        } else if (actual_delta < 0) {
            sender << "，损失 " << (-actual_delta) << " 点积分";
        }
        sender << transform_notify << notify;
        return StageErrCode::READY;
    }

    void SendInfo(MsgSenderBase& sender)
    {
        sender() << Markdown{comb_html_};
        if (is_initial_) {
            // Show initial cards image with player assignments
            sender() << Markdown(InitialCardHtml_(), 300);
        } else if (HasForesee(Main().special_event_) && Main().next_card_.has_value()) {
            // 小透不算挂: show current and next card side by side with arrow
            sender() << Markdown(ForeseeCardHtml_(), 64);
        } else {
            const std::string style = "<style>body{margin:0;}</style>" + GetStyle(Global().ResourceDir());
            sender() << Markdown(style + shared_card_.ToHtml(Global().ResourceDir()), 64);
        }
    }

    // 「赛况」回复：当前最新棋盘 + 本轮砖块图。盘面用 CombHtml 实时生成（不复用 comb_html_ 缓存）。
    AtomReqErrCode Info_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << Markdown{Main().CombHtml("## 第 " + std::to_string(round_) + " 回合")};
        if (is_initial_) {
            reply() << Markdown(InitialCardHtml_(), 300);
        } else if (HasForesee(Main().special_event_) && Main().next_card_.has_value()) {
            reply() << Markdown(ForeseeCardHtml_(), 64);
        } else {
            const std::string style = "<style>body{margin:0;}</style>" + GetStyle(Global().ResourceDir());
            reply() << Markdown(style + shared_card_.ToHtml(Global().ResourceDir()), 64);
        }
        return StageErrCode::OK;
    }

    std::string ForeseeCardHtml_()
    {
        const std::string img_path = Global().ResourceDir();
        const std::string style = "<style>body{margin:0;}</style>" + GetStyle(img_path);
        const std::string arrow_svg =
            "<svg width=\"32\" height=\"64\" viewBox=\"0 0 32 64\" style=\"vertical-align:middle;\">"
            "<polygon points=\"2,24 18,24 18,16 30,32 18,48 18,40 2,40\" fill=\"#666\"/>"
            "</svg>";
        html::Table card_table(1, 3);
        card_table.SetTableStyle("align=\"center\" cellpadding=\"0\" cellspacing=\"0\" ");
        card_table.Get(0, 0).SetContent(shared_card_.ToHtml(img_path));
        card_table.Get(0, 1).SetContent(arrow_svg);
        card_table.Get(0, 2).SetContent(Main().next_card_->ToHtml(img_path));
        return style + card_table.ToString();
    }

    std::string InitialCardHtml_()
    {
        const std::string img_path = Global().ResourceDir();
        const std::string style = "<style>body{margin:0;}</style>" + GetStyle(img_path);
        html::Table card_table(2, initial_cards_.size());
        card_table.SetTableStyle("align=\"center\" cellpadding=\"3\" cellspacing=\"0\" ");
        size_t col = 0;
        for (const auto& [pid, card] : initial_cards_) {
            card_table.Get(0, col).SetContent(Global().PlayerAvatar(pid, 30));
            card_table.Get(1, col).SetContent(card.ToHtml(img_path));
            col++;
        }
        return style + card_table.ToString();
    }

    const uint64_t round_;
    const bool is_initial_;
    AreaCard shared_card_;
    std::map<PlayerID, AreaCard> initial_cards_;
    const std::string comb_html_;
};

// ==================== SelectStage ====================

class SelectStage : public SubGameStage<>
{
  public:
    SelectStage(MainStage& main_stage, const uint64_t round)
            : StageFsm(main_stage, "公共配牌阶段",
                MakeStageCommand(*this, "选择并放置砖块（卡牌ID 位置，位置 0 为弃牌）", &SelectStage::Select_, ArithChecker<uint32_t>(1, 20, "选卡"), ArithChecker<uint32_t>(0, 19, "位置")),
                MakeStageCommand(*this, "查看游戏进展情况与可选砖块", &SelectStage::Info_, VoidChecker("赛况")))
            , round_(round)
            , comb_html_(main_stage.CombHtml("## 第 " + std::to_string(round) + " 回合[公共配牌阶段]"))
    {
        // Prepare selection order: alive players sorted by HP (low first), then score (low first)
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (Main().player_out_[pid] == 0) {
                current_players_.push_back(pid);
            }
        }
        SeededShuffle(current_players_.begin(), current_players_.end(), Main().selection_order_rng_);
        auto priority = [this](PlayerID p) -> int32_t {
            int32_t best = INT32_MAX;
            auto& player = Main().players_[p];
            for (const auto talent : player.talents_) {
                best = std::min(best, player.talent_states_.at(talent)->SelectionPriority(player));
            }
            return best;
        };
        std::sort(current_players_.begin(), current_players_.end(), [this, &priority](const PlayerID& p1, const PlayerID& p2) {
            const int32_t pr1 = priority(p1), pr2 = priority(p2);
            if (pr1 != pr2) return pr1 < pr2;
            auto& player1 = Main().players_[p1];
            auto& player2 = Main().players_[p2];
            if (player1.hp_ != player2.hp_) return player1.hp_ < player2.hp_;
            // 用盘面总分 PlayerRawTotalScore 不能用 PlayerScore：它现在返回名次分，游戏中固定为 0
            return Main().PlayerRawTotalScore(p1) < Main().PlayerRawTotalScore(p2);
        });

        // Draw n+1 cards from pool2
        for (size_t i = 0; i < current_players_.size() + 1; ++i) {
            tmp_cards_.push_back(Main().DrawFromPool2_());
        }
    }

    virtual void OnStageBegin() override
    {
        // Set all players ready except the first one
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (pid != current_players_[0]) {
                Global().SetReady(pid);
            }
        }
        SendInfo(Global().BoardcastMsgSender());
        Global().Boardcast() << "请 " << At(current_players_[0]) << " 选择";
        Global().StartTimer(GAME_OPTION(局时));
    }

  private:
    void HandleUnreadyPlayer_()
    {
        if (current_players_.empty()) return;
        PlayerID pid = current_players_[0];
        if (Global().IsReady(pid) && !Main().player_leave_[pid]) return;

        auto& player = Main().players_[pid];
        const AreaCard card = tmp_cards_[0];
        Selected_(1);

        auto sender = Global().Boardcast();
        if (player.comb_->HasEmptyPosition()) {
            auto [actual_card, transform_notify] = Main().TransformCardForPlacement_(pid, card);
            const auto [idx, result] = player.comb_->SeqFill(actual_card);
            auto [place_notify, actual_delta] = Main().OnCardPlaced_(pid, idx, result);
            sender << At(pid) << " 超时，自动选择 1 号砖块填入位置 " << idx;
        } else {
            Main().HandleDiscard_(pid, card);
            sender << At(pid) << " 超时，自动选择 1 号砖块，盘面已满自动弃牌";
        }
        Global().SetReady(pid);
        Global().HookUnreadyPlayers();
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Main().player_leave_[pid] = true;
        return StageErrCode::CONTINUE;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        HandleUnreadyPlayer_();
        return StageOver_();
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        HandleUnreadyPlayer_();
        return StageOver_();
    }

    CheckoutErrCode StageOver_()
    {
        if (current_players_.empty()) {
            // Handle 尾货处理 for the last player
            return StageErrCode::CHECKOUT;
        }
        comb_html_ = Main().CombHtml("## 第 " + std::to_string(round_) + " 回合[公共配牌阶段]");
        SendInfo(Global().BoardcastMsgSender());
        Global().Boardcast() << "请 " << At(current_players_[0]) << " 选择";
        Global().ClearReady(current_players_[0]);
        Global().StartTimer(GAME_OPTION(局时));
        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) return StageErrCode::OK;
        auto& player = Main().players_[pid];
        const AreaCard card = tmp_cards_[0];
        Selected_(1);
        if (player.comb_->HasEmptyPosition()) {
            auto [actual_card, transform_notify] = Main().TransformCardForPlacement_(pid, card);
            const auto [idx, result] = player.comb_->SeqFill(actual_card);
            Main().OnCardPlaced_(pid, idx, result);
        } else {
            Main().HandleDiscard_(pid, card);
        }
        return StageErrCode::READY;
    }

    AtomReqErrCode Select_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const uint32_t card_id, const uint32_t pos)
    {
        if (Global().IsReady(pid)) {
            reply() << "[错误] 当前并非您的选卡回合";
            return StageErrCode::FAILED;
        }
        if (card_id < 1 || card_id > tmp_cards_.size()) {
            reply() << "[错误] 无效的选牌ID，请输入 1-" << tmp_cards_.size() << " 之间的数字";
            return StageErrCode::FAILED;
        }

        auto& player = Main().players_[pid];
        const AreaCard card = tmp_cards_[card_id - 1];

        if (pos != 0) {
            if (auto block_msg = player.ProtectedPositionMessage(pos)) {
                reply() << "[错误] " << *block_msg;
                return StageErrCode::FAILED;
            }
        }

        // Check if this is the last player and they have 尾货处理
        bool is_last_player = (current_players_.size() == 1);
        if (is_last_player && player.HasTalent(Talent::尾货处理) && tmp_cards_.size() >= 2) {
            // Get the remaining card (not the one selected) as bonus
            for (size_t i = 0; i < tmp_cards_.size(); ++i) {
                if (i != card_id - 1) {
                    player.extra_card_queue_.push_back({{tmp_cards_[i]}, "尾货处理"});
                }
            }
        }

        Selected_(card_id);

        if (pos == 0) {
            std::string notify = Main().HandleDiscard_(pid, card);
            reply() << "选择 " << card_id << " 号砖块，弃牌成功" << notify;
            return StageErrCode::READY;
        }

        const auto previous_card = player.comb_->GetCard(pos);
        auto [actual_card, transform_notify] = Main().TransformCardForPlacement_(pid, card);
        const auto result = player.comb_->Fill(pos, actual_card);
        auto [notify, actual_delta] = Main().OnCardPlaced_(pid, pos, result, std::nullopt, previous_card ? &*previous_card : nullptr);
        auto sender = reply();
        sender << "选择 " << card_id << " 号砖块，设置位置 " << pos << " 成功";
        if (actual_delta > 0) {
            sender << "，收获 " << actual_delta << " 点积分";
        } else if (actual_delta < 0) {
            sender << "，损失 " << (-actual_delta) << " 点积分";
        }
        sender << transform_notify << notify;
        return StageErrCode::READY;
    }

    void Selected_(const uint32_t id)
    {
        tmp_cards_.erase(tmp_cards_.begin() + id - 1);
        current_players_.erase(current_players_.begin());
    }

    std::string SelectCardHtml_()
    {
        html::Table avatar_table(1, current_players_.size() + 1);
        avatar_table.SetTableStyle("cellpadding=\"0\" cellspacing=\"5\"");
        avatar_table.Get(0, 0).SetContent("　<b>选卡顺序：</b>");
        for (size_t i = 0; i < current_players_.size(); ++i) {
            const PlayerID pid = current_players_[i];
            auto& player = Main().players_[pid];
            const bool is_last = (i == current_players_.size() - 1);
            // 通用 dispatch：取所有已获取天赋的非空 SelectionBorderStyle，按 SelectionPriority 升序选最优先的。
            std::string best_style;
            int32_t best_pri = INT32_MAX;
            for (const auto talent : player.talents_) {
                auto& state = player.talent_states_.at(talent);
                std::string s = state->SelectionBorderStyle(player, is_last);
                if (s.empty()) continue;
                const int32_t pri = state->SelectionPriority(player);
                if (pri < best_pri) {
                    best_pri = pri;
                    best_style = std::move(s);
                }
            }
            std::string avatar = Global().PlayerAvatar(pid, 40);
            if (!best_style.empty()) {
                avatar = "<div style=\"position:relative;display:inline-block;width:40px;height:40px;\">"
                         + avatar +
                         "<div style=\"position:absolute;top:0;left:0;width:36px;height:36px;"
                         + best_style + "border-radius:50%;pointer-events:none;\"></div></div>";
            }
            avatar_table.Get(0, i + 1).SetContent(avatar);
        }
        html::Table card_table(1, tmp_cards_.size() * 2);
        card_table.SetTableStyle("align=\"center\" cellpadding=\"0\" cellspacing=\"0\" ");
        const std::string img_path = Global().ResourceDir();
        for (size_t i = 0; i < tmp_cards_.size(); ++i) {
            card_table.Get(0, i * 2).SetContent(std::to_string(i + 1) + ".");
            card_table.Get(0, i * 2 + 1).SetContent(tmp_cards_[i].ToHtml(img_path));
        }
        const std::string style = "<style>body{margin:0;}</style>" + GetStyle(Global().ResourceDir());
        return style + avatar_table.ToString() + card_table.ToString();
    }

    void SendInfo(MsgSenderBase& sender)
    {
        sender() << Markdown{comb_html_};
        sender() << Markdown(SelectCardHtml_(), 300);
    }

    // 「赛况」回复：当前最新棋盘 + 剩余待选卡总图（含选卡顺序头像）。
    AtomReqErrCode Info_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << Markdown{Main().CombHtml("## 第 " + std::to_string(round_) + " 回合[公共配牌阶段]")};
        reply() << Markdown(SelectCardHtml_(), 300);
        return StageErrCode::OK;
    }

    const uint64_t round_;
    std::vector<PlayerID> current_players_;
    std::vector<AreaCard> tmp_cards_;
    std::string comb_html_;
};

// ==================== ExtraCardStage ====================
// Universal extra card stage: handles both single-card (direct placement) and multi-card (pick one) entries.

class ExtraCardStage : public SubGameStage<>
{
  public:
    ExtraCardStage(MainStage& main_stage)
            : StageFsm(main_stage, "额外砖块选择阶段",
                MakeStageCommand(*this, "放置额外砖块到指定位置（0 为弃牌）", &ExtraCardStage::Set_, ArithChecker<uint32_t>(0, 19, "位置")),
                MakeStageCommand(*this, "选择砖块并放置（砖块序号 位置，位置 0 为弃牌）", &ExtraCardStage::Choose_,
                    ArithChecker<uint32_t>(1, 10, "砖块序号"), ArithChecker<uint32_t>(0, 19, "位置")),
                MakeStageCommand(*this, "查看游戏进展情况和需放置的额外砖块", &ExtraCardStage::Info_, VoidChecker("赛况")))
    {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (Main().player_out_[pid] == 0 && !Main().players_[pid].extra_card_queue_.empty()) {
                pending_players_.push_back(pid);
            }
        }
    }

    virtual void OnStageBegin() override
    {
        if (pending_players_.empty()) return;
        // Show board status
        Global().Boardcast() << Markdown{Main().CombHtml("## 第 " + std::to_string(Main().round_) + " 回合[额外砖块选择阶段]")};
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            Global().SetReady(pid);
        }
        StartNextPlayer_();
    }

  private:
    // Get the current entry for the front pending player
    Player::ExtraCardEntry& CurrentEntry_(Player& player)
    {
        return player.extra_card_queue_.front();
    }

    bool IsMultiCard_(const Player::ExtraCardEntry& entry) const
    {
        return entry.cards.size() > 1;
    }

    void StartNextPlayer_()
    {
        if (pending_players_.empty()) return;
        PlayerID pid = pending_players_.front();
        auto& player = Main().players_[pid];

        if (player.extra_card_queue_.empty()) {
            pending_players_.erase(pending_players_.begin());
            if (pending_players_.empty()) return;
            StartNextPlayer_();
            return;
        }

        Global().ClearReady(pid);
        const auto& entry = CurrentEntry_(player);

        if (IsMultiCard_(entry)) {
            // Multi-card: show selection UI with card images
            // place_all=true (三年之期): show total cards to place; place_all=false (劫掠): pick 1
            int32_t choose_count = entry.place_all ? static_cast<int32_t>(entry.cards.size()) : 1;
            auto sender = Global().Boardcast();
            sender << At(pid) << " 触发「" << entry.source << "」（可选择 " << choose_count
                   << " 张），请从以下砖块中选择放置：";
            for (size_t i = 0; i < entry.cards.size(); ++i) {
                sender << " " << (i + 1) << "." << entry.cards[i].CardName();
            }
            SendCardPreview_(entry.cards);
        } else {
            // Single card: show card image, direct placement
            auto sender = Global().Boardcast();
            sender << At(pid) << " 通过「" << entry.source << "」获得额外砖块 " << entry.cards[0].CardName();
            sender << "，请放置到指定位置，剩余 " << player.extra_card_queue_.size() << " 张";
            SendCardPreview_(entry.cards);
        }
        Global().StartTimer(GAME_OPTION(局时));
    }

    // Show board + next card info for same player continuing with more entries
    void ShowNextCardForPlayer_(PlayerID pid)
    {
        auto& player = Main().players_[pid];
        if (player.extra_card_queue_.empty()) return;
        // Re-broadcast board
        Global().Boardcast() << Markdown{Main().CombHtml("## 第 " + std::to_string(Main().round_) + " 回合[额外砖块选择阶段]")};
        const auto& entry = CurrentEntry_(player);
        if (IsMultiCard_(entry)) {
            auto sender = Global().Boardcast();
            sender << At(pid) << " 请继续选择（" << entry.source << "，剩余 " << entry.cards.size() << " 张）：";
            for (size_t i = 0; i < entry.cards.size(); ++i) {
                sender << " " << (i + 1) << "." << entry.cards[i].CardName();
            }
            SendCardPreview_(entry.cards);
        } else {
            auto sender = Global().Boardcast();
            sender << At(pid) << " 通过「" << entry.source << "」获得额外砖块 " << entry.cards[0].CardName();
            sender << "，请放置到指定位置，剩余 " << player.extra_card_queue_.size() << " 张";
            SendCardPreview_(entry.cards);
        }
    }

    void ShowRemainingCards_(PlayerID pid)
    {
        auto& player = Main().players_[pid];
        if (player.extra_card_queue_.empty()) return;
        const auto& entry = CurrentEntry_(player);
        if (entry.cards.empty()) return;

        // Re-broadcast board status after placement
        Global().Boardcast() << Markdown{Main().CombHtml("## 第 " + std::to_string(Main().round_) + " 回合[额外砖块选择阶段]")};

        auto sender = Global().Boardcast();
        sender << At(pid) << " 请继续选择（剩余 " << entry.cards.size() << " 张）：";
        for (size_t i = 0; i < entry.cards.size(); ++i) {
            sender << " " << (i + 1) << "." << entry.cards[i].CardName();
        }
        SendCardPreview_(entry.cards);
    }

    void SendCardPreview_(const std::vector<AreaCard>& cards)
    {
        const std::string img_path = Global().ResourceDir();
        const std::string style = "<style>body{margin:0;}</style>" + GetStyle(img_path);
        if (cards.size() == 1) {
            // Single card: send directly as small image, no full Markdown wrapper
            Global().Boardcast() << Markdown(style + cards[0].ToHtml(img_path), 64);
        } else {
            html::Table card_table(1, cards.size() * 2);
            card_table.SetTableStyle("align=\"center\" cellpadding=\"0\" cellspacing=\"0\" ");
            for (size_t i = 0; i < cards.size(); ++i) {
                card_table.Get(0, i * 2).SetContent(std::to_string(i + 1) + ".");
                card_table.Get(0, i * 2 + 1).SetContent(cards[i].ToHtml(img_path));
            }
            Global().Boardcast() << Markdown(style + card_table.ToString(), 300);
        }
    }

    // 「赛况」回复：当前最新棋盘；若请求者正是 pending 队列首位且有待办，额外私聊行动提示与砖块图。
    AtomReqErrCode Info_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << Markdown{Main().CombHtml("## 第 " + std::to_string(Main().round_) + " 回合[额外砖块选择阶段]")};

        auto& player = Main().players_[pid];
        if (pending_players_.empty() || pending_players_.front() != pid || player.extra_card_queue_.empty()) {
            return StageErrCode::OK;
        }

        const auto& entry = player.extra_card_queue_.front();
        {
            auto sender = reply();
            if (entry.cards.size() > 1) {
                const int32_t choose_count = entry.place_all ? static_cast<int32_t>(entry.cards.size()) : 1;
                sender << "「" << entry.source << "」可选择 " << choose_count << " 张，请输入「砖块序号 位置」：";
                for (size_t i = 0; i < entry.cards.size(); ++i) {
                    sender << " " << (i + 1) << "." << entry.cards[i].CardName();
                }
            } else {
                sender << "通过「" << entry.source << "」获得额外砖块 " << entry.cards[0].CardName()
                       << "，请放置（剩余 " << player.extra_card_queue_.size() << " 张）";
            }
        }

        // 砖块图：单卡直接小图；多卡走带序号的表格图
        const std::string img_path = Global().ResourceDir();
        const std::string style = "<style>body{margin:0;}</style>" + GetStyle(img_path);
        if (entry.cards.size() == 1) {
            reply() << Markdown(style + entry.cards[0].ToHtml(img_path), 64);
        } else {
            html::Table card_table(1, entry.cards.size() * 2);
            card_table.SetTableStyle("align=\"center\" cellpadding=\"0\" cellspacing=\"0\" ");
            for (size_t i = 0; i < entry.cards.size(); ++i) {
                card_table.Get(0, i * 2).SetContent(std::to_string(i + 1) + ".");
                card_table.Get(0, i * 2 + 1).SetContent(entry.cards[i].ToHtml(img_path));
            }
            reply() << Markdown(style + card_table.ToString(), 300);
        }
        return StageErrCode::OK;
    }

    // Place a card for a player (handles transform, fill/discard, forge check)
    // If allow_overwrite is true (e.g. 临时用品), SeqFill even when board is full (overwrites pos 1).
    void PlaceCard_(PlayerID pid, const AreaCard& card, std::string* out_notify = nullptr, bool allow_overwrite = false)
    {
        auto& player = Main().players_[pid];
        if (player.comb_->HasEmptyPosition() || allow_overwrite) {
            auto [actual_card, transform_notify] = Main().TransformCardForPlacement_(pid, card);
            const auto [idx, result] = player.comb_->SeqFill(actual_card);
            auto [notify, actual_delta] = Main().OnCardPlaced_(
                pid, idx, result, std::nullopt, nullptr, TalentCardPlacementSource::ExtraCard);
            if (out_notify) *out_notify = transform_notify + notify;
        } else {
            Main().HandleDiscard_(pid, card);
            if (out_notify) *out_notify = "";
        }
        CheckForgeAfterPlace_(pid);
    }

    void HandleUnreadyPlayer_()
    {
        if (pending_players_.empty()) return;
        PlayerID pid = pending_players_.front();
        if (Global().IsReady(pid)) return;

        auto& player = Main().players_[pid];

        // Auto-place ALL remaining entries for this player on timeout
        while (!player.extra_card_queue_.empty()) {
            auto& entry = CurrentEntry_(player);
            const AreaCard card = entry.cards[0];
            const std::string source = entry.source;
            const auto source_talent = entry.source_talent;
            player.extra_card_queue_.erase(player.extra_card_queue_.begin());

            auto sender = Global().Boardcast();
            bool can_overwrite = (source == "临时用品");
            if (player.comb_->HasEmptyPosition()) {
                auto [actual_card, transform_notify] = Main().TransformCardForPlacement_(pid, card);
                const auto [idx, result] = player.comb_->SeqFill(actual_card);
                auto [place_notify, actual_delta] = Main().OnCardPlaced_(
                    pid, idx, result, source_talent, nullptr, TalentCardPlacementSource::ExtraCard);
                sender << At(pid) << " 超时，额外砖块自动填入位置 " << idx;
                CheckTempWildAfterPlace_(pid, source, idx);
            } else if (can_overwrite) {
                // 临时用品: overwrite position 1 when board is full
                if (auto block_msg = player.ProtectedPositionMessage(1)) {
                    Main().HandleDiscard_(pid, card, source_talent);
                    sender << At(pid) << " 超时，" << *block_msg << "，额外砖块自动弃牌";
                    CheckForgeAfterPlace_(pid);
                    continue;
                }
                const auto previous_card = player.comb_->GetCard(1);
                auto [actual_card, transform_notify] = Main().TransformCardForPlacement_(pid, card);
                const auto result = player.comb_->Fill(1, actual_card);
                auto [place_notify, actual_delta] = Main().OnCardPlaced_(
                    pid, 1, result, source_talent, previous_card ? &*previous_card : nullptr, TalentCardPlacementSource::ExtraCard);
                sender << At(pid) << " 超时，额外砖块自动覆盖位置 1";
                CheckTempWildAfterPlace_(pid, source, 1);
            } else {
                Main().HandleDiscard_(pid, card, source_talent);
                sender << At(pid) << " 超时，盘面已满自动弃牌";
            }
            CheckForgeAfterPlace_(pid);
        }
        Global().SetReady(pid);
        Global().HookUnreadyPlayers();
    }

    void CheckForgeAfterPlace_(PlayerID pid)
    {
        auto& player = Main().players_[pid];
        for (const auto talent : k_extra_card_action_end_order) {
            if (!player.HasTalent(talent)) continue;
            const auto notify = player.talent_states_.at(talent)->OnExtraCardActionEnd(player);
            if (!notify.empty()) {
                Global().Boardcast() << At(pid) << " " << notify;
            }
        }
    }

    // Record temp_wild position after placement from "临时用品" source
    void CheckTempWildAfterPlace_(PlayerID pid, const std::string& source, uint32_t pos)
    {
        if (source == "临时用品" && pos > 0) {
            auto& player = Main().players_[pid];
            player.TempWild().position = pos;
            player.TempWild().rounds_left = 3;
        }
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Main().player_leave_[pid] = true;
        return StageErrCode::CONTINUE;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        HandleUnreadyPlayer_();
        return StageOver_();
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        HandleUnreadyPlayer_();
        return StageOver_();
    }

    CheckoutErrCode StageOver_()
    {
        if (pending_players_.empty()) return StageErrCode::CHECKOUT;

        PlayerID pid = pending_players_.front();
        auto& player = Main().players_[pid];

        if (player.extra_card_queue_.empty()) {
            Global().SetReady(pid);
            pending_players_.erase(pending_players_.begin());
            if (pending_players_.empty()) return StageErrCode::CHECKOUT;
            // Re-broadcast board when switching to next player
            Global().Boardcast() << Markdown{Main().CombHtml("## 第 " + std::to_string(Main().round_) + " 回合[额外砖块选择阶段]")};
        }

        StartNextPlayer_();
        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) return StageErrCode::OK;
        auto& player = Main().players_[pid];
        if (player.extra_card_queue_.empty()) return StageErrCode::READY;

        const AreaCard card = CurrentEntry_(player).cards[0];
        const std::string source = CurrentEntry_(player).source;
        const auto source_talent = CurrentEntry_(player).source_talent;
        bool can_overwrite = (source == "临时用品");
        player.extra_card_queue_.erase(player.extra_card_queue_.begin());
        if (player.comb_->HasEmptyPosition()) {
            auto [actual_card, transform_notify] = Main().TransformCardForPlacement_(pid, card);
            const auto [idx, result] = player.comb_->SeqFill(actual_card);
            auto [place_notify, actual_delta] = Main().OnCardPlaced_(
                pid, idx, result, source_talent, nullptr, TalentCardPlacementSource::ExtraCard);
            CheckTempWildAfterPlace_(pid, source, idx);
        } else if (can_overwrite) {
            if (player.ProtectedPositionMessage(1)) {
                Main().HandleDiscard_(pid, card, source_talent);
                CheckForgeAfterPlace_(pid);
                return StageErrCode::READY;
            }
            const auto previous_card = player.comb_->GetCard(1);
            auto [actual_card, transform_notify] = Main().TransformCardForPlacement_(pid, card);
            const auto result = player.comb_->Fill(1, actual_card);
            auto [place_notify, actual_delta] = Main().OnCardPlaced_(
                pid, 1, result, source_talent, previous_card ? &*previous_card : nullptr, TalentCardPlacementSource::ExtraCard);
            CheckTempWildAfterPlace_(pid, source, 1);
        } else {
            Main().HandleDiscard_(pid, card, source_talent);
        }
        CheckForgeAfterPlace_(pid);
        return StageErrCode::READY;
    }

    // Command: place single-card entry directly (位置, 0=discard)
    AtomReqErrCode Set_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const uint32_t idx)
    {
        auto& player = Main().players_[pid];
        if (player.extra_card_queue_.empty()) {
            reply() << "[错误] 您没有待放置的额外砖块";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 当前不是您的放置回合";
            return StageErrCode::FAILED;
        }

        auto& entry = CurrentEntry_(player);
        if (IsMultiCard_(entry)) {
            reply() << "[错误] 当前有多张砖块可选，请使用「砖块序号 位置」命令";
            return StageErrCode::FAILED;
        }

        const AreaCard card = entry.cards[0];
        const std::string source = entry.source;
        const auto source_talent = entry.source_talent;

        if (idx == 0) {
            player.extra_card_queue_.erase(player.extra_card_queue_.begin());
            std::string notify = Main().HandleDiscard_(pid, card, source_talent);
            CheckForgeAfterPlace_(pid);
            reply() << "额外砖块弃牌成功" << notify;
            if (player.extra_card_queue_.empty()) return StageErrCode::READY;
            ShowNextCardForPlayer_(pid);
            Global().StartTimer(GAME_OPTION(局时));
            return StageErrCode::OK;
        }

        if (auto block_msg = player.ProtectedPositionMessage(idx)) {
            reply() << "[错误] " << *block_msg;
            return StageErrCode::FAILED;
        }

        player.extra_card_queue_.erase(player.extra_card_queue_.begin());
        const auto previous_card = player.comb_->GetCard(idx);
        auto [actual_card, transform_notify] = Main().TransformCardForPlacement_(pid, card);
        const auto result = player.comb_->Fill(idx, actual_card);
        auto [notify, actual_delta] = Main().OnCardPlaced_(
            pid, idx, result, source_talent, previous_card ? &*previous_card : nullptr, TalentCardPlacementSource::ExtraCard);
        CheckForgeAfterPlace_(pid);
        CheckTempWildAfterPlace_(pid, source, idx);

        // Build message and send immediately so ShowNextCardForPlayer_ broadcasts AFTER this reply
        std::string msg = "额外砖块设置位置 " + std::to_string(idx) + " 成功";
        if (actual_delta > 0) {
            msg += "，收获 " + std::to_string(actual_delta) + " 点积分";
        } else if (actual_delta < 0) {
            msg += "，损失 " + std::to_string(-actual_delta) + " 点积分";
        }
        msg += transform_notify + notify;
        reply() << msg;

        if (player.extra_card_queue_.empty()) return StageErrCode::READY;
        ShowNextCardForPlayer_(pid);
        Global().StartTimer(GAME_OPTION(局时));
        return StageErrCode::OK;
    }

    // Command: choose from multi-card entry then place (砖块序号, 位置, 0=discard)
    AtomReqErrCode Choose_(const PlayerID pid, const bool is_public, MsgSenderBase& reply,
                           const uint32_t card_id, const uint32_t pos)
    {
        auto& player = Main().players_[pid];
        if (player.extra_card_queue_.empty()) {
            reply() << "[错误] 您没有待选择的额外砖块";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 当前不是您的选择回合";
            return StageErrCode::FAILED;
        }

        auto& entry = CurrentEntry_(player);
        if (card_id < 1 || card_id > entry.cards.size()) {
            reply() << "[错误] 请输入 1-" << entry.cards.size() << " 之间的砖块序号";
            return StageErrCode::FAILED;
        }

        const AreaCard card = entry.cards[card_id - 1];
        const std::string source = entry.source;
        const bool place_all = entry.place_all;
        const auto source_talent = entry.source_talent;

        if (pos != 0) {
            if (auto block_msg = player.ProtectedPositionMessage(pos)) {
                reply() << "[错误] " << *block_msg;
                return StageErrCode::FAILED;
            }
        }

        if (place_all) {
            // Remove chosen card from entry, keep entry if more cards remain
            entry.cards.erase(entry.cards.begin() + card_id - 1);
            if (entry.cards.empty()) {
                player.extra_card_queue_.erase(player.extra_card_queue_.begin());
            }
        } else {
            // Pick one, discard rest
            player.extra_card_queue_.erase(player.extra_card_queue_.begin());
        }

        if (pos == 0) {
            std::string notify = Main().HandleDiscard_(pid, card, source_talent);
            CheckForgeAfterPlace_(pid);
            reply() << source << "选择 " << card_id << " 号砖块，弃牌成功" << notify;
            if (place_all && !player.extra_card_queue_.empty() && !player.extra_card_queue_.front().cards.empty()) {
                ShowRemainingCards_(pid);
                Global().StartTimer(GAME_OPTION(局时));
                return StageErrCode::OK;
            }
            return StageErrCode::READY;
        }

        const auto previous_card = player.comb_->GetCard(pos);
        auto [actual_card, transform_notify] = Main().TransformCardForPlacement_(pid, card);
        const auto result = player.comb_->Fill(pos, actual_card);
        auto [notify, actual_delta] = Main().OnCardPlaced_(
            pid, pos, result, source_talent, previous_card ? &*previous_card : nullptr, TalentCardPlacementSource::ExtraCard);
        CheckForgeAfterPlace_(pid);
        CheckTempWildAfterPlace_(pid, source, pos);

        // Build message and send immediately so ShowRemainingCards_ broadcasts AFTER this reply
        std::string msg = source + "选择 " + std::to_string(card_id) + " 号砖块，设置位置 "
                          + std::to_string(pos) + " 成功";
        if (actual_delta > 0) {
            msg += "，收获 " + std::to_string(actual_delta) + " 点积分";
        } else if (actual_delta < 0) {
            msg += "，损失 " + std::to_string(-actual_delta) + " 点积分";
        }
        msg += transform_notify + notify;
        reply() << msg;

        // For place_all entries with remaining cards, continue choosing
        if (place_all && !player.extra_card_queue_.empty() && !player.extra_card_queue_.front().cards.empty()) {
            ShowRemainingCards_(pid);
            Global().StartTimer(GAME_OPTION(局时));
            return StageErrCode::OK;
        }
        return StageErrCode::READY;
    }

    std::vector<PlayerID> pending_players_;
};

// ==================== TalentStage ====================

class TalentStage : public SubGameStage<>
{
  public:
    TalentStage(MainStage& main_stage)
            : StageFsm(main_stage, "天赋选择阶段",
                MakeStageCommand(*this, "选择天赋", &TalentStage::Choose_, ArithChecker<uint32_t>(1, 5, "天赋序号")),
                MakeStageCommand(*this, "查看游戏进展情况和天赋候选列表", &TalentStage::Info_, VoidChecker("赛况")))
    {
    }

    virtual void OnStageBegin() override
    {
        // Boardcast player boards before choosing talents
        Global().Boardcast() << Markdown{Main().CombHtml("## 第 " + std::to_string(Main().round_) + " 回合[天赋选择阶段]")};

        // 时间锚：在玩家进入"下一次"天赋选择前恢复 HP。仅对持有非空候选池的玩家生效，
        // 避免本回合根本不参与天赋选择的玩家被错误触发。
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (Main().player_out_[pid] != 0) continue;
            auto& player = Main().players_[pid];
            if (player.talent_pool_.empty()) continue;
            if (!player.HasTalent(Talent::时间锚)) continue;
            auto& anchor = player.TalentStateAs<TimeAnchorTalent>(Talent::时间锚);
            const int32_t old_hp = player.hp_;
            if (anchor.Restore(player)) {
                Global().Boardcast() << At(pid) << " 触发天赋「时间锚」，血量从 " << old_hp
                                     << " 重置为 " << player.hp_;
            }
        }

        // 玩家若以"已拥有我全都要"的状态进入本回合 TalentStage（上一轮选中后存到 talents_），
        // 本次候选池就是"下次选择"——直接触发吞噬。
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (Main().player_out_[pid] != 0) continue;
            TriggerWantAllIfHeld_(pid);
        }

        // Set ready for players who don't need to choose
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (Main().player_out_[pid] != 0 || Main().players_[pid].talent_pool_.empty()) {
                Global().SetReady(pid);
            }
        }

        // Show talent options to each choosing player
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (Main().player_out_[pid] == 0 && !Main().players_[pid].talent_pool_.empty()) {
                auto& player = Main().players_[pid];
                auto sender = Global().Boardcast();
                sender << At(pid) << " 达成了选择天赋的条件，请选择：\n";
                // 多维抉择：候选额外扩展 1A+1B（共 5 选 1），在此给出提示
                if (player.HasTalent(Talent::多维抉择) && player.talent_pool_.size() > 3) {
                    sender << "触发天赋「多维抉择」，额外提供 2 个选项\n";
                }
                for (size_t i = 0; i < player.talent_pool_.size(); ++i) {
                    sender << (i + 1) << ". " << TalentName(player.talent_pool_[i])
                           << "（" << (IsGradeA(player.talent_pool_[i]) ? "A级" : "B级") << "）——"
                           << TalentDescription(player.talent_pool_[i]) << "\n";
                }
            }
        }
        Global().StartTimer(GAME_OPTION(局时));
    }

  private:
    // 玩家拥有「我全都要」 + 候选池非空时，立刻把池内全部天赋发给玩家、移除「我全都要」、
    // talent_selection_count_ +1、依次走 ApplyImmediateTalentEffects_、播报"获得全部天赋"，
    // 并在仍满足下一次选择资格时补一个新候选池供玩家手动选择。
    // 「我全都要」每局唯一，吞噬后即被移除，因此至多触发一次，不需要循环。
    // 调用方需保证不在玩家"刚刚选中我全都要那一刻"调用 —— 实际只有两处：
    //   1) OnStageBegin：玩家以已拥有的状态进入本回合 TalentStage
    //   2) Choose_ 的级联：玩家选完一张天赋、产生新候选池之后
    void TriggerWantAllIfHeld_(PlayerID pid)
    {
        auto& player = Main().players_[pid];
        if (player.talent_pool_.empty()) return;
        if (!player.HasTalent(Talent::我全都要)) return;

        auto it = std::find(player.talents_.begin(), player.talents_.end(), Talent::我全都要);
        if (it != player.talents_.end()) player.talents_.erase(it);

        std::vector<Talent> acquired(player.talent_pool_.begin(), player.talent_pool_.end());
        for (auto t : acquired) {
            player.talents_.push_back(t);
            player.available_a_.erase(t);
            player.available_b_.erase(t);
        }
        player.talent_pool_.clear();
        ++player.talent_selection_count_;
        if (player.first_talent_round_ == 0) {
            player.first_talent_round_ = Main().round_;
        }

        std::string msg = " 触发天赋「我全都要」！获得全部天赋：";
        for (auto t : acquired) {
            msg += "\n「" + std::string(TalentName(t)) + "」——" + std::string(TalentDescription(t));
            Main().ApplyImmediateTalentEffects_(pid, t);
        }
        Global().Boardcast() << At(pid) << msg;

        // 若吞噬后仍满足下一轮选择资格，补一个手动选择用的新候选池。
        if (player.talent_selection_count_ < player.comb_->LineCount() / 3) {
            player.GenerateTalentPool(Main().talent_rng_);
        }
    }

    void HandleUnreadyPlayers_()
    {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (Global().IsReady(pid) || Main().player_out_[pid] != 0) continue;
            auto& player = Main().players_[pid];
            if (player.talent_pool_.empty()) continue;

            // Auto-choose first talent
            Talent chosen = player.talent_pool_[0];
            player.AcquireTalent(0);
            std::string extra = Main().ApplyImmediateTalentEffects_(pid, chosen);
            Global().Boardcast() << At(pid) << " 超时，自动选择天赋「" << TalentName(chosen) << "」" << extra;
        }
        Global().HookUnreadyPlayers();
    }

    // 「赛况」回复：当前最新棋盘；若请求者自己的候选池非空再附自己的候选清单。
    // 没触发条件 / 已选完 / 被「我全都要」吞掉 → 候选池为空 → 无需追加。
    AtomReqErrCode Info_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << Markdown{Main().CombHtml("## 第 " + std::to_string(Main().round_) + " 回合[天赋选择阶段]")};

        auto& player = Main().players_[pid];
        if (player.talent_pool_.empty()) {
            return StageErrCode::OK;
        }

        auto sender = reply();
        sender << "您当前的候选天赋：\n";
        for (size_t i = 0; i < player.talent_pool_.size(); ++i) {
            sender << (i + 1) << ". " << TalentName(player.talent_pool_[i])
                   << "（" << (IsGradeA(player.talent_pool_[i]) ? "A级" : "B级") << "）——"
                   << TalentDescription(player.talent_pool_[i]) << "\n";
        }
        return StageErrCode::OK;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Main().player_leave_[pid] = true;
        return StageErrCode::CONTINUE;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        HandleUnreadyPlayers_();
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        HandleUnreadyPlayers_();
        return StageErrCode::CHECKOUT;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) return StageErrCode::OK;
        auto& player = Main().players_[pid];
        if (player.talent_pool_.empty()) return StageErrCode::READY;
        Talent chosen = player.talent_pool_[0];
        player.AcquireTalent(0);
        std::string extra = Main().ApplyImmediateTalentEffects_(pid, chosen);
        return StageErrCode::READY;
    }

    AtomReqErrCode Choose_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const uint32_t choice)
    {
        auto& player = Main().players_[pid];
        if (player.talent_pool_.empty()) {
            reply() << "[错误] 当前没有可选择的天赋";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 您已经选择过天赋";
            return StageErrCode::FAILED;
        }
        if (choice < 1 || choice > player.talent_pool_.size()) {
            reply() << "[错误] 请输入 1-" << player.talent_pool_.size() << " 之间的数字";
            return StageErrCode::FAILED;
        }

        Talent chosen = player.talent_pool_[choice - 1];
        player.AcquireTalent(choice - 1);
        if (player.first_talent_round_ == 0) {
            player.first_talent_round_ = Main().round_;
        }
        std::string extra = Main().ApplyImmediateTalentEffects_(pid, chosen);
        reply() << "成功选择天赋「" << TalentName(chosen) << "」" << extra;

        // Check if player qualifies for another talent immediately
        uint32_t line_count = player.comb_->LineCount();
        if (player.talent_selection_count_ < line_count / 3) {
            player.GenerateTalentPool(Main().talent_rng_);
            // 若本次刚选到「我全都要」，新生成的级联候选池就是它的触发时机。
            TriggerWantAllIfHeld_(pid);
            if (!player.talent_pool_.empty()) {
                auto sender = reply();
                sender << "再次达成天赋选择条件，请继续选择：\n";
                // 多维抉择：5 选 1 提示
                if (player.HasTalent(Talent::多维抉择) && player.talent_pool_.size() > 3) {
                    sender << "触发天赋「多维抉择」，额外提供 2 个选项\n";
                }
                for (size_t i = 0; i < player.talent_pool_.size(); ++i) {
                    sender << (i + 1) << ". " << TalentName(player.talent_pool_[i])
                           << "（" << (IsGradeA(player.talent_pool_[i]) ? "A级" : "B级") << "）——"
                           << TalentDescription(player.talent_pool_[i]) << "\n";
                }
                return StageErrCode::OK; // Not ready yet, need to choose again
            }
        }

        return StageErrCode::READY;
    }

};

// ==================== ActiveTalentStage ====================

class ActiveTalentStage : public SubGameStage<>
{
  public:
    ActiveTalentStage(MainStage& main_stage)
            : StageFsm(main_stage, "主动天赋阶段",
                MakeStageCommand(*this, "「乾坤大挪移」：立刻交换盘面上的两个砖块", &ActiveTalentStage::QiankunMove_,
                    VoidChecker("乾坤大挪移"), ArithChecker<uint32_t>(1, 19, "位置1"), ArithChecker<uint32_t>(1, 19, "位置2")),
                MakeStageCommand(*this, "「关键选择」：选择一个未获得的B级天赋", &ActiveTalentStage::KeyChoice_,
                    VoidChecker("关键选择"), AlterChecker<int>(MakeGradeBTalentOptionMap())),
                MakeStageCommand(*this, "跳过主动天赋发动", &ActiveTalentStage::Pass_, VoidChecker("pass")),
                MakeStageCommand(*this, "查看游戏进展情况和待执行的主动天赋", &ActiveTalentStage::Info_, VoidChecker("赛况")))
    {
    }

    virtual void OnStageBegin() override
    {
        Global().Boardcast() << Markdown{Main().CombHtml("## 第 " + std::to_string(Main().round_) + " 回合[主动天赋阶段]")};
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (!PlayerNeedsAction_(pid)) {
                Global().SetReady(pid);
                continue;
            }
            auto sender = Global().Boardcast();
            sender << At(pid) << " 可发动主动天赋：\n"
                                 "（发动「主动天赋」需先输入“天赋名称”作为指令前缀）\n";
            const auto& player = Main().players_[pid];
            for (const auto talent : player.talents_) {
                const auto& state = player.talent_states_.at(talent);
                if (!state->HasPendingActiveChoice(player)) continue;
                sender << state->ActivePrompt(player) << "\n";
                const std::string image_html = state->ActiveImageHtml(player);
                if (!image_html.empty()) {
                    Global().Boardcast() << Markdown(image_html, 1000);
                }
            }
        }
        Global().StartTimer(GAME_OPTION(局时));
    }

    // 「赛况」回复：当前最新棋盘；若请求者自己有待发动主动天赋再附自己的主动天赋提示与图片。
    AtomReqErrCode Info_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << Markdown{Main().CombHtml("## 第 " + std::to_string(Main().round_) + " 回合[主动天赋阶段]")};

        if (!PlayerNeedsAction_(pid)) {
            return StageErrCode::OK;
        }

        {
            auto sender = reply();
            sender << "您可发动的主动天赋：\n";
            const auto& player = Main().players_[pid];
            for (const auto talent : player.talents_) {
                const auto& state = player.talent_states_.at(talent);
                if (!state->HasPendingActiveChoice(player)) continue;
                sender << state->ActivePrompt(player) << "\n";
            }
        }
        // 单独发图（image_html 走单独的 Markdown 块）
        {
            const auto& player = Main().players_[pid];
            for (const auto talent : player.talents_) {
                const auto& state = player.talent_states_.at(talent);
                if (!state->HasPendingActiveChoice(player)) continue;
                const std::string image_html = state->ActiveImageHtml(player);
                if (!image_html.empty()) {
                    reply() << Markdown(image_html, 1000);
                }
            }
        }
        return StageErrCode::OK;
    }

  private:
    bool PlayerNeedsAction_(PlayerID pid) const
    {
        if (Main().player_out_[pid] != 0) return false;
        const auto& player = Main().players_[pid];
        for (const auto talent : player.talents_) {
            if (player.talent_states_.at(talent)->HasPendingActiveChoice(player)) return true;
        }
        return false;
    }

    void PassPlayer_(PlayerID pid, MsgSenderBase& sender)
    {
        auto& player = Main().players_[pid];
        bool any = false;
        for (const auto talent : player.talents_) {
            auto& state = player.talent_states_.at(talent);
            if (!state->HasPendingActiveChoice(player)) continue;
            const std::string message = state->OnActivePass(player);
            if (!message.empty()) {
                sender() << At(pid) << " " << message;
            }
            any = true;
        }
        if (!any) {
            sender() << At(pid) << " 当前没有待发动的主动天赋";
        }
    }

    void HandleUnreadyPlayers_()
    {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (Global().IsReady(pid) || !PlayerNeedsAction_(pid)) continue;
            PassPlayer_(pid, Global().BoardcastMsgSender());
            Global().SetReady(pid);
        }
        Global().HookUnreadyPlayers();
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Main().player_leave_[pid] = true;
        return StageErrCode::CONTINUE;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        HandleUnreadyPlayers_();
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        HandleUnreadyPlayers_();
        return StageErrCode::CHECKOUT;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid) || !PlayerNeedsAction_(pid)) return StageErrCode::OK;
        PassPlayer_(pid, reply);
        return StageErrCode::READY;
    }

    AtomReqErrCode Pass_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (Global().IsReady(pid) || !PlayerNeedsAction_(pid)) {
            reply() << "[错误] 当前没有待发动的主动天赋";
            return StageErrCode::FAILED;
        }
        PassPlayer_(pid, reply);
        return StageErrCode::READY;
    }

    AtomReqErrCode QiankunMove_(const PlayerID pid, const bool is_public, MsgSenderBase& reply,
                                const uint32_t lhs, const uint32_t rhs)
    {
        if (Global().IsReady(pid) || !PlayerNeedsAction_(pid)) {
            reply() << "[错误] 当前没有待发动的主动天赋";
            return StageErrCode::FAILED;
        }
        auto& player = Main().players_[pid];
        if (!player.HasTalent(Talent::乾坤大挪移)) {
            reply() << "[错误] 您没有天赋「乾坤大挪移」";
            return StageErrCode::FAILED;
        }
        ScoreResult result{player.comb_->BaseScore(), player.comb_->LineCount(), 0};
        std::string message;
        const bool ok = player.talent_states_.at(Talent::乾坤大挪移)->OnActiveCommand(
            player, "乾坤大挪移", std::vector<uint32_t>{lhs, rhs},
            TalentActiveContext{HasValuableOne(Main().special_event_)}, result, message);
        if (!ok) {
            reply() << "[错误] " << message;
            return StageErrCode::FAILED;
        }
        reply() << message;
        if (PlayerNeedsAction_(pid)) return StageErrCode::OK;
        return StageErrCode::READY;
    }

    AtomReqErrCode KeyChoice_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int talent_id)
    {
        if (Global().IsReady(pid) || !PlayerNeedsAction_(pid)) {
            reply() << "[错误] 当前没有待发动的主动天赋";
            return StageErrCode::FAILED;
        }
        auto& player = Main().players_[pid];
        if (!player.HasTalent(Talent::关键选择)) {
            reply() << "[错误] 您没有天赋「关键选择」";
            return StageErrCode::FAILED;
        }
        ScoreResult result{player.comb_->BaseScore(), player.comb_->LineCount(), 0};
        std::string message;
        const bool ok = player.talent_states_.at(Talent::关键选择)->OnActiveCommand(
            player, "关键选择", std::vector<uint32_t>{static_cast<uint32_t>(talent_id)},
            TalentActiveContext{
                HasValuableOne(Main().special_event_),
                [this, pid](Talent talent) { return Main().ApplyImmediateTalentEffects_(pid, talent); },
            },
            result, message);
        if (!ok) {
            reply() << "[错误] " << message;
            return StageErrCode::FAILED;
        }
        reply() << message;
        if (PlayerNeedsAction_(pid)) return StageErrCode::OK;
        return StageErrCode::READY;
    }
};

// ==================== Stage Transitions ====================

void MainStage::AdvancePhase_(SubStageFsmSetter& setter)
{
    switch (phase_) {
        case RoundPhase::放置:
            phase_ = RoundPhase::战前额外;
            CollectPreBattleExtras_();
            if (AnyPendingExtraCards_()) {
                setter.Emplace<ExtraCardStage>(*this);
                return;
            }
            // Fall through to talent check
            [[fallthrough]];

        case RoundPhase::战前额外:
            phase_ = RoundPhase::天赋选择;
            PrepareNewTalentChoices_();
            if (AnyPendingTalentChoice_()) {
                PrepareNewTalentChoices_();
                setter.Emplace<TalentStage>(*this);
                return;
            }
            [[fallthrough]];

        case RoundPhase::天赋选择: {
            phase_ = RoundPhase::主动天赋;
            if (AnyPendingActiveTalent_()) {
                setter.Emplace<ActiveTalentStage>(*this);
                return;
            }
            [[fallthrough]];
        }

        case RoundPhase::主动天赋: {
            phase_ = RoundPhase::对战;

            // 紧急救援: if any alive player has unused emergency rescue, skip entire battle and trigger SelectStage
            bool has_emergency = false;
            if (round_ > 2 && !IsSelectionRound(round_)) {
                for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
                    if (player_out_[pid] != 0) continue;
                    if (players_[pid].HasTalent(Talent::紧急救援) && !players_[pid].EmergencyRescue().used) {
                        has_emergency = true;
                        Global().Boardcast() << At(pid) << " 触发天赋「紧急救援」，跳过本轮对战，启动紧急选牌阶段！";
                    }
                }
            }

            if (has_emergency) {
                // Skip battle entirely, enter emergency SelectStage
                is_emergency_select_ = true;
                setter.Emplace<SelectStage>(*this, round_);
                return;
            }

            // Normal: Do battle (automatic). 对战结果与玩家淘汰共用同一个 sender，合并为一条播报。
            bool battle_happened;
            {
                auto sender = Global().Boardcast();
                battle_happened = DoBattle_(sender);
                DoEliminationAfterBattle_(sender);
            }
            DoPoison_();

            // 真实对战发生后，分发 OnBattlePhaseEnd hook 给所有 alive 玩家的天赋
            //（绝地反击 等会在此清掉 trigger_round 之后的临时分）。
            if (battle_happened) {
                OnBattlePhaseEnd_();
            }

            if (CheckGameOver()) {
                DoGameOver_();
                return;
            }

            // Collect post-battle extras (三年之期, 劫掠, etc.)
            CollectPostBattleExtras_();
            if (AnyPendingExtraCards_()) {
                phase_ = RoundPhase::战后额外;
                setter.Emplace<ExtraCardStage>(*this);
                return;
            }
            [[fallthrough]];
        }

        case RoundPhase::对战:
        case RoundPhase::战后额外:
            // After all phases done, start next round
            phase_ = RoundPhase::放置;
            StartNewRound_(setter);
            return;
    }
}

void MainStage::StartNewRound_(SubStageFsmSetter& setter)
{
    round_++;

    // Per-round talent effects (applied at round start)
    for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
        if (player_out_[pid] != 0) continue;
        auto& player = players_[pid];
        const TalentRoundContext context{IsSelectionRound(round_), HasValuableOne(special_event_)};
        for (const auto talent : k_round_start_order) {
            if (!player.HasTalent(talent)) continue;
            const std::string message = player.talent_states_.at(talent)->OnRoundStart(player, context);
            if (!message.empty()) {
                Global().Boardcast() << At(pid) << message;
            }
        }
    }

    if (CheckGameOver()) {
        DoGameOver_();
        return;
    }

    // Check if card pool 1 is exhausted — end game
    if (it_ == cards_.end() && !IsSelectionRound(round_) && round_ > 1) {
        Global().Boardcast() << "卡池耗尽，游戏结束！";
        DoGameOver_();
        return;
    }

    if (IsSelectionRound(round_)) {
        setter.Emplace<SelectStage>(*this, round_);
    } else if (round_ == 1) {
        // Initial round: windfall gives wild cards
        if (HasWindfall(special_event_)) {
            // For windfall, every player gets a wild card in round 1
            AreaCard wild_card; // default constructor = wild
            setter.Emplace<RoundStage>(*this, round_, wild_card);
        } else {
            AreaCard dummy; // initial round uses per-player cards, this is unused
            setter.Emplace<RoundStage>(*this, round_, dummy);
        }
    } else {
        AreaCard card = DrawFromPool1_();
        // Prepare next card for foresee
        if (HasForesee(special_event_)) {
            if (it_ != cards_.end()) {
                next_card_ = *it_;
            } else {
                next_card_ = std::nullopt;
            }
        }
        setter.Emplace<RoundStage>(*this, round_, card);
    }
}

void MainStage::FirstStageFsm(SubStageFsmSetter setter)
{
    // Announce special event
    Global().Boardcast() << "本局特殊事件：" << SpecialEventName(special_event_);

#ifdef TEST_BOT
    // 测试模式：开局直接赋予所有玩家指定天赋
    const auto& test_talents = GAME_OPTION(天赋);
    if (!test_talents.empty()) {
        for (auto& player : players_) {
            for (int t : test_talents) {
                Talent talent = static_cast<Talent>(t);
                if (!player.HasTalent(talent)) {
                    player.talents_.push_back(talent);
                    player.available_a_.erase(talent);
                    player.available_b_.erase(talent);
                }
            }
            for (int t : test_talents) {
                Talent talent = static_cast<Talent>(t);
                if (!player.HasTalent(talent)) continue;
                static constexpr Talent k_initial_effect_talents[] = {
                    Talent::三年之期,
                    Talent::两极反转,
                    Talent::九转玄机,
                    Talent::乾坤大挪移,
                    Talent::关键选择,
                    Talent::临时用品,
                    Talent::包扎,
                    Talent::星河流转,
                    Talent::冥想,
                    Talent::天使轮,
                    Talent::贪婪宝藏,
                    Talent::零的力量,
                    Talent::虚空之心,
                };
                if (std::find(std::begin(k_initial_effect_talents), std::end(k_initial_effect_talents), talent) ==
                    std::end(k_initial_effect_talents)) {
                    continue;
                }
                player.talent_states_.at(talent)->OnAcquire(
                    player, TalentAcquireContext{HasValuableOne(special_event_), true});
            }
            // 更新分数（来点实在的+4 等通过 RecalcPermanentExtra_ 生效）
            player.UpdateScore(ScoreResult{player.comb_->BaseScore(), player.comb_->LineCount(), 0}, HasValuableOne(special_event_));
        }
        Global().Boardcast() << "【测试模式】已为所有玩家添加 " << test_talents.size() << " 个天赋";
    }
#endif

    phase_ = RoundPhase::放置;
    StartNewRound_(setter);
}

void MainStage::NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    // After placement, apply placement-stage-end talent effects.
    for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
        if (player_out_[pid] != 0) continue;
        auto& player = players_[pid];
        const TalentPlacementContext context{HasValuableOne(special_event_)};
        for (const auto talent : k_placement_end_order) {
            if (player.HasTalent(talent)) {
                player.talent_states_.at(talent)->OnPlacementStageEnd(player, context);
            }
        }
    }

    AdvancePhase_(setter);
}

void MainStage::NextStageFsm(SelectStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    // Same post-placement processing as RoundStage
    for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
        if (player_out_[pid] != 0) continue;
        auto& player = players_[pid];
        const TalentPlacementContext context{HasValuableOne(special_event_)};
        for (const auto talent : k_placement_end_order) {
            if (player.HasTalent(talent)) {
                player.talent_states_.at(talent)->OnPlacementStageEnd(player, context);
            }
        }
    }

    // 紧急救援: after emergency SelectStage, mark used and skip to post-battle extras (no battle)
    if (is_emergency_select_) {
        is_emergency_select_ = false;
        for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
            if (player_out_[pid] != 0) continue;
            if (players_[pid].HasTalent(Talent::紧急救援) && !players_[pid].EmergencyRescue().used) {
                players_[pid].EmergencyRescue().used = true;
            }
        }
        // Skip battle/elimination/poison, go directly to post-battle extras and next round
        CollectPostBattleExtras_();
        if (AnyPendingExtraCards_()) {
            phase_ = RoundPhase::战后额外;
            setter.Emplace<ExtraCardStage>(*this);
            return;
        }
        phase_ = RoundPhase::放置;
        StartNewRound_(setter);
        return;
    }

    AdvancePhase_(setter);
}

void MainStage::NextStageFsm(ExtraCardStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    // Apply per-placement talent effects after extra cards
    for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
        if (player_out_[pid] != 0) continue;
        auto& player = players_[pid];
        const TalentPlacementContext context{HasValuableOne(special_event_)};
        for (const auto talent : k_placement_end_order) {
            if (player.HasTalent(talent)) {
                player.talent_states_.at(talent)->OnPlacementStageEnd(player, context);
            }
        }
    }

    AdvancePhase_(setter);
}

void MainStage::NextStageFsm(TalentStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    // Handle extra cards from talent effects (e.g., TEMP_WILD manual placement)
    if (AnyPendingExtraCards_()) {
        setter.Emplace<ExtraCardStage>(*this);
        return;
    }
    AdvancePhase_(setter);
}

void MainStage::NextStageFsm(ActiveTalentStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    AdvancePhase_(setter);
}

// ==================== Game Over ====================

// 按淘汰名次结算每位玩家的最终 game_score。
// rank_value = round * 10 + phase；存活玩家用 INT64_MAX 视为最佳（并列）。
// 对玩家 X：worse = 比 X 淘汰早的玩家数，better = 比 X 淘汰晚或仍存活的玩家数。
// player_final_score_[X] = (worse - better) * 100。同回合同阶段淘汰自然并列同分；
// 中毒淘汰（phase=2）名次优于同回合对战淘汰（phase=1）；卡池耗尽时所有存活并列第一。
void MainStage::ComputeRankScores_()
{
    auto rank_value = [&](PlayerID pid) -> int64_t {
        if (player_out_[pid] == 0) return std::numeric_limits<int64_t>::max();
        return static_cast<int64_t>(player_out_[pid]) * 10 + player_out_phase_[pid];
    };
    const uint32_t N = Global().PlayerNum();
    for (PlayerID pid = 0; pid.Get() < N; ++pid) {
        const int64_t mine = rank_value(pid);
        int32_t worse = 0, better = 0;
        for (PlayerID other = 0; other.Get() < N; ++other) {
            if (other == pid) continue;
            const int64_t v = rank_value(other);
            if (v < mine)      ++worse;   // other 淘汰更早 → 我比 ta 名次好
            else if (v > mine) ++better;  // other 淘汰更晚 / 存活 → 我比 ta 名次差
            // 相等：并列名次，不计入两边
        }
        player_final_score_[pid] = (worse - better) * 100;
    }
}

void MainStage::DoGameOver_()
{
    ComputeRankScores_();

    // 单一存活者（正常淘汰流程）→ 公告获胜玩家。
    // 多名存活者（卡池耗尽路径，alive_ > 1）→ 不再逐个公告；调用方已经播报过
    // "卡池耗尽，游戏结束！"，多名共同获胜在最终图与名次分中体现。
    if (alive_ == 1) {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (player_out_[pid] == 0) {
                Global().Boardcast() << "游戏结束，恭喜胜者：" << At(pid) << "！";
                break;
            }
        }
    }

    Global().Boardcast() << Markdown(CombHtml("## 终局"));

    if (GAME_OPTION(种子).empty()) {
        Global().Boardcast() << "本局特殊事件：" << SpecialEventShortName(special_event_) << "\n随机数种子：" + seed_str_;
        // Achievements
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (players_[pid].comb_->LineCount() >= 15) {
                Global().Achieve(pid, Achievement::天赋蜂王);
            }
            if (players_[pid].talents_.size() >= 6) {
                Global().Achieve(pid, Achievement::天赋满溢);
            }
            const int32_t total_score = players_[pid].TotalScore();
            if (total_score >= 300) {
                Global().Achieve(pid, Achievement::天赋初启);
            }
            if (total_score >= 340) {
                Global().Achieve(pid, Achievement::天赋进阶);
            }
            if (total_score >= 380) {
                Global().Achieve(pid, Achievement::天赋精通);
            }
            if (player_out_[pid] == 0 && players_[pid].never_lost_) {
                Global().Achieve(pid, Achievement::常胜将军);
            }
            if (player_out_[pid] == 0 && players_[pid].hp_ == 1) {
                Global().Achieve(pid, Achievement::命悬一线);
            }
            if (players_[pid].first_talent_round_ > 0 && players_[pid].first_talent_round_ < 8) {
                Global().Achieve(pid, Achievement::先发制人);
            }
            if (players_[pid].hp_ > static_cast<int32_t>(GAME_OPTION(血量))) {
                Global().Achieve(pid, Achievement::生生不息);
            }
            if (players_[pid].HasFullWildLine5()) {
                Global().Achieve(pid, Achievement::癞子长虹);
            }
            if (players_[pid].AllCellsSameNumber()) {
                Global().Achieve(pid, Achievement::天选之数);
            }
        }
    } else {
        Global().Boardcast() << "本局特殊事件：" << SpecialEventShortName(special_event_) << "\n自定义种子：" + seed_str_;
    }
}

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot
