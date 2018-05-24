#pragma once

#include <iostream>
#include <vector>
#include <functional>
#include "game_state.h"

class GamePlayer;
template <class ID> class GameState;

class Game
{
protected:
  enum ID;
	const std::string                   kGameId = "undefined";
	const int                           kMinPlayer = 2;
	const int                           kMaxPlayer = 0;	// set 0 if no upper limit
  std::vector<GamePlayer>             players_;
  std::shared_ptr<GameState<ID>>      main_state_;
                                      Game();
  void                                Reply(uint32_t, std::string, int32_t) const;
  void                                Broadcast(std::string, int32_t) const;
  void                                Join(GamePlayer player);  // add new player
  void                                Begin();
  virtual void                        Request(uint32_t pid, const char* msg, int32_t sub_type);
};
