// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

GAME_TEST(1, player_not_enough)
{
    ASSERT_FALSE(StartGame());
}

GAME_TEST(3, cannot_public_act)
{
    START_GAME();
    ASSERT_PUB_MSG(FAILED, 0, "check");
}

GAME_TEST(5, check_bet_chip_valid)
{
    ASSERT_PUB_MSG(OK, 0, "局数 1");
    ASSERT_PUB_MSG(OK, 0, "筹码 100");
    ASSERT_PUB_MSG(OK, 0, "种子 ABC");
    ASSERT_PUB_MSG(OK, 0, "底注变化 5 10");
    ASSERT_PUB_MSG(OK, 0, "底注变化局数 2");
    START_GAME();

    ASSERT_PRI_MSG(NOT_FOUND, 0, "bet 0"); // should not be zero
    ASSERT_PRI_MSG(FAILED, 0, "bet 1"); // must be multiple of bet_chips
    ASSERT_PRI_MSG(FAILED, 0, "bet 100"); // no enough chips
    ASSERT_PRI_MSG(OK, 0, "bet 5");
    ASSERT_PRI_MSG(OK, 1, "bet 10");
    ASSERT_PRI_MSG(OK, 2, "bet 95"); // all-in is ok
}

GAME_TEST(5, bet_same_is_consensus)
{
    ASSERT_PUB_MSG(OK, 0, "局数 1");
    ASSERT_PUB_MSG(OK, 0, "筹码 100");
    ASSERT_PUB_MSG(OK, 0, "种子 ABC");
    ASSERT_PUB_MSG(OK, 0, "底注变化 5 10");
    ASSERT_PUB_MSG(OK, 0, "底注变化局数 2");
    START_GAME();

    for (uint32_t i = 0; i < 4; ++i) { // 4 rounds, each chips = 5 + 10 * 4 = 45
        for (uint64_t pid = 0; pid < 4; ++pid) {
            ASSERT_PRI_MSG(OK, pid, "bet 10");
        }
        ASSERT_PRI_MSG(CHECKOUT, 4, "bet 10");
    }
    ASSERT_SCORE(280, 55, 55, 55, 55);
}

#define ALL_CHECK_FOR_ONE_GAME() \
do { \
    for (uint32_t i = 0; i < 4; ++i) { \
        for (uint64_t pid = 0; pid < options_.generic_options_.PlayerNum() - 1; ++pid) { \
            ASSERT_PRI_MSG(OK, pid, "check"); \
        } \
        ASSERT_PRI_MSG(CHECKOUT, options_.generic_options_.PlayerNum() - 1, "check"); \
    } \
} while (0)

GAME_TEST(5, check_same_is_consensus)
{
    ASSERT_PUB_MSG(OK, 0, "局数 1");
    ASSERT_PUB_MSG(OK, 0, "筹码 100");
    ASSERT_PUB_MSG(OK, 0, "种子 ABC");
    ASSERT_PUB_MSG(OK, 0, "底注变化 5 10");
    ASSERT_PUB_MSG(OK, 0, "底注变化局数 2");
    START_GAME();

    ALL_CHECK_FOR_ONE_GAME();
    ASSERT_SCORE(120, 95, 95, 95, 95);
}

GAME_TEST(3, all_fold_except_one_will_finish_game)
{
    ASSERT_PUB_MSG(OK, 0, "局数 1");
    ASSERT_PUB_MSG(OK, 0, "筹码 100");
    ASSERT_PUB_MSG(OK, 0, "种子 ABC");
    ASSERT_PUB_MSG(OK, 0, "底注变化 5 10");
    ASSERT_PUB_MSG(OK, 0, "底注变化局数 2");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "bet 10"); // 5 -> 15
    ASSERT_PRI_MSG(OK, 1, "bet 20"); // 5 -> 25
    ASSERT_PRI_MSG(CHECKOUT, 2, "check"); // 5

    ASSERT_PRI_MSG(OK, 0, "raise 20"); // 15 -> 45
    ASSERT_PRI_MSG(CHECKOUT, 2, "fold");

    ASSERT_PRI_MSG(FAILED, 0, "fold");
    ASSERT_PRI_MSG(CHECKOUT, 1, "fold");

    ASSERT_SCORE(130, 75, 95);
}

