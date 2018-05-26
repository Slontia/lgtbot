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
  StateContainer<ID>&                 state_container_;
  std::vector<GamePlayer>             players_;
  std::shared_ptr<GameState<ID>>      main_state_;
public:
                                      Game();
  /* send msg to a specific player */
  void                                Reply(uint32_t, std::string, int32_t) const;
  /* send msg to all player */
  void                                Broadcast(std::string, int32_t) const;
  /* add new player */
  void                                Join(GamePlayer player);
  /* create the main_state_ with proper player number */
  bool                                Begin();
  /* write results to the database */
  void                                Over();
  /* transmit msg to main_state_ */
  void                                Request(uint32_t pid, const char* msg, int32_t sub_type);
};


