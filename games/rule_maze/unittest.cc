// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

GAME_TEST(2, leave_test1)
{
    START_GAME();

    ASSERT_LEAVE(CHECKOUT, 0);

    ASSERT_SCORE(-1, 0);
}

GAME_TEST(2, leave_test2)
{
    START_GAME();

    ASSERT_LEAVE(CHECKOUT, 1);

    ASSERT_SCORE(0, -1);
}

// 准备阶段双方均超时，两人同时判负
GAME_TEST(2, setup_timeout_both_lose)
{
    START_GAME();

    ASSERT_TIMEOUT(CHECKOUT);

    ASSERT_SCORE(-1, -1);
}

// 准备阶段仅一方超时，另一方获胜
GAME_TEST(2, setup_timeout_one_lose)
{
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "1 3");
    ASSERT_TIMEOUT(CHECKOUT);

    ASSERT_SCORE(0, -1);
}

// 开局信息提交完毕后进入情报阶段，三条情报必须互不相同
GAME_TEST(2, setup_then_intel)
{
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "1 3");
    ASSERT_PRI_MSG(CHECKOUT, 1, "2 4");

    ASSERT_PRI_MSG(FAILED, 0, "A A B");
    // 情报共 A~G 七项，H 不是合法情报
    ASSERT_PRI_MSG(NOT_FOUND, 0, "A B H");
    ASSERT_PRI_MSG(OK, 0, "A B C");
    ASSERT_PRI_MSG(CHECKOUT, 1, "D E F");

    ASSERT_FINISHED(false);
}

// 开局信息的取值范围校验
GAME_TEST(2, setup_boundary_value)
{
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "0 2");
    ASSERT_PRI_MSG(CHECKOUT, 1, "2 7");

    ASSERT_FINISHED(false);
}

// 赛况指令在公屏与私聊均可使用
GAME_TEST(2, status_command)
{
    START_GAME();

    ASSERT_PUB_MSG(OK, 0, "赛况");
    ASSERT_PRI_MSG(OK, 0, "赛况");

    ASSERT_PRI_MSG(OK, 0, "0 2");
    ASSERT_PRI_MSG(CHECKOUT, 1, "0 2");

    ASSERT_PUB_MSG(OK, 1, "赛况");
    ASSERT_PRI_MSG(OK, 1, "赛况");
}

// 行动阶段双方均超时，两人同时判负
GAME_TEST(2, action_timeout_lose)
{
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "1 3");
    ASSERT_PRI_MSG(CHECKOUT, 1, "2 4");
    ASSERT_PRI_MSG(OK, 0, "A B C");
    ASSERT_PRI_MSG(CHECKOUT, 1, "D E F");

    ASSERT_TIMEOUT(CHECKOUT);

    ASSERT_SCORE(-1, -1);
}

// 行动阶段一方退出，另一方直接获胜
GAME_TEST(2, action_leave_lose)
{
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "1 3");
    ASSERT_PRI_MSG(CHECKOUT, 1, "2 4");
    ASSERT_PRI_MSG(OK, 0, "A B C");
    ASSERT_PRI_MSG(CHECKOUT, 1, "D E F");

    ASSERT_LEAVE(CHECKOUT, 1);

    ASSERT_SCORE(0, -1);
}

// 双方持续行动直至分出胜负或达到回合数上限，游戏必定结束
GAME_TEST(2, play_until_game_over)
{
    ASSERT_PRI_MSG(OK, 0, "回合数 5");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "1 2");
    ASSERT_PRI_MSG(CHECKOUT, 1, "1 2");
    ASSERT_PRI_MSG(OK, 0, "A B C");
    ASSERT_PRI_MSG(CHECKOUT, 1, "D E F");

    // 使用 sxzy 别名，同时验证该格式可被正确解析
    const char* const directions[] = {"s", "y", "x", "z"};
    for (int i = 0; i < 200 && !this->main_stage_->IsOver(); ++i) {
        this->PrivateRequest(0, directions[i % 4]);
        if (this->main_stage_->IsOver()) {
            break;
        }
        this->PrivateRequest(1, directions[(i + 2) % 4]);
    }

    ASSERT_FINISHED(true);
}

// 墙数上限的迷宫同样可以正常开局与行动
GAME_TEST(2, play_with_max_wall)
{
    ASSERT_PRI_MSG(OK, 0, "回合数 5");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "2 7");
    ASSERT_PRI_MSG(CHECKOUT, 1, "2 7");
    ASSERT_PRI_MSG(OK, 0, "E F G");
    ASSERT_PRI_MSG(CHECKOUT, 1, "G F B");

    // 使用 UDLR 别名，同时验证该格式可被正确解析
    const char* const directions[] = {"R", "D", "L", "U"};
    for (int i = 0; i < 200 && !this->main_stage_->IsOver(); ++i) {
        this->PrivateRequest(0, directions[i % 4]);
        if (this->main_stage_->IsOver()) {
            break;
        }
        this->PrivateRequest(1, directions[(i + 1) % 4]);
    }

    ASSERT_FINISHED(true);
}

// 电脑玩家可以独立完成整局游戏
GAME_TEST(2, computer_play_until_game_over)
{
    ASSERT_PRI_MSG(OK, 0, "回合数 5");
    START_GAME();

    for (int i = 0; i < 500 && !this->main_stage_->IsOver(); ++i) {
        this->ComputerActRequest_(i % 2);
    }

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
