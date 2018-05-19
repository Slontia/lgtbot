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
  void                GameStart(); // switch status, create game
private:
  static GameCountainer             game_container_;
  enum { PREPARE, GAMING, OVER }    status_;
  std::string                       game_id_;
  int64_t                           host_qq_;
  Game                              game_;
  std::map<int64_t, uint32_t>       qq2pid_;
};