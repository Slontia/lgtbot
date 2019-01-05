#include "stdafx.h"
#include "game.h"
#include "match.h"
#include "time_trigger.h"

/* Constructor
*/
StageContainer::StageContainer(Game& game, GameStage& main_stage, StageCreatorMap&& stage_creators) :
  game_(game), main_stage_(main_stage), stage_creators_(std::forward<StageCreatorMap>(stage_creators)) {}

/* Returns game Stage pointer with id
* if the id is main Stage id, returns the main game Stage pointer
*/
StagePtr StageContainer::Make(const StageId& id, GameStage& father_stage) const
{
  auto it = stage_creators_.find(id);
  if (it == stage_creators_.end())
  {
    /* Not found stage id. */
    throw "Stage id " + id + " has not been bound.";
  }
  return it->second(father_stage);
}

Game::Game(
  Match& match,
  const std::string& game_id,
  const uint32_t& min_player,
  const uint32_t& max_player,
  StagePtr&& main_stage,
  StageCreatorMap&& stage_creators) :
  match_(match), kGameId(game_id), kMinPlayer(min_player), kMaxPlayer(max_player),
  stage_container_(*this, *main_stage_, std::forward<StageCreatorMap>(stage_creators)),
  main_stage_(std::forward<StagePtr>(main_stage))
{}

Game::~Game() {}

inline bool Game::valid_pnum(const uint32_t& pnum) const
{
  return pnum >= kMinPlayer && (kMaxPlayer == 0 || pnum <= kMaxPlayer);
}

/* send msg to all player */
void Game::Broadcast(const std::string& msg) const
{
  match_.broadcast(msg);
}

/* add new player */
int32_t Game::Join(std::shared_ptr<GamePlayer> player)
{
  int32_t pid = players_.size();
  players_.push_back(player);
  return pid;
}

/* create main_state_ */
void Game::StartGame()
{
  timer.clear_stack();
  timer.push_handle_to_stack(std::bind(&Game::RecordResult, this));
  main_stage_->start_up();
}

/* a callback function which write results to the database */
bool Game::RecordResult()
{
  // TODO: record result
  match_manager.DeleteMatch(match_.get_id());
  return true;  // always returns true
}

/* transmit msg to main_state_ */
void Game::Request(const uint32_t& pid, MessageIterator& msg)
{
  if (main_stage_->Request(pid, msg)) // game over
  {
    RecordResult();
  }
}

//std::unique_ptr<sql::Statement> state(sql::mysql::get_mysql_driver_instance()
//                                      ->connect("tcp://127.0.0.1:3306", "root", "")
//                                      ->createStatement());
