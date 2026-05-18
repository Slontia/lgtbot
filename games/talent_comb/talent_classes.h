// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).
//
// Concrete talent classes. This file is included after Player is declared so a
// talent can keep its metadata, counters, display text, and hooks together.

#pragma once

inline std::string TalentBase::BoardDisplay(const Player& player) const { return Name(); }
inline std::string TalentBase::ScoreDetail(const Player& player) const { return ""; }
inline int32_t TalentBase::ScoreDetailDisplayedPermanentExtra(const Player& player) const { return 0; }
inline int32_t TalentBase::TempBattleScore(const Player& player) const { return 0; }
inline int32_t TalentBase::ModifyHealAmount(const Player& player, int32_t amount) const { return amount; }
inline int32_t TalentBase::PermanentExtraScorePass() const { return 0; }
inline int32_t TalentBase::PermanentExtraScore(Player& player, int32_t current_extra) { return 0; }
inline bool TalentBase::IsCompatibleWithSpecialEvent(SpecialEvent event) const { return true; }
inline std::string TalentBase::OnAcquire(Player& player, const TalentAcquireContext& context) { return ""; }
inline std::string TalentBase::OnBeforePlaceCard(Player& player, AreaCard& card, bool is_normal_round) { return ""; }
inline std::string TalentBase::OnRoundStart(Player& player, const TalentRoundContext& context) { return ""; }
inline void TalentBase::OnPlacementStageEnd(Player& player, const TalentPlacementContext& context) {}
inline std::string TalentBase::OnDiscard(Player& player, const AreaCard& card, const TalentDiscardContext& context) { return ""; }
inline std::string TalentBase::OnCardPlaced(Player& player, uint32_t idx, const ScoreResult& original_result, ScoreResult& effective_result,
                                            const TalentCardPlacedContext& context) { return ""; }
inline std::string TalentBase::AfterScoreUpdatedOnCardPlaced(Player& player, uint32_t idx, const ScoreResult& effective_result,
                                                             const TalentCardPlacedContext& context, int32_t old_permanent_extra) { return ""; }
inline std::string TalentBase::OnDefeat(Player& player, const TalentDefeatContext& context, int32_t& damage) { return ""; }
inline std::string TalentBase::OnVictory(Player& player, const TalentVictoryContext& context) { return ""; }
inline std::string TalentBase::OnPreBattleExtraCards(Player& player, const TalentPreBattleExtraContext& context) { return ""; }
inline std::string TalentBase::OnExtraCardActionEnd(Player& player) { return ""; }
inline bool TalentBase::HasPendingActiveChoice(const Player& player) const { return false; }
inline std::string TalentBase::ActivePrompt(const Player& player) const { return ""; }
inline std::string TalentBase::ActiveImageHtml(const Player& player) const { return ""; }
inline std::string TalentBase::OnActivePass(Player& player) { return ""; }
inline bool TalentBase::OnActiveCommand(Player& player, std::string_view command, const std::vector<uint32_t>& args,
                                        const TalentActiveContext& context, ScoreResult& result, std::string& message)
{
    message = "当前没有可发动的主动天赋指令";
    return false;
}
inline TalentDamageEffect TalentBase::AttackDamageDelta(Player& attacker, Player& defender, int32_t damage, std::mt19937& rng) { return {}; }
inline int32_t TalentBase::DefenseDamageDelta(Player& defender, int32_t damage) { return 0; }
inline void TalentBase::OnHealApplied(Player& player, int32_t actual_heal) {}
inline void TalentBase::OnDamageReceived(Player& player, int32_t damage) {}
inline std::string TalentBase::OnLethalDamage(Player& player, int32_t& damage, int32_t current_round) { return ""; }
inline void TalentBase::OnBattlePhaseEnd(Player& player, int32_t current_round) {}
inline std::string TalentBase::OnDefeatedOpponent(Player& killer, Player& victim, std::mt19937& rng) { return ""; }
inline int32_t TalentBase::SelectionPriority(const Player& player) const { return INT32_MAX; }
inline std::string TalentBase::SelectionBorderStyle(const Player& player, bool is_last_selector) const { return ""; }
inline void TalentBase::ModifyTalentPoolPicks(const Player& player, uint32_t& a_pick, uint32_t& b_pick) const {}

inline constexpr const char* kTalentColorInactive = "#999999";
inline constexpr const char* kTalentColorGlobal = "#D94A6A";
inline constexpr const char* kTalentColorProgress = "#DAA520";
inline constexpr const char* kTalentColorTempScore = "#FF6347";
inline constexpr const char* kTalentColorExtraScore = "#1E3A8A";
inline constexpr const char* kTalentColorBlueBuff = "#4169E1";
inline constexpr const char* kTalentColorPenalty = "#8B0000";
inline constexpr const char* kTalentColorPoison = "#6A0DAD";
inline constexpr const char* kTalentColorPermanentGreen = "green";

inline std::string TalentColoredText(const char* color, const std::string& text)
{
    return "<font color=" + std::string(color) + ">" + text + HTML_FONT_TAIL;
}

inline std::string TalentInactiveText(const std::string& text) { return TalentColoredText(kTalentColorInactive, text); }
inline std::string TalentGlobalText(const std::string& text) { return TalentColoredText(kTalentColorGlobal, text); }
inline std::string TalentProgressText(const std::string& text) { return TalentColoredText(kTalentColorProgress, text); }
inline std::string TalentTempScoreText(const std::string& text) { return TalentColoredText(kTalentColorTempScore, text); }
inline std::string TalentExtraScoreText(const std::string& text) { return TalentColoredText(kTalentColorExtraScore, text); }
inline std::string TalentBlueBuffText(const std::string& text) { return TalentColoredText(kTalentColorBlueBuff, text); }
inline std::string TalentPenaltyText(const std::string& text) { return TalentColoredText(kTalentColorPenalty, text); }
inline std::string TalentPoisonText(const std::string& text) { return TalentColoredText(kTalentColorPoison, text); }
inline std::string TalentPermanentGreenText(const std::string& text) { return TalentColoredText(kTalentColorPermanentGreen, text); }

class PerfectBlockTalent : public TalentBase
{
  public:
    PerfectBlockTalent() : TalentBase({"A", Talent::完美块, "完美块", "你的312视为万能牌"}) {}

    bool IsCompatibleWithSpecialEvent(SpecialEvent event) const override
    {
        return event != SpecialEvent::大的要来了;
    }

    std::string OnAcquire(Player& player, const TalentAcquireContext& context) override
    {
        if (player.HasTalent(Talent::以退为进)) {
            player.comb_->ApplyRetreatingAdvance();
        }
        player.comb_->ApplyPerfectBlock();
        player.UpdateScore(ScoreResult{player.comb_->BaseScore(), player.comb_->LineCount(), 0}, context.has_valuable_one);
        return "，盘面上所有312变为万能牌！";
    }

    std::string OnBeforePlaceCard(Player& player, AreaCard& card, bool is_normal_round) override
    {
        const bool was_wild = card.IsWild();
        card.ApplyPerfectBlock();
        if (!was_wild && card.IsWild()) {
            return "\n触发天赋「完美块」，该砖块视为万能牌！";
        }
        return "";
    }

    void OnPlacementStageEnd(Player& player, const TalentPlacementContext& context) override
    {
        player.comb_->ApplyPerfectBlock();
        player.UpdateScore(ScoreResult{player.comb_->BaseScore(), player.comb_->LineCount(), 0}, context.has_valuable_one);
    }
};

class CounterattackTalent : public TalentBase
{
  public:
    CounterattackTalent() : TalentBase({"A", Talent::绝地反击, "绝地反击", "受到致命伤害时，血量降为1，下回合作战时分数短暂提升15%"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        if (extra_score > 0) {
            return Name() + TalentTempScoreText("[+" + std::to_string(extra_score) + "]");
        }
        if (used) {
            return TalentInactiveText(Name());
        }
        return Name();
    }

    int32_t TempBattleScore(const Player& player) const override { return extra_score; }

    // 致死救援：把当前总分 15% 作为下次对战的临时分，把 hp 抬升至 1，吸收掉本次伤害。
    std::string OnLethalDamage(Player& player, int32_t& damage, int32_t current_round) override
    {
        if (used) return "";
        used = true;
        extra_score = static_cast<int32_t>(std::ceil(player.TotalScore() * 0.15));
        trigger_round = current_round;
        player.hp_ = 1;
        damage = 0;
        return "触发天赋「绝地反击」，血量降为1，获得 " + std::to_string(extra_score) + " 点反击加成！";
    }

    // 触发回合的下一个真实对战阶段结束时清掉 extra_score。
    void OnBattlePhaseEnd(Player& /*player*/, int32_t current_round) override
    {
        if (extra_score > 0 && trigger_round < current_round) {
            extra_score = 0;
            trigger_round = 0;
        }
    }

    bool used = false;
    int32_t extra_score = 0;
    int32_t trigger_round = 0;
};

class SeizeTalent : public TalentBase
{
  public:
    SeizeTalent() : TalentBase({"A", Talent::占得先机, "占得先机", "选牌阶段优先选牌"}) {}

    int32_t SelectionPriority(const Player& /*player*/) const override { return 1; }

    std::string SelectionBorderStyle(const Player& /*player*/, bool /*is_last*/) const override
    {
        return "border:2px solid red;";
    }
};

class IronBodyTalent : public TalentBase
{
  public:
    IronBodyTalent() : TalentBase({"A", Talent::钢铁之躯, "钢铁之躯", "你受到的伤害降低30%"}) {}

    int32_t DefenseDamageDelta(Player& defender, int32_t damage) override
    {
        return -static_cast<int32_t>(std::ceil(damage * 0.3));
    }
};

class RetreatAdvanceTalent : public TalentBase
{
  public:
    RetreatAdvanceTalent() : TalentBase({"A", Talent::以退为进, "以退为进", "你的7均视为6，4均视为3"}) {}

    std::string OnAcquire(Player& player, const TalentAcquireContext& context) override
    {
        player.comb_->ApplyRetreatingAdvance();
        if (player.HasTalent(Talent::完美块)) {
            player.comb_->ApplyPerfectBlock();
        }
        player.UpdateScore(ScoreResult{player.comb_->BaseScore(), player.comb_->LineCount(), 0}, context.has_valuable_one);
        return "，盘面上所有7变6、4变3！";
    }

