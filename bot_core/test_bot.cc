// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <filesystem>
#include <thread>
#include <chrono>

#include <gtest/gtest.h>
#include <gflags/gflags.h>

#include "bot_core/msg_sender.h"
#include "bot_core/bot_core.h"
#include "bot_core/bot_ctx.h"
#include "bot_core/db_manager.h"
#include "bot_core/score_calculation.h"
#include "bot_core/match.h"
#include "utility/process_signals.h"

static_assert(TEST_BOT);

static std::ostream& operator<<(std::ostream& os, const ErrCode e) { return os << errcode2str(e); }

constexpr const char* const k_this_qq = "114514";
constexpr const char* const k_admin_qq = "1919810";
static int g_argc = 0;
static char** g_argv = nullptr;

class MockDBManager : public DBManagerBase
{
  public:
    virtual std::vector<ScoreInfo> RecordMatch(const std::string& game_name, const std::optional<GroupID> gid,
            const UserID& host_uid, const uint64_t multiple,
            const std::vector<std::pair<UserID, int64_t>>& game_score_infos,
            const std::vector<std::pair<UserID, std::string>>& achievements) override
    {
        std::vector<UserInfoForCalScore> user_infos;
        for (const auto& [uid, game_score] : game_score_infos) {
            user_infos.emplace_back(uid, game_score, 0, 1500);
        }
        auto score_infos = CalScores(user_infos, multiple);
        for (const auto& info : score_infos) {
            match_profiles_.emplace_back(game_name, "sometime", score_infos.size(), multiple, info.game_score_,
                    info.zero_sum_score_, info.top_score_);
        }
        for (const auto& [user_id, achievement_name] : achievements) {
            user_achievements_[user_id].emplace_back(achievement_name);
        }
        return score_infos;
    }

    virtual UserProfile GetUserProfile(const UserID& uid, const std::string_view& time_range_begin,
            const std::string_view& time_range_end) override
    {
        return user_profiles_[uid];
    }

    virtual bool Suicide(const UserID& uid, const uint32_t required_match_num) override { return true; }

    virtual RankInfo GetRank(const std::string_view& time_range_begin, const std::string_view& time_range_end,
            const std::optional<GroupID>& gid = std::nullopt) override
    {
        return {};
    }

    virtual GameRankInfo GetLevelScoreRank(const std::string& game_name, const std::string_view& time_range_begin,
            const std::string_view& time_range_end, const std::optional<GroupID>& gid = std::nullopt) override
    {
        return {};
    }

    virtual AchievementStatisticInfo GetAchievementStatistic(const UserID& uid, const std::string& game_name,
            const std::string& achievement_name) override
    {
        return {};
    }

    virtual std::vector<HonorInfo> GetHonors(const std::string& honor, const uint32_t limit) override { return {}; }

    virtual bool AddHonor(const UserID& uid, const std::string_view& description) override { return true; }

    virtual bool DeleteHonor(const int32_t id) override { return true; }

    std::vector<MatchProfile> match_profiles_;
    std::map<UserID, UserProfile> user_profiles_;
    std::map<UserID, std::vector<std::string>> user_achievements_;
};

// When non-null, HandleMessages appends each message (as a rendered string) to this vector
// instead of (or in addition to) printing it.  Tests set this to capture outgoing messages
// for assertion.
static std::vector<std::string>* g_captured_messages = nullptr;

void HandleMessages(void* handler, const char* const id, const int is_uid, const LGTBot_Message* messages, const size_t size)
{
    std::string s = is_uid ? "[BOT -> USER_" : "[BOT -> GROUP_";
    s.append(id);
    s.append("]\n");
    for (size_t i = 0; i < size; ++i) {
        const auto& msg = messages[i];
        switch (msg.type_) {
        case LGTBOT_MSG_TEXT:
            s.append(msg.str_);
            break;
        case LGTBOT_MSG_USER_MENTION:
            s.append("@");
            s.append(msg.str_);
            break;
        case LGTBOT_MSG_IMAGE:
            s.append("[image=");
            s.append(msg.str_);
            s.append("]");
            break;
        default:
            assert(false);
        }
    }
    std::cout << s << std::endl;
    if (g_captured_messages) {
        g_captured_messages->push_back(s);
    }
}

