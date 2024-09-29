// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

GAME_TEST(4, player_not_enough)
{
    ASSERT_FALSE(StartGame());
}

GAME_TEST(5, start_game)
{
    START_GAME();
}

GAME_TEST(6, start_game)
{
    START_GAME();
}

GAME_TEST(7, start_game)
{
    START_GAME();
}

GAME_TEST(8, start_game)
{
    START_GAME();
}

GAME_TEST(9, start_game)
{
    START_GAME();
}

GAME_TEST(5, user_define_occupation_list_size_mismatch)
{
    ASSERT_PUB_MSG(OK, 0, "五人身份 替身 平民 平民 圣女 守卫 平民");
    ASSERT_FALSE(StartGame());
}

GAME_TEST(5, user_define_occupation_list_must_has_killer)
{
    ASSERT_PUB_MSG(OK, 0, "五人身份 替身 平民 平民 圣女 守卫");
    ASSERT_FALSE(StartGame());
}

GAME_TEST(5, user_define_occupation_list_cannot_has_two_killers)
{
    ASSERT_PUB_MSG(OK, 0, "五人身份 杀手 杀手 替身 圣女 守卫");
    ASSERT_FALSE(StartGame());
}

GAME_TEST(5, user_define_occupation_list)
{
    ASSERT_PUB_MSG(OK, 0, "五人身份 替身 杀手 平民 圣女 守卫");
    START_GAME();
    for (uint32_t i = 0; i < 5; ++i) {
        if (CHECK_PRI_MSG(OK, i, "挡刀 A")) {
            return;
        }
    }
    FAIL();
}

GAME_TEST(5, user_define_occupation_list_with_npc)
{
    ASSERT_PUB_MSG(OK, 0, "五人身份 替身 杀手 平民 圣女 守卫 人偶");
    START_GAME();
    for (uint32_t i = 0; i < 5; ++i) {
        if (CHECK_PRI_MSG(OK, i, "挡刀 A")) {
            return;
        }
    }
    FAIL();
}

GAME_TEST(5, max_three_cure)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 替身 杀手 平民 圣女 侦探");
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "治愈 A 10");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(CONTINUE, 0, "治愈 A 10");
    ASSERT_PRI_MSG(CONTINUE, 0, "治愈 A 10");
    ASSERT_PRI_MSG(FAILED, 0, "治愈 A 10");
}

GAME_TEST(5, cannot_act_after_dead)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 平民 平民 平民");
    ASSERT_PUB_MSG(OK, 0, "血量 15");
    START_GAME();
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(CONTINUE, 1, "攻击 B 15");
    ASSERT_ELIMINATED(1);
    ASSERT_PRI_MSG(CONTINUE, 0, "pass"); // need not wait B act
}

GAME_TEST(5, cannot_hurt_cure_dead_role)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 平民 平民 平民");
    ASSERT_PUB_MSG(OK, 0, "血量 15");
    START_GAME();
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(CONTINUE, 0, "攻击 B 25");
    ASSERT_PRI_MSG(FAILED, 0, "治愈 B 10");
    ASSERT_PRI_MSG(FAILED, 0, "攻击 B 15");
}

GAME_TEST(5, can_detect_dead_role)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 侦探 平民 平民");
    ASSERT_PUB_MSG(OK, 0, "血量 15");
    START_GAME();
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(CONTINUE, 0, "攻击 B 25");
    ASSERT_PRI_MSG(OK, 2, "侦查 B");
}

GAME_TEST(5, killer_win_by_three_civilian_team_dead)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 替身 杀手 平民 圣女 侦探");
    START_GAME();
    ASSERT_PRI_MSG(OK, 1, "攻击 C 25");
    ASSERT_TIMEOUT(CONTINUE);
    for (uint32_t i = 0; i < 4; ++i) {
        ASSERT_PRI_MSG(CONTINUE, 1, "攻击 D 25");
        ASSERT_PRI_MSG(CONTINUE, 1, "攻击 E 25");
    }
    ASSERT_PRI_MSG(CONTINUE, 1, "攻击 C 25");
    ASSERT_PRI_MSG(CONTINUE, 1, "攻击 C 25");
    ASSERT_PRI_MSG(CHECKOUT, 1, "攻击 C 25");
    ASSERT_SCORE(1, 1, 0, 0, 0);
}

GAME_TEST(5, killer_win_by_two_civilian_role_dead)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 替身 杀手 平民 平民 侦探");
    START_GAME();
    ASSERT_PRI_MSG(OK, 1, "攻击 C 25");
    ASSERT_TIMEOUT(CONTINUE);
    for (uint32_t i = 0; i < 4; ++i) {
        ASSERT_PRI_MSG(CONTINUE, 1, "攻击 D 25");
    }
    ASSERT_PRI_MSG(CONTINUE, 1, "攻击 C 25");
    ASSERT_PRI_MSG(CONTINUE, 1, "攻击 C 25");
    ASSERT_PRI_MSG(CHECKOUT, 1, "攻击 C 25");
    ASSERT_SCORE(1, 1, 0, 0, 0);
}

GAME_TEST(5, civilian_win)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 替身 杀手 平民 圣女 侦探");
    START_GAME();
    ASSERT_PRI_MSG(OK, 1, "攻击 B 25");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(CONTINUE, 1, "攻击 B 25");
    ASSERT_PRI_MSG(CONTINUE, 1, "攻击 B 25");
    ASSERT_PRI_MSG(CHECKOUT, 1, "攻击 B 25");
    ASSERT_SCORE(0, 0, 1, 1, 1);
}

GAME_TEST(5, civilian_and_killer_dead_at_the_same_time_killer_win)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 替身 杀手 平民 圣女 侦探");
    START_GAME();
    ASSERT_PRI_MSG(OK, 1, "攻击 C 25");
    ASSERT_TIMEOUT(CONTINUE);
    for (uint32_t i = 0; i < 4; ++i) {
        ASSERT_PRI_MSG(CONTINUE, 1, "攻击 D 25");
        ASSERT_PRI_MSG(CONTINUE, 1, "攻击 E 25");
    }
    ASSERT_PRI_MSG(CONTINUE, 1, "攻击 C 25");
    ASSERT_PRI_MSG(CONTINUE, 1, "攻击 C 25"); // C hp = 25
    ASSERT_PRI_MSG(CONTINUE, 1, "攻击 C 15"); // C hp = 10
    ASSERT_PRI_MSG(CONTINUE, 1, "攻击 B 25");
    ASSERT_PRI_MSG(CONTINUE, 1, "攻击 B 25");
    ASSERT_PRI_MSG(CONTINUE, 1, "攻击 B 25");
    ASSERT_PRI_MSG(OK, 0, "攻击 C 15"); // C dead
    ASSERT_PRI_MSG(CHECKOUT, 1, "攻击 B 25"); // B dead
    ASSERT_SCORE(1, 1, 0, 0, 0);
}

GAME_TEST(5, killer_heavy_hurt)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 替身 杀手 平民 圣女 侦探");
    ASSERT_PUB_MSG(OK, 0, "血量 50");
    START_GAME();
    ASSERT_PRI_MSG(OK, 1, "攻击 A 25");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(CONTINUE, 1, "攻击 A 25");
    ASSERT_ELIMINATED(0);
}

