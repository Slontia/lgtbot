#pragma once
#include "bot_core.h"
#include "utility/msg_sender.h"
#include "game_framework/game_main.h"
#include "utility/msg_sender.h"
#include <functional>
#include <optional>
#include <map>
#include <functional>
#include <optional>
#include <mutex>
#include <atomic>

#define RETURN_IF_FAILED(str) \
do\
{\
  if (const std::string& err = (str); !err.empty())\
    return err;\
} while (0);

using ModGuard = std::function<void()>;

struct GameHandle
{
  GameHandle(const std::optional<uint64_t> game_id, const std::string& name, const uint64_t min_player, const uint64_t max_player, const std::string& rule,
    const std::function<GameBase*(void* const)>& new_game, const std::function<void(GameBase* const)>& delete_game,
    ModGuard&& mod_guard)
    : game_id_(game_id), name_(name), min_player_(min_player), max_player_(max_player), rule_(rule),
    new_game_(new_game), delete_game_(delete_game), mod_guard_(std::forward<ModGuard>(mod_guard)) {}
  GameHandle(GameHandle&&) = delete;
  
  std::atomic<std::optional<uint64_t>> game_id_;
  const std::string name_;
  const uint64_t min_player_;
  const uint64_t max_player_;
  const std::string rule_;
  const std::function<GameBase*(void* const)> new_game_;
  const std::function<void(GameBase* const)> delete_game_;
  ModGuard mod_guard_;
};

extern const int32_t LGT_AC;
extern const UserID INVALID_USER_ID;
extern const GroupID INVALID_GROUP_ID;

extern std::mutex g_mutex;
extern std::map<std::string, std::unique_ptr<GameHandle>> g_game_handles;

extern NEW_MSG_SENDER_CALLBACK g_new_msg_sender_cb;
extern DELETE_MSG_SENDER_CALLBACK g_delete_msg_sender_cb;

extern UserID g_this_uid;

inline MsgSenderWrapper ToUser(const UserID uid)
{
  return MsgSenderWrapper(std::unique_ptr<MsgSender, void (*)(MsgSender* const)>(g_new_msg_sender_cb(Target::TO_USER, uid), g_delete_msg_sender_cb));
}

inline MsgSenderWrapper ToGroup(const GroupID gid)
{
  return MsgSenderWrapper(std::unique_ptr<MsgSender, void (*)(MsgSender* const)>(g_new_msg_sender_cb(Target::TO_GROUP, gid), g_delete_msg_sender_cb));
}