void GetUserName(void* handler, char* buffer, size_t size, const char* const user_id)
{
    strncpy(buffer, user_id, size);
}

void GetUserNameInGroup(void* handler, char* buffer, size_t size, const char* group_id, const char* const user_id)
{
    snprintf(buffer, size, "%s(gid=%s)", user_id, group_id);
}

int DownloadUserAvatar(void* handler, const char* const uid_str, const char* const dest_filename) { return false; }

class TestBot : public testing::Test
{
  public:
    virtual void SetUp() override
    {
        auto game_handles = BotCtx::LoadGameModules(TEST_GAME_PLUGIN_DIR);
        ASSERT_TRUE(std::holds_alternative<GameHandleMap>(game_handles)) << "Failed to load test game plugin";
        bot_.reset(new BotCtx(
                    TEST_GAME_PLUGIN_DIR, // game_path
                    "", // conf_path
                    "/tmp/lgtbot_test_bot", // image_path
                    LGTBot_Callback{
                        .get_user_name = GetUserName,
                        .get_user_name_in_group = GetUserNameInGroup,
                        .download_user_avatar = DownloadUserAvatar,
                        .handle_messages = HandleMessages,
                    },
                    std::move(std::get<GameHandleMap>(game_handles)),
                    std::set<UserID>{k_admin_qq},
#ifdef WITH_SQLITE
                    std::make_unique<MockDBManager>(),
#endif
                    MutableBotOption{},
                    nlohmann::json{},
                    nullptr));
    }

    MockDBManager& db_manager() { return *static_cast<MockDBManager*>(bot_->db_manager()); }

  protected:
    std::unique_ptr<BotCtx, void(*)(void*)> bot_{nullptr, &LGTBot_Release};

};

#define ASSERT_PUB_MSG(ret, gid, uid, msg)\
do\
{\
  std::cout << "[USER_" << uid <<  " -> GROUP_" << gid << "]" << std::endl << msg << std::endl;\
  ASSERT_EQ((ret), LGTBot_HandlePublicRequest(bot_.get(), (gid), (uid), (msg)));\
} while (0)

#define ASSERT_PRI_MSG(ret, uid, msg)\
do\
{\
  std::cout << "[USER_" << uid <<  " -> BOT]" << std::endl << msg << std::endl;\
  ASSERT_EQ((ret), LGTBot_HandlePrivateRequest(bot_.get(), (uid), (msg)));\
} while (0)

// Normally Start Game

TEST_F(TestBot, join_game_without_player_limit)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 0");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
}

TEST_F(TestBot, start_game_immediately_finish)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 直接结束");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏 单机"); // game starts then finishes immediately
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
}

// Join Not Existing Game

TEST_F(TestBot, pub_join_game_failed)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_MATCH_GROUP_NOT_IN_MATCH, "1", "1", "#加入");
}

TEST_F(TestBot, pub_join_pri_game_failed)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_MATCH_NEED_REQUEST_PRIVATE, "1", "1", "#加入 1");
}

TEST_F(TestBot, pri_join_game_failed)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_MATCH_NOT_EXIST, "1", "#加入 1");
}

TEST_F(TestBot, pri_join_pub_game_failed)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_MATCH_NEED_ID, "1", "#加入");
}

// Repeat New Game

TEST_F(TestBot, pub_repeat_new_game)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏"); // the old match is terminated
}

TEST_F(TestBot, pub_repeat_new_game_other_group)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, "2", "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, pub_repeat_new_pri_game)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, pri_repeat_new_game)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, pri_repeat_new_pub_game)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, "1", "1", "#新游戏 测试游戏");
}

// New game with customed commands