GAME_TEST(5, normal_player_not_allow_heavy_hurt)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 替身 杀手 平民 圣女 侦探");
    START_GAME();
    ASSERT_PRI_MSG(FAILED, 0, "攻击 A 25");
    ASSERT_PRI_MSG(FAILED, 2, "攻击 A 25");
    ASSERT_PRI_MSG(FAILED, 3, "攻击 A 25");
    ASSERT_PRI_MSG(FAILED, 4, "攻击 A 25");
}

GAME_TEST(5, body_double_block_hurt_to_killer)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 替身 杀手 平民 圣女 侦探");
    ASSERT_PUB_MSG(OK, 0, "血量 75");
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "挡刀 B");
    ASSERT_PRI_MSG(OK, 1, "攻击 B 25");
    ASSERT_PRI_MSG(OK, 2, "攻击 B 15");
    ASSERT_PRI_MSG(OK, 3, "攻击 B 15");
    ASSERT_PRI_MSG(CONTINUE, 4, "攻击 B 15");
    ASSERT_PRI_MSG(OK, 1, "攻击 A 15"); // B is alive
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_ELIMINATED(0); // A is dead
}

GAME_TEST(5, body_double_block_hurt_from_someone)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 替身 杀手 平民 圣女 侦探");
    ASSERT_PUB_MSG(OK, 0, "血量 60");
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "挡刀 C");
    ASSERT_PRI_MSG(OK, 1, "攻击 C 25");
    ASSERT_PRI_MSG(OK, 2, "攻击 C 15");
    ASSERT_PRI_MSG(OK, 3, "pass");
    ASSERT_PRI_MSG(CONTINUE, 4, "攻击 C 15");
    ASSERT_PRI_MSG(OK, 2, "攻击 A 15"); // C is alive
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_ELIMINATED(0); // A is dead
}

GAME_TEST(5, goddess_can_not_hurt_civilian_team)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 替身 杀手 平民 圣女 侦探");
    ASSERT_PUB_MSG(OK, 0, "血量 15");
    START_GAME();
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(CONTINUE, 3, "攻击 C 15");
    ASSERT_PRI_MSG(CONTINUE, 3, "pass");
    ASSERT_PRI_MSG(CONTINUE, 3, "攻击 D 15");
    ASSERT_PRI_MSG(CONTINUE, 3, "pass");
    ASSERT_PRI_MSG(CONTINUE, 3, "攻击 E 15");
    ASSERT_PRI_MSG(CONTINUE, 3, "pass");
    ASSERT_PRI_MSG(CHECKOUT, 3, "攻击 B 15");
    ASSERT_SCORE(0, 0, 1, 1, 1);
}

GAME_TEST(5, DISABLED_goddess_can_hurt_civilian)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 替身 杀手 圣女 平民 内奸");
    ASSERT_PUB_MSG(OK, 0, "血量 5");
    START_GAME();
    ASSERT_PRI_MSG(OK, 2, "攻击 D 15");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_ELIMINATED(3);
}

GAME_TEST(5, goddess_can_hurt_traitor)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 替身 杀手 圣女 平民 内奸");
    ASSERT_PUB_MSG(OK, 0, "血量 5");
    START_GAME();
    ASSERT_PRI_MSG(OK, 2, "攻击 E 15");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_ELIMINATED(4);
}

GAME_TEST(5, goddess_cant_repeat_hurt)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 替身 杀手 平民 圣女 侦探");
    START_GAME();
    ASSERT_PRI_MSG(OK, 3, "攻击 C 15");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(FAILED, 3, "攻击 C 15");
}

GAME_TEST(5, goddess_not_limit_cure)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 替身 杀手 平民 圣女 侦探");
    START_GAME();
    ASSERT_PRI_MSG(OK, 3, "治愈 A 10");
    ASSERT_TIMEOUT(CONTINUE);
    for (uint32_t i = 0; i < 10; ++i) {
        ASSERT_PRI_MSG(CONTINUE, 3, "治愈 A 10");
    }
}

GAME_TEST(5, detective_first_round_cant_detect)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 替身 杀手 平民 圣女 侦探");
    START_GAME();
    ASSERT_PRI_MSG(FAILED, 4, "侦查 A");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(CONTINUE, 4, "侦查 A");
}

GAME_TEST(5, detective_cant_repeat_detect)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 替身 杀手 平民 圣女 侦探");
    START_GAME();
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(CONTINUE, 4, "侦查 A");
    ASSERT_PRI_MSG(FAILED, 4, "侦查 A");
}

GAME_TEST(5, killer_dead_traitor_alive_not_over)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 平民 圣女 侦探 内奸");
    START_GAME();
    ASSERT_TIMEOUT(CONTINUE);
    for (uint32_t i = 0; i < 4; ++i) {
        ASSERT_PRI_MSG(CONTINUE, 0, "攻击 A 25");
    }
    // game not over because traitor is alive
    for (uint32_t i = 0; i < 6; ++i) {
        ASSERT_PRI_MSG(CONTINUE, 1, "攻击 E 15");
    }
    ASSERT_PRI_MSG(CHECKOUT, 1, "攻击 E 15");
    ASSERT_SCORE(0, 1, 1, 1, 0);
}

GAME_TEST(5, killer_dead_first_version_traitor_alive_not_over)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 平民 圣女 侦探 初版内奸");
    START_GAME();
    ASSERT_TIMEOUT(CONTINUE);
    for (uint32_t i = 0; i < 4; ++i) {
        ASSERT_PRI_MSG(CONTINUE, 0, "攻击 A 25");
    }
    // game not over because traitor is alive
    for (uint32_t i = 0; i < 6; ++i) {
        ASSERT_PRI_MSG(CONTINUE, 1, "攻击 E 15");
    }
    ASSERT_PRI_MSG(CHECKOUT, 1, "攻击 E 15");
    ASSERT_SCORE(0, 1, 1, 1, 0);
}

GAME_TEST(5, three_civilians_dead_traitor_alive_not_over)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 平民 圣女 侦探 内奸");
    START_GAME();
    ASSERT_TIMEOUT(CONTINUE);
    for (uint32_t i = 0; i < 4; ++i) {
        ASSERT_PRI_MSG(CONTINUE, 0, "攻击 B 25");
        ASSERT_PRI_MSG(CONTINUE, 0, "攻击 C 25");
        ASSERT_PRI_MSG(CONTINUE, 0, "攻击 D 25");
    }
    // game not over because traitor is alive
    for (uint32_t i = 0; i < 3; ++i) {
        ASSERT_PRI_MSG(CONTINUE, 0, "攻击 E 25");
    }
    ASSERT_PRI_MSG(CHECKOUT, 0, "攻击 E 25");
    ASSERT_SCORE(1, 0, 0, 0, 0);
}

GAME_TEST(5, traitor_dead_first_then_killer_and_civilian_dead_at_the_same_time_killer_win)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 平民 圣女 侦探 内奸");
    START_GAME();
    ASSERT_TIMEOUT(CONTINUE);
    for (uint32_t i = 0; i < 4; ++i) {
        ASSERT_PRI_MSG(CONTINUE, 0, "攻击 E 25");
    }
    for (uint32_t i = 0; i < 4; ++i) {
        ASSERT_PRI_MSG(CONTINUE, 0, "攻击 B 25");
        ASSERT_PRI_MSG(CONTINUE, 0, "攻击 C 25");
    }
    ASSERT_PRI_MSG(CONTINUE, 0, "攻击 A 25");
    ASSERT_PRI_MSG(CONTINUE, 0, "攻击 A 25"); // A hp = 25
    ASSERT_PRI_MSG(CONTINUE, 0, "攻击 A 15"); // A hp = 10
    ASSERT_PRI_MSG(CONTINUE, 0, "攻击 D 25");
    ASSERT_PRI_MSG(CONTINUE, 0, "攻击 D 25");
    ASSERT_PRI_MSG(CONTINUE, 0, "攻击 D 25");
    ASSERT_PRI_MSG(OK, 3, "攻击 A 15"); // A dead
    ASSERT_PRI_MSG(CHECKOUT, 0, "攻击 D 25"); // D dead
    ASSERT_SCORE(1, 0, 0, 0, 0);
}

