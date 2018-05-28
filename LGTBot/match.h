#pragma once

#include <iostream>
#include <map>
#include "game.h"

class GamePlayer;     // player info
class GameCountainer;

class Match
{
public:
                      Match(std::string game_id, int64_t host_qq);
  std::string         MsgHandle(int64_t from_qq, const char* msg, int32_t subType);
  /* switch status, create game */
  void                GameStart();
  /* send msg to a specific player */
  void                                Reply(uint32_t pid, std::string) const;
  /* send msg to all player */
  void                                Broadcast(std::string msg) const;
  /* bind to map, create GamePlayer and send to game */
  void                              Join(int64_t player_qq);
private:
  static GameCountainer             game_container_;
  enum { PREPARE, GAMING, OVER }    status_;
  std::string                       game_id_;
  int64_t                           host_qq_;
  std::shared_ptr<Game>             game_;
  std::map<int64_t, uint32_t>       qq2pid_;
  uint32_t                          player_count;
};  