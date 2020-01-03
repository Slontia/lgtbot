#pragma once
#include <cassert>
#include <iostream>
#include <vector>
#include <memory>
#include <functional>
#include "game_stage.h"
#include "msg_checker.h"
#include "mygame.h"
#include "game_base.h"

class Player;
class GameEnv;

static std::function<void(const uint64_t, const std::string&)> boardcast_f;
static std::function<void(const uint64_t, const uint64_t, const std::string&)> tell_f;
static std::function<std::string(const uint64_t, const uint64_t)> at_f;
static std::function<void(const uint64_t game_id, const std::vector<int64_t>& scores)> game_over_f;

class Game : public GameBase
{
public:
  Game(const uint64_t match_id, std::unique_ptr<GameEnv>&& game_env, std::unique_ptr<Stage>&& main_stage)
    : match_id_(match_id), game_env_(std::move(game_env)), main_stage_(std::move(main_stage)), is_over_(false) {}
  virtual ~Game() {}

  const char* __cdecl HandleRequest(const uint64_t player_id, const bool is_public, const char* const msg) override
  {
    assert(!is_over_);
    assert(msg);
    MsgReader reader(msg);
    std::string& reply_msg = tmp_str_;
    const auto reply = [&reply_msg](const std::string& msg) { reply_msg = msg; };
    if (!main_stage_->HandleRequest(reader, player_id, is_public, reply)) { reply("[错误] 未预料的游戏请求"); }
    if (main_stage_->IsOver()) { is_over_ = true; }
    return reply_msg.c_str();
  }

  std::vector<int64_t> PlayerScores() const;
  bool IsOver() const { return is_over_; }

private:
  const uint64_t match_id_;
  const std::unique_ptr<GameEnv> game_env_;
  const std::unique_ptr<Stage> main_stage_;
  bool is_over_;
  std::string tmp_str_;
};

