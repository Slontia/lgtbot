#include <future>

#include <gtest/gtest.h>
#include <gflags/gflags.h>

#include "game_framework/game_stage.h"

#include "bot_core/msg_sender.h"
#include "bot_core/bot_core.h"
#include "bot_core/timer.h"
#include "bot_core/bot_ctx.h"
#include "bot_core/game_handle.h"

static_assert(TEST_BOT);

std::condition_variable Timer::cv_;
bool Timer::skip_timer_ = false;
std::condition_variable Timer::remaining_thread_cv_;
uint64_t Timer::remaining_thread_count_ = 0;
std::mutex Timer::mutex_;

static std::ostream& operator<<(std::ostream& os, const ErrCode e) { return os << errcode2str(e); }

constexpr const uint64_t k_this_qq = 114514;
constexpr const uint64_t k_admin_qq = 1919810;
static int g_argc = 0;
static char** g_argv = nullptr;

typedef bool(*InitBotFunc)(const UserID, int argc, char** argv);
typedef ErrCode(*HandlePrivateRequestFunc)(const UserID uid, const char* const msg);
typedef ErrCode(*HandlePublicRequestFunc)(const UserID uid, const GroupID gid, const char* const msg);

struct Messager
{
    Messager(const uint64_t id, const bool is_uid) : id_(id), is_uid_(is_uid) {}
    const uint64_t id_;
    const bool is_uid_;
    std::stringstream ss_;
};

void* OpenMessager(const uint64_t id, const bool is_uid)
{
    return new Messager(id, is_uid);
}

void MessagerPostText(void* p, const char* data, uint64_t len)
{
    Messager* const messager = static_cast<Messager*>(p);
    messager->ss_ << std::string_view(data, len);
}

void MessagerPostUser(void* p, uint64_t uid, bool is_at)
{
    Messager* const messager = static_cast<Messager*>(p);
    if (is_at) {
        messager->ss_ << "@" << uid;
    } else if (messager->is_uid_) {
        messager->ss_ << uid;
    } else {
        messager->ss_ << uid << "(gid=" << messager->id_ << ")";
    }
}

void MessagerFlush(void* p)
{
    Messager* const messager = static_cast<Messager*>(p);
    if (messager->is_uid_) {
        std::cout << "[BOT -> USER_" << messager->id_ << "]" << std::endl << messager->ss_.str() << std::endl;
    } else {
        std::cout << "[BOT -> GROUP_" << messager->id_ << "]" << std::endl << messager->ss_.str() << std::endl;
    }
    messager->ss_.str("");
}

void CloseMessager(void* p)
{
    Messager* const messager = static_cast<Messager*>(p);
    delete messager;
}

class TestBot;

static std::mutex substage_blocked_mutex_;
static std::condition_variable substage_block_request_cv_;
static std::atomic<bool> substage_blocked_;

class MainStage;

class SubStage : public SubGameStage<>
{
  public:
    SubStage(MainStage& main_stage)
        : GameStage(main_stage, "子阶段"
                , MakeStageCommand("结束", &SubStage::Over_, VoidChecker("结束子阶段"))
                , MakeStageCommand("准备重新计时", &SubStage::ToResetTimer_, VoidChecker("准备重新计时"))
                , MakeStageCommand("阻塞", &SubStage::Block_, VoidChecker("阻塞"))
                , MakeStageCommand("阻塞并结束", &SubStage::BlockAndOver_, VoidChecker("阻塞并结束"))
                , MakeStageCommand("准备", &SubStage::Ready_, VoidChecker("准备"))
          )
        , to_reset_timer_(false)
    {}

    virtual void OnStageBegin() override
    {
        Boardcast() << "子阶段开始";
        StartTimer(1);
    }

    virtual TimeoutErrCode OnTimeout() override
    {
        if (to_reset_timer_) {
            to_reset_timer_ = false;
            StartTimer(1);
            Boardcast() << "时间到，但是回合继续";
            return StageErrCode::OK;
        }
        Boardcast() << "时间到，回合结束";
        return StageErrCode::CHECKOUT;
    }

  private:
    AtomReqErrCode Ready_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        return StageErrCode::READY;
    }

    AtomReqErrCode Over_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        return StageErrCode::CHECKOUT;
    }

    AtomReqErrCode ToResetTimer_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        to_reset_timer_ = true;
        return StageErrCode::OK;
    }

    AtomReqErrCode Block_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        std::unique_lock<std::mutex> l(substage_blocked_mutex_);
        if (substage_blocked_.exchange(true) == true) {
            throw std::runtime_error("a substage has ready been blocked");
        }
        substage_block_request_cv_.wait(l);
        substage_blocked_ = false;
        return StageErrCode::OK;
    }

    AtomReqErrCode BlockAndOver_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        Block_(pid, is_public, reply);
        return StageErrCode::CHECKOUT;
    }

    bool to_reset_timer_;
};

class MainStage : public MainGameStage<SubStage>
{
  public:
    MainStage(const GameOption& option, MatchBase& match)
        : GameStage(option, match, "主阶段", MakeStageCommand("准备切换", &MainStage::ToCheckout_, VoidChecker("准备切换")))
        , to_checkout_(false)
    {}

    virtual VariantSubStage OnStageBegin() override
    {
        return std::make_unique<SubStage>(*this);
    }

    virtual VariantSubStage NextSubStage(SubStage& sub_stage, const CheckoutReason reason) override
    {
        if (to_checkout_) {
            Boardcast() << "回合结束，切换子阶段";
            to_checkout_ = false;
            return std::make_unique<SubStage>(*this);
        }
        Boardcast() << "回合结束，游戏结束";
        return {};
    }

    virtual int64_t PlayerScore(const PlayerID pid) const override { return 1; };

  private:
    CompReqErrCode ToCheckout_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        to_checkout_ = true;
        return StageErrCode::OK;
    }

    bool to_checkout_;
};

class GameOption : public GameOptionBase
{
  public:
    GameOption() : GameOptionBase(0) {}
    virtual const char* Info(const uint64_t index) const { return "这是配置介绍"; };
    virtual const char* Status() const { return "这是配置状态"; };
    virtual bool SetOption(const char* const msg) { return true; };
};

class TestBot : public testing::Test
{
  public:
    virtual void SetUp() override
    {
        Timer::skip_timer_ = false;
        bot_ = BOT_API::Init(k_this_qq, nullptr, &k_admin_qq, 1);
        ASSERT_NE(bot_, nullptr) << "init bot failed";
    }

    virtual void TearDown() override
    {
        BOT_API::Release(bot_);
    }

  protected:
    void AddGame(const char* const name, const uint64_t max_player)
    {
        static_cast<BotCtx*>(bot_)->game_handles().emplace(name, std::make_unique<GameHandle>(
                    std::nullopt, name, max_player, "这是规则介绍",
                    []() -> GameOptionBase* { return new GameOption(); },
                    [](const GameOptionBase* const options) { delete options; },
                    [](MsgSenderBase&, const GameOptionBase& option, MatchBase& match)
                            -> MainStageBase* { return new MainStage(static_cast<const GameOption&>(option), match); },
                    [](const MainStageBase* const main_stage) { delete main_stage; },
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
        while (!substage_blocked_.load())
            ;
    }

    void* bot_;

};

#define ASSERT_PUB_MSG(ret, gid, uid, msg)\
do\
{\
  std::cout << "[USER_" << uid <<  " -> GROUP_" << gid << "]" << std::endl << msg << std::endl;\
  ASSERT_EQ((ret), BOT_API::HandlePublicRequest(bot_, (gid), (uid), (msg)));\
} while (0)

#define ASSERT_PRI_MSG(ret, uid, msg)\
do\
{\
  std::cout << "[USER_" << uid <<  " -> BOT]" << std::endl << msg << std::endl;\
  ASSERT_EQ((ret), BOT_API::HandlePrivateRequest(bot_, (uid), (msg)));\
} while (0)

// Join Not Existing Game

TEST_F(TestBot, pub_join_game_failed)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_MATCH_GROUP_NOT_IN_MATCH, 1, 1, "#加入游戏");
}

TEST_F(TestBot, pub_join_pri_game_failed)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_MATCH_NEED_REQUEST_PRIVATE, 1, 1, "#加入游戏 1");
}

TEST_F(TestBot, pri_join_game_failed)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_MATCH_NOT_EXIST, 1, "#加入游戏 1");
}

TEST_F(TestBot, pri_join_pub_game_failed)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_MATCH_NEED_ID, 1, "#加入游戏");
}

// Repeat New Game

TEST_F(TestBot, pub_repeat_new_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, 1, 1, "#新游戏 测试游戏");
}

TEST_F(TestBot, pub_repeat_new_game_other_group)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, 2, 1, "#新游戏 测试游戏");
}

TEST_F(TestBot, pub_repeat_new_pri_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, 1, "#新游戏 测试游戏");
}

TEST_F(TestBot, pri_repeat_new_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, 1, "#新游戏 测试游戏");
}

TEST_F(TestBot, pri_repeat_new_pub_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, 1, 1, "#新游戏 测试游戏");
}

// Join Self Game

TEST_F(TestBot, pub_join_self_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, 1, 1, "#加入游戏");
}

TEST_F(TestBot, pri_join_self_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, 1, "#加入游戏 1");
}

// Join Other Game After Creating Game

TEST_F(TestBot, pub_join_other_pub_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, 2, 2, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_USER_ALREADY_IN_OTHER_MATCH, 2, 1, "#加入游戏");
}

TEST_F(TestBot, pub_join_other_pri_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, 2, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_USER_ALREADY_IN_OTHER_MATCH, 1, "#加入游戏 2");
}

TEST_F(TestBot, pri_join_other_pub_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, 2, 2, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_USER_ALREADY_IN_OTHER_MATCH, 2, 1, "#加入游戏");
}

TEST_F(TestBot, pri_join_other_pri_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, 2, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_USER_ALREADY_IN_OTHER_MATCH, 1, "#加入游戏 2");
}

// Start Game Failed

TEST_F(TestBot, pub_start_game_not_host)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 2, "#加入游戏");
  ASSERT_PUB_MSG(EC_MATCH_NOT_HOST, 1, 2, "#开始游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#开始游戏");
}

TEST_F(TestBot, pri_start_game_not_host)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, 2, "#加入游戏 1");
  ASSERT_PRI_MSG(EC_MATCH_NOT_HOST, 2, "#开始游戏");
  ASSERT_PRI_MSG(EC_OK, 1, "#开始游戏");
}

TEST_F(TestBot, pub_start_game_not_in_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 2, "#加入游戏");
  ASSERT_PUB_MSG(EC_MATCH_USER_NOT_IN_MATCH, 1, 3, "#开始游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#开始游戏");
}

TEST_F(TestBot, pri_start_game_not_in_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, 2, "#加入游戏 1");
  ASSERT_PRI_MSG(EC_MATCH_USER_NOT_IN_MATCH, 3, "#开始游戏");
  ASSERT_PRI_MSG(EC_OK, 1, "#开始游戏");
}

TEST_F(TestBot, pub_start_other_pub_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 2, "#加入游戏");
  ASSERT_PUB_MSG(EC_OK, 2, 3, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, 2, 4, "#加入游戏");
  ASSERT_PUB_MSG(EC_MATCH_NOT_THIS_GROUP, 2, 1, "#开始游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#开始游戏");
  ASSERT_PUB_MSG(EC_OK, 2, 3, "#开始游戏");
}

TEST_F(TestBot, pri_start_other_pub_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, 2, "#加入游戏 1");
  ASSERT_PUB_MSG(EC_OK, 2, 3, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, 2, 4, "#加入游戏");
  ASSERT_PUB_MSG(EC_MATCH_NOT_THIS_GROUP, 2, 1, "#开始游戏");
  ASSERT_PRI_MSG(EC_OK, 1, "#开始游戏");
  ASSERT_PUB_MSG(EC_OK, 2, 3, "#开始游戏");
}

TEST_F(TestBot, pub_game_pri_start)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 2, "#加入游戏");
  ASSERT_PRI_MSG(EC_OK, 1, "#开始游戏");
}

// Exit Not Existing Game

TEST_F(TestBot, pub_exit_not_exist_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_MATCH_USER_NOT_IN_MATCH, 1, 1, "#退出游戏");
}

TEST_F(TestBot, pri_exit_not_exist_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_MATCH_USER_NOT_IN_MATCH, 1, "#退出游戏");
}

// Exit And New Game

TEST_F(TestBot, exit_pub_game_then_new_pub_game_same_group)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#退出游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
}

TEST_F(TestBot, exit_pub_game_then_new_pub_game_other_group)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#退出游戏");
  ASSERT_PUB_MSG(EC_OK, 2, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_GROUP_NOT_IN_MATCH, 1, 2, "#加入游戏");
}

TEST_F(TestBot, exit_pri_game_then_new_pub_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, 1, "#退出游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_NOT_EXIST, 2, "#加入游戏 1");
}

TEST_F(TestBot, exit_pri_game_then_new_pri_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, 1, "#退出游戏");
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_NOT_EXIST, 2, "#加入游戏 1");
  ASSERT_PRI_MSG(EC_OK, 2, "#加入游戏 2");
}

// Exit And Join Game

TEST_F(TestBot, exit_pub_game_then_join_pub_game_same_group)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 2, "#加入游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#退出游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#加入游戏");
}

TEST_F(TestBot, exit_pub_game_then_join_pub_game_other_group)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, 2, 2, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#退出游戏");
  ASSERT_PUB_MSG(EC_OK, 2, 1, "#加入游戏");
}

TEST_F(TestBot, exit_pri_game_then_join_pub_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 2, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, 1, "#退出游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#加入游戏");
}

TEST_F(TestBot, exit_pri_game_then_join_pri_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, 2, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, 1, "#退出游戏");
  ASSERT_PRI_MSG(EC_MATCH_NOT_EXIST, 1, "#加入游戏 1");
  ASSERT_PRI_MSG(EC_OK, 1, "#加入游戏 2");
}

// Switch Host

TEST_F(TestBot, switch_host)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 2, "#加入游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#退出游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 3, "#加入游戏");
  ASSERT_PUB_MSG(EC_MATCH_USER_NOT_IN_MATCH, 1, 1, "#开始游戏");
  ASSERT_PUB_MSG(EC_MATCH_NOT_HOST, 1, 3, "#开始游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 2, "#开始游戏");
}

// Config Game

TEST_F(TestBot, config_game_exit)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#退出游戏");
  ASSERT_PUB_MSG(EC_MATCH_GROUP_NOT_IN_MATCH, 1, 2, "#加入游戏");
}

// Player Limit

TEST_F(TestBot, exceed_max_player)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 2, "#加入游戏");
  ASSERT_PUB_MSG(EC_MATCH_ACHIEVE_MAX_PLAYER, 1, 3, "#加入游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#开始游戏");
}

// Administor

TEST_F(TestBot, interrupt_private_without_mid)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_MATCH_NEED_REQUEST_PUBLIC, k_admin_qq, "%中断游戏");
}

TEST_F(TestBot, interrupt_public_not_game)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_MATCH_GROUP_NOT_IN_MATCH, 1, k_admin_qq, "%中断游戏");
}

TEST_F(TestBot, interrupt_public_config)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, 1, k_admin_qq, "%中断游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
}

TEST_F(TestBot, interrupt_public_wait)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 2, "#加入游戏");
  ASSERT_PUB_MSG(EC_OK, 1, k_admin_qq, "%中断游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
}

TEST_F(TestBot, interrupt_public_start)
{
  AddGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 2, "#加入游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#开始游戏");
  ASSERT_PUB_MSG(EC_OK, 1, k_admin_qq, "%中断游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
}

TEST_F(TestBot, interrupt_private_wait)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%中断游戏 1");
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
}

TEST_F(TestBot, interrupt_private_config)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, 2, "#加入游戏 1");
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%中断游戏 1");
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
}

TEST_F(TestBot, interrupt_private_start)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, 2, "#加入游戏 1");
  ASSERT_PRI_MSG(EC_OK, 1, "#开始游戏");
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%中断游戏 1");
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
}

TEST_F(TestBot, interrupt_private_wait_in_public)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, 999, k_admin_qq, "%中断游戏 1");
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
}

TEST_F(TestBot, interrupt_private_config_in_public)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, 2, "#加入游戏 1");
  ASSERT_PUB_MSG(EC_OK, 999, k_admin_qq, "%中断游戏 1");
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
}

TEST_F(TestBot, interrupt_private_start_in_public)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, 2, "#加入游戏 1");
  ASSERT_PRI_MSG(EC_OK, 1, "#开始游戏");
  ASSERT_PUB_MSG(EC_OK, 999, k_admin_qq, "%中断游戏 1");
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
}

// Computer Player

TEST_F(TestBot, set_computer)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, 1, "#电脑数量 1");
  ASSERT_PRI_MSG(EC_MATCH_ACHIEVE_MAX_PLAYER, 2, ("#加入游戏 1"));
  ASSERT_PRI_MSG(EC_OK, 1, "#开始游戏");
}

TEST_F(TestBot, set_computer_config)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, 1, "#电脑数量 1");
  ASSERT_PRI_MSG(EC_MATCH_ACHIEVE_MAX_PLAYER, 2, ("#加入游戏 1"));
  ASSERT_PRI_MSG(EC_OK, 1, "#开始游戏");
}

TEST_F(TestBot, computer_exceed_max_player)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_ACHIEVE_MAX_PLAYER, 1, "#电脑数量 2");
  ASSERT_PRI_MSG(EC_OK, 2, ("#加入游戏 1"));
}

TEST_F(TestBot, computer_exceed_max_player_when_config)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_ACHIEVE_MAX_PLAYER, 1, "#电脑数量 2");
}

// Test Game

TEST_F(TestBot, game_over_by_request)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, 2, "#加入游戏 1");
  ASSERT_PRI_MSG(EC_OK, 1, "#开始游戏");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, 1, "结束子阶段");
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
}

TEST_F(TestBot, game_over_by_timeup)
{
  //TODO: support control timer
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, 2, "#加入游戏 1");
  ASSERT_PRI_MSG(EC_OK, 1, "#开始游戏");
  SkipTimer();
  WaitTimerThreadFinish();
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
}

TEST_F(TestBot, checkout_substage_by_request)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, 2, "#加入游戏 1");
  ASSERT_PRI_MSG(EC_OK, 1, "#开始游戏");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, 1, "准备切换");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, 1, "结束子阶段");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, 1, "结束子阶段");
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
}

TEST_F(TestBot, checkout_substage_by_timeout)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, 2, "#加入游戏 1");
  ASSERT_PRI_MSG(EC_OK, 1, "#开始游戏");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, 1, "准备切换");
  SkipTimer();
  WaitTimerThreadFinish();
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
}

TEST_F(TestBot, substage_reset_timer)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, 2, "#加入游戏 1");
  ASSERT_PRI_MSG(EC_OK, 1, "#开始游戏");
  ASSERT_PRI_MSG(EC_GAME_REQUEST_OK, 1, "准备重新计时");
  SkipTimer();
  WaitTimerThreadFinish();
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
}

TEST_F(TestBot, timeout_during_handle_rquest)
{
  AddGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, 2, "#加入游戏 1");
  ASSERT_PRI_MSG(EC_OK, 1, "#开始游戏");

  auto fut_1 = std::async([this]
        {
            ASSERT_PRI_MSG(EC_GAME_REQUEST_CHECKOUT, 1, "阻塞并结束");
        });
  WaitSubStageBlock();
  SkipTimer();
  auto fut_2 = std::async([this]
        {
            WaitTimerThreadFinish();
        });
  NotifySubStage();
  fut_1.wait();
  fut_2.wait();

  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  g_argc = argc;
  g_argv = argv;
  return RUN_ALL_TESTS();
}