GAME_TEST(3, all_allin_except_raise_highest_will_finish_game)
{
    ASSERT_PUB_MSG(OK, 0, "局数 2");
    ASSERT_PUB_MSG(OK, 0, "筹码 100");
    ASSERT_PUB_MSG(OK, 0, "种子 ABC");
    ASSERT_PUB_MSG(OK, 0, "底注变化 5 10");
    ASSERT_PUB_MSG(OK, 0, "底注变化局数 2");
    ASSERT_PUB_MSG(OK, 0, "卡牌 扑克");
    START_GAME();

    ALL_CHECK_FOR_ONE_GAME(); // chips = {115, 95, 95}

    ASSERT_PRI_MSG(OK, 0, "bet 100"); // 5 -> 105
    ASSERT_PRI_MSG(OK, 1, "allin"); // 5 -> 95
    ASSERT_PRI_MSG(CHECKOUT, 2, "allin"); // 5 -> 95

    ASSERT_SCORE(300, 0, 0);
}

GAME_TEST(3, all_allin_or_fold_except_raise_highest_will_finish_game)
{
    ASSERT_PUB_MSG(OK, 0, "局数 2");
    ASSERT_PUB_MSG(OK, 0, "筹码 100");
    ASSERT_PUB_MSG(OK, 0, "种子 ABC");
    ASSERT_PUB_MSG(OK, 0, "底注变化 5 10");
    ASSERT_PUB_MSG(OK, 0, "底注变化局数 2");
    ASSERT_PUB_MSG(OK, 0, "卡牌 扑克");
    START_GAME();

    ALL_CHECK_FOR_ONE_GAME(); // chips = {115, 95, 95}

    ASSERT_PRI_MSG(OK, 0, "bet 100"); // 5 -> 105
    ASSERT_PRI_MSG(OK, 1, "allin"); // 5 -> 95
    ASSERT_PRI_MSG(CHECKOUT, 2, "check"); // 5
                                          //
    ASSERT_PRI_MSG(CHECKOUT, 2, "fold"); // 5

    ASSERT_SCORE(210, 0, 90);
}

GAME_TEST(3, cannot_act_after_fold)
{
    ASSERT_PUB_MSG(OK, 0, "局数 1");
    ASSERT_PUB_MSG(OK, 0, "筹码 100");
    ASSERT_PUB_MSG(OK, 0, "种子 ABC");
    ASSERT_PUB_MSG(OK, 0, "底注变化 5 10");
    ASSERT_PUB_MSG(OK, 0, "底注变化局数 2");
    START_GAME();

    // preflop
    ASSERT_PRI_MSG(OK, 0, "check");
    ASSERT_PRI_MSG(OK, 1, "bet 10");
    ASSERT_PRI_MSG(CHECKOUT, 2, "check");

    ASSERT_PRI_MSG(OK, 0, "raise 10");
    ASSERT_PRI_MSG(CHECKOUT, 2, "fold");

    ASSERT_PRI_MSG(FAILED, 2, "call");
    ASSERT_PRI_MSG(CHECKOUT, 1, "call");

    // flop
    ASSERT_PRI_MSG(FAILED, 2, "check");
}

GAME_TEST(5, check_when_timeout)
{
    ASSERT_PUB_MSG(OK, 0, "局数 1");
    ASSERT_PUB_MSG(OK, 0, "筹码 100");
    ASSERT_PUB_MSG(OK, 0, "种子 ABC");
    ASSERT_PUB_MSG(OK, 0, "底注变化 5 10");
    ASSERT_PUB_MSG(OK, 0, "底注变化局数 2");
    START_GAME();

    for (uint64_t pid = 1; pid < options_.generic_options_.PlayerNum(); ++pid) {
        ASSERT_PRI_MSG(OK, pid, "check");
    }
    ASSERT_TIMEOUT(CHECKOUT);

    for (uint32_t i = 0; i < 3; ++i) {
        for (uint64_t pid = 1; pid < options_.generic_options_.PlayerNum() - 1; ++pid) {
            ASSERT_PRI_MSG(OK, pid, "check");
        }
        ASSERT_PRI_MSG(CHECKOUT, options_.generic_options_.PlayerNum() - 1, "check");
    }

    ASSERT_SCORE(120, 95, 95, 95, 95);
}