TEST_F(TestBot, new_single_player_game)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏 单机");
  ASSERT_PUB_MSG(EC_MATCH_ALREADY_BEGIN, "1", "2", "#加入");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "1", "准备");
}

TEST_F(TestBot, new_multi_players_game)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏 多人");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
}

TEST_F(TestBot, invalid_init_options_command)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_INVALID_ARGUMENT, "1", "1", "#新游戏 测试游戏 非法指令");
  ASSERT_PUB_MSG(EC_MATCH_GROUP_NOT_IN_MATCH, "1", "2", "#加入");
}

// New game when group already has game

TEST_F(TestBot, terminate_not_begin_match_when_new_game)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#加入");
}

TEST_F(TestBot, cannot_terminate_gaming_match_when_new_game)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
  ASSERT_PUB_MSG(EC_MATCH_ALREADY_BEGIN, "1", "2", "#新游戏 测试游戏");
}

// Join Self Game

TEST_F(TestBot, pub_join_self_game)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, "1", "1", "#加入");
}

TEST_F(TestBot, pri_join_self_game)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, "1", "#加入 1");
}

// Join Other Game After Creating Game

TEST_F(TestBot, pub_join_other_pub_game)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "2", "2", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_USER_ALREADY_IN_OTHER_MATCH, "2", "1", "#加入");
}

TEST_F(TestBot, pub_join_other_pri_game)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_USER_ALREADY_IN_OTHER_MATCH, "1", "#加入 2");
}

TEST_F(TestBot, pri_join_other_pub_game)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "2", "2", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_USER_ALREADY_IN_OTHER_MATCH, "2", "1", "#加入");
}

TEST_F(TestBot, pri_join_other_pri_game)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_USER_ALREADY_IN_OTHER_MATCH, "1", "#加入 2");
}

// Start Game Failed

TEST_F(TestBot, pub_start_game_not_host)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_MATCH_NOT_HOST, "1", "2", "#开始");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
}

TEST_F(TestBot, pri_start_game_not_host)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_MATCH_NOT_HOST, "2", "#开始");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
}

TEST_F(TestBot, pub_start_game_not_in_game)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_MATCH_USER_NOT_IN_MATCH, "1", "3", "#开始");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
}

TEST_F(TestBot, pri_start_game_not_in_game)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_MATCH_USER_NOT_IN_MATCH, "3", "#开始");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
}

TEST_F(TestBot, pub_start_other_pub_game)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_OK, "2", "3", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "2", "4", "#加入");
  ASSERT_PUB_MSG(EC_MATCH_NOT_THIS_GROUP, "2", "1", "#开始");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
  ASSERT_PUB_MSG(EC_OK, "2", "3", "#开始");
}

TEST_F(TestBot, pri_start_other_pub_game)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PUB_MSG(EC_OK, "2", "3", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "2", "4", "#加入");
  ASSERT_PUB_MSG(EC_MATCH_NOT_THIS_GROUP, "2", "1", "#开始");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PUB_MSG(EC_OK, "2", "3", "#开始");
}

TEST_F(TestBot, pub_game_pri_start)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
}

// Exit Not Existing Game

TEST_F(TestBot, pub_exit_not_exist_game)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_MATCH_USER_NOT_IN_MATCH, "1", "1", "#退出");
}

TEST_F(TestBot, pri_exit_not_exist_game)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_MATCH_USER_NOT_IN_MATCH, "1", "#退出");
}

// Exit And New Game

TEST_F(TestBot, exit_pub_game_then_new_pub_game_same_group)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, exit_pub_game_then_new_pub_game_other_group)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出");
  ASSERT_PUB_MSG(EC_OK, "2", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_GROUP_NOT_IN_MATCH, "1", "2", "#加入");
}

TEST_F(TestBot, exit_pri_game_then_new_pub_game)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#退出");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_NOT_EXIST, "2", "#加入 1");
}

TEST_F(TestBot, exit_pri_game_then_new_pri_game)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#退出");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_NOT_EXIST, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 2");
}

// Exit And Join Game

