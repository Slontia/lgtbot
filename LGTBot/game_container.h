#pragma once

#include <string>
#include <map>
#include <functional>
#include <memory>
#include "game.h"
#include "game_state.h"

class GamePlayer;
class Match;

class GameContainer
{
private:
  typedef std::shared_ptr<Game> GamePtr;
  typedef std::shared_ptr<GamePlayer> PlayerPtr;
  typedef std::shared_ptr<StateContainer> StateConPtr;
  std::map<std::string, std::function<GamePtr(Match&)>> game_creator_map_;
  std::map<std::string, std::function<PlayerPtr()>> player_creator_map_;
  std::map<std::string, std::function<StateConPtr(Game&)>> state_con_creator_map_;
public:
  template <class G, class P, class SC> void Bind(std::string game_id);
  /* return game binded with state container */
  GamePtr get_game(std::string game_id, Match& match, uint32_t p_num);
};



 