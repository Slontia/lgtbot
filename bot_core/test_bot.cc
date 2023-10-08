// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#if defined EXTEND_ACHIEVEMENT

EXTEND_ACHIEVEMENT(普通成就, "一个专门用来测试的成就")

#elif defined EXTEND_OPTION

EXTEND_OPTION("时间限制", 时限, (ArithChecker<int>(0, 10)), 1)

#else

#include <future>
#include <filesystem>

#include <gtest/gtest.h>
#include <gflags/gflags.h>

#define GAME_OPTION_FILENAME "test_bot.cc"
#define GAME_ACHIEVEMENT_FILENAME "test_bot.cc"
#define GAME_MODULE_NAME test_game
#include "game_framework/game_stage.h"

#include "bot_core/msg_sender.h"
#include "bot_core/bot_core.h"
#include "bot_core/timer.h"
#include "bot_core/bot_ctx.h"
#include "bot_core/game_handle.h"
#include "bot_core/db_manager.h"
#include "bot_core/score_calculation.h"
#include "bot_core/match.h"

static_assert(TEST_BOT);

std::condition_variable Timer::cv_;
bool Timer::skip_timer_ = false;
std::condition_variable Timer::remaining_thread_cv_;
uint64_t Timer::remaining_thread_count_ = 0;
std::mutex Timer::mutex_;

static std::ostream& operator<<(std::ostream& os, const ErrCode e) { return os << errcode2str(e); }

constexpr const char* const k_this_qq = "114514";
constexpr const char* const k_admin_qq = "1919810";
static int g_argc = 0;
static char** g_argv = nullptr;

typedef bool(*InitBotFunc)(const UserID, int argc, char** argv);
typedef ErrCode(*HandlePrivateRequestFunc)(const UserID uid, const char* const msg);
typedef ErrCode(*HandlePublicRequestFunc)(const UserID uid, const GroupID gid, const char* const msg);

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

    virtual RankInfo GetRank(const std::string_view& time_range_begin, const std::string_view& time_range_end) override
    {
        return {};
    }

    virtual GameRankInfo GetLevelScoreRank(const std::string& game_name, const std::string_view& time_range_begin,
            const std::string_view& time_range_end) override
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

class TestBot;

static std::mutex substage_blocked_mutex_;
static std::condition_variable substage_block_request_cv_;
static std::atomic<bool> substage_blocked_;

static void BlockStage()
{
    std::unique_lock<std::mutex> l(substage_blocked_mutex_);
    if (substage_blocked_.exchange(true) == true) {
        throw std::runtime_error("a substage has ready been blocked");
    }
    substage_block_request_cv_.wait(l);
    substage_blocked_ = false;
}

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

std::string GameOption::StatusInfo() const { return ""; }

bool GameOption::ToValid(MsgSenderBase& reply) { return true; }

uint64_t GameOption::BestPlayerNum() const { return 2; }

class MainStage;
template <typename... SubStages> using SubGameStage = GameStage<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = GameStage<void, SubStages...>;

class SubStage : public SubGameStage<>
{
  public:
    SubStage(MainStage& main_stage)
        : GameStage(main_stage, "子阶段"
                , MakeStageCommand("结束", &SubStage::Over_, VoidChecker("结束子阶段"))
                , MakeStageCommand("时间到时重新计时", &SubStage::ToResetTimer_, VoidChecker("重新计时"))
                , MakeStageCommand("所有人准备好时重置准备情况", &SubStage::ToResetReady_, VoidChecker("重新准备"), ArithChecker(0, 10))
                , MakeStageCommand("阻塞", &SubStage::Block_, VoidChecker("阻塞"))
                , MakeStageCommand("阻塞并准备", &SubStage::BlockAndReady_, VoidChecker("阻塞并准备"))
                , MakeStageCommand("阻塞并结束", &SubStage::BlockAndOver_, VoidChecker("阻塞并结束"))
                , MakeStageCommand("准备", &SubStage::Ready_, VoidChecker("准备"))
                , MakeStageCommand("断言并清除电脑行动次数", &SubStage::CheckComputerActCount_, VoidChecker("电脑行动次数"), ArithChecker<uint64_t>(0, UINT64_MAX))
                , MakeStageCommand("电脑失败次数", &SubStage::ToComputerFailed_, VoidChecker("电脑失败"),
                    BasicChecker<PlayerID>(), ArithChecker<uint64_t>(0, UINT64_MAX))
                , MakeStageCommand("淘汰", &SubStage::Eliminate_, VoidChecker("淘汰"))
                , MakeStageCommand("挂机", &SubStage::Hook_, VoidChecker("挂机"))
          )
        , computer_act_count_(0)
        , to_reset_timer_(false)
        , to_reset_ready_(0)
        , is_over_(false)
    {}