TEST_F(TestBot, exit_pub_game_then_join_pub_game_same_group)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#加入");
}

TEST_F(TestBot, exit_pub_game_then_join_pub_game_other_group)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "2", "2", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出");
  ASSERT_PUB_MSG(EC_OK, "2", "1", "#加入");
}

TEST_F(TestBot, exit_pri_game_then_join_pub_game)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#退出");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#加入");
}

TEST_F(TestBot, exit_pri_game_then_join_pri_game)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#退出");
  ASSERT_PRI_MSG(EC_MATCH_NOT_EXIST, "1", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#加入 2");
}

// Switch Host

TEST_F(TestBot, switch_host)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出");
  ASSERT_PUB_MSG(EC_OK, "1", "3", "#加入");
  ASSERT_PUB_MSG(EC_MATCH_USER_NOT_IN_MATCH, "1", "1", "#开始");
  ASSERT_PUB_MSG(EC_MATCH_NOT_HOST, "1", "3", "#开始");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#开始");
}

// Exit During Game

TEST_F(TestBot, exit_non_force_during_game)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
  ASSERT_PUB_MSG(EC_MATCH_ALREADY_BEGIN, "1", "1", "#退出");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出 强制");
  ASSERT_PUB_MSG(EC_MATCH_ALREADY_BEGIN, "1", "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, force_exit)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出 强制");
  ASSERT_PUB_MSG(EC_MATCH_ALREADY_BEGIN, "1", "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, force_exit_when_other_ready)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
  ASSERT_PUB_MSG(EC_GAME_REQUEST_OK, "1", "2", "准备");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出 强制");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, force_exit_auto_ready)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
  ASSERT_PUB_MSG(EC_GAME_REQUEST_OK, "1", "2", "准备切换 1");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出 强制");
  ASSERT_PUB_MSG(EC_GAME_REQUEST_CHECKOUT, "1", "2", "准备");
  ASSERT_PUB_MSG(EC_GAME_REQUEST_CHECKOUT, "1", "2", "准备");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, force_exit_computer)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#替补至 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
  ASSERT_PUB_MSG(EC_GAME_REQUEST_OK, "1", "1", "准备切换 5");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出 强制");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, force_exit_computer_multi)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 5");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#替补至 5");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
  ASSERT_PUB_MSG(EC_GAME_REQUEST_OK, "1", "1", "准备切换 5");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出 强制");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, force_exit_computer_multi_failed)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 5");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#替补至 5");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
  ASSERT_PUB_MSG(EC_GAME_REQUEST_OK, "1", "1", "准备切换 5");
  ASSERT_PUB_MSG(EC_GAME_REQUEST_OK, "1", "1", "电脑失败 1 5");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出 强制");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, force_exit_computer_multi_all_ready_continue)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 5");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#替补至 5");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
  ASSERT_PUB_MSG(EC_GAME_REQUEST_OK, "1", "1", "准备切换 5");
  ASSERT_PUB_MSG(EC_GAME_REQUEST_OK, "1", "1", "全员重新准备 10");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出 强制");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, all_force_exit_checkout)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 5");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_OK, "1", "3", "#加入");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
  ASSERT_PUB_MSG(EC_GAME_REQUEST_OK, "1", "1", "准备切换 1");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出 强制");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#退出 强制");
  ASSERT_PUB_MSG(EC_OK, "1", "3", "#退出 强制");
  // game should auto run and over
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, all_force_exit_timeout)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 5");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_OK, "1", "3", "#加入");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
  ASSERT_PUB_MSG(EC_GAME_REQUEST_OK, "1", "1", "重新计时");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出 强制");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#退出 强制");
  ASSERT_PUB_MSG(EC_OK, "1", "3", "#退出 强制");
  // game should auto run and over
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, all_force_exit_all_ready)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 5");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_OK, "1", "3", "#加入");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
  ASSERT_PUB_MSG(EC_GAME_REQUEST_OK, "1", "1", "全员重新准备 1");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出 强制");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#退出 强制");
  ASSERT_PUB_MSG(EC_OK, "1", "3", "#退出 强制");
  // game should auto run and over
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
}

