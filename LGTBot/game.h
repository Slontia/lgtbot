#pragma once

#include <iostream>
#include <vector>
#include <functional>
#include "time_trigger.h"
#include "game_state.h"


class GamePlayer;
class GameState;
class StateContainer;
class Match;

class Game
{
private:
  Match&                              match_;
protected:
  std::shared_ptr<StateContainer>     state_container_;
  std::vector<std::shared_ptr<GamePlayer>>             players_;
  std::shared_ptr<GameState>          main_state_;
public:
  static TimeTrigger                      timer_;
  const std::string                   kGameId = "undefined";
  const uint32_t                           kMinPlayer = 2;
  const uint32_t                           kMaxPlayer = 0;	// set 0 if no upper limit
                                      Game(Match& match);
  inline bool                         valid_pnum(uint32_t pnum);
  void                                set_state_container(std::shared_ptr<StateContainer> state_container);
  /* send msg to a specific player */
  void                                Reply(uint32_t pid, std::string) const;
  /* send msg to all player */
  void                                Broadcast(std::string msg) const;
  /* add new player */
  void                                Join(std::shared_ptr<GamePlayer> player);
  /* create main_state_ */
  bool                                StartGame();
  /* write results to the database */
  virtual bool                        RecordResult() = 0;
  /* transmit msg to main_state_ */
  void                                Request(uint32_t pid, const char* msg, int32_t sub_type);
};


