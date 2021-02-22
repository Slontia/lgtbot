#include <memory>
#include <map>
#include <functional>
#include <vector>
#include <iostream>
#include "game.h"
#include "game_main.h"
#include "util.h"

NEW_BOARDCAST_MSG_SENDER_CALLBACK g_new_boardcast_msg_sender_cb = nullptr;
NEW_TELL_MSG_SENDER_CALLBACK g_new_tell_msg_sender_cb = nullptr;
DELETE_MSG_SENDER_CALLBACK g_delete_msg_sender_cb = nullptr;
GAME_PREPARE_CALLBACK g_game_prepare_cb = nullptr;
GAME_OVER_CALLBACK g_game_over_cb = nullptr;
START_TIMER_CALLBACK g_start_timer_cb = nullptr;
STOP_TIMER_CALLBACK g_stop_timer_cb = nullptr;

bool /*__cdecl*/ Init(
    const NEW_BOARDCAST_MSG_SENDER_CALLBACK new_boardcast_msg_sender,
    const NEW_TELL_MSG_SENDER_CALLBACK new_tell_msg_sender,
    const DELETE_MSG_SENDER_CALLBACK delete_msg_sender,
    const GAME_PREPARE_CALLBACK game_prepare,
    const GAME_OVER_CALLBACK game_over,
    const START_TIMER_CALLBACK start_timer,
    const STOP_TIMER_CALLBACK stop_timer)
{
  g_new_boardcast_msg_sender_cb = new_boardcast_msg_sender;
  g_new_tell_msg_sender_cb = new_tell_msg_sender;
  g_delete_msg_sender_cb = delete_msg_sender;
  g_game_prepare_cb = game_prepare;
  g_game_over_cb = game_over;
  g_start_timer_cb = start_timer;
  g_stop_timer_cb = stop_timer;
  return true;
}

const char* /*__cdecl*/ GameInfo(uint64_t* max_player, const char** rule)
{
  if (!max_player || !rule) { return nullptr; }
  *max_player = k_max_player;
  *rule = Rule();
  return k_game_name.c_str();
}

GameBase* /*__cdecl*/ NewGame(void* const match)
{
  Game* game = new Game(match);
  return game;
}

void /*__cdecl*/ DeleteGame(GameBase* const game)
{
  delete(game);
}