// Config Game

TEST_F(TestBot, config_game)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_GAME_REQUEST_OK, "1", "1", "时限 1");
}

TEST_F(TestBot, config_game_not_host)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_MATCH_NOT_HOST, "1", "2", "时限 1");
}


TEST_F(TestBot, config_game_kick_joined_player)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_GAME_REQUEST_OK, "1", "1", "时限 1");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
}

// Player Limit

TEST_F(TestBot, exceed_max_player)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_MATCH_ACHIEVE_MAX_PLAYER, "1", "3", "#加入");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
}

// Administor

TEST_F(TestBot, interrupt_private_without_mid)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_MATCH_NEED_REQUEST_PUBLIC, k_admin_qq, "%中断");
}

TEST_F(TestBot, interrupt_public_not_game)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_MATCH_GROUP_NOT_IN_MATCH, "1", k_admin_qq, "%中断");
}

TEST_F(TestBot, interrupt_public)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", k_admin_qq, "%中断");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, interrupt_public_wait)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_OK, "1", k_admin_qq, "%中断");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
}

TEST_F(TestBot, interrupt_public_start)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
  ASSERT_PUB_MSG(EC_OK, "1", k_admin_qq, "%中断");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
}

TEST_F(TestBot, interrupt_private_wait)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%中断 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, interrupt_private)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%中断 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 2");
}

TEST_F(TestBot, interrupt_private_start)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%中断 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 2");
}

TEST_F(TestBot, interrupt_private_wait_in_public)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "999", k_admin_qq, "%中断 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, interrupt_private_in_public)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PUB_MSG(EC_OK, "999", k_admin_qq, "%中断 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 2");
}

TEST_F(TestBot, interrupt_private_start_in_public)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PUB_MSG(EC_OK, "999", k_admin_qq, "%中断 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
}

// Computer Player

TEST_F(TestBot, set_computer)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 4");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#替补至 4");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "1", "电脑行动次数 3");
}

TEST_F(TestBot, set_computer_no_limit)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 0");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#替补至 12");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "1", "电脑行动次数 11");
}

TEST_F(TestBot, set_computer_not_host)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 4");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_MATCH_NOT_HOST, "2", "#替补至 4");
}

TEST_F(TestBot, set_computer_and_player_enough)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 5");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#替补至 4");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "电脑行动次数 2");
}

TEST_F(TestBot, set_computer_but_player_enough)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 3");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#替补至 2");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "电脑行动次数 0");
}

TEST_F(TestBot, computer_exceed_max_player)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_ACHIEVE_MAX_PLAYER, "1", "#替补至 3");
  ASSERT_PRI_MSG(EC_OK, "2", ("#加入 1"));
}

TEST_F(TestBot, computer_kick_joined_player)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 3");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", ("#加入 1"));
  ASSERT_PRI_MSG(EC_OK, "1", "#替补至 3");
  ASSERT_PRI_MSG(EC_OK, "2", ("#加入 1"));
}

TEST_F(TestBot, computer_leave_when_all_users_eliminated)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 3");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#替补至 3");
  ASSERT_PRI_MSG(EC_OK, "2", ("#加入 1"));
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "准备切换 5");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "淘汰");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "2", "准备");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "2", "淘汰");
  // Now all computers leave because all users are eliminated.
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏"); // game is over
}

TEST_F(TestBot, auto_set_ready_when_other_players_are_computer_should_checkout)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#替补至 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");

  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "别人重新准备");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_CONTINUE, "1", "准备");

  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏"); // match 1 is over
}

// Test Game

TEST_F(TestBot, game_over_by_request)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "1", "结束子阶段");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, checkout_substage_by_request)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "准备切换 1");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "1", "结束子阶段");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "1", "结束子阶段");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, leave_and_join_other_game)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_OK, "1", "#退出 强制");

  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "3", "#加入 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");

  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "2", "结束子阶段");
  ASSERT_PRI_MSG(EC_OK, "2", "#新游戏 测试游戏"); // match 1 is over

  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "准备");
}

TEST_F(TestBot, auto_set_ready_when_other_players_have_left_should_checkout)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");

  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "准备");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "别人重新准备");
  ASSERT_PRI_MSG(EC_OK, "2", "#退出 强制");

  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏"); // match 1 is over
}

// Eliminate

TEST_F(TestBot, eliminate_first)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "准备切换 1");

  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "淘汰");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "2", "准备");

  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "2", "准备");

  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, eliminate_last)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "准备切换 1");

  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "2", "准备");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "1", "淘汰");

  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "2", "准备");

  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, eliminate_leave_need_not_force)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "淘汰");
  ASSERT_PRI_MSG(EC_OK, "1", "#退出");
}

TEST_F(TestBot, auto_set_ready_when_other_players_have_eliminated_should_checkout)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");

  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "准备");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "别人重新准备");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "2", "淘汰");

  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏"); // match 1 is over
}

TEST_F(TestBot, all_players_elimindated)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");

  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "准备切换 1");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "淘汰");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "2", "淘汰");

  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏"); // match 1 is over
}

TEST_F(TestBot, all_players_elimindated_or_leaved)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");

  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "准备切换 1");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "淘汰");
  ASSERT_PRI_MSG(EC_OK, "2", "#退出 强制");

  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏"); // match 1 is over
}

TEST_F(TestBot, elimdated_when_others_hooked_does_not_finish_game)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");

  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "准备切换 1");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "挂机");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "2", "淘汰");

  ASSERT_PRI_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, "1", "#新游戏 测试游戏"); // match 1 is NOT over
}

// Record Score

TEST_F(TestBot, record_score)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "分数 1");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "2", "分数 2");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "准备");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "2", "准备");
  ASSERT_EQ(2, db_manager().match_profiles_.size());
}

TEST_F(TestBot, not_released_game_not_record)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%计分 测试游戏 关闭");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "分数 1");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "2", "分数 2");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "准备");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "2", "准备");
  ASSERT_EQ(0, db_manager().match_profiles_.size());
}

TEST_F(TestBot, one_player_game_not_record)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#替补至 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "分数 1");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "1", "准备");
  ASSERT_EQ(0, db_manager().match_profiles_.size());
}

TEST_F(TestBot, all_player_leave_not_record)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出 强制");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#退出 强制");
  ASSERT_EQ(0, db_manager().match_profiles_.size());
}

// User Interrupt

TEST_F(TestBot, user_interrupt_game)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_OK, "1", "#中断");
  ASSERT_PRI_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#中断");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, user_interrupt_game_cancel)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_OK, "1", "#中断");
  ASSERT_PRI_MSG(EC_OK, "1", "#中断 取消");
  ASSERT_PRI_MSG(EC_OK, "2", "#中断");
  ASSERT_PRI_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, user_interrupt_game_not_consider_left_users)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_OK, "2", "#退出 强制");
  ASSERT_PRI_MSG(EC_OK, "1", "#中断");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, user_interrupt_when_someone_eliminates)
{
  // Users should wait the elimindated users to determine whether to interrupt the game. It is because the elimindated users may
  // have gotten the highest score.
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");

  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "2", "淘汰");
  ASSERT_PRI_MSG(EC_OK, "1", "#中断");
  ASSERT_PRI_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, "1", "#新游戏 测试游戏");

  ASSERT_PRI_MSG(EC_OK, "2", "#退出 强制");
  ASSERT_PRI_MSG(EC_OK, "1", "#中断");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, user_interrupt_when_someone_hooks)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_OK, "1", "#中断");
  ASSERT_PRI_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "2", "挂机");
  ASSERT_PRI_MSG(EC_OK, "1", "#中断"); // must interrupt again to finish match
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, user_interrupt_when_someone_has_hooked)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "挂机");
  ASSERT_PRI_MSG(EC_OK, "2", "#中断");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, user_interrupt_before_game_starts)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_MATCH_NOT_BEGIN, "1", "#中断");
  ASSERT_PRI_MSG(EC_MATCH_NOT_BEGIN, "2", "#中断");
  ASSERT_PRI_MSG(EC_MATCH_NOT_BEGIN, "1", "#中断 取消");
  ASSERT_PRI_MSG(EC_MATCH_NOT_BEGIN, "2", "#中断 取消");
}