    virtual void OnStageBegin() override
    {
        Boardcast() << "子阶段开始";
        StartTimer(GET_OPTION_VALUE(option(), 时限));
    }

    virtual CheckoutErrCode OnTimeout() override
    {
        EXPECT_FALSE(is_over_);
        if (to_reset_timer_) {
            to_reset_timer_ = false;
            StartTimer(GET_OPTION_VALUE(option(), 时限));
            Boardcast() << "时间到，但是回合继续";
            return StageErrCode::OK;
        }
        Boardcast() << "时间到，回合结束";
        return StageErrCode::CHECKOUT;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply)
    {
        ++computer_act_count_;
        if (to_computer_failed_[pid] > 0) {
            reply() << "电脑行动失败，剩余次数" << (--to_computer_failed_[pid]);
            return StageErrCode::FAILED;
        }
        return StageErrCode::READY;
    }

    virtual void OnAllPlayerReady()
    {
        if (to_reset_ready_ > 0) {
            --to_reset_ready_;
            ClearReady();
            if (to_reset_timer_) {
                to_reset_timer_ = false;
                StartTimer(GET_OPTION_VALUE(option(), 时限));
                Boardcast() << "全员行动完毕，但是回合继续";
            }
        }
    }

  private:
    AtomReqErrCode Ready_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        return StageErrCode::READY;
    }

    AtomReqErrCode Over_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        is_over_ = true;
        return StageErrCode::CHECKOUT;
    }

    AtomReqErrCode ToResetTimer_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        to_reset_timer_ = true;
        return StageErrCode::OK;
    }

    AtomReqErrCode ToResetReady_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const uint32_t count)
    {
        to_reset_ready_ = count;
        return StageErrCode::OK;
    }

    AtomReqErrCode ToComputerFailed_(const PlayerID pid, const bool is_public, MsgSenderBase& reply,
            const PlayerID failed_pid, const uint32_t count)
    {
        to_computer_failed_[failed_pid] += count;
        return StageErrCode::OK;
    }

    AtomReqErrCode Block_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        BlockStage();
        return StageErrCode::OK;
    }

    AtomReqErrCode BlockAndOver_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        Block_(pid, is_public, reply);
        is_over_ = true;
        return StageErrCode::CHECKOUT;
    }

    AtomReqErrCode BlockAndReady_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        Block_(pid, is_public, reply);
        return StageErrCode::READY;
    }

    AtomReqErrCode CheckComputerActCount_(const PlayerID pid, const bool is_public, MsgSenderBase& reply,
            const uint64_t expected)
    {
        EXPECT_EQ(expected, computer_act_count_);
        computer_act_count_ = 0;
        return StageErrCode::READY;
    }

    AtomReqErrCode Eliminate_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        Eliminate(pid);
        return StageErrCode::OK;
    }

    AtomReqErrCode Hook_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        Hook(pid);
        return StageErrCode::OK;
    }

    uint64_t computer_act_count_;
    bool to_reset_timer_;
    uint32_t to_reset_ready_;
    std::map<PlayerID, uint32_t> to_computer_failed_;
    bool is_over_;
};