    std::string OnBeforePlaceCard(Player& player, AreaCard& card, bool is_normal_round) override
    {
        card.ApplyRetreatingAdvance();
        return "";
    }

    void OnPlacementStageEnd(Player& player, const TalentPlacementContext& context) override
    {
        player.comb_->ApplyRetreatingAdvance();
        player.UpdateScore(ScoreResult{player.comb_->BaseScore(), player.comb_->LineCount(), 0}, context.has_valuable_one);
    }
};

class DeadlyMagicTalent : public TalentBase
{
  public:
    DeadlyMagicTalent() : TalentBase({"A", Talent::致命魔术, "致命魔术", "造成伤害时有15%概率造成额外100%伤害"}) {}

    TalentDamageEffect AttackDamageDelta(Player& attacker, Player& defender, int32_t damage, std::mt19937& rng) override
    {
        if (RandInt(rng, 1, 100) > 15) return {};
        int32_t magic_damage = static_cast<int32_t>(std::ceil(damage * 1.0));
        return {magic_damage, "触发天赋「致命魔术」，额外造成 " + std::to_string(magic_damage) + " 点伤害！"};
    }
};

class TriForceTalent : public TalentBase
{
  public:
    TriForceTalent() : TalentBase({"A", Talent::三相之力, "三相之力", "你的下三张非选牌阶段的牌依次获得左/中/右单线癞子"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        if (progress >= 3) {
            return TalentInactiveText(Name());
        }
        return Name() + TalentProgressText("(" + std::to_string(progress) + "/3)");
    }

    std::string OnBeforePlaceCard(Player& player, AreaCard& card, bool is_normal_round) override
    {
        if (!is_normal_round || progress >= 3) return "";
        const int32_t dir = progress;
        card.SetDirectionWild(dir);
        progress++;
        static const char* dir_names[3] = {"左上", "垂直", "右上"};
        return "\n触发天赋「三相之力」，" + std::string(dir_names[dir]) + "方向获得单线癞子（" + std::to_string(progress) + "/3）";
    }

    int32_t progress = 0;
};

class EmergencyRescueTalent : public TalentBase
{
  public:
    EmergencyRescueTalent() : TalentBase({"A", Talent::紧急救援, "紧急救援", "跳过下一轮对战，立刻启动选牌阶段，你最先选择"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        if (used) {
            return TalentInactiveText(Name());
        }
        return TalentBlueBuffText(Name());
    }

    // 未消耗时把选牌优先级压到最低（0），保证 紧急救援 持有者一定排在最前。
    int32_t SelectionPriority(const Player& /*player*/) const override { return used ? INT32_MAX : 0; }

    // 未消耗时显示金/红特效边框。
    std::string SelectionBorderStyle(const Player& /*player*/, bool /*is_last*/) const override
    {
        if (used) return "";
        return "border:2px solid #FFD700;box-shadow:0 0 4px #FF6347,0 0 8px #FFD700;";
    }

    bool used = false;
};

class WantAllTalent : public TalentBase
{
  public:
    WantAllTalent() : TalentBase({"A", Talent::我全都要, "我全都要", "下次选择天赋时获得全部"}) {}

    // 我全都要：玩家拥有该天赋后，"下次选择天赋"——也就是下一次出现非空候选池时——直接获得池中全部天赋，同时把"我全都要"从 talents_ 移除。
    // 需要注意"刚选到我全都要的那一次选择"本身不算"下次"，触发要等到再次选择天赋（同回合的级联池，或下一回合的天赋阶段）。
    // 该效果由 TalentStage::TriggerWantAllIfHeld_ 处理；当前是唯一的整池吞噬天赋，不引入基类通用 hook，避免为单例特例增加 API 噪音。
};

class PandoraBoxTalent : public TalentBase
{
  public:
    PandoraBoxTalent() : TalentBase({"A", Talent::潘多拉魔盒, "潘多拉魔盒", "随机获得两个B级天赋"}) {}
};

class CompoundInterestTalent : public TalentBase
{
  public:
    CompoundInterestTalent() : TalentBase({"A", Talent::利滚利, "利滚利", "你每有125分，每回合加1分"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        std::string s = Name();
        if (accumulated > 0) {
            s += TalentProgressText("(+" + std::to_string(accumulated) + ")");
        }
        return s;
    }

    std::string OnRoundStart(Player& player, const TalentRoundContext& context) override
    {
        const int32_t interest = player.TotalScore() / 125;
        if (interest <= 0) return "";
        accumulated += interest;
        player.UpdateScore(ScoreResult{player.comb_->BaseScore(), player.comb_->LineCount(), 0}, context.has_valuable_one);
        return "";
    }

    int32_t PermanentExtraScore(Player& player, int32_t current_extra) override { return accumulated; }

    int32_t accumulated = 0;
};

class DysonSphereTalent : public TalentBase
{
  public:
    DysonSphereTalent() : TalentBase({"A", Talent::戴森球, "戴森球", "若6条长度3的连线均完成，则分数+6%"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        std::string s = Name();
        const int32_t count = player.CountCompletedLength3Lines_();
        if (count >= 6) {
            s += TalentTempScoreText("(6/6✦)");
        } else {
            s += TalentProgressText("(" + std::to_string(count) + "/6)");
        }
        return s;
    }

    std::string ScoreDetail(const Player& player) const override
    {
        const int32_t bonus = player.DysonSphereBonus();
        if (bonus == 0) return "";
        return "+" + TalentExtraScoreText("[戴森球+" + std::to_string(bonus) + "]");
    }

    std::string OnAcquire(Player& player, const TalentAcquireContext& context) override
    {
        if (player.CountCompletedLength3Lines_() < 6) return "";
        activated = true;
        return "，已达成条件！立即获得 6% 额外分数";
    }

    std::string AfterScoreUpdatedOnCardPlaced(Player& player, uint32_t idx, const ScoreResult& effective_result,
                                              const TalentCardPlacedContext& context, int32_t old_permanent_extra) override
    {
        if (activated || player.CountCompletedLength3Lines_() < 6) return "";
        activated = true;
        std::string notify;
        const int32_t bonus = player.DysonSphereBonus();
        notify += "\n达成「戴森球」条件！获得 6% 分数加成（+" + std::to_string(bonus) + "）";
        return notify;
    }

    bool activated = false;
};

class DiscardScorerTalent : public TalentBase
{
  public:
    DiscardScorerTalent() : TalentBase({"A", Talent::零号位, "0号位", "弃牌时，获得弃牌三个方向数字之和的临时分（持续1回合，可叠加）"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        std::string s = Name();
        if (temp_score > 0) {
            s += TalentTempScoreText("[+" + std::to_string(temp_score) + "]");
        }
        return s;
    }

    int32_t TempBattleScore(const Player& player) const override { return temp_score; }

    std::string OnRoundStart(Player& player, const TalentRoundContext& context) override
    {
        temp_score = 0;
        return "";
    }

    std::string OnDiscard(Player& player, const AreaCard& card, const TalentDiscardContext& context) override
    {
        const int32_t bonus = card.PointSum();
        if (bonus <= 0) return "";
        temp_score += bonus;
        return "\n触发天赋「0号位」，获得 " + std::to_string(bonus) + " 点临时分";
    }

    int32_t temp_score = 0;
};

class SincereTalent : public TalentBase
{
  public:
    SincereTalent() : TalentBase({"A", Talent::坦诚相见, "坦诚相见", "对战时仅比较双方盘面的基础连线分，无视双方全部额外分数加成"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        std::string s = TalentGlobalText(Name());
        const int32_t ignored = player.TotalScore() + player.TempBattleScore() - player.base_score_;
        if (ignored > 0) {
            s += TalentPenaltyText("[-" + std::to_string(ignored) + "]");
        }
        return s;
    }
};

class GalaxyFlowTalent : public TalentBase
{
  public:
    GalaxyFlowTalent() : TalentBase({"A", Talent::星河流转, "星河流转", "位于2、4、7、13、16、18号位置的砖块获得长度3连线方向的单线癞子"}) {}

    std::string OnAcquire(Player& player, const TalentAcquireContext& context) override
    {
        player.comb_->ApplyGalaxyFlow();
        player.UpdateScore(ScoreResult{player.comb_->BaseScore(), player.comb_->LineCount(), 0}, context.has_valuable_one);
        return "，盘面上2、4、7、13、16、18号位的砖块已获得单线癞子！";
    }

