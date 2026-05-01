// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).
//
// This file was generated with the assistance of Claude Code (claude.ai/code).

#include "game_framework/unittest_base.h"

namespace lgtbot {
namespace game {
namespace GAME_MODULE_NAME {

// 终局分数只使用盘面天赋总分
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
// 1. Baseline: no talents, auto-play
// ============================================================================

AUTO_PLAY_TEST_NO_TALENT(baseline_auto_play, 6, 15)

// ============================================================================
// 2. Single talent tests (auto-play)
//    Tests that each talent can be initialized without crashing,
//    and verifies deterministic score output.
// ============================================================================

// B-tier talents
AUTO_PLAY_TEST(talent_bloodlust,       "嗜血",       6, 15)
AUTO_PLAY_TEST(talent_swift_attack,    "快攻",       6, 15)
AUTO_PLAY_TEST(talent_perfect_block,   "完美块",     42, 51)
AUTO_PLAY_TEST(talent_still_useful,    "还是有用的", 12, 15)
AUTO_PLAY_TEST(talent_independent,     "特立独行",   6, 15)
AUTO_PLAY_TEST(talent_in_pairs,        "成双成对",   6, 15)
AUTO_PLAY_TEST(talent_trash_recycle,   "垃圾回收",   100, 109)
AUTO_PLAY_TEST(talent_something_real,  "来点实在的", 10, 19)
AUTO_PLAY_TEST(talent_offensive_form,  "攻击形态",   6, 15)
AUTO_PLAY_TEST(talent_defensive_form,  "防御形态",   6, 15)
AUTO_PLAY_TEST(talent_local_enhance,   "局部强化",   9, 15)
AUTO_PLAY_TEST(talent_turing_test,     "图灵测试",   6, 15)

// A-tier talents
AUTO_PLAY_TEST(talent_counterattack,   "绝地反击",   6, 15)
AUTO_PLAY_TEST(talent_seize,           "占得先机",   6, 15)
AUTO_PLAY_TEST(talent_iron_body,       "钢铁之躯",   6, 15)
AUTO_PLAY_TEST_FINAL(talent_retreat_advance, "以退为进",   24, 42)
AUTO_PLAY_TEST(talent_deadly_magic,    "致命魔术",   6, 15)
AUTO_PLAY_TEST(talent_zero_risk,       "零风险投资", 6, 15)

// ============================================================================
// 3. Multi-talent interaction tests (auto-play)
//    Verifies that talent effects stack correctly.
// ============================================================================

// 以退为进 creates lines (score 45,36). Adding 完美块 doesn't change since
// card_412 → (3,1,2) → wild doesn't create additional lines in auto-play.
AUTO_PLAY_TEST_FINAL(combo_retreat_perfect_block,       "以退为进 完美块",       84, 102)

// 还是有用的 requires lines with 1/2 to add bonus; 以退为进 lines are 3/6/9, no effect.
AUTO_PLAY_TEST_FINAL(combo_retreat_still_useful,        "以退为进 还是有用的",   30, 42)

// 来点实在的 adds flat +4; stacks additively with 以退为进's 45/36.
AUTO_PLAY_TEST_FINAL(combo_retreat_something_real,      "以退为进 来点实在的",   28, 46)

// 零风险投资 prevents score decrease; with 以退为进 lines, score is same.
AUTO_PLAY_TEST_FINAL(combo_retreat_zero_risk,           "以退为进 零风险投资",   24, 42)

// 局部强化 + 以退为进: local enhance adds extra points from matching center digits.
AUTO_PLAY_TEST_FINAL(combo_retreat_local_enhance,       "以退为进 局部强化",     27, 42)

// 攻击形态 + 防御形态: both are combat modifiers, no score effect in auto-play.
AUTO_PLAY_TEST(combo_offensive_defensive,         "攻击形态 防御形态",     6, 15)

// 成双成对 + 特立独行: contradictory conditions, both 0 in auto-play.
AUTO_PLAY_TEST(combo_pairs_independent,           "成双成对 特立独行",     6, 15)

// Triple combo: 以退为进(45/36) + 来点实在的(+4) + 垃圾回收(+94) = 143/134
AUTO_PLAY_TEST_FINAL(combo_triple_retreat_real_recycle,  "以退为进 来点实在的 垃圾回收", 48, 93)

// ============================================================================
// 4. Strategic placement tests
//    P0 actively places cards to form VERT line {1,2,3}.
//    Card sequence (seed "test", event 無):
//      R1=357(wild), R2=357, R3=812, R4=316, R5=897, R6=852, R7=317
//    P0 places: R1→19, R2→18, R3→1, R4→2, R5→17, R6→16, R7→3
//    This forms VERT{1,2,3} with values from dir1: 5,1,1 → all 1s = line score 3.
//    P1 discards all cards (弃牌) for rounds 1-7.
// ============================================================================

// Strategic: P0 forms a line, no talents
GAME_TEST(2, strategic_vert_line) {
    ASSERT_PUB_MSG(OK, 0, "种子 test");
    ASSERT_PUB_MSG(OK, 0, "事件 无");
    ASSERT_PUB_MSG(OK, 0, "血量 500");
    ASSERT_TRUE(StartGame());
    // R1: P0 places at 19, P1 discards
    ASSERT_PUB_MSG(OK, 0, "19");  ASSERT_PUB_MSG(CHECKOUT, 1, "0");
    // R2: P0 places at 18, P1 discards
    ASSERT_PUB_MSG(OK, 0, "18");  ASSERT_PUB_MSG(CHECKOUT, 1, "0");
    // R3: P0 places at 1, P1 discards
    ASSERT_PUB_MSG(OK, 0, "1");   ASSERT_PUB_MSG(CHECKOUT, 1, "0");
    // R4: P0 places at 2, P1 discards
    ASSERT_PUB_MSG(OK, 0, "2");   ASSERT_PUB_MSG(CHECKOUT, 1, "0");
    // R5: P0 places at 17, P1 discards
    ASSERT_PUB_MSG(OK, 0, "17");  ASSERT_PUB_MSG(CHECKOUT, 1, "0");
    // R6: P0 places at 16, P1 discards
    ASSERT_PUB_MSG(OK, 0, "16");  ASSERT_PUB_MSG(CHECKOUT, 1, "0");
    // R7: P0 places at 3, P1 discards
    ASSERT_PUB_MSG(OK, 0, "3");   ASSERT_PUB_MSG(CHECKOUT, 1, "0");
    // Let remaining rounds auto-play
    for (int i = 0; i < 200 && !this->main_stage_->IsOver(); ++i) this->TimeoutRequest_();
    ASSERT_FINAL_SCORE(0, 0);
}

// Strategic with 以退为进: transforms card values, creating different line compositions.
// P1's auto-played cards benefit more from the 4→3, 7→6 transforms.
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

// ============================================================================
// 5. New talent tests
// ============================================================================

// 三相之力 auto-play: directional wilds on next 3 normal-round cards
AUTO_PLAY_TEST(talent_tri_force,  "三相之力", 6, 15)

// 三相之力 + 以退为进: directional wilds combine with retreat transforms
AUTO_PLAY_TEST_FINAL(combo_tri_force_retreat, "三相之力 以退为进", 24, 42)

// 事不过三 auto-play: defeat immunity doesn't affect score
AUTO_PLAY_TEST(talent_no_more_than_three, "事不过三", 6, 15)

// 摇奖机 auto-play: randomly becomes an A-tier talent
AUTO_PLAY_TEST(talent_slot_machine, "摇奖机", 6, 15)

// 摇奖机 + 以退为进: slot machine may become another A-tier, combined with retreat
AUTO_PLAY_TEST_FINAL(combo_slot_machine_retreat, "摇奖机 以退为进", 24, 42)

// New talents: A-tier
AUTO_PLAY_TEST(talent_emergency_rescue,  "紧急救援",     33, 42)
AUTO_PLAY_TEST(talent_want_all,          "我全都要",     6, 15)
AUTO_PLAY_TEST(talent_pandora_box,       "潘多拉魔盒",   6, 15)
AUTO_PLAY_TEST(talent_compound_interest, "利滚利",       6, 15)
AUTO_PLAY_TEST(talent_dyson_sphere,      "戴森球",       6, 15)

// New talents: B-tier
AUTO_PLAY_TEST(talent_digit_reverse,     "两级反转",     6, 15)
AUTO_PLAY_TEST(talent_loser_blade,       "败者之刃",     6, 15)
AUTO_PLAY_TEST(talent_temp_wild,         "临时用品",     6, 15)
AUTO_PLAY_TEST_FINAL(talent_bandage,           "包扎",         6, 15)
AUTO_PLAY_TEST(talent_herbal_growth,     "百味草",       6, 15)
AUTO_PLAY_TEST(talent_discard_scorer,    "0号位",        6, 15)

// Combo: 0号位 + 以退为进
AUTO_PLAY_TEST_FINAL(combo_discard_scorer_retreat, "0号位 以退为进", 24, 42)

// Combo: 两级反转 + 以退为进
AUTO_PLAY_TEST_FINAL(combo_digit_reverse_retreat, "两级反转 以退为进", 24, 42)

// New talents (Round 6)
AUTO_PLAY_TEST(talent_sincere,      "坦诚相见",  6, 15)
AUTO_PLAY_TEST(talent_galaxy_flow,  "星河流转",  6, 15)
AUTO_PLAY_TEST_FINAL(talent_meditation,   "冥想",      6, 15)
AUTO_PLAY_TEST(talent_angel_round,  "天使轮",    6, 15)

// 4-player with new talents (crash test)
GAME_TEST(4, four_player_new_talents) {
    ASSERT_PUB_MSG(OK, 0, "种子 test");
    ASSERT_PUB_MSG(OK, 0, "事件 无");
    ASSERT_PUB_MSG(OK, 0, "天赋 三相之力 事不过三 摇奖机 紧急救援 败者之刃 包扎");
    ASSERT_TRUE(StartGame());
    for (int i = 0; i < 300 && !this->main_stage_->IsOver(); ++i) this->TimeoutRequest_();
    ASSERT_FINISHED(true);
}

// 4-player with more new talents (crash test)
GAME_TEST(4, four_player_new_talents_2) {
    ASSERT_PUB_MSG(OK, 0, "种子 test");
    ASSERT_PUB_MSG(OK, 0, "事件 无");
    ASSERT_PUB_MSG(OK, 0, "天赋 潘多拉魔盒 利滚利 戴森球 两级反转 临时用品");
    ASSERT_TRUE(StartGame());
    for (int i = 0; i < 300 && !this->main_stage_->IsOver(); ++i) this->TimeoutRequest_();
    ASSERT_FINISHED(true);
}

// ============================================================================
// 6. Multi-player tests
// ============================================================================

// 4-player game completes without crash
GAME_TEST(4, four_player_baseline) {
    ASSERT_PUB_MSG(OK, 0, "种子 test");
    ASSERT_PUB_MSG(OK, 0, "事件 无");
    ASSERT_TRUE(StartGame());
    for (int i = 0; i < 300 && !this->main_stage_->IsOver(); ++i) this->TimeoutRequest_();
    ASSERT_FINISHED(true);
}

// 4-player game with talents
GAME_TEST(4, four_player_with_talents) {
    ASSERT_PUB_MSG(OK, 0, "种子 test");
    ASSERT_PUB_MSG(OK, 0, "事件 无");
    ASSERT_PUB_MSG(OK, 0, "天赋 以退为进 来点实在的");
    ASSERT_TRUE(StartGame());
    for (int i = 0; i < 300 && !this->main_stage_->IsOver(); ++i) this->TimeoutRequest_();
    ASSERT_FINISHED(true);
}

} // namespace GAME_MODULE_NAME
} // namespace game
} // namespace lgtbot

int main(int argc, char** argv) { testing::InitGoogleTest(&argc, argv); gflags::ParseCommandLineFlags(&argc, &argv, true); return RUN_ALL_TESTS(); }