class MainStage : public MainGameStage<SubStage>
{
  public:
    MainStage(const GameOption& option, MatchBase& match)
        : GameStage(option, match,
                MakeStageCommand("准备切换", &MainStage::ToCheckout_, VoidChecker("准备切换"), ArithChecker(0, 10)),
                MakeStageCommand("设置玩家分数", &MainStage::Score_, VoidChecker("分数"), ArithChecker<int64_t>(-10, 10)),
                MakeStageCommand("获得成就", &MainStage::Achievement_, VoidChecker("成就"), ArithChecker<uint8_t>(1, 10)))
        , to_checkout_(0)
        , scores_(option.PlayerNum(), 0)
    {}

    virtual VariantSubStage OnStageBegin() override
    {
        return std::make_unique<SubStage>(*this);
    }

    virtual VariantSubStage NextSubStage(SubStage& sub_stage, const CheckoutReason reason) override
    {
        if (to_checkout_) {
            Boardcast() << "回合结束，切换子阶段";
            --to_checkout_;
            return std::make_unique<SubStage>(*this);
        }
        Boardcast() << "回合结束，游戏结束";
        for (const PlayerID pid : achievement_pids_) {
            global_info().Achieve(pid, Achievement::普通成就);
        }
        return {};
    }

    virtual int64_t PlayerScore(const PlayerID pid) const override { return scores_[pid]; };

  private:
    CompReqErrCode ToCheckout_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const uint32_t count)
    {
        to_checkout_ = count;
        return StageErrCode::OK;
    }

    CompReqErrCode Score_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int64_t score)
    {
        scores_[pid] = score;
        return StageErrCode::OK;
    }

    CompReqErrCode Achievement_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const uint8_t count)
    {
        for (uint8_t i = 0; i < count; ++i) {
            achievement_pids_.emplace_back(pid);
        }
        return StageErrCode::OK;
    }

    uint32_t to_checkout_;
    std::vector<int64_t> scores_;
    std::vector<PlayerID> achievement_pids_;
};

class AtomMainStage : public MainGameStage<>
{
  public:
    AtomMainStage(const GameOption& option, MatchBase& match)
        : GameStage(option, match,
                MakeStageCommand("阻塞并结束", &AtomMainStage::BlockAndOver_, VoidChecker("阻塞并结束")))
        , is_over_(false)
    {}

    virtual void OnStageBegin() override
    {
        Boardcast() << "原子主阶段开始";
        StartTimer(GET_OPTION_VALUE(option(), 时限));
    }

    virtual int64_t PlayerScore(const PlayerID pid) const override { return 0; };

  private:
    AtomReqErrCode BlockAndOver_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        BlockStage();
        is_over_ = true;
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnTimeout() override
    {
        EXPECT_FALSE(is_over_);
        return StageErrCode::CHECKOUT;
    }

    bool is_over_;
};

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

class TestBot : public testing::Test
{
  public:
    virtual void SetUp() override
    {
        Timer::skip_timer_ = false;
        bot_ = new BotCtx(
                "./", // game_path
                "", // conf_path
                "/tmp/lgtbot_test_bot", // image_path
                LGTBot_Callback{
                    .get_user_name = GetUserName,
                    .get_user_name_in_group = GetUserNameInGroup,
                    .download_user_avatar = DownloadUserAvatar,
                    .handle_messages = HandleMessages,
                },
                GameHandleMap{},
                std::set<UserID>{k_admin_qq},
#ifdef WITH_SQLITE
                std::make_unique<MockDBManager>(),
#endif
                MutableBotOption{},
                nlohmann::json{},
                nullptr
                );
    }

    virtual void TearDown() override
    {
        LGTBot_Release(bot_);
    }

    MockDBManager& db_manager() { return static_cast<MockDBManager&>(*static_cast<BotCtx*>(bot_)->db_manager()); }

  protected:
    template <class MyMainStage = lgtbot::game::GAME_MODULE_NAME::MainStage>
    void AddGame(const char* const name, const uint64_t max_player, const GameHandle::game_options_allocator new_option)
    {
        const_cast<GameHandleMap&>(static_cast<BotCtx*>(bot_)->game_handles_).emplace(name, std::make_unique<GameHandle>(
                    name, name, max_player, "这是规则介绍", std::vector<GameHandle::Achievement>{}, 1, "这是开发者",
                    "这是游戏描述", new_option,
                    [](const lgtbot::game::GameOptionBase* const options) {},
                    [](MsgSenderBase&, const lgtbot::game::GameOptionBase& option, MatchBase& match)
                            -> lgtbot::game::MainStageBase* { return new MyMainStage(static_cast<const lgtbot::game::GAME_MODULE_NAME::GameOption&>(option), match); },
                    [](const lgtbot::game::MainStageBase* const main_stage) { delete main_stage; },
                    [](const char* const msg) -> const char* { return nullptr; },
                    []() {}
        ));
    }

    template <class MyMainStage = lgtbot::game::GAME_MODULE_NAME::MainStage>
    void AddGame(const char* const name, const uint64_t max_player)
    {
        const_cast<GameHandleMap&>(static_cast<BotCtx*>(bot_)->game_handles_).emplace(name, std::make_unique<GameHandle>(
                    name, name, max_player, "这是规则介绍", std::vector<GameHandle::Achievement>{}, 1, "这是开发者",
                    "这是游戏描述", []() -> lgtbot::game::GameOptionBase* { return new lgtbot::game::GAME_MODULE_NAME::GameOption(); },
                    [](const lgtbot::game::GameOptionBase* const options) { delete options; },
                    [](MsgSenderBase&, const lgtbot::game::GameOptionBase& option, MatchBase& match)
                            -> lgtbot::game::MainStageBase* { return new MyMainStage(static_cast<const lgtbot::game::GAME_MODULE_NAME::GameOption&>(option), match); },
                    [](const lgtbot::game::MainStageBase* const main_stage) { delete main_stage; },
                    [](const char* const msg) -> const char* { return nullptr; },
                    []() {}
        ));
    }

    static void SkipTimer()
    {
        std::unique_lock<std::mutex> l(Timer::mutex_);
        Timer::skip_timer_ = true;
        Timer::cv_.notify_all();
    }

    static void WaitTimerThreadFinish()
    {
        std::unique_lock<std::mutex> l(Timer::mutex_);
        Timer::remaining_thread_cv_.wait(l, [] { return Timer::remaining_thread_count_ == 0; });
    }

    void WaitBeforeHandleTimeout(UserID uid)
    {
        auto& match = *static_cast<BotCtx*>(bot_)->match_manager().GetMatch(uid);
        std::unique_lock<std::mutex> l(match.before_handle_timeout_mutex_);
        match.before_handle_timeout_cv_.wait(l, [&match] { return match.before_handle_timeout_; });
    }

    static void BlockTimer()
    {
        Timer::skip_timer_ = false;
    }

    void NotifySubStage()
    {
        substage_block_request_cv_.notify_all();
    }

    void WaitSubStageBlock()
    {
        // substage_blocked_ is set true in game request which means the game lock has been held
        while (!substage_blocked_.load());
    }

    void* bot_ = nullptr;

};

#define ASSERT_PUB_MSG(ret, gid, uid, msg)\
do\
{\
  std::cout << "[USER_" << uid <<  " -> GROUP_" << gid << "]" << std::endl << msg << std::endl;\
  ASSERT_EQ((ret), LGTBot_HandlePublicRequest(bot_, (gid), (uid), (msg)));\
} while (0)

#define ASSERT_PRI_MSG(ret, uid, msg)\
do\
{\
  std::cout << "[USER_" << uid <<  " -> BOT]" << std::endl << msg << std::endl;\
  ASSERT_EQ((ret), LGTBot_HandlePrivateRequest(bot_, (uid), (msg)));\
} while (0)

// Join Not Existing Game

TEST_F(TestBot, pub_join_game_failed)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_MATCH_GROUP_NOT_IN_MATCH, "1", "1", "#加入");
}

TEST_F(TestBot, pub_join_pri_game_failed)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_MATCH_NEED_REQUEST_PRIVATE, "1", "1", "#加入 1");
}

TEST_F(TestBot, pri_join_game_failed)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_MATCH_NOT_EXIST, "1", "#加入 1");
}

TEST_F(TestBot, pri_join_pub_game_failed)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_MATCH_NEED_ID, "1", "#加入");
}

// Repeat New Game

TEST_F(TestBot, pub_repeat_new_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏"); // the old match is terminated
}

TEST_F(TestBot, pub_repeat_new_game_other_group)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, "2", "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, pub_repeat_new_pri_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, pri_repeat_new_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, pri_repeat_new_pub_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, "1", "1", "#新游戏 测试游戏");
}

// New game when group already has game

TEST_F(TestBot, terminate_not_begin_match_when_new_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#加入");
}

TEST_F(TestBot, cannot_terminate_gaming_match_when_new_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
  ASSERT_PUB_MSG(EC_MATCH_ALREADY_BEGIN, "1", "2", "#新游戏 测试游戏");
}

// Join Self Game

TEST_F(TestBot, pub_join_self_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, "1", "1", "#加入");
}

TEST_F(TestBot, pri_join_self_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, "1", "#加入 1");
}

// Join Other Game After Creating Game

TEST_F(TestBot, pub_join_other_pub_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "2", "2", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_USER_ALREADY_IN_OTHER_MATCH, "2", "1", "#加入");
}

TEST_F(TestBot, pub_join_other_pri_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_USER_ALREADY_IN_OTHER_MATCH, "1", "#加入 2");
}

TEST_F(TestBot, pri_join_other_pub_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "2", "2", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_USER_ALREADY_IN_OTHER_MATCH, "2", "1", "#加入");
}

TEST_F(TestBot, pri_join_other_pri_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_USER_ALREADY_IN_OTHER_MATCH, "1", "#加入 2");
}

// Start Game Failed

TEST_F(TestBot, pub_start_game_not_host)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_MATCH_NOT_HOST, "1", "2", "#开始");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
}

TEST_F(TestBot, pri_start_game_not_host)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_MATCH_NOT_HOST, "2", "#开始");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
}

TEST_F(TestBot, pub_start_game_not_in_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_MATCH_USER_NOT_IN_MATCH, "1", "3", "#开始");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
}

TEST_F(TestBot, pri_start_game_not_in_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_MATCH_USER_NOT_IN_MATCH, "3", "#开始");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
}

TEST_F(TestBot, pub_start_other_pub_game)
{
  AddGame("测试游戏", 2);
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
  AddGame("测试游戏", 2);
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
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
}

// Exit Not Existing Game

TEST_F(TestBot, pub_exit_not_exist_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_MATCH_USER_NOT_IN_MATCH, "1", "1", "#退出");
}

TEST_F(TestBot, pri_exit_not_exist_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_MATCH_USER_NOT_IN_MATCH, "1", "#退出");
}

// Exit And New Game

TEST_F(TestBot, exit_pub_game_then_new_pub_game_same_group)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, exit_pub_game_then_new_pub_game_other_group)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出");
  ASSERT_PUB_MSG(EC_OK, "2", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_GROUP_NOT_IN_MATCH, "1", "2", "#加入");
}

TEST_F(TestBot, exit_pri_game_then_new_pub_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#退出");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_NOT_EXIST, "2", "#加入 1");
}

TEST_F(TestBot, exit_pri_game_then_new_pri_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#退出");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_NOT_EXIST, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 2");
}

// Exit And Join Game

TEST_F(TestBot, exit_pub_game_then_join_pub_game_same_group)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#加入");
}

TEST_F(TestBot, exit_pub_game_then_join_pub_game_other_group)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "2", "2", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出");
  ASSERT_PUB_MSG(EC_OK, "2", "1", "#加入");
}

TEST_F(TestBot, exit_pri_game_then_join_pub_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#退出");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#加入");
}

TEST_F(TestBot, exit_pri_game_then_join_pri_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#退出");
  ASSERT_PRI_MSG(EC_MATCH_NOT_EXIST, "1", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#加入 2");
}

// Switch Host

TEST_F(TestBot, switch_host)
{
  AddGame("测试游戏", 2);
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
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
  ASSERT_PUB_MSG(EC_MATCH_ALREADY_BEGIN, "1", "1", "#退出");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出 强制");
  ASSERT_PUB_MSG(EC_MATCH_ALREADY_BEGIN, "1", "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, force_exit)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出 强制");
  ASSERT_PUB_MSG(EC_MATCH_ALREADY_BEGIN, "1", "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, force_exit_when_other_ready)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
  ASSERT_PUB_MSG(EC_GAME_REQUEST_OK, "1", "2", "准备");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出 强制");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, force_exit_auto_ready)
{
  AddGame("测试游戏", 2);
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
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#替补至 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
  ASSERT_PUB_MSG(EC_GAME_REQUEST_OK, "1", "1", "准备切换 5");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出 强制");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, force_exit_computer_multi)
{
  AddGame("测试游戏", 5);
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#替补至 5");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
  ASSERT_PUB_MSG(EC_GAME_REQUEST_OK, "1", "1", "准备切换 5");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出 强制");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, force_exit_computer_multi_failed)
{
  AddGame("测试游戏", 5);
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
  AddGame("测试游戏", 5);
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#替补至 5");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
  ASSERT_PUB_MSG(EC_GAME_REQUEST_OK, "1", "1", "准备切换 5");
  ASSERT_PUB_MSG(EC_GAME_REQUEST_OK, "1", "1", "重新准备 10");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出 强制");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, all_force_exit_checkout)
{
  AddGame("测试游戏", 5);
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
  AddGame("测试游戏", 5);
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
  AddGame("测试游戏", 5);
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_OK, "1", "3", "#加入");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
  ASSERT_PUB_MSG(EC_GAME_REQUEST_OK, "1", "1", "重新准备 1");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出 强制");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#退出 强制");
  ASSERT_PUB_MSG(EC_OK, "1", "3", "#退出 强制");
  // game should auto run and over
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
}

// Config Game

TEST_F(TestBot, config_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_GAME_REQUEST_OK, "1", "1", "时限 1");
}

TEST_F(TestBot, config_game_not_host)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_MATCH_NOT_HOST, "1", "2", "时限 1");
}


TEST_F(TestBot, config_game_kick_joined_player)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_GAME_REQUEST_OK, "1", "1", "时限 1");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
}

// Player Limit

TEST_F(TestBot, exceed_max_player)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_MATCH_ACHIEVE_MAX_PLAYER, "1", "3", "#加入");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
}

// Administor

TEST_F(TestBot, interrupt_private_without_mid)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_MATCH_NEED_REQUEST_PUBLIC, k_admin_qq, "%中断");
}

TEST_F(TestBot, interrupt_public_not_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_MATCH_GROUP_NOT_IN_MATCH, "1", k_admin_qq, "%中断");
}

TEST_F(TestBot, interrupt_public)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", k_admin_qq, "%中断");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, interrupt_public_wait)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_OK, "1", k_admin_qq, "%中断");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
}

TEST_F(TestBot, interrupt_public_start)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
  ASSERT_PUB_MSG(EC_OK, "1", k_admin_qq, "%中断");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
}

TEST_F(TestBot, interrupt_private_wait)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%中断 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, interrupt_private)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%中断 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 2");
}

TEST_F(TestBot, interrupt_private_start)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%中断 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 2");
}

TEST_F(TestBot, interrupt_private_wait_in_public)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "999", k_admin_qq, "%中断 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, interrupt_private_in_public)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PUB_MSG(EC_OK, "999", k_admin_qq, "%中断 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 2");
}

