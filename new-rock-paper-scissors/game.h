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

class Game
{
public:
  typedef std::function<void(const uint64_t player_id, const std::string& msg)> PrivateMsg;
  typedef std::function<void(const std::string& msg)> Boardcast;
  typedef std::function<void(const std::vector<int64_t>&)> GameOver;
  typedef std::function<std::string(const uint64_t player_id)> At;
  Game(PrivateMsg&& private_msg,
       Boardcast&& boardcast,
       GameOver&& game_over,
       At&& at,
       std::unique_ptr<GameEnv>&& game_env,
       std::unique_ptr<Stage>&& main_stage)
    : private_msg_f_(std::move(private_msg)),
    boardcast_f_(std::move(boardcast)),
    game_over_f_(std::move(game_over)),
    at_f_(std::move(at)),
    game_env_(std::move(game_env)),
    main_stage_(std::move(main_stage)),
    is_over_(false) {}
  ~Game() {}

  std::string HandleRequest(const uint64_t player_id, const std::string& msg)
  {
    assert(!is_over_);
    MsgReader reader(msg);
    if (!main_stage_->HandleRequest(reader)) { return "[错误] 未预料的请求格式"; }
    if (main_stage_->IsOver())
    {
      game_over_f_(PlayerScores());
      is_over_ = true;
    }
    return {};
  }

  std::vector<int64_t> PlayerScores() const;
  bool IsOver() const { return is_over_; }

private:
  const PrivateMsg private_msg_f_;
  const Boardcast boardcast_f_;
  const GameOver game_over_f_;
  const At at_f_;
  const std::unique_ptr<GameEnv> game_env_;
  const std::unique_ptr<Stage> main_stage_;
  bool is_over_;
};

