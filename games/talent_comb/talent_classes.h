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
inline TalentDamageEffect TalentBase::AttackDamageDelta(Player& attacker, Player& defender, int32_t damage, std::mt19937& rng) { return {}; }
inline int32_t TalentBase::DefenseDamageDelta(Player& defender, int32_t damage) { return 0; }

inline constexpr const char* kTalentColorInactive = "#999999";
inline constexpr const char* kTalentColorProgress = "#DAA520";
inline constexpr const char* kTalentColorTempScore = "#FF6347";
inline constexpr const char* kTalentColorExtraScore = "#1E3A8A";
inline constexpr const char* kTalentColorBlueBuff = "#4169E1";
inline constexpr const char* kTalentColorPenalty = "#8B0000";
inline constexpr const char* kTalentColorPermanentGreen = "green";

inline std::string TalentColoredText(const char* color, const std::string& text)
{
    return "<font color=" + std::string(color) + ">" + text + HTML_FONT_TAIL;
}

inline std::string TalentInactiveText(const std::string& text) { return TalentColoredText(kTalentColorInactive, text); }
inline std::string TalentProgressText(const std::string& text) { return TalentColoredText(kTalentColorProgress, text); }
inline std::string TalentTempScoreText(const std::string& text) { return TalentColoredText(kTalentColorTempScore, text); }
inline std::string TalentExtraScoreText(const std::string& text) { return TalentColoredText(kTalentColorExtraScore, text); }
inline std::string TalentPenaltyText(const std::string& text) { return TalentColoredText(kTalentColorPenalty, text); }

class PerfectBlockTalent : public TalentBase
{
  public:
    PerfectBlockTalent() : TalentBase({"A", Talent::完美块, "完美块", "你的312视为万能牌"}) {}

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
            return Name() + TalentColoredText(kTalentColorBlueBuff, "[+" + std::to_string(extra_score) + "]");
        }
        if (used) {
            return TalentInactiveText(Name());
        }
        return TalentColoredText(kTalentColorBlueBuff, Name());
    }

    std::string ScoreDetail(const Player& player) const override
    {
        if (extra_score == 0) return "";
        return "+" + TalentColoredText(kTalentColorBlueBuff, "[反击+" + std::to_string(extra_score) + "]");
    }

    bool triggered = false;
    bool used = false;
    int32_t extra_score = 0;
    int32_t trigger_round = 0;
};

class SeizeTalent : public TalentBase
{
  public:
    SeizeTalent() : TalentBase({"A", Talent::占得先机, "占得先机", "选牌阶段优先选牌"}) {}

    std::string OnAcquire(Player& player, const TalentAcquireContext& context) override
    {
        selection_priority = 1;
        return "";
    }

    int32_t selection_priority = 2;
};

class ZeroRiskInvestmentTalent : public TalentBase
{
  public:
    ZeroRiskInvestmentTalent() : TalentBase({"A", Talent::零风险投资, "零风险投资", "你的分数不会降低"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        std::string s = Name();
        const int32_t floor_bonus = player.ZeroRiskFloor();
        if (floor_bonus > 0) {
            s += TalentProgressText("[+" + std::to_string(floor_bonus) + "]");
        }
        return s;
    }

    std::string ScoreDetail(const Player& player) const override
    {
        const int32_t floor_bonus = player.ZeroRiskFloor();
        if (floor_bonus == 0) return "";
        return "+" + TalentProgressText("[保底+" + std::to_string(floor_bonus) + "]");
    }

    int32_t max_total_score = 0;
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
        return TalentColoredText(kTalentColorBlueBuff, Name());
    }

    bool used = false;
};

class WantAllTalent : public TalentBase
{
  public:
    WantAllTalent() : TalentBase({"A", Talent::我全都要, "我全都要", "下次选择天赋时获得全部"}) {}
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
            s += TalentTempScoreText("(+" + std::to_string(temp_score) + ")");
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
        std::string s = TalentColoredText("#D94A6A", Name());
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
        if (active) player.hp_ += 5;
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
        const int32_t bonus = player.LightInterferenceBonus();
        if (bonus > 0) {
            s += TalentProgressText("(+" + std::to_string(bonus) + ")");
        }
        return s;
    }
};

class NineMysteryTalent : public TalentBase
{
  public:
    NineMysteryTalent() : TalentBase({"A", Talent::九转玄机, "九转玄机", "你的9视为癞子线"}) {}

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

class BloodlustTalent : public TalentBase
{
  public:
    BloodlustTalent() : TalentBase({"B", Talent::嗜血, "嗜血", "战斗获胜时，生命值+4"}) {}

    std::string OnVictory(Player& player, const TalentVictoryContext& context) override
    {
        player.hp_ += 4;
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
        const int32_t bonus = player.StillUsefulBonus();
        if (bonus > 0) {
            s += TalentProgressText("(+" + std::to_string(bonus) + ")");
        }
        return s;
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

    int32_t discard_count = 0;
};

class SomethingRealTalent : public TalentBase
{
  public:
    SomethingRealTalent() : TalentBase({"B", Talent::来点实在的, "来点实在的", "立刻获得4分"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        return Name() + TalentColoredText(kTalentColorPermanentGreen, "(+4)");
    }

    std::string OnAcquire(Player& player, const TalentAcquireContext& context) override
    {
        return "，立即获得 4 分！";
    }
};

class OffensiveFormTalent : public TalentBase
{
  public:
    OffensiveFormTalent() : TalentBase({"B", Talent::攻击形态, "攻击形态", "造成的伤害增加15%，受到的伤害增加5%"}) {}

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
    DefensiveFormTalent() : TalentBase({"B", Talent::防御形态, "防御形态", "受到的伤害减少15%，造成的伤害减少5%"}) {}

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

    std::array<bool, 3> triggered = {false, false, false};
};

class GainAfterLossTalent : public TalentBase
{
  public:
    GainAfterLossTalent() : TalentBase({"B", Talent::有舍有得, "有舍有得", "每战败4次，随机获得一枚额外的砖块"}) {}

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
        if (loss_count < 4 || context.offered_card == nullptr) return "";
        player.extra_card_queue_.push_back({{*context.offered_card}, Name(), false, Talent::有舍有得});
        loss_count -= 4;
        return "触发天赋「有舍有得」，获得 1 枚额外砖块";
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
    DigitReverseTalent() : TalentBase({"B", Talent::两级反转, "两级反转", "你的1视为9，你的9视为1"}) {}

    std::string OnAcquire(Player& player, const TalentAcquireContext& context) override
    {
        player.comb_->ApplyDigitReverse();
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
            s += TalentTempScoreText("(+" + std::to_string(temp_score) + ")");
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
        return Name() + TalentProgressText("(" + std::to_string(rounds_left) + "回合)");
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
        player.extra_card_queue_.push_back({{wild_card}, Name()});
        return "，请在额外放置阶段选择位置放置临时癞子（3回合后到期）";
    }

    std::string OnRoundStart(Player& player, const TalentRoundContext& context) override
    {
        if (position <= 0 || context.is_selection_round) return "";
        rounds_left--;
        if (rounds_left > 0) return "";
        const uint32_t expired_position = position;
        const auto& card = player.comb_->GetCard(expired_position);
        if (card.has_value() && card->IsWild()) {
            auto result = player.comb_->RemoveCard(expired_position);
            player.UpdateScore(result, context.has_valuable_one);
        }
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
        player.hp_ += 20;
        return "，立即获得 20 点生命！";
    }
};

class HerbalGrowthTalent : public TalentBase
{
  public:
    HerbalGrowthTalent() : TalentBase({"B", Talent::百味草, "百味草", "获得1层中毒，每因中毒损失1点生命，获得1分"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        std::string s = Name() + TalentColoredText("#6A0DAD", "(" + std::to_string(player.poison_layers_) + "毒");
        if (poison_score > 0) {
            s += TalentColoredText("#6A0DAD", "+" + std::to_string(poison_score) + "分)");
        } else {
            s += TalentColoredText("#6A0DAD", ")");
        }
        return s;
    }

    std::string OnAcquire(Player& player, const TalentAcquireContext& context) override
    {
        player.poison_layers_++;
        return "，获得1层中毒（当前中毒：" + std::to_string(player.poison_layers_) + " 层）";
    }

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
};

class MultiChoiceTalent : public TalentBase
{
  public:
    MultiChoiceTalent() : TalentBase({"B", Talent::多维抉择, "多维抉择", "天赋选择时，候选中额外提供1个A级与1个B级天赋"}) {}
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
        player.hp_ += heal;
        return "\n触发天赋「张三来袭」，获得 " + std::to_string(heal) + " 点生命";
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
    ZeroPowerTalent() : TalentBase({"B", Talent::零的力量, "0的力量", "获得一张000，只要000在场上存在，你放置卡牌时3视为4"}) {}

    std::string BoardDisplay(const Player& player) const override
    {
        if (!active) return TalentInactiveText(Name());
        std::string s = Name();
        if (position > 0) {
            s += TalentProgressText("[位置" + std::to_string(position) + "]");
        }
        return s;
    }

    std::string OnAcquire(Player& player, const TalentAcquireContext& context) override
    {
        player.extra_card_queue_.push_back({{AreaCard(0, 0, 0)}, Name(), false, Talent::零的力量});
        return "，获得一张000；它在场时，你新放置的3视为4！";
    }

    std::string OnBeforePlaceCard(Player& player, AreaCard& card, bool is_normal_round) override
    {
        if (!active) return "";
        AreaCard before = card;
        card.ApplyThreeToFour();
        if (card == before) return "";
        return "\n触发天赋「0的力量」，该砖块的3视为4！";
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
    PerformancePersonalityTalent() : TalentBase({"B", Talent::表演型人格, "表演型人格", "此后，常规回合得分时总分-0.5%；选秀回合/额外卡牌放置得分时总分+2%"}) {}

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
        return source == TalentCardPlacementSource::RegularRound ? -5 : 20;
    }

    int32_t percent_permille = 0;
    int32_t current_bonus = 0;
};

inline std::unique_ptr<TalentBase> CreateTalentState(Talent talent)
{
    switch (talent) {
        case Talent::完美块: return std::make_unique<PerfectBlockTalent>();
        case Talent::绝地反击: return std::make_unique<CounterattackTalent>();
        case Talent::占得先机: return std::make_unique<SeizeTalent>();
        case Talent::零风险投资: return std::make_unique<ZeroRiskInvestmentTalent>();
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
        case Talent::两级反转: return std::make_unique<DigitReverseTalent>();
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
