// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).
//
// This file was generated with the assistance of Claude Code (claude.ai/code).

#include "game_framework/unittest_base.h"

namespace lgtbot {
namespace game {
namespace GAME_MODULE_NAME {

// 注：MainStage::PlayerScore 现在按"游戏是否结束"切换：
//   - 终局后：按淘汰名次结算的 game_score（提交给框架作入库分）
//   - 终局前：盘面+天赋原分（用于此文件历史 ASSERT_SCORE 断言）
// 因此本文件内 ASSERT_SCORE / ASSERT_FINAL_SCORE 走默认的 AssertScoresImpl_ 即可，
// 在 AUTO_PLAY_TEST 等"未到游戏结束就断言"的场景下，PlayerScore 会自动回退到盘面分。
#undef ASSERT_SCORE
#define ASSERT_SCORE(...) \
    do { \
        this->AssertScoresImpl_({__VA_ARGS__}); \
    } while (0)

// Backward-compatible alias used by older tests; still asserts raw board/talent score.
#define ASSERT_FINAL_SCORE(...) \
    do { \
        this->AssertScoresImpl_({__VA_ARGS__}); \
    } while (0)

// Auto-play variant for cases whose expected raw score differs from the generic helper.
#define AUTO_PLAY_TEST_FINAL(test_name, talent_str, final_p0, final_p1) \
GAME_TEST(2, test_name) { \
    ASSERT_PUB_MSG(OK, 0, "种子 test"); \
    ASSERT_PUB_MSG(OK, 0, "事件 无"); \
    ASSERT_PUB_MSG(OK, 0, "血量 500"); \
    ASSERT_PUB_MSG(OK, 0, "天赋 " talent_str); \
    ASSERT_TRUE(StartGame()); \
    for (int i = 0; i < 200 && !this->main_stage_->IsOver(); ++i) this->TimeoutRequest_(); \
    ASSERT_FINAL_SCORE(final_p0, final_p1); \
}

// ============================================================================
// Helper macros for common test patterns
// ============================================================================

// Auto-play test: both players timeout every round, HP=500 to prevent early elimination
#define AUTO_PLAY_TEST(test_name, talent_str, expected_p0, expected_p1) \
GAME_TEST(2, test_name) { \
    ASSERT_PUB_MSG(OK, 0, "种子 test"); \
    ASSERT_PUB_MSG(OK, 0, "事件 无"); \
    ASSERT_PUB_MSG(OK, 0, "血量 500"); \
    ASSERT_PUB_MSG(OK, 0, "天赋 " talent_str); \
    ASSERT_TRUE(StartGame()); \
    for (int i = 0; i < 200 && !this->main_stage_->IsOver(); ++i) this->TimeoutRequest_(); \
    ASSERT_SCORE(expected_p0, expected_p1); \
}

// Auto-play test with no talent
#define AUTO_PLAY_TEST_NO_TALENT(test_name, expected_p0, expected_p1) \
GAME_TEST(2, test_name) { \
    ASSERT_PUB_MSG(OK, 0, "种子 test"); \
    ASSERT_PUB_MSG(OK, 0, "事件 无"); \
    ASSERT_PUB_MSG(OK, 0, "血量 500"); \
    ASSERT_TRUE(StartGame()); \
    for (int i = 0; i < 200 && !this->main_stage_->IsOver(); ++i) this->TimeoutRequest_(); \
    ASSERT_SCORE(expected_p0, expected_p1); \
}

// ============================================================================
// 1. 常规测试 (baseline / 策略性放置 / 多人崩溃测试)
// ============================================================================

// ---- 1a. Baseline: 无天赋 auto-play ----
AUTO_PLAY_TEST_NO_TALENT(baseline_auto_play, 6, 15)

// ---- 1b. 策略性放置：P0 主动放牌，P1 全弃牌 ----
// P0 actively places cards to form VERT line {1,2,3}.
// Card sequence (seed "test", event 無):
//   R1=357(wild), R2=357, R3=812, R4=316, R5=897, R6=852, R7=317
// P0 places: R1→19, R2→18, R3→1, R4→2, R5→17, R6→16, R7→3
// This forms VERT{1,2,3} with values from dir1: 5,1,1 → all 1s = line score 3.
// P1 discards all cards (弃牌) for rounds 1-7.

// Strategic: P0 forms a line, no talents
GAME_TEST(2, strategic_vert_line) {
    ASSERT_PUB_MSG(OK, 0, "种子 test");
    ASSERT_PUB_MSG(OK, 0, "事件 无");
    ASSERT_PUB_MSG(OK, 0, "血量 500");
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(OK, 0, "19");  ASSERT_PUB_MSG(CHECKOUT, 1, "0");
    ASSERT_PUB_MSG(OK, 0, "18");  ASSERT_PUB_MSG(CHECKOUT, 1, "0");
    ASSERT_PUB_MSG(OK, 0, "1");   ASSERT_PUB_MSG(CHECKOUT, 1, "0");
    ASSERT_PUB_MSG(OK, 0, "2");   ASSERT_PUB_MSG(CHECKOUT, 1, "0");
    ASSERT_PUB_MSG(OK, 0, "17");  ASSERT_PUB_MSG(CHECKOUT, 1, "0");
    ASSERT_PUB_MSG(OK, 0, "16");  ASSERT_PUB_MSG(CHECKOUT, 1, "0");
    ASSERT_PUB_MSG(OK, 0, "3");   ASSERT_PUB_MSG(CHECKOUT, 1, "0");
    for (int i = 0; i < 200 && !this->main_stage_->IsOver(); ++i) this->TimeoutRequest_();
    ASSERT_FINAL_SCORE(0, 0);
}

// Strategic with 以退为进: transforms card values, creating different line compositions.
// P1's auto-played cards benefit from transforms; the later A-tier lottery can pick an active
// talent that is passed on timeout, so the final score follows the current random pool.
GAME_TEST(2, strategic_retreat_advance) {
    ASSERT_PUB_MSG(OK, 0, "种子 test");
    ASSERT_PUB_MSG(OK, 0, "事件 无");
    ASSERT_PUB_MSG(OK, 0, "血量 500");
    ASSERT_PUB_MSG(OK, 0, "天赋 以退为进");
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(OK, 0, "19");  ASSERT_PUB_MSG(CHECKOUT, 1, "0");
    ASSERT_PUB_MSG(OK, 0, "18");  ASSERT_PUB_MSG(CHECKOUT, 1, "0");
    ASSERT_PUB_MSG(OK, 0, "1");   ASSERT_PUB_MSG(CHECKOUT, 1, "0");
    ASSERT_PUB_MSG(OK, 0, "2");   ASSERT_PUB_MSG(CHECKOUT, 1, "0");
    ASSERT_PUB_MSG(OK, 0, "17");  ASSERT_PUB_MSG(CHECKOUT, 1, "0");
    ASSERT_PUB_MSG(OK, 0, "16");  ASSERT_PUB_MSG(CHECKOUT, 1, "0");
    ASSERT_PUB_MSG(OK, 0, "3");   ASSERT_PUB_MSG(CHECKOUT, 1, "0");
    for (int i = 0; i < 200 && !this->main_stage_->IsOver(); ++i) this->TimeoutRequest_();
    ASSERT_FINAL_SCORE(0, 45);
}

// Strategic with 垃圾回收: P1 discards 7 cards → 14 bonus points from recycle.
// P0 places cards so fewer discards → less recycle bonus.
GAME_TEST(2, strategic_trash_recycle) {
    ASSERT_PUB_MSG(OK, 0, "种子 test");
    ASSERT_PUB_MSG(OK, 0, "事件 无");
    ASSERT_PUB_MSG(OK, 0, "血量 500");
    ASSERT_PUB_MSG(OK, 0, "天赋 垃圾回收");
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(OK, 0, "19");  ASSERT_PUB_MSG(CHECKOUT, 1, "0");
    ASSERT_PUB_MSG(OK, 0, "18");  ASSERT_PUB_MSG(CHECKOUT, 1, "0");
    ASSERT_PUB_MSG(OK, 0, "1");   ASSERT_PUB_MSG(CHECKOUT, 1, "0");
    ASSERT_PUB_MSG(OK, 0, "2");   ASSERT_PUB_MSG(CHECKOUT, 1, "0");
    ASSERT_PUB_MSG(OK, 0, "17");  ASSERT_PUB_MSG(CHECKOUT, 1, "0");
    ASSERT_PUB_MSG(OK, 0, "16");  ASSERT_PUB_MSG(CHECKOUT, 1, "0");
    ASSERT_PUB_MSG(OK, 0, "3");   ASSERT_PUB_MSG(CHECKOUT, 1, "0");
    for (int i = 0; i < 200 && !this->main_stage_->IsOver(); ++i) this->TimeoutRequest_();
    ASSERT_FINAL_SCORE(94, 94);
}

// ---- 1c. 多人崩溃测试：仅验证游戏能跑完 ----

// 4-player baseline (无天赋)
GAME_TEST(4, four_player_baseline) {
    ASSERT_PUB_MSG(OK, 0, "种子 test");
    ASSERT_PUB_MSG(OK, 0, "事件 无");
    ASSERT_TRUE(StartGame());
    for (int i = 0; i < 300 && !this->main_stage_->IsOver(); ++i) this->TimeoutRequest_();
    ASSERT_FINISHED(true);
}

// 4-player 基本天赋组合
GAME_TEST(4, four_player_with_talents) {
    ASSERT_PUB_MSG(OK, 0, "种子 test");
    ASSERT_PUB_MSG(OK, 0, "事件 无");
    ASSERT_PUB_MSG(OK, 0, "天赋 以退为进 来点实在的");
    ASSERT_TRUE(StartGame());
    for (int i = 0; i < 300 && !this->main_stage_->IsOver(); ++i) this->TimeoutRequest_();
    ASSERT_FINISHED(true);
}

// 4-player 含主动触发的复杂天赋（三相之力 摇奖机 紧急救援 ...）
GAME_TEST(4, four_player_new_talents) {
    ASSERT_PUB_MSG(OK, 0, "种子 test");
    ASSERT_PUB_MSG(OK, 0, "事件 无");
    ASSERT_PUB_MSG(OK, 0, "天赋 三相之力 事不过三 摇奖机 紧急救援 败者之刃 包扎");
    ASSERT_TRUE(StartGame());
    for (int i = 0; i < 300 && !this->main_stage_->IsOver(); ++i) this->TimeoutRequest_();
    ASSERT_FINISHED(true);
}

// 4-player 含板面变换型天赋（潘多拉魔盒 利滚利 戴森球 ...）
GAME_TEST(4, four_player_new_talents_2) {
    ASSERT_PUB_MSG(OK, 0, "种子 test");
    ASSERT_PUB_MSG(OK, 0, "事件 无");
    ASSERT_PUB_MSG(OK, 0, "天赋 潘多拉魔盒 利滚利 戴森球 两极反转 临时用品");
    ASSERT_TRUE(StartGame());
    for (int i = 0; i < 300 && !this->main_stage_->IsOver(); ++i) this->TimeoutRequest_();
    ASSERT_FINISHED(true);
}

// ============================================================================
// 2. A 级天赋单独测试 (single A-tier, auto-play)
//    顺序按 Talent enum 在 talent.h 中的声明顺序。
// ============================================================================

AUTO_PLAY_TEST(talent_perfect_block,        "完美块",     42, 51)
AUTO_PLAY_TEST(talent_counterattack,        "绝地反击",   6, 15)
AUTO_PLAY_TEST(talent_seize,                "占得先机",   6, 15)
AUTO_PLAY_TEST(talent_iron_body,            "钢铁之躯",   6, 15)
AUTO_PLAY_TEST_FINAL(talent_retreat_advance,"以退为进",   24, 42)
AUTO_PLAY_TEST(talent_deadly_magic,         "致命魔术",   6, 15)
AUTO_PLAY_TEST(talent_tri_force,            "三相之力",   6, 15)
AUTO_PLAY_TEST(talent_emergency_rescue,     "紧急救援",   33, 42)
AUTO_PLAY_TEST(talent_want_all,             "我全都要",   6, 15)
AUTO_PLAY_TEST(talent_pandora_box,          "潘多拉魔盒", 6, 15)
AUTO_PLAY_TEST(talent_compound_interest,    "利滚利",     6, 15)
AUTO_PLAY_TEST(talent_dyson_sphere,         "戴森球",     6, 15)
AUTO_PLAY_TEST(talent_discard_scorer,       "0号位",      6, 15)
AUTO_PLAY_TEST(talent_sincere,              "坦诚相见",   6, 15)
AUTO_PLAY_TEST(talent_galaxy_flow,          "星河流转",   6, 15)
AUTO_PLAY_TEST_FINAL(talent_meditation,     "冥想",       6, 15)
AUTO_PLAY_TEST(talent_light_interference,   "光波干涉",   6, 15)
AUTO_PLAY_TEST_FINAL(talent_nine_mystery,   "九转玄机",   48, 42)
AUTO_PLAY_TEST(talent_qiankun_move,         "乾坤大挪移", 6, 15)
AUTO_PLAY_TEST(talent_key_choice,           "关键选择",   6, 15)
AUTO_PLAY_TEST_FINAL(talent_life_game,      "生命游戏",   16, 17)
AUTO_PLAY_TEST(talent_y_zone,               "Y区域",      6, 15)

// ============================================================================
// 3. B 级天赋单独测试 (single B-tier, auto-play)
//    顺序按 Talent enum 在 talent.h 中的声明顺序。
// ============================================================================

AUTO_PLAY_TEST(talent_bloodlust,                    "嗜血",       6, 15)
AUTO_PLAY_TEST(talent_still_useful,                 "还是有用的", 12, 15)
AUTO_PLAY_TEST(talent_swift_attack,                 "快攻",       6, 15)
AUTO_PLAY_TEST(talent_independent,                  "特立独行",   6, 15)
AUTO_PLAY_TEST(talent_in_pairs,                     "成双成对",   6, 15)
AUTO_PLAY_TEST(talent_trash_recycle,                "垃圾回收",   100, 109)
AUTO_PLAY_TEST(talent_something_real,               "来点实在的", 10, 19)
AUTO_PLAY_TEST(talent_offensive_form,               "攻击形态",   6, 15)
AUTO_PLAY_TEST(talent_defensive_form,               "防御形态",   6, 15)
AUTO_PLAY_TEST(talent_local_enhance,                "局部强化",   9, 15)
AUTO_PLAY_TEST(talent_gain_after_loss,              "有舍有得",   6, 15)
AUTO_PLAY_TEST_FINAL(talent_three_year,             "三年之期",   24, 24)
AUTO_PLAY_TEST(talent_forge,                        "锻造",       6, 15)
AUTO_PLAY_TEST(talent_tail_goods,                   "尾货处理",   6, 15)
AUTO_PLAY_TEST(talent_turing_test,                  "图灵测试",   6, 15)
AUTO_PLAY_TEST(talent_no_more_than_three,           "事不过三",   6, 15)
AUTO_PLAY_TEST(talent_slot_machine,                 "摇奖机",     6, 15)
AUTO_PLAY_TEST(talent_digit_reverse,                "两极反转",   6, 15)
AUTO_PLAY_TEST(talent_loser_blade,                  "败者之刃",   6, 15)
AUTO_PLAY_TEST(talent_temp_wild,                    "临时用品",   6, 15)
AUTO_PLAY_TEST_FINAL(talent_bandage,                "包扎",       6, 15)
AUTO_PLAY_TEST(talent_herbal_growth,                "百味草",     6, 15)
AUTO_PLAY_TEST(talent_angel_round,                  "天使轮",     6, 15)
AUTO_PLAY_TEST(talent_plunder,                      "劫掠",       6, 15)
AUTO_PLAY_TEST(talent_multi_choice,                 "多维抉择",   6, 15)
AUTO_PLAY_TEST(talent_zhang_san,                    "张三来袭",   6, 15)
AUTO_PLAY_TEST_FINAL(talent_greedy_treasure,        "贪婪宝藏",   33, 33)
AUTO_PLAY_TEST_FINAL(talent_zero_power,             "0的力量",    33, 49)
AUTO_PLAY_TEST(talent_void_heart,                   "虚空之心",   6, 15)
AUTO_PLAY_TEST_FINAL(talent_performance_personality,"表演型人格", 7, 14)
AUTO_PLAY_TEST(talent_inner_ring,                   "二环里",     6, 15)
AUTO_PLAY_TEST(talent_chestnut,                     "恭喜栗子",   6, 15)
AUTO_PLAY_TEST(talent_vitality,                     "勃勃生机",   6, 15)
AUTO_PLAY_TEST(talent_one_way,                      "一方通行",   6, 15)
AUTO_PLAY_TEST(talent_zero_risk,                    "零风险投资", 6, 15)
AUTO_PLAY_TEST(talent_battle_hardened,              "以战代练",   6, 15)
AUTO_PLAY_TEST(talent_fatal_rhythm,                 "致命节奏",   6, 15)
AUTO_PLAY_TEST(talent_time_anchor,                  "时间锚",     6, 15)
AUTO_PLAY_TEST(talent_rhythm_remnant,               "律动残余",   6, 15)

// ============================================================================
// 4. 多天赋组合测试 (multi-talent combos, auto-play)
//    验证多个天赋叠加时的交互行为；以 以退为进 为基线对比。
// ============================================================================

// 以退为进 creates lines (score 45,36). Adding 完美块 doesn't change since
// card_412 → (3,1,2) → wild doesn't create additional lines in auto-play.
AUTO_PLAY_TEST_FINAL(combo_retreat_perfect_block,        "以退为进 完美块",            84, 102)

// 还是有用的 requires lines with 1/2 to add bonus; 以退为进 lines are 3/6/9, no effect.
AUTO_PLAY_TEST_FINAL(combo_retreat_still_useful,         "以退为进 还是有用的",        30, 42)

// 来点实在的 adds flat +4; stacks additively with 以退为进's 45/36.
AUTO_PLAY_TEST_FINAL(combo_retreat_something_real,       "以退为进 来点实在的",        28, 46)

// 零风险投资 prevents score decrease; with 以退为进 lines, score is same.
AUTO_PLAY_TEST_FINAL(combo_retreat_zero_risk,            "以退为进 零风险投资",        24, 42)

// 局部强化 + 以退为进: local enhance adds extra points from matching center digits.
AUTO_PLAY_TEST_FINAL(combo_retreat_local_enhance,        "以退为进 局部强化",          27, 42)

// 攻击形态 + 防御形态: both are combat modifiers, no score effect in auto-play.
AUTO_PLAY_TEST(combo_offensive_defensive,                "攻击形态 防御形态",          6, 15)

// 成双成对 + 特立独行: contradictory conditions, both 0 in auto-play.
AUTO_PLAY_TEST(combo_pairs_independent,                  "成双成对 特立独行",          6, 15)

// Triple combo: 以退为进(45/36) + 来点实在的(+4) + 垃圾回收(+94) = 143/134
AUTO_PLAY_TEST_FINAL(combo_triple_retreat_real_recycle,  "以退为进 来点实在的 垃圾回收", 72, 90)

// 三相之力 + 以退为进: directional wilds combine with retreat transforms.
AUTO_PLAY_TEST_FINAL(combo_tri_force_retreat,            "三相之力 以退为进",          24, 42)

// 摇奖机 + 以退为进: slot machine may become another A-tier, combined with retreat.
AUTO_PLAY_TEST_FINAL(combo_slot_machine_retreat,         "摇奖机 以退为进",            24, 42)

// 0号位 + 以退为进: discard scorer's temp score doesn't affect ASSERT_SCORE (board total only).
AUTO_PLAY_TEST_FINAL(combo_discard_scorer_retreat,       "0号位 以退为进",             24, 42)

// 两极反转 + 以退为进: 1↔9 swap before retreat's 7→6/4→3 transform.
AUTO_PLAY_TEST_FINAL(combo_digit_reverse_retreat,        "两极反转 以退为进",          24, 42)

// 九转玄机 + 两极反转：两极反转 OnAcquire 翻转后链式 ApplyNineAsWild，把新出现的 9 全部转为癞子。
AUTO_PLAY_TEST_FINAL(combo_digit_reverse_nine_mystery,   "九转玄机 两极反转",          13, 22)

// ============================================================================
// 5. 名次结算冒烟测试 (rank-based scoring smoke tests)
//    生产构建（无 TEST_BOT）下 PlayerScore 返回按淘汰名次结算的 game_score；
//    测试构建（TEST_BOT）下 PlayerScore 仍返回盘面分以兼容历史断言。
//    这里只验证不同人数、不同 HP 设置下游戏能正常跑完，不直接断言名次分数（具体
//    分布通过手算验证：4 人完整分胜负 → -300/-100/+100/+300；同回合同阶段
//    并列；中毒淘汰晚于对战；卡池耗尽多人并列第一。详见 plan 文件）。
// ============================================================================

GAME_TEST(2, rank_score_2_player_low_hp) {
    ASSERT_PUB_MSG(OK, 0, "种子 test");
    ASSERT_PUB_MSG(OK, 0, "事件 无");
    ASSERT_PUB_MSG(OK, 0, "血量 50");
    ASSERT_TRUE(StartGame());
    for (int i = 0; i < 300 && !this->main_stage_->IsOver(); ++i) this->TimeoutRequest_();
    ASSERT_FINISHED(true);
}

GAME_TEST(4, rank_score_4_player_low_hp) {
    ASSERT_PUB_MSG(OK, 0, "种子 test");
    ASSERT_PUB_MSG(OK, 0, "事件 无");
    ASSERT_PUB_MSG(OK, 0, "血量 50");
    ASSERT_TRUE(StartGame());
    for (int i = 0; i < 400 && !this->main_stage_->IsOver(); ++i) this->TimeoutRequest_();
    ASSERT_FINISHED(true);
}

GAME_TEST(2, rank_score_pool_exhaust_survival) {
    ASSERT_PUB_MSG(OK, 0, "种子 test");
    ASSERT_PUB_MSG(OK, 0, "事件 无");
    ASSERT_PUB_MSG(OK, 0, "血量 500");
    ASSERT_TRUE(StartGame());
    for (int i = 0; i < 600 && !this->main_stage_->IsOver(); ++i) this->TimeoutRequest_();
    ASSERT_FINISHED(true);
}

} // namespace GAME_MODULE_NAME
} // namespace game
} // namespace lgtbot

int main(int argc, char** argv) { testing::InitGoogleTest(&argc, argv); gflags::ParseCommandLineFlags(&argc, &argv, true); return RUN_ALL_TESTS(); }