TEST_F(TestBot, interrupt_private_start_in_public)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PUB_MSG(EC_OK, "999", k_admin_qq, "%中断 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
}

// Computer Player

TEST_F(TestBot, set_computer)
{
  AddGame("测试游戏", 5);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#替补至 5");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "1", "电脑行动次数 4");
}

TEST_F(TestBot, set_computer_no_limit)
{
  AddGame("测试游戏", 0);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#替补至 12");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "1", "电脑行动次数 11");
}

TEST_F(TestBot, set_computer_not_host)
{
  AddGame("测试游戏", 5);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_MATCH_NOT_HOST, "2", "#替补至 5");
}

TEST_F(TestBot, set_computer_and_player_enough)
{
  AddGame("测试游戏", 5);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#替补至 4");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "电脑行动次数 2");
}

TEST_F(TestBot, set_computer_but_player_enough)
{
  AddGame("测试游戏", 5);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#替补至 2");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "电脑行动次数 0");
}

TEST_F(TestBot, computer_exceed_max_player)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_ACHIEVE_MAX_PLAYER, "1", "#替补至 3");
  ASSERT_PRI_MSG(EC_OK, "2", ("#加入 1"));
}

TEST_F(TestBot, computer_kick_joined_player)
{
  AddGame("测试游戏", 3);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", ("#加入 1"));
  ASSERT_PRI_MSG(EC_OK, "1", "#替补至 3");
  ASSERT_PRI_MSG(EC_OK, "2", ("#加入 1"));
}

TEST_F(TestBot, computer_leave_when_all_users_eliminated)
{
  AddGame("测试游戏", 3);
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

// Test Game

TEST_F(TestBot, game_over_by_request)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "1", "结束子阶段");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, game_over_by_timeup)
{
  //TODO: support control timer
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  SkipTimer();
  WaitTimerThreadFinish();
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, checkout_substage_by_request)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "准备切换 1");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "1", "结束子阶段");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "1", "结束子阶段");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, checkout_substage_by_timeout)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "准备切换 1");
  SkipTimer();
  WaitTimerThreadFinish();
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, substage_reset_timer)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "重新计时");
  SkipTimer();
  WaitTimerThreadFinish();
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, timeout_during_handle_request_checkout)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");

  auto fut = std::async([this]
        {
            ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "1", "阻塞并结束");
        });
  WaitSubStageBlock();
  SkipTimer();
  WaitBeforeHandleTimeout(UserID{"1"});
  NotifySubStage();
  fut.wait();

  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");

  WaitTimerThreadFinish();
}

TEST_F(TestBot, timeout_during_handle_request_checkout_atomic_main_stage)
{
  AddGame<lgtbot::game::GAME_MODULE_NAME::AtomMainStage>("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");

  auto fut = std::async([this]
        {
            ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "1", "阻塞并结束");
        });
  WaitSubStageBlock();
  SkipTimer();
  WaitBeforeHandleTimeout(UserID{"1"});
  NotifySubStage();
  fut.wait();

  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");

  WaitTimerThreadFinish();
}

TEST_F(TestBot, timeout_during_handle_request_all_ready_and_reset_timer)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");

  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "重新准备 1");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "重新计时");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "准备");

  auto fut = std::async([this]
        {
            ASSERT_PRI_MSG(EC_GAME_REQUEST_CONTINUE, "2", "阻塞并准备");
        });
  WaitSubStageBlock();

  SkipTimer();
  WaitBeforeHandleTimeout(UserID{"1"});
  BlockTimer(); // prevent timeout after resetting timer

  NotifySubStage();
  fut.wait(); // now the timer is reset

  // OnTimeout should not be invoked and game should not be over because timer is reset.
  // So the player can execute 准备 command
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "准备");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "2", "准备");
}

