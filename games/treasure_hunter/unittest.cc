// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

#include "board.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

GAME_TEST(3, player_not_enough)
{
    ASSERT_FALSE(StartGame());
}

GAME_TEST(4, all_continue_to_next_round)
{
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "继续");
    ASSERT_PRI_MSG(OK, 1, "继续");
    ASSERT_PRI_MSG(OK, 2, "继续");
    ASSERT_PRI_MSG(CHECKOUT, 3, "继续");
    ASSERT_FINISHED(false);

    // 游戏尚未结束，此时不应发放任何成就
    ASSERT_ACHIEVEMENTS(0);
}

GAME_TEST(4, all_retreat_finishes_game)
{
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "撤离");
    ASSERT_PRI_MSG(OK, 1, "撤离");
    ASSERT_PRI_MSG(OK, 2, "撤离");
    ASSERT_PRI_MSG(CHECKOUT, 3, "撤离");
    ASSERT_FINISHED(true);

    // 全员在第一回合一起撤离，金币必然完全相同，此时无人算作全场最高，不发放成就
    for (uint32_t pid = 0; pid < 4; ++pid) {
        ASSERT_ACHIEVEMENTS(pid);
    }
}

GAME_TEST(4, choice_must_be_private)
{
    START_GAME();

    ASSERT_PUB_MSG(FAILED, 0, "继续");
    ASSERT_PUB_MSG(FAILED, 0, "撤离");
}

GAME_TEST(4, can_not_choose_twice)
{
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "继续");
    ASSERT_PRI_MSG(FAILED, 0, "继续");
    ASSERT_PRI_MSG(FAILED, 0, "撤离");
}

GAME_TEST(4, retreated_player_can_not_act)
{
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "撤离");
    ASSERT_PRI_MSG(OK, 1, "继续");
    ASSERT_PRI_MSG(OK, 2, "继续");
    ASSERT_PRI_MSG(CHECKOUT, 3, "继续");

    ASSERT_PRI_MSG(FAILED, 0, "继续");
    ASSERT_PRI_MSG(FAILED, 0, "撤离");
}

GAME_TEST(4, timeout_acts_as_continue)
{
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "继续");
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_FINISHED(false);
}

GAME_TEST(4, auto_explore_toggles_hook)
{
    START_GAME();

    // 存在挂机玩家时，其余玩家行动完毕只会进入最短等待窗口，不会立刻结算
    ASSERT_PRI_MSG(OK, 0, "自动探险");
    ASSERT_PRI_MSG(OK, 1, "继续");
    ASSERT_PRI_MSG(OK, 2, "继续");
    ASSERT_PRI_MSG(CONTINUE, 3, "继续");
    ASSERT_TIMEOUT(CHECKOUT);

    // 挂机玩家不再被等待，仍然只需其余三人行动即可推进回合
    ASSERT_PRI_MSG(OK, 1, "继续");
    ASSERT_PRI_MSG(OK, 2, "继续");
    ASSERT_PRI_MSG(CONTINUE, 3, "继续");
    ASSERT_TIMEOUT(CHECKOUT);

    // 关闭后无人挂机，四人全部行动完毕即可立刻结算
    ASSERT_PRI_MSG(OK, 0, "自动探险");
    ASSERT_PRI_MSG(OK, 1, "继续");
    ASSERT_PRI_MSG(OK, 2, "继续");
    ASSERT_PRI_MSG(OK, 3, "继续");
    ASSERT_PRI_MSG(CHECKOUT, 0, "继续");
}

// 全员挂机的短回合中解除挂机，应恢复正常时限，且不会被其余挂机玩家阻塞
GAME_TEST(4, unhook_restores_normal_round)
{
    START_GAME();

    ASSERT_TIMEOUT(CHECKOUT);  // 四人全部超时，进入挂机
    ASSERT_PRI_MSG(OK, 0, "自动探险");

    // 其余三人仍在挂机且不再阻塞，玩家 0 行动后只需等待最短窗口
    ASSERT_PRI_MSG(CONTINUE, 0, "继续");
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_FINISHED(false);
}

// 挂机玩家直接撤离，本回合结算时必须真的离开秘境
GAME_TEST(4, hooked_player_retreat_takes_effect)
{
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "自动探险");
    ASSERT_PRI_MSG(OK, 0, "撤离");
    ASSERT_PRI_MSG(OK, 1, "继续");
    ASSERT_PRI_MSG(OK, 2, "继续");
    ASSERT_PRI_MSG(CHECKOUT, 3, "继续");

    ASSERT_PRI_MSG(FAILED, 0, "继续");  // 已撤离的玩家不能再行动
}

// 全员挂机的短回合中直接撤离，同样必须生效
GAME_TEST(4, hooked_player_retreat_in_short_round)
{
    START_GAME();

    ASSERT_TIMEOUT(CHECKOUT);  // 四人全部超时挂机
    ASSERT_PRI_MSG(OK, 0, "撤离");
    ASSERT_TIMEOUT(CHECKOUT);

    ASSERT_PRI_MSG(FAILED, 0, "继续");
}

