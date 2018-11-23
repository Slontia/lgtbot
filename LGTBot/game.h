#pragma once

#include <iostream>
#include <vector>
#include <functional>
#include <utility>

#include "game_stage.h"

class GamePlayer;
class GameStage;
class Match;

//template <class MainStage, class MyStageContainer>
class Game
{
public:
  typedef std::unique_ptr<GameStage> StagePtr;
  const std::string                   kGameId;
  const uint32_t                           kMinPlayer = 2;
  const uint32_t                           kMaxPlayer = 0;	// set 0 if no upper limit
  StageContainer                      stage_container_;
  StagePtr  main_stage_;

  Game(
    Match& match,
    const std::string& game_id,
    const uint32_t& min_player,
    const uint32_t& max_player,
    StagePtr&& main_stage,
    StageCreatorMap&& stage_creators);

  inline bool valid_pnum(uint32_t pnum);

  /* send msg to a specific player */
  void Reply(uint32_t pid, std::string msg) const;

  /* send msg to all player */
  void Broadcast(std::string msg) const;

  /* add new player */
  int32_t Join(std::shared_ptr<GamePlayer> player);

  /* create main_state_ */
  bool StartGame();

  /* a callback function which write results to the database */
  bool RecordResult();

  /* transmit msg to main_state_ */
  void Request(uint32_t pid, const char* msg, int32_t sub_type);

protected:
  
  std::vector<std::shared_ptr<GamePlayer>>             players_;

private:
  Match&                              match_;
};


