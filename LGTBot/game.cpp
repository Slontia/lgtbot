#include "stdafx.h"
#include "game.h"
#include "match.h"
#include "time_trigger.h"

/* Constructor
*/
StageContainer::StageContainer(Game& game, StageCreatorMap&& stage_creators) :
  game_(game), stage_creators_(std::forward<StageCreatorMap>(stage_creators)) {}

/* Returns game Stage pointer with id
* if the id is main Stage id, returns the main game Stage pointer
*/
StagePtr StageContainer::Make(StageId id, GameStage& father_stage) const
{
  auto it = stage_creators_.find(id);
  if (it == stage_creators_.end())
  {
    /* Not found stage id. */
    throw ("Stage id " + std::to_string(id) + " has not been bound.");
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
  stage_container_(*this, std::forward<StageCreatorMap>(stage_creators)),
  main_stage_(std::forward<StagePtr>(main_stage))
{}

inline bool Game::valid_pnum(uint32_t pnum)
{
  return pnum >= kMinPlayer && (kMaxPlayer == 0 || pnum <= kMaxPlayer);
}

/* send msg to a specific player */
void Game::Reply(uint32_t pid, std::string msg) const
{
  match_.Reply(pid, msg);
}

/* send msg to all player */
void Game::Broadcast(std::string msg) const
{
  match_.Broadcast(msg);
}

/* add new player */
int32_t Game::Join(std::shared_ptr<GamePlayer> player)
{
  int32_t pid = players_.size();
  players_.push_back(player);
  return pid;
}

/* create main_state_ */
bool Game::StartGame()
{
  timer.clear_stack();
  timer.push_handle_to_stack(std::bind(&Game::RecordResult, this));
  main_stage_->start_up();
}

/* a callback function which write results to the database */
bool Game::RecordResult()
{
  std::cout << "record" << std::endl;
  return true;  // always returns true
}

/* transmit msg to main_state_ */
void Game::Request(uint32_t pid, const char* msg, int32_t sub_type)
{
  if (main_stage_->Request(pid, msg, sub_type)) // game over
  {
    RecordResult();
  }
}

//std::unique_ptr<sql::Statement> state(sql::mysql::get_mysql_driver_instance()
//                                      ->connect("tcp://127.0.0.1:3306", "root", "")
//                                      ->createStatement());
