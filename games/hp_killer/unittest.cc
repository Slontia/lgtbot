// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

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
}

GAME_TEST(5, killer_win)
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

GAME_TEST(5, body_double_protect_killer)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 替身 杀手 平民 圣女 侦探");
    ASSERT_PUB_MSG(OK, 0, "血量 75");
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "挡刀");
    ASSERT_PRI_MSG(OK, 1, "攻击 B 25");
    ASSERT_PRI_MSG(OK, 2, "攻击 B 15");
    ASSERT_PRI_MSG(OK, 3, "攻击 B 15");
    ASSERT_PRI_MSG(CONTINUE, 4, "攻击 B 15");
    ASSERT_PRI_MSG(OK, 1, "攻击 A 15");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_ELIMINATED(0);
}

GAME_TEST(5, goddess_can_not_hurt_civilian)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 替身 杀手 平民 圣女 侦探");
    ASSERT_PUB_MSG(OK, 0, "血量 15");
    START_GAME();
    ASSERT_PRI_MSG(OK, 3, "攻击 C 15");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(CONTINUE, 3, "pass");
    ASSERT_PRI_MSG(CONTINUE, 3, "攻击 D 15");
    ASSERT_PRI_MSG(CONTINUE, 3, "pass");
    ASSERT_PRI_MSG(CONTINUE, 3, "攻击 E 15");
    ASSERT_PRI_MSG(CONTINUE, 3, "pass");
    ASSERT_PRI_MSG(CHECKOUT, 3, "攻击 B 15");
    ASSERT_SCORE(0, 0, 1, 1, 1);
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

GAME_TEST(5, detective_cant_repeat_detect)
{
    ASSERT_PUB_MSG(OK, 0, "身份列表 替身 杀手 平民 圣女 侦探");
    START_GAME();
    ASSERT_PRI_MSG(OK, 4, "侦查 A");
    ASSERT_TIMEOUT(CONTINUE);
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
    ASSERT_PUB_MSG(OK, 0, "身份列表 替身 杀手 平民 平民 内奸");
    START_GAME();
    ASSERT_TIMEOUT(CONTINUE);
    for (uint32_t i = 0; i < 4; ++i) {
        ASSERT_PRI_MSG(CONTINUE, 1, "攻击 C 25");
        ASSERT_PRI_MSG(CONTINUE, 1, "攻击 D 25");
        ASSERT_PRI_MSG(CONTINUE, 1, "攻击 E 25");
    }
    ASSERT_FINISHED(false);
}


// TODO: game 20 round finish


int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    return RUN_ALL_TESTS();
}
