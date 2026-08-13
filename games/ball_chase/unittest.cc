// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).
//
// This file was generated with the assistance of Claude Code (claude.ai/code).

#include "game_framework/unittest_base.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

GAME_TEST(3, player_not_enough)
{
    ASSERT_FALSE(StartGame());
}

// 指令合法性与赛况查询：公聊拒绝、身份校验、越界球号/目标、重复行动
GAME_TEST(4, errors_and_status)
{
    START_GAME();

    // 公聊行动被拒绝
    ASSERT_PUB_MSG(FAILED, 0, "不抢");
    // 无球时不能持球/护球/定向护球
    ASSERT_PRI_MSG(FAILED, 0, "持球");
    ASSERT_PRI_MSG(FAILED, 0, "护球");
    ASSERT_PRI_MSG(FAILED, 0, "定向 2");
    // 球号/目标超出范围（4 人局有 3 个球）
    ASSERT_PRI_MSG(NOT_FOUND, 0, "抢 4");
    ASSERT_PRI_MSG(NOT_FOUND, 0, "抢 0");
    ASSERT_PRI_MSG(NOT_FOUND, 0, "定向 5");
    // 赛况查询公聊私聊均可用，且不影响行动状态
    ASSERT_PUB_MSG(OK, 0, "赛况");
    ASSERT_PRI_MSG(OK, 0, "赛况");
    // 行动成功后不能再次行动
    ASSERT_PRI_MSG(OK, 0, "不抢");
    ASSERT_PRI_MSG(FAILED, 0, "护球");
    ASSERT_PRI_MSG(OK, 1, "不抢");
    ASSERT_PRI_MSG(OK, 2, "不抢");
    ASSERT_PRI_MSG(CHECKOUT, 3, "不抢");

    // R2：0 号独自抢 1 号球获得
    ASSERT_PRI_MSG(OK, 0, "抢 1");
    ASSERT_PRI_MSG(OK, 1, "不抢");
    ASSERT_PRI_MSG(OK, 2, "不抢");
    ASSERT_PRI_MSG(CHECKOUT, 3, "不抢");

    // R3：持球者的非法行动
    ASSERT_PRI_MSG(FAILED, 0, "不抢");
    ASSERT_PRI_MSG(FAILED, 0, "定向 1");
    ASSERT_PRI_MSG(FAILED, 0, "抢 1");
    ASSERT_PRI_MSG(OK, 0, "定向 2");
    ASSERT_PRI_MSG(FAILED, 0, "持球");
}

// 自由球争抢与持球转移：多人抢自由球流拍、独抢获得、持球被一人抢转移、被多人抢变自由
GAME_TEST(4, free_ball_contention_and_transfer)
{
    START_GAME();

    // R1：0、1 争抢 1 号球（流拍），2 独抢 2 号球（获得）
    ASSERT_PRI_MSG(OK, 0, "抢 1");
    ASSERT_PRI_MSG(OK, 1, "抢 1");
    ASSERT_PRI_MSG(OK, 2, "抢 2");
    ASSERT_PRI_MSG(CHECKOUT, 3, "不抢");

    // R2：0 独抢自由的 1 号球获得；1 独抢 2 号持球者的球，持球被恰一人抢，球权转移
    ASSERT_PRI_MSG(OK, 0, "抢 1");
    ASSERT_PRI_MSG(OK, 1, "抢 2");
    ASSERT_PRI_MSG(OK, 2, "持球");
    ASSERT_PRI_MSG(CHECKOUT, 3, "不抢");

    // R3：2、3 同抢 1 号持球者的球，多人抢导致该球变为自由球
    ASSERT_PRI_MSG(OK, 0, "持球");
    ASSERT_PRI_MSG(OK, 1, "持球");
    ASSERT_PRI_MSG(OK, 2, "抢 1");
    ASSERT_PRI_MSG(CHECKOUT, 3, "抢 1");

    // R4：2、3 退出（淘汰 2 人移除 2 球），仅剩 0、1 两人且 1 号球为自由球，游戏结束
    ASSERT_LEAVE(CONTINUE, 2);
    ASSERT_LEAVE(CONTINUE, 3);
    ASSERT_PRI_MSG(OK, 0, "不抢");
    ASSERT_PRI_MSG(CHECKOUT, 1, "持球");

    ASSERT_FINISHED(true);
    ASSERT_SCORE(7, 9, 3, 3);
}

