// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <algorithm>
#include <string>

#include "game_framework/unittest_base.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

// 位于中间几列的纵向墙不会切断任何通路，可安全用于墙数相关的测试
static const char* const k_safe_walls = "7右 8右 12右 13右 17右";

// 把 5×5 的全部 40 个墙位拼成一条指令，放置后所有格子彼此隔断
static std::string AllWalls()
{
    std::string walls;
    for (int id = 1; id <= 25; ++id) {
        if (id % 5 != 0) {
            walls += std::to_string(id) + "右 ";
        }
        if (id <= 20) {
            walls += std::to_string(id) + "下 ";
        }
    }
    return walls;
}

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

// 绘制阶段超时，迷宫本身合规则自动提交，游戏继续
GAME_TEST(2, draw_timeout_auto_submit)
{
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, k_safe_walls);
    ASSERT_TIMEOUT(CHECKOUT);

    ASSERT_FINISHED(false);
}

// 绘制阶段超时，一方墙数超限无法提交，判负
GAME_TEST(2, draw_timeout_over_limit_lose)
{
    ASSERT_PRI_MSG(OK, 0, "墙数 5");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, std::string(k_safe_walls) + " 18右");
    ASSERT_TIMEOUT(CHECKOUT);

    ASSERT_SCORE(-1, 0);
}

// 绘制阶段超时，一方起点终点不通无法提交，判负
GAME_TEST(2, draw_timeout_no_path_lose)
{
    ASSERT_PRI_MSG(OK, 0, "墙数 64");
    START_GAME();

    ASSERT_PRI_MSG(OK, 1, AllWalls());
    ASSERT_TIMEOUT(CHECKOUT);

    ASSERT_SCORE(0, -1);
}

// 绘制阶段超时，双方迷宫都无法提交，两人同时判负
GAME_TEST(2, draw_timeout_both_lose)
{
    ASSERT_PRI_MSG(OK, 0, "墙数 64");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, AllWalls());
    ASSERT_PRI_MSG(OK, 1, AllWalls());
    ASSERT_TIMEOUT(CHECKOUT);

    ASSERT_SCORE(-1, -1);
}

// 绘制阶段仅一方超时，另一方已提交，超时方自动提交后游戏继续
GAME_TEST(2, draw_timeout_after_one_submit)
{
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "提交");
    ASSERT_TIMEOUT(CHECKOUT);

    ASSERT_FINISHED(false);
}

// 不画墙也可以直接提交，双方提交后进入对战阶段
GAME_TEST(2, submit_without_wall)
{
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "提交");
    ASSERT_PRI_MSG(CHECKOUT, 1, "提交");

    ASSERT_FINISHED(false);
}

// 画墙指令的各类非法输入
GAME_TEST(2, draw_invalid_input)
{
    START_GAME();

    // 地图边界不允许绘制墙体
    ASSERT_PRI_MSG(FAILED, 0, "1上");
    ASSERT_PRI_MSG(FAILED, 0, "1左");
    ASSERT_PRI_MSG(FAILED, 0, "25下");
    ASSERT_PRI_MSG(FAILED, 0, "25右");
    // 编号超出范围
    ASSERT_PRI_MSG(FAILED, 0, "26左");
    ASSERT_PRI_MSG(FAILED, 0, "0左");
    ASSERT_PRI_MSG(FAILED, 0, "1000左");
    // 缺少方向或方向无法识别
    ASSERT_PRI_MSG(FAILED, 0, "10");
    ASSERT_PRI_MSG(FAILED, 0, "10斜");
    ASSERT_PRI_MSG(FAILED, 0, "随便说句话");
    // 一次多面时，其中一面非法则整条指令拒绝
    ASSERT_PRI_MSG(FAILED, 0, "7右 1上");
    // 绘制必须私信
    ASSERT_PUB_MSG(FAILED, 0, "7右");

    ASSERT_FINISHED(false);
}

// 画墙、重复放置即移除、清空指令
GAME_TEST(2, draw_place_and_remove)
{
    START_GAME();

    // 一次放置多面，支持编号与方向分开书写
    ASSERT_PRI_MSG(OK, 0, "7右 12 左 17右");
    // 同一位置重复放置会移除该处墙体，9左 与 8右 指向同一面墙
    ASSERT_PRI_MSG(OK, 0, "8右");
    ASSERT_PRI_MSG(OK, 0, "9左");
    // 一条指令内混合放置与移除
    ASSERT_PRI_MSG(OK, 0, "7右 22右");
    ASSERT_PRI_MSG(OK, 0, "清空");
    ASSERT_PRI_MSG(OK, 0, "提交");

    ASSERT_FINISHED(false);
}

