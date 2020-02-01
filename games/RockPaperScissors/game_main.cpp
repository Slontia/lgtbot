
#include <memory>
#include <map>
#include <functional>
#include <vector>
#include <iostream>
#include "game.h"
#include "dllmain.h"
#include "mygame.h"

typedef void (*request_f)(char const* msg);

bool __cdecl Init(const boardcast boardcast, const tell tell, const at at, const game_over game_over)
{
  boardcast_f = [boardcast](const uint64_t mid, const std::string& msg) { boardcast(mid, msg.c_str()); };
  tell_f = [tell](const uint64_t mid, const uint64_t pid, const std::string& msg) { tell(mid, pid, msg.c_str()); };
  at_f = [at](const uint64_t mid, const uint64_t pid)
  {
    char buf[128] = { 0 };
    at(mid, pid, buf, 128);
    return std::string(buf);
  };
  game_over_f = [game_over](const uint64_t mid, const std::vector<int64_t>& scores) { game_over(mid, scores.data()); };
  return true;
}

char* __cdecl GameInfo(uint64_t* min_player, uint64_t* max_player)
{

  return NULL;
}

GameBase* __cdecl NewGame(const uint64_t match_id, const uint64_t player_num)
{
  return new Game<StageEnum, GameEnv>(match_id, MakeGameEnv(player_num), MakeMainStage());
}

void __cdecl DeleteGame(GameBase* const game)
{
  delete(game);
}