GAME_TEST(5, check_when_timeout_and_then_fold)
{
    ASSERT_PUB_MSG(OK, 0, "局数 1");
    ASSERT_PUB_MSG(OK, 0, "筹码 100");
    ASSERT_PUB_MSG(OK, 0, "种子 ABC");
    ASSERT_PUB_MSG(OK, 0, "底注变化 5 10");
    ASSERT_PUB_MSG(OK, 0, "底注变化局数 2");
    ASSERT_PUB_MSG(OK, 0, "卡牌 扑克");
    START_GAME();

    // preflop
    for (uint64_t pid = 2; pid < options_.generic_options_.PlayerNum(); ++pid) {
        ASSERT_PRI_MSG(OK, pid, "check");
    }
    ASSERT_PRI_MSG(OK, 1, "bet 5"); // 5 -> 10
    ASSERT_TIMEOUT(CHECKOUT);

    for (uint64_t pid = 3; pid < options_.generic_options_.PlayerNum(); ++pid) {
        ASSERT_PRI_MSG(OK, pid, "call");
    }
    ASSERT_PRI_MSG(CHECKOUT, 2, "call");

    // flop, turn, river
    for (uint32_t i = 0; i < 3; ++i) {
        for (uint64_t pid = 1; pid < options_.generic_options_.PlayerNum() - 1; ++pid) {
            ASSERT_PRI_MSG(OK, pid, "check");
        }
        ASSERT_PRI_MSG(CHECKOUT, options_.generic_options_.PlayerNum() - 1, "check");
    }

    ASSERT_SCORE(95, 90, 135, 90, 90);
}

GAME_TEST(5, fold_when_timeout)
{
    ASSERT_PUB_MSG(OK, 0, "局数 1");
    ASSERT_PUB_MSG(OK, 0, "筹码 100");
    ASSERT_PUB_MSG(OK, 0, "种子 ABC");
    ASSERT_PUB_MSG(OK, 0, "底注变化 5 10");
    ASSERT_PUB_MSG(OK, 0, "底注变化局数 2");
    ASSERT_PUB_MSG(OK, 0, "卡牌 扑克");
    START_GAME();

    // preflop
    for (uint64_t pid = 0; pid < options_.generic_options_.PlayerNum() - 1; ++pid) {
        ASSERT_PRI_MSG(OK, pid, "check");
    }
    ASSERT_PRI_MSG(CHECKOUT, options_.generic_options_.PlayerNum() - 1, "bet 10"); // 5 -> 15

    for (uint64_t pid = 1; pid < options_.generic_options_.PlayerNum() - 1; ++pid) {
        ASSERT_PRI_MSG(OK, pid, "call");
    }
    ASSERT_TIMEOUT(CHECKOUT); // player 0 timeout, fold

    // flop, turn, river
    for (uint32_t i = 0; i < 3; ++i) {
        for (uint64_t pid = 1; pid < options_.generic_options_.PlayerNum() - 1; ++pid) {
            ASSERT_PRI_MSG(OK, pid, "check");
        }
        ASSERT_PRI_MSG(CHECKOUT, options_.generic_options_.PlayerNum() - 1, "check");
    }

    ASSERT_SCORE(95, 85, 150, 85, 85);
}