GAME_TEST(5, killer_and_three_civilian_dead_traitor_win)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 平民 圣女 侦探 内奸");
    ASSERT_PUB_MSG(OK, 0, "回合数 100");
    START_GAME();
    ASSERT_TIMEOUT(CONTINUE);
    for (uint32_t i = 0; i < 4; ++i) {
        ASSERT_PRI_MSG(CONTINUE, 0, "攻击 A 25");
    }
    // killer dead at the first time
    for (uint32_t i = 0; i < 6; ++i) {
        ASSERT_PRI_MSG(CONTINUE, 4, "攻击 B 15");
        ASSERT_PRI_MSG(CONTINUE, 4, "攻击 C 15");
        ASSERT_PRI_MSG(CONTINUE, 4, "攻击 D 15");
    }
    ASSERT_PRI_MSG(CONTINUE, 4, "攻击 B 15");
    ASSERT_PRI_MSG(CONTINUE, 4, "攻击 C 15");
    ASSERT_PRI_MSG(CHECKOUT, 4, "攻击 D 15");
    ASSERT_SCORE(0, 0, 0, 0, 1);
}

GAME_TEST(5, civilian_and_traitor_dead_at_the_same_time_traitor_win)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 平民 平民 平民 内奸");
    ASSERT_PUB_MSG(OK, 0, "回合数 100");
    START_GAME();
    ASSERT_TIMEOUT(CONTINUE);
    for (uint32_t i = 0; i < 4; ++i) {
        ASSERT_PRI_MSG(CONTINUE, 0, "攻击 A 25");
    }
    ASSERT_PRI_MSG(CONTINUE, 4, "pass"); // to prevent immediate checkout for following requests
    for (uint32_t i = 0; i < 6; ++i) {
        ASSERT_PRI_MSG(OK, 1, "攻击 B 15");
        ASSERT_PRI_MSG(OK, 2, "攻击 C 15");
        ASSERT_PRI_MSG(OK, 3, "攻击 D 15");
        ASSERT_PRI_MSG(CONTINUE, 4, "攻击 E 15");
    }
    ASSERT_PRI_MSG(OK, 1, "攻击 B 15");
    ASSERT_PRI_MSG(OK, 2, "攻击 C 15");
    ASSERT_PRI_MSG(OK, 3, "攻击 D 15");
    ASSERT_PRI_MSG(CHECKOUT, 4, "攻击 E 15");
    ASSERT_SCORE(0, 0, 0, 0, 1);
}

GAME_TEST(5, civilian_and_killer_and_traitor_dead_at_the_same_time_traitor_win)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 平民 平民 平民 内奸");
    ASSERT_PUB_MSG(OK, 0, "回合数 100");
    START_GAME();
    for (uint32_t i = 0; i < 6; ++i) {
        ASSERT_PRI_MSG(OK, 0, "攻击 A 15");
        ASSERT_PRI_MSG(OK, 1, "攻击 B 15");
        ASSERT_PRI_MSG(OK, 2, "攻击 C 15");
        ASSERT_PRI_MSG(OK, 3, "攻击 D 15");
        ASSERT_PRI_MSG(CONTINUE, 4, "攻击 E 15");
    }
    ASSERT_PRI_MSG(OK, 0, "攻击 A 15");
    ASSERT_PRI_MSG(OK, 1, "攻击 B 15");
    ASSERT_PRI_MSG(OK, 2, "攻击 C 15");
    ASSERT_PRI_MSG(OK, 3, "攻击 D 15");
    ASSERT_PRI_MSG(CHECKOUT, 4, "攻击 E 15");
    ASSERT_SCORE(0, 0, 0, 0, 1);
}

GAME_TEST(5, killer_dead_body_double_cannot_act)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 平民 平民 内奸");
    ASSERT_PUB_MSG(OK, 0, "回合数 100");
    START_GAME();
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(CONTINUE, 2, "pass");
    for (uint32_t i = 0; i < 7; ++i) {
        ASSERT_PRI_MSG(OK, 1, "pass");
        ASSERT_PRI_MSG(CONTINUE, 2, "攻击 A 15");
    }
    ASSERT_ELIMINATED(1);
    ASSERT_PRI_MSG(CONTINUE, 2, "pass"); // need not wait body double act
}

GAME_TEST(6, civilian_lost_cannot_act)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 侦探 圣女 平民 平民 内奸");
    START_GAME();
    ASSERT_TIMEOUT(CONTINUE);
    for (uint32_t i = 0; i < 4; ++i) {
        ASSERT_PRI_MSG(CONTINUE, 0, "攻击 B 25");
        ASSERT_PRI_MSG(CONTINUE, 0, "攻击 C 25");
        ASSERT_PRI_MSG(CONTINUE, 0, "攻击 D 25");
    }
    ASSERT_ELIMINATED(1);
    ASSERT_ELIMINATED(2);
    ASSERT_ELIMINATED(3);
    ASSERT_ELIMINATED(4);
}

GAME_TEST(5, traitor_can_heavy_hurt_cure_after_killer_dead)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 平民 平民 内奸");
    ASSERT_PUB_MSG(OK, 0, "回合数 100");
    START_GAME();
    ASSERT_PRI_MSG(FAILED, 4, "攻击 B 25");
    ASSERT_TIMEOUT(CONTINUE);
    for (uint32_t i = 0; i < 4; ++i) {
        ASSERT_PRI_MSG(CONTINUE, 0, "攻击 A 25");
    }
    ASSERT_PRI_MSG(CONTINUE, 4, "攻击 B 25");
}

GAME_TEST(5, killer_cannot_win_when_two_civilians_and_one_traitor_dead)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 平民 杀手 圣女 平民 内奸");
    START_GAME();
    ASSERT_TIMEOUT(CONTINUE);
    for (uint32_t i = 0; i < 4; ++i) {
        ASSERT_PRI_MSG(CONTINUE, 1, "攻击 C 25");
        ASSERT_PRI_MSG(CONTINUE, 1, "攻击 D 25");
        ASSERT_PRI_MSG(CONTINUE, 1, "攻击 E 25");
    }
    ASSERT_FINISHED(false);
}

GAME_TEST(5, max_round_killer_failed)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 替身 杀手 平民 平民 内奸");
    ASSERT_PUB_MSG(OK, 0, "回合数 6");
    START_GAME();
    ASSERT_TIMEOUT(CONTINUE);
    for (uint32_t i = 0; i < 4; ++i) {
        ASSERT_PRI_MSG(CONTINUE, 1, "攻击 B 25");
    }
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_SCORE(0, 0, 1, 1, 1);
}

