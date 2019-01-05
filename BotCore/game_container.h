#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <memory>

class GamePlayer;
class Match;
class Game;
class GameContainer;

extern GameContainer game_container;
class GameContainer
{
private:
  typedef std::unique_ptr<Game> GamePtr;
  typedef std::shared_ptr<GamePlayer> PlayerPtr;
  std::unordered_map<std::string, std::function<GamePtr(Match&)>> game_creator_map_;
  std::unordered_map<std::string, std::function<PlayerPtr()>> player_creator_map_;
public:
  GameContainer() {}
  template <class G, class P> void Bind(std::string game_id)
  {
    LOG_INFO("°ó¶¨ÁËÓÎÏ· " + game_id);
    game_creator_map_[game_id] = [](Match& match) -> GamePtr
    {
      return std::make_unique<G>(match);
    };
    player_creator_map_[game_id] = []() -> PlayerPtr
    {
      return std::make_shared<P>();
    };
  }
  /* return game binded with state container */
  GamePtr MakeGame(const std::string& game_id, Match& match) const;
  PlayerPtr MakePlayer(const std::string& game_id) const;
  std::vector<std::string> gamelist() const
  {
    std::vector<std::string> res;
    for (auto pair : game_creator_map_)
    {
      res.push_back(pair.first);
    }
    return res;
  }

  bool has_game(const std::string& name) const
  {
    return game_creator_map_.find(name) != game_creator_map_.end();
  }
};



 