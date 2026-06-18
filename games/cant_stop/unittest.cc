// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <set>
#include <vector>
#include <algorithm>

#include "game_framework/unittest_base.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

#include "board.h"  // 直接测试核心棋盘逻辑（确定性，不依赖随机骰子）

// ====================== 核心逻辑单元测试（确定性） ======================

// 4 骰恰有 3 种配对：(5,3,4,1) → (8,5)/(9,4)/(6,7)，开局 3 个白子可同时推进两列
TEST(CantStopBoard, basic_three_pairings)
{
    Board b;
    b.Initialize(2, {"A", "B"}, {"", ""}, 3);
    b.turn_pid_ = 0;
    b.dice_ = {5, 3, 4, 1};
    b.ComputeOptions();
    ASSERT_EQ(3u, b.options_.size());
    std::set<std::vector<int>> got;
    for (auto& o : b.options_) {
        auto a = o.advances;
        std::sort(a.begin(), a.end());
        got.insert(a);
    }
    EXPECT_TRUE(got.count(std::vector<int>{5, 8}));
    EXPECT_TRUE(got.count(std::vector<int>{4, 9}));
    EXPECT_TRUE(got.count(std::vector<int>{6, 7}));
}

// 四骰同点 (3,3,3,3)：仅一种有效配对 (6,6)，可在第 6 列连进两步
TEST(CantStopBoard, doubles_advance_same_column_twice)
{
    Board b;
    b.Initialize(2, {"A", "B"}, {"", ""}, 3);
    b.turn_pid_ = 0;
    b.dice_ = {3, 3, 3, 3};
    b.ComputeOptions();
    ASSERT_EQ(1u, b.options_.size());
    EXPECT_EQ((std::vector<int>{6, 6}), b.options_[0].advances);
}

// 仅剩 1 个空闲白子且配对的两列均为新列时，拆分为多个单列选项（二选一）
TEST(CantStopBoard, single_runner_splits_into_single_column_options)
{
    Board b;
    b.Initialize(2, {"A", "B"}, {"", ""}, 3);
    b.turn_pid_ = 0;
    b.runner_[2] = 1; // 已占用 2 个白子，剩 1 个空闲
    b.runner_[3] = 1;
    b.dice_ = {1, 3, 4, 4}; // 配对 (4,8) 与 (5,7) 均为新列
    b.ComputeOptions();
    ASSERT_EQ(4u, b.options_.size());
    std::set<int> cols;
    for (auto& o : b.options_) {
        EXPECT_EQ(1u, o.advances.size());
        cols.insert(o.advances[0]);
    }
    EXPECT_EQ((std::set<int>{4, 5, 7, 8}), cols);
}

// 停止结算：白子位于列顶则夺得该列，列被封闭，其他玩家在该列的棋子被收回
TEST(CantStopBoard, commit_claims_column_and_returns_others)
{
    Board b;
    b.Initialize(2, {"A", "B"}, {"", ""}, 3);
    b.turn_pid_ = 0;
    b.progress_[0][2] = COL_HEIGHT[2] - 1; // 自己已落定到顶端下一格
    b.progress_[1][2] = 1;                 // 对手在该列也有进度
    b.runner_[2] = COL_HEIGHT[2];          // 本回合白子推到列顶
    const std::vector<int> claimed = b.Commit(0);
    ASSERT_EQ(1u, claimed.size());
    EXPECT_EQ(2, claimed[0]);
    EXPECT_EQ(0, b.claimed_[2]);                 // 由 pid 0 夺得
    EXPECT_EQ(COL_HEIGHT[2], b.progress_[0][2]); // 自己进度提交到顶
    EXPECT_EQ(0, b.progress_[1][2]);             // 对手在该列棋子被收回
    EXPECT_EQ(1, b.ClaimedCountOf(0));
}