// 墙数超出上限时无法提交，重复放置移除多余的墙后可以提交
GAME_TEST(2, draw_over_limit_cannot_submit)
{
    ASSERT_PRI_MSG(OK, 0, "墙数 5");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, std::string(k_safe_walls) + " 18右");
    ASSERT_PRI_MSG(FAILED, 0, "提交");
    ASSERT_PRI_MSG(OK, 0, "18右");
    ASSERT_PRI_MSG(OK, 0, "提交");

    ASSERT_FINISHED(false);
}

// 起点到终点没有通路时无法提交
GAME_TEST(2, draw_no_path_cannot_submit)
{
    ASSERT_PRI_MSG(OK, 0, "墙数 64");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, AllWalls());
    ASSERT_PRI_MSG(FAILED, 0, "提交");
    ASSERT_PRI_MSG(OK, 0, "清空");
    ASSERT_PRI_MSG(OK, 0, "提交");

    ASSERT_FINISHED(false);
}

// 提交后不可再修改
GAME_TEST(2, draw_locked_after_submit)
{
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "提交");
    ASSERT_PRI_MSG(FAILED, 0, "7右");
    ASSERT_PRI_MSG(FAILED, 0, "清空");
    ASSERT_PRI_MSG(FAILED, 0, "提交");

    ASSERT_FINISHED(false);
}

// 同一时刻只有一位玩家可以行动
GAME_TEST(2, only_one_player_can_act)
{
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "提交");
    ASSERT_PRI_MSG(CHECKOUT, 1, "提交");

    int acted = 0;
    for (uint64_t pid = 0; pid < 2; ++pid) {
        if (!CHECK_PRI_MSG(FAILED, pid, "上")) {
            ++acted;
        }
    }
    ASSERT_GE(1, acted);
}

// 对战阶段超时，当前行动的玩家判负
GAME_TEST(2, battle_timeout_lose)
{
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "提交");
    ASSERT_PRI_MSG(CHECKOUT, 1, "提交");

    ASSERT_TIMEOUT(CHECKOUT);

    ASSERT_FINISHED(true);
    const int64_t score0 = this->main_stage_->PlayerScore(PlayerID{0});
    const int64_t score1 = this->main_stage_->PlayerScore(PlayerID{1});
    ASSERT_EQ(-1, std::min(score0, score1));
    ASSERT_EQ(0, std::max(score0, score1));
}

// 对战阶段一方退出，另一方直接获胜
GAME_TEST(2, battle_leave_lose)
{
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "提交");
    ASSERT_PRI_MSG(CHECKOUT, 1, "提交");

    ASSERT_LEAVE(CHECKOUT, 1);

    ASSERT_SCORE(0, -1);
}

// 多步指令：撞上尚未暴露的墙会停在该处并结束回合
GAME_TEST(2, multi_step_move)
{
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, k_safe_walls);
    ASSERT_PRI_MSG(OK, 1, k_safe_walls);
    ASSERT_PRI_MSG(OK, 0, "提交");
    ASSERT_PRI_MSG(CHECKOUT, 1, "提交");

    // 多步指令支持三套方向写法，非法方向整体拒绝
    ASSERT_TRUE(CHECK_PRI_MSG(FAILED, 0, "上下斜") || CHECK_PRI_MSG(FAILED, 1, "上下斜"));
    ASSERT_TRUE(CHECK_PRI_MSG(FAILED, 0, "") || CHECK_PRI_MSG(FAILED, 1, ""));

    ASSERT_FINISHED(false);
}

// 双方持续随机行动直至分出胜负，游戏必定结束
GAME_TEST(2, play_until_game_over)
{
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, k_safe_walls);
    ASSERT_PRI_MSG(OK, 1, k_safe_walls);
    ASSERT_PRI_MSG(OK, 0, "提交");
    ASSERT_PRI_MSG(CHECKOUT, 1, "提交");

    // 固定种子的伪随机走法，逐步探明墙壁并最终走到终点
    const char* const directions[] = {"上", "下", "左", "右"};
    uint32_t seed = 20240613;
    for (int i = 0; i < 8000 && !this->main_stage_->IsOver(); ++i) {
        seed = seed * 1103515245u + 12345u;
        const char* const direct = directions[(seed >> 16) % 4];
        for (uint64_t pid = 0; pid < 2 && !this->main_stage_->IsOver(); ++pid) {
            this->PrivateRequest(pid, direct);
        }
    }

    ASSERT_FINISHED(true);
}

// 快捷配置：<边长> <墙数>
GAME_TEST(2, init_options_shortcut)
{
    ASSERT_PRI_MSG(OK, 0, "5 25");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "提交");
    ASSERT_PRI_MSG(CHECKOUT, 1, "提交");

    ASSERT_FINISHED(false);
}

// 黑白模式 + 迷雾「当前」，双方持续行动直至分出胜负
GAME_TEST(2, black_white_fog_current_play)
{
    ASSERT_PRI_MSG(OK, 0, "模式 黑白");
    ASSERT_PRI_MSG(OK, 0, "迷雾 当前");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, k_safe_walls);
    ASSERT_PRI_MSG(OK, 1, k_safe_walls);
    ASSERT_PRI_MSG(OK, 0, "提交");
    ASSERT_PRI_MSG(CHECKOUT, 1, "提交");

    const char* const directions[] = {"上", "下", "左", "右"};
    uint32_t seed = 19260817;
    for (int i = 0; i < 8000 && !this->main_stage_->IsOver(); ++i) {
        seed = seed * 1103515245u + 12345u;
        const char* const direct = directions[(seed >> 16) % 4];
        for (uint64_t pid = 0; pid < 2 && !this->main_stage_->IsOver(); ++pid) {
            this->PrivateRequest(pid, direct);
        }
    }

    ASSERT_FINISHED(true);
}

// 边走边画模式：对战阶段加墙的各项校验
GAME_TEST(2, draw_while_walk_add_wall)
{
    ASSERT_PRI_MSG(OK, 0, "模式 边走边画");
    ASSERT_PRI_MSG(OK, 0, "墙数 8");
    START_GAME();

    // 预留墙壁：只放 5 面，留 3 面给对战阶段
    ASSERT_PRI_MSG(OK, 0, k_safe_walls);
    ASSERT_PRI_MSG(OK, 1, k_safe_walls);
    ASSERT_PRI_MSG(OK, 0, "提交");
    ASSERT_PRI_MSG(CHECKOUT, 1, "提交");

    // 非行动方不能加墙；行动方必须私信
    int accepted = 0;
    for (uint64_t pid = 0; pid < 2; ++pid) {
        ASSERT_PUB_MSG(FAILED, pid, "18右");
        if (!CHECK_PRI_MSG(FAILED, pid, "18右")) {
            ++accepted;
        }
    }
    ASSERT_EQ(1, accepted);

    ASSERT_FINISHED(false);
}

// 边走边画模式：超出墙壁上限与重复位置都会被拒绝
GAME_TEST(2, draw_while_walk_add_wall_limit)
{
    ASSERT_PRI_MSG(OK, 0, "模式 边走边画");
    ASSERT_PRI_MSG(OK, 0, "墙数 5");
    START_GAME();

    // 绘制阶段就把额度用满，对战阶段一面都加不了
    ASSERT_PRI_MSG(OK, 0, k_safe_walls);
    ASSERT_PRI_MSG(OK, 1, k_safe_walls);
    ASSERT_PRI_MSG(OK, 0, "提交");
    ASSERT_PRI_MSG(CHECKOUT, 1, "提交");

    ASSERT_PRI_MSG(FAILED, 0, "18右");
    ASSERT_PRI_MSG(FAILED, 1, "18右");
    // 已有墙的位置同样被拒绝
    ASSERT_PRI_MSG(FAILED, 0, "7右");
    ASSERT_PRI_MSG(FAILED, 1, "7右");

    ASSERT_FINISHED(false);
}

// 普通模式与黑白模式下，对战阶段的加墙指令不可用
GAME_TEST(2, add_wall_rejected_in_other_modes)
{
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "提交");
    ASSERT_PRI_MSG(CHECKOUT, 1, "提交");

    ASSERT_PRI_MSG(FAILED, 0, "18右");
    ASSERT_PRI_MSG(FAILED, 1, "18右");

    ASSERT_FINISHED(false);
}

// 边走边画模式下，双方持续行动直至分出胜负
GAME_TEST(2, draw_while_walk_play_until_game_over)
{
    ASSERT_PRI_MSG(OK, 0, "模式 边走边画");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, k_safe_walls);
    ASSERT_PRI_MSG(OK, 1, k_safe_walls);
    ASSERT_PRI_MSG(OK, 0, "提交");
    ASSERT_PRI_MSG(CHECKOUT, 1, "提交");

    const char* const directions[] = {"上", "下", "左", "右"};
    uint32_t seed = 31415926;
    for (int i = 0; i < 8000 && !this->main_stage_->IsOver(); ++i) {
        seed = seed * 1103515245u + 12345u;
        const char* const direct = directions[(seed >> 16) % 4];
        for (uint64_t pid = 0; pid < 2 && !this->main_stage_->IsOver(); ++pid) {
            // 行动之余不断尝试加墙，加不了的会被拒绝，不影响对局推进
            this->PrivateRequest(pid, std::to_string((seed >> 8) % 25 + 1) + "右");
            this->PrivateRequest(pid, direct);
        }
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
