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
    // 顶部总分仍走数学层（EffectiveTempBattleScore 经过零风险压制，避免与保底重复计入）；
    // 括号内的临时分直接显示玩家"当前"持有的临时分。零风险天赋的 ScoreDetail 也已经把
    // 临时分占用的部分从「保底+X」里扣除，于是断点：raw + 保底_visible + 临时分 = 总分。
    int32_t display_temp = player.TempBattleScore();
    int32_t total = player.TotalScore() + player.EffectiveTempBattleScore();
    std::string talent_detail;
    for (const auto talent : player.talents_) {
        const auto& state = player.talent_states_.at(talent);
        perm -= state->ScoreDetailDisplayedPermanentExtra(player);
        talent_detail += state->ScoreDetail(player);
    }

    std::string score_line = std::to_string(total);
    std::string detail_line;

    bool has_detail = (perm != 0 || valuable != 0 || display_temp != 0 || !talent_detail.empty());
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
        if (display_temp != 0) {
            detail_line += "+" HTML_COLOR_FONT_HEADER(#FF6347) "[" + std::to_string(display_temp) + "]" HTML_FONT_TAIL;
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

    for (const auto talent : k_after_score_order) {
        if (!player.HasTalent(talent)) continue;
        notify += player.talent_states_.at(talent)->AfterScoreUpdatedOnCardPlaced(
            player, idx, effective_result,
            TalentCardPlacedContext{HasValuableOne(special_event_), placement_source, source_talent, previous_card}, old_perm);
    }

    // Permanent extra changes are reported as a generic talent effect notification.
    // 放在 after-score hooks 之后：例如表演型人格在 AfterScoreUpdatedOnCardPlaced 内修改百分比并触发
    // UpdateScore，此时 permanent_extra_ 才是最终值；提早比较会显示与玩家实际拿到的额外分不符的数值。
    {
        int32_t perm_delta = player.permanent_extra_ - old_perm;
        if (perm_delta > 0) {
            notify += "\n触发天赋效果，额外获得 " + std::to_string(perm_delta) + " 点积分";
        } else if (perm_delta < 0) {
            notify += "\n天赋效果变动，减少 " + std::to_string(-perm_delta) + " 点额外积分";
        }
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

    for (const auto talent : k_discard_order) {
        if (!player.HasTalent(talent)) continue;
        notify += player.talent_states_.at(talent)->OnDiscard(player, card, TalentDiscardContext{HasValuableOne(special_event_), source_talent});
    }
    player.UpdateZeroRiskMaxScore();

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

inline bool MainStage::DoBattle_(MsgSenderBase::MsgSenderGuard& sender)
{
    if (alive_ <= 1) return false;

    // Skip battle for rounds 1, 2, and selection rounds
    if (round_ <= 2 || IsSelectionRound(round_)) {
        sender << "本轮不进行玩家对战";
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

    // Snapshot pre-battle scores for mirror battles, so mid-round mutations
    // (e.g. 贪婪宝藏 transforming 000→wild after a real defeat) do not leak
    // into the mirror opponent's score this same battle round.
    std::unordered_map<int32_t, std::pair<int64_t, int32_t>> pre_battle_scores;
    for (PlayerID pid : alive_players) {
        pre_battle_scores[pid.Get()] = {PlayerBattleScore(pid), players_[pid].base_score_};
    }

    // Generate fight pairs (avoid recent repeats)
    std::unordered_map<int32_t, int32_t> fight_map;
    std::vector<PlayerID> list = alive_players;
    bool retry;
    int32_t max_attempts = 100;
    do {
        retry = false;
        fight_map.clear();
        SeededShuffle(list.begin(), list.end(), battle_rng_);
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
            mirror = list[RandInt(battle_rng_, 0, static_cast<uint32_t>(list.size() - 2))];
        } while (mirror == last_mirror_ && ++attempts < 20);
        last_mirror_ = mirror;
        const auto snap_it = pre_battle_scores.find(mirror.Get());
        std::optional<int64_t> mirror_battle_score;
        std::optional<int32_t> mirror_base_score;
        if (snap_it != pre_battle_scores.end()) {
            mirror_battle_score = snap_it->second.first;
            mirror_base_score = snap_it->second.second;
        }
        ProcessBattle_(solo, mirror, true, result, mirror_battle_score, mirror_base_score);
    }


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

    sender << result;
    return true;
}

// 应用伤害到玩家。若 (hp_ - damage <= 0) 先给玩家"已获取"的天赋一次 OnLethalDamage 机会
// （救援/吸收类动作型 hook，必须玩家持有该天赋才能生效），然后再扣血。
// 之后分发 OnDamageReceived：遍历 talent_states_ 而非 talents_，
// 让「生命游戏」等需要从游戏开局就累计的"记账型 hook"在被玩家获取之前也能记账。
// 返回 OnLethalDamage 累计的播报文本（不含玩家名/前缀），调用方负责拼前缀并写入 sender/result。
inline std::string MainStage::ApplyDamage_(PlayerID pid, int32_t damage)
{
    auto& player = players_[pid];
    std::string notify;
    if (damage > 0 && player.hp_ - damage <= 0) {
        for (const auto talent : player.talents_) {
            const std::string n = player.talent_states_.at(talent)->OnLethalDamage(player, damage, round_);
            if (!n.empty()) {
                if (!notify.empty()) notify += "\n";
                notify += n;
            }
            if (damage <= 0) break;
        }
    }
    player.hp_ -= damage;
    for (auto& [talent, state] : player.talent_states_) {
        state->OnDamageReceived(player, damage);
    }
    return notify;
}

inline void MainStage::ProcessBattle_(PlayerID pid1, PlayerID pid2, bool mirror, std::string& result,
                                     std::optional<int64_t> mirror_battle_score,
                                     std::optional<int32_t> mirror_base_score)
{
    // 坦诚相见: if either player has this talent, only compare base_score_ (ignores all extras)
    const bool sincere = players_[pid1].HasTalent(Talent::坦诚相见) ||
                         (!mirror && players_[pid2].HasTalent(Talent::坦诚相见));
    const int64_t score1 = sincere ? players_[pid1].base_score_ : PlayerBattleScore(pid1);
    // 镜像对战时使用 pid2 的战前快照分数，避免 pid2 在本轮其他对战中的得分变动影响镜像。
    const int64_t score2 = sincere
        ? (mirror && mirror_base_score.has_value() ? static_cast<int64_t>(*mirror_base_score) : players_[pid2].base_score_)
        : (mirror && mirror_battle_score.has_value() ? *mirror_battle_score : PlayerBattleScore(pid2));
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
        ApplyVictoryTalents_(pid1, mirror, score1, score2, result);

        // Apply defense talents for pid2 (injured party)
        int32_t defense = ApplyDefenseTalents_(pid2, total);
        total += defense;
        if (total < 0) total = 0;

        if (!mirror) {
            ApplyDefeatTalents_(pid2, mirror, total, score2, score1, result);
            const std::string lethal_notify = ApplyDamage_(pid2, total);
            if (!lethal_notify.empty()) {
                result += GetName(Global().PlayerName(pid2)) + " " + lethal_notify + "\n";
            }
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

            ApplyVictoryTalents_(pid2, mirror, score2, score1, result);
            ApplyDefeatTalents_(pid1, mirror, total, score1, score2, result);
            const std::string lethal_notify = ApplyDamage_(pid1, total);
            if (!lethal_notify.empty()) {
                result += GetName(Global().PlayerName(pid1)) + " " + lethal_notify + "\n";
            }
            p1_info = "(" + std::to_string(-total) + ")";
        } else {
            extra_damage = ApplyAttackTalents_(pid2, pid1, base_damage);
            int32_t total = base_damage + extra_damage;
            int32_t defense = ApplyDefenseTalents_(pid1, total);
            total += defense;
            if (total < 0) total = 0;

            ApplyDefeatTalents_(pid1, mirror, total, score1, score2, result);
            const std::string lethal_notify = ApplyDamage_(pid1, total);
            if (!lethal_notify.empty()) {
                result += GetName(Global().PlayerName(pid1)) + " " + lethal_notify + "\n";
            }
            p1_info = "(" + std::to_string(-total) + ")";
        }
    } else {
        if (!mirror) p2_info = "(0)";
    }

    // 通用 OnBattleEnd dispatch：对真实参与者触发（含平局；镜像只通知真实玩家一侧）。
    // outcome：从 pid1 视角的胜负标记，pid2 视角符号相反。
    ApplyBattleEndTalents_(pid1, mirror, score1, score2, score_cmp, result);
    if (!mirror) {
        ApplyBattleEndTalents_(pid2, mirror, score2, score1, -score_cmp, result);
    }

    result += name1 + p1_info + " vs " + name2 + p2_info + "\n";
}

// 对该玩家所有已获取天赋分发 OnBattleEnd。任何 hook 返回的文本会以 "玩家名 文本" 形式追加到 result。
inline void MainStage::ApplyBattleEndTalents_(PlayerID pid, bool mirror, int64_t my_score, int64_t opp_score,
                                              int32_t outcome, std::string& result)
{
    const TalentBattleEndContext context{mirror, HasValuableOne(special_event_), my_score, opp_score, outcome};
    auto& player = players_[pid];
    for (const auto talent : player.talents_) {
        const std::string notify = player.talent_states_.at(talent)->OnBattleEnd(player, context);
        if (!notify.empty()) {
            result += GetName(Global().PlayerName(pid)) + " " + notify + "\n";
        }
    }
}

// Returns extra damage from attacker's talents
inline int32_t MainStage::ApplyAttackTalents_(PlayerID attacker, PlayerID defender, int32_t damage)
{
    int32_t extra = 0;
    for (const auto talent : k_attack_order) {
        if (!players_[attacker].HasTalent(talent)) continue;
        auto effect = players_[attacker].talent_states_.at(talent)->AttackDamageDelta(players_[attacker], players_[defender], damage, battle_rng_);
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
    for (const auto talent : k_defense_order) {
        if (!players_[defender].HasTalent(talent)) continue;
        reduction += players_[defender].talent_states_.at(talent)->DefenseDamageDelta(players_[defender], damage);
    }
    return reduction;
}

inline void MainStage::ApplyDefeatTalents_(PlayerID loser, bool mirror, int32_t& damage,
                                            int64_t my_battle_score, int64_t opp_battle_score, std::string& result)
{
    // 顺序定义在 talent_order.h 的 k_defeat_order；
    // 律动残余 在最前：先把伤害封顶在 25，再交给后续 hook 进一步调整（如 事不过三 归零）。
    for (const auto talent : k_defeat_order) {
        if (!players_[loser].HasTalent(talent)) continue;
        const auto notify = players_[loser].talent_states_.at(talent)->OnDefeat(
            players_[loser], TalentDefeatContext{HasValuableOne(special_event_), mirror, my_battle_score, opp_battle_score}, damage);
        if (!notify.empty()) {
            result += GetName(Global().PlayerName(loser)) + " " + notify + "\n";
        }
    }
    players_[loser].UpdateZeroRiskMaxScore();
}

inline void MainStage::ApplyVictoryTalents_(PlayerID winner, bool mirror,
                                             int64_t my_battle_score, int64_t opp_battle_score, std::string& result)
{
    for (const auto talent : k_victory_order) {
        if (!players_[winner].HasTalent(talent)) continue;
        const auto notify = players_[winner].talent_states_.at(talent)->OnVictory(
            players_[winner], TalentVictoryContext{mirror, HasValuableOne(special_event_), my_battle_score, opp_battle_score});
        if (!notify.empty()) {
            result += GetName(Global().PlayerName(winner)) + " " + notify + "\n";
        }
    }
}

// Process deaths and eliminations after battle. 共用 sender，将淘汰播报与击杀触发型天赋（如劫掠）合并播报。
// 注：致死救援（绝地反击 等 OnLethalDamage）已在 ApplyDamage_ 内处理，到达这里的玩家是真正死亡的。
inline void MainStage::DoEliminationAfterBattle_(MsgSenderBase::MsgSenderGuard& sender)
{
    for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
        if (player_out_[pid] != 0 || players_[pid].hp_ > 0) continue;

        // Player dies in battle phase. phase=1 排名差于同回合中毒淘汰（phase=2）。
        alive_--;
        player_out_[pid] = round_;
        player_out_phase_[pid] = 1;
        sender << At(pid) << " 已被淘汰！\n";
        Global().Eliminate(pid);

        // 通用 dispatch：让 killer（同对对战中存活的对手）已获取的天赋有机会响应"击杀对手"。
        std::optional<PlayerID> killer = FindKillerFromFightMap_(pid);
        if (killer.has_value()) {
            auto& k_player = players_[*killer];
            for (const auto talent : k_player.talents_) {
                const std::string notify = k_player.talent_states_.at(talent)->OnDefeatedOpponent(k_player, players_[pid], random_card_rng_);
                if (!notify.empty()) {
                    sender << At(*killer) << " " << notify << "\n";
                }
            }
        }
    }
}

// 从当前对战 fight_map 中查找击杀 victim 的存活对手；若 victim 是镜像对战/无配对则返回空。
inline std::optional<PlayerID> MainStage::FindKillerFromFightMap_(PlayerID victim) const
{
    for (const auto& [p1, p2] : current_fight_map_) {
        if (PlayerID(p1) == victim && player_out_[PlayerID(p2)] == 0) {
            return PlayerID(p2);
        }
        if (PlayerID(p2) == victim && player_out_[PlayerID(p1)] == 0) {
            return PlayerID(p1);
        }
    }
    return std::nullopt;
}

inline void MainStage::DoPoison_()
{
    bool any_poisoned = false;
    std::string poison_msg;
    std::string lethal_msg;  // 中毒致死时 OnLethalDamage 的累积播报，在 sender 创建后一并输出
    for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
        if (player_out_[pid] != 0) continue;
        if (players_[pid].poison_layers_ > 0) {
            const int32_t original_damage = players_[pid].poison_layers_;
            int32_t damage = original_damage;
            const std::string lethal_notify = ApplyDamage_(pid, damage);
            poison_msg += GetName(Global().PlayerName(pid)) + "（" + std::to_string(-original_damage) + "）";
            // 百味草: gain 1 score per 1 HP lost to poison（用实际扣血量结算，被吸收时为 0）
            if (players_[pid].HasTalent(Talent::百味草) && damage > 0) {
                players_[pid].HerbalGrowth().poison_score += damage;
                players_[pid].UpdateScore(ScoreResult{players_[pid].comb_->BaseScore(), players_[pid].comb_->LineCount(), 0}, HasValuableOne(special_event_));
                poison_msg += "，触发「百味草」+" + std::to_string(damage) + "分";
            }
            poison_msg += "\n";
            if (!lethal_notify.empty()) {
                // 用 placeholder 暂存（带 pid 信息），等到 sender 创建后再 @-mention 输出。
                lethal_msg += GetName(Global().PlayerName(pid)) + " " + lethal_notify + "\n";
            }
            any_poisoned = true;
        }
    }
    if (!any_poisoned) return;

    // 中毒结算与中毒淘汰共用同一个 sender，合并为一条播报。
    auto sender = Global().Boardcast();
    sender << "中毒结算：\n" << poison_msg;
    if (!lethal_msg.empty()) sender << lethal_msg;
    for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
        if (player_out_[pid] != 0 || players_[pid].hp_ > 0) continue;
        // 中毒淘汰：phase=2 在同回合内排名优于对战阶段淘汰（phase=1），因结算时序更晚。
        alive_--;
        player_out_[pid] = round_;
        player_out_phase_[pid] = 2;
        sender << At(pid) << " 因中毒被淘汰！\n";
        Global().Eliminate(pid);
    }
}

// Collect pre-battle extra cards
inline void MainStage::CollectPreBattleExtras_()
{
    for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
        if (player_out_[pid] != 0) continue;
        auto& player = players_[pid];

        for (const auto talent : k_pre_battle_extra_order) {
            if (!player.HasTalent(talent)) continue;
            std::vector<AreaCard> offered_cards;
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
                // 3 选 1：生成 3 张备选随机砖块
                // 注：必须把三次 RandInt 拆成有序语句——C++ 函数实参之间是"不确定顺序"，
                // 直接写在 emplace_back 的实参里会让 GCC(libstdc++) 与 Clang(libc++) 以不同顺序消耗 random_card_rng_，造成跨平台 RNG 序列分叉。
                for (int i = 0; i < 3; ++i) {
                    const uint32_t left  = RandInt(random_card_rng_, 0, static_cast<uint32_t>(k_points[0].size() - 1));
                    const uint32_t mid   = RandInt(random_card_rng_, 0, static_cast<uint32_t>(pool_mid.size() - 1));
                    const uint32_t right = RandInt(random_card_rng_, 0, static_cast<uint32_t>(k_points[2].size() - 1));
                    offered_cards.emplace_back(k_points[0][left], pool_mid[mid], k_points[2][right]);
                }
            }
            const auto notify = player.talent_states_.at(talent)->OnPreBattleExtraCards(
                player, TalentPreBattleExtraContext{HasValuableOne(special_event_), special_event_, random_card_rng_,
                                                    offered_cards.empty() ? nullptr : &offered_cards});
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
        SeededShuffle(cards2_.begin(), cards2_.end(), pool2_rng_);
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
        SeededShuffle(cards2_.begin(), cards2_.end(), pool2_rng_);
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
    it2_ = cards2_.begin() + pos;

    // Shuffle the result so assignment order is random
    SeededShuffle(result.begin(), result.end(), pool2_rng_);
    return result;
}

// Draw next card from pool1 (should never be called when exhausted)
inline AreaCard MainStage::DrawFromPool1_()
{
    assert(it_ != cards_.end());
    return *(it_++);
}


