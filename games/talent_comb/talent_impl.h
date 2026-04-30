// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).
//
// This file was generated with the assistance of Claude Code (claude.ai/code).
//
// talent_impl.h — Talent effects, UI rendering, and battle settlement logic for talent_comb.
// This file contains MainStage method implementations that are included from mygame.cc.

// ===== UI / Display =====

inline std::string MainStage::GetName(const std::string& x)
{
    std::string ret;
    int n = x.length();
    if (n == 0) return ret;
    int l = 0;
    int r = n - 1;
    if (x[0] == '<') l++;
    if (x[r] == '>') {
        while (r >= 0 && x[r] != '(') r--;
        r--;
    }
    for (int i = l; i <= r; i++) {
        ret += x[i];
    }
    return ret;
}

// Build score display as two lines: "积分：120" and detail "(90+12+14+[4])"
// Returns {score_line, detail_line}. detail_line is always present (empty string with &nbsp; if no detail).
inline std::pair<std::string, std::string> MainStage::PlayerScoreDetail(const PlayerID pid) const
{
    const auto& player = players_[pid];
    int32_t base = player.base_score_;
    int32_t perm = player.permanent_extra_;
    int32_t valuable = player.valuable_one_bonus_;
    int32_t temp = player.TempBattleScore();
    int32_t total = player.TotalScore() + temp;
    std::string talent_detail;
    for (const auto talent : player.talents_) {
        const auto& state = player.talent_states_.at(talent);
        perm -= state->ScoreDetailDisplayedPermanentExtra(player);
        talent_detail += state->ScoreDetail(player);
    }

    std::string score_line = std::to_string(total);
    std::string detail_line;

    bool has_detail = (perm != 0 || valuable != 0 || temp != 0 || !talent_detail.empty());
    if (has_detail) {
        detail_line += "（";
        detail_line += std::to_string(base);
        if (valuable != 0) {
            detail_line += "+" HTML_COLOR_FONT_HEADER(#1E3A8A) + std::to_string(valuable) + HTML_FONT_TAIL;
        }
        if (perm != 0) {
            detail_line += (perm > 0 ? "+" : "") + std::to_string(perm);
        }
        detail_line += talent_detail;
        if (temp != 0) {
            detail_line += "+" HTML_COLOR_FONT_HEADER(#FF6347) "[" + std::to_string(temp) + "]" HTML_FONT_TAIL;
        }
        detail_line += "）";
    } else {
        detail_line = "&nbsp;"; // Reserve empty line to prevent UI misalignment
    }
    return {score_line, detail_line};
}

inline void MainStage::SetPlayerBoard(html::Table& table, const int pos, const PlayerID pid, const bool isEliminated)
{
    // Build talent display string with extra info indicators
    // Sort display: A级 talents first, then B级
    std::vector<Talent> sorted_talents = players_[pid].talents_;
    std::stable_sort(sorted_talents.begin(), sorted_talents.end(), [](Talent a, Talent b) {
        return IsGradeA(a) && !IsGradeA(b);
    });

    std::string talent_str;
    for (size_t i = 0; i < sorted_talents.size(); ++i) {
        if (i > 0 && i % 4 == 0) {
            talent_str += "<br>";
        } else if (i > 0) {
            talent_str += " ";
        }
        const auto talent = sorted_talents[i];
        const auto& player = players_[pid];

        talent_str += player.talent_states_.at(talent)->BoardDisplay(player);
    }
    if (talent_str.empty()) talent_str = "无";

    auto [score_val, score_detail] = PlayerScoreDetail(pid);
    std::string board = "### " + Global().PlayerAvatar(pid, 40) + "&nbsp;&nbsp; " + Global().PlayerName(pid) + "</h3>"
                        "<h3 style=\"margin:12px 0 0 0;\">"
                        HTML_COLOR_FONT_HEADER(green) "积分：" + score_val + HTML_FONT_TAIL "</h3>"
                        "<p style=\"margin:0 0 2px 0;\">"
                        HTML_COLOR_FONT_HEADER(green) + score_detail + HTML_FONT_TAIL "</p>"
                        "<h3 style=\"margin:4px 0;\">"
                        HTML_COLOR_FONT_HEADER(#8B0000) "血量：" + std::to_string(players_[pid].hp_) + HTML_FONT_TAIL
                        HTML_COLOR_FONT_HEADER(#FF8C00) "　连线：" + std::to_string(players_[pid].comb_->LineCount()) + HTML_FONT_TAIL "</h3>"
                        "<p style=\"margin:4px 0;\">"
                        HTML_COLOR_FONT_HEADER(#4169E1) + talent_str + HTML_FONT_TAIL "</p>"
                        "<div style=\"margin-top:10px;\">" + players_[pid].comb_->ToHtml() + "</div>";
    if (isEliminated) {
        table.Get(pos / 2, pos % 2).SetColor("#C0C0C0").SetContent(board);
    } else {
        table.Get(pos / 2, pos % 2).SetContent(board);
    }
}

inline std::string MainStage::CombHtml(const std::string& str)
{
    html::Table table(players_.size() / 2 + 1, 2);
    table.SetTableStyle("align=\"center\" cellpadding=\"20\" cellspacing=\"0\"");
    int pos = 0;
    // Active players
    for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
        if (player_out_[pid] == 0) {
            SetPlayerBoard(table, pos++, pid, false);
        }
    }
    // Eliminated players
    for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
        if (player_out_[pid] > 0) {
            SetPlayerBoard(table, pos++, pid, true);
        }
    }
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << dRate_;
    return str + "（伤害倍率：" + ss.str() + "）" + GetStyle(Global().ResourceDir()) + table.ToString();
}

// ===== Talent Effects / Settlement =====

// Called after a card is placed to update score and check talents.
// Returns {notification string, base score delta (board-only change)}.
inline std::pair<std::string, int32_t> MainStage::OnCardPlaced_(PlayerID pid, uint32_t idx, const ScoreResult& result,
                                                                std::optional<Talent> source_talent,
                                                                const AreaCard* previous_card,
                                                                TalentCardPlacementSource placement_source)
{
    auto& player = players_[pid];
    std::string notify;

    ScoreResult effective_result = result;
    static constexpr Talent k_card_placed_order[] = {
        Talent::星河流转,
        Talent::张三来袭,
        Talent::贪婪宝藏,
        Talent::零的力量,
        Talent::虚空之心,
    };
    for (const auto talent : k_card_placed_order) {
        if (!player.HasTalent(talent)) continue;
        notify += player.talent_states_.at(talent)->OnCardPlaced(
            player, idx, result, effective_result,
            TalentCardPlacedContext{HasValuableOne(special_event_), placement_source, source_talent, previous_card});
    }

    // Track old values to detect changes
    int32_t old_perm = player.permanent_extra_;
    int32_t old_valuable = player.valuable_one_bonus_;
    // For ZERO_RISK: track old total to compute actual net gain (floor may offset losses)
    const bool has_zero_risk = player.HasTalent(Talent::零风险投资);

    // Update base score
    player.UpdateScore(effective_result, HasValuableOne(special_event_));

    // "有1吗" special event: notify if bonus changed
    if (HasValuableOne(special_event_)) {
        int32_t valuable_delta = player.valuable_one_bonus_ - old_valuable;
        if (valuable_delta > 0) {
            notify += "\n触发特殊事件「有1吗」，额外获得 " + std::to_string(valuable_delta) + " 点积分";
        } else if (valuable_delta < 0) {
            notify += "\n特殊事件「有1吗」积分变化，减少 " + std::to_string(-valuable_delta) + " 点额外积分";
        }
    }

    // Permanent extra changes are reported as a generic talent effect notification.
    {
        int32_t perm_delta = player.permanent_extra_ - old_perm;
        if (perm_delta > 0) {
            notify += "\n触发天赋效果，额外获得 " + std::to_string(perm_delta) + " 点积分";
        } else if (perm_delta < 0) {
            notify += "\n天赋效果变动，减少 " + std::to_string(-perm_delta) + " 点额外积分";
        }
    }

    static constexpr Talent k_after_score_order[] = {
        Talent::戴森球,
        Talent::冥想,
        Talent::天使轮,
        Talent::表演型人格,
        Talent::二环里,
    };
    for (const auto talent : k_after_score_order) {
        if (!player.HasTalent(talent)) continue;
        notify += player.talent_states_.at(talent)->AfterScoreUpdatedOnCardPlaced(
            player, idx, effective_result,
            TalentCardPlacedContext{HasValuableOne(special_event_), placement_source, source_talent, previous_card}, old_perm);
    }

    // Return score delta:
    // - For ZERO_RISK: always non-negative (floor prevents true loss; show 0 if board went down)
    // - Otherwise: board change only, excluding talent/event extras
    int32_t return_delta = has_zero_risk ? std::max(0, effective_result.score_delta) : effective_result.score_delta;
    return {notify, return_delta};
}

// Transform card based on active talents before placement.
// Returns the transformed card and notification string.
// is_normal_round: true if from RoundStage (for 三相之力 tracking)
inline std::pair<AreaCard, std::string> MainStage::TransformCardForPlacement_(PlayerID pid, const AreaCard& card, bool is_normal_round)
{
    auto& player = players_[pid];
    AreaCard actual = card;
    std::string notify;

    static constexpr Talent k_transform_order[] = {
        Talent::三相之力,
        Talent::两级反转,
        Talent::九转玄机,
        Talent::零的力量,
        Talent::以退为进,
        Talent::完美块,
    };
    for (const auto talent : k_transform_order) {
        if (!player.HasTalent(talent)) continue;
        notify += player.talent_states_.at(talent)->OnBeforePlaceCard(player, actual, is_normal_round);
    }

    return {actual, notify};
}

// Handle discard (position 0) talent effects. Returns notification string.
inline std::string MainStage::HandleDiscard_(PlayerID pid, const AreaCard& card, std::optional<Talent> source_talent)
{
    auto& player = players_[pid];
    std::string notify;

    static constexpr Talent k_discard_order[] = {
        Talent::垃圾回收,
        Talent::零号位,
        Talent::锻造,
        Talent::贪婪宝藏,
    };
    for (const auto talent : k_discard_order) {
        if (!player.HasTalent(talent)) continue;
        notify += player.talent_states_.at(talent)->OnDiscard(player, card, TalentDiscardContext{HasValuableOne(special_event_), source_talent});
    }

    return notify;
}

// Apply "三年之期" storage for a player this round
// Returns true if the card was stored (player skips placement)
inline bool MainStage::HandleThreeYearStore_(PlayerID pid, const AreaCard& card)
{
    auto& player = players_[pid];
    if (!player.HasTalent(Talent::三年之期)) return false;
    if (!player.ThreeYear().active) return false;
    if (player.ThreeYear().rounds_left <= 0) return false;

    player.ThreeYear().cards.push_back(card);
    player.ThreeYear().rounds_left--;
    if (player.ThreeYear().rounds_left == 0) {
        // Cards will be placed after this round's battle (in post-battle extras)
    }
    return true;
}

// ===== Battle System =====

inline bool MainStage::DoBattle_()
{
    if (alive_ <= 1) return false;

    // Skip battle for rounds 1, 2, and selection rounds
    if (round_ <= 2 || IsSelectionRound(round_)) {
        Global().Boardcast() << "本轮不进行玩家对战";
        return false;
    }

    std::string result;
    std::vector<PlayerID> alive_players;
    for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
        if (player_out_[pid] == 0) {
            alive_players.push_back(pid);
        }
    }

    int32_t min_round = (alive_ + 1) / 2 - 1;
    if (alive_ == 2) min_round = 0;
    while (fought_list_.size() > static_cast<size_t>(min_round)) {
        fought_list_.erase(fought_list_.begin());
    }

    // Apply temporary battle score from talents
    std::map<PlayerID, int32_t> temp_scores;
    for (PlayerID pid : alive_players) {
        int32_t temp = players_[pid].TempBattleScore();
        if (temp != 0) {
            temp_scores[pid] = temp;
        }
    }

    // Generate fight pairs (avoid recent repeats)
    std::unordered_map<int32_t, int32_t> fight_map;
    std::vector<PlayerID> list = alive_players;
    bool retry;
    int32_t max_attempts = 100;
    do {
        retry = false;
        fight_map.clear();
        SeededShuffle(list.begin(), list.end(), g_);
        for (size_t i = 0; i + 1 < list.size(); i += 2) {
            for (const auto& hist : fought_list_) {
                auto it1 = hist.find(list[i]);
                if (it1 != hist.end() && it1->second == list[i + 1]) {
                    retry = true;
                    break;
                }
                auto it2 = hist.find(list[i + 1]);
                if (it2 != hist.end() && it2->second == list[i]) {
                    retry = true;
                    break;
                }
            }
            if (retry) break;
            fight_map[list[i]] = list[i + 1];
        }
    } while (retry && --max_attempts > 0);

    fought_list_.push_back(fight_map);
    while (fought_list_.size() > static_cast<size_t>(min_round)) {
        fought_list_.erase(fought_list_.begin());
    }

    // Process each battle pair
    for (const auto& entry : fight_map) {
        const int32_t pid1 = entry.first;
        const int32_t pid2 = entry.second;
        ProcessBattle_(PlayerID(pid1), PlayerID(pid2), false, result);
    }

    // Mirror match for odd player
    if (list.size() % 2 == 1) {
        PlayerID solo = list.back();
        PlayerID mirror;
        int32_t attempts = 0;
        do {
            mirror = list[RandInt(g_, 0, static_cast<uint32_t>(list.size() - 2))];
        } while (mirror == last_mirror_ && ++attempts < 20);
        last_mirror_ = mirror;
        ProcessBattle_(solo, mirror, true, result);
    }

    // 绝地反击 的触发/清除时机说明（终局计分规则相关）：
    //   本轮触发的 Counterattack().extra_score 要保留到下一次真实对战结束后清除；
    //   若中间遇到选牌轮或其他跳过对战的阶段，则继续保留。若那次对战后游戏
    //   即时结束，则保留 extra 以计入最终结算。清理由外层在确认本轮真实发生
    //   对战且 CheckGameOver()==false 后调用 ClearCounterattackAfterRound_()。

    // Update damage rate
    fought_round_++;
    if (dRate_ < 1.0) {
        dRate_ = std::min(std::exp(fought_round_ * 0.22 / Global().PlayerNum()) * iRate_, 1.0);
    }
    if (dRate_ > 1.0) {
        dRate_ = std::max(std::exp(fought_round_ * -0.44 / Global().PlayerNum()) * iRate_, 1.0);
    }

    // Store fight map for 劫掠 talent
    current_fight_map_ = fight_map;

    Global().Boardcast() << result;
    return true;
}

// Apply damage to a player, tracking 三年之期 damage if active
inline void MainStage::ApplyDamage_(PlayerID pid, int32_t damage)
{
    players_[pid].hp_ -= damage;
    if (players_[pid].ThreeYear().active) {
        players_[pid].ThreeYear().damage_stored += damage;
    }
}

inline void MainStage::ProcessBattle_(PlayerID pid1, PlayerID pid2, bool mirror, std::string& result)
{
    // 坦诚相见: if either player has this talent, only compare base_score_ (ignores all extras)
    const bool sincere = players_[pid1].HasTalent(Talent::坦诚相见) ||
                         (!mirror && players_[pid2].HasTalent(Talent::坦诚相见));
    const int64_t score1 = sincere ? players_[pid1].base_score_ : PlayerBattleScore(pid1);
    const int64_t score2 = sincere ? players_[pid2].base_score_ : PlayerBattleScore(pid2);
    const std::string name1 = GetName(Global().PlayerName(pid1));
    const std::string name2 = GetName(Global().PlayerName(pid2));

    const int32_t score_cmp = (score1 > score2) ? 1 : (score1 < score2 ? -1 : 0);
    int32_t base_damage = static_cast<int32_t>((score1 - score2) * dRate_);
    int32_t extra_damage = 0;

    std::string p1_info, p2_info;
    if (mirror) p2_info = "(镜像)";

    if (score_cmp > 0) {
        // pid1 wins, pid2 loses
        if (!mirror) players_[pid2].never_lost_ = false;
        extra_damage = ApplyAttackTalents_(pid1, pid2, base_damage);
        int32_t total = base_damage + extra_damage;
        ApplyVictoryTalents_(pid1, mirror, result);

        // Apply defense talents for pid2 (injured party)
        int32_t defense = ApplyDefenseTalents_(pid2, total);
        total += defense;
        if (total < 0) total = 0;

        if (!mirror) {
            ApplyDefeatTalents_(pid2, mirror, total, result);
            ApplyDamage_(pid2, total);
            p2_info = "(" + std::to_string(-total) + ")";
        }
    } else if (score_cmp < 0) {
        base_damage = std::abs(base_damage); // make positive for talent processing
        // pid2 wins, pid1 loses
        players_[pid1].never_lost_ = false;
        if (!mirror) {
            extra_damage = ApplyAttackTalents_(pid2, pid1, base_damage);
            int32_t total = base_damage + extra_damage;

            int32_t defense = ApplyDefenseTalents_(pid1, total);
            total += defense;
            if (total < 0) total = 0;

            ApplyVictoryTalents_(pid2, mirror, result);
            ApplyDefeatTalents_(pid1, mirror, total, result);
            ApplyDamage_(pid1, total);
            p1_info = "(" + std::to_string(-total) + ")";
        } else {
            extra_damage = ApplyAttackTalents_(pid2, pid1, base_damage);
            int32_t total = base_damage + extra_damage;
            int32_t defense = ApplyDefenseTalents_(pid1, total);
            total += defense;
            if (total < 0) total = 0;

            ApplyDefeatTalents_(pid1, mirror, total, result);
            ApplyDamage_(pid1, total);
            p1_info = "(" + std::to_string(-total) + ")";
        }
    } else {
        if (!mirror) p2_info = "(0)";
    }

    result += name1 + p1_info + " vs " + name2 + p2_info + "\n";
}

// Returns extra damage from attacker's talents
inline int32_t MainStage::ApplyAttackTalents_(PlayerID attacker, PlayerID defender, int32_t damage)
{
    int32_t extra = 0;
    static constexpr Talent k_attack_order[] = {
        Talent::快攻,
        Talent::致命魔术,
        Talent::攻击形态,
        Talent::防御形态,
    };
    for (const auto talent : k_attack_order) {
        if (!players_[attacker].HasTalent(talent)) continue;
        auto effect = players_[attacker].talent_states_.at(talent)->AttackDamageDelta(players_[attacker], players_[defender], damage, g_);
        extra += effect.delta;
        if (!effect.message.empty()) {
            Global().Boardcast() << At(attacker) << " " << effect.message;
        }
    }
    return extra;
}

// Returns damage reduction (negative = less damage taken)
inline int32_t MainStage::ApplyDefenseTalents_(PlayerID defender, int32_t damage)
{
    int32_t reduction = 0;
    static constexpr Talent k_defense_order[] = {
        Talent::钢铁之躯,
        Talent::防御形态,
        Talent::攻击形态,
    };
    for (const auto talent : k_defense_order) {
        if (!players_[defender].HasTalent(talent)) continue;
        reduction += players_[defender].talent_states_.at(talent)->DefenseDamageDelta(players_[defender], damage);
    }
    return reduction;
}

inline void MainStage::ApplyDefeatTalents_(PlayerID loser, bool mirror, int32_t& damage, std::string& result)
{
    static constexpr Talent k_defeat_order[] = {
        Talent::图灵测试,
        Talent::事不过三,
        Talent::有舍有得,
        Talent::败者之刃,
        Talent::贪婪宝藏,
    };
    for (const auto talent : k_defeat_order) {
        if (!players_[loser].HasTalent(talent)) continue;
        const auto notify = players_[loser].talent_states_.at(talent)->OnDefeat(
            players_[loser], TalentDefeatContext{HasValuableOne(special_event_), mirror}, damage);
        if (!notify.empty()) {
            result += GetName(Global().PlayerName(loser)) + " " + notify + "\n";
        }
    }
}

inline void MainStage::ApplyVictoryTalents_(PlayerID winner, bool mirror, std::string& result)
{
    static constexpr Talent k_victory_order[] = {
        Talent::嗜血,
        Talent::败者之刃,
    };
    for (const auto talent : k_victory_order) {
        if (!players_[winner].HasTalent(talent)) continue;
        const auto notify = players_[winner].talent_states_.at(talent)->OnVictory(players_[winner], TalentVictoryContext{mirror});
        if (!notify.empty()) {
            result += GetName(Global().PlayerName(winner)) + " " + notify + "\n";
        }
    }
}

// Process deaths and eliminations after battle
inline void MainStage::DoEliminationAfterBattle_()
{
    for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
        if (player_out_[pid] != 0 || players_[pid].hp_ > 0) continue;

        // Check 绝地反击
        if (players_[pid].HasTalent(Talent::绝地反击) && !players_[pid].Counterattack().used) {
            players_[pid].hp_ = 1;
            players_[pid].Counterattack().triggered = true;
            players_[pid].Counterattack().used = true;
            players_[pid].Counterattack().extra_score = static_cast<int32_t>(std::ceil(players_[pid].TotalScore() * 0.15));
            players_[pid].Counterattack().trigger_round = round_;
            Global().Boardcast() << At(pid) << " 触发天赋「绝地反击」，血量降为1，获得 "
                                 << players_[pid].Counterattack().extra_score << " 点反击加成！";
            continue;
        }

        // Player dies
        alive_--;
        player_out_[pid] = round_;
        Global().Boardcast() << At(pid) << " 已被淘汰！";
        Global().Eliminate(pid);

        // Check 劫掠: find the opponent from current fight map
        std::optional<PlayerID> killer;
        for (const auto& [p1, p2] : current_fight_map_) {
            if (PlayerID(p1) == pid && player_out_[PlayerID(p2)] == 0 && players_[PlayerID(p2)].HasTalent(Talent::劫掠)) {
                killer = PlayerID(p2);
                break;
            } else if (PlayerID(p2) == pid && player_out_[PlayerID(p1)] == 0 && players_[PlayerID(p1)].HasTalent(Talent::劫掠)) {
                killer = PlayerID(p1);
                break;
            }
        }
        if (killer.has_value()) {
            // Pick up to 5 random filled positions from dead player's board
            auto filled = players_[pid].comb_->GetFilledPositions();
            SeededShuffle(filled.begin(), filled.end(), g_);
            size_t pick_count = std::min<size_t>(5, filled.size());
            std::vector<AreaCard> loot_cards;
            for (size_t i = 0; i < pick_count; ++i) {
                const auto& card = players_[pid].comb_->GetCard(filled[i]);
                if (card.has_value()) {
                    loot_cards.push_back(*card);
                }
            }
            if (!loot_cards.empty()) {
                players_[*killer].extra_card_queue_.push_back({std::move(loot_cards), "劫掠"});
                Global().Boardcast() << At(*killer) << " 触发天赋「劫掠」，从被淘汰玩家处获得 "
                                     << players_[*killer].extra_card_queue_.back().cards.size() << " 枚砖块供选择！";
            }
        }
    }
}

inline void MainStage::DoPoison_()
{
    bool any_poisoned = false;
    std::string poison_msg;
    for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
        if (player_out_[pid] != 0) continue;
        if (players_[pid].poison_layers_ > 0) {
            int32_t damage = players_[pid].poison_layers_;
            ApplyDamage_(pid, damage);
            poison_msg += GetName(Global().PlayerName(pid)) + "（" + std::to_string(-damage) + "）";
            // 百味草: gain 1 score per 1 HP lost to poison
            if (players_[pid].HasTalent(Talent::百味草)) {
                players_[pid].HerbalGrowth().poison_score += damage;
                players_[pid].UpdateScore(ScoreResult{players_[pid].comb_->BaseScore(), players_[pid].comb_->LineCount(), 0}, HasValuableOne(special_event_));
                poison_msg += "，触发「百味草」+" + std::to_string(damage) + "分";
            }
            poison_msg += "\n";
            any_poisoned = true;
        }
    }
    if (any_poisoned) {
        Global().Boardcast() << "中毒结算：\n" << poison_msg;
        // Check deaths from poison
        for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
            if (player_out_[pid] != 0 || players_[pid].hp_ > 0) continue;
            if (players_[pid].HasTalent(Talent::绝地反击) && !players_[pid].Counterattack().used) {
                players_[pid].hp_ = 1;
                players_[pid].Counterattack().triggered = true;
                players_[pid].Counterattack().used = true;
                players_[pid].Counterattack().extra_score = static_cast<int32_t>(std::ceil(players_[pid].TotalScore() * 0.15));
                players_[pid].Counterattack().trigger_round = round_;
                Global().Boardcast() << At(pid) << " 触发天赋「绝地反击」，获得 "
                                     << players_[pid].Counterattack().extra_score << " 点反击加成！";
                continue;
            }
            alive_--;
            player_out_[pid] = round_;
            Global().Boardcast() << At(pid) << " 因中毒被淘汰！";
            Global().Eliminate(pid);
        }
    }
}