    std::string OnCardPlaced(Player& player, uint32_t idx, const ScoreResult& original_result, ScoreResult& effective_result,
                             const TalentCardPlacedContext& context) override
    {
        if (idx == 0) return "";
        auto r = player.comb_->ApplyGalaxyFlowAt(idx);
        if (!r.has_value()) return "";
        const auto& [dir, new_result] = *r;
        effective_result.base_score = new_result.base_score;
        effective_result.line_count = new_result.line_count;
        effective_result.score_delta = original_result.score_delta + new_result.score_delta;
        static const char* k_dir_name[3] = {"左上", "垂直", "右上"};
        return std::string("\n触发天赋「星河流转」，") + k_dir_name[dir] + "方向获得单线癞子";
    }
};

class MeditationTalent : public TalentBase
{
  public:
    MeditationTalent() : TalentBase({"A", Talent::冥想, "冥想", "每回合获得5点生命，直到单次连线获得25分及以上的分数"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        if (active) return Name();
        return TalentInactiveText(Name());
    }

    std::string OnRoundStart(Player& player, const TalentRoundContext& context) override
    {
        if (active) player.Heal(5);
        return "";
    }

    std::string OnAcquire(Player& player, const TalentAcquireContext& context) override
    {
        active = true;
        return "，每回合获得5点生命，直到单次连线≥25分";
    }

    std::string AfterScoreUpdatedOnCardPlaced(Player& player, uint32_t idx, const ScoreResult& effective_result,
                                              const TalentCardPlacedContext& context, int32_t old_permanent_extra) override
    {
        if (!active || effective_result.score_delta < 25) return "";
        active = false;
        return "\n天赋「冥想」已停止！（本次连线获得 " + std::to_string(effective_result.score_delta) + " 分，达到25分条件）";
    }

    bool active = false;
};

class LightInterferenceTalent : public TalentBase
{
  public:
    LightInterferenceTalent() : TalentBase({"A", Talent::光波干涉, "光波干涉", "如果2条连线上的数字相同且连线长度也相同，这些连线获得20%额外分数"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        std::string s = Name();
        const int32_t bonus = Bonus(player);
        if (bonus > 0) {
            s += TalentProgressText("(+" + std::to_string(bonus) + ")");
        }
        return s;
    }

    int32_t PermanentExtraScore(Player& player, int32_t current_extra) override { return Bonus(player); }

    int32_t Bonus(const Player& player) const
    {
        std::map<std::pair<int32_t, size_t>, std::pair<int32_t, int32_t>> groups;
        for (const auto& line_def : k_all_lines) {
            if (!IsLineCompleted(player, line_def)) continue;
            int32_t matched = 0;
            bool all_wild = true;
            const uint32_t dir_idx = static_cast<uint32_t>(line_def.direction);
            for (uint32_t pos : line_def.positions) {
                const auto& card = player.comb_->GetCard(pos);
                if (card.has_value() && card->PointAt(dir_idx) != 10) {
                    all_wild = false;
                    matched = card->PointAt(dir_idx);
                    break;
                }
            }
            const size_t len = line_def.positions.size();
            const int32_t line_score = (all_wild ? 10 : matched) * static_cast<int32_t>(len);
            auto& entry = groups[{matched, len}];
            entry.first += 1;
            entry.second += line_score;
        }
        int32_t bonus = 0;
        for (const auto& [key, val] : groups) {
            if (val.first >= 2) {
                bonus += static_cast<int32_t>(std::ceil(val.second * 0.20));
            }
        }
        return bonus;
    }

    bool IsLineCompleted(const Player& player, const LineDefinition& line_def) const
    {
        const uint32_t dir_idx = static_cast<uint32_t>(line_def.direction);
        std::optional<int32_t> matched_value;
        for (uint32_t pos : line_def.positions) {
            const auto& card = player.comb_->GetCard(pos);
            if (!card.has_value()) return false;
            const int32_t val = card->PointAt(dir_idx);
            if (val != 10) {
                if (!matched_value.has_value()) {
                    matched_value = val;
                } else if (val != *matched_value) {
                    return false;
                }
            }
        }
        return true;
    }
};

class NineMysteryTalent : public TalentBase
{
  public:
    NineMysteryTalent() : TalentBase({"A", Talent::九转玄机, "九转玄机", "你的9视为癞子线"}) {}

    bool IsCompatibleWithSpecialEvent(SpecialEvent event) const override
    {
        return event != SpecialEvent::大的要来了 && event != SpecialEvent::两极分化 && event != SpecialEvent::大的没了;
    }

    std::string OnAcquire(Player& player, const TalentAcquireContext& context) override
    {
        player.comb_->ApplyNineAsWild();
        player.UpdateScore(ScoreResult{player.comb_->BaseScore(), player.comb_->LineCount(), 0}, context.has_valuable_one);
        return "，盘面上所有9已变为癞子线！";
    }

    std::string OnBeforePlaceCard(Player& player, AreaCard& card, bool is_normal_round) override
    {
        AreaCard before = card;
        card.ApplyNineAsWild();
        if (card == before) return "";
        return "\n触发天赋「九转玄机」，9变为癞子线！";
    }
};

class QiankunMoveTalent : public TalentBase
{
  public:
    QiankunMoveTalent() : TalentBase({"A", Talent::乾坤大挪移, "乾坤大挪移", "你可以立刻交换盘面上的两个砖块"}, false) {}

    std::string BoardDisplay(const Player& player) const override
    {
        if (!pending) return TalentInactiveText(Name());
        return Name() + TalentProgressText("[待发动]");
    }

    std::string OnAcquire(Player& player, const TalentAcquireContext& context) override
    {
        pending = true;
        return "，请在主动天赋阶段选择两个位置交换";
    }

    bool HasPendingActiveChoice(const Player& player) const override
    {
        return pending;
    }

    std::string ActivePrompt(const Player& player) const override
    {
        return "「乾坤大挪移」：输入“乾坤大挪移 位置1 位置2”交换两个位置，或输入“pass”放弃";
    }

    std::string OnActivePass(Player& player) override
    {
        if (!pending) return "";
        pending = false;
        return "放弃发动天赋「乾坤大挪移」，天赋失效";
    }

    bool OnActiveCommand(Player& player, std::string_view command, const std::vector<uint32_t>& args,
                         const TalentActiveContext& context, ScoreResult& result, std::string& message) override
    {
        if (!player.HasTalent(Talent::乾坤大挪移) || !pending) {
            message = "当前没有可发动的「乾坤大挪移」";
            return false;
        }
        if (args.size() != 2) {
            message = "「乾坤大挪移」需要输入两个位置";
            return false;
        }
        const uint32_t lhs = args[0];
        const uint32_t rhs = args[1];
        if (!player.comb_->IsFilled(lhs) && !player.comb_->IsFilled(rhs)) {
            message = "「乾坤大挪移」发动失败：位置 " + std::to_string(lhs) + " 与位置 " + std::to_string(rhs) + " 均为空";
            return false;
        }

        const int32_t old_score = player.TotalScore();
        result = player.comb_->SwapCards(lhs, rhs);
        player.UpdateScore(result, context.has_valuable_one);
        const int32_t delta = player.TotalScore() - old_score;
        pending = false;

        message = "发动天赋「乾坤大挪移」，交换位置 " + std::to_string(lhs) + " 与 " + std::to_string(rhs);
        if (delta > 0) {
            message += "，获得 " + std::to_string(delta) + " 点积分";
        } else if (delta < 0) {
            message += "，损失 " + std::to_string(-delta) + " 点积分";
        }
        return true;
    }

    bool pending = false;
};

class KeyChoiceTalent : public TalentBase
{
  public:
    KeyChoiceTalent() : TalentBase({"A", Talent::关键选择, "关键选择", "从未获得的B级天赋中任选一个获得"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        if (!pending) return TalentInactiveText(Name());
        return Name() + TalentProgressText("[待选择]");
    }

    std::string OnAcquire(Player& player, const TalentAcquireContext& context) override
    {
        if (player.available_b_.empty()) {
            pending = false;
            return "，但已没有可选择的B级天赋";
        }
        pending = true;
        return "，请在主动天赋阶段选择一个未获得的B级天赋";
    }

    bool HasPendingActiveChoice(const Player& player) const override
    {
        return pending;
    }

    std::string ActivePrompt(const Player& player) const override
    {
        return "「关键选择」：输入“关键选择 B级天赋完整名称”选择一个可选B级天赋，或输入“pass”放弃";
    }

    std::string ActiveImageHtml(const Player& player) const override
    {
        if (!pending) return "";
        std::string html = "<div style=\"padding: 20px;\">";
        html += "<h2 style=\"text-align:center; margin: 0 0 16px 0;\">「关键选择」可选B级天赋</h2>";
        html += "<table style=\"border-collapse: collapse; width: 100%; font-size: 18px;\">";
        html += "<tr><th style=\"border:1px solid #999; padding:8px; width:20%;\">天赋</th>"
                "<th style=\"border:1px solid #999; padding:8px;\">效果</th></tr>";
        for (const auto talent : player.available_b_) {
            html += "<tr><td style=\"border:1px solid #bbb; padding:8px; font-weight:bold;\">"
                    + std::string(TalentName(talent)) + "</td><td style=\"border:1px solid #bbb; padding:8px;\">"
                    + std::string(TalentDescription(talent)) + "</td></tr>";
        }
        html += "</table></div>";
        return html;
    }

    std::string OnActivePass(Player& player) override
    {
        if (!pending) return "";
        pending = false;
        return "放弃发动天赋「关键选择」，天赋失效";
    }

    bool OnActiveCommand(Player& player, std::string_view command, const std::vector<uint32_t>& args,
                         const TalentActiveContext& context, ScoreResult& result, std::string& message) override
    {
        if (!player.HasTalent(Talent::关键选择) || !pending) {
            message = "当前没有可发动的「关键选择」";
            return false;
        }
        if (args.size() != 1) {
            message = "「关键选择」需要输入一个B级天赋名称";
            return false;
        }
        const Talent selected = static_cast<Talent>(args[0]);
        if (!IsGradeB(selected)) {
            message = "「" + std::string(TalentName(selected)) + "」不是B级天赋";
            return false;
        }
        if (player.available_b_.find(selected) == player.available_b_.end()) {
            message = "「" + std::string(TalentName(selected)) + "」不是当前可选择的B级天赋";
            return false;
        }

        auto it = std::find(player.talents_.begin(), player.talents_.end(), Talent::关键选择);
        if (it != player.talents_.end()) {
            *it = selected;
        } else {
            player.talents_.push_back(selected);
        }
        player.available_a_.erase(selected);
        player.available_b_.erase(selected);
        pending = false;

        std::string extra;
        if (context.apply_immediate_talent_effects) {
            extra = context.apply_immediate_talent_effects(selected);
        } else {
            extra = player.talent_states_.at(selected)->OnAcquire(player, TalentAcquireContext{context.has_valuable_one});
        }
        message = "发动天赋「关键选择」，获得B级天赋「" + std::string(TalentName(selected)) + "」" + extra;
        return true;
    }

    bool pending = false;
};

class LifeGameTalent : public TalentBase
{
  public:
    LifeGameTalent() : TalentBase({"A", Talent::生命游戏, "生命游戏", "在本局游戏内，每累计被扣20生命，额外+2分；每累计回复5生命，额外+1分"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        const int32_t dmg_bonus = DamageBonus();
        const int32_t heal_bonus = HealBonus();
        if (dmg_bonus == 0 && heal_bonus == 0) return Name();
        return Name() + TalentProgressText("(" + std::to_string(dmg_bonus) + "+" + std::to_string(heal_bonus) + ")");
    }

    void OnDamageReceived(Player& player, int32_t damage) override
    {
        if (damage <= 0) return;
        const int32_t before = DamageBonus();
        damage_taken += damage;
        // 只在玩家已持有「生命游戏」时刷新
        if (DamageBonus() != before && player.HasTalent(Talent::生命游戏)) {
            player.RefreshPermanentExtra();
        }
    }

    void OnHealApplied(Player& player, int32_t actual_heal) override
    {
        if (actual_heal <= 0) return;
        const int32_t before = HealBonus();
        heal_done += actual_heal;
        // 同上：回血累计跨过门槛时立即刷新分数，避免滞后。
        if (HealBonus() != before && player.HasTalent(Talent::生命游戏)) {
            player.RefreshPermanentExtra();
        }
    }

    int32_t PermanentExtraScore(Player& player, int32_t current_extra) override
    {
        return DamageBonus() + HealBonus();
    }

    int32_t DamageBonus() const { return (damage_taken / 20) * 2; }
    int32_t HealBonus() const { return heal_done / 5; }

    int32_t damage_taken = 0;
    int32_t heal_done = 0;
};

class YZoneTalent : public TalentBase
{
  public:
    YZoneTalent() : TalentBase({"A", Talent::Y区域, "Y区域", "1-5-10(左上)、10-14-17(右上)、10-11-12(垂直)每有一条线连接成功，你的总分增加2%"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        const int32_t count = SegmentCount(player);
        if (count == 0) return Name();
        return Name() + TalentProgressText("(+" + std::to_string(count * 2) + "%)");
    }

    std::string ScoreDetail(const Player& player) const override
    {
        if (current_bonus == 0) return "";
        return "+" + TalentExtraScoreText("[Y区域+" + std::to_string(current_bonus) + "]");
    }

    int32_t ScoreDetailDisplayedPermanentExtra(const Player& player) const override
    {
        return current_bonus;
    }

    int32_t PermanentExtraScorePass() const override { return 1; }

    int32_t PermanentExtraScore(Player& player, int32_t current_extra) override
    {
        const int32_t count = SegmentCount(player);
        if (count == 0) {
            current_bonus = 0;
            return 0;
        }
        const int32_t basis = player.base_score_ + player.valuable_one_bonus_ + current_extra;
        current_bonus = static_cast<int32_t>(std::ceil(std::abs(basis) * count * 2.0 / 100.0));
        if (basis < 0) current_bonus = -current_bonus;
        return current_bonus;
    }

    int32_t SegmentCount(const Player& player) const
    {
        int32_t count = 0;
        if (IsSegmentConnected(player, {1, 5, 10}, 0)) ++count;
        if (IsSegmentConnected(player, {10, 14, 17}, 2)) ++count;
        if (IsSegmentConnected(player, {10, 11, 12}, 1)) ++count;
        return count;
    }

    bool IsSegmentConnected(const Player& player, std::initializer_list<uint32_t> positions, uint32_t dir) const
    {
        std::optional<int32_t> matched;
        for (uint32_t pos : positions) {
            const auto& card = player.comb_->GetCard(pos);
            if (!card.has_value()) return false;
            const int32_t val = card->PointAt(dir);
            if (val != 10) {
                if (!matched.has_value()) matched = val;
                else if (val != *matched) return false;
            }
        }
        return true;
    }

    int32_t current_bonus = 0;
};

class BloodlustTalent : public TalentBase
{
  public:
    BloodlustTalent() : TalentBase({"B", Talent::嗜血, "嗜血", "战斗获胜时，生命值+4"}) {}

