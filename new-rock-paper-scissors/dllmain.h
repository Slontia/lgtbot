#pragma once
#include <stdint.h>

extern "C"
{
  typedef void (*boardcast)(const uint64_t game_id, char* msg);
  typedef void (*tell)(const uint64_t game_id, const uint64_t player_id, char* msg);
  typedef char* (*at)(const uint64_t game_id, const uint64_t player_id);
  typedef void (*game_over)(const uint64_t game_id, const int64_t scores[]);

  enum ERR_CODE
  {
    LGT_SUCCESS = 0,
    LGT_INVALID_ARGUMENT,
    LGT_GAME_NOT_FOUND,
  };

  __declspec(dllexport) ERR_CODE __cdecl Init(const boardcast boardcast, const tell tell, const at at, const game_over game_over);
  __declspec(dllexport) ERR_CODE __cdecl GameInfo(char** game_name, uint64_t* min_player, uint64_t* max_player);
  __declspec(dllexport) ERR_CODE __cdecl NewGame(const unsigned long player_num, uint64_t* game_id);
  __declspec(dllexport) ERR_CODE __cdecl HandleRequest(const uint64_t game_id, const uint64_t player_id, const bool is_public, const char* msg);

}