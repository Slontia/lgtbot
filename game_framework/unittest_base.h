#include <gtest/gtest.h>

#include <memory>
#include <optional>

#include "game_framework/game_stage.h"
#include "game_framework/game_options.h"
#include "game_framework/game_main.h"
#include "bot_core/msg_sender.h"

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

class MockMsgSender : public MsgSenderBase
{
  public:
    MockMsgSender() : is_public_(true) {}
    MockMsgSender(const PlayerID pid, const bool is_public) : pid_(pid), is_public_(is_public) {}
    virtual MsgSenderGuard operator()() override
    {
        if (is_public_ && pid_.has_value())
        {
            SavePlayer(*pid_, true);
            SaveText(" ", 1);
        }
        return MsgSenderGuard(*this);
    }

  protected:
    virtual void SaveText(const char* const data, const uint64_t len) override
    {
        ss_ << std::string_view(data, len);
    }

    virtual void SaveUser(const UserID& pid, const bool is_at) override
    {
        throw std::runtime_error("user should not appear in game");
    }

    virtual void SavePlayer(const PlayerID& pid, const bool is_at)
    {
        if (is_at) {
            ss_ << "@" << pid;
        } else {
            ss_ << "[PLAYER_" << pid << "]";
        }
    }

    virtual void Flush() override
    {
        if (is_public_) {
            std::cout << "[BOT -> GROUP]";
        } else if (pid_.has_value()) {
            std::cout << "[BOT -> PLAYER_" << *pid_ << "]";
        } else {
            throw std::runtime_error("invalid msg_sender");
        }
        std::cout << std::endl << ss_.str() << std::endl;
        ss_.str("");
    }

  private:
    const std::optional<PlayerID> pid_;
    const bool is_public_;
    std::stringstream ss_;
};


MsgSenderBase::MsgSenderGuard Boardcast(void* match_p)
{
    static MockMsgSender boardcast_sender;
    return boardcast_sender();
}

MsgSenderBase::MsgSenderGuard Tell(void* match_p, const uint64_t pid)
{
    static std::map<uint64_t, MockMsgSender> tell_senders;
    auto it = tell_senders.try_emplace(pid, MockMsgSender(pid, false)).first;
    return (it->second)();
}

void StartTimer(void* p, const uint64_t) { *static_cast<bool*>(p) = true; }
void StopTimer(void* p) { *static_cast<bool*>(p) = false; }

template <uint64_t k_player_num>
class TestGame : public testing::Test
{
   public:
    using ScoreArray = std::array<int64_t, k_player_num>;
    static void SetUpTestCase()
    {
    }

    TestGame() : timer_started_(false) {}

    virtual void SetUp()
    {
        option_.SetPlayerNum(k_player_num);
    }

    virtual void SetDown() {}

    bool StartGame()
    {
        MockMsgSender sender(0, false);
        main_stage_.reset(MakeMainStage(sender, option_));
        if (main_stage_) {
            main_stage_->Init(&timer_started_);
        }
        return main_stage_ != nullptr;
    }

    auto& expected_scores() const { return expected_scores_; }

   protected:
    StageBase::StageErrCode TIMEOUT()
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

#define ASSERT_TIMEOUT(ret) __ASSERT_ERRCODE_BASE(ret, (TIMEOUT()))
#define CHECK_TIMEOUT(ret) (StageBase::StageErrCode::ret == (TIMEOUT()))

#define ASSERT_PUB_MSG(ret, pid, msg) __ASSERT_ERRCODE_BASE(ret, (PublicRequest(pid, msg)))
#define CHECK_PUB_MSG(ret, pid, msg) (StageBase::StageErrCode::ret == PublicRequest(pid, msg))

#define ASSERT_PRI_MSG(ret, pid, msg) __ASSERT_ERRCODE_BASE(ret, (PrivateRequest(pid, msg)))
#define CHECK_PRI_MSG(ret, pid, msg) (StageBase::StageErrCode::ret == PrivateRequest(pid, msg))

#define ASSERT_LEAVE(ret, pid) __ASSERT_ERRCODE_BASE(ret, Leave(pid))
#define CHECK_LEAVE(ret, pid) (StageBase::StageErrCode::ret == Leave(pid))

#define GAME_TEST(player_num, test_name) \
    using TestGame_##player_num##_##test_name = TestGame<player_num>; \
    TEST_F(TestGame_##player_num##_##test_name, test_name)