// 护球规则：有人抢保住、无人抢失球；定向护球无人抢失球；持球无人抢保留
GAME_TEST(4, hold_and_protect_rules)
{
    START_GAME();

    // R1：2 独抢 2 号球获得
    ASSERT_PRI_MSG(OK, 0, "不抢");
    ASSERT_PRI_MSG(OK, 1, "不抢");
    ASSERT_PRI_MSG(OK, 2, "抢 2");
    ASSERT_PRI_MSG(CHECKOUT, 3, "不抢");

    // R2：2 护球，0、1 两人来抢，球权保留
    ASSERT_PRI_MSG(OK, 0, "抢 2");
    ASSERT_PRI_MSG(OK, 1, "抢 2");
    ASSERT_PRI_MSG(OK, 2, "护球");
    ASSERT_PRI_MSG(CHECKOUT, 3, "不抢");

    // R3：2 护球但无人抢，该球变为自由球
    ASSERT_PRI_MSG(OK, 0, "不抢");
    ASSERT_PRI_MSG(OK, 1, "不抢");
    ASSERT_PRI_MSG(OK, 2, "护球");
    ASSERT_PRI_MSG(CHECKOUT, 3, "不抢");

    // R4：2 重新独抢 2 号球成功，证明其已变为自由球
    ASSERT_PRI_MSG(OK, 0, "不抢");
    ASSERT_PRI_MSG(OK, 1, "不抢");
    ASSERT_PRI_MSG(OK, 2, "抢 2");
    ASSERT_PRI_MSG(CHECKOUT, 3, "不抢");

    // R5：2 定向护球但无人抢，该球变为自由球
    ASSERT_PRI_MSG(OK, 0, "不抢");
    ASSERT_PRI_MSG(OK, 1, "不抢");
    ASSERT_PRI_MSG(OK, 2, "定向 1");
    ASSERT_PRI_MSG(CHECKOUT, 3, "不抢");

    // R6：2 再次独抢 2 号球成功，证明其已变为自由球
    ASSERT_PRI_MSG(OK, 0, "不抢");
    ASSERT_PRI_MSG(OK, 1, "不抢");
    ASSERT_PRI_MSG(OK, 2, "抢 2");
    ASSERT_PRI_MSG(CHECKOUT, 3, "不抢");

    // R7：2 持球且无人抢，球权保留
    ASSERT_PRI_MSG(OK, 0, "不抢");
    ASSERT_PRI_MSG(OK, 1, "不抢");
    ASSERT_PRI_MSG(OK, 2, "持球");
    ASSERT_PRI_MSG(CHECKOUT, 3, "不抢");

    // R8：0、1 退出，3 血量耗尽（1-1=0），共淘汰 3 人，移除剩余 3 球，仅剩 2 号存活，游戏结束
    ASSERT_LEAVE(CONTINUE, 0);
    ASSERT_LEAVE(CONTINUE, 1);
    ASSERT_PRI_MSG(OK, 2, "持球");
    ASSERT_PRI_MSG(CHECKOUT, 3, "不抢");

    ASSERT_ELIMINATED(3);
    ASSERT_FINISHED(true);
    // 2 号：8 回合存活 +8，唯一存活且血量最高 +5
    ASSERT_SCORE(7, 7, 13, 7);
}

