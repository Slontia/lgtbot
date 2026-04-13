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
    int32_t zero_risk_floor = player.ZeroRiskFloor();
    int32_t dyson_bonus = player.DysonSphereBonus();
    int32_t counterattack_extra = player.counterattack_extra_;
    int32_t temp = player.TempBattleScore();
    int32_t total = player.TotalScore() + temp;

    std::string score_line = std::to_string(total);
    std::string detail_line;

    bool has_detail = (perm != 0 || valuable != 0 || temp != 0 || zero_risk_floor != 0 || dyson_bonus != 0 || counterattack_extra != 0);
    if (has_detail) {
        detail_line += "（";
        detail_line += std::to_string(base);
        if (valuable != 0) {
            detail_line += "+" HTML_COLOR_FONT_HEADER(#1E3A8A) + std::to_string(valuable) + HTML_FONT_TAIL;
        }
        if (perm != 0) {
            detail_line += (perm > 0 ? "+" : "") + std::to_string(perm);
        }
        if (zero_risk_floor != 0) {
            detail_line += "+" HTML_COLOR_FONT_HEADER(#DAA520) "[保底+" + std::to_string(zero_risk_floor) + "]" HTML_FONT_TAIL;
        }
        if (dyson_bonus != 0) {
            detail_line += "+" HTML_COLOR_FONT_HEADER(#1E3A8A) "[戴森球+" + std::to_string(dyson_bonus) + "]" HTML_FONT_TAIL;
        }
        if (counterattack_extra != 0) {
            detail_line += "+" HTML_COLOR_FONT_HEADER(#4169E1) "[反击+" + std::to_string(counterattack_extra) + "]" HTML_FONT_TAIL;
        }
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

        if (talent == Talent::绝地反击) {
            // 绝地反击: blue if unused, show [+N] when active, grey if consumed
            if (player.counterattack_extra_ > 0) {
                talent_str += TalentName(talent);
                talent_str += HTML_COLOR_FONT_HEADER(#4169E1) "[+" + std::to_string(player.counterattack_extra_) + "]" HTML_FONT_TAIL;
            } else if (player.counterattack_used_) {
                talent_str += HTML_COLOR_FONT_HEADER(#999999) + std::string(TalentName(talent)) + HTML_FONT_TAIL;
            } else {
                talent_str += HTML_COLOR_FONT_HEADER(#4169E1) + std::string(TalentName(talent)) + HTML_FONT_TAIL;
            }
        } else if (talent == Talent::零风险投资) {
            // 零风险投资: show current floor bonus in gold when active
            talent_str += TalentName(talent);
            int32_t floor_bonus = player.ZeroRiskFloor();
            if (floor_bonus > 0) {
                talent_str += HTML_COLOR_FONT_HEADER(#DAA520) "[+" + std::to_string(floor_bonus) + "]" HTML_FONT_TAIL;
            }
        } else if (talent == Talent::特立独行) {
            // 特立独行: show +6 when score is odd
            talent_str += TalentName(talent);
            if (player.TotalScore() % 2 == 1) {
                talent_str += HTML_COLOR_FONT_HEADER(#FF6347) "[+6]" HTML_FONT_TAIL;
            }
        } else if (talent == Talent::成双成对) {
            // 成双成对: show +6 when score is even
            talent_str += TalentName(talent);
            if (player.TotalScore() % 2 == 0) {
                talent_str += HTML_COLOR_FONT_HEADER(#FF6347) "[+6]" HTML_FONT_TAIL;
            }
        } else if (talent == Talent::来点实在的) {
            // 来点实在的: always show green +4
            talent_str += TalentName(talent);
            talent_str += HTML_COLOR_FONT_HEADER(green) "(+4)" HTML_FONT_TAIL;
        } else if (talent == Talent::坦诚相见) {
            // 坦诚相见: dark red with [-N] indicating ignored extras
            talent_str += HTML_COLOR_FONT_HEADER(#D94A6A) + std::string(TalentName(talent)) + HTML_FONT_TAIL;
            int32_t ignored = player.TotalScore() + player.TempBattleScore() - player.base_score_;
            if (ignored > 0) {
                talent_str += HTML_COLOR_FONT_HEADER(#8B0000) "[-" + std::to_string(ignored) + "]" HTML_FONT_TAIL;
            }
        } else if (talent == Talent::冥想) {
            // 冥想: grey when deactivated (player has scored)
            if (player.meditation_active_) {
                talent_str += TalentName(talent);
            } else {
                talent_str += HTML_COLOR_FONT_HEADER(#999999) + std::string(TalentName(talent)) + HTML_FONT_TAIL;
            }
        } else if (talent == Talent::天使轮) {
            // 天使轮: show +15 when active, grey when deactivated
            if (player.angel_round_active_) {
                talent_str += TalentName(talent);
                talent_str += HTML_COLOR_FONT_HEADER(#FF6347) "[+15]" HTML_FONT_TAIL;
            } else {
                talent_str += HTML_COLOR_FONT_HEADER(#999999) + std::string(TalentName(talent)) + HTML_FONT_TAIL;
            }
        } else if (talent == Talent::有舍有得) {
            // 有舍有得: show progress
            talent_str += TalentName(talent);
            talent_str += HTML_COLOR_FONT_HEADER(#DAA520) "(" + std::to_string(player.loss_count_) + "/4)" HTML_FONT_TAIL;
        } else if (talent == Talent::三年之期) {
            // 三年之期: show progress when active, grey when completed
            if (player.three_year_active_) {
                int32_t stored = static_cast<int32_t>(player.three_year_cards_.size());
                talent_str += TalentName(talent);
                talent_str += HTML_COLOR_FONT_HEADER(#DAA520) "(" + std::to_string(stored) + "/3)" HTML_FONT_TAIL;
            } else {
                talent_str += HTML_COLOR_FONT_HEADER(#999999) + std::string(TalentName(talent)) + HTML_FONT_TAIL;
            }
        } else if (talent == Talent::锻造) {
            // 锻造: show forge fragments
            talent_str += TalentName(talent);
            talent_str += HTML_COLOR_FONT_HEADER(#DAA520) "(" + ForgeFragmentStr_(player) + ")" HTML_FONT_TAIL;
        } else if (talent == Talent::三相之力) {
            // 三相之力: show progress, grey when completed
            if (player.tri_force_progress_ >= 3) {
                talent_str += HTML_COLOR_FONT_HEADER(#999999) + std::string(TalentName(talent)) + HTML_FONT_TAIL;
            } else {
                talent_str += TalentName(talent);
                talent_str += HTML_COLOR_FONT_HEADER(#DAA520) "(" + std::to_string(player.tri_force_progress_) + "/3)" HTML_FONT_TAIL;
            }
        } else if (talent == Talent::事不过三) {
            // 事不过三: show progress (at 3/3 next defeat is immune)
            talent_str += TalentName(talent);
            if (player.defeat_count_ >= 3) {
                talent_str += HTML_COLOR_FONT_HEADER(#FF6347) "(3/3✦)" HTML_FONT_TAIL;
            } else {
                talent_str += HTML_COLOR_FONT_HEADER(#DAA520) "(" + std::to_string(player.defeat_count_) + "/3)" HTML_FONT_TAIL;
            }
        } else if (talent == Talent::紧急救援) {
            // 紧急救援: blue if unused, grey if used
            if (player.emergency_rescue_used_) {
                talent_str += HTML_COLOR_FONT_HEADER(#999999) + std::string(TalentName(talent)) + HTML_FONT_TAIL;
            } else {
                talent_str += HTML_COLOR_FONT_HEADER(#4169E1) + std::string(TalentName(talent)) + HTML_FONT_TAIL;
            }
        } else if (talent == Talent::我全都要) {
            // 我全都要: always blue (will be removed when triggered)
            talent_str += HTML_COLOR_FONT_HEADER(#4169E1) + std::string(TalentName(talent)) + HTML_FONT_TAIL;
        } else if (talent == Talent::利滚利) {
            // 利滚利: show accumulated interest
            talent_str += TalentName(talent);
            if (player.compound_interest_accumulated_ > 0) {
                talent_str += HTML_COLOR_FONT_HEADER(#DAA520) "(+" + std::to_string(player.compound_interest_accumulated_) + ")" HTML_FONT_TAIL;
            }
        } else if (talent == Talent::戴森球) {
            // 戴森球: show progress of length-3 lines
            talent_str += TalentName(talent);
            int32_t count = player.CountCompletedLength3Lines_();
            if (count >= 6) {
                talent_str += HTML_COLOR_FONT_HEADER(#FF6347) "(6/6✦)" HTML_FONT_TAIL;
            } else {
                talent_str += HTML_COLOR_FONT_HEADER(#DAA520) "(" + std::to_string(count) + "/6)" HTML_FONT_TAIL;
            }
        } else if (talent == Talent::败者之刃) {
            // 败者之刃: show accumulated temp score
            talent_str += TalentName(talent);
            if (player.loser_blade_temp_ > 0) {
                talent_str += HTML_COLOR_FONT_HEADER(#FF6347) "(+" + std::to_string(player.loser_blade_temp_) + ")" HTML_FONT_TAIL;
            }
        } else if (talent == Talent::临时用品) {
            // 临时用品: show remaining rounds or grey if expired
            if (player.temp_wild_pos_ > 0) {
                talent_str += TalentName(talent);
                talent_str += HTML_COLOR_FONT_HEADER(#DAA520) "(" + std::to_string(player.temp_wild_rounds_) + "回合)" HTML_FONT_TAIL;
            } else {
                talent_str += HTML_COLOR_FONT_HEADER(#999999) + std::string(TalentName(talent)) + HTML_FONT_TAIL;
            }
        } else if (talent == Talent::垃圾回收) {
            // 垃圾回收: show accumulated score
            talent_str += TalentName(talent);
            int32_t bonus = player.discard_count_ * 2;
            if (bonus > 0) {
                talent_str += HTML_COLOR_FONT_HEADER(#DAA520) "(+" + std::to_string(bonus) + ")" HTML_FONT_TAIL;
            }
        } else if (talent == Talent::还是有用的) {
            // 还是有用的: show current bonus
            talent_str += TalentName(talent);
            int32_t bonus = player.StillUsefulBonus();
            if (bonus > 0) {
                talent_str += HTML_COLOR_FONT_HEADER(#DAA520) "(+" + std::to_string(bonus) + ")" HTML_FONT_TAIL;
            }
        } else if (talent == Talent::光波干涉) {
            // 光波干涉：显示当前额外分数值（同数同长度连线≥2时累计 15%）
            talent_str += TalentName(talent);
            int32_t bonus = player.LightInterferenceBonus();
            if (bonus > 0) {
                talent_str += HTML_COLOR_FONT_HEADER(#DAA520) "(+" + std::to_string(bonus) + ")" HTML_FONT_TAIL;
            }
        } else if (talent == Talent::局部强化) {
            // 局部强化: show current bonus from triggered directions
            talent_str += TalentName(talent);
            int32_t bonus = 0;
            for (int d = 0; d < 3; ++d) {
                if (player.local_enhance_triggered_[d]) bonus += 3;
            }
            if (bonus > 0) {
                talent_str += HTML_COLOR_FONT_HEADER(#DAA520) "(+" + std::to_string(bonus) + ")" HTML_FONT_TAIL;
            }
        } else if (talent == Talent::零号位) {
            // 0号位: show current round temp score
            talent_str += TalentName(talent);
            if (player.discard_scorer_temp_ > 0) {
                talent_str += HTML_COLOR_FONT_HEADER(#FF6347) "(+" + std::to_string(player.discard_scorer_temp_) + ")" HTML_FONT_TAIL;
            }
        } else if (talent == Talent::百味草) {
            // 百味草: show poison layers and accumulated score
            talent_str += TalentName(talent);
            talent_str += HTML_COLOR_FONT_HEADER(#6A0DAD) "(" + std::to_string(player.poison_layers_) + "毒" HTML_FONT_TAIL;
            if (player.poison_score_ > 0) {
                talent_str += HTML_COLOR_FONT_HEADER(#6A0DAD) "+" + std::to_string(player.poison_score_) + "分)" HTML_FONT_TAIL;
            } else {
                talent_str += HTML_COLOR_FONT_HEADER(#6A0DAD) ")" HTML_FONT_TAIL;
            }
        } else {
            talent_str += TalentName(talent);
        }
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

// Format forge fragments as "3--" / "3X-" / "3X2"
inline std::string MainStage::ForgeFragmentStr_(const Player& player)
{
    std::string s;
    for (size_t i = 0; i < 3; ++i) {
        if (i < player.forge_fragments_.size()) {
            int32_t val = player.forge_fragments_[i];
            s += (val == 10) ? "X" : std::to_string(val);
        } else {
            s += "-";
        }
    }
    return s;
}

// ===== Talent Effects / Settlement =====

// Called after a card is placed to update score and check talents.
// Returns {notification string, base score delta (board-only change)}.
inline std::pair<std::string, int32_t> MainStage::OnCardPlaced_(PlayerID pid, uint32_t idx, const ScoreResult& result)
{
    auto& player = players_[pid];
    std::string notify;

    // 星河流转: 放置到 2/4/7/13/16/18 号位时，立即为该砖块对应方向添加单线癞子，
    // 并使用转换后的盘面重新计算得分，使"收获/损失分数"按修改后的砖块显示。
    // （与「完美块」一样在放置阶段触发；避免出现"先扣分再加回"的错位提示。）
    ScoreResult effective_result = result;
    if (player.HasTalent(Talent::星河流转) && idx != 0) {
        auto r = player.comb_->ApplyGalaxyFlowAt(idx);
        if (r.has_value()) {
            const auto& [dir, new_result] = *r;
            // 合并两段 delta：Fill 带来的 result.score_delta + 星河流转转换后的 new_result.score_delta
            effective_result.base_score = new_result.base_score;
            effective_result.line_count = new_result.line_count;
            effective_result.score_delta = result.score_delta + new_result.score_delta;
            static const char* k_dir_name[3] = {"左上", "垂直", "右上"};
            notify += std::string("\n触发天赋「星河流转」，") + k_dir_name[dir] + "方向获得单线癞子";
        }
    }

    // 张三来袭：已放置砖块中每出现一个数字 3，回复 3 点生命（各方向独立计数；
    // 由于在 TransformCardForPlacement_ 中 4→3/7→6 等已完成，
    // 配合「以退为进」等转换后的 3 同样触发；星河流转可能把方向变成 10（癞子），不计入。）
    if (player.HasTalent(Talent::张三来袭) && idx != 0) {
        const auto& placed = player.comb_->GetCard(idx);
        if (placed.has_value()) {
            int32_t threes = 0;
            for (uint32_t d = 0; d < k_direct_max; ++d) {
                if (placed->PointAt(d) == 3) ++threes;
            }
            if (threes > 0) {
                int32_t heal = threes * 3;
                player.hp_ += heal;
                notify += "\n触发天赋「张三来袭」，回复 " + std::to_string(heal) + " 点生命";
            }
        }
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

    // Permanent extra changes + Dyson sphere bonus change: combined as talent effect notification
    {
        int32_t perm_delta = player.permanent_extra_ - old_perm;
        int32_t dyson_delta = 0;
        bool dyson_first_activate = false;
        if (player.HasTalent(Talent::戴森球) && !player.dyson_sphere_activated_) {
            if (player.CountCompletedLength3Lines_() >= 6) {
                player.dyson_sphere_activated_ = true;
                dyson_first_activate = true;
                dyson_delta = player.DysonSphereBonus();
            }
        }
        int32_t total_extra = perm_delta + dyson_delta;
        if (total_extra > 0) {
            notify += "\n触发天赋效果，额外获得 " + std::to_string(total_extra) + " 点积分";
        } else if (total_extra < 0) {
            notify += "\n天赋效果变动，减少 " + std::to_string(-total_extra) + " 点额外积分";
        }
        if (dyson_first_activate) {
            notify += "\n达成「戴森球」条件！获得 6% 额外分数";
        }
    }

    // 冥想: 单次连线获得 ≥ 25 分（本次放置带来的基础连线分增量）时停用。
    // 天赋额外分不计入；只看 effective_result.score_delta 这一瞬间的值，不做累计。
    if (player.HasTalent(Talent::冥想) && player.meditation_active_) {
        if (effective_result.score_delta >= 25) {
            player.meditation_active_ = false;
            notify += "\n天赋「冥想」已停止！（本次连线获得 "
                   + std::to_string(effective_result.score_delta) + " 分，达到 25 分条件）";
        }
    }

    // 天使轮: deactivate when a line is completed (board score increases)
    if (player.HasTalent(Talent::天使轮) && player.angel_round_active_ && effective_result.score_delta > 0) {
        player.angel_round_active_ = false;
        notify += "\n天赋「天使轮」临时分已消失！（完成连线）";
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

    // 三相之力: apply directional wild for next 3 normal-round cards
    if (is_normal_round && player.HasTalent(Talent::三相之力) && player.tri_force_progress_ < 3) {
        int32_t dir = player.tri_force_progress_;
        int32_t a = actual.PointAt(0), b = actual.PointAt(1), c = actual.PointAt(2);
        bool already_wild = false;
        switch (dir) {
            case 0: already_wild = (a == 10); a = 10; break;
            case 1: already_wild = (b == 10); b = 10; break;
            case 2: already_wild = (c == 10); c = 10; break;
        }
        actual = AreaCard(a, b, c);
        player.tri_force_progress_++;
        static const char* dir_names[] = {"左", "中", "右"};
        if (!already_wild) {
            notify += "\n触发天赋「三相之力」，" + std::string(dir_names[dir]) + "方向获得单线癞子（" + std::to_string(player.tri_force_progress_) + "/3）";
        } else {
            notify += "\n触发天赋「三相之力」进度（" + std::to_string(player.tri_force_progress_) + "/3）";
        }
    }

    if (actual.IsWild()) return {actual, notify};

    // 两级反转: 1↔9 swap (skip wild directions = 10)
    if (player.HasTalent(Talent::两级反转)) {
        int32_t a = actual.PointAt(0), b = actual.PointAt(1), c = actual.PointAt(2);
        bool changed = false;
        auto swap19 = [&](int32_t& v) {
            if (v != 10) {
                if (v == 1) { v = 9; changed = true; }
                else if (v == 9) { v = 1; changed = true; }
            }
        };
        swap19(a); swap19(b); swap19(c);
        if (changed) actual = AreaCard(a, b, c);
    }

    // 以退为进: 7→6, 4→3 (skip wild directions = 10)
    if (player.HasTalent(Talent::以退为进)) {
        int32_t a = actual.PointAt(0), b = actual.PointAt(1), c = actual.PointAt(2);
        bool changed = false;
        if (a != 10 && a == 4) { a = 3; changed = true; } if (a != 10 && a == 7) { a = 6; changed = true; }
        if (b != 10 && b == 4) { b = 3; changed = true; } if (b != 10 && b == 7) { b = 6; changed = true; }
        if (c != 10 && c == 4) { c = 3; changed = true; } if (c != 10 && c == 7) { c = 6; changed = true; }
        if (changed) actual = AreaCard(a, b, c);
    }

    // 完美块: 312→wild (directions already wild (10) count as matching)
    if (player.HasTalent(Talent::完美块)) {
        int32_t a = actual.PointAt(0), b = actual.PointAt(1), c = actual.PointAt(2);
        if ((a == 3 || a == 10) && (b == 1 || b == 10) && (c == 2 || c == 10)) {
            actual = AreaCard(); // full wild
            notify += "\n触发天赋「完美块」，该砖块视为万能牌！";
        }
    }

    return {actual, notify};
}

// Handle discard (position 0) talent effects. Returns notification string.
inline std::string MainStage::HandleDiscard_(PlayerID pid, const AreaCard& card)
{
    auto& player = players_[pid];
    std::string notify;

    // 垃圾回收: +2 score per discard
    if (player.HasTalent(Talent::垃圾回收)) {
        player.discard_count_++;
        player.UpdateScore(ScoreResult{player.comb_->BaseScore(), player.comb_->LineCount(), 0}, HasValuableOne(special_event_));
        notify += "\n触发天赋「垃圾回收」，获得 2 点积分";
    }

    // 0号位: gain the card's full point sum as temporary score this round
    // （本回合有效，可叠加；由“一半”平衡调整为“全部”。）
    if (player.HasTalent(Talent::零号位)) {
        int32_t bonus = card.PointSum();
        if (bonus > 0) {
            player.discard_scorer_temp_ += bonus;
            notify += "\n触发天赋「0号位」，获得 " + std::to_string(bonus) + " 点临时分";
        }
    }

    // 锻造: collect fragments from discarded card (wild cards give wild fragment = 10)
    if (player.HasTalent(Talent::锻造)) {
        size_t frag_count = player.forge_fragments_.size();
        if (frag_count < 3) {
            int32_t frag_val = card.PointAt(frag_count);  // 10 for wild directions
            player.forge_fragments_.push_back(frag_val);
            std::string frag_display = (frag_val == 10) ? "X" : std::to_string(frag_val);
            notify += "\n触发天赋「锻造」，获得碎片 " + frag_display;
            notify += "，当前碎片：" + ForgeFragmentStr_(player);
        }
    }

    return notify;
}

// Apply "三年之期" storage for a player this round
// Returns true if the card was stored (player skips placement)
inline bool MainStage::HandleThreeYearStore_(PlayerID pid, const AreaCard& card)
{
    auto& player = players_[pid];
    if (!player.HasTalent(Talent::三年之期)) return false;
    if (!player.three_year_active_) return false;
    if (player.three_year_rounds_left_ <= 0) return false;

    player.three_year_cards_.push_back(card);
    player.three_year_rounds_left_--;
    if (player.three_year_rounds_left_ == 0) {
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
    //   本轮触发的 counterattack_extra_ 要保留到下一次真实对战结束后清除；
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
    if (players_[pid].three_year_active_) {
        players_[pid].three_year_damage_stored_ += damage;
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

    int32_t base_damage = static_cast<int32_t>((score1 - score2) * dRate_);
    int32_t extra_damage = 0;

    std::string p1_info, p2_info;
    if (mirror) p2_info = "(镜像)";

    if (base_damage > 0) {
        // pid1 wins, pid2 loses
        if (!mirror) players_[pid2].never_lost_ = false;
        extra_damage = ApplyAttackTalents_(pid1, pid2, base_damage);
        int32_t total = base_damage + extra_damage;

        // Apply defense talents for pid2 (injured party)
        int32_t defense = ApplyDefenseTalents_(pid2, total);
        total += defense;
        if (total < 0) total = 0;

        if (!mirror) {
            // 事不过三: check immune on 4th defeat
            if (total > 0 && players_[pid2].HasTalent(Talent::事不过三)) {
                if (players_[pid2].defeat_count_ >= 3) {
                    players_[pid2].defeat_count_ = 0;
                    total = 0;
                    result += GetName(Global().PlayerName(pid2)) + " 触发天赋「事不过三」，免疫此次伤害！\n";
                } else {
                    players_[pid2].defeat_count_++;
                }
            }
            // 有舍有得: count loss
            if (players_[pid2].HasTalent(Talent::有舍有得)) {
                players_[pid2].loss_count_++;
            }
            ApplyDamage_(pid2, total);
            p2_info = "(" + std::to_string(-total) + ")";
        }
    } else if (base_damage < 0) {
        base_damage = -base_damage; // make positive for talent processing
        // pid2 wins, pid1 loses
        players_[pid1].never_lost_ = false;
        if (!mirror) {
            extra_damage = ApplyAttackTalents_(pid2, pid1, base_damage);
            int32_t total = base_damage + extra_damage;

            int32_t defense = ApplyDefenseTalents_(pid1, total);
            total += defense;
            if (total < 0) total = 0;

            // 事不过三: check immune on 4th defeat
            if (total > 0 && players_[pid1].HasTalent(Talent::事不过三)) {
                if (players_[pid1].defeat_count_ >= 3) {
                    players_[pid1].defeat_count_ = 0;
                    total = 0;
                    result += GetName(Global().PlayerName(pid1)) + " 触发天赋「事不过三」，免疫此次伤害！\n";
                } else {
                    players_[pid1].defeat_count_++;
                }
            }

            // 有舍有得: count loss
            if (players_[pid1].HasTalent(Talent::有舍有得)) {
                players_[pid1].loss_count_++;
            }
            ApplyDamage_(pid1, total);
            p1_info = "(" + std::to_string(-total) + ")";
        } else {
            // Mirror: pid2 wins but player doesn't take damage if turing_test
            if (players_[pid1].HasTalent(Talent::图灵测试)) {
                // Turing test: no damage from mirror
            } else {
                extra_damage = ApplyAttackTalents_(pid2, pid1, base_damage);
                int32_t total = base_damage + extra_damage;
                int32_t defense = ApplyDefenseTalents_(pid1, total);
                total += defense;
                if (total < 0) total = 0;

                // 事不过三: check immune on 4th defeat (mirror loss counts too)
                if (total > 0 && players_[pid1].HasTalent(Talent::事不过三)) {
                    if (players_[pid1].defeat_count_ >= 3) {
                        players_[pid1].defeat_count_ = 0;
                        total = 0;
                        result += GetName(Global().PlayerName(pid1)) + " 触发天赋「事不过三」，免疫此次伤害！\n";
                    } else {
                        players_[pid1].defeat_count_++;
                    }
                }

                // 有舍有得: solo player lost to mirror, count it
                if (players_[pid1].HasTalent(Talent::有舍有得)) {
                    players_[pid1].loss_count_++;
                }
                ApplyDamage_(pid1, total);
                p1_info = "(" + std::to_string(-total) + ")";
            }
        }
    } else {
        if (!mirror) p2_info = "(0)";
    }

    // 败者之刃: 胜利后立即清除临时分，战败后累计+4（在本回合伤害结算之后处理）
    if (base_damage != 0 && !mirror) {
        // Determine winner and loser (base_damage was already made positive for pid2-wins case)
        PlayerID winner = (score1 > score2) ? pid1 : pid2;
        PlayerID loser  = (score1 > score2) ? pid2 : pid1;
        if (players_[winner].HasTalent(Talent::败者之刃)) {
            players_[winner].loser_blade_temp_ = 0;
        }
        if (players_[loser].HasTalent(Talent::败者之刃)) {
            players_[loser].loser_blade_temp_ += 4;
        }
    } else if (base_damage != 0 && mirror) {
        // Mirror: pid1 is the solo player, pid2 is mirror copy
        // If pid1 lost to mirror, accumulate (don't clear for mirror wins)
        if (score1 < score2 && players_[pid1].HasTalent(Talent::败者之刃)) {
            players_[pid1].loser_blade_temp_ += 4;
        }
    }

    result += name1 + p1_info + " vs " + name2 + p2_info + "\n";
}

// Returns extra damage from attacker's talents
inline int32_t MainStage::ApplyAttackTalents_(PlayerID attacker, PlayerID defender, int32_t damage)
{
    int32_t extra = 0;

    // 嗜血: +2 HP on win
    if (players_[attacker].HasTalent(Talent::嗜血)) {
        players_[attacker].hp_ += 2;
    }

    // 快攻: +6 damage on win
    if (players_[attacker].HasTalent(Talent::快攻)) {
        extra += 6;
    }

    // 致命魔术: 15% chance for 100% extra damage (ceil)
    if (players_[attacker].HasTalent(Talent::致命魔术)) {
        if (RandInt(g_, 1, 100) <= 15) {
            int32_t magic_damage = static_cast<int32_t>(std::ceil(damage * 1.0));
            extra += magic_damage;
            Global().Boardcast() << At(attacker) << " 触发天赋「致命魔术」，额外造成 " << magic_damage << " 点伤害！";
        }
    }

    // 攻击形态: +15% damage dealt (ceil)
    if (players_[attacker].HasTalent(Talent::攻击形态)) {
        extra += static_cast<int32_t>(std::ceil(damage * 0.15));
    }

    // 防御形态: -5% damage dealt (ceil)
    if (players_[attacker].HasTalent(Talent::防御形态)) {
        extra -= static_cast<int32_t>(std::ceil(damage * 0.05));
    }

    return extra;
}

// Returns damage reduction (negative = less damage taken)
inline int32_t MainStage::ApplyDefenseTalents_(PlayerID defender, int32_t damage)
{
    int32_t reduction = 0;

    // 钢铁之躯: -30% damage taken (ceil)
    if (players_[defender].HasTalent(Talent::钢铁之躯)) {
        reduction -= static_cast<int32_t>(std::ceil(damage * 0.3));
    }

    // 防御形态: -15% damage taken (ceil)
    if (players_[defender].HasTalent(Talent::防御形态)) {
        reduction -= static_cast<int32_t>(std::ceil(damage * 0.15));
    }

    // 攻击形态: +5% damage taken (ceil)
    if (players_[defender].HasTalent(Talent::攻击形态)) {
        reduction += static_cast<int32_t>(std::ceil(damage * 0.05));
    }

    return reduction;
}

// Process deaths and eliminations after battle
inline void MainStage::DoEliminationAfterBattle_()
{
    for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
        if (player_out_[pid] != 0 || players_[pid].hp_ > 0) continue;

        // Check 绝地反击
        if (players_[pid].HasTalent(Talent::绝地反击) && !players_[pid].counterattack_used_) {
            players_[pid].hp_ = 1;
            players_[pid].counterattack_triggered_ = true;
            players_[pid].counterattack_used_ = true;
            players_[pid].counterattack_extra_ = static_cast<int32_t>(std::ceil(players_[pid].TotalScore() * 0.15));
            players_[pid].counterattack_trigger_round_ = round_;
            Global().Boardcast() << At(pid) << " 触发天赋「绝地反击」，血量降为1，获得 "
                                 << players_[pid].counterattack_extra_ << " 点反击加成！";
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
                players_[pid].poison_score_ += damage;
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
            if (players_[pid].HasTalent(Talent::绝地反击) && !players_[pid].counterattack_used_) {
                players_[pid].hp_ = 1;
                players_[pid].counterattack_triggered_ = true;
                players_[pid].counterattack_used_ = true;
                players_[pid].counterattack_extra_ = static_cast<int32_t>(std::ceil(players_[pid].TotalScore() * 0.15));
                players_[pid].counterattack_trigger_round_ = round_;
                Global().Boardcast() << At(pid) << " 触发天赋「绝地反击」，获得 "
                                     << players_[pid].counterattack_extra_ << " 点反击加成！";
                continue;
            }
            alive_--;
            player_out_[pid] = round_;
            Global().Boardcast() << At(pid) << " 因中毒被淘汰！";
            Global().Eliminate(pid);
        }
    }
}

// Collect pre-battle extra cards (锻造, 有舍有得)
inline void MainStage::CollectPreBattleExtras_()
{
    for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
        if (player_out_[pid] != 0) continue;
        auto& player = players_[pid];

        // 锻造: if 3 fragments collected, create a card
        if (player.HasTalent(Talent::锻造) && player.forge_fragments_.size() >= 3) {
            AreaCard forged(player.forge_fragments_[0], player.forge_fragments_[1], player.forge_fragments_[2]);
            player.extra_card_queue_.push_back({{forged}, "锻造"});
            player.forge_fragments_.clear();
        }

        // 有舍有得: every 4 losses, get a random card (respecting current special event; never wild)
        if (player.HasTalent(Talent::有舍有得) && player.loss_count_ >= 4) {
            // Filter the middle-direction pool based on special event: NO_SMALL removes 1, NO_MIDDLE removes 5, NO_BIG removes 9
            std::vector<uint32_t> pool_mid;
            for (uint32_t v : k_points[1]) {
                if (special_event_ == SpecialEvent::大的要来了 && v == 1) continue;
                if (special_event_ == SpecialEvent::两极分化 && v == 5) continue;
                if (special_event_ == SpecialEvent::大的没了 && v == 9) continue;
                pool_mid.push_back(v);
            }
            if (pool_mid.empty()) {
                // Fallback (shouldn't happen because each event removes only one value)
                pool_mid.assign(k_points[1].begin(), k_points[1].end());
            }
            AreaCard card(
                k_points[0][RandInt(g_, 0, static_cast<uint32_t>(k_points[0].size() - 1))],
                pool_mid[RandInt(g_, 0, static_cast<uint32_t>(pool_mid.size() - 1))],
                k_points[2][RandInt(g_, 0, static_cast<uint32_t>(k_points[2].size() - 1))]);
            player.extra_card_queue_.push_back({{card}, "有舍有得"});
            player.loss_count_ -= 4;
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
        if (player.HasTalent(Talent::三年之期) && player.three_year_active_ && player.three_year_rounds_left_ == 0 && !player.three_year_cards_.empty()) {
            // Push as a single multi-card entry so player can see all and pick one at a time
            player.extra_card_queue_.push_back({player.three_year_cards_, "三年之期", true});
            // Restore HP accumulated during storage period
            if (player.three_year_damage_stored_ > 0) {
                player.hp_ += player.three_year_damage_stored_;
                Global().Boardcast() << At(pid) << " 触发天赋「三年之期」，恢复存储期间受到的 "
                                     << player.three_year_damage_stored_ << " 点伤害！";
                player.three_year_damage_stored_ = 0;
            }
            player.three_year_cards_.clear();
            player.three_year_active_ = false;
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
