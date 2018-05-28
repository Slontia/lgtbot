#include "stdafx.h"

template <class G, class P, class SC> void GameContainer::Bind(std::string game_id)
{
  game_creator_map_[game_id] = [](Match& match) -> GamePtr
  {
    return new G();
  };
  player_creator_map_[game_id] = []() -> PlayerPtr
  {
    return new P();
  };
  state_con_creator_map_[game_id] = [](Game& game) ->StateConPtr
  {
    return new SC(game);
  }
}

std::shared_ptr<Game> GameContainer::get_game(std::string game_id, Match& match, uint32_t pnum)
{
  std::shared_ptr<Game> game_ptr = game_creator_map_[game_id](match);  // create game
  if (game_ptr != nullptr)
  {
    game_ptr->set_state_container(state_con_creator_map_[game_id](*game_ptr));  // set state container
    if (game_ptr->valid_pnum(pnum))
    {
      for (uint32_t i = 0; i < pnum; i++)
      {
        game_ptr->Join(player_creator_map_[game_id]());  // add players
      }
    }
    else
    {
      return nullptr;
    }
  }
  return game_ptr;
}