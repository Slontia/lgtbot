#pragma once

#include <gtest/gtest.h>

#include "game_framework/game_stage.h"
#include "game_framework/game_options.h"
#include "game_framework/game_main.h"
#include "game_framework/mock_match.h"

static const char* StageErrCodeToString(const StageBase::StageErrCode& rc)
{
    switch (rc) {
#define HANDLE_ERRCODE(rc) case StageBase::StageErrCode::rc: return #rc;
        HANDLE_ERRCODE(OK)
        HANDLE_ERRCODE(CHECKOUT)
        HANDLE_ERRCODE(FAILED)
        HANDLE_ERRCODE(NOT_FOUND)
#undef HANDLE_ERRCODE
    }
    return nullptr; // should not reach here
}

MainStageBase* MakeMainStage(MsgSenderBase& reply, const GameOption& options);

template <uint64_t k_player_num>
class TestGame : public MockMatch, public testing::Test
{
   public:
    using ScoreArray = std::array<int64_t, k_player_num>;

    TestGame() : timer_started_(false) {}

    virtual ~TestGame() {}

    virtual void SetUp()
    {
        option_.SetPlayerNum(k_player_num);
    }

    bool StartGame()
    {
        MockMsgSender sender;
        main_stage_.reset(MakeMainStage(sender, option_));
        if (main_stage_) {
            main_stage_->Init(this);
        }
        return main_stage_ != nullptr;
    }

    auto& expected_scores() const { return expected_scores_; }

    virtual void StartTimer(const uint64_t /*sec*/) override { timer_started_ = true; }

    virtual void StopTimer() override { timer_started_ = false; }

   protected:
    StageBase::StageErrCode Timeout()
    {
        if (!timer_started_) {
            throw std::runtime_error("timer is not started");
        }
        std::cout << "[TIMEOUT]" << std::endl;
        const auto rc = main_stage_->HandleTimeout();
        HandleGameOver_();
        return rc;
    }

    StageBase::StageErrCode PublicRequest(const PlayerID pid, const char* const msg)
    {
        std::cout << "[PLAYER_" << pid << " -> GROUP]" << std::endl << msg << std::endl;
        return Request_(pid, msg, true);
    }

    StageBase::StageErrCode PrivateRequest(const PlayerID pid, const char* const msg)
    {
        std::cout << "[PLAYER_" << pid << " -> BOT]" << std::endl << msg << std::endl;
        return Request_(pid, msg, false);
    }

    StageBase::StageErrCode Leave(const PlayerID pid)
    {
        std::cout << "[PLAYER_" << pid << " LEAVE GAME]" << std::endl;
        const auto rc = main_stage_->HandleLeave(pid);
        HandleGameOver_();
        return rc;
    }

    GameOption option_;
    std::unique_ptr<MainStageBase> main_stage_;
    std::optional<ScoreArray> expected_scores_;
    bool timer_started_;

  private:
    void HandleGameOver_()
    {
        if (main_stage_ && main_stage_->IsOver()) {
            auto& dst_scores = expected_scores_.emplace();
            for (size_t i = 0; i < dst_scores.size(); ++i) {
                dst_scores.at(i) = main_stage_->PlayerScore(i);
            }
        }
    }

    StageBase::StageErrCode Request_(const PlayerID pid, const char* const msg, const bool is_public)
    {
        MockMsgSender sender(pid, is_public);
        const auto rc = main_stage_            ? main_stage_->HandleRequest(msg, pid, is_public, sender) :
                        option_.SetOption(msg) ? StageBase::StageErrCode::OK :
                                                 StageBase::StageErrCode::FAILED; // for easy test
        HandleGameOver_();
        return rc;
    }
};

#define ASSERT_FINISHED(finished) ASSERT_EQ(expected_scores().has_value(), finished);

#define ASSERT_SCORE(scores...) \
    do { \
        ASSERT_TRUE(expected_scores().has_value()) << "Game not finish"; \
        ASSERT_EQ((ScoreArray{scores}), *expected_scores()) << "Score not match"; \
    } while (0)

#define START_GAME() \
    do { \
        std::cout << "[GAME START]" << std::endl; \
        ASSERT_TRUE(StartGame()) << "Start game failed"; \
    } while (0)

#define __ASSERT_ERRCODE_BASE(ret, statement) \
    do { \
        const auto rc = statement; \
        ASSERT_EQ(StageBase::StageErrCode::ret, rc) << "ErrCode Mismatch, Actual: " << StageErrCodeToString(rc); \
    } while (0)

#define ASSERT_TIMEOUT(ret) __ASSERT_ERRCODE_BASE(ret, (Timeout()))
#define CHECK_TIMEOUT(ret) (StageBase::StageErrCode::ret == (Timeout()))

#define ASSERT_PUB_MSG(ret, pid, msg) __ASSERT_ERRCODE_BASE(ret, (PublicRequest(pid, msg)))
#define CHECK_PUB_MSG(ret, pid, msg) (StageBase::StageErrCode::ret == PublicRequest(pid, msg))

#define ASSERT_PRI_MSG(ret, pid, msg) __ASSERT_ERRCODE_BASE(ret, (PrivateRequest(pid, msg)))
#define CHECK_PRI_MSG(ret, pid, msg) (StageBase::StageErrCode::ret == PrivateRequest(pid, msg))

#define ASSERT_LEAVE(ret, pid) __ASSERT_ERRCODE_BASE(ret, Leave(pid))
#define CHECK_LEAVE(ret, pid) (StageBase::StageErrCode::ret == Leave(pid))

#define GAME_TEST(player_num, test_name) \
    using TestGame_##player_num##_##test_name = TestGame<player_num>; \
    TEST_F(TestGame_##player_num##_##test_name, test_name)