// 定向护球：命中目标立即淘汰、球权保留，第三方同抢也不获得；淘汰触发移除最大编号球
GAME_TEST(4, directed_guard_kill)
{
    START_GAME();

    // R1：0 获得 1 号球，1 获得 2 号球
    ASSERT_PRI_MSG(OK, 0, "抢 1");
    ASSERT_PRI_MSG(OK, 1, "抢 2");
    ASSERT_PRI_MSG(OK, 2, "不抢");
    ASSERT_PRI_MSG(CHECKOUT, 3, "不抢");

    // R2：0 定向护球指定 3 号（pid=2），3 号来抢，立即淘汰；1 持球无人抢保留
    ASSERT_PRI_MSG(OK, 0, "定向 3");
    ASSERT_PRI_MSG(OK, 1, "持球");
    ASSERT_PRI_MSG(OK, 2, "抢 1");
    ASSERT_PRI_MSG(CHECKOUT, 3, "不抢");

    ASSERT_ELIMINATED(2);

    // R3：不能以已出局玩家为定向目标
    ASSERT_PRI_MSG(FAILED, 0, "定向 3");
    // 0 定向护球指定 4 号（pid=3），1、4 同抢 1 号球：4 号命中淘汰，1 号也不获得，球权保留
    ASSERT_PRI_MSG(OK, 0, "定向 4");
    ASSERT_PRI_MSG(OK, 1, "抢 1");
    ASSERT_PRI_MSG(CHECKOUT, 3, "抢 1");

    ASSERT_ELIMINATED(3);

    // R4：仅剩 0、1 两人，0 护球无人抢，1 号球变为自由球，游戏结束
    ASSERT_PRI_MSG(OK, 0, "护球");
    ASSERT_PRI_MSG(CHECKOUT, 1, "不抢");

    ASSERT_FINISHED(true);
    ASSERT_SCORE(9, 7, 1, 2);
}

// 换球：两名持球者互抢对方的球，原球先释放，双方作为唯一抢球者成功互换
GAME_TEST(4, swap_balls)
{
    START_GAME();

    // R1：0 获得 1 号球，1 获得 2 号球
    ASSERT_PRI_MSG(OK, 0, "抢 1");
    ASSERT_PRI_MSG(OK, 1, "抢 2");
    ASSERT_PRI_MSG(OK, 2, "不抢");
    ASSERT_PRI_MSG(CHECKOUT, 3, "不抢");

    // R2：0、1 互抢对方的球，成功互换
    ASSERT_PRI_MSG(OK, 0, "抢 2");
    ASSERT_PRI_MSG(OK, 1, "抢 1");
    ASSERT_PRI_MSG(OK, 2, "不抢");
    ASSERT_PRI_MSG(CHECKOUT, 3, "不抢");

    // R3：0 号现持有 2 号球，不能抢自己的球，证明互换成立；
    //     2、3 退出，0、1 均护球放开手中球，仅剩 2 人且 1 号球为自由球，游戏结束
    ASSERT_PRI_MSG(FAILED, 0, "抢 2");
    ASSERT_LEAVE(CONTINUE, 2);
    ASSERT_LEAVE(CONTINUE, 3);
    ASSERT_PRI_MSG(OK, 0, "护球");
    ASSERT_PRI_MSG(CHECKOUT, 1, "护球");

    ASSERT_FINISHED(true);
    // 0、1 号：3 回合存活 +3，终局血量均为 7 并列最高，各 +5
    ASSERT_SCORE(8, 8, 2, 2);
}

// 全员血量耗尽：8 回合无人抢球，同回合全部淘汰，按存活回合数计分
GAME_TEST(4, bleed_out_all)
{
    START_GAME();

    for (int r = 0; r < 8; r++) {
        ASSERT_PRI_MSG(OK, 0, "不抢");
        ASSERT_PRI_MSG(OK, 1, "不抢");
        ASSERT_PRI_MSG(OK, 2, "不抢");
        ASSERT_PRI_MSG(CHECKOUT, 3, "不抢");
    }

    ASSERT_ELIMINATED(0);
    ASSERT_ELIMINATED(1);
    ASSERT_ELIMINATED(2);
    ASSERT_ELIMINATED(3);
    ASSERT_FINISHED(true);
    // 前 7 回合每人 +1，第 8 回合全员淘汰不加分，无终局加分
    ASSERT_SCORE(7, 7, 7, 7);
}

