#pragma once
#include "game_options.h"
#include "utility/msg_sender.h"
#include "game_main.h"
#include <string>

class MainStageBase;
class Game;

extern NEW_BOARDCAST_MSG_SENDER_CALLBACK g_new_boardcast_msg_sender_cb;
extern NEW_TELL_MSG_SENDER_CALLBACK g_new_tell_msg_sender_cb;
extern DELETE_GAME_MSG_SENDER_CALLBACK g_delete_msg_sender_cb;
extern GAME_PREPARE_CALLBACK g_game_prepare_cb;
extern GAME_OVER_CALLBACK g_game_over_cb;
extern START_TIMER_CALLBACK g_start_timer_cb;
extern STOP_TIMER_CALLBACK g_stop_timer_cb;

extern const std::string k_game_name;
extern const uint64_t k_min_player;
extern const uint64_t k_max_player;
extern const char* Rule();

using replier_t = std::function<MsgSenderWrapper<MsgSenderForGame>()>;

std::unique_ptr<MainStageBase> MakeMainStage(const replier_t& reply, const GameOption& options);

inline const auto Boardcast(void* const match)
{
  return MsgSenderWrapper(g_new_boardcast_msg_sender_cb(match), g_delete_msg_sender_cb);
}

inline const auto Tell(void* const match, const uint64_t pid)
{
  return MsgSenderWrapper(g_new_tell_msg_sender_cb(match, pid), g_delete_msg_sender_cb);
}