// Achievement

TEST_F(TestBot, get_achievement)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 3");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "3", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "成就 1");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "准备");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "2", "成就 2"); // can achieve same achievement for several times in one match
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "2", "准备");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "3", "准备");
  ASSERT_EQ(1, db_manager().user_achievements_[UserID("1")].size());
  ASSERT_EQ(2, db_manager().user_achievements_[UserID("2")].size());
  ASSERT_EQ(0, db_manager().user_achievements_[UserID("3")].size());
  ASSERT_EQ("普通成就", db_manager().user_achievements_[UserID("1")][0]);
  ASSERT_EQ("普通成就", db_manager().user_achievements_[UserID("2")][0]);
  ASSERT_EQ("普通成就", db_manager().user_achievements_[UserID("2")][1]);
}

// Public mode reply must not produce a bare "@uid\n" with no text body.
//
// Root cause: MsgSenderGuard::~MsgSenderGuard calls Flush(), so every
// `reply() << "text"` inside a game handler already sends a ReplyResp and
// clears reply_.items before returning.  The subsequent flush_out() call in
// HandleExecute then sends a *second*, empty ReplyResp.  The host process
// wraps each ReplyResp in a new PublicReplyMsgSender guard (which prepends
// "@uid\n"), so the empty one produces a bare "@uid\n" message.
//
// Fix: ReplySender::Flush() skips sending when items is empty.
TEST_F(TestBot, pub_game_request_reply_has_no_extra_bare_at_message)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏 单机");

  std::vector<std::string> captured;
  g_captured_messages = &captured;
  // "准备" writes nothing to reply but triggers Boardcast messages via stage
  // transitions.  Before the fix, flush_out() would also emit a bare "@1\n".
  ASSERT_PUB_MSG(EC_GAME_REQUEST_CHECKOUT, "1", "1", "准备");
  g_captured_messages = nullptr;

  // There must be no message consisting solely of "@1\n".
  const std::string bare_at = "[BOT -> GROUP_1]\n@1\n";
  for (const auto& msg : captured) {
      EXPECT_NE(bare_at, msg) << "Got a bare @-only reply; flush_out() sent an empty ReplyResp";
  }
}

// Subprocess Kill

TEST_F(TestBot, subprocess_killed_during_game)
{
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 最大玩家数 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  // "崩溃" triggers _exit(1) in the subprocess; the IPC returns an error.
  ASSERT_PRI_MSG(EC_MATCH_UNEXPECTED_CONFIG, "1", "崩溃");
  // Wait for the read thread's OnEof to finish cleaning up the match.
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  // Match is now terminated; players can start a new game.
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
}

// Rule Command

TEST_F(TestBot, custom_rule_valid_command)
{
  std::vector<std::string> captured;
  g_captured_messages = &captured;
  ASSERT_PRI_MSG(EC_OK, "1", "#规则 测试游戏 细节");
  g_captured_messages = nullptr;
  ASSERT_FALSE(captured.empty());
  EXPECT_NE(std::string::npos, captured[0].find("这是测试规则细节"))
      << "Expected rule detail text, got: " << captured[0];
}

TEST_F(TestBot, custom_rule_invalid_command)
{
  ASSERT_PRI_MSG(EC_INVALID_ARGUMENT, "1", "#规则 测试游戏 不存在的指令");
}

int main(int argc, char** argv)
{
  lgtbot::InstallDefaultSignalHandlersOnce();
  testing::InitGoogleTest(&argc, argv);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  g_argc = argc;
  g_argv = argv;
  return RUN_ALL_TESTS();
}