// 终局进度仅统计仍在盘面上的棋子：自己夺得的列只计夺列数、不再计进度；被他人夺取的列棋子已移除
TEST(CantStopBoard, on_board_progress_counts_only_pieces_on_board)
{
    Board b;
    b.Initialize(2, {"A", "B"}, {"", ""}, 3);
    // A 在两条开放列上有落定进度（应计入）
    b.progress_[0][5] = 3;
    b.progress_[0][9] = 2;
    // A 在第 6 列有进度，但随后 B 夺得该列 → A 的棋子被收回（不应计入）
    b.progress_[0][6] = 4;
    b.progress_[1][6] = COL_HEIGHT[6] - 1;
    b.runner_[6] = COL_HEIGHT[6];
    b.Commit(1);
    ASSERT_EQ(1, b.claimed_[6]);
    ASSERT_EQ(0, b.progress_[0][6]);
    // A 自己夺得第 7 列（满高）：只显示★、计入夺列数，但不再计进度
    b.runner_.fill(0);
    b.progress_[0][7] = COL_HEIGHT[7] - 1;
    b.runner_[7] = COL_HEIGHT[7];
    b.Commit(0);
    ASSERT_EQ(0, b.claimed_[7]);
    ASSERT_EQ(1, b.ClaimedCountOf(0));
    // 仅统计开放列 5、9：3 + 2 = 5（第 6 列他人夺取、第 7 列自己夺取均不计）
    EXPECT_EQ(5, b.OnBoardProgressOf(0));
}

// 三足鼎立基础：三个白子同时位于列顶时，一次停止可夺得 3 列
TEST(CantStopBoard, commit_can_claim_three_columns_at_once)
{
    Board b;
    b.Initialize(2, {"A", "B"}, {"", ""}, 3);
    b.runner_[2] = COL_HEIGHT[2];
    b.runner_[3] = COL_HEIGHT[3];
    b.runner_[12] = COL_HEIGHT[12];
    const std::vector<int> claimed = b.Commit(0);
    ASSERT_EQ(3u, claimed.size());
    EXPECT_EQ(3, b.ClaimedCountOf(0));
    EXPECT_EQ(0, b.claimed_[2]);
    EXPECT_EQ(0, b.claimed_[3]);
    EXPECT_EQ(0, b.claimed_[12]);
}

// 一掷乾坤基础：四颗骰子同点才算成功
TEST(CantStopBoard, all_dice_equal_detects_quad)
{
    Board b;
    b.Initialize(2, {"A", "B"}, {"", ""}, 3);
    b.dice_ = {4, 4, 4, 4};
    EXPECT_TRUE(b.AllDiceEqual());
    b.dice_ = {4, 4, 4, 5};
    EXPECT_FALSE(b.AllDiceEqual());
    b.dice_ = {1, 2, 3, 4};
    EXPECT_FALSE(b.AllDiceEqual());
}

// 所有列均被封闭时，任何掷骰都无法推进 → 爆掉
TEST(CantStopBoard, bust_when_all_columns_closed)
{
    Board b;
    b.Initialize(2, {"A", "B"}, {"", ""}, 3);
    b.turn_pid_ = 0;
    for (int c = COL_MIN; c <= COL_MAX; ++c) {
        b.claimed_[c] = 1; // 全部封闭
    }
    b.dice_ = {1, 2, 3, 4};
    b.ComputeOptions();
    EXPECT_TRUE(b.IsBust());
}

// 配对的一个点数所在列已封闭，则只推进另一个开放列
TEST(CantStopBoard, one_closed_column_advances_only_the_open_one)
{
    Board b;
    b.Initialize(2, {"A", "B"}, {"", ""}, 3);
    b.turn_pid_ = 0;
    b.claimed_[8] = 1;      // 第 8 列封闭
    b.dice_ = {5, 3, 4, 1}; // 含配对 (8,5)
    b.ComputeOptions();
    bool found_single_5 = false;
    for (auto& o : b.options_) {
        if (o.advances == std::vector<int>{5}) {
            found_single_5 = true;
        }
        // 不应出现推进到第 8 列的走法
        EXPECT_TRUE(std::find(o.advances.begin(), o.advances.end(), 8) == o.advances.end());
    }
    EXPECT_TRUE(found_single_5);
}

