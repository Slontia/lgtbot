#pragma once
#include <cassert>
#include <iostream>
#include <vector>
#include <memory>
#include <functional>
#include "game_stage.h"
#include "msg_checker.h"
#include "mygame.h"

class Player;
class GameEnv;

static std::function<void(const uint64_t, const std::string&)> boardcast_f;
static std::function<void(const uint64_t, const uint64_t, const std::string&)> tell_f;
static std::function<std::string(const uint64_t, const uint64_t)> at_f;
static std::function<void(const uint64_t game_id, const std::vector<int64_t>& scores)> game_over_f;

class Game
{
public:
  Game(const uint64_t game_id, std::unique_ptr<GameEnv>&& game_env, std::unique_ptr<Stage>&& main_stage)
    : game_id_(game_id), game_env_(std::move(game_env)), main_stage_(std::move(main_stage)), is_over_(false) {}
  ~Game() {}

  bool HandleRequest(const uint64_t player_id, const bool is_public, const std::string& msg, const std::function<void(const std::string&)>& reply_f)
  {
    assert(!is_over_);
    MsgReader reader(msg);
    bool reply_msg = main_stage_->HandleRequest(reader, reply_f, is_public);
    if (main_stage_->IsOver())
    {
      game_over_f(game_id_, PlayerScores());
      is_over_ = true;
    }
    return {};
  }

  std::vector<int64_t> PlayerScores() const;
  bool IsOver() const { return is_over_; }

private:
  const uint64_t game_id_;
  const std::unique_ptr<GameEnv> game_env_;
  const std::unique_ptr<Stage> main_stage_;
  bool is_over_;
};

