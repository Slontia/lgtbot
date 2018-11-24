#include "stdafx.h"
#include "game.h"
#include "log.h"
#include "game_container.h"



/* Create game without players
*/
std::unique_ptr<Game> GameContainer::MakeGame(std::string game_id, Match& match)
{
  if (!game_creator_map_[game_id])  // unexpected game_id
  {
    LOG_ERROR("Unexpected game_id " << game_id);
    return nullptr;
  }
  auto game_ptr = game_creator_map_[game_id](match);  // create game
  if (!game_ptr)
  {
    LOG_ERROR("Failed to create game");
  }
  return game_ptr;
}

/* Create a player
*/
std::shared_ptr<GamePlayer> GameContainer::MakePlayer(std::string game_id)
{
  if (!game_creator_map_[game_id])  // unexpected game_id
  {
    LOG_ERROR("Unexpected game_id " << game_id);
    return nullptr;
  }
  return player_creator_map_[game_id]();
}

