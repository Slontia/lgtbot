// Simulator.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "../BotCore/lgtbot.h"
#include <iostream>
#include <sstream>
#include <cassert>

void at_cq(const UserID& usr_qq, char* buf, const uint64_t len)
{
  sprintf_s(buf, len, "<@%llu>", usr_qq);
}

void send_private_msg(const UserID& usr_qq, const char* msg)
{
  std::cout << "[private -> user: " << usr_qq << "]" << std::endl << msg << std::endl;
}

void send_group_msg(const UserID& group_qq, const char* msg)
{
  std::cout << "[public -> group: " << group_qq << "]" << std::endl << msg << std::endl;
}

void init_bot()
{
  BOT_API::Init(114514, send_private_msg, send_group_msg, at_cq);
}

void commands()
{
  BOT_API::HandlePublicRequest(1, 1, "#规则 猜拳游戏");
}

int main()
{
  init_bot();
 
  commands();

  std::cout << "> ";

  for (std::string s; std::getline(std::cin, s); )
  {
    try
    {
      std::stringstream ss(s);

      uint64_t uid = 0;
      uint64_t gid = 0;
      if (!(ss >> uid >> gid)) { throw ""; }

      std::string args;
      for (std::string arg; ss >> arg; )
      {
        args += arg;
        args += " ";
      }

      if (gid == 0) { BOT_API::HandlePrivateRequest(uid, args.c_str()); }
      else { BOT_API::HandlePublicRequest(uid, gid, args.c_str()); }
      
    }
    catch (...) { std::cout << "[ERROR] usage: <UserID> <GroupID | 0> <arg1> <arg2> ..." << std::endl; }
    std::cout << "> ";
  }
  
  return 0;
}

