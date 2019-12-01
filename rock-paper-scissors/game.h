#pragma once

#include <iostream>
#include <vector>
#include <functional>
#include <utility>

#include "timer.h"
#include "game_stage.h"

class GamePlayer;
class StageContainer;
class Game;

/*
将stage改为phase
去掉StageContainer，实现StagePtr Make<MyStage>(GameStage& father_stage)

  
*/


typedef std::unique_ptr<GameStage> StagePtr;
typedef std::function<StagePtr(const std::string&, GameStage&)> StageCreator;
typedef std::map<StageId, StageCreator> StageCreatorMap;

class StageContainer
{
private:
  Game& game_;
  const std::map<StageId, StageCreator>   stage_creators_;
public:
  /* Constructor
  */
  StageContainer(Game& game, StageCreatorMap&& stage_creators);

  /* Returns game Stage pointer with id
  * if the id is main Stage id, returns the main game Stage pointer
  */
  StagePtr Make(const StageId& id, GameStage& father_stage) const;

  template <class Stage>
  StageCreator get_creator()
  {
    return [this](const std::string& name, GameStage& father_stage) -> StagePtr
    {
      /* Cast GameStage& to FatherStage&. */
      return std::make_unique<Stage>(game_, name, father_stage);
    };
  }
};

//template <class MainStage, class MyStageContainer>

class Game
{
public:
  typedef std::unique_ptr<GameStage> StagePtr;
  static const std::string kName;
  static const uint32_t kMinPlayer;
  static const uint32_t kMaxPlayer;

  Game(void* match, uint64_t players_num) : match_(match), players_(players_num)
  {
    timer_.clear_stack();
    timer_.push_handle_to_stack(std::bind(&Game::GameOver_, this));
    main_stage_->start_up();
  }

  virtual ~Game() {}

  /* send msg to all player */
  void Broadcast(const std::string& msg) const
  {
    public_response_cb(match_, msg.c_str());
  }

  /* a callback function which write results to the database */
  bool GameOver_() { return true; }

  /* transmit msg to main_state_ */
  void Game::Request(const uint32_t& pid, MessageIterator& msg)
  {
    if (main_stage_->Request(pid, msg)) // game over
    {
      main_stage_->end_up();
      GameOver_();
    }
  }

  std::vector<std::shared_ptr<GamePlayer>>             players_;

private:
  void* const                              match_;
  StagePtr main_stage_;
  TimeTrigger timer_;

};