TEST_F(TestBot, leave_during_handle_request)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");

  auto fut_1 = std::async([this]
        {
            ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "1", "阻塞并结束");
        });
  WaitSubStageBlock();
  const auto match_use_count = static_cast<BotCtx*>(bot_)->match_manager().GetMatch(UserID("1")).use_count();
  auto fut_2 = std::async([this]
        {
            ASSERT_PRI_MSG(EC_MATCH_ALREADY_OVER, "1", "#退出 强制");
        });
  // wait leave command get the match reference count
  while (static_cast<BotCtx*>(bot_)->match_manager().GetMatch(UserID("1")).use_count() == match_use_count);
  NotifySubStage();
  fut_1.wait();
  fut_2.wait();

  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, leave_and_join_other_game)
{
  AddGame("测试游戏", 2);
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

// Eliminate

TEST_F(TestBot, eliminate_first)
{
  AddGame("测试游戏", 2);
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
  AddGame("测试游戏", 2);
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
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "淘汰");
  ASSERT_PRI_MSG(EC_OK, "1", "#退出");
}

// Record Score

TEST_F(TestBot, record_score)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%倍率 测试游戏 1");
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
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%倍率 测试游戏 0");
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
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%倍率 测试游戏 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#替补至 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "分数 1");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "1", "准备");
  ASSERT_EQ(0, db_manager().match_profiles_.size());
}

TEST_F(TestBot, all_player_leave_not_record)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%倍率 测试游戏 1");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出 强制");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#退出 强制");
  ASSERT_EQ(0, db_manager().match_profiles_.size());
}

TEST_F(TestBot, score_not_enough_cannot_set_multiple_greater)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_SCORE_NOT_ENOUGH, "1", "1", "#倍率 2");
}

TEST_F(TestBot, score_not_enough_cannot_join_multiple_greater)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  db_manager().user_profiles_["1"].total_zero_sum_score_ = 4000;
  db_manager().user_profiles_["1"].total_top_score_ = 80;
  db_manager().user_profiles_["1"].recent_matches_.emplace_back();
  db_manager().user_profiles_["1"].recent_matches_.emplace_back();
  db_manager().user_profiles_["1"].recent_matches_[0].multiple_ = 1;
  db_manager().user_profiles_["1"].recent_matches_[1].multiple_ = 1;
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#倍率 2");
  ASSERT_PUB_MSG(EC_MATCH_SCORE_NOT_ENOUGH, "1", "2", "#加入");
  ASSERT_PRI_MSG(EC_OK, "2", "#新游戏 测试游戏");
}

TEST_F(TestBot, score_not_enough_can_set_multiple_less_or_equal)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%倍率 测试游戏 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#倍率 1");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#倍率 2");
}

TEST_F(TestBot, set_multiple_effects_zero_sum_score)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%倍率 测试游戏 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "分数 1");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "2", "分数 2");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "准备");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "2", "准备");
  ASSERT_EQ(2, db_manager().match_profiles_.size());
  ASSERT_EQ(2000, db_manager().match_profiles_[0].zero_sum_score_);
  ASSERT_EQ(40, db_manager().match_profiles_[0].top_score_);
}

// User Interrupt

TEST_F(TestBot, user_interrupt_game)
{
  AddGame("测试游戏", 2);
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
  AddGame("测试游戏", 2);
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
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_OK, "2", "#退出 强制");
  ASSERT_PRI_MSG(EC_OK, "1", "#中断");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, user_interrupt_when_someone_eliminates)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_OK, "1", "#中断");
  ASSERT_PRI_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "2", "淘汰");
  ASSERT_PRI_MSG(EC_OK, "1", "#中断"); // must interrupt again to finish match
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, user_interrupt_when_someone_has_eliminated)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "淘汰");
  ASSERT_PRI_MSG(EC_OK, "2", "#中断");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, user_interrupt_when_someone_hooks)
{
  AddGame("测试游戏", 2);
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
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "挂机");
  ASSERT_PRI_MSG(EC_OK, "2", "#中断");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
}

// Achievement

TEST_F(TestBot, get_achievement)
{
  AddGame("测试游戏", 3);
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

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  g_argc = argc;
  g_argv = argv;
  return RUN_ALL_TESTS();
}

#endif
