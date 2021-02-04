#include "utility/msg_sender.h"
#include "bot_core/bot_core.h"
#include <gtest/gtest.h>
#include <gflags/gflags.h>

constexpr const UserID k_this_qq = 114514;
static int g_argc = 0;
static char** g_argv = nullptr;

typedef bool(*InitBotFunc)(const UserID, const NEW_MSG_SENDER_CALLBACK, const DELETE_MSG_SENDER_CALLBACK, int argc, char** argv);
typedef ErrCode(*HandlePrivateRequestFunc)(const UserID uid, const char* const msg);
typedef ErrCode(*HandlePublicRequestFunc)(const UserID uid, const GroupID gid, const char* const msg);

class MyMsgSender : public MsgSender
{
 public:
  virtual ~MyMsgSender() {}
  virtual void SendString(const char* const str, const size_t len) override {}
  virtual void SendAt(const uint64_t uid) override {}
};

MsgSender* create_msg_sender(const Target target, const UserID id)
{
  return new MyMsgSender();
}

void delete_msg_sender(MsgSender* const msg_sender)
{
  delete msg_sender;
}
class TestBot : public testing::Test
{
 public:
  virtual void SetUp()
  {
    ASSERT_TRUE(InitBot(k_this_qq, create_msg_sender, delete_msg_sender, g_argc, g_argv));
  }

  virtual void SetDown()
  {
  }
};

TEST_F(TestBot, BotLinked)
{
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  g_argc = argc;
  g_argv = argv;
  return RUN_ALL_TESTS();
}
