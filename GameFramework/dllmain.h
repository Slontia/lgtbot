#pragma once
#include <stdint.h>
#include "game_base.h"

extern "C"
{
  typedef void (*boardcast)(const uint64_t mid, const char* const msg);
  typedef void (*tell)(const uint64_t mid, const uint64_t pid, const char* const msg);
  typedef void (*at)(const uint64_t mid, const uint64_t pid, char* buf, const uint64_t len);
  typedef void (*game_over)(const uint64_t mid, const int64_t scores[]);
 
  __declspec(dllexport) bool __cdecl Init(const boardcast boardcast, const tell tell, const at at, const game_over game_over);
  __declspec(dllexport) const char* __cdecl GameInfo(uint64_t* min_player, uint64_t* max_player);
  __declspec(dllexport) GameBase* __cdecl NewGame(const uint64_t match_id, const uint64_t player_num);
  /* game should be delete in game dll */
  __declspec(dllexport) void __cdecl DeleteGame(GameBase* const game);

}