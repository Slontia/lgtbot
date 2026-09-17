// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).
//
// This file was generated with the assistance of Claude Code (claude.ai/code).

#include "game_framework/unittest_base.h"

namespace lgtbot {
namespace game {
namespace GAME_MODULE_NAME {

// 注：MainStage::PlayerScore 由「计分」选项决定返回值——排名模式返回淘汰名次分，
// 分数模式返回玩家最终总分。本文件所有用例都在开局前设置「计分 分数」，
// 因此 ASSERT_SCORE 断言的是玩家的盘面总分（含「有1吗」加成）。

// ============================================================================
// 1. 开局校验
// ============================================================================

// 少于 2 人时 AdaptOptions 应拒绝开局
GAME_TEST(1, one_player_rejected) {
    ASSERT_PUB_MSG(OK, 0, "计分 分数");
    ASSERT_FALSE(StartGame());
}

GAME_TEST(2, two_player_start_ok) {
    ASSERT_PUB_MSG(OK, 0, "计分 分数");
    ASSERT_PUB_MSG(OK, 0, "种子 test");
    ASSERT_PUB_MSG(OK, 0, "事件 无");
    ASSERT_TRUE(StartGame());
    ASSERT_FINISHED(false);
}

// ============================================================================
// 2. 放置阶段指令校验
// ============================================================================

// 重复设置应失败
GAME_TEST(2, set_twice_failed) {
    ASSERT_PUB_MSG(OK, 0, "计分 分数");
    ASSERT_PUB_MSG(OK, 0, "种子 test");
    ASSERT_PUB_MSG(OK, 0, "事件 无");
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(OK, 0, "10");
    ASSERT_PUB_MSG(FAILED, 0, "11");
}

// 位置超出 0-19 的范围时参数校验不通过，匹配不到任何指令 → NOT_FOUND
GAME_TEST(2, set_out_of_range_failed) {
    ASSERT_PUB_MSG(OK, 0, "计分 分数");
    ASSERT_PUB_MSG(OK, 0, "种子 test");
    ASSERT_PUB_MSG(OK, 0, "事件 无");
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(NOT_FOUND, 0, "20");
}

// 弃牌（位置 0）与放置都能让回合正常结束；第一回合放 1 张牌不可能成线，双方均为 0 分
GAME_TEST(2, discard_and_place_first_round) {
    ASSERT_PUB_MSG(OK, 0, "计分 分数");
    ASSERT_PUB_MSG(OK, 0, "种子 test");
    ASSERT_PUB_MSG(OK, 0, "事件 无");
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(OK, 0, "1");        // P0 放置
    ASSERT_PUB_MSG(CHECKOUT, 1, "0");  // P1 弃牌，本回合结束
    ASSERT_SCORE(0, 0);
}

// 覆盖放置：同一位置连续放两回合，盘面仍只有 1 张牌 → 不可能成线
GAME_TEST(2, overwrite_same_position) {
    ASSERT_PUB_MSG(OK, 0, "计分 分数");
    ASSERT_PUB_MSG(OK, 0, "种子 test");
    ASSERT_PUB_MSG(OK, 0, "事件 无");
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(OK, 0, "10");  ASSERT_PUB_MSG(CHECKOUT, 1, "0");
    ASSERT_PUB_MSG(OK, 0, "10");  ASSERT_PUB_MSG(CHECKOUT, 1, "0");
    ASSERT_SCORE(0, 0);
}

// ============================================================================
// 3. 完整流程冒烟测试（含选牌轮、对战、淘汰、终局结算）
//    仅验证不同人数 / 血量 / 事件下游戏都能正常跑完，不断言具体分数。
// ============================================================================

#define AUTO_PLAY_FINISH_TEST(player_num, test_name, hp_option, event_option, max_steps) \
GAME_TEST(player_num, test_name) { \
    ASSERT_PUB_MSG(OK, 0, "计分 分数"); \
    ASSERT_PUB_MSG(OK, 0, "种子 test"); \
    ASSERT_PUB_MSG(OK, 0, "事件 " event_option); \
    ASSERT_PUB_MSG(OK, 0, "血量 " hp_option); \
    ASSERT_TRUE(StartGame()); \
    for (int i = 0; i < (max_steps) && !this->main_stage_->IsOver(); ++i) this->TimeoutRequest_(); \
    ASSERT_FINISHED(true); \
}

// 低血量：正常走完淘汰流程（剩 1 人结束）
AUTO_PLAY_FINISH_TEST(2, two_player_low_hp,   "50",  "无", 300)
AUTO_PLAY_FINISH_TEST(4, four_player_low_hp,  "50",  "无", 400)
AUTO_PLAY_FINISH_TEST(8, eight_player_low_hp, "50",  "无", 600)

// 高血量：走"卡池耗尽"结束分支（多人并列存活）
AUTO_PLAY_FINISH_TEST(2, two_player_pool_exhaust,   "1000", "无", 600)
AUTO_PLAY_FINISH_TEST(3, three_player_pool_exhaust, "1000", "无", 600)

// 奇数人数：每轮都会产生一场镜像对战
AUTO_PLAY_FINISH_TEST(3, three_player_mirror_battle, "50", "无", 400)
AUTO_PLAY_FINISH_TEST(5, five_player_mirror_battle,  "50", "无", 500)

// 逐个特殊事件：验证卡池裁剪 / 癞子 / 加分 / 预知都不会破坏流程
AUTO_PLAY_FINISH_TEST(2, event_big_coming,   "500", "大的要来了",   600)
AUTO_PLAY_FINISH_TEST(2, event_polarization, "500", "两极分化",     600)
AUTO_PLAY_FINISH_TEST(2, event_big_gone,     "500", "大的没了",     600)
AUTO_PLAY_FINISH_TEST(2, event_windfall,     "500", "天降恩泽",     600)
AUTO_PLAY_FINISH_TEST(2, event_palette,      "500", "调色盘",       600)
AUTO_PLAY_FINISH_TEST(2, event_valuable_one, "500", "有1吗",        600)
AUTO_PLAY_FINISH_TEST(2, event_foresee,      "500", "小透不算挂",   600)
AUTO_PLAY_FINISH_TEST(2, event_april_fool,   "500", "？？？",       600)

// 4 人 + 随机事件：不指定事件时走随机分支
GAME_TEST(4, four_player_random_event) {
    ASSERT_PUB_MSG(OK, 0, "计分 分数");
    ASSERT_PUB_MSG(OK, 0, "种子 test");
    ASSERT_TRUE(StartGame());
    for (int i = 0; i < 400 && !this->main_stage_->IsOver(); ++i) this->TimeoutRequest_();
    ASSERT_FINISHED(true);
}

// ============================================================================
// 4. 选牌顺序
// ============================================================================

// 顺位模式：第 1~7 回合双方全部弃牌 → 分数恒为 0、对战平局不扣血，
// 进入第 8 回合选牌轮时双方血量与分数完全相同，此时先后完全由「选牌顺序」决定。
// 顺位模式下平局按开局玩家编号升序，应当由 0 号玩家先选、1 号玩家此时无法选牌。
GAME_TEST(2, select_order_sequential_tie_breaks_by_player_id) {
    ASSERT_PUB_MSG(OK, 0, "计分 分数");
    ASSERT_PUB_MSG(OK, 0, "选牌顺序 顺位");
    ASSERT_PUB_MSG(OK, 0, "种子 test");
    ASSERT_PUB_MSG(OK, 0, "事件 无");
    ASSERT_PUB_MSG(OK, 0, "血量 500");
    ASSERT_TRUE(StartGame());
    for (int i = 0; i < 7; ++i) {
        ASSERT_PUB_MSG(OK, 0, "0");
        ASSERT_PUB_MSG(CHECKOUT, 1, "0");
    }
    ASSERT_SCORE(0, 0);
    ASSERT_PUB_MSG(FAILED, 1, "1 1");       // 尚未轮到 1 号玩家
    // 0 号玩家先选；选完后阶段交棒给 1 号玩家（StageOver_ 返回 CONTINUE，阶段不结束）
    ASSERT_PUB_MSG(CONTINUE, 0, "1 1");
    for (int i = 0; i < 600 && !this->main_stage_->IsOver(); ++i) this->TimeoutRequest_();
    ASSERT_FINISHED(true);
}

// 顺位模式的多人局：验证选项在人数更多、血量更低（会发生淘汰）时也能跑完整局
GAME_TEST(4, four_player_select_order_sequential) {
    ASSERT_PUB_MSG(OK, 0, "计分 分数");
    ASSERT_PUB_MSG(OK, 0, "选牌顺序 顺位");
    ASSERT_PUB_MSG(OK, 0, "种子 test");
    ASSERT_PUB_MSG(OK, 0, "事件 无");
    ASSERT_PUB_MSG(OK, 0, "血量 50");
    ASSERT_TRUE(StartGame());
    for (int i = 0; i < 400 && !this->main_stage_->IsOver(); ++i) this->TimeoutRequest_();
    ASSERT_FINISHED(true);
}

// ============================================================================
// 5. 分数基线（回归基线，改动放置 / 计分 / 卡池逻辑时若此值变化需确认是否预期）
//    auto-play 用「计分 分数、种子 test、事件 无、血量 500」，2 人局，双方每轮都超时
//    按 SeqFill 顺序填入，结果确定。两位玩家的盘面只差首轮那张砖块，因此首轮发牌顺序
//    直接决定两人分数的归属——两种「选牌顺序」各锁一条基线，且都显式指定该选项，
//    避免以后改动默认值时基线被静默改写。
// ============================================================================

GAME_TEST(2, baseline_auto_play_sequential_deal) {
    ASSERT_PUB_MSG(OK, 0, "计分 分数");
    ASSERT_PUB_MSG(OK, 0, "选牌顺序 顺位");
    ASSERT_PUB_MSG(OK, 0, "种子 test");
    ASSERT_PUB_MSG(OK, 0, "事件 无");
    ASSERT_PUB_MSG(OK, 0, "血量 500");
    ASSERT_TRUE(StartGame());
    for (int i = 0; i < 600 && !this->main_stage_->IsOver(); ++i) this->TimeoutRequest_();
    ASSERT_FINISHED(true);
    ASSERT_SCORE(15, 6);
}

GAME_TEST(2, baseline_auto_play_random_deal) {
    ASSERT_PUB_MSG(OK, 0, "计分 分数");
    ASSERT_PUB_MSG(OK, 0, "选牌顺序 随机");
    ASSERT_PUB_MSG(OK, 0, "种子 test");
    ASSERT_PUB_MSG(OK, 0, "事件 无");
    ASSERT_PUB_MSG(OK, 0, "血量 500");
    ASSERT_TRUE(StartGame());
    for (int i = 0; i < 600 && !this->main_stage_->IsOver(); ++i) this->TimeoutRequest_();
    ASSERT_FINISHED(true);
    ASSERT_SCORE(6, 15);
}

} // namespace GAME_MODULE_NAME
} // namespace game
} // namespace lgtbot

int main(int argc, char** argv) { testing::InitGoogleTest(&argc, argv); gflags::ParseCommandLineFlags(&argc, &argv, true); return RUN_ALL_TESTS(); }
