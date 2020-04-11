#pragma once
#include "bot_core.h"
#include "../GameFramework/dllmain.h"

#define DLL_EXPORT __declspec(dllexport)
#define DLL_IMPORT __declspec(dllimport)

#define RETURN_IF_FAILED(str) \
do\
{\
  if (const std::string& err = (str); !err.empty())\
    return err;\
} while (0);

struct GameHandle
{
  GameHandle(const std::optional<uint64_t> game_id, const std::string& name, const uint64_t min_player, const uint64_t max_player, const std::string& rule,
    const std::function<GameBase*(void* const, const uint64_t)>& new_game, const std::function<void(GameBase* const)>& delete_game,
    const HINSTANCE& module)
    : game_id_(game_id), name_(name), min_player_(min_player), max_player_(max_player), rule_(rule),
    new_game_(new_game), delete_game_(delete_game), module_(module) {}
  GameHandle(GameHandle&&) = delete;
  ~GameHandle() { FreeLibrary(module_); }
  
  std::atomic<std::optional<uint64_t>> game_id_;
  const std::string name_;
  const uint64_t min_player_;
  const uint64_t max_player_;
  const std::string rule_;
  const std::function<GameBase*(void* const, const uint64_t)> new_game_;
  const std::function<void(GameBase* const)> delete_game_;
  const HINSTANCE module_;
};

extern const int32_t LGT_AC;
extern const UserID INVALID_USER_ID;
extern const GroupID INVALID_GROUP_ID;

extern std::mutex g_mutex;
extern std::map<std::string, std::unique_ptr<GameHandle>> g_game_handles;

extern AT_CALLBACK g_at_cb;
extern PRIVATE_MSG_CALLBACK g_send_pri_msg_cb;
extern PUBLIC_MSG_CALLBACK g_send_pub_msg_cb;
extern UserID g_this_uid;

static void At(const UserID uid, char* buf, const uint64_t len) { g_at_cb(uid, buf, len); }
static std::string At(const UserID uid)
{
  char buf[128] = { 0 };
  g_at_cb(uid, buf, 128);
  return std::string(buf);
}
static void SendPrivateMsg(const UserID uid, const std::string& msg) { return g_send_pri_msg_cb(uid, msg.c_str()); }
static void SendPublicMsg(const GroupID gid, const std::string& msg) { return g_send_pub_msg_cb(gid, msg.c_str()); }