// 初始血量可配置：将血量设为 2，两回合无球即全员淘汰
GAME_TEST(4, custom_hp_option)
{
    ASSERT_PUB_MSG(OK, 0, "血量 2");
    START_GAME();

    // R1：全员不抢，无球者血量 2->1
    ASSERT_PRI_MSG(OK, 0, "不抢");
    ASSERT_PRI_MSG(OK, 1, "不抢");
    ASSERT_PRI_MSG(OK, 2, "不抢");
    ASSERT_PRI_MSG(CHECKOUT, 3, "不抢");

    // R2：全员不抢，血量 1->0，同回合全部淘汰
    ASSERT_PRI_MSG(OK, 0, "不抢");
    ASSERT_PRI_MSG(OK, 1, "不抢");
    ASSERT_PRI_MSG(OK, 2, "不抢");
    ASSERT_PRI_MSG(CHECKOUT, 3, "不抢");

    ASSERT_ELIMINATED(0);
    ASSERT_ELIMINATED(1);
    ASSERT_ELIMINATED(2);
    ASSERT_ELIMINATED(3);
    ASSERT_FINISHED(true);
    // 仅第 1 回合存活 +1，第 2 回合全员淘汰
    ASSERT_SCORE(1, 1, 1, 1);
}

// 结束条件：仅剩 2 人但球仍被持有则继续，直到球变为自由球才结束
GAME_TEST(4, endgame_needs_free_ball)
{
    START_GAME();

    // R1：0 独抢 1 号球
    ASSERT_PRI_MSG(OK, 0, "抢 1");
    ASSERT_PRI_MSG(OK, 1, "不抢");
    ASSERT_PRI_MSG(OK, 2, "不抢");
    ASSERT_PRI_MSG(CHECKOUT, 3, "不抢");

    // R2：2、3 退出，移除 2、3 号球；仅剩 0、1 两人，但 1 号球仍被 0 持有，游戏继续
    ASSERT_LEAVE(CONTINUE, 2);
    ASSERT_LEAVE(CONTINUE, 3);
    ASSERT_PRI_MSG(OK, 0, "持球");
    ASSERT_PRI_MSG(CHECKOUT, 1, "不抢");
    ASSERT_FINISHED(false);

    // R3：0 护球无人抢，1 号球变为自由球，此时仅剩 2 人且球自由，游戏结束
    ASSERT_PRI_MSG(OK, 0, "护球");
    ASSERT_PRI_MSG(CHECKOUT, 1, "不抢");
    ASSERT_FINISHED(true);
    ASSERT_SCORE(8, 6, 1, 1);
}

// 超时直接淘汰：超时玩家所持球立即变为自由球，可被本回合抢球者获得
GAME_TEST(4, timeout_eliminates)
{
    START_GAME();

    // R1：0 独抢 1 号球获得
    ASSERT_PRI_MSG(OK, 0, "抢 1");
    ASSERT_PRI_MSG(OK, 1, "不抢");
    ASSERT_PRI_MSG(OK, 2, "不抢");
    ASSERT_PRI_MSG(CHECKOUT, 3, "不抢");

    // R2：1 抢 0 号玩家持有的 1 号球，0 超时被淘汰，其球释放后由 1 号获得
    ASSERT_PRI_MSG(OK, 1, "抢 1");
    ASSERT_PRI_MSG(OK, 2, "不抢");
    ASSERT_PRI_MSG(OK, 3, "不抢");
    ASSERT_TIMEOUT(CHECKOUT);

    ASSERT_ELIMINATED(0);

    // R3：2、3 退出，淘汰 2 人移除剩余球，仅剩 1 号存活，游戏结束
    ASSERT_LEAVE(CONTINUE, 2);
    ASSERT_LEAVE(CONTINUE, 3);
    ASSERT_PRI_MSG(CHECKOUT, 1, "持球");

    ASSERT_FINISHED(true);
    ASSERT_SCORE(1, 8, 2, 2);
}

// 退出：已提交的行动作废、所持球立即变为自由球、按淘汰移除最大编号球
GAME_TEST(4, leave_voids_action_and_frees_ball)
{
    START_GAME();

    // R1：0 独抢 1 号球获得
    ASSERT_PRI_MSG(OK, 0, "抢 1");
    ASSERT_PRI_MSG(OK, 1, "不抢");
    ASSERT_PRI_MSG(OK, 2, "不抢");
    ASSERT_PRI_MSG(CHECKOUT, 3, "不抢");

    // R2：0 提交定向护球（目标 2 号）后退出：行动作废，1 号球变为自由球
    // 1 抢 1 号球未被定向命中且成功获得，证明两点均生效
    ASSERT_PRI_MSG(OK, 0, "定向 2");
    ASSERT_LEAVE(CONTINUE, 0);
    ASSERT_PRI_MSG(OK, 1, "抢 1");
    ASSERT_PRI_MSG(OK, 2, "不抢");
    ASSERT_PRI_MSG(CHECKOUT, 3, "不抢");

    // R3：3 号球已随淘汰被移除，无法抢夺
    ASSERT_PRI_MSG(FAILED, 1, "抢 3");
    // 1 定向护球命中 3 号（pid=2），移除最大编号球
    ASSERT_PRI_MSG(OK, 1, "定向 3");
    ASSERT_PRI_MSG(OK, 2, "抢 1");
    ASSERT_PRI_MSG(CHECKOUT, 3, "不抢");

    ASSERT_ELIMINATED(2);

    // R4：仅剩 1、4 两人，1 护球无人抢，1 号球变自由，游戏结束
    ASSERT_PRI_MSG(OK, 1, "护球");
    ASSERT_PRI_MSG(CHECKOUT, 3, "不抢");

    ASSERT_FINISHED(true);
    ASSERT_SCORE(1, 9, 2, 7);
}

// 弃球去抢却抢夺失败：原持球被释放、新球没抢到，最终丢球
GAME_TEST(4, abandon_grab_fail)
{
    START_GAME();

    // R1：0 获得 1 号球，1 获得 2 号球
    ASSERT_PRI_MSG(OK, 0, "抢 1");
    ASSERT_PRI_MSG(OK, 1, "抢 2");
    ASSERT_PRI_MSG(OK, 2, "不抢");
    ASSERT_PRI_MSG(CHECKOUT, 3, "不抢");

    // R2：0 弃 1 号球去抢 2 号球，2 也抢 2 号球，2 号球被多人抢变自由；
    //      0 原球已释放、新球没抢到，最终丢球；1 持有的 2 号球也丢失
    ASSERT_PRI_MSG(OK, 0, "抢 2");
    ASSERT_PRI_MSG(OK, 2, "抢 2");
    ASSERT_PRI_MSG(OK, 1, "持球");
    ASSERT_PRI_MSG(CHECKOUT, 3, "不抢");

    // R3：1、2、3 退出，0 抢 1 号球，淘汰 3 人移除全部球，仅剩 0 号存活，游戏结束
    ASSERT_LEAVE(CONTINUE, 1);
    ASSERT_LEAVE(CONTINUE, 2);
    ASSERT_LEAVE(CONTINUE, 3);
    ASSERT_PRI_MSG(CHECKOUT, 0, "抢 1");

    ASSERT_FINISHED(true);
    // 0 号：存活 3 回合 +3，终局唯一存活 +5；1、2、3 号：各存活 2 回合 +2
    ASSERT_SCORE(8, 2, 2, 2);
}

// 5 人局有 4 个球，抢球指令的边界随人数变化
GAME_TEST(5, five_players_four_balls)
{
    START_GAME();

    ASSERT_PRI_MSG(NOT_FOUND, 0, "抢 5");
    ASSERT_PRI_MSG(OK, 0, "抢 4");
    ASSERT_PRI_MSG(OK, 1, "不抢");
    ASSERT_PRI_MSG(OK, 2, "不抢");
    ASSERT_PRI_MSG(OK, 3, "不抢");
    ASSERT_PRI_MSG(CHECKOUT, 4, "不抢");

    // 0 号已持有 4 号球
    ASSERT_PRI_MSG(FAILED, 0, "抢 4");
    ASSERT_PRI_MSG(OK, 0, "护球");
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