GAME_TEST(5, max_round_civilian_failed)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 圣女 侦探 平民 内奸");
    ASSERT_PUB_MSG(OK, 0, "回合数 14");
    START_GAME();
    ASSERT_TIMEOUT(CONTINUE);
    for (uint32_t i = 0; i < 4; ++i) {
        ASSERT_PRI_MSG(CONTINUE, 0, "攻击 B 25");
        ASSERT_PRI_MSG(CONTINUE, 0, "攻击 C 25");
        ASSERT_PRI_MSG(CONTINUE, 0, "攻击 D 25");
    }
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_SCORE(1, 0, 0, 0, 1);
}

GAME_TEST(5, max_round_traitor_failed)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 圣女 侦探 平民 内奸");
    ASSERT_PUB_MSG(OK, 0, "回合数 6");
    START_GAME();
    ASSERT_TIMEOUT(CONTINUE);
    for (uint32_t i = 0; i < 4; ++i) {
        ASSERT_PRI_MSG(CONTINUE, 0, "攻击 E 25");
    }
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_SCORE(1, 1, 1, 1, 0);
}

GAME_TEST(5, ghost_still_limit_cure_after_death)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 圣女 侦探 灵媒 恶灵");
    START_GAME();
    ASSERT_TIMEOUT(CONTINUE);
    for (uint32_t i = 0; i < 4; ++i) {
        ASSERT_PRI_MSG(CONTINUE, 0, "攻击 E 25");
    }
    ASSERT_TIMEOUT(CONTINUE);
    for (uint32_t i = 0; i < 3; ++i) {
        ASSERT_PRI_MSG(CONTINUE, 4, "治愈 A 10");
    }
    ASSERT_PRI_MSG(FAILED, 4, "治愈 A 10");
}

GAME_TEST(5, ghost_is_exorcism_after_death_cant_act)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 圣女 侦探 灵媒 恶灵");
    START_GAME();
    ASSERT_TIMEOUT(CONTINUE);
    for (uint32_t i = 0; i < 4; ++i) {
        ASSERT_PRI_MSG(CONTINUE, 0, "攻击 E 25");
    }
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(CONTINUE, 3, "通灵 E");
    ASSERT_ELIMINATED(4);
}

GAME_TEST(5, ghost_is_detected_when_move_after_death_cant_act)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 圣女 侦探 灵媒 恶灵");
    ASSERT_PRI_MSG(OK, 0, "血量 15");
    START_GAME();
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(CONTINUE, 4, "pass");
    ASSERT_PRI_MSG(OK, 2, "侦查 E");
    ASSERT_PRI_MSG(CONTINUE, 4, "攻击 B 15"); // E is alive can still act
    ASSERT_PRI_MSG(OK, 2, "攻击 E 15"); // E is dead
    ASSERT_PRI_MSG(CONTINUE, 4, "pass");
    ASSERT_PRI_MSG(OK, 2, "侦查 E");
    ASSERT_PRI_MSG(CONTINUE, 4, "pass"); // is not detected action, can still act
    ASSERT_PRI_MSG(OK, 2, "pass");
    ASSERT_PRI_MSG(CONTINUE, 4, "pass");
    ASSERT_PRI_MSG(OK, 2, "侦查 E");
    ASSERT_PRI_MSG(CONTINUE, 4, "攻击 D 15"); // is detected action so cannot act anymore
    ASSERT_ELIMINATED(1);
    ASSERT_ELIMINATED(3);
    ASSERT_ELIMINATED(4);
}

GAME_TEST(5, ghost_cannot_exorcism_alive_roles)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 圣女 侦探 灵媒 恶灵");
    START_GAME();
    ASSERT_PRI_MSG(FAILED, 3, "通灵 A");
    ASSERT_PRI_MSG(FAILED, 3, "通灵 B");
    ASSERT_PRI_MSG(FAILED, 3, "通灵 C");
    ASSERT_PRI_MSG(FAILED, 3, "通灵 D");
    ASSERT_PRI_MSG(FAILED, 3, "通灵 E");
}

GAME_TEST(5, ghost_act_has_effect_when_exocrism)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 圣女 侦探 灵媒 恶灵");
    ASSERT_PRI_MSG(OK, 0, "血量 15");
    START_GAME();
    ASSERT_PRI_MSG(OK, 3, "pass");
    ASSERT_PRI_MSG(OK, 4, "攻击 E 15"); // E dead
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(OK, 3, "通灵 E");
    ASSERT_PRI_MSG(CONTINUE, 4, "攻击 B 15");
    ASSERT_ELIMINATED(1);
    ASSERT_ELIMINATED(4);
}

GAME_TEST(5, ghost_cannot_exocrism_twice)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 圣女 侦探 灵媒 恶灵");
    ASSERT_PRI_MSG(OK, 0, "血量 15");
    START_GAME();
    ASSERT_PRI_MSG(OK, 3, "pass");
    ASSERT_PRI_MSG(OK, 2, "攻击 C 15"); // C dead
    ASSERT_PRI_MSG(OK, 4, "攻击 E 15"); // E dead
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_ELIMINATED(2);
    ASSERT_PRI_MSG(OK, 3, "通灵 C");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(FAILED, 3, "通灵 E");
}

// It is a bug case that the ghost keeps taking it latest action before being exocrismed.
GAME_TEST(5, ghost_act_has_effect_when_exocrism_only_one_round)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 圣女 侦探 灵媒 恶灵");
    ASSERT_PRI_MSG(OK, 0, "血量 30");
    START_GAME();
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(CONTINUE, 4, "攻击 E 15");
    ASSERT_PRI_MSG(OK, 3, "pass");
    ASSERT_PRI_MSG(CONTINUE, 4, "攻击 E 15"); // E dead
    ASSERT_PRI_MSG(OK, 3, "通灵 E");
    ASSERT_PRI_MSG(CONTINUE, 4, "攻击 B 15");
    ASSERT_ELIMINATED(4);
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(CONTINUE, 1, "攻击 C 15"); // still alive
}

GAME_TEST(5, ghost_attact_sorcerer_ghost_not_be_hurted)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 圣女 侦探 灵媒 恶灵");
    ASSERT_PRI_MSG(OK, 0, "血量 15");
    START_GAME();
    ASSERT_PRI_MSG(OK, 3, "pass");
    ASSERT_PRI_MSG(OK, 4, "攻击 D 15"); // D dead
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_ELIMINATED(3);
    ASSERT_PRI_MSG(OK, 0, "攻击 E 15");
}

GAME_TEST(5, DISABLED_ghost_heavy_attact_hurt_self)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 圣女 侦探 灵媒 恶灵");
    ASSERT_PRI_MSG(OK, 0, "血量 25");
    START_GAME();
    ASSERT_PRI_MSG(OK, 3, "pass");
    ASSERT_PRI_MSG(OK, 4, "攻击 D 25"); // D E dead, but E can still act
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_ELIMINATED(3);
    ASSERT_PRI_MSG(FAILED, 0, "攻击 E 15");
    ASSERT_PRI_MSG(FAILED, 4, "攻击 A 25"); // cannot heavy attact when dead
    ASSERT_PRI_MSG(OK, 4, "攻击 A 15");
}

GAME_TEST(5, ghost_cannot_heavy_cure)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 圣女 侦探 灵媒 恶灵");
    ASSERT_PRI_MSG(OK, 0, "血量 15");
    START_GAME();
    ASSERT_PRI_MSG(FAILED, 4, "治愈 D 15");
}

GAME_TEST(5, assassin_hurt_little_blood)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 平民 平民 替身 刺客");
    ASSERT_PRI_MSG(OK, 0, "血量 15");
    START_GAME();
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(FAILED, 4, "攻击 B 1");
    ASSERT_PRI_MSG(FAILED, 4, "攻击 B 20");
    ASSERT_PRI_MSG(FAILED, 4, "攻击 B 25");
    ASSERT_PRI_MSG(CONTINUE, 4, "攻击 B 5");
    ASSERT_PRI_MSG(OK, 1, "pass"); // still alive
    ASSERT_PRI_MSG(CONTINUE, 4, "攻击 B 5");
    ASSERT_PRI_MSG(OK, 1, "pass"); // still alive
    ASSERT_PRI_MSG(CONTINUE, 4, "攻击 B 5");
    ASSERT_ELIMINATED(1);
}

GAME_TEST(5, assassin_hurt_zero_blood)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 平民 替身 替身 刺客");
    ASSERT_PRI_MSG(OK, 0, "血量 1");
    START_GAME();
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(CONTINUE, 4, "攻击 B 0");
    ASSERT_PRI_MSG(OK, 1, "pass"); // still alive
}

GAME_TEST(5, assassin_hurt_multi_blood)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 平民 圣女 侦探 刺客");
    ASSERT_PRI_MSG(OK, 0, "血量 5");
    START_GAME();
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(CHECKOUT, 4, "攻击 B C D 5");
    ASSERT_SCORE(1, 0, 0, 0, 1);
}

GAME_TEST(6, assassin_invalid_hurt)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 平民 圣女 侦探 刺客 平民");
    START_GAME();
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(FAILED, 4, "攻击 15");
    ASSERT_PRI_MSG(FAILED, 4, "攻击 A B C D E F 5");
    ASSERT_PRI_MSG(CONTINUE, 4, "攻击 A B C D E 5");
    ASSERT_PRI_MSG(FAILED, 4, "攻击 A B C 10");
    ASSERT_PRI_MSG(CONTINUE, 4, "攻击 A B 10");
    ASSERT_PRI_MSG(FAILED, 4, "攻击 A B 15");
    ASSERT_PRI_MSG(CONTINUE, 4, "攻击 A 15");
    ASSERT_PRI_MSG(FAILED, 4, "攻击 A B 0");
    ASSERT_PRI_MSG(CONTINUE, 4, "攻击 A 0");
}

GAME_TEST(5, non_assassin_can_only_hurt_one_role)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 平民 圣女 侦探 刺客");
    START_GAME();
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(FAILED, 0, "攻击 5");
    ASSERT_PRI_MSG(FAILED, 0, "攻击 A B 5");
}

GAME_TEST(5, DISABLED_assassin_cannot_hurt_guard_and_killer_team)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 恶灵 守卫 刺客");
    ASSERT_PRI_MSG(OK, 0, "血量 1");
    START_GAME();
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(CONTINUE, 4, "攻击 A 5");
    ASSERT_PRI_MSG(CONTINUE, 4, "攻击 B 5");
    ASSERT_PRI_MSG(CONTINUE, 4, "攻击 C 5");
    ASSERT_PRI_MSG(CONTINUE, 4, "攻击 D 5");
    ASSERT_PRI_MSG(OK, 0, "pass");
    ASSERT_PRI_MSG(OK, 1, "pass");
    ASSERT_PRI_MSG(OK, 2, "pass");
    ASSERT_PRI_MSG(OK, 3, "pass");
}

GAME_TEST(5, assassin_cannot_hurt_same_role)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 恶灵 守卫 刺客");
    ASSERT_PRI_MSG(OK, 0, "血量 1");
    START_GAME();
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(FAILED, 4, "攻击 A B B C D 5");
}

GAME_TEST(5, guard_invalid_shield_anti)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 恶灵 守卫 平民");
    START_GAME();
    ASSERT_PRI_MSG(FAILED, 3, "盾反");
    ASSERT_PRI_MSG(FAILED, 3, "盾反 A 100 B 100 C 100");
    ASSERT_PRI_MSG(FAILED, 3, "盾反 A 100 A 100");
}

GAME_TEST(5, guard_cannot_repeat_shield_anti_for_each_role)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 恶灵 守卫 平民");
    START_GAME();
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(CONTINUE, 3, "盾反 A 100 B 100");

    ASSERT_PRI_MSG(FAILED, 3, "盾反 A 100");
    ASSERT_PRI_MSG(FAILED, 3, "盾反 B 100");
    ASSERT_PRI_MSG(FAILED, 3, "盾反 A 100 B 100");
    ASSERT_PRI_MSG(FAILED, 3, "盾反 B 100 C 100");
    ASSERT_PRI_MSG(CONTINUE, 3, "盾反 C 100");

    ASSERT_PRI_MSG(FAILED, 3, "盾反 C 100");
    ASSERT_PRI_MSG(CONTINUE, 3, "盾反 A 100 B 100");

    ASSERT_PRI_MSG(CONTINUE, 3, "攻击 C 15");

    ASSERT_PRI_MSG(CONTINUE, 3, "盾反 A 100");
    ASSERT_TIMEOUT(CONTINUE);

    ASSERT_PRI_MSG(CONTINUE, 3, "盾反 A 100");
}

GAME_TEST(5, shield_anti_single_hurt)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 恶灵 守卫 平民");
    ASSERT_PRI_MSG(OK, 0, "血量 15");
    START_GAME();
    ASSERT_PRI_MSG(OK, 1, "攻击 D 15");
    ASSERT_PRI_MSG(OK, 4, "攻击 D 15");
    ASSERT_PRI_MSG(OK, 3, "盾反 D -15");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_ELIMINATED(1);
    ASSERT_ELIMINATED(4);
}

GAME_TEST(5, shield_anti_two_roles)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 恶灵 守卫 平民");
    ASSERT_PRI_MSG(OK, 0, "血量 15");
    START_GAME();
    ASSERT_PRI_MSG(OK, 1, "攻击 C 15");
    ASSERT_PRI_MSG(OK, 4, "攻击 D 15");
    ASSERT_PRI_MSG(OK, 3, "盾反 C 0 D 0");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_ELIMINATED(1);
    ASSERT_ELIMINATED(4);
}

GAME_TEST(5, shield_anti_hurt_self)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 恶灵 守卫 平民");
    ASSERT_PRI_MSG(OK, 0, "血量 15");
    START_GAME();
    ASSERT_PRI_MSG(OK, 1, "攻击 B 15");
    ASSERT_PRI_MSG(OK, 3, "盾反 B 0");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_ELIMINATED(1);
}

GAME_TEST(5, shield_anti_hurt_each_other)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 恶灵 守卫 平民");
    ASSERT_PRI_MSG(OK, 0, "血量 15");
    START_GAME();
    ASSERT_PRI_MSG(OK, 1, "攻击 E 15");
    ASSERT_PRI_MSG(OK, 4, "攻击 B 15");
    ASSERT_PRI_MSG(OK, 3, "盾反 B 0 E 0");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_ELIMINATED(1);
    ASSERT_ELIMINATED(4);
}

GAME_TEST(5, shield_anti_single_cure)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 恶灵 守卫 平民");
    ASSERT_PRI_MSG(OK, 0, "血量 25");
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "治愈 D 10");
    ASSERT_PRI_MSG(OK, 3, "盾反 D 35");
    ASSERT_TIMEOUT(CONTINUE); // D hp 25 -> 35
    ASSERT_PRI_MSG(OK, 0, "攻击 D 25");
    ASSERT_TIMEOUT(CONTINUE); // D hp 35 -> 10
    ASSERT_PRI_MSG(OK, 3, "pass");
}

GAME_TEST(5, shield_anti_takes_no_effects_when_blocked_hurt)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 平民 守卫 平民");
    ASSERT_PRI_MSG(OK, 0, "血量 15");
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "治愈 D 10");
    ASSERT_PRI_MSG(OK, 1, "挡刀 D");
    ASSERT_PRI_MSG(OK, 2, "攻击 D 15");
    ASSERT_PRI_MSG(OK, 3, "盾反 D 25");
    ASSERT_TIMEOUT(CONTINUE); // cure takes no effects
    ASSERT_ELIMINATED(1); // body double dead
    ASSERT_PRI_MSG(OK, 2, "攻击 D 15"); // C is alive
    ASSERT_PRI_MSG(OK, 3, "pass"); // D is alive, remains 25 hp
    ASSERT_TIMEOUT(CONTINUE); // cure takes no effects
    ASSERT_PRI_MSG(OK, 3, "pass"); // D is alive, remains 10 hp
}

GAME_TEST(5, shield_anti_hurt_cannot_be_blocked)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 平民 守卫 平民");
    ASSERT_PRI_MSG(OK, 0, "血量 15");
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "攻击 D 25");
    ASSERT_PRI_MSG(OK, 1, "挡刀 A");
    ASSERT_PRI_MSG(OK, 3, "盾反 D -10");
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_SCORE(0, 0, 1, 1, 1);
}

GAME_TEST(5, shield_anti_goddess_not_hurt)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 平民 守卫 圣女");
    ASSERT_PRI_MSG(OK, 0, "血量 15");
    START_GAME();
    ASSERT_PRI_MSG(OK, 4, "攻击 D 15");
    ASSERT_PRI_MSG(OK, 3, "盾反 D 15");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(OK, 4, "pass"); // E alive
    ASSERT_PRI_MSG(CONTINUE, 3, "pass"); // D alive
}

GAME_TEST(5, goddess_can_hurt_puppet)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 平民 守卫 圣女 人偶");
    ASSERT_PRI_MSG(OK, 0, "血量 15");
    START_GAME();
    ASSERT_PRI_MSG(OK, 4, "攻击 F 15");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(FAILED, 0, "攻击 F 15"); // puppet is dead
}

GAME_TEST(5, twin_cannot_attack_self_and_each_other)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 双子（邪） 双子（正） 平民 守卫");
    START_GAME();
    ASSERT_PRI_MSG(FAILED, 1, "攻击 B 15");
    ASSERT_PRI_MSG(FAILED, 2, "攻击 B 15");
    ASSERT_PRI_MSG(FAILED, 1, "攻击 C 15");
    ASSERT_PRI_MSG(FAILED, 2, "攻击 C 15");
    ASSERT_PRI_MSG(OK, 1, "攻击 D 15");
    ASSERT_PRI_MSG(OK, 2, "攻击 D 15");
}

GAME_TEST(5, killer_twin_change_team_if_one_dead_and_another_one_alive)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 双子（邪） 双子（正） 平民 守卫");
    ASSERT_PRI_MSG(OK, 0, "血量 15");
    START_GAME();
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(CONTINUE, 0, "攻击 C 15"); // civilian twin dead
    ASSERT_PRI_MSG(CHECKOUT, 0, "攻击 A 15"); // killer dead, civilian team wins
    ASSERT_SCORE(0, 1, 1, 1, 1);
}

GAME_TEST(5, civilian_twin_change_team_if_one_dead_and_another_one_alive)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 双子（邪） 双子（正） 平民 守卫");
    ASSERT_PRI_MSG(OK, 0, "血量 15");
    START_GAME();
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(CONTINUE, 0, "攻击 B 15"); // killer twin dead
    ASSERT_PRI_MSG(CHECKOUT, 0, "攻击 A 15"); // killer dead, civilian team wins
    ASSERT_SCORE(0, 0, 0, 1, 1);
}

GAME_TEST(5, twin_do_not_change_team_if_dead_at_the_same_time)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 双子（邪） 双子（正） 平民 守卫");
    ASSERT_PRI_MSG(OK, 0, "血量 15");
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "攻击 B 15"); // killer twin dead
    ASSERT_PRI_MSG(OK, 3, "攻击 C 15"); // civilian twin dead
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(OK, 0, "攻击 A 15"); // killer dead, civilian team wins
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_SCORE(0, 0, 1, 1, 1);
}

GAME_TEST(5, twin_change_team_from_the_next_round_on)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 双子（邪） 双子（正） 平民 守卫");
    ASSERT_PRI_MSG(OK, 0, "血量 15");
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "攻击 A 15"); // killer dead, civilian team wins
    ASSERT_PRI_MSG(OK, 3, "攻击 C 15"); // civilian twin dead
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_SCORE(0, 0, 1, 1, 1);
}

GAME_TEST(5, all_civilian_dead_killer_win)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 双子（邪） 双子（正） 平民 守卫");
    ASSERT_PRI_MSG(OK, 0, "血量 15");
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "攻击 D 15");
    ASSERT_PRI_MSG(OK, 1, "攻击 E 15");
    ASSERT_PRI_MSG(OK, 3, "攻击 B 15"); // civilian twin become killer team, no alive civilian team members any more
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_SCORE(1, 1, 0, 0, 0);
}

GAME_TEST(5, witch_cannot_be_with_guard)
{
    ASSERT_PUB_MSG(OK, 0, "五人身份 杀手 魔女 守卫 平民 平民");
    ASSERT_FALSE(StartGame());
}

GAME_TEST(5, only_witch_can_curse)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 魔女 骑士 平民 平民");
    START_GAME();
    ASSERT_PRI_MSG(FAILED, 0, "诅咒 A 5");
}

GAME_TEST(5, witch_invalid_curse)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 魔女 骑士 平民 平民");
    START_GAME();
    ASSERT_PRI_MSG(FAILED, 1, "诅咒 A 6");
}

GAME_TEST(5, curse_do_not_hurt_when_target_takes_no_actions_the_first_round)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 魔女 骑士 平民 平民");
    ASSERT_PRI_MSG(OK, 0, "血量 5");
    START_GAME();
    ASSERT_PRI_MSG(OK, 1, "诅咒 A 5");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_TIMEOUT(CONTINUE);
}

GAME_TEST(5, curse_do_not_hurt_when_target_takes_no_actions)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 魔女 骑士 平民 平民");
    ASSERT_PRI_MSG(OK, 0, "血量 10");
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "治愈 B 10");
    ASSERT_PRI_MSG(OK, 1, "诅咒 A 5");
    ASSERT_TIMEOUT(CONTINUE); // -5
    ASSERT_TIMEOUT(CONTINUE);
}

GAME_TEST(5, witch_cannot_attact)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 魔女 骑士 平民 平民");
    START_GAME();
    ASSERT_PRI_MSG(FAILED, 1, "攻击 A 15");
    ASSERT_PRI_MSG(FAILED, 1, "攻击 A 0");
}

GAME_TEST(5, witch_make_role_curse)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 魔女 骑士 平民 平民");
    ASSERT_PRI_MSG(OK, 0, "血量 15");
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "治愈 B 10");
    ASSERT_PRI_MSG(OK, 1, "诅咒 A 5");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(OK, 0, "治愈 B 10");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(CHECKOUT, 0, "治愈 B 10");
    ASSERT_SCORE(0, 0, 1, 1, 1);
}

GAME_TEST(5, double_curse)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 魔女 魔女 骑士 平民");
    ASSERT_PRI_MSG(OK, 0, "血量 20");
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "治愈 B 10");
    ASSERT_PRI_MSG(OK, 1, "诅咒 A 5");
    ASSERT_PRI_MSG(OK, 2, "诅咒 A 5");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(OK, 0, "治愈 B 10");
    ASSERT_TIMEOUT(CHECKOUT); // -10
    ASSERT_SCORE(0, 0, 0, 1, 1);
}

GAME_TEST(5, curse_do_not_hurt_after_attacked)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 魔女 骑士 平民 平民");
    ASSERT_PRI_MSG(OK, 0, "血量 30");
    ASSERT_PRI_MSG(OK, 0, "治愈次数 10");
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "治愈 B 10");
    ASSERT_PRI_MSG(OK, 1, "诅咒 A 5");
    ASSERT_TIMEOUT(CONTINUE); // -5
    ASSERT_PRI_MSG(OK, 0, "治愈 B 10");
    ASSERT_PRI_MSG(OK, 2, "攻击 A 15"); // -15
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(OK, 0, "治愈 B 10");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(CONTINUE, 0, "治愈 B 10");
    ASSERT_FINISHED(false);
}

GAME_TEST(5, curse_do_not_hurt_after_goddess_attacked)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 魔女 骑士 圣女 平民");
    ASSERT_PRI_MSG(OK, 0, "血量 15");
    ASSERT_PRI_MSG(OK, 0, "治愈次数 10");
    START_GAME();
    ASSERT_PRI_MSG(OK, 4, "治愈 B 10");
    ASSERT_PRI_MSG(OK, 1, "诅咒 E 5");
    ASSERT_TIMEOUT(CONTINUE); // -5
    ASSERT_PRI_MSG(OK, 4, "治愈 B 10");
    ASSERT_PRI_MSG(OK, 3, "攻击 E 15"); // 0
    ASSERT_TIMEOUT(CONTINUE); // -5
    ASSERT_PRI_MSG(OK, 4, "治愈 B 10");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(CONTINUE, 4, "治愈 B 10");
    ASSERT_PRI_MSG(CONTINUE, 4, "治愈 B 10");
}

GAME_TEST(5, curse_cannot_be_cancel_when_curse_be_concurrent_with_attack)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 魔女 骑士 圣女 平民");
    ASSERT_PRI_MSG(OK, 0, "血量 25");
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "治愈 B 10");
    ASSERT_PRI_MSG(OK, 1, "诅咒 A 5"); // -5
    ASSERT_PRI_MSG(OK, 3, "攻击 A 15"); // -15
    ASSERT_TIMEOUT(CONTINUE); // -5
    ASSERT_PRI_MSG(OK, 0, "治愈 B 10");
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_SCORE(0, 0, 1, 1, 1);
}

GAME_TEST(5, curse_hurt_when_cure)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 魔女 骑士 平民 平民");
    ASSERT_PRI_MSG(OK, 0, "血量 10");
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "治愈 A 10"); // +10
    ASSERT_PRI_MSG(OK, 1, "诅咒 A 5"); // -5
    ASSERT_PRI_MSG(OK, 2, "攻击 A 15"); // -15
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_SCORE(0, 0, 1, 1, 1);
}

GAME_TEST(5, DISABLED_curse_cannot_shield_anti)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 魔女 守卫 平民 平民");
    ASSERT_PRI_MSG(OK, 0, "血量 5");
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "治愈 B 10");
    ASSERT_PRI_MSG(OK, 1, "诅咒 A 5");
    ASSERT_PRI_MSG(OK, 2, "盾反 A 0");
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_SCORE(0, 0, 1, 1, 1);
}

GAME_TEST(5, DISABLED_shield_anti_settlement_after_curse)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 魔女 守卫 平民 平民");
    ASSERT_PRI_MSG(OK, 0, "血量 15");
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "治愈 B 10");
    ASSERT_PRI_MSG(OK, 1, "诅咒 A 5");
    ASSERT_PRI_MSG(OK, 2, "盾反 A -5");
    ASSERT_PRI_MSG(OK, 3, "攻击 A 15");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_ELIMINATED(3);
}

GAME_TEST(5, curse_cannot_be_cancel_when_curse_be_concurrent_with_attack_but_old_curse_can_be_cancel)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 魔女 骑士 圣女 平民");
    ASSERT_PRI_MSG(OK, 0, "血量 30");
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "治愈 B 10");
    ASSERT_PRI_MSG(OK, 1, "诅咒 A 5");
    ASSERT_TIMEOUT(CONTINUE); // -5
    ASSERT_PRI_MSG(OK, 0, "治愈 B 10");
    ASSERT_PRI_MSG(OK, 3, "攻击 A 15"); // -15
    ASSERT_PRI_MSG(CONTINUE, 1, "诅咒 A 5"); // -5
    ASSERT_PRI_MSG(OK, 0, "治愈 B 10");
    ASSERT_TIMEOUT(CHECKOUT); // -5
    ASSERT_SCORE(0, 0, 1, 1, 1);
}

GAME_TEST(5, witch_curse_cannot_be_blocked)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 魔女 替身 平民 平民");
    ASSERT_PRI_MSG(OK, 0, "血量 15");
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "治愈 B 10");
    ASSERT_PRI_MSG(OK, 1, "诅咒 A 5");
    ASSERT_PRI_MSG(OK, 2, "挡刀 A");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(OK, 0, "治愈 B 10");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(CHECKOUT, 0, "治愈 B 10");
    ASSERT_SCORE(0, 0, 0, 1, 1);
}

GAME_TEST(5, DISABLED_witch_curse_cannot_be_shield_anti)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 魔女 守卫 平民 平民");
    ASSERT_PRI_MSG(OK, 0, "血量 15");
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "治愈 B 10");
    ASSERT_PRI_MSG(OK, 1, "诅咒 A 5");
    ASSERT_PRI_MSG(OK, 2, "盾反 A 10");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(OK, 0, "治愈 B 10");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(CHECKOUT, 0, "治愈 B 10");
    ASSERT_SCORE(0, 0, 1, 1, 1);
}

GAME_TEST(5, knight_can_attact_self)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 魔女 骑士 平民 平民");
    ASSERT_PRI_MSG(OK, 0, "血量 15");
    START_GAME();
    ASSERT_PRI_MSG(OK, 2, "攻击 C 15");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_ELIMINATED(2);
}

GAME_TEST(5, others_cannot_hurt_knight_when_they_attack_each_other)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 魔女 骑士 平民 平民");
    ASSERT_PRI_MSG(OK, 0, "血量 15");
    START_GAME();
    ASSERT_PRI_MSG(OK, 2, "攻击 E 15");
    ASSERT_PRI_MSG(OK, 4, "攻击 C 15");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(CONTINUE, 2, "pass");
    ASSERT_ELIMINATED(4);
}

GAME_TEST(5, knight_attack_each_other_can_cancel_curse)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 魔女 骑士 平民 平民");
    ASSERT_PRI_MSG(OK, 0, "血量 10");
    START_GAME();
    ASSERT_PRI_MSG(OK, 2, "治愈 B 10");
    ASSERT_PRI_MSG(OK, 1, "诅咒 C 5");
    ASSERT_TIMEOUT(CONTINUE); // C - 5
    ASSERT_PRI_MSG(OK, 2, "攻击 E 15");
    ASSERT_PRI_MSG(OK, 4, "攻击 C 15");
    ASSERT_TIMEOUT(CONTINUE); // C - 0
    ASSERT_PRI_MSG(CONTINUE, 2, "pass");
    ASSERT_ELIMINATED(4);
}

GAME_TEST(5, agent_should_not_be_with_traitor)
{
    ASSERT_PUB_MSG(OK, 0, "五人身份 杀手 替身 平民 内奸 特工");
    ASSERT_FALSE(StartGame());
}

GAME_TEST(5, agent_cannot_attack_directly)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 平民 平民 特工");
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(FAILED, 4, "攻击 A 15");
}

GAME_TEST(5, more_than_one_agent_win)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 平民 特工 特工");
    ASSERT_PRI_MSG(OK, 0, "血量 5");
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(OK, 3, "蓄力 A 5 C 5");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(CHECKOUT, 3, "释放 A C");
    ASSERT_SCORE(0, 0, 0, 1, 1);
}

GAME_TEST(5, agent_release_part_of_damages_to_roles)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 平民 特工 特工");
    ASSERT_PRI_MSG(OK, 0, "血量 5");
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(OK, 3, "蓄力 A 5 C 5");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(CHECKOUT, 3, "释放 A");
    ASSERT_SCORE(0, 0, 1, 0, 0);
}

GAME_TEST(5, agent_accumulate_damages)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 侦探 特工 特工");
    ASSERT_PRI_MSG(OK, 0, "血量 20");
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(OK, 3, "蓄力 A 15 C 5");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(CONTINUE, 3, "蓄力 A 5 C 15");
    ASSERT_PRI_MSG(OK, 2, "侦查 D");
    ASSERT_PRI_MSG(CHECKOUT, 3, "释放 A C");
    ASSERT_SCORE(0, 0, 0, 1, 1);
}

GAME_TEST(5, agent_cannot_release_without_tokens)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 平民 平民 特工");
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(FAILED, 4, "释放");
}

GAME_TEST(5, agent_cannot_release_without_assign)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 平民 平民 特工");
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(FAILED, 4, "释放 A");
}

GAME_TEST(5, agent_cannot_release_with_some_of_unassign)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 平民 平民 特工");
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(OK, 4, "蓄力 A 5 C 15");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(FAILED, 4, "释放 A D");
}

GAME_TEST(5, agent_invalid_assignments)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 平民 平民 特工");
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(FAILED, 4, "蓄力 A 5 C 15 D 5");
    ASSERT_PRI_MSG(FAILED, 4, "蓄力 A 5 B 5 A 5");
    ASSERT_PRI_MSG(NOT_FOUND, 4, "蓄力 A 0");
    ASSERT_PRI_MSG(FAILED, 4, "蓄力");
    ASSERT_PRI_MSG(NOT_FOUND, 4, "蓄力 A 20");
    ASSERT_PRI_MSG(FAILED, 4, "蓄力 A 11");
}

GAME_TEST(5, agent_cannot_double_release)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 平民 平民 特工");
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(OK, 4, "蓄力 A 5 C 15");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(CONTINUE, 4, "释放 A");
    ASSERT_PRI_MSG(FAILED, 4, "释放 A");
}

GAME_TEST(5, agent_release_can_be_blocked)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 平民 平民 特工");
    ASSERT_PRI_MSG(OK, 0, "血量 5");
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(OK, 4, "蓄力 A 5");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(OK, 1, "挡刀 A");
    ASSERT_PRI_MSG(CONTINUE, 4, "释放 A");
    ASSERT_ELIMINATED(1); // B is dead
}

GAME_TEST(5, agent_release_can_be_sheild_anti)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 侦探 守卫 特工");
    ASSERT_PRI_MSG(OK, 0, "血量 20");
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(OK, 4, "蓄力 A 5 C 15");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(OK, 2, "侦查 E");
    ASSERT_PRI_MSG(OK, 3, "盾反 A 15 C 5");
    ASSERT_PRI_MSG(CONTINUE, 4, "释放 C A");
    ASSERT_ELIMINATED(4); // B is dead
}

GAME_TEST(5, dead_agent_win)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 平民 特工 特工");
    ASSERT_PRI_MSG(OK, 0, "血量 15");
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(OK, 0, "攻击 E 15");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_ELIMINATED(4); // E is dead
    ASSERT_PRI_MSG(OK, 2, "攻击 A 15");
    ASSERT_PRI_MSG(CHECKOUT, 0, "攻击 C 15");
    ASSERT_SCORE(0, 0, 0, 1, 1);
}

GAME_TEST(5, goddess_can_hurt_agent)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 平民 圣女 特工");
    ASSERT_PRI_MSG(OK, 0, "血量 15");
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(OK, 3, "攻击 E 15");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_ELIMINATED(4); // E is dead
}

GAME_TEST(5, agent_achieve_max_round_lose)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 平民 圣女 特工");
    ASSERT_PRI_MSG(OK, 0, "回合数 1");
    ASSERT_TRUE(StartGame());
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_SCORE(1, 1, 1, 1, 0);
}

GAME_TEST(5, cannot_good_night_without_good_night_mode)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 平民 圣女 侦探");
    ASSERT_PUB_MSG(OK, 0, "晚安模式 关闭");
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(FAILED, 3, "晚安");
}

GAME_TEST(5, good_night_mode_cannot_act_after_good_night)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 平民 圣女 侦探");
    ASSERT_PUB_MSG(OK, 0, "晚安模式 开启");
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(OK, 3, "晚安");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_ELIMINATED(3);
}

GAME_TEST(5, good_night_mode_killer_cannot_good_night)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 平民 圣女 侦探");
    ASSERT_PUB_MSG(OK, 0, "晚安模式 开启");
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(FAILED, 0, "晚安");
}

GAME_TEST(5, good_night_mode_civilians_cannot_win_until_quorum_good_night)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 平民 圣女 侦探");
    ASSERT_PUB_MSG(OK, 0, "晚安模式 开启");
    ASSERT_PUB_MSG(OK, 0, "血量 15");
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(OK, 0, "攻击 A 15");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(CONTINUE, 2, "晚安");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(CHECKOUT, 3, "晚安");
    ASSERT_SCORE(0, 0, 1, 1, 1);
}

GAME_TEST(5, good_night_mode_civilians_cannot_win_until_quorum_good_night_by_killed)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 杀手 替身 平民 圣女 侦探");
    ASSERT_PUB_MSG(OK, 0, "晚安模式 开启");
    ASSERT_PUB_MSG(OK, 0, "血量 15");
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(OK, 0, "攻击 A 15");
    ASSERT_PRI_MSG(OK, 2, "晚安");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(CONTINUE, 1, "攻击 D 15");
    ASSERT_PRI_MSG(OK, 4, "攻击 E 15");
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_SCORE(0, 0, 1, 1, 1);
}


} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    return RUN_ALL_TESTS();
}
