#pragma once
#include <stdint.h>
#include "game_base.h"

extern "C"
{
  typedef void (*boardcast)(const uint64_t game_id, char* msg);
  typedef void (*tell)(const uint64_t game_id, const uint64_t player_id, char* msg);
  typedef char* (*at)(const uint64_t game_id, const uint64_t player_id);
  typedef void (*game_over)(const uint64_t game_id, const int64_t scores[]);

  __declspec(dllexport) int __cdecl Init(const boardcast boardcast, const tell tell, const at at, const game_over game_over);
  __declspec(dllexport) char* __cdecl GameInfo(uint64_t* min_player, uint64_t* max_player);
  __declspec(dllexport) GameBase* __cdecl NewGame(const uint64_t match_id, const uint64_t player_num);

}