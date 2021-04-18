#pragma once
#include <stdint.h>

#include "game_base.h"

#ifdef _WIN32
#define DLLEXPORT(type) __declspec(dllexport) type __cdecl
#else
#define DLLEXPORT(type) type
#endif

class MsgSenderForGame;

extern "C" {
typedef MsgSenderForGame* (*NEW_BOARDCAST_MSG_SENDER_CALLBACK)(void* match);
typedef MsgSenderForGame* (*NEW_TELL_MSG_SENDER_CALLBACK)(void* match, const uint64_t pid);
typedef void (*DELETE_GAME_MSG_SENDER_CALLBACK)(MsgSenderForGame* const msg_sender);
typedef void (*GAME_PREPARE_CALLBACK)(void* match);
typedef void (*GAME_OVER_CALLBACK)(void* match, const int64_t scores[]);
typedef void (*START_TIMER_CALLBACK)(void* match, const uint64_t sec);
typedef void (*STOP_TIMER_CALLBACK)(void* match);

DLLEXPORT(bool)
Init(const NEW_BOARDCAST_MSG_SENDER_CALLBACK, const NEW_TELL_MSG_SENDER_CALLBACK, const DELETE_GAME_MSG_SENDER_CALLBACK,
     const GAME_PREPARE_CALLBACK, const GAME_OVER_CALLBACK, const START_TIMER_CALLBACK, const STOP_TIMER_CALLBACK);
DLLEXPORT(const char*) GameInfo(uint64_t* max_player, const char** rule);
DLLEXPORT(GameBase*) NewGame(void* const match);
/* game should be delete in game dll */
DLLEXPORT(void) DeleteGame(GameBase* const game);
}