    std::string OnVictory(Player& player, const TalentVictoryContext& context) override
    {
        player.Heal(4);
        return "";
    }
};

class StillUsefulTalent : public TalentBase
{
  public:
    StillUsefulTalent() : TalentBase({"B", Talent::还是有用的, "还是有用的", "你的每一行1分数额外加3，每一行2分数额外加6"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        std::string s = Name();
        const int32_t bonus = Bonus(player);
        if (bonus > 0) {
            s += TalentProgressText("(+" + std::to_string(bonus) + ")");
        }
        return s;
    }

    int32_t PermanentExtraScore(Player& player, int32_t current_extra) override { return Bonus(player); }

    int32_t Bonus(const Player& player) const
    {
        int32_t bonus = 0;
        for (const auto& line_def : k_all_lines) {
            if (!IsLineCompleted(player, line_def)) continue;
            const int32_t matched = GetLineMatchedValue(player, line_def);
            if (matched == 1) bonus += 3;
            if (matched == 2) bonus += 6;
        }
        return bonus;
    }

    bool IsLineCompleted(const Player& player, const LineDefinition& line_def) const
    {
        const uint32_t dir_idx = static_cast<uint32_t>(line_def.direction);
        std::optional<int32_t> matched_value;
        for (uint32_t pos : line_def.positions) {
            const auto& card = player.comb_->GetCard(pos);
            if (!card.has_value()) return false;
            const int32_t val = card->PointAt(dir_idx);
            if (val != 10) {
                if (!matched_value.has_value()) {
                    matched_value = val;
                } else if (val != *matched_value) {
                    return false;
                }
            }
        }
        return true;
    }

    int32_t GetLineMatchedValue(const Player& player, const LineDefinition& line_def) const
    {
        const uint32_t dir_idx = static_cast<uint32_t>(line_def.direction);
        for (uint32_t pos : line_def.positions) {
            const auto& card = player.comb_->GetCard(pos);
            if (card.has_value() && card->PointAt(dir_idx) != 10) {
                return card->PointAt(dir_idx);
            }
        }
        return 0;
    }
};

class SwiftAttackTalent : public TalentBase
{
  public:
    SwiftAttackTalent() : TalentBase({"B", Talent::快攻, "快攻", "战斗获胜时，对对手造成伤害+6"}) {}

    TalentDamageEffect AttackDamageDelta(Player& attacker, Player& defender, int32_t damage, std::mt19937& rng) override
    {
        return {6, ""};
    }
};

class IndependentTalent : public TalentBase
{
  public:
    IndependentTalent() : TalentBase({"B", Talent::特立独行, "特立独行", "分数为奇数时，战斗时分数短暂提升6"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        std::string s = Name();
        if (TempBattleScore(player) > 0) {
            s += TalentTempScoreText("[+6]");
        }
        return s;
    }

    int32_t TempBattleScore(const Player& player) const override
    {
        return player.TotalScore() % 2 == 1 ? 6 : 0;
    }
};

class InPairsTalent : public TalentBase
{
  public:
    InPairsTalent() : TalentBase({"B", Talent::成双成对, "成双成对", "分数为偶数时，战斗时分数短暂提升6"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        std::string s = Name();
        if (TempBattleScore(player) > 0) {
            s += TalentTempScoreText("[+6]");
        }
        return s;
    }

    int32_t TempBattleScore(const Player& player) const override
    {
        return player.TotalScore() % 2 == 0 ? 6 : 0;
    }
};

class TrashRecycleTalent : public TalentBase
{
  public:
    TrashRecycleTalent() : TalentBase({"B", Talent::垃圾回收, "垃圾回收", "每次弃牌时获得2分"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        std::string s = Name();
        const int32_t bonus = discard_count * 2;
        if (bonus > 0) {
            s += TalentProgressText("(+" + std::to_string(bonus) + ")");
        }
        return s;
    }

    std::string OnDiscard(Player& player, const AreaCard& card, const TalentDiscardContext& context) override
    {
        discard_count++;
        player.UpdateScore(ScoreResult{player.comb_->BaseScore(), player.comb_->LineCount(), 0}, context.has_valuable_one);
        return "\n触发天赋「垃圾回收」，获得 2 点积分";
    }

    int32_t PermanentExtraScore(Player& player, int32_t current_extra) override { return discard_count * 2; }

    int32_t discard_count = 0;
};

class SomethingRealTalent : public TalentBase
{
  public:
    SomethingRealTalent() : TalentBase({"B", Talent::来点实在的, "来点实在的", "立刻获得4分"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        return Name() + TalentPermanentGreenText("(+4)");
    }

    std::string OnAcquire(Player& player, const TalentAcquireContext& context) override
    {
        return "，立即获得 4 分！";
    }

    int32_t PermanentExtraScore(Player& player, int32_t current_extra) override { return 4; }
};

class OffensiveFormTalent : public TalentBase
{
  public:
    OffensiveFormTalent() : TalentBase({"B", Talent::攻击形态, "攻击形态", "造成的伤害增加15%，受到的伤害增加5%"}, false) {}

    TalentDamageEffect AttackDamageDelta(Player& attacker, Player& defender, int32_t damage, std::mt19937& rng) override
    {
        return {static_cast<int32_t>(std::ceil(damage * 0.15)), ""};
    }

    int32_t DefenseDamageDelta(Player& defender, int32_t damage) override
    {
        return static_cast<int32_t>(std::ceil(damage * 0.05));
    }
};

class DefensiveFormTalent : public TalentBase
{
  public:
    DefensiveFormTalent() : TalentBase({"B", Talent::防御形态, "防御形态", "受到的伤害减少15%，造成的伤害减少5%"}, false) {}

    TalentDamageEffect AttackDamageDelta(Player& attacker, Player& defender, int32_t damage, std::mt19937& rng) override
    {
        return {-static_cast<int32_t>(std::ceil(damage * 0.05)), ""};
    }

    int32_t DefenseDamageDelta(Player& defender, int32_t damage) override
    {
        return -static_cast<int32_t>(std::ceil(damage * 0.15));
    }
};

class LocalEnhanceTalent : public TalentBase
{
  public:
    LocalEnhanceTalent() : TalentBase({"B", Talent::局部强化, "局部强化", "10号位每个方向的数字完成一条匹配连线+3分（每方向仅一次，癞子可匹配任意数字，上限9分）"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        std::string s = Name();
        int32_t bonus = 0;
        for (const bool is_triggered : triggered) {
            if (is_triggered) bonus += 3;
        }
        if (bonus > 0) {
            s += TalentProgressText("(+" + std::to_string(bonus) + ")");
        }
        return s;
    }

    int32_t PermanentExtraScore(Player& player, int32_t current_extra) override
    {
        const auto& center_card = player.comb_->GetCard(10);
        if (!center_card.has_value()) {
            triggered = {false, false, false};
            return 0;
        }

        const std::array<int32_t, 3> enhance_nums = {
            center_card->PointAt(0),
            center_card->PointAt(1),
            center_card->PointAt(2)
        };

        std::array<bool, 3> new_triggered = {false, false, false};
        for (const auto& line_def : k_all_lines) {
            if (!IsLineCompleted(player, line_def)) continue;
            const int32_t matched = GetLineMatchedValue(player, line_def);
            for (int i = 0; i < 3; ++i) {
                if (new_triggered[i]) continue;
                if (enhance_nums[i] == 10 || matched == enhance_nums[i]) {
                    new_triggered[i] = true;
                }
            }
        }
        triggered = new_triggered;

        int32_t bonus = 0;
        for (const bool is_triggered : triggered) {
            if (is_triggered) bonus += 3;
        }
        return bonus;
    }

    bool IsLineCompleted(const Player& player, const LineDefinition& line_def) const
    {
        const uint32_t dir_idx = static_cast<uint32_t>(line_def.direction);
        std::optional<int32_t> matched_value;
        for (uint32_t pos : line_def.positions) {
            const auto& card = player.comb_->GetCard(pos);
            if (!card.has_value()) return false;
            const int32_t val = card->PointAt(dir_idx);
            if (val != 10) {
                if (!matched_value.has_value()) {
                    matched_value = val;
                } else if (val != *matched_value) {
                    return false;
                }
            }
        }
        return true;
    }

    int32_t GetLineMatchedValue(const Player& player, const LineDefinition& line_def) const
    {
        const uint32_t dir_idx = static_cast<uint32_t>(line_def.direction);
        for (uint32_t pos : line_def.positions) {
            const auto& card = player.comb_->GetCard(pos);
            if (card.has_value() && card->PointAt(dir_idx) != 10) {
                return card->PointAt(dir_idx);
            }
        }
        return 0;
    }

    std::array<bool, 3> triggered = {false, false, false};
};

class GainAfterLossTalent : public TalentBase
{
  public:
    GainAfterLossTalent() : TalentBase({"B", Talent::有舍有得, "有舍有得", "每战败4次，随机获得3枚砖块，从中选择1枚放置"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        return Name() + TalentProgressText("(" + std::to_string(loss_count) + "/4)");
    }

    std::string OnDefeat(Player& player, const TalentDefeatContext& context, int32_t& damage) override
    {
        loss_count++;
        return "";
    }

    std::string OnPreBattleExtraCards(Player& player, const TalentPreBattleExtraContext& context) override
    {
        if (loss_count < 4 || context.offered_cards == nullptr || context.offered_cards->empty()) return "";
        // place_all=false → 玩家在 ExtraCardStage 中从 3 张候选里挑 1 张放置，剩余丢弃。
        player.extra_card_queue_.push_back({*context.offered_cards, Name(), false, Talent::有舍有得});
        loss_count -= 4;
        return "触发天赋「有舍有得」，从 3 枚砖块中选 1 枚放置";
    }

    int32_t loss_count = 0;
};

class ThreeYearTalent : public TalentBase
{
  public:
    ThreeYearTalent() : TalentBase({"B", Talent::三年之期, "三年之期", "把接下来三个回合获得的砖块存起来，第四个回合时一次性摆放，并恢复存储期间受到的所有伤害"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        if (!active) {
            return TalentInactiveText(Name());
        }
        return Name() + TalentProgressText("(" + std::to_string(cards.size()) + "/3)");
    }

    std::string OnAcquire(Player& player, const TalentAcquireContext& context) override
    {
        active = true;
        rounds_left = 3;
        return "";
    }

    // 存储期间每次受到伤害都累计；存储结束后由 CollectPostBattleExtras_ 在放置时回血。
    void OnDamageReceived(Player& player, int32_t damage) override
    {
        if (active && damage > 0) damage_stored += damage;
    }

    bool active = false;
    int32_t rounds_left = 0;
    std::vector<AreaCard> cards;
    int32_t damage_stored = 0;
};

class ForgeTalent : public TalentBase
{
  public:
    ForgeTalent() : TalentBase({"B", Talent::锻造, "锻造", "弃牌时，按顺序获得砖块三个方向数字的碎片，集齐三个方向的碎片合成一枚砖块放置"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        return Name() + TalentProgressText("(" + FragmentDisplay() + ")");
    }

    std::string FragmentDisplay() const
    {
        std::string fragments_str;
        for (size_t i = 0; i < 3; ++i) {
            if (i < fragments.size()) {
                const int32_t val = fragments[i];
                fragments_str += (val == 10) ? "X" : std::to_string(val);
            } else {
                fragments_str += "-";
            }
        }
        return fragments_str;
    }

    std::string OnDiscard(Player& player, const AreaCard& card, const TalentDiscardContext& context) override
    {
        const size_t frag_count = fragments.size();
        if (frag_count >= 3) return "";

        const int32_t frag_val = card.PointAt(static_cast<uint32_t>(frag_count));
        fragments.push_back(frag_val);

        const std::string frag_display = (frag_val == 10) ? "X" : std::to_string(frag_val);
        std::string notify = "\n触发天赋「锻造」，获得碎片 " + frag_display + "，当前碎片：" + FragmentDisplay();
        return notify;
    }

    std::string TryForge(Player& player)
    {
        if (fragments.size() < 3) return "";
        AreaCard forged(fragments[0], fragments[1], fragments[2]);
        player.extra_card_queue_.push_back({{forged}, Name(), false, Talent::锻造});
        fragments.clear();
        return "通过「锻造」合成了一枚新砖块！";
    }

    std::string OnPreBattleExtraCards(Player& player, const TalentPreBattleExtraContext& context) override
    {
        return TryForge(player);
    }

    std::string OnExtraCardActionEnd(Player& player) override
    {
        return TryForge(player);
    }

    std::vector<int32_t> fragments;
};

class TailGoodsTalent : public TalentBase
{
  public:
    TailGoodsTalent() : TalentBase({"B", Talent::尾货处理, "尾货处理", "选牌时，如果你最后一个选，则可以获得剩下两个砖块"}) {}

    // 仅当玩家排在最后一位选牌时显示青色边框；触发"末位补牌"的逻辑保留在 SelectStage::Select_ 中特判。
    std::string SelectionBorderStyle(const Player& /*player*/, bool is_last_selector) const override
    {
        return is_last_selector ? "border:2px solid #00E5FF;" : "";
    }
};

class TuringTestTalent : public TalentBase
{
  public:
    TuringTestTalent() : TalentBase({"B", Talent::图灵测试, "图灵测试", "对战镜像战败也不会受到伤害"}) {}

    std::string OnDefeat(Player& player, const TalentDefeatContext& context, int32_t& damage) override
    {
        if (!context.is_mirror) return "";
        damage = 0;
        return "";
    }
};

class NoMoreThanThreeTalent : public TalentBase
{
  public:
    NoMoreThanThreeTalent() : TalentBase({"B", Talent::事不过三, "事不过三", "你第4次战败时，免疫此次伤害"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        std::string s = Name();
        if (defeat_count >= 3) {
            s += TalentTempScoreText("(3/3✦)");
        } else {
            s += TalentProgressText("(" + std::to_string(defeat_count) + "/3)");
        }
        return s;
    }

    std::string OnDefeat(Player& player, const TalentDefeatContext& context, int32_t& damage) override
    {
        if (defeat_count >= 3) {
            defeat_count = 0;
            damage = 0;
            return "触发天赋「事不过三」，免疫此次伤害！";
        }
        defeat_count++;
        return "";
    }

    int32_t defeat_count = 0;
};

class SlotMachineTalent : public TalentBase
{
  public:
    SlotMachineTalent() : TalentBase({"B", Talent::摇奖机, "摇奖机", "随机获得一个A级天赋"}) {}
};

class DigitReverseTalent : public TalentBase
{
  public:
    DigitReverseTalent() : TalentBase({"B", Talent::两极反转, "两极反转", "你的1视为9，你的9视为1"}) {}

    bool IsCompatibleWithSpecialEvent(SpecialEvent event) const override
    {
        return event != SpecialEvent::大的要来了 && event != SpecialEvent::大的没了;
    }

    std::string OnAcquire(Player& player, const TalentAcquireContext& context) override
    {
        player.comb_->ApplyDigitReverse();
        if (player.HasTalent(Talent::九转玄机)) {
            player.comb_->ApplyNineAsWild();
        }
        if (player.HasTalent(Talent::以退为进)) {
            player.comb_->ApplyRetreatingAdvance();
        }
        if (player.HasTalent(Talent::完美块)) {
            player.comb_->ApplyPerfectBlock();
        }
        player.UpdateScore(ScoreResult{player.comb_->BaseScore(), player.comb_->LineCount(), 0}, context.has_valuable_one);
        return "，盘面上所有1变9、9变1！";
    }

    std::string OnBeforePlaceCard(Player& player, AreaCard& card, bool is_normal_round) override
    {
        card.ApplyDigitReverse();
        return "";
    }
};

class LoserBladeTalent : public TalentBase
{
  public:
    LoserBladeTalent() : TalentBase({"B", Talent::败者之刃, "败者之刃", "你战败后获得4分临时分（可累积），在战胜一次后清除"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        std::string s = Name();
        if (temp_score > 0) {
            s += TalentTempScoreText("[+" + std::to_string(temp_score) + "]");
        }
        return s;
    }

    int32_t TempBattleScore(const Player& player) const override { return temp_score; }

    std::string OnDefeat(Player& player, const TalentDefeatContext& context, int32_t& damage) override
    {
        temp_score += 4;
        return "";
    }

    std::string OnVictory(Player& player, const TalentVictoryContext& context) override
    {
        temp_score = 0;
        return "";
    }

    int32_t temp_score = 0;
};

class TempWildTalent : public TalentBase
{
  public:
    TempWildTalent() : TalentBase({"B", Talent::临时用品, "临时用品", "获得一个仅三回合可用的癞子"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        if (position <= 0) {
            return TalentInactiveText(Name());
        }
        return Name() + TalentTempScoreText("[" + std::to_string(rounds_left) + "回合]");
    }

    std::string OnAcquire(Player& player, const TalentAcquireContext& context) override
    {
        AreaCard wild_card;
        if (context.initial_test_mode && player.comb_->HasEmptyPosition()) {
            const auto [idx, result] = player.comb_->SeqFill(wild_card);
            position = idx;
            rounds_left = 3;
            return "，获得一张临时癞子砖块";
        }
        player.extra_card_queue_.push_back({{wild_card}, Name(), false, Talent::临时用品});
        return "，请在额外放置阶段选择位置放置临时癞子（3回合后到期）";
    }

    std::string OnCardPlaced(Player& player, uint32_t idx, const ScoreResult& original_result, ScoreResult& effective_result,
                             const TalentCardPlacedContext& context) override
    {
        if (position <= 0 || idx != position || context.previous_card == nullptr || !context.previous_card->IsWild()) return "";
        if (context.source_talent == Talent::临时用品) return "";
        position = 0;
        rounds_left = 0;
        return "\n「临时用品」已被覆盖，天赋不再生效";
    }

    std::string OnRoundStart(Player& player, const TalentRoundContext& context) override
    {
        if (position <= 0 || context.is_selection_round) return "";
        rounds_left--;
        if (rounds_left > 0) return "";
        const uint32_t expired_position = position;
        const auto& card = player.comb_->GetCard(expired_position);
        if (!card.has_value() || !card->IsWild()) return "";
        auto result = player.comb_->RemoveCard(expired_position);
        player.UpdateScore(result, context.has_valuable_one);
        position = 0;
        return " 的「临时用品」癞子到期，已从位置 " + std::to_string(expired_position) + " 移除";
    }

    uint32_t position = 0;
    int32_t rounds_left = 0;
};

class BandageTalent : public TalentBase
{
  public:
    BandageTalent() : TalentBase({"B", Talent::包扎, "包扎", "立即获得20点生命"}) {}

    std::string OnAcquire(Player& player, const TalentAcquireContext& context) override
    {
        player.Heal(20);
        return "，立即获得 20 点生命！";
    }
};

class HerbalGrowthTalent : public TalentBase
{
  public:
    HerbalGrowthTalent() : TalentBase({"B", Talent::百味草, "百味草", "获得1层中毒，每因中毒损失1点生命，获得1分"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        std::string s = Name() + TalentPoisonText("(" + std::to_string(player.poison_layers_) + "毒+" + std::to_string(poison_score) + "分)");
        return s;
    }

    std::string OnAcquire(Player& player, const TalentAcquireContext& context) override
    {
        player.poison_layers_++;
        return "，获得1层中毒（当前中毒：" + std::to_string(player.poison_layers_) + " 层）";
    }

    int32_t PermanentExtraScore(Player& player, int32_t current_extra) override { return poison_score; }

    int32_t poison_score = 0;
};

class AngelRoundTalent : public TalentBase
{
  public:
    AngelRoundTalent() : TalentBase({"B", Talent::天使轮, "天使轮", "获得15点临时分，直到下次完成一次连线为止"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        if (!active) {
            return TalentInactiveText(Name());
        }
        return Name() + TalentTempScoreText("[+15]");
    }

    int32_t TempBattleScore(const Player& player) const override { return active ? 15 : 0; }

    std::string OnAcquire(Player& player, const TalentAcquireContext& context) override
    {
        active = true;
        return "，获得15点临时分，直到下次完成连线为止";
    }

    std::string AfterScoreUpdatedOnCardPlaced(Player& player, uint32_t idx, const ScoreResult& effective_result,
                                              const TalentCardPlacedContext& context, int32_t old_permanent_extra) override
    {
        if (!active || effective_result.score_delta <= 0) return "";
        active = false;
        return "\n天赋「天使轮」临时分已消失！（完成连线）";
    }

    bool active = false;
};

class PlunderTalent : public TalentBase
{
  public:
    PlunderTalent() : TalentBase({"B", Talent::劫掠, "劫掠", "击败玩家后，从他的盘面上随机挑选5个砖块，你选择其中一枚放置"}) {}

    std::string OnDefeatedOpponent(Player& killer, Player& victim, std::mt19937& rng) override
    {
        auto filled = victim.comb_->GetFilledPositions();
        SeededShuffle(filled.begin(), filled.end(), rng);
        const size_t pick = std::min<size_t>(5, filled.size());
        std::vector<AreaCard> loot;
        loot.reserve(pick);
        for (size_t i = 0; i < pick; ++i) {
            const auto& card = victim.comb_->GetCard(filled[i]);
            if (card.has_value()) loot.push_back(*card);
        }
        if (loot.empty()) return "";
        const size_t n = loot.size();
        killer.extra_card_queue_.push_back({std::move(loot), "劫掠"});
        return "触发天赋「劫掠」，从被淘汰玩家处获得 " + std::to_string(n) + " 枚砖块供选择！";
    }
};

class MultiChoiceTalent : public TalentBase
{
  public:
    MultiChoiceTalent() : TalentBase({"B", Talent::多维抉择, "多维抉择", "天赋选择时，候选中额外提供1个A级与1个B级天赋"}) {}

    void ModifyTalentPoolPicks(const Player& player, uint32_t& a_pick, uint32_t& b_pick) const override
    {
        a_pick += 1;
        b_pick += 1;
    }
};

class ZhangSanTalent : public TalentBase
{
  public:
    ZhangSanTalent() : TalentBase({"B", Talent::张三来袭, "张三来袭", "你每放置一张3，获得3点生命"}) {}

