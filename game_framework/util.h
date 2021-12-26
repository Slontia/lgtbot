#pragma once
#include <string>

#include "game_framework/game_main.h"
#include "bot_core/msg_sender.h"

extern MsgSenderBase& BoardcastMsgSender(void* match_p);
extern MsgSenderBase& TellMsgSender(void* match_p, const uint64_t pid);
extern void StartTimer(void* match, const uint64_t sec);
extern void StopTimer(void* match);
extern const char* Rule();

extern const std::string k_game_name;
extern const uint64_t k_max_player;
extern const uint64_t k_multiple;

