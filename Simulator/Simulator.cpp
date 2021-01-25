// Simulator.cpp: 定义控制台应用程序的入口点。
//

#include "BotCore/bot_core.h"
#include "Utility/msg_sender.h"
#include <iostream>
#include <sstream>
#include <cassert>
#include <vector>

class MyMsgSender : public MsgSender
{
 public:
  MyMsgSender(const Target target, const uint64_t id) : target_(target), id_(id) {}
  virtual ~MyMsgSender()
  {
    if (target_ == TO_USER)
    {
      std::cout << "[private -> user: " << id_ << "]" << std::endl << ss_.str() << std::endl;
    }
    else if (target_ == TO_GROUP)
    {
      std::cout << "[public -> group: " << id_ << "]" << std::endl << ss_.str() << std::endl;
    }
  }
  virtual void SendString(const char* const str, const size_t len) override
  {
    ss_ << std::string_view(str, len);
  }
  virtual void SendAt(const uint64_t uid) override
  {
    ss_ << "<@" << uid << ">";
  }

 private:
  const Target target_;
  const uint64_t id_;
  std::stringstream ss_;
};

MsgSender* create_msg_sender(const Target target, const UserID id)
{
  return new MyMsgSender(target, id);
}

void delete_msg_sender(MsgSender* const msg_sender)
{
  delete msg_sender;
}

void bug()
{
  BOT_API::HandlePublicRequest(1, 1, "#配置新游戏 猜拳游戏");
  BOT_API::HandlePublicRequest(1, 1, "局时 10");
  BOT_API::HandlePublicRequest(1, 1, "胜利局数 1");
  BOT_API::HandlePublicRequest(1, 1, "#配置完成");
  BOT_API::HandlePublicRequest(2, 1, "#加入游戏");
  BOT_API::HandlePublicRequest(1, 1, "#开始游戏");
  BOT_API::HandlePrivateRequest(1, "剪刀");
  BOT_API::HandlePrivateRequest(2, "石头");
}

void lie_commands()
{
  BOT_API::HandlePublicRequest(1, 1, "#新游戏 LIE");
  BOT_API::HandlePublicRequest(2, 1, "#加入游戏");
  BOT_API::HandlePublicRequest(1, 1, "#开始游戏");
  for (int num = 1; num <= 6; ++num)
  {
    BOT_API::HandlePrivateRequest(2, std::to_string(num).c_str());
    BOT_API::HandlePrivateRequest(2, std::to_string(num).c_str());
    BOT_API::HandlePrivateRequest(1, "相信");
  }
}

void rps_commands()
{
  BOT_API::HandlePublicRequest(1, 1, "#新游戏 猜拳游戏");
  BOT_API::HandlePublicRequest(2, 1, "#加入游戏");
  BOT_API::HandlePublicRequest(1, 1, "#开始游戏");

}

void init_bot(int argc, char** argv)
{
  const char* errmsg = nullptr;
  //std::cout << "code: " << BOT_API::ConnectDatabase("cdb-e4pq5sf8.cd.tencentcdb.com:10083", "root", "i8349276i", "lgt_bot_test", &errmsg) << std::endl;
  std::cout << "errmsg: " << (errmsg ? errmsg : "(null)");
  BOT_API::Init(114514, create_msg_sender, delete_msg_sender, argc, argv);
  lie_commands();
}

int main(int argc, char** argv)
{
  std::locale::global(std::locale(""));

  init_bot(argc, argv);
  std::cout << ">>> ";
  bug();

  for (std::string s; std::getline(std::cin, s); )
  {
    std::stringstream ss(s);

    uint64_t uid = 0;
    uint64_t gid = 0;
    if (!(ss >> uid >> gid))
    {
      std::cout << "[ERROR] usage: <UserID> <GroupID | 0> <arg1> <arg2> ..." << std::endl << ">>> ";
      continue;
    }

    std::string args;
    for (std::string arg; ss >> arg; )
    {
      args += arg;
      args += " ";
    }

    if (gid == 0) { BOT_API::HandlePrivateRequest(uid, args.c_str()); }
    else { BOT_API::HandlePublicRequest(uid, gid, args.c_str()); }
    std::cout << ">>> ";
  }
  
  return 0;
}

