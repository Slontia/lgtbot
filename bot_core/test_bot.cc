// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#if defined EXTEND_ACHIEVEMENT

EXTEND_ACHIEVEMENT(普通成就, "一个专门用来测试的成就")

#elif defined EXTEND_OPTION

EXTEND_OPTION("时间限制", 时限, (ArithChecker<int>(0, 10)), 1)
EXTEND_OPTION("直接结束游戏", 直接结束, (OptionalDefaultChecker<BoolChecker>(true, "开启", "关闭")), false)

#else

#include <future>
#include <filesystem>

#include <gtest/gtest.h>
#include <gflags/gflags.h>

#include "game_framework/stage.h"

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

static bool g_fail_to_create_game = false;

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly,
        MutableGenericOptions& generic_options)
{
    return true;
}

template <typename T, T V>
constexpr T ValueFunc() { return V; }

uint64_t MaxPlayerNum(const CustomOptions& options) { return 4; }

uint32_t Multiple(const CustomOptions& options) { return 1; }

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;

class SubStage : public SubGameStage<>
{
  public:
    SubStage(MainStage& main_stage)
        : StageFsm(main_stage, "子阶段"
                , MakeStageCommand(*this, "结束", &SubStage::Over_, VoidChecker("结束子阶段"))
                , MakeStageCommand(*this, "时间到时重新计时", &SubStage::ToResetTimer_, VoidChecker("重新计时"))
                , MakeStageCommand(*this, "所有人准备好时重置准备情况", &SubStage::ToResetReadyAll_, VoidChecker("全员重新准备"), ArithChecker(0, 10))
                , MakeStageCommand(*this, "重置准备情况时将除自己外设置为准备完成", &SubStage::ToResetOthersReady_, VoidChecker("别人重新准备"))
                , MakeStageCommand(*this, "阻塞", &SubStage::Block_, VoidChecker("阻塞"))
                , MakeStageCommand(*this, "阻塞并准备", &SubStage::BlockAndReady_, VoidChecker("阻塞并准备"))
                , MakeStageCommand(*this, "阻塞并结束", &SubStage::BlockAndOver_, VoidChecker("阻塞并结束"))
                , MakeStageCommand(*this, "准备", &SubStage::Ready_, VoidChecker("准备"))
                , MakeStageCommand(*this, "断言并清除电脑行动次数", &SubStage::CheckComputerActCount_, VoidChecker("电脑行动次数"), ArithChecker<uint64_t>(0, UINT64_MAX))
                , MakeStageCommand(*this, "电脑失败次数", &SubStage::ToComputerFailed_, VoidChecker("电脑失败"),
                    BasicChecker<PlayerID>(), ArithChecker<uint64_t>(0, UINT64_MAX))
                , MakeStageCommand(*this, "淘汰", &SubStage::Eliminate_, VoidChecker("淘汰"))
                , MakeStageCommand(*this, "挂机", &SubStage::Hook_, VoidChecker("挂机"))
          )
    {}

    virtual void OnStageBegin() override
    {
        Global().Boardcast() << "子阶段开始";
        Global().StartTimer(GAME_OPTION(时限));
        if (GAME_OPTION(直接结束)) {
            Global().Boardcast() << "游戏直接结束";
            for (PlayerID player_id = 0; player_id < Global().PlayerNum(); ++player_id) {
                Global().SetReady(player_id);
            }
        }
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        EXPECT_FALSE(is_over_);
        if (to_reset_timer_) {
            to_reset_timer_ = false;
            Global().StartTimer(GAME_OPTION(时限));
            Global().Boardcast() << "时间到，但是回合继续";
            return StageErrCode::OK;
        }
        Global().Boardcast() << "时间到，回合结束";
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

    virtual CheckoutErrCode OnStageOver()
    {
        if (!to_reset_others_ready_players_.empty()) {
            for (PlayerID player_id = 0; player_id < Global().PlayerNum(); ++player_id) {
                if (to_reset_others_ready_players_.find(player_id) == to_reset_others_ready_players_.end()) {
                    Global().ClearReady(player_id);
                }
            }
            to_reset_others_ready_players_.clear();
        } else if (to_reset_ready_ > 0) {
            --to_reset_ready_;
            Global().ClearReady();
        } else {
            return StageErrCode::CHECKOUT;
        }
        if (to_reset_timer_) {
            to_reset_timer_ = false;
            Global().StartTimer(GAME_OPTION(时限));
            Global().Boardcast() << "全员行动完毕，但是回合继续";
        }
        return StageErrCode::CONTINUE;
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

    AtomReqErrCode ToResetReadyAll_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const uint32_t count)
    {
        to_reset_ready_ = count;
        return StageErrCode::OK;
    }

    AtomReqErrCode ToResetOthersReady_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        to_reset_others_ready_players_.emplace(pid);
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
        Global().Eliminate(pid);
        return StageErrCode::OK;
    }

    AtomReqErrCode Hook_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        Global().Hook(pid);
        return StageErrCode::OK;
    }

    uint64_t computer_act_count_{0};
    bool to_reset_timer_{false};
    uint32_t to_reset_ready_{0};
    std::set<PlayerID> to_reset_others_ready_players_;
    std::map<PlayerID, uint32_t> to_computer_failed_;
    bool is_over_;
};

class MainStage : public MainGameStage<SubStage>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility),
                MakeStageCommand(*this, "准备切换", &MainStage::ToCheckout_, VoidChecker("准备切换"), ArithChecker(0, 10)),
                MakeStageCommand(*this, "设置玩家分数", &MainStage::Score_, VoidChecker("分数"), ArithChecker<int64_t>(-10, 10)),
                MakeStageCommand(*this, "获得成就", &MainStage::Achievement_, VoidChecker("成就"), ArithChecker<uint8_t>(1, 10)))
        , to_checkout_(0)
        , scores_(utility.PlayerNum(), 0)
    {}

    virtual void FirstStageFsm(SubStageFsmSetter setter) override
    {
        setter.Emplace<SubStage>(*this);
    }

    virtual void NextStageFsm(SubStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override
    {
        if (to_checkout_) {
            Global().Boardcast() << "回合结束，切换子阶段";
            --to_checkout_;
            setter.Emplace<SubStage>(*this);
        }
        Global().Boardcast() << "回合结束，游戏结束";
        for (const PlayerID pid : achievement_pids_) {
            Global().Achieve(pid, Achievement::普通成就);
        }
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
    AtomMainStage(StageUtility&& utility)
        : StageFsm(std::move(utility),
                MakeStageCommand(*this, "阻塞并结束", &AtomMainStage::BlockAndOver_, VoidChecker("阻塞并结束")))
        , is_over_(false)
    {}

    virtual void OnStageBegin() override
    {
        Global().Boardcast() << "原子主阶段开始";
        Global().StartTimer(GAME_OPTION(时限));
        if (GAME_OPTION(直接结束)) {
            Global().Boardcast() << "游戏直接结束";
            for (PlayerID player_id = 0; player_id < Global().PlayerNum(); ++player_id) {
                Global().SetReady(player_id);
            }
        }
    }

    virtual int64_t PlayerScore(const PlayerID pid) const override { return 0; };

  private:
    AtomReqErrCode BlockAndOver_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        BlockStage();
        is_over_ = true;
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnStageTimeout() override
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
        g_fail_to_create_game = false;
        Timer::skip_timer_ = false;
        bot_.reset(new BotCtx(
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
                    nullptr));
    }

    MockDBManager& db_manager() { return *static_cast<MockDBManager*>(bot_->db_manager()); }

  protected:
    template <uint64_t k_max_player, class MyMainStage = lgtbot::game::GAME_MODULE_NAME::MainStage>
    void AddGame(const char* const name)
    {
        using namespace lgtbot::game::GAME_MODULE_NAME;

        GameHandle::BasicInfo basic_info;
        basic_info.name_ = name;
        basic_info.developer_ = "测试开发者";
        basic_info.description_ = "用来测试的游戏";
        basic_info.module_name_ = name;
        basic_info.rule_ = "没有规则";
        basic_info.max_player_num_fn_ = [](const lgtbot::game::GameOptionsBase*) -> uint64_t { return k_max_player; };
        basic_info.multiple_fn_ = [](const lgtbot::game::GameOptionsBase*) -> uint32_t { return 1; };
        basic_info.handle_rule_command_fn_ = [](const char*) -> const char* { return nullptr; };
        basic_info.handle_init_options_command_fn_ =
            [](const char* const cmd, lgtbot::game::GameOptionsBase*, lgtbot::game::MutableGenericOptions*)
                -> lgtbot::game::InitOptionsResult
            {
                const std::string_view cmd_sv{cmd};
                if (cmd_sv.find("单机") != std::string_view::npos) {
                    return lgtbot::game::InitOptionsResult::NEW_SINGLE_USER_MODE_GAME;
                }
                if (cmd_sv.find("多人") != std::string_view::npos) {
                    return lgtbot::game::InitOptionsResult::NEW_MULTIPLE_USERS_MODE_GAME;
                }
                return lgtbot::game::InitOptionsResult::INVALID_INIT_OPTIONS_COMMAND;
            };

        bot_->game_handles().emplace(std::piecewise_construct, std::forward_as_tuple(name), std::forward_as_tuple(
                    std::move(basic_info),
                    GameHandle::InternalHandler{
                        .game_options_allocator_ = []() -> lgtbot::game::GameOptionsBase* { return new lgtbot::game::GAME_MODULE_NAME::GameOptions(); },
                        .game_options_deleter_ = [](const lgtbot::game::GameOptionsBase* const options) { delete options; },
                        .main_stage_allocator_ =
                            [](MsgSenderBase*, lgtbot::game::GameOptionsBase* const game_options,
                                    lgtbot::game::GenericOptions* const generic_options, MatchBase* const match)
                                -> lgtbot::game::MainStageBase*
                            {
                                if (g_fail_to_create_game) {
                                    return nullptr;
                                }
                                auto* const my_game_options = static_cast<lgtbot::game::GAME_MODULE_NAME::GameOptions*>(game_options);
                                return new internal::MainStage(std::make_unique<MyMainStage>(StageUtility{*my_game_options, *generic_options, *match}));
                            },
                        .main_stage_deleter_ = [](const lgtbot::game::MainStageBase* const main_stage) { delete main_stage; },
                        .mod_guard_ = [] {},
                    },
                    lgtbot::game::MutableGenericOptions{}
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
        auto& match = *bot_->match_manager().GetMatch(uid);
        std::unique_lock<std::mutex> l(match.timer_cntl_.before_handle_timeout_mutex_);
        match.timer_cntl_.before_handle_timeout_cv_.wait(l, [&match] { return match.timer_cntl_.before_handle_timeout_; });
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
  AddGame<0>("测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
}

TEST_F(TestBot, start_game_after_start_failure)
{
  // It reproduce the bug where different users share the same player ID.
  AddGame<0>("测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#新游戏 测试游戏");
  g_fail_to_create_game = true;
  ASSERT_PUB_MSG(EC_MATCH_UNEXPECTED_CONFIG, "1", "2", "#开始");
  g_fail_to_create_game = false;
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#加入");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "准备");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "2", "准备");
}

TEST_F(TestBot, start_game_immediately_finish)
{
  AddGame<0>("测试游戏");
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%配置 测试游戏 直接结束");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏 单机"); // game starts then finishes immediately
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
}

// Join Not Existing Game

TEST_F(TestBot, pub_join_game_failed)
{
  AddGame<2>("测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_GROUP_NOT_IN_MATCH, "1", "1", "#加入");
}

TEST_F(TestBot, pub_join_pri_game_failed)
{
  AddGame<2>("测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_NEED_REQUEST_PRIVATE, "1", "1", "#加入 1");
}

TEST_F(TestBot, pri_join_game_failed)
{
  AddGame<2>("测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_NOT_EXIST, "1", "#加入 1");
}

TEST_F(TestBot, pri_join_pub_game_failed)
{
  AddGame<2>("测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_NEED_ID, "1", "#加入");
}

// Repeat New Game

TEST_F(TestBot, pub_repeat_new_game)
{
  AddGame<2>("测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏"); // the old match is terminated
}

TEST_F(TestBot, pub_repeat_new_game_other_group)
{
  AddGame<2>("测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, "2", "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, pub_repeat_new_pri_game)
{
  AddGame<2>("测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, pri_repeat_new_game)
{
  AddGame<2>("测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, pri_repeat_new_pub_game)
{
  AddGame<2>("测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, "1", "1", "#新游戏 测试游戏");
}

// New game with customed commands

TEST_F(TestBot, new_single_player_game)
{
  AddGame<2>("测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏 单机");
  ASSERT_PUB_MSG(EC_MATCH_ALREADY_BEGIN, "1", "2", "#加入");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "1", "准备");
}

TEST_F(TestBot, new_multi_players_game)
{
  AddGame<2>("测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏 多人");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
}

TEST_F(TestBot, invalid_init_options_command)
{
  AddGame<2>("测试游戏");
  ASSERT_PUB_MSG(EC_INVALID_ARGUMENT, "1", "1", "#新游戏 测试游戏 非法指令");
  ASSERT_PUB_MSG(EC_MATCH_GROUP_NOT_IN_MATCH, "1", "2", "#加入");
}

// New game when group already has game

TEST_F(TestBot, terminate_not_begin_match_when_new_game)
{
  AddGame<2>("测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#加入");
}

TEST_F(TestBot, cannot_terminate_gaming_match_when_new_game)
{
  AddGame<2>("测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
  ASSERT_PUB_MSG(EC_MATCH_ALREADY_BEGIN, "1", "2", "#新游戏 测试游戏");
}

// Join Self Game

TEST_F(TestBot, pub_join_self_game)
{
  AddGame<2>("测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, "1", "1", "#加入");
}

TEST_F(TestBot, pri_join_self_game)
{
  AddGame<2>("测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, "1", "#加入 1");
}

// Join Other Game After Creating Game

TEST_F(TestBot, pub_join_other_pub_game)
{
  AddGame<2>("测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "2", "2", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_USER_ALREADY_IN_OTHER_MATCH, "2", "1", "#加入");
}

TEST_F(TestBot, pub_join_other_pri_game)
{
  AddGame<2>("测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_USER_ALREADY_IN_OTHER_MATCH, "1", "#加入 2");
}

TEST_F(TestBot, pri_join_other_pub_game)
{
  AddGame<2>("测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "2", "2", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_USER_ALREADY_IN_OTHER_MATCH, "2", "1", "#加入");
}

TEST_F(TestBot, pri_join_other_pri_game)
{
  AddGame<2>("测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_USER_ALREADY_IN_OTHER_MATCH, "1", "#加入 2");
}

// Start Game Failed

TEST_F(TestBot, pub_start_game_not_host)
{
  AddGame<2>("测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_MATCH_NOT_HOST, "1", "2", "#开始");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
}

TEST_F(TestBot, pri_start_game_not_host)
{
  AddGame<2>("测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_MATCH_NOT_HOST, "2", "#开始");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
}

TEST_F(TestBot, pub_start_game_not_in_game)
{
  AddGame<2>("测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_MATCH_USER_NOT_IN_MATCH, "1", "3", "#开始");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
}

TEST_F(TestBot, pri_start_game_not_in_game)
{
  AddGame<2>("测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_MATCH_USER_NOT_IN_MATCH, "3", "#开始");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
}

TEST_F(TestBot, pub_start_other_pub_game)
{
  AddGame<2>("测试游戏");
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
  AddGame<2>("测试游戏");
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
  AddGame<2>("测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
}

// Exit Not Existing Game

TEST_F(TestBot, pub_exit_not_exist_game)
{
  AddGame<2>("测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_USER_NOT_IN_MATCH, "1", "1", "#退出");
}

TEST_F(TestBot, pri_exit_not_exist_game)
{
  AddGame<2>("测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_USER_NOT_IN_MATCH, "1", "#退出");
}

// Exit And New Game

TEST_F(TestBot, exit_pub_game_then_new_pub_game_same_group)
{
  AddGame<2>("测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, exit_pub_game_then_new_pub_game_other_group)
{
  AddGame<2>("测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出");
  ASSERT_PUB_MSG(EC_OK, "2", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_GROUP_NOT_IN_MATCH, "1", "2", "#加入");
}

TEST_F(TestBot, exit_pri_game_then_new_pub_game)
{
  AddGame<2>("测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#退出");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_NOT_EXIST, "2", "#加入 1");
}

TEST_F(TestBot, exit_pri_game_then_new_pri_game)
{
  AddGame<2>("测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#退出");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_NOT_EXIST, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 2");
}

// Exit And Join Game

TEST_F(TestBot, exit_pub_game_then_join_pub_game_same_group)
{
  AddGame<2>("测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#加入");
}

TEST_F(TestBot, exit_pub_game_then_join_pub_game_other_group)
{
  AddGame<2>("测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "2", "2", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出");
  ASSERT_PUB_MSG(EC_OK, "2", "1", "#加入");
}

TEST_F(TestBot, exit_pri_game_then_join_pub_game)
{
  AddGame<2>("测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#退出");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#加入");
}

TEST_F(TestBot, exit_pri_game_then_join_pri_game)
{
  AddGame<2>("测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#退出");
  ASSERT_PRI_MSG(EC_MATCH_NOT_EXIST, "1", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#加入 2");
}

// Switch Host

TEST_F(TestBot, switch_host)
{
  AddGame<2>("测试游戏");
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
  AddGame<2>("测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
  ASSERT_PUB_MSG(EC_MATCH_ALREADY_BEGIN, "1", "1", "#退出");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出 强制");
  ASSERT_PUB_MSG(EC_MATCH_ALREADY_BEGIN, "1", "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, force_exit)
{
  AddGame<2>("测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出 强制");
  ASSERT_PUB_MSG(EC_MATCH_ALREADY_BEGIN, "1", "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, force_exit_when_other_ready)
{
  AddGame<2>("测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
  ASSERT_PUB_MSG(EC_GAME_REQUEST_OK, "1", "2", "准备");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出 强制");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, force_exit_auto_ready)
{
  AddGame<2>("测试游戏");
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
  AddGame<2>("测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#替补至 2");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
  ASSERT_PUB_MSG(EC_GAME_REQUEST_OK, "1", "1", "准备切换 5");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出 强制");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, force_exit_computer_multi)
{
  AddGame<5>("测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#替补至 5");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
  ASSERT_PUB_MSG(EC_GAME_REQUEST_OK, "1", "1", "准备切换 5");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#退出 强制");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, force_exit_computer_multi_failed)
{
  AddGame<5>("测试游戏");
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
  AddGame<5>("测试游戏");
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
  AddGame<5>("测试游戏");
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
  AddGame<5>("测试游戏");
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
  AddGame<5>("测试游戏");
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
  AddGame<2>("测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_GAME_REQUEST_OK, "1", "1", "时限 1");
}

TEST_F(TestBot, config_game_not_host)
{
  AddGame<2>("测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_MATCH_NOT_HOST, "1", "2", "时限 1");
}


TEST_F(TestBot, config_game_kick_joined_player)
{
  AddGame<2>("测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_GAME_REQUEST_OK, "1", "1", "时限 1");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
}

// Player Limit

TEST_F(TestBot, exceed_max_player)
{
  AddGame<2>("测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_MATCH_ACHIEVE_MAX_PLAYER, "1", "3", "#加入");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
}

// Administor

TEST_F(TestBot, interrupt_private_without_mid)
{
  AddGame<2>("测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_NEED_REQUEST_PUBLIC, k_admin_qq, "%中断");
}

TEST_F(TestBot, interrupt_public_not_game)
{
  AddGame<2>("测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_GROUP_NOT_IN_MATCH, "1", k_admin_qq, "%中断");
}

TEST_F(TestBot, interrupt_public)
{
  AddGame<2>("测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", k_admin_qq, "%中断");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, interrupt_public_wait)
{
  AddGame<2>("测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_OK, "1", k_admin_qq, "%中断");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
}

TEST_F(TestBot, interrupt_public_start)
{
  AddGame<2>("测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#开始");
  ASSERT_PUB_MSG(EC_OK, "1", k_admin_qq, "%中断");
  ASSERT_PUB_MSG(EC_OK, "1", "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "1", "2", "#加入");
}

TEST_F(TestBot, interrupt_private_wait)
{
  AddGame<2>("测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%中断 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, interrupt_private)
{
  AddGame<2>("测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%中断 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 2");
}

TEST_F(TestBot, interrupt_private_start)
{
  AddGame<2>("测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%中断 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 2");
}

TEST_F(TestBot, interrupt_private_wait_in_public)
{
  AddGame<2>("测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, "999", k_admin_qq, "%中断 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, interrupt_private_in_public)
{
  AddGame<2>("测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PUB_MSG(EC_OK, "999", k_admin_qq, "%中断 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 2");
}

TEST_F(TestBot, interrupt_private_start_in_public)
{
  AddGame<2>("测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PUB_MSG(EC_OK, "999", k_admin_qq, "%中断 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
}

// Computer Player

TEST_F(TestBot, set_computer)
{
  AddGame<5>("测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#替补至 5");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "1", "电脑行动次数 4");
}

TEST_F(TestBot, set_computer_no_limit)
{
  AddGame<0>("测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#替补至 12");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "1", "电脑行动次数 11");
}

TEST_F(TestBot, set_computer_not_host)
{
  AddGame<5>("测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_MATCH_NOT_HOST, "2", "#替补至 5");
}

TEST_F(TestBot, set_computer_and_player_enough)
{
  AddGame<5>("测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#替补至 4");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "电脑行动次数 2");
}

TEST_F(TestBot, set_computer_but_player_enough)
{
  AddGame<5>("测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#替补至 2");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "电脑行动次数 0");
}

TEST_F(TestBot, computer_exceed_max_player)
{
  AddGame<2>("测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_ACHIEVE_MAX_PLAYER, "1", "#替补至 3");
  ASSERT_PRI_MSG(EC_OK, "2", ("#加入 1"));
}

TEST_F(TestBot, computer_kick_joined_player)
{
  AddGame<3>("测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", ("#加入 1"));
  ASSERT_PRI_MSG(EC_OK, "1", "#替补至 3");
  ASSERT_PRI_MSG(EC_OK, "2", ("#加入 1"));
}

TEST_F(TestBot, computer_leave_when_all_users_eliminated)
{
  AddGame<3>("测试游戏");
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
  AddGame<2>("测试游戏");
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
  AddGame<2>("测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "1", "结束子阶段");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, game_over_by_timeup)
{
  //TODO: support control timer
  AddGame<2>("测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  SkipTimer();
  WaitTimerThreadFinish();
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, checkout_substage_by_request)
{
  AddGame<2>("测试游戏");
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
  AddGame<2>("测试游戏");
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
  AddGame<2>("测试游戏");
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
  AddGame<2>("测试游戏");
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
  AddGame<2, lgtbot::game::GAME_MODULE_NAME::AtomMainStage>("测试游戏");
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
  AddGame<2>("测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");

  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "全员重新准备 1");
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

  // OnStageTimeout should not be invoked and game should not be over because timer is reset.
  // So the player can execute 准备 command
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "准备");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "2", "准备");

  WaitTimerThreadFinish();
}

TEST_F(TestBot, leave_during_handle_request)
{
  AddGame<2>("测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");

  auto fut_1 = std::async([this]
        {
            ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "1", "阻塞并结束");
        });
  WaitSubStageBlock();
  const auto match_use_count = bot_->match_manager().GetMatch(UserID("1")).use_count();
  auto fut_2 = std::async([this]
        {
            ASSERT_PRI_MSG(EC_MATCH_ALREADY_OVER, "1", "#退出 强制");
        });
  // wait leave command get the match reference count
  while (bot_->match_manager().GetMatch(UserID("1")).use_count() == match_use_count);
  NotifySubStage();
  fut_1.wait();
  fut_2.wait();

  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");

  WaitTimerThreadFinish();
}

TEST_F(TestBot, leave_and_join_other_game)
{
  AddGame<2>("测试游戏");
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
  AddGame<2>("测试游戏");
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
  AddGame<2>("测试游戏");
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
  AddGame<2>("测试游戏");
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
  AddGame<2>("测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "淘汰");
  ASSERT_PRI_MSG(EC_OK, "1", "#退出");
}

TEST_F(TestBot, auto_set_ready_when_other_players_have_eliminated_should_checkout)
{
  AddGame<2>("测试游戏");
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
  AddGame<2>("测试游戏");
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
  AddGame<2>("测试游戏");
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
  AddGame<2>("测试游戏");
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
  AddGame<2>("测试游戏");
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
  AddGame<2>("测试游戏");
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
  AddGame<2>("测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#替补至 2");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "分数 1");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, "1", "准备");
  ASSERT_EQ(0, db_manager().match_profiles_.size());
}

TEST_F(TestBot, all_player_leave_not_record)
{
  AddGame<2>("测试游戏");
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
  AddGame<2>("测试游戏");
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
  AddGame<2>("测试游戏");
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
  AddGame<2>("测试游戏");
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
  AddGame<2>("测试游戏");
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
  AddGame<2>("测试游戏");
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
  AddGame<2>("测试游戏");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, "2", "#加入 1");
  ASSERT_PRI_MSG(EC_OK, "1", "#开始");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, "1", "挂机");
  ASSERT_PRI_MSG(EC_OK, "2", "#中断");
  ASSERT_PRI_MSG(EC_OK, "1", "#新游戏 测试游戏");
}

TEST_F(TestBot, user_interrupt_before_game_starts)
{
  AddGame<2>("测试游戏");
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
  AddGame<3>("测试游戏");
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
