#pragma once

#include <string>
#include <map>
#include <functional>
#include <memory>
#include "game.h"
#include "game_state.h"

class GamePlayer;

class GameContainer
{
private:
  typedef std::shared_ptr<Game> GamePtr;
  typedef std::shared_ptr<GamePlayer> PlayerPtr;
  typedef std::shared_ptr<StateContainer> StateConPtr;
  std::map<std::string, std::function<GamePtr()>> game_creator_map_;
  std::map<std::string, std::function<PlayerPtr()>> player_creator_map_;
  std::map<std::string, std::function<StateContainer>> state_con_creator_map_;
public:
  template <class G, class P, class SC> void Bind(std::string game_id)
  {
    game_creator_map_[game_id] = [](uint32_t p_num) -> GamePtr
    {
      return new G(p_num);
    };
    player_creator_map_[game_id] = [](int32_t p_id) -> PlayerPtr
    {
      return new P(p_id);
    };
    state_con_creator_map_[game_id] = [](Game& game) ->StateConPtr
    {
      return new SC(game);
    }
  }
  /* return game binded with state container */
  GamePtr get_game(std::string game_id, uint32_t p_num);
  /* return player */
  PlayerPtr get_player(std::string game_id, int32_t p_id);
};

 