// Collect pre-battle extra cards
inline void MainStage::CollectPreBattleExtras_()
{
    for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
        if (player_out_[pid] != 0) continue;
        auto& player = players_[pid];

        static constexpr Talent k_pre_battle_extra_order[] = {
            Talent::锻造,
            Talent::有舍有得,
        };
        for (const auto talent : k_pre_battle_extra_order) {
            if (!player.HasTalent(talent)) continue;
            std::optional<AreaCard> offered_card;
            if (talent == Talent::有舍有得) {
                std::vector<uint32_t> pool_mid;
                for (uint32_t v : k_points[1]) {
                    if (special_event_ == SpecialEvent::大的要来了 && v == 1) continue;
                    if (special_event_ == SpecialEvent::两极分化 && v == 5) continue;
                    if (special_event_ == SpecialEvent::大的没了 && v == 9) continue;
                    pool_mid.push_back(v);
                }
                if (pool_mid.empty()) {
                    pool_mid.assign(k_points[1].begin(), k_points[1].end());
                }
                offered_card.emplace(
                    k_points[0][RandInt(g_, 0, static_cast<uint32_t>(k_points[0].size() - 1))],
                    pool_mid[RandInt(g_, 0, static_cast<uint32_t>(pool_mid.size() - 1))],
                    k_points[2][RandInt(g_, 0, static_cast<uint32_t>(k_points[2].size() - 1))]);
            }
            const auto notify = player.talent_states_.at(talent)->OnPreBattleExtraCards(
                player, TalentPreBattleExtraContext{HasValuableOne(special_event_), special_event_, g_,
                                                    offered_card.has_value() ? &*offered_card : nullptr});
            if (!notify.empty()) {
                Global().Boardcast() << At(pid) << " " << notify;
            }
        }
    }
}

// Collect post-battle extra cards (三年之期)
inline void MainStage::CollectPostBattleExtras_()
{
    for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
        if (player_out_[pid] != 0) continue;
        auto& player = players_[pid];

        // 三年之期: if ready to place (3 cards stored and waited one round)
        if (player.HasTalent(Talent::三年之期) && player.ThreeYear().active && player.ThreeYear().rounds_left == 0 && !player.ThreeYear().cards.empty()) {
            // Push as a single multi-card entry so player can see all and pick one at a time
            player.extra_card_queue_.push_back({player.ThreeYear().cards, "三年之期", true});
            // Restore HP accumulated during storage period
            if (player.ThreeYear().damage_stored > 0) {
                const int32_t restored = player.Heal(player.ThreeYear().damage_stored);
                Global().Boardcast() << At(pid) << " 触发天赋「三年之期」，恢复存储期间受到的 "
                                     << restored << " 点伤害！";
                player.ThreeYear().damage_stored = 0;
            }
            player.ThreeYear().cards.clear();
            player.ThreeYear().active = false;
        }
    }
}

// ===== Card Pool Management =====

// Draw next card from pool2
inline AreaCard MainStage::DrawFromPool2_()
{
    if (it2_ == cards2_.end()) {
        // Reshuffle pool2
        SeededShuffle(cards2_.begin(), cards2_.end(), g_);
        it2_ = cards2_.begin();
    }
    return *(it2_++);
}

// Draw N balanced initial cards from pool2 (pseudo-random: minimize PointSum spread)
// Under special events that reduce card pool, target max sum difference <= 4
inline std::vector<AreaCard> MainStage::DrawBalancedCards_(uint32_t count)
{
    // Draw a candidate pool (draw more than needed, pick the most balanced set)
    const uint32_t candidate_count = std::min(static_cast<uint32_t>(std::distance(it2_, cards2_.end())),
                                               count * 3);
    if (candidate_count < count) {
        // Not enough candidates, reshuffle
        SeededShuffle(cards2_.begin(), cards2_.end(), g_);
        it2_ = cards2_.begin();
    }

    // Draw candidates
    std::vector<AreaCard> candidates;
    uint32_t draw_count = std::min(std::max(count * 3, count + 6), static_cast<uint32_t>(std::distance(it2_, cards2_.end())));
    for (uint32_t i = 0; i < draw_count; ++i) {
        candidates.push_back(*(it2_++));
    }

    // Sort candidates by PointSum
    std::sort(candidates.begin(), candidates.end(), [](const AreaCard& a, const AreaCard& b) {
        return a.PointSum() < b.PointSum();
    });

    // Find the best window of `count` consecutive cards with minimal spread
    int32_t best_spread = INT32_MAX;
    size_t best_start = 0;
    for (size_t i = 0; i + count <= candidates.size(); ++i) {
        int32_t spread = candidates[i + count - 1].PointSum() - candidates[i].PointSum();
        if (spread < best_spread) {
            best_spread = spread;
            best_start = i;
        }
    }

    // Take the best window
    std::vector<AreaCard> result(candidates.begin() + best_start, candidates.begin() + best_start + count);

    // Put unused candidates back into pool (before the iterator)
    // Since we already advanced it2_, we rebuild the remaining pool
    std::vector<AreaCard> unused;
    for (size_t i = 0; i < candidates.size(); ++i) {
        if (i < best_start || i >= best_start + count) {
            unused.push_back(candidates[i]);
        }
    }
    // Insert unused cards back at current position
    auto pos = it2_ - cards2_.begin();
    cards2_.insert(it2_, unused.begin(), unused.end());
    it2_ = cards2_.begin() + pos + unused.size();

    // Shuffle the result so assignment order is random
    SeededShuffle(result.begin(), result.end(), g_);
    return result;
}

// Draw next card from pool1 (should never be called when exhausted)
inline AreaCard MainStage::DrawFromPool1_()
{
    assert(it_ != cards_.end());
    return *(it_++);
}


