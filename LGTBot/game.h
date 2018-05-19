#pragma once

#include <iostream>
#include <vector>
#include <functional>
#include "game_state.h"

class GamePlayer;

class Game
{
protected:
	const std::string                   kGameId = "undefined";
	const int                           kMinPlayer = 2;
	const int                           kMaxPlayer = 0;	// set 0 if no upper limit
  std::vector<GamePlayer>             players_;
  GameState                           main_state_;
                                      Game(int player_num);
  void                                Reply(uint32_t, std::string, int32_t) const;
  void                                Broadcast(std::string, int32_t) const;
  virtual void                        Request(uint32_t pid, const char* msg, int32_t sub_type);
};