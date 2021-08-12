#include <gtest/gtest.h>
#include <gflags/gflags.h>
#include "bot_core/util.h"
#include "bot_core/msg_sender.h"
#include "bot_core/bot_core.h"
#include "game_framework/game_stage.h"

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
}

void CloseMessager(void* p)
{
    Messager* const messager = static_cast<Messager*>(p);
    delete messager;
}

class EmptyMainStage : public MainGameStage<>
{
  public:
    EmptyMainStage() : GameStage() {}
    virtual int64_t PlayerScore(const PlayerID pid) const override { return 1; };
};

class EmptyGameOption : public GameOptionBase
{
  public:
    EmptyGameOption() : GameOptionBase(0) {}
    virtual const char* Info(const uint64_t index) const { return "这是配置介绍"; };
    virtual const char* Status() const { return "这是配置状态"; };
    virtual bool SetOption(const char* const msg) { return true; };
};

class TestBot : public testing::Test
{
  public:
    virtual void SetUp() override
    {
        bot_ = BOT_API::Init(k_this_qq, nullptr, &k_admin_qq, 1);
        ASSERT_NE(bot_, nullptr) << "init bot failed";
    }

    virtual void TearDown() override
    {
        BOT_API::Release(bot_);
    }

  protected:
    void NewGame(const char* const name, const uint64_t max_player)
    {
        static_cast<BotCtx*>(bot_)->game_handles().emplace(name, std::make_unique<GameHandle>(
                    std::nullopt, name, max_player, "这是规则介绍",
                    []() -> GameOptionBase* { return new EmptyGameOption(); },
                    [](const GameOptionBase* const options) { delete options; },
                    [](MsgSenderBase&, const GameOptionBase&) -> MainStageBase* { return new EmptyMainStage(); },
                    [](const MainStageBase* const main_stage) { delete main_stage; },
                    []() {}
        ));
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
  NewGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_MATCH_GROUP_NOT_IN_MATCH, 1, 1, "#加入游戏");
}

TEST_F(TestBot, pub_join_pri_game_failed)
{
  NewGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_MATCH_NEED_REQUEST_PRIVATE, 1, 1, "#加入游戏 1");
}

TEST_F(TestBot, pri_join_game_failed)
{
  NewGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_MATCH_NOT_EXIST, 1, "#加入游戏 1");
}

TEST_F(TestBot, pri_join_pub_game_failed)
{
  NewGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_MATCH_NEED_ID, 1, "#加入游戏");
}

// Repeat New Game

TEST_F(TestBot, pub_repeat_new_game)
{
  NewGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, 1, 1, "#新游戏 测试游戏");
}

TEST_F(TestBot, pub_repeat_new_game_other_group)
{
  NewGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, 2, 1, "#新游戏 测试游戏");
}

TEST_F(TestBot, pub_repeat_new_pri_game)
{
  NewGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, 1, "#新游戏 测试游戏");
}

TEST_F(TestBot, pri_repeat_new_game)
{
  NewGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, 1, "#新游戏 测试游戏");
}

TEST_F(TestBot, pri_repeat_new_pub_game)
{
  NewGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, 1, 1, "#新游戏 测试游戏");
}

// Join Self Game

TEST_F(TestBot, pub_join_self_game)
{
  NewGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, 1, 1, "#加入游戏");
}

TEST_F(TestBot, pri_join_self_game)
{
  NewGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_USER_ALREADY_IN_MATCH, 1, "#加入游戏 1");
}

// Join Other Game After Creating Game

TEST_F(TestBot, pub_join_other_pub_game)
{
  NewGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, 2, 2, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_USER_ALREADY_IN_OTHER_MATCH, 2, 1, "#加入游戏");
}

TEST_F(TestBot, pub_join_other_pri_game)
{
  NewGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, 2, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_USER_ALREADY_IN_OTHER_MATCH, 1, "#加入游戏 2");
}

TEST_F(TestBot, pri_join_other_pub_game)
{
  NewGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, 2, 2, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_USER_ALREADY_IN_OTHER_MATCH, 2, 1, "#加入游戏");
}

TEST_F(TestBot, pri_join_other_pri_game)
{
  NewGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, 2, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_USER_ALREADY_IN_OTHER_MATCH, 1, "#加入游戏 2");
}

// Start Game Failed

TEST_F(TestBot, pub_start_game_not_host)
{
  NewGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 2, "#加入游戏");
  ASSERT_PUB_MSG(EC_MATCH_NOT_HOST, 1, 2, "#开始游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#开始游戏");
}

TEST_F(TestBot, pri_start_game_not_host)
{
  NewGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, 2, "#加入游戏 1");
  ASSERT_PRI_MSG(EC_MATCH_NOT_HOST, 2, "#开始游戏");
  ASSERT_PRI_MSG(EC_OK, 1, "#开始游戏");
}

TEST_F(TestBot, pub_start_game_not_in_game)
{
  NewGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 2, "#加入游戏");
  ASSERT_PUB_MSG(EC_MATCH_USER_NOT_IN_MATCH, 1, 3, "#开始游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#开始游戏");
}

TEST_F(TestBot, pri_start_game_not_in_game)
{
  NewGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, 2, "#加入游戏 1");
  ASSERT_PRI_MSG(EC_MATCH_USER_NOT_IN_MATCH, 3, "#开始游戏");
  ASSERT_PRI_MSG(EC_OK, 1, "#开始游戏");
}

TEST_F(TestBot, pub_start_other_pub_game)
{
  NewGame("测试游戏", 2);
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
  NewGame("测试游戏", 2);
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
  NewGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 2, "#加入游戏");
  ASSERT_PRI_MSG(EC_OK, 1, "#开始游戏");
}

// Exit Not Existing Game

TEST_F(TestBot, pub_exit_not_exist_game)
{
  NewGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_MATCH_USER_NOT_IN_MATCH, 1, 1, "#退出游戏");
}

TEST_F(TestBot, pri_exit_not_exist_game)
{
  NewGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_MATCH_USER_NOT_IN_MATCH, 1, "#退出游戏");
}

// Exit And New Game

TEST_F(TestBot, exit_pub_game_then_new_pub_game_same_group)
{
  NewGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#退出游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
}

TEST_F(TestBot, exit_pub_game_then_new_pub_game_other_group)
{
  NewGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#退出游戏");
  ASSERT_PUB_MSG(EC_OK, 2, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_MATCH_GROUP_NOT_IN_MATCH, 1, 2, "#加入游戏");
}

TEST_F(TestBot, exit_pri_game_then_new_pub_game)
{
  NewGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, 1, "#退出游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_NOT_EXIST, 2, "#加入游戏 1");
}

TEST_F(TestBot, exit_pri_game_then_new_pri_game)
{
  NewGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, 1, "#退出游戏");
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_NOT_EXIST, 2, "#加入游戏 1");
  ASSERT_PRI_MSG(EC_OK, 2, "#加入游戏 2");
}

// Exit And Join Game

TEST_F(TestBot, exit_pub_game_then_join_pub_game_same_group)
{
  NewGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 2, "#加入游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#退出游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#加入游戏");
}

TEST_F(TestBot, exit_pub_game_then_join_pub_game_other_group)
{
  NewGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, 2, 2, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#退出游戏");
  ASSERT_PUB_MSG(EC_OK, 2, 1, "#加入游戏");
}

TEST_F(TestBot, exit_pri_game_then_join_pub_game)
{
  NewGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 2, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, 1, "#退出游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#加入游戏");
}

TEST_F(TestBot, exit_pri_game_then_join_pri_game)
{
  NewGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, 2, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, 1, "#退出游戏");
  ASSERT_PRI_MSG(EC_MATCH_NOT_EXIST, 1, "#加入游戏 1");
  ASSERT_PRI_MSG(EC_OK, 1, "#加入游戏 2");
}

// Switch Host

TEST_F(TestBot, switch_host)
{
  NewGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 2, "#加入游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#退出游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 3, "#加入游戏");
  ASSERT_PUB_MSG(EC_MATCH_USER_NOT_IN_MATCH, 1, 1, "#开始游戏");
  ASSERT_PUB_MSG(EC_MATCH_NOT_HOST, 1, 3, "#开始游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 2, "#开始游戏");
}

// Config Game

TEST_F(TestBot, config_game)
{
  NewGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏 配置");
  ASSERT_PUB_MSG(EC_MATCH_IN_CONFIG, 1, 1, "#开始游戏");
  ASSERT_PUB_MSG(EC_MATCH_IN_CONFIG, 1, 2, "#加入游戏");
  ASSERT_PUB_MSG(EC_MATCH_USER_NOT_IN_MATCH, 1, 2, "#配置完成");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#配置完成");
  ASSERT_PUB_MSG(EC_OK, 1, 2, "#加入游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#开始游戏");
}

TEST_F(TestBot, config_game_exit)
{
  NewGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏 配置");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#退出游戏");
  ASSERT_PUB_MSG(EC_MATCH_GROUP_NOT_IN_MATCH, 1, 2, "#加入游戏");
}

// Player Limit

TEST_F(TestBot, exceed_max_player)
{
  NewGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 2, "#加入游戏");
  ASSERT_PUB_MSG(EC_MATCH_ACHIEVE_MAX_PLAYER, 1, 3, "#加入游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#开始游戏");
}

// Administor

TEST_F(TestBot, interrupt_private_without_mid)
{
  NewGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_MATCH_NEED_REQUEST_PUBLIC, k_admin_qq, "%中断游戏");
}

TEST_F(TestBot, interrupt_public_not_game)
{
  NewGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_MATCH_GROUP_NOT_IN_MATCH, 1, k_admin_qq, "%中断游戏");
}

TEST_F(TestBot, interrupt_public_config)
{
  NewGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏 配置");
  ASSERT_PUB_MSG(EC_OK, 1, k_admin_qq, "%中断游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
}

TEST_F(TestBot, interrupt_public_wait)
{
  NewGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 2, "#加入游戏");
  ASSERT_PUB_MSG(EC_OK, 1, k_admin_qq, "%中断游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
}

TEST_F(TestBot, interrupt_public_start)
{
  NewGame("测试游戏", 2);
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 2, "#加入游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#开始游戏");
  ASSERT_PUB_MSG(EC_OK, 1, k_admin_qq, "%中断游戏");
  ASSERT_PUB_MSG(EC_OK, 1, 1, "#新游戏 测试游戏");
}

TEST_F(TestBot, interrupt_private_wait)
{
  NewGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏 配置");
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%中断游戏 1");
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
}

TEST_F(TestBot, interrupt_private_config)
{
  NewGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, 2, "#加入游戏 1");
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%中断游戏 1");
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
}

TEST_F(TestBot, interrupt_private_start)
{
  NewGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, 2, "#加入游戏 1");
  ASSERT_PRI_MSG(EC_OK, 1, "#开始游戏");
  ASSERT_PRI_MSG(EC_OK, k_admin_qq, "%中断游戏 1");
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
}

TEST_F(TestBot, interrupt_private_wait_in_public)
{
  NewGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏 配置");
  ASSERT_PUB_MSG(EC_OK, 999, k_admin_qq, "%中断游戏 1");
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
}

TEST_F(TestBot, interrupt_private_config_in_public)
{
  NewGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, 2, "#加入游戏 1");
  ASSERT_PUB_MSG(EC_OK, 999, k_admin_qq, "%中断游戏 1");
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
}

TEST_F(TestBot, interrupt_private_start_in_public)
{
  NewGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_OK, 2, "#加入游戏 1");
  ASSERT_PRI_MSG(EC_OK, 1, "#开始游戏");
  ASSERT_PUB_MSG(EC_OK, 999, k_admin_qq, "%中断游戏 1");
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
}

// Computer Player

TEST_F(TestBot, set_computer)
{
  NewGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏 电脑");
  ASSERT_PRI_MSG(EC_OK, 1, "#电脑数量 1");
  ASSERT_PRI_MSG(EC_MATCH_ACHIEVE_MAX_PLAYER, 2, ("#加入游戏 1"));
  ASSERT_PRI_MSG(EC_OK, 1, "#开始游戏");
}

TEST_F(TestBot, set_computer_config)
{
  NewGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏 配置 电脑");
  ASSERT_PRI_MSG(EC_OK, 1, "#电脑数量 1");
  ASSERT_PRI_MSG(EC_OK, 1, "#配置完成");
  ASSERT_PRI_MSG(EC_MATCH_ACHIEVE_MAX_PLAYER, 2, ("#加入游戏 1"));
  ASSERT_PRI_MSG(EC_OK, 1, "#开始游戏");
}

TEST_F(TestBot, not_allow_set_computer)
{
  NewGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏");
  ASSERT_PRI_MSG(EC_MATCH_FORBIDDEN_OPERATION, 1, "#电脑数量 1");
}

TEST_F(TestBot, computer_exceed_max_player)
{
  NewGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏 电脑");
  ASSERT_PRI_MSG(EC_MATCH_ACHIEVE_MAX_PLAYER, 1, "#电脑数量 2");
  ASSERT_PRI_MSG(EC_OK, 2, ("#加入游戏 1"));
}

TEST_F(TestBot, computer_exceed_max_player_when_config)
{
  NewGame("测试游戏", 2);
  ASSERT_PRI_MSG(EC_OK, 1, "#新游戏 测试游戏 配置 电脑");
  ASSERT_PRI_MSG(EC_MATCH_ACHIEVE_MAX_PLAYER, 1, "#电脑数量 2");
  ASSERT_PRI_MSG(EC_OK, 1, ("#配置完成"));
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  g_argc = argc;
  g_argv = argv;
  return RUN_ALL_TESTS();
}