    std::string OnCardPlaced(Player& player, uint32_t idx, const ScoreResult& original_result, ScoreResult& effective_result,
                             const TalentCardPlacedContext& context) override
    {
        if (idx == 0) return "";
        const auto& placed = player.comb_->GetCard(idx);
        if (!placed.has_value()) return "";
        int32_t threes = 0;
        for (uint32_t d = 0; d < k_direct_max; ++d) {
            if (placed->PointAt(d) == 3 || placed->PointAt(d) == 10) ++threes;
        }
        if (threes <= 0) return "";
        const int32_t heal = threes * 3;
        const int32_t actual_heal = player.Heal(heal);
        return "\n触发天赋「张三来袭」，获得 " + std::to_string(actual_heal) + " 点生命";
    }
};

class GreedyTreasureTalent : public TalentBase
{
  public:
    GreedyTreasureTalent() : TalentBase({"B", Talent::贪婪宝藏, "贪婪宝藏", "获得一张无法被覆盖的000，战败3次后变形为癞子"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        if (!active) return TalentInactiveText(Name());
        return Name() + TalentProgressText("(" + std::to_string(defeat_count) + "/3)");
    }

    std::string OnAcquire(Player& player, const TalentAcquireContext& context) override
    {
        player.extra_card_queue_.push_back({{AreaCard(0, 0, 0)}, Name(), false, Talent::贪婪宝藏});
        return "，获得一张无法被覆盖的000！";
    }

    std::string OnDiscard(Player& player, const AreaCard& card, const TalentDiscardContext& context) override
    {
        if (context.source_talent != Talent::贪婪宝藏 || !card.IsZero()) return "";
        active = false;
        disabled = true;
        position = 0;
        return "\n天赋「贪婪宝藏」的000被弃掉，天赋失效";
    }

    std::string OnCardPlaced(Player& player, uint32_t idx, const ScoreResult& original_result, ScoreResult& effective_result,
                             const TalentCardPlacedContext& context) override
    {
        if (context.source_talent != Talent::贪婪宝藏) return "";
        position = idx;
        active = true;
        disabled = false;
        transformed = false;
        defeat_count = 0;
        return "\n天赋「贪婪宝藏」生效：此000在变形前无法被覆盖";
    }

    std::string OnDefeat(Player& player, const TalentDefeatContext& context, int32_t& damage) override
    {
        if (!active || transformed || position == 0) return "";
        defeat_count++;
        if (defeat_count < 3) return "";
        transformed = true;
        active = false;
        auto result = player.comb_->SetWildAt(position);
        player.UpdateScore(result, context.has_valuable_one);
        return "触发天赋「贪婪宝藏」，位置 " + std::to_string(position) + " 的000变形为癞子！";
    }

    bool ProtectsPosition(uint32_t idx) const
    {
        return active && !transformed && !disabled && position == idx;
    }

    uint32_t position = 0;
    int32_t defeat_count = 0;
    bool active = false;
    bool transformed = false;
    bool disabled = false;
};

class ZeroPowerTalent : public TalentBase
{
  public:
    ZeroPowerTalent() : TalentBase({"B", Talent::零的力量, "0的力量", "获得一张000，只要000在场上存在，你放置卡牌时3视为4，6视为7"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        if (!active) return TalentInactiveText(Name());
        std::string s = Name();
        if (position > 0) {
            s += TalentTempScoreText("[位置" + std::to_string(position) + "]");
        }
        return s;
    }

    std::string OnAcquire(Player& player, const TalentAcquireContext& context) override
    {
        player.extra_card_queue_.push_back({{AreaCard(0, 0, 0)}, Name(), false, Talent::零的力量});
        return "，获得一张000；它在场时，你新放置的3视为4，6视为7！";
    }

    std::string OnBeforePlaceCard(Player& player, AreaCard& card, bool is_normal_round) override
    {
        if (!active) return "";
        AreaCard before = card;
        card.ApplyThreeSixToFourSeven();
        if (card == before) return "";
        return "\n触发天赋「0的力量」，该砖块的3视为4，6视为7！";
    }

    std::string OnCardPlaced(Player& player, uint32_t idx, const ScoreResult& original_result, ScoreResult& effective_result,
                             const TalentCardPlacedContext& context) override
    {
        if (context.source_talent == Talent::零的力量) {
            position = idx;
            active = true;
            return "\n天赋「0的力量」场地效果生效";
        }
        if (active && idx == position && context.previous_card != nullptr && context.previous_card->IsZero()) {
            active = false;
            position = 0;
            return "\n位置上的000被覆盖，天赋「0的力量」效果解除";
        }
        return "";
    }

    uint32_t position = 0;
    bool active = false;
};

class VoidHeartTalent : public TalentBase
{
  public:
    VoidHeartTalent() : TalentBase({"B", Talent::虚空之心, "虚空之心", "若你的10号位空置，则为你提供10点临时分"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        if (disabled || player.comb_->IsFilled(10)) return TalentInactiveText(Name());
        return Name() + TalentTempScoreText("[+10]");
    }

    int32_t TempBattleScore(const Player& player) const override
    {
        return (!disabled && !player.comb_->IsFilled(10)) ? 10 : 0;
    }

    std::string OnAcquire(Player& player, const TalentAcquireContext& context) override
    {
        disabled = player.comb_->IsFilled(10);
        return disabled ? "，但10号位已被占用，天赋未生效" : "，10号位空置时获得10点临时分！";
    }

    std::string OnCardPlaced(Player& player, uint32_t idx, const ScoreResult& original_result, ScoreResult& effective_result,
                             const TalentCardPlacedContext& context) override
    {
        if (disabled || idx != 10) return "";
        disabled = true;
        return "\n10号位被覆盖，天赋「虚空之心」失效";
    }

    bool disabled = false;
};

class PerformancePersonalityTalent : public TalentBase
{
  public:
    PerformancePersonalityTalent() : TalentBase({"B", Talent::表演型人格, "表演型人格", "此后，常规回合得分时总分-0.5%；选秀回合/额外卡牌放置得分时总分+3%"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        return Name() + (percent_permille >= 0 ? TalentExtraScoreText("(" + PercentText() + ")")
                                               : TalentPenaltyText("(" + PercentText() + ")"));
    }

    std::string ScoreDetail(const Player& player) const override
    {
        const int32_t bonus = current_bonus;
        if (bonus > 0) return "+" + TalentExtraScoreText("[表演+" + std::to_string(bonus) + "]");
        if (bonus < 0) return "+" + TalentPenaltyText("[表演" + std::to_string(bonus) + "]");
        return "";
    }

    int32_t ScoreDetailDisplayedPermanentExtra(const Player& player) const override
    {
        return current_bonus;
    }

    int32_t PermanentExtraScorePass() const override { return 1; }

    int32_t PermanentExtraScore(Player& player, int32_t current_extra) override
    {
        current_bonus = ScoreBonus(player, current_extra);
        return current_bonus;
    }

    std::string AfterScoreUpdatedOnCardPlaced(Player& player, uint32_t idx, const ScoreResult& effective_result,
                                              const TalentCardPlacedContext& context, int32_t old_permanent_extra) override
    {
        if (effective_result.score_delta <= 0) return "";
        percent_permille += PercentDeltaFor(context.source);
        player.UpdateScore(ScoreResult{player.comb_->BaseScore(), player.comb_->LineCount(), 0}, context.has_valuable_one);
        return "\n触发天赋「表演型人格」，当前倍率：" + PercentText();
    }

    int32_t ScoreBonus(const Player& player, int32_t extra_before_self) const
    {
        if (percent_permille == 0) return 0;
        const int32_t basis = player.base_score_ + player.valuable_one_bonus_ + extra_before_self;
        const int32_t abs_bonus = static_cast<int32_t>(std::ceil(std::abs(basis * percent_permille) / 1000.0));
        return percent_permille > 0 ? abs_bonus : -abs_bonus;
    }

    std::string PercentText() const
    {
        const int32_t abs_permille = std::abs(percent_permille);
        std::string text = percent_permille >= 0 ? "+" : "-";
        text += std::to_string(abs_permille / 10);
        if (abs_permille % 10 != 0) {
            text += "." + std::to_string(abs_permille % 10);
        }
        text += "%";
        return text;
    }

    int32_t PercentDeltaFor(TalentCardPlacementSource source) const
    {
        return source == TalentCardPlacementSource::RegularRound ? -5 : 30;
    }

    int32_t percent_permille = 0;
    int32_t current_bonus = 0;
};

class InnerRingTalent : public TalentBase
{
  public:
    InnerRingTalent() : TalentBase({"B", Talent::二环里, "二环里", "与10号位相邻的六个块若连接成功，这六个块朝向中心的方向各自变为单线癞子"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        if (triggered) return TalentInactiveText(Name());
        return Name() + TalentProgressText("(" + std::to_string(ConnectedPairCount(player)) + "/6)");
    }

    std::string OnAcquire(Player& player, const TalentAcquireContext& context) override
    {
        if (TryTrigger(player, context.has_valuable_one)) {
            return "，已达成条件，二环六块朝向中心的方向变为单线癞子！";
        }
        return "，若10号位周围二环连线成功，这六个块朝向中心的方向将变为单线癞子";
    }

    std::string AfterScoreUpdatedOnCardPlaced(Player& player, uint32_t idx, const ScoreResult& effective_result,
                                              const TalentCardPlacedContext& context, int32_t old_permanent_extra) override
    {
        if (!TryTrigger(player, context.has_valuable_one)) return "";
        return "\n触发天赋「二环里」，二环六块朝向中心的方向变为单线癞子！";
    }

    bool TryTrigger(Player& player, bool has_valuable_one)
    {
        if (triggered || !IsOuterRingCompleted(player)) return false;
        auto result = player.comb_->ApplyInnerRingTransform();
        player.UpdateScore(result, has_valuable_one);
        triggered = true;
        return true;
    }

    bool IsOuterRingCompleted(const Player& player) const
    {
        return ConnectedPairCount(player) >= 6;
    }

    int32_t ConnectedPairCount(const Player& player) const
    {
        int32_t count = 0;
        for (const auto& [lhs, rhs, dir] : OuterPairs()) {
            if (IsPairConnected(player, lhs, rhs, dir)) ++count;
        }
        return count;
    }

    std::array<std::tuple<uint32_t, uint32_t, uint32_t>, 6> OuterPairs() const
    {
        return {{
            {5, 6, 1}, {6, 11, 0}, {11, 15, 2}, {14, 15, 1}, {9, 14, 0}, {5, 9, 2},
        }};
    }

    bool IsPairConnected(const Player& player, uint32_t lhs, uint32_t rhs, uint32_t dir) const
    {
        const auto& lhs_card = player.comb_->GetCard(lhs);
        const auto& rhs_card = player.comb_->GetCard(rhs);
        if (!lhs_card.has_value() || !rhs_card.has_value()) return false;
        const int32_t lhs_value = lhs_card->PointAt(dir);
        const int32_t rhs_value = rhs_card->PointAt(dir);
        return lhs_value == 10 || rhs_value == 10 || lhs_value == rhs_value;
    }

    bool triggered = false;
};

class ChestnutTalent : public TalentBase
{
  public:
    ChestnutTalent() : TalentBase({"B", Talent::恭喜栗子, "恭喜栗子", "你的中线连成4、9、7时，分别加4分、9分、7分"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        const int32_t bonus = Bonus(player);
        if (bonus <= 0) return Name();
        return Name() + TalentProgressText("(+" + std::to_string(bonus) + ")");
    }

    int32_t PermanentExtraScore(Player& player, int32_t current_extra) override
    {
        return Bonus(player);
    }

    int32_t Bonus(const Player& player) const
    {
        int32_t bonus = 0;
        if (IsLineCompletedWith(player, {1, 5, 10, 15, 19}, 0, 4)) bonus += 4;
        if (IsLineCompletedWith(player, {8, 9, 10, 11, 12}, 1, 9)) bonus += 9;
        if (IsLineCompletedWith(player, {3, 6, 10, 14, 17}, 2, 7)) bonus += 7;
        return bonus;
    }

    bool IsLineCompletedWith(const Player& player, std::initializer_list<uint32_t> positions, uint32_t dir, int32_t target) const
    {
        for (uint32_t pos : positions) {
            const auto& card = player.comb_->GetCard(pos);
            if (!card.has_value()) return false;
            const int32_t value = card->PointAt(dir);
            if (value != target && value != 10) {
                return false;
            }
        }
        return true;
    }
};

class VitalityTalent : public TalentBase
{
  public:
    VitalityTalent() : TalentBase({"B", Talent::勃勃生机, "勃勃生机", "立即获得15点生命。你的回血效果翻倍"}) {}

    int32_t ModifyHealAmount(const Player& player, int32_t amount) const override
    {
        return amount * 2;
    }

    std::string OnAcquire(Player& player, const TalentAcquireContext& context) override
    {
        player.Heal(15, false);
        return "，立即获得15点生命，此后的回血效果翻倍！";
    }
};

class OneWayTalent : public TalentBase
{
  public:
    OneWayTalent() : TalentBase({"B", Talent::一方通行, "一方通行", "从X00、0X0、00X中选择一张牌"}) {}

    std::string OnAcquire(Player& player, const TalentAcquireContext& context) override
    {
        player.extra_card_queue_.push_back({{
            AreaCard(10, 0, 0),
            AreaCard(0, 10, 0),
            AreaCard(0, 0, 10),
        }, Name(), false, Talent::一方通行});
        return "，请在额外放置阶段从X00、0X0、00X中选择一张牌";
    }
};

class ZeroRiskInvestmentTalent : public TalentBase
{
  public:
    ZeroRiskInvestmentTalent() : TalentBase({"B", Talent::零风险投资, "零风险投资", "你的分数不会降低"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        std::string s = Name();
        const int32_t floor_bonus = FloorDisplay(player);
        if (floor_bonus > 0) {
            s += TalentProgressText("[+" + std::to_string(floor_bonus) + "]");
        }
        return s;
    }

    std::string ScoreDetail(const Player& player) const override
    {
        const int32_t floor_bonus = FloorDisplay(player);
        if (floor_bonus == 0) return "";
        return "+" + TalentProgressText("[保底+" + std::to_string(floor_bonus) + "]");
    }

    // 展示用保底加分：把当前临时分排除在保底外。
    // 数学层（ZeroRiskBaseFloor / EffectiveTempBattleScore）依旧保证总分不下降，
    // 这里仅决定 UI 上哪一部分挂在「保底」、哪一部分仍归在「临时分」。
    // 临时分在场时，保底只显示真正用于补偿盘面下降的部分；临时分到期后，
    // 原本由临时分占位的差值会自然过渡为保底显示。
    int32_t FloorDisplay(const Player& player) const
    {
        const int32_t raw = player.RawTotalScore();
        const int32_t temp = player.TempBattleScore();
        return std::max(0, max_total_score - raw - temp);
    }

    int32_t max_total_score = 0;
};

class BattleHardenedTalent : public TalentBase
{
  public:
    BattleHardenedTalent() : TalentBase({"B", Talent::以战代练, "以战代练", "在你和分数差距5以内的对手对战后，你的分数永久+3"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        if (accumulated == 0) return Name();
        return Name() + TalentProgressText("(+" + std::to_string(accumulated) + ")");
    }

    int32_t PermanentExtraScore(Player& player, int32_t current_extra) override { return accumulated; }

    std::string OnVictory(Player& player, const TalentVictoryContext& context) override
    {
        Accrue(player, context.my_battle_score, context.opponent_battle_score, context.has_valuable_one);
        return "";
    }

    std::string OnDefeat(Player& player, const TalentDefeatContext& context, int32_t& damage) override
    {
        Accrue(player, context.my_battle_score, context.opponent_battle_score, context.has_valuable_one);
        return "";
    }

    // 命中条件即时记账 +3，并立刻刷新 permanent_extra_，避免分数详情/总分滞后到下次 UpdateScore。
    void Accrue(Player& player, int64_t my_score, int64_t opp_score, bool has_valuable_one)
    {
        if (std::abs(my_score - opp_score) > 5) return;
        accumulated += 3;
        player.UpdateScore(ScoreResult{player.comb_->BaseScore(), player.comb_->LineCount(), 0}, has_valuable_one);
    }

    int32_t accumulated = 0;
};

class FatalRhythmTalent : public TalentBase
{
  public:
    FatalRhythmTalent() : TalentBase({"B", Talent::致命节奏, "致命节奏", "若没有弃牌，则获得1分；若弃牌，则清除通过本天赋获得的分数"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        if (bonus_score == 0) return Name();
        return Name() + TalentProgressText("(+" + std::to_string(bonus_score) + ")");
    }

    int32_t PermanentExtraScore(Player& player, int32_t current_extra) override { return bonus_score; }

    std::string OnCardPlaced(Player& player, uint32_t idx, const ScoreResult& original_result, ScoreResult& effective_result,
                             const TalentCardPlacedContext& context) override
    {
        bonus_score += 1;
        return "";
    }

    std::string OnDiscard(Player& player, const AreaCard& card, const TalentDiscardContext& context) override
    {
        if (bonus_score == 0) return "";
        const int32_t old = bonus_score;
        bonus_score = 0;
        // 立即刷新 permanent_extra_，否则 HandleDiscard_ 不会再调 UpdateScore，
        // 弃牌后到下次放置之间的 UI 仍会显示旧的 +X，且下次放置时 perm_delta 会错位。
        player.UpdateScore(ScoreResult{player.comb_->BaseScore(), player.comb_->LineCount(), 0}, context.has_valuable_one);
        return "\n天赋「致命节奏」效果变动，清除累计的 " + std::to_string(old) + " 分";
    }

    int32_t bonus_score = 0;
};

class TimeAnchorTalent : public TalentBase
{
  public:
    TimeAnchorTalent() : TalentBase({"B", Talent::时间锚, "时间锚", "记录你当前的血量，在下次天赋选择时将血量调整为此数值"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        if (!active) return TalentInactiveText(Name());
        return Name() + TalentTempScoreText("[血量" + std::to_string(anchored_hp) + "]");
    }

    std::string OnAcquire(Player& player, const TalentAcquireContext& context) override
    {
        anchored_hp = player.hp_;
        active = true;
        return "，已记录当前血量 " + std::to_string(anchored_hp);
    }

    // TalentStage::OnStageBegin 在进入新一次天赋选择时调用，把 hp_ 直接赋值为 anchored_hp。
    // 行为说明：
    //   - 当前血量低于锚定值：抬升（不走 Heal，因此不会被「勃勃生机」翻倍）。
    //   - 当前血量高于锚定值：倒扣回锚定值（强制重置）。
    //   - 当前血量等于锚定值：无变化但仍标记为已消耗。
    // 返回 true 表示触发并消耗；调用方负责播报具体的前后值。
    bool Restore(Player& player)
    {
        if (!active) return false;
        active = false;
        player.hp_ = anchored_hp;
        return true;
    }

    bool active = false;
    int32_t anchored_hp = 0;
};

class RhythmRemnantTalent : public TalentBase
{
  public:
    RhythmRemnantTalent() : TalentBase({"B", Talent::律动残余, "律动残余", "你每次受到的伤害不超过25"}) {}

    std::string OnDefeat(Player& player, const TalentDefeatContext& context, int32_t& damage) override
    {
        if (damage > 25) damage = 25;
        return "";
    }
};

inline std::unique_ptr<TalentBase> CreateTalentState(Talent talent)
{
    switch (talent) {
        case Talent::完美块: return std::make_unique<PerfectBlockTalent>();
        case Talent::绝地反击: return std::make_unique<CounterattackTalent>();
        case Talent::占得先机: return std::make_unique<SeizeTalent>();
        case Talent::钢铁之躯: return std::make_unique<IronBodyTalent>();
        case Talent::以退为进: return std::make_unique<RetreatAdvanceTalent>();
        case Talent::致命魔术: return std::make_unique<DeadlyMagicTalent>();
        case Talent::三相之力: return std::make_unique<TriForceTalent>();
        case Talent::紧急救援: return std::make_unique<EmergencyRescueTalent>();
        case Talent::我全都要: return std::make_unique<WantAllTalent>();
        case Talent::潘多拉魔盒: return std::make_unique<PandoraBoxTalent>();
        case Talent::利滚利: return std::make_unique<CompoundInterestTalent>();
        case Talent::戴森球: return std::make_unique<DysonSphereTalent>();
        case Talent::零号位: return std::make_unique<DiscardScorerTalent>();
        case Talent::坦诚相见: return std::make_unique<SincereTalent>();
        case Talent::星河流转: return std::make_unique<GalaxyFlowTalent>();
        case Talent::冥想: return std::make_unique<MeditationTalent>();
        case Talent::光波干涉: return std::make_unique<LightInterferenceTalent>();
        case Talent::九转玄机: return std::make_unique<NineMysteryTalent>();
        case Talent::乾坤大挪移: return std::make_unique<QiankunMoveTalent>();
        case Talent::关键选择: return std::make_unique<KeyChoiceTalent>();
        case Talent::生命游戏: return std::make_unique<LifeGameTalent>();
        case Talent::Y区域: return std::make_unique<YZoneTalent>();
        case Talent::嗜血: return std::make_unique<BloodlustTalent>();
        case Talent::还是有用的: return std::make_unique<StillUsefulTalent>();
        case Talent::快攻: return std::make_unique<SwiftAttackTalent>();
        case Talent::特立独行: return std::make_unique<IndependentTalent>();
        case Talent::成双成对: return std::make_unique<InPairsTalent>();
        case Talent::垃圾回收: return std::make_unique<TrashRecycleTalent>();
        case Talent::来点实在的: return std::make_unique<SomethingRealTalent>();
        case Talent::攻击形态: return std::make_unique<OffensiveFormTalent>();
        case Talent::防御形态: return std::make_unique<DefensiveFormTalent>();
        case Talent::局部强化: return std::make_unique<LocalEnhanceTalent>();
        case Talent::有舍有得: return std::make_unique<GainAfterLossTalent>();
        case Talent::三年之期: return std::make_unique<ThreeYearTalent>();
        case Talent::锻造: return std::make_unique<ForgeTalent>();
        case Talent::尾货处理: return std::make_unique<TailGoodsTalent>();
        case Talent::图灵测试: return std::make_unique<TuringTestTalent>();
        case Talent::事不过三: return std::make_unique<NoMoreThanThreeTalent>();
        case Talent::摇奖机: return std::make_unique<SlotMachineTalent>();
        case Talent::两极反转: return std::make_unique<DigitReverseTalent>();
        case Talent::败者之刃: return std::make_unique<LoserBladeTalent>();
        case Talent::临时用品: return std::make_unique<TempWildTalent>();
        case Talent::包扎: return std::make_unique<BandageTalent>();
        case Talent::百味草: return std::make_unique<HerbalGrowthTalent>();
        case Talent::天使轮: return std::make_unique<AngelRoundTalent>();
        case Talent::劫掠: return std::make_unique<PlunderTalent>();
        case Talent::多维抉择: return std::make_unique<MultiChoiceTalent>();
        case Talent::张三来袭: return std::make_unique<ZhangSanTalent>();
        case Talent::贪婪宝藏: return std::make_unique<GreedyTreasureTalent>();
        case Talent::零的力量: return std::make_unique<ZeroPowerTalent>();
        case Talent::虚空之心: return std::make_unique<VoidHeartTalent>();
        case Talent::表演型人格: return std::make_unique<PerformancePersonalityTalent>();
        case Talent::二环里: return std::make_unique<InnerRingTalent>();
        case Talent::恭喜栗子: return std::make_unique<ChestnutTalent>();
        case Talent::勃勃生机: return std::make_unique<VitalityTalent>();
        case Talent::一方通行: return std::make_unique<OneWayTalent>();
        case Talent::零风险投资: return std::make_unique<ZeroRiskInvestmentTalent>();
        case Talent::以战代练: return std::make_unique<BattleHardenedTalent>();
        case Talent::致命节奏: return std::make_unique<FatalRhythmTalent>();
        case Talent::时间锚: return std::make_unique<TimeAnchorTalent>();
        case Talent::律动残余: return std::make_unique<RhythmRemnantTalent>();
        case Talent::COUNT: break;
    }
    return std::make_unique<WantAllTalent>();
}

inline const TalentInfo& GetTalentInfo(Talent t)
{
    static const auto infos = [] {
        std::array<TalentInfo, static_cast<size_t>(Talent::COUNT)> result{};
        for (int i = 0; i < static_cast<int>(Talent::COUNT); ++i) {
            const Talent talent = static_cast<Talent>(i);
            result[static_cast<size_t>(talent)] = CreateTalentState(talent)->Info();
        }
        return result;
    }();
    const auto idx = static_cast<size_t>(t);
    if (idx >= infos.size()) {
        static const TalentInfo unknown{"", Talent::COUNT, "未知", ""};
        return unknown;
    }
    return infos[idx];
}

inline std::string TalentName(Talent t)
{
    return GetTalentInfo(t).name;
}

inline std::string TalentDescription(Talent t)
{
    return GetTalentInfo(t).description;
}

inline std::string_view TalentGrade(Talent t)
{
    return GetTalentInfo(t).grade;
}

inline bool IsGrade(Talent t, std::string_view grade)
{
    return TalentGrade(t) == grade;
}

inline bool IsGradeA(Talent t) { return IsGrade(t, "A"); }
inline bool IsGradeB(Talent t) { return IsGrade(t, "B"); }

inline const std::vector<Talent>& TalentsOfGrade(std::string_view grade)
{
    static std::map<std::string, std::vector<Talent>> cache;
    auto it = cache.find(std::string(grade));
    if (it != cache.end()) return it->second;
    std::vector<Talent> r;
    for (int i = 0; i < static_cast<int>(Talent::COUNT); ++i) {
        const auto talent = static_cast<Talent>(i);
        if (TalentGrade(talent) == grade) r.push_back(talent);
    }
    return cache.emplace(std::string(grade), std::move(r)).first->second;
}

inline const std::vector<Talent>& GradeATalents() { return TalentsOfGrade("A"); }
inline const std::vector<Talent>& GradeBTalents() { return TalentsOfGrade("B"); }

inline std::map<std::string, int> MakeTalentOptionMap()
{
    std::map<std::string, int> m;
    for (int i = 0; i < static_cast<int>(Talent::COUNT); ++i) {
        const auto talent = static_cast<Talent>(i);
        m.emplace(TalentName(talent), static_cast<int>(talent));
    }
    return m;
}

inline std::map<std::string, int> MakeGradeBTalentOptionMap()
{
    std::map<std::string, int> m;
    for (const auto talent : GradeBTalents()) {
        m.emplace(TalentName(talent), static_cast<int>(talent));
    }
    return m;
}
