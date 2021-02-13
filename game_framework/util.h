#pragma once
#include "game_options.h"
#include "utility/msg_sender.h"
#include "game_main.h"
#include <string>

class MainStageBase;
class Game;

extern NEW_BOARDCAST_MSG_SENDER_CALLBACK g_new_boardcast_msg_sender_cb;
extern NEW_TELL_MSG_SENDER_CALLBACK g_new_tell_msg_sender_cb;
extern DELETE_MSG_SENDER_CALLBACK g_delete_msg_sender_cb;
extern GAME_OVER_CALLBACK g_game_over_cb;
extern START_TIMER_CALLBACK g_start_timer_cb;
extern STOP_TIMER_CALLBACK g_stop_timer_cb;

extern const std::string k_game_name;
extern const uint64_t k_min_player;
extern const uint64_t k_max_player;
extern const char* Rule();

std::unique_ptr<MainStageBase> MakeMainStage(MsgSenderWrapper& sender, const GameOption& options);

inline MsgSenderWrapper Boardcast(void* const match)
{
  return MsgSenderWrapper(std::unique_ptr<MsgSender, void (*)(MsgSender* const)>(g_new_boardcast_msg_sender_cb(match), g_delete_msg_sender_cb));
}

inline MsgSenderWrapper Tell(void* const match, const uint64_t pid)
{
  return MsgSenderWrapper(std::unique_ptr<MsgSender, void (*)(MsgSender* const)>(g_new_tell_msg_sender_cb(match, pid), g_delete_msg_sender_cb));
}

using replier_t = std::function<MsgSenderWrapper()>;