GAME_TEST(5, allin_when_cant_afford_base_chips)
{
    ASSERT_PUB_MSG(OK, 0, "局数 2");
    ASSERT_PUB_MSG(OK, 0, "筹码 100");
    ASSERT_PUB_MSG(OK, 0, "种子 ABC");
    ASSERT_PUB_MSG(OK, 0, "底注变化 5 100");
    ASSERT_PUB_MSG(OK, 0, "底注变化局数 1");
    ASSERT_PUB_MSG(OK, 0, "卡牌 扑克");
    START_GAME();

    // preflop -- bet
    for (uint64_t pid = 0; pid < options_.generic_options_.PlayerNum() - 1; ++pid) {
        ASSERT_PRI_MSG(OK, pid, "check");
    }
    ASSERT_PRI_MSG(CHECKOUT, options_.generic_options_.PlayerNum() - 1, "bet 10"); // 5 -> 15

    // preflop -- raise
    for (uint64_t pid = 1; pid < options_.generic_options_.PlayerNum() - 1; ++pid) {
        ASSERT_PRI_MSG(OK, pid, "call");
    }
    ASSERT_PRI_MSG(CHECKOUT, 0, "fold"); // player 0 timeout, fold

    // flop, turn, river
    for (uint32_t i = 0; i < 3; ++i) {
        for (uint64_t pid = 1; pid < options_.generic_options_.PlayerNum() - 1; ++pid) {
            ASSERT_PRI_MSG(OK, pid, "check");
        }
        ASSERT_PRI_MSG(CHECKOUT, options_.generic_options_.PlayerNum() - 1, "check");
    }

    // round 1 scores: 95, 85, 150, 85, 85

    ASSERT_SCORE(445, 0, 55, 0, 0);
}

GAME_TEST(5, raise_must_be_greater_than_raise_bet)
{
    ASSERT_PUB_MSG(OK, 0, "局数 1");
    ASSERT_PUB_MSG(OK, 0, "筹码 100");
    ASSERT_PUB_MSG(OK, 0, "种子 ABC");
    ASSERT_PUB_MSG(OK, 0, "底注变化 5 100");
    ASSERT_PUB_MSG(OK, 0, "底注变化局数 1");
    START_GAME();

    // preflop -- bet
    for (uint64_t pid = 0; pid < options_.generic_options_.PlayerNum() - 1; ++pid) {
        ASSERT_PRI_MSG(OK, pid, "check");
    }
    ASSERT_PRI_MSG(CHECKOUT, options_.generic_options_.PlayerNum() - 1, "bet 5"); // 5 -> 10

    // preflop -- raise
    ASSERT_PRI_MSG(FAILED, 0, "raise 4");
    ASSERT_PRI_MSG(OK, 0, "raise 5"); // 5 -> 15
}

GAME_TEST(5, raise_can_be_less_than_raise_bet_when_allin)
{
    ASSERT_PUB_MSG(OK, 0, "局数 1");
    ASSERT_PUB_MSG(OK, 0, "筹码 12");
    ASSERT_PUB_MSG(OK, 0, "种子 ABC");
    ASSERT_PUB_MSG(OK, 0, "底注变化 5 100");
    ASSERT_PUB_MSG(OK, 0, "底注变化局数 1");
    START_GAME();

    // preflop -- bet
    for (uint64_t pid = 0; pid < options_.generic_options_.PlayerNum() - 1; ++pid) {
        ASSERT_PRI_MSG(OK, pid, "check");
    }
    ASSERT_PRI_MSG(CHECKOUT, options_.generic_options_.PlayerNum() - 1, "bet 5"); // 5 -> 10

    // preflop -- raise
    ASSERT_PRI_MSG(FAILED, 0, "raise 1");
    ASSERT_PRI_MSG(OK, 0, "raise 2"); // allin 5 -> 12
}

GAME_TEST(2, one_of_two_players_allin_open_card_directly)
{
    ASSERT_PUB_MSG(OK, 0, "局数 2");
    ASSERT_PUB_MSG(OK, 0, "筹码 100");
    ASSERT_PUB_MSG(OK, 0, "种子 ABC");
    ASSERT_PUB_MSG(OK, 0, "底注变化 50 5");
    ASSERT_PUB_MSG(OK, 0, "底注变化局数 1");
    ASSERT_PUB_MSG(OK, 0, "卡牌 扑克");
    START_GAME();

    ALL_CHECK_FOR_ONE_GAME();

    // round 1 scores: 150, 50

    // preflop -- bet
    ASSERT_PRI_MSG(OK, 0, "check");
    ASSERT_PRI_MSG(CHECKOUT, 1, "bet 10"); // 5 -> 15

    // preflop -- raise 1
    ASSERT_PRI_MSG(CHECKOUT, 0, "raise 50"); // 5 -> 65

    // preflop -- raise 2
    ASSERT_PRI_MSG(CHECKOUT, 1, "call"); // allin 15 -> 50

    ASSERT_SCORE(200, 0);
}

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    return RUN_ALL_TESTS();
}
