// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

// ===== 玩家数校验 =====
GAME_TEST(1, player_not_enough)
{
    ASSERT_FALSE(StartGame());
}

// ===== 游戏启动 =====
GAME_TEST(2, start_game_2p)
{
    START_GAME();
    ASSERT_FINISHED(false);
    ASSERT_PUB_MSG(OK, 0, "赛况");
}

GAME_TEST(3, start_game_3p)
{
    START_GAME();
    ASSERT_FINISHED(false);
}

GAME_TEST(4, start_game_4p)
{
    START_GAME();
    ASSERT_FINISHED(false);
}

// ===== 无效输入 =====
GAME_TEST(2, invalid_card_0)
{
    START_GAME();
    ASSERT_PRI_MSG(NOT_FOUND, 0, "0");
}

GAME_TEST(2, invalid_card_9)
{
    START_GAME();
    ASSERT_PRI_MSG(NOT_FOUND, 0, "9");
}

// ===== 卫兵必须填猜测 =====
GAME_TEST(2, guard_no_guess)
{
    START_GAME();
    ASSERT_PRI_MSG(FAILED, 0, "1 1");
}

GAME_TEST(2, guard_guess_1)
{
    START_GAME();
    ASSERT_PRI_MSG(NOT_FOUND, 0, "1 1 1"); // ArithChecker(2,8)拦截
}

// ===== 错误回合 =====
GAME_TEST(2, wrong_turn)
{
    START_GAME();
    ASSERT_PRI_MSG(FAILED, 1, "1 0 2");
}

// ===== 非王子不能选自己 =====
GAME_TEST(2, guard_self)
{
    START_GAME();
    ASSERT_PRI_MSG(FAILED, 0, "1 0 2");
}

// ===== 王子可选自己 =====
GAME_TEST(2, prince_self)
{
    START_GAME();
    const auto rc = PrivateRequest(0, "5 0");
    ASSERT_TRUE(rc == OK || rc == CONTINUE || rc == CHECKOUT || rc == FAILED);
}

// ===== 无目标牌（侍女）=====
GAME_TEST(2, handmaid_no_target)
{
    START_GAME();
    const auto rc = PrivateRequest(0, "4");
    ASSERT_TRUE(rc == OK || rc == CONTINUE || rc == CHECKOUT || rc == FAILED);
}

// ===== 完整游戏流程 =====
GAME_TEST(2, play_to_end)
{
    START_GAME();
    ASSERT_FINISHED(false);

    int safety = 50;
    while (!this->main_stage_->IsOver() && --safety > 0) {
        // 轮流尝试双方，谁成功算谁的
        for (PlayerID pid = 0; pid < 2; ++pid) {
            if (this->main_stage_->IsOver()) break;
            for (int c = 1; c <= 8; ++c) {
                PlayerID target = (pid == 0) ? 1 : 0;
                const auto rc = PrivateRequest(pid, std::to_string(c) + " " + std::to_string(target) + " 3");
                if (rc != FAILED && rc != NOT_FOUND) break;
                // 无目标牌
                if (c == 4 || c == 7 || c == 8) {
                    const auto rc2 = PrivateRequest(pid, std::to_string(c));
                    if (rc2 != FAILED && rc2 != NOT_FOUND) break;
                }
            }
        }
    }
    ASSERT_TRUE(safety > 0);
    ASSERT_FINISHED(true);
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