GAME_TEST(4, hooked_player_can_still_choose)
{
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "自动探险");
    // 挂机玩家本回合尚未做出选择，仍可主动撤离并同时解除挂机
    ASSERT_PRI_MSG(OK, 0, "撤离");
    ASSERT_PRI_MSG(FAILED, 0, "继续");
}

GAME_TEST(4, timeout_enters_hook_state)
{
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "继续");
    ASSERT_TIMEOUT(CHECKOUT);

    // 玩家 1~3 超时后进入挂机，本回合只需玩家 0 行动，随后走完最短等待窗口
    ASSERT_PRI_MSG(CONTINUE, 0, "继续");
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_FINISHED(false);
}

GAME_TEST(4, status_command_is_available)
{
    START_GAME();

    ASSERT_PUB_MSG(OK, 0, "赛况");
    ASSERT_PRI_MSG(OK, 1, "赛况");
}

GAME_TEST(4, leaver_loses_all_gold)
{
    START_GAME();

    ASSERT_LEAVE(CONTINUE, 0);
    ASSERT_EQ(0, this->main_stage_->PlayerScore(PlayerID{0u}));

    ASSERT_PRI_MSG(OK, 1, "继续");
    ASSERT_PRI_MSG(OK, 2, "继续");
    ASSERT_PRI_MSG(CHECKOUT, 3, "继续");
    ASSERT_EQ(0, this->main_stage_->PlayerScore(PlayerID{0u}));
}

GAME_TEST(4, extract_nickname)
{
    EXPECT_EQ("铁蛋", ExtractNickname("<铁蛋(1234567)>"));
    EXPECT_EQ("PLAYER_0", ExtractNickname("PLAYER_0"));
    EXPECT_EQ("机器人1号", ExtractNickname("机器人1号"));
    EXPECT_EQ("abc", ExtractNickname("<abc>"));
    EXPECT_EQ("a(b)c", ExtractNickname("<a(b)c(1234567)>"));
    EXPECT_EQ("", ExtractNickname(""));
    EXPECT_EQ("<>", ExtractNickname("<>"));
}

// 昵称长度不可控，检查超长昵称不会撑破状态栏
static void BoardcastLongNameTable(const std::string& resource_dir, ChildMsgSenderBase& sender, const size_t player_num)
{
    Board board(resource_dir);
    board.Initialize();
    const char* const names[] = {
        "这是一个非常非常长到离谱的玩家昵称用来测试排版是否会被撑破",
        "AVeryVeryLongEnglishNicknameWithoutAnySpace",
        "短",
        "中等长度的昵称",
        "刚好八个字的昵称",
    };
    for (size_t i = 0; i < player_num; ++i) {
        board.players.emplace_back(names[i], "");
    }
    board.players[1].gold = 12345;
    board.players[2].state = PlayerState::RETREATED;
    board.players[3].state = PlayerState::DEVOURED;
    for (int i = 0; i < 9; ++i) {
        board.past.emplace_back(board.pool[i]);
    }
    sender() << Markdown(board.GetTableMarkdown(), board.TableWidth());
}

// 彩蛋卡面只替换地狱犬，且整局保持一致
GAME_TEST(4, special_cerberus_card)
{
    Board board(this->resource_dir_str_);
    board.Initialize();

    board.special_cerberus = false;
    EXPECT_EQ(string::npos, board.GetCardMarkdown(MakeMonsterCard(CERBERUS)).find("cerberus_special.png"));

    board.special_cerberus = true;
    EXPECT_NE(string::npos, board.GetCardMarkdown(MakeMonsterCard(CERBERUS)).find("cerberus_special.png"));
    EXPECT_EQ(string::npos, board.GetCardMarkdown(MakeMonsterCard(PHOENIX)).find("cerberus_special.png"));
    EXPECT_EQ(string::npos, board.GetCardMarkdown(MakeMonsterCard(MEDUSA)).find("cerberus_special.png"));

    for (int monster = PHOENIX; monster <= MEDUSA; ++monster) {
        board.past.emplace_back(MakeMonsterCard(monster));
    }
    this->match_->BoardcastMsgSender()() << Markdown(board.GetTableMarkdown(), board.TableWidth());
}

GAME_TEST(4, long_player_name_layout_4p)
{
    BoardcastLongNameTable(this->resource_dir_str_, this->match_->BoardcastMsgSender(), 4);
}

GAME_TEST(5, long_player_name_layout_5p)
{
    BoardcastLongNameTable(this->resource_dir_str_, this->match_->BoardcastMsgSender(), 5);
}

GAME_TEST(5, game_always_finishes)
{
    START_GAME();

    // 怪物共 6 张、每种 2 张，牌堆必定在耗尽前触发同种怪物二次出现，游戏一定会结束
    for (int round = 0; round < 40 && !this->main_stage_->IsOver(); ++round) {
        TimeoutRequest_();
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
