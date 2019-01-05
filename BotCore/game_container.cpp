#include "stdafx.h"
#include "game.h"
#include "log.h"
#include "game_container.h"


/* Create game without players
*/
std::unique_ptr<Game> GameContainer::MakeGame(const std::string& game_id, Match& match) const
{
  auto it = game_creator_map_.find(game_id);
  if (it == game_creator_map_.end())  // unexpected game_id
  {
    LOG_ERROR("Unexpected game_id " + game_id);
    return nullptr;
  }
  auto game_ptr = (it->second)(match);  // create game
  if (!game_ptr)
  {
    LOG_ERROR("Failed to create game");
  }
  return game_ptr;
}

/* Create a player
*/
std::shared_ptr<GamePlayer> GameContainer::MakePlayer(const std::string& game_id) const
{
  auto it = player_creator_map_.find(game_id);
  if (it == player_creator_map_.end())  // unexpected game_id
  {
    LOG_ERROR("Unexpected game_id " + game_id);
    return nullptr;
  }
  return (it->second)();
}