// 同一回合内连续掷骰时，白色跑子应保留并持续累加（不在掷骰间重置）——
// 这是 Can't Stop 的核心机制，对应 MainStage 中"回合内重掷不 ResetTurn"。
TEST(CantStopBoard, runner_accumulates_across_rolls_without_reset)
{
    Board b;
    b.Initialize(2, {"A", "B"}, {"", ""}, 3);
    b.turn_pid_ = 0;
    // 第 1 次掷骰：1,1,1,1 → 唯一配对 (2,2) → 第 2 列连进 2 格
    b.dice_ = {1, 1, 1, 1};
    b.ComputeOptions();
    ASSERT_EQ(1u, b.options_.size());
    EXPECT_EQ((std::vector<int>{2, 2}), b.options_[0].advances);
    b.ApplyOption(b.options_[0]);
    EXPECT_EQ(2, b.runner_[2]); // 0 → +2

    // 第 2 次掷骰（不 ResetTurn）：跑子保留，仅能再进 1 格到顶（第 2 列高度为 3）
    b.dice_ = {1, 1, 1, 1};
    b.ComputeOptions();
    ASSERT_EQ(1u, b.options_.size());
    EXPECT_EQ((std::vector<int>{2}), b.options_[0].advances);
    b.ApplyOption(b.options_[0]);
    EXPECT_EQ(3, b.runner_[2]); // 到达列顶

    // 已到顶后再掷相同骰 → 无法推进（爆掉）
    b.dice_ = {1, 1, 1, 1};
    b.ComputeOptions();
    EXPECT_TRUE(b.IsBust());

    // 提交后夺得第 2 列
    const std::vector<int> claimed = b.Commit(0);
    ASSERT_EQ(1u, claimed.size());
    EXPECT_EQ(2, claimed[0]);
    EXPECT_EQ(1, b.ClaimedCountOf(0));
}

// 跳跃变体：跑子推进到对手棋子所在格时跳过该格，落到上方第一个空格
TEST(CantStopBoard, jump_variant_skips_opponent_cube)
{
    Board b;
    b.Initialize(2, {"A", "B"}, {"", ""}, 3, "", Variant::JUMP);
    b.turn_pid_ = 0;
    b.progress_[1][7] = 1; // 对手 B 位于第 7 列位置 1
    MoveOption opt;
    opt.advances = {7};
    b.ApplyOption(opt);
    EXPECT_EQ(2, b.runner_[7]); // 本应落位置 1，被占据 → 跳到位置 2
}

// 跳跃变体：连续被占据时一路跳过，落到第一个空格
TEST(CantStopBoard, jump_variant_chains_over_consecutive)
{
    Board b;
    b.Initialize(3, {"A", "B", "C"}, {"", "", ""}, 3, "", Variant::JUMP);
    b.turn_pid_ = 0;
    b.progress_[1][7] = 1; // B 在位置 1
    b.progress_[2][7] = 2; // C 在位置 2
    MoveOption opt;
    opt.advances = {7};
    b.ApplyOption(opt);
    EXPECT_EQ(3, b.runner_[7]); // 跳过位置 1、2 → 落到位置 3
}

// 强制运动变体：跑子与对手同格时不能停止；离开后恢复可停止
TEST(CantStopBoard, forced_variant_blocks_stop_then_unblocks)
{
    Board b;
    b.Initialize(2, {"A", "B"}, {"", ""}, 3, "", Variant::FORCED);
    b.turn_pid_ = 0;
    b.progress_[1][7] = 2; // 对手 B 在第 7 列位置 2
    b.runner_[7] = 2;      // 当前玩家跑子也在位置 2 → 同格
    EXPECT_TRUE(b.MustContinue());
    b.runner_[7] = 3;      // 跑子离开
    EXPECT_FALSE(b.MustContinue());
}

