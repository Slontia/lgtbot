// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <gtest/gtest.h>
#include <gflags/gflags.h>
#if WITH_GLOG
#include <glog/logging.h>
#endif

#include "game_framework/util.h"
#include "game_framework/stage.h"
#include "game_framework/game_options.h"
#include "game_framework/game_main.h"
#include "game_framework/mock_match.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

DEFINE_string(resource_dir, "./resource_dir/", "The path of game image resources");
DEFINE_bool(gen_image, false, "Whether generate image or not");
DEFINE_string(image_dir, "./.lgtbot_image/", "The path of directory to store generated images");

internal::MainStage* MakeMainStage(MainStageFactory factory);

static inline const std::string g_begin_timestamp =
    std::to_string(std::chrono::system_clock::now().time_since_epoch().count());

template <uint64_t k_player_num>
class TestGame : public MockMatch, public testing::Test
{

   public:
    using ScoreArray = std::array<int64_t, k_player_num>;
    using AchievementsArray = std::array<std::vector<std::string>, k_player_num>;

    TestGame()
        : MockMatch(std::filesystem::path(FLAGS_image_dir) / g_begin_timestamp / ::testing::UnitTest::GetInstance()->current_test_info()->name(), k_player_num)
        , timer_started_(false) {}

    virtual ~TestGame() {}

    virtual void SetUp() override
    {
#ifdef WITH_GLOG
        google::InitGoogleLogging(::testing::UnitTest::GetInstance()->current_test_info()->name());
#endif
        enable_markdown_to_image = FLAGS_gen_image && !FLAGS_image_dir.empty();
    }

    virtual void TearDown() override
    {
#ifdef WITH_GLOG
        google::ShutdownGoogleLogging();
#endif
    }

    virtual const char* GameName() const override { return k_properties.name_; }

    bool StartGame()
    {
        MockMsgSender sender(image_dir());
        options_.resource_holder_.resource_dir_ = std::filesystem::absolute(FLAGS_resource_dir + "/").string();
        options_.resource_holder_.saved_image_dir_ = std::filesystem::absolute(image_dir()).string();
        options_.generic_options_.resource_dir_ = options_.resource_holder_.resource_dir_.c_str();
        options_.generic_options_.saved_image_dir_ = options_.resource_holder_.saved_image_dir_.c_str();
        options_.generic_options_.user_num_ = k_player_num;
        if (!AdaptOptions(sender, options_.game_options_, options_.generic_options_, options_.generic_options_)) {
            return false;
        }
        main_stage_.reset(MakeMainStage(MainStageFactory{options_.game_options_, options_.generic_options_, *this}));
        if (main_stage_) {
            main_stage_->HandleStageBegin();
        }
        return main_stage_ != nullptr;
    }

    auto& actual_scores() const { return actual_scores_; }

    auto& actual_achievements() const { return actual_achievements_; }

    virtual void StartTimer(const uint64_t /*sec*/, void* p, void(*cb)(void*, uint64_t)) override { timer_started_ = true; }

    virtual void StopTimer() override { timer_started_ = false; }

   protected:
    StageErrCode Timeout()
    {
        if (!timer_started_) {
            throw std::runtime_error("timer is not started");
        }
        std::cout << "[TIMEOUT]" << std::endl;
        const auto rc = main_stage_->HandleTimeout();
        HandleGameOver_();
        return rc;
    }

    StageErrCode PublicRequest(const PlayerID pid, const std::string& msg)
    {
        std::cout << "[PLAYER_" << pid << " -> GROUP]" << std::endl << msg << std::endl;
        return Request_(pid, msg, true);
    }

    StageErrCode PrivateRequest(const PlayerID pid, const std::string& msg)
    {
        std::cout << "[PLAYER_" << pid << " -> BOT]" << std::endl << msg << std::endl;
        return Request_(pid, msg, false);
    }

    StageErrCode Leave(const PlayerID pid)
    {
        std::cout << "[PLAYER_" << pid << " LEAVE GAME]" << std::endl;
        const auto rc = main_stage_->HandleLeave(pid);
        HandleGameOver_();
        return rc;
    }

    StageErrCode ComputerAct(const PlayerID pid)
    {
        std::cout << "[PLAYER_" << pid << " COMPUTER ACT]" << std::endl;
        const auto rc = main_stage_->HandleComputerAct(pid, false);
        HandleGameOver_();
        return rc;
    }

    struct Options
    {
        struct ResourceHolder
        {
            std::string resource_dir_;
            std::string saved_image_dir_;
        };

        ResourceHolder resource_holder_;
        GameOptions game_options_;
        lgtbot::game::GenericOptions generic_options_;
    };

    Options options_;
    std::unique_ptr<MainStageBase> main_stage_;
    std::optional<ScoreArray> actual_scores_;
    std::optional<AchievementsArray> actual_achievements_;
    bool timer_started_;

  private:
    void HandleGameOver_()
    {
        if (main_stage_ && main_stage_->IsOver()) {
            auto& dst_scores = actual_scores_.emplace();
            auto& dst_achievements = actual_achievements_.emplace();
            for (size_t i = 0; i < dst_scores.size(); ++i) {
                dst_scores.at(i) = main_stage_->PlayerScore(i);
                for (const char* const* achievement_name_p = main_stage_->VerdictateAchievements(i);
                        *achievement_name_p;
                        ++achievement_name_p) {
                    dst_achievements.at(i).emplace_back(*achievement_name_p);
                }
            }
        }
    }

    StageErrCode Request_(const PlayerID pid, const std::string& msg, const bool is_public)
    {
        MockMsgSender sender(image_dir(), pid, is_public);
        assert(!main_stage_ || !main_stage_->IsOver());
        const auto rc =
            main_stage_  ? main_stage_->HandleRequest(msg.c_str(), pid, is_public, sender)
                         : StageErrCode::Condition(options_.game_options_.SetOption(msg.c_str()), StageErrCode::OK , StageErrCode::FAILED); // for easy test
        HandleGameOver_();
        return rc;
    }
};

#define ASSERT_FINISHED(finished) ASSERT_EQ(actual_scores().has_value(), finished);

#define ASSERT_SCORE(scores...) \
    do { \
        ASSERT_TRUE(actual_scores().has_value()) << "Game not finish"; \
        ASSERT_EQ((ScoreArray{scores}), *actual_scores()) << "Score not match"; \
    } while (0)

#define ASSERT_ACHIEVEMENTS(pid, achievements...) \
    do { \
        ASSERT_TRUE(actual_achievements().has_value()) << "Game not finish"; \
        auto expected_achievements = std::vector<std::string>{achievements}; \
        std::ranges::sort(expected_achievements); \
        auto player_actual_achievements = actual_achievements()->at(pid); \
        std::ranges::sort(player_actual_achievements); \
        ASSERT_EQ(expected_achievements, player_actual_achievements) << "Achievements not match"; \
    } while (0)

#define START_GAME() \
    do { \
        std::cout << "[GAME START]" << std::endl; \
        ASSERT_TRUE(StartGame()) << "Start game failed"; \
    } while (0)

#define ASSERT_ERRCODE(expected, actual) ASSERT_TRUE((expected) == (actual)) << "ErrCode Mismatch, Actual: " << (actual.ToString())

#define __ASSERT_ERRCODE_BASE(ret, statement) \
    do { \
        const auto rc = statement; \
        ASSERT_ERRCODE(StageErrCode::ret, rc); \
    } while (0)

#define ASSERT_TIMEOUT(ret) __ASSERT_ERRCODE_BASE(ret, (Timeout()))
#define CHECK_TIMEOUT(ret) (StageErrCode::ret == (Timeout()))

#define ASSERT_PUB_MSG(ret, pid, msg) \
    do { \
        ASSERT_FALSE(IsEliminated(pid)) << "player is eliminated"; \
        __ASSERT_ERRCODE_BASE(ret, (PublicRequest(pid, msg))); \
    } while (0)
#define CHECK_PUB_MSG(ret, pid, msg) \
    ([&] { ASSERT_FALSE(IsEliminated(pid)) << "player is eliminated"; }(), StageErrCode::ret == PublicRequest(pid, msg))

#define ASSERT_PRI_MSG(ret, pid, msg) \
    do { \
        ASSERT_FALSE(IsEliminated(pid)) << "player is eliminated"; \
        __ASSERT_ERRCODE_BASE(ret, (PrivateRequest(pid, msg))); \
    } while (0)
#define CHECK_PRI_MSG(ret, pid, msg) \
    ([&] { ASSERT_FALSE(IsEliminated(pid)) << "player is eliminated"; }(), StageErrCode::ret == PrivateRequest(pid, msg))

#define ASSERT_LEAVE(ret, pid) __ASSERT_ERRCODE_BASE(ret, Leave(pid))
#define CHECK_LEAVE(ret, pid) (StageErrCode::ret == Leave(pid))

#define ASSERT_COMPUTER_ACT(ret, pid) __ASSERT_ERRCODE_BASE(ret, ComputerAct(pid))
#define CHECK_COMPUTER_ACT(ret, pid) (StageErrCode::ret == ComputerAct(pid))

#define GAME_TEST(player_num, test_name) \
    using TestGame_##player_num##_##test_name = TestGame<player_num>; \
    TEST_F(TestGame_##player_num##_##test_name, test_name)

#define ASSERT_ELIMINATED(pid) ASSERT_TRUE(IsEliminated(pid))

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot
