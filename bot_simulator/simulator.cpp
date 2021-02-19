#include "bot_core/bot_core.h"
#include "utility/msg_sender.h"
#include "linenoise/linenoise.h"
#include <iostream>
#include <sstream>
#include <cassert>
#include <vector>
#include <gflags/gflags.h>

DEFINE_string(game_path, "plugins", "The path of game modules");
DEFINE_string(history_filename, ".simulator_history.txt", "The file saving history commands");
DEFINE_bool(color, true, "Enable color");
DEFINE_uint64(bot_uid, 114514, "The UserID of bot");
DEFINE_uint64(admin_uid, 0, "The UserID of administor");

DEFINE_string(db_addr, "", "Address of database <ip>:<port>");
DEFINE_string(db_user, "root", "User of database");
DEFINE_string(db_name, "lgtbot_simulator", "Name of database");
DEFINE_string(db_passwd, "", "Password of database");

const char* Red() { return FLAGS_color ? "\033[31m" : ""; }
const char* Green() { return FLAGS_color ? "\033[32m" : ""; }
const char* Yellow() { return FLAGS_color ? "\033[33m" : ""; }
const char* Blue() { return FLAGS_color ? "\033[34m" : ""; }
const char* Purple() { return FLAGS_color ? "\033[35m" : ""; }
const char* LightPink() { return FLAGS_color ? "\033[1;95m" : ""; }
const char* LightCyan() { return FLAGS_color ? "\033[96m" : ""; }
const char* Default() { return FLAGS_color ? "\033[0m" : ""; }

static const char* ErrCodeColor(const ErrCode rc)
{
  switch (rc)
  {
    case EC_OK:
    case EC_GAME_REQUEST_OK:
      return Green();
    case EC_GAME_REQUEST_CHECKOUT:
      return Yellow();
    default:
      return Red();
  }
}

std::ostream& Error() { return std::cerr << Red() << "[ERROR] " << Default(); }
std::ostream& Log() { return std::clog << Blue() << "[LOG] " << Default(); }

class MyMsgSender : public MsgSender
{
 public:
  MyMsgSender(const Target target, const uint64_t id) : target_(target), id_(id) {}
  virtual ~MyMsgSender()
  {
    if (target_ == TO_USER)
    {
      std::cout << Blue() << "[BOT -> USER_" << id_ << "]" << Default() << std::endl << ss_.str() << std::endl;
    }
    else if (target_ == TO_GROUP)
    {
      std::cout << Purple() << "[BOT -> GROUP_" << id_ << "]" << Default() << std::endl << ss_.str() << std::endl;
    }
  }
  virtual void SendString(const char* const str, const size_t len) override
  {
    ss_ << std::string_view(str, len);
  }
  virtual void SendAt(const uint64_t uid) override
  {
    ss_ << LightPink() << "@" << uid << Default();
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

auto init_bot(int argc, char** argv)
{
  const char* errmsg = nullptr;
}

std::pair<std::string_view, std::string_view> cut(const std::string_view line)
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

bool handle_request(void* bot, const std::string_view line)
{
  const auto [gid_s, gid_remain_s] = cut(line);
  const auto [uid_s, request_s] = cut(gid_remain_s);

  if (gid_s.empty() || uid_s.empty() || request_s.empty())
  {
    Error() << "Invalid request format" << std::endl;
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
    Error() << "Invalid GroupID \'" << gid_s << "\', can only be integer or \'pri\'" << std::endl;;
    return false;
  }

  try
  {
    uid = std::stoull(uid_s.data());
  }
  catch (const std::invalid_argument& e)
  {
    Error() << "Invalid UserID \'" << uid_s << "\', can only be integer" << std::endl;;
    return false;
  }

  ErrCode rc = gid.has_value() ? BOT_API::HandlePublicRequest(bot, gid.value(), uid, request_s.data()) :
                                      BOT_API::HandlePrivateRequest(bot, uid, request_s.data());
  std::cout << ErrCodeColor(rc) << "Error Code: " << errcode2str(rc) << Default() << std::endl;

  return true;
}

static void ConnectDatabase(void* const bot)
{
  if (!FLAGS_db_addr.empty())
  {
    const char* errmsg = nullptr;
    if (FLAGS_db_passwd.empty())
    {
      char passwd[128] = {0};
      std::cout << "Password: ";
      std::cin >> passwd;
      BOT_API::ConnectDatabase(bot, FLAGS_db_addr.c_str(), FLAGS_db_user.c_str(), passwd, FLAGS_db_name.c_str(), &errmsg);
    }
    else
    {
      BOT_API::ConnectDatabase(bot, FLAGS_db_addr.c_str(), FLAGS_db_user.c_str(), FLAGS_db_passwd.c_str(), FLAGS_db_name.c_str(), &errmsg);
    }
    if (errmsg)
    {
      Error() << "Connect database failed errmsg:" << *errmsg << std::endl;
    }
    else
    {
      Log() << "Connect database success" << std::endl;
    }
  }
}

int main(int argc, char** argv)
{
  std::locale::global(std::locale(""));
  gflags::ParseCommandLineFlags(&argc, &argv, true); 
  auto bot = BOT_API::Init(FLAGS_bot_uid, create_msg_sender, delete_msg_sender, FLAGS_game_path.c_str(), &FLAGS_admin_uid, 1);
  ConnectDatabase(bot);

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
      linenoiseFree(line_cstr);
      std::cout << "Bye." << std::endl;
      break;
    }
    else if (handle_request(bot, line))
    {
      linenoiseHistoryAdd(line_cstr);
      linenoiseHistorySave(FLAGS_history_filename.c_str());
    }
    else
    {
      Error() << "Usage: <GroupID | \'pri\'> <UserID> <arg1> <arg2> ..." << std::endl;
    }
    linenoiseFree(line_cstr);
  }

  BOT_API::Release(bot);

  return 0;
}

