// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).
//
// Player struct — per-player state, score calculation, talent pool management.

#pragma once

// Note: this header is included from the bottom of talent_comb.h after AreaCard
// and TalentComb are defined. Do not include it directly — include talent_comb.h.
#include "utility/random.h"
#include "games/talent_comb/talent.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

// ==================== Player ====================

struct Player
{
    Player(std::string resource_path, int32_t init_hp = 150)
        : comb_(new TalentComb(std::move(resource_path)))
        , hp_(init_hp)
        , base_score_(0)
        , permanent_extra_(0)
        , valuable_one_bonus_(0)
        , max_total_score_(0)
        , is_leave_(false)
        , poison_layers_(0)
        , counterattack_triggered_(false)
        , counterattack_used_(false)
        , three_year_active_(false)
        , three_year_rounds_left_(0)
        , discard_count_(0)
        , loss_count_(0)
        , talent_selection_count_(0)
        , selection_priority_(2)  // default: normal priority (1 = high from 占得先机)
    {}

    Player(Player&&) = default;

    // --- Core state ---
    std::unique_ptr<TalentComb> comb_;
    int32_t hp_;
    int32_t base_score_;
    int32_t permanent_extra_;    // Accumulated from talents: 垃圾回收, 来点实在的, etc.
    int32_t valuable_one_bonus_; // "有1吗" special event bonus (stored separately for display)
    int32_t max_total_score_;    // For 零风险投资
    bool is_leave_;

    // --- Talents ---
    std::vector<Talent> talents_;          // Acquired talents
    std::vector<Talent> talent_pool_;      // Current talent choices (3 options)
    std::set<Talent> available_a_;         // Remaining grade A talents
    std::set<Talent> available_b_;         // Remaining grade B talents
    uint32_t talent_selection_count_;      // Number of talent selections made (not affected by PANDORA_BOX/WANT_ALL bonus talents)

    // --- Talent-specific state ---
    int32_t poison_layers_;                // 中毒叠加层数
    bool counterattack_triggered_;         // 绝地反击 death trigger activated
    bool counterattack_used_;              // 绝地反击 already consumed
    int32_t counterattack_extra_ = 0;      // 绝地反击 permanent bonus score while active
    int32_t counterattack_trigger_round_ = 0; // Round when 绝地反击 extra was granted
    bool three_year_active_;               // 三年之期 currently storing cards
    int32_t three_year_rounds_left_;       // Rounds left to store
    std::vector<AreaCard> three_year_cards_;  // Stored cards
    int32_t three_year_damage_stored_ = 0; // Damage accumulated during storage for HP restore
    std::vector<int32_t> forge_fragments_; // 锻造 collected fragments
    int32_t discard_count_;                // For 垃圾回收
    int32_t loss_count_;                   // For 有舍有得
    std::array<bool, 3> local_enhance_triggered_ = {false, false, false};  // 局部强化 per-digit trigger
    int32_t tri_force_progress_ = 0;       // 三相之力 progress (0-3)
    int32_t defeat_count_ = 0;             // 事不过三 defeat counter (0-3, at 3 next defeat is immune)
    bool emergency_rescue_used_ = false;   // 紧急救援 one-time use flag
    int32_t compound_interest_accumulated_ = 0; // 利滚利 accumulated interest
    bool dyson_sphere_activated_ = false;      // 戴森球 condition already activated
    int32_t loser_blade_temp_ = 0;         // 败者之刃 accumulated temp score (cleared on win)
    int32_t discard_scorer_temp_ = 0;     // 0号位 temp score from discards (cleared each round)
    int32_t poison_score_ = 0;             // 百味草 accumulated score from poison damage
    uint32_t temp_wild_pos_ = 0;           // 临时用品 position on board (0 = none)
    int32_t temp_wild_rounds_ = 0;         // 临时用品 rounds remaining
    bool meditation_active_ = false;       // 冥想 active (true until 单次连线分 ≥ 25)
    bool angel_round_active_ = false;      // 天使轮 active (+15 temp until next line)

    // Extra card queue: each entry has 1+ cards and a source name.
    // 1 card = direct placement; >1 cards = player picks one then places it.
    struct ExtraCardEntry {
        std::vector<AreaCard> cards;
        std::string source;
        bool place_all = false;  // true: place each card one by one; false: pick one, discard rest
    };
    std::vector<ExtraCardEntry> extra_card_queue_;

    // Selection priority (1 = 占得先机, 2 = normal)
    int32_t selection_priority_;

    // Battle tracking (for achievements)
    bool never_lost_ = true;        // True if player never lost a battle (draws OK)
    int32_t first_talent_round_ = 0; // Round when first talent was selected (0 = never)

    // --- Helper methods ---

    bool HasTalent(Talent t) const
    {
        return std::find(talents_.begin(), talents_.end(), t) != talents_.end();
    }

    // Calculate raw total (without ZERO_RISK floor)
    int32_t RawTotalScore() const
    {
        return base_score_ + permanent_extra_ + valuable_one_bonus_;
    }

    // Calculate total score (base + permanent extra + valuable_one, with ZERO_RISK floor and DYSON_SPHERE bonus)
    // counterattack_extra_ is added AFTER floor/dyson so it doesn't inflate those calculations
    int32_t TotalScore() const
    {
        int32_t total = RawTotalScore();
        if (HasTalent(Talent::零风险投资)) {
            total = std::max(total, max_total_score_);
        }
        if (HasTalent(Talent::戴森球) && CountCompletedLength3Lines_() >= 6) {
            total = static_cast<int32_t>(std::ceil(total * 1.06));
        }
        total += counterattack_extra_;
        return total;
    }

    // ZERO_RISK floor adjustment (how much the floor added)
    int32_t ZeroRiskFloor() const
    {
        if (!HasTalent(Talent::零风险投资)) return 0;
        int32_t raw = RawTotalScore();
        int32_t floored = std::max(raw, max_total_score_);
        return floored - raw;
    }

    // DYSON_SPHERE bonus (how much the 6% adds)
    int32_t DysonSphereBonus() const
    {
        if (!HasTalent(Talent::戴森球) || CountCompletedLength3Lines_() < 6) return 0;
        int32_t before_dyson = RawTotalScore() + ZeroRiskFloor();
        int32_t after_dyson = static_cast<int32_t>(std::ceil(before_dyson * 1.06));
        return after_dyson - before_dyson;
    }

    // Calculate the "还是有用的" bonus based on current board state
    int32_t StillUsefulBonus() const
    {
        if (!HasTalent(Talent::还是有用的)) return 0;
        int32_t bonus = 0;
        for (const auto& line_def : k_all_lines) {
            if (IsLineCompleted_(line_def)) {
                int32_t matched = GetLineMatchedValue_(line_def);
                if (matched == 1) bonus += 3;
                if (matched == 2) bonus += 6;
            }
        }
        return bonus;
    }

    // 光波干涉: 若用相同数字形成了相同长度的连线（≥2 条同数同长度），这些连线整体获得 15% 额外分数。
    // 实现方式：按 (matched_value, length) 分组累计连线分；分组数量 ≥2 时对该组总分取 ceil(total * 0.15)。
    int32_t LightInterferenceBonus() const
    {
        if (!HasTalent(Talent::光波干涉)) return 0;
        // key: (matched_value, length); value: {count, total_line_score}
        std::map<std::pair<int32_t, size_t>, std::pair<int32_t, int32_t>> groups;
        for (const auto& line_def : k_all_lines) {
            if (!IsLineCompleted_(line_def)) continue;
            int32_t matched = GetLineMatchedValue_(line_def);     // 0 表示全癞子行
            size_t len = line_def.positions.size();
            int32_t line_score = (matched == 0 ? 10 : matched) * static_cast<int32_t>(len);
            auto& entry = groups[{matched, len}];
            entry.first += 1;
            entry.second += line_score;
        }
        int32_t bonus = 0;
        for (const auto& [key, val] : groups) {
            if (val.first >= 2) {
                bonus += static_cast<int32_t>(std::ceil(val.second * 0.15));
            }
        }
        return bonus;
    }

    // Calculate the "有1吗" special event bonus
    int32_t ValuableOneBonus() const
    {
        int32_t bonus = 0;
        for (const auto& line_def : k_all_lines) {
            if (IsLineCompleted_(line_def)) {
                int32_t matched = GetLineMatchedValue_(line_def);
                if (matched == 1) bonus += 12;
            }
        }
        return bonus;
    }

    // Check if all filled cells share the same number in any direction (wilds count as any)
    bool AllCellsSameNumber() const
    {
        auto filled = comb_->GetFilledPositions();
        if (filled.size() < 19) return false; // Must have all cells filled

        for (uint32_t dir = 0; dir < k_direct_max; ++dir) {
            // Try each possible digit 1-9
            for (int32_t digit = 1; digit <= 9; ++digit) {
                bool all_match = true;
                for (uint32_t pos : filled) {
                    const auto& card = comb_->GetCard(pos);
                    if (!card.has_value()) { all_match = false; break; }
                    int32_t val = card->PointAt(dir);
                    if (val != 10 && val != digit) {  // 10 = wild in this direction
                        all_match = false;
                        break;
                    }
                }
                if (all_match) return true;
            }
        }
        return false;
    }

    // Check if any length-5 line is entirely wild in its direction
    bool HasFullWildLine5() const
    {
        for (const auto& line_def : k_all_lines) {
            if (line_def.positions.size() != 5) continue;
            const uint32_t dir_idx = static_cast<uint32_t>(line_def.direction);
            bool all_wild = true;
            for (uint32_t pos : line_def.positions) {
                const auto& card = comb_->GetCard(pos);
                if (!card.has_value() || card->PointAt(dir_idx) != 10) {
                    all_wild = false;
                    break;
                }
            }
            if (all_wild) return true;
        }
        return false;
    }

    // Calculate temporary battle score from talents
    int32_t TempBattleScore() const
    {
        int32_t temp = 0;
        int32_t total = TotalScore();
        if (HasTalent(Talent::特立独行) && total % 2 == 1) temp += 6;
        if (HasTalent(Talent::成双成对) && total % 2 == 0) temp += 6;
        // COUNTERATTACK bonus is now in counterattack_extra_ (part of TotalScore), not here
        temp += loser_blade_temp_;      // 败者之刃 accumulated temp score
        temp += discard_scorer_temp_;   // 0号位 temp score from discards this round
        if (HasTalent(Talent::天使轮) && angel_round_active_) temp += 15;  // 天使轮
        return temp;
    }

    // Update base score from board rescore result
    void UpdateScore(const ScoreResult& result, bool has_valuable_one)
    {
        base_score_ = result.base_score;
        permanent_extra_ = RecalcPermanentExtra_();
        valuable_one_bonus_ = has_valuable_one ? ValuableOneBonus() : 0;
        int32_t total = base_score_ + permanent_extra_ + valuable_one_bonus_;
        if (HasTalent(Talent::零风险投资)) {
            max_total_score_ = std::max(max_total_score_, total);
        }
    }

    // Generate talent pool based on Python logic
    void GenerateTalentPool(std::mt19937& rng)
    {
        talent_pool_.clear();
        uint32_t talent_count = talent_selection_count_;
        std::vector<Talent> pool_a(available_a_.begin(), available_a_.end());
        std::vector<Talent> pool_b(available_b_.begin(), available_b_.end());

        if (pool_a.empty() && pool_b.empty()) return;

        // 基础 A/B 名额分配（随 talent_count 提升，A 占比增加）
        uint32_t a_pick = 0, b_pick = 0;
        if (talent_count == 0) {
            b_pick = 3;
        } else if (talent_count == 1 || talent_count == 2) {
            a_pick = 1; b_pick = 2;
        } else if (talent_count == 3) {
            a_pick = 2; b_pick = 1;
        } else {
            a_pick = 3;
        }
        // 多维抉择：每次天赋候选额外提供 1 个 A 级 + 1 个 B 级（共 5 选 1）
        if (HasTalent(Talent::多维抉择)) {
            a_pick += 1;
            b_pick += 1;
        }

        std::vector<Talent> selected;
        auto a_picks = SampleTalents_(pool_a, a_pick, rng);
        auto b_picks = SampleTalents_(pool_b, b_pick, rng);
        selected.insert(selected.end(), a_picks.begin(), a_picks.end());
        selected.insert(selected.end(), b_picks.begin(), b_picks.end());

        // 若某池不够：用对方池补齐（尽量凑满 a_pick + b_pick 个选项）
        uint32_t target = a_pick + b_pick;
        auto try_fill_from = [&](const std::vector<Talent>& src) {
            for (const auto& t : src) {
                if (selected.size() >= target) break;
                if (std::find(selected.begin(), selected.end(), t) == selected.end()) {
                    selected.push_back(t);
                }
            }
        };
        try_fill_from(pool_a);
        try_fill_from(pool_b);

        talent_pool_ = selected;
    }

    // Acquire a talent from the pool
    void AcquireTalent(uint32_t choice_idx)
    {
        assert(choice_idx < talent_pool_.size());
        Talent chosen = talent_pool_[choice_idx];
        talents_.push_back(chosen);
        available_a_.erase(chosen);
        available_b_.erase(chosen);
        talent_pool_.clear();
        ++talent_selection_count_;
    }

    // Initialize available talent pools
    void InitTalentPools()
    {
        for (auto t : k_grade_a_talents) available_a_.insert(t);
        for (auto t : k_grade_b_talents) available_b_.insert(t);
    }

    // Remove talents incompatible with special events (called after InitTalentPools)
    void RemoveEventIncompatibleTalents(SpecialEvent event)
    {
        // 完美块 requires 1s in the card pool; remove from 大的要来了(无1) event
        if (event == SpecialEvent::大的要来了) {
            available_a_.erase(Talent::完美块);
        }
        // 两级反转 requires both 1 and 9 in card pool
        if (event == SpecialEvent::大的要来了 || event == SpecialEvent::大的没了) {
            available_b_.erase(Talent::两级反转);
        }
    }

    // Count completed lines of length 3 (for 戴森球) — public for UI display
    int32_t CountCompletedLength3Lines_() const
    {
        int32_t count = 0;
        for (const auto& line_def : k_all_lines) {
            if (line_def.positions.size() == 3 && IsLineCompleted_(line_def)) {
                count++;
            }
        }
        return count;
    }

  private:
    // Recalculate 局部强化 bonus from current board state.
    // Each of the 3 directions on pos 10 defines an enhance digit.
    // A completed line matching that digit triggers +3 for that direction (once per direction, cap 9).
    // Wild direction (value 10) matches ANY completed line's digit.
    int32_t LocalEnhanceBonus_()
    {
        const auto& center_card = comb_->GetCard(10);
        if (!center_card.has_value()) {
            local_enhance_triggered_ = {false, false, false};
            return 0;
        }

        std::array<int32_t, 3> enhance_nums = {
            center_card->PointAt(0),
            center_card->PointAt(1),
            center_card->PointAt(2)
        };

        // Recalculate from scratch each time
        std::array<bool, 3> new_triggered = {false, false, false};
        for (const auto& line_def : k_all_lines) {
            if (!IsLineCompleted_(line_def)) continue;
            int32_t matched = GetLineMatchedValue_(line_def);
            for (int i = 0; i < 3; ++i) {
                if (new_triggered[i]) continue;
                if (enhance_nums[i] == 10) {
                    // Wild direction: any completed line triggers this direction
                    new_triggered[i] = true;
                } else if (matched == enhance_nums[i]) {
                    new_triggered[i] = true;
                }
            }
        }
        local_enhance_triggered_ = new_triggered;

        int32_t bonus = 0;
        for (int i = 0; i < 3; ++i) {
            if (local_enhance_triggered_[i]) bonus += 3;  // 局部强化：每方向 +3，上限 9
        }
        return bonus;
    }

    int32_t RecalcPermanentExtra_()
    {
        int32_t extra = 0;
        if (HasTalent(Talent::来点实在的)) extra += 4;
        if (HasTalent(Talent::垃圾回收)) extra += discard_count_ * 2;
        extra += StillUsefulBonus();
        extra += LightInterferenceBonus();        // 光波干涉：同数同长度≥2条连线整体+15%
        if (HasTalent(Talent::局部强化)) extra += LocalEnhanceBonus_();
        extra += compound_interest_accumulated_;  // 利滚利 accumulated interest
        extra += poison_score_;                   // 百味草 accumulated score from poison
        return extra;
    }

    bool IsLineCompleted_(const LineDefinition& line_def) const
    {
        const uint32_t dir_idx = static_cast<uint32_t>(line_def.direction);
        std::optional<int32_t> matched_value;
        for (uint32_t pos : line_def.positions) {
            const auto& card = comb_->GetCard(pos);
            if (!card.has_value()) return false;
            int32_t val = card->PointAt(dir_idx);
            if (val != 10) {  // 10 = wild in this direction
                if (!matched_value.has_value()) {
                    matched_value = val;
                } else if (val != *matched_value) {
                    return false;
                }
            }
        }
        return true;
    }

    int32_t GetLineMatchedValue_(const LineDefinition& line_def) const
    {
        const uint32_t dir_idx = static_cast<uint32_t>(line_def.direction);
        for (uint32_t pos : line_def.positions) {
            const auto& card = comb_->GetCard(pos);
            if (card.has_value() && card->PointAt(dir_idx) != 10) {
                return card->PointAt(dir_idx);
            }
        }
        return 0; // all wild
    }

    static std::vector<Talent> SampleTalents_(std::vector<Talent>& pool, uint32_t count, std::mt19937& rng)
    {
        std::vector<Talent> result;
        if (pool.empty() || count == 0) return result;
        SeededShuffle(pool.begin(), pool.end(), rng);
        for (uint32_t i = 0; i < std::min(count, static_cast<uint32_t>(pool.size())); ++i) {
            result.push_back(pool[i]);
        }
        // Remove selected from pool for this call (not permanent)
        return result;
    }
};

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot
