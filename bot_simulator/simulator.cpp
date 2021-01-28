#include "bot_core/bot_core.h"
#include "utility/msg_sender.h"
#include "linenoise/linenoise.h"
#include <iostream>
#include <sstream>
#include <cassert>
#include <vector>
#include <gflags/gflags.h>

DEFINE_string(history_filename, ".simulator_history.txt", "The file saving history commands");
DEFINE_bool(color, true, "Enable color");
DEFINE_uint64(bot_uid, 114514, "The UserID of bot");

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

void init_bot(int argc, char** argv)
{
  const char* errmsg = nullptr;
  BOT_API::Init(FLAGS_bot_uid, create_msg_sender, delete_msg_sender, argc, argv);
}

bool handle_request(const std::string_view line)
{
  const auto cut = [](const std::string_view line) -> std::pair<std::string_view, std::string_view>
  {
    if (const auto start_pos = line.find_first_not_of(' '); start_pos == std::string_view::npos)
    {
      return { std::string_view(), std::string_view() };
    }
    else if (const auto end_pos = line.find_first_of(' ', start_pos); end_pos == std::string_view::npos)
    {
      return { line.substr(start_pos), std::string_view() };
    }
    else
    {
      return { line.substr(start_pos, end_pos - start_pos), line.substr(end_pos) };
    }
  };

  const auto [gid_s, gid_remain_s] = cut(line);
  const auto [uid_s, request_s] = cut(gid_remain_s);

  if (gid_s.empty() || uid_s.empty() || request_s.empty())
  {
    std::cerr << "[ERROR] Invalid request format" << std::endl;
    return false;
  }

  std::optional<uint64_t> gid;
  uint64_t uid = 0;

  try
  {
    if (gid_s != "pri") { gid = std::stoull(gid_s.data()); }
  }
  catch (const std::invalid_argument& e)
  {
    std::cerr << "[ERROR] Invalid GroupID \'" << gid_s << "\', can only be integer or \'pri\'" << std::endl;;
    return false;
  }

  try
  {
    uid = std::stoull(uid_s.data());
  }
  catch (const std::invalid_argument& e)
  {
    std::cerr << "[ERROR] Invalid UserID \'" << uid_s << "\', can only be integer" << std::endl;;
    return false;
  }

  if (gid.has_value())
  {
    BOT_API::HandlePublicRequest(uid, gid.value(), request_s.data());
  }
  else
  {
    BOT_API::HandlePrivateRequest(uid, request_s.data());
  }
  return true;
}

int main(int argc, char** argv)
{
  std::locale::global(std::locale(""));
  init_bot(argc, argv);

  linenoiseHistoryLoad(FLAGS_history_filename.c_str());
  for (char* line_cstr = nullptr; (line_cstr = linenoise("Simulator>>> ")) != nullptr; )
  {
    const std::string_view line(line_cstr);
    if (line.find_first_not_of(' ') == std::string_view::npos)
    {
      // do nothing
    }
    else if (line == "quit")
    {
      free(line_cstr);
      std::cout << "Bye." << std::endl;
      break;
    }
    else if (handle_request(line))
    {
      linenoiseHistoryAdd(line_cstr);
      linenoiseHistorySave(FLAGS_history_filename.c_str());
    }
    else
    {
      std::cerr << "[ERROR] Usage: <GroupID | \'pri\'> <UserID> <arg1> <arg2> ..." << std::endl;
    }
    free(line_cstr);
  }

  return 0;
}