// 默认规则：允许与对手共享格子，且从不强制继续
TEST(CantStopBoard, default_variant_allows_sharing_no_force)
{
    Board b;
    b.Initialize(2, {"A", "B"}, {"", ""}, 3); // 默认变体
    b.turn_pid_ = 0;
    b.progress_[1][7] = 1;
    MoveOption opt;
    opt.advances = {7};
    b.ApplyOption(opt);
    EXPECT_EQ(1, b.runner_[7]); // 默认：与对手同处位置 1（不跳跃）
    EXPECT_FALSE(b.MustContinue()); // 默认：从不强制继续
}

// 所有列被夺取的判定（用于「棋盘填满即结束」）
TEST(CantStopBoard, all_columns_claimed_detection)
{
    Board b;
    b.Initialize(2, {"A", "B"}, {"", ""}, 3);
    EXPECT_FALSE(b.AllColumnsClaimed());
    for (int c = COL_MIN; c <= COL_MAX; ++c) {
        b.claimed_[c] = 0;
    }
    EXPECT_TRUE(b.AllColumnsClaimed());
    b.claimed_[7] = UNCLAIMED; // 留一列开放
    EXPECT_FALSE(b.AllColumnsClaimed());
}

// ====================== 集成测试（驱动游戏阶段） ======================

// 人数不足（1 人）无法开局
GAME_TEST(1, player_not_enough)
{
    ASSERT_FALSE(StartGame());
}

// 人数过多（5 人）无法开局（上限 4 人）
GAME_TEST(5, too_many_players)
{
    ASSERT_FALSE(StartGame());
}

// 快捷配置指令：开局前「跳跃」「强制运动」可被识别（命中 InitOptionsCommand），随后正常开局
GAME_TEST(2, init_command_accepts_variant_keywords)
{
    ASSERT_PUB_MSG(OK, 0, "跳跃");
    ASSERT_PUB_MSG(OK, 0, "强制运动");
    START_GAME();
}

// 当前行动玩家（先手 pid 0）退出 → 仅剩 pid 1，pid 1 获胜
GAME_TEST(2, leave_current_player_other_wins)
{
    START_GAME();
    ASSERT_LEAVE(CHECKOUT, 0);
    ASSERT_SCORE(-1, 0);
}

// 非当前行动玩家（pid 1）退出 → 仅剩 pid 0，pid 0 获胜
GAME_TEST(2, leave_other_player_current_wins)
{
    START_GAME();
    ASSERT_LEAVE(CHECKOUT, 1);
    ASSERT_SCORE(0, -1);
}

// 当前玩家超时 = 本回合进度作废，轮转到下一位玩家，游戏继续
GAME_TEST(2, timeout_discards_progress_and_continues)
{
    START_GAME();
    ASSERT_TIMEOUT(CONTINUE);
}

// 非当前回合的玩家无法行动（选择走法 / 选择并停止）
GAME_TEST(2, cannot_act_out_of_turn)
{
    START_GAME();
    ASSERT_PUB_MSG(FAILED, 1, "1");
    ASSERT_PUB_MSG(FAILED, 1, "1 停止");
}

// 「编号 停止」：当前玩家推进编号 1 后立即结算本回合（首回合不可能夺列，轮转后游戏继续）
GAME_TEST(2, choose_then_stop_ends_turn)
{
    START_GAME();
    ASSERT_PUB_MSG(CONTINUE, 0, "1 停止");
}

// 任意玩家随时可查看赛况
GAME_TEST(2, status_available_to_anyone)
{
    START_GAME();
    ASSERT_PUB_MSG(OK, 0, "赛况");
    ASSERT_PUB_MSG(OK, 1, "赛况");
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
