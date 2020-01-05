#pragma once
#include <cassert>
#include <iostream>
#include <vector>
#include <memory>
#include <functional>
#include "game_stage.h"
#include "../util/msg_checker.h"
//#include "mygame.h"
#include "game_base.h"

class Player;
struct GameEnv;

static std::function<void(const uint64_t, const std::string&)> boardcast_f;
static std::function<void(const uint64_t, const uint64_t, const std::string&)> tell_f;
static std::function<std::string(const uint64_t, const uint64_t)> at_f;
static std::function<void(const uint64_t game_id, const std::vector<int64_t>& scores)> game_over_f;

template <typename StageEnum, typename GameEnv>
class Game : public GameBase
{
public:
  Game(const uint64_t mid, std::unique_ptr<GameEnv>&& game_env, std::unique_ptr<Stage<StageEnum, GameEnv>>&& main_stage)
    : mid_(mid), game_env_(std::move(game_env)), main_stage_(std::move(main_stage)), is_over_(false) {}
  virtual ~Game() {}

  /* Return true when is_over_ switch from false to true */
  virtual void __cdecl HandleRequest(const uint64_t pid, const bool is_public, const char* const msg) override
  {
    const auto reply = [this, pid, is_public](const std::string& msg) { is_public ? Boardcast(At(pid) + msg) : Tell(pid, msg); };
    if (is_over_)
    {
      reply("[错误] 差一点点，游戏已经结束了哦~");
      return;
    }
    assert(msg);
    MsgReader reader(msg);
    if (!main_stage_->HandleRequest(reader, pid, is_public, reply)) { reply("[错误] 未预料的游戏请求"); }
    if (main_stage_->IsOver())
    {
      is_over_ = true;
      game_over_f(mid_, game_env_->PlayerScores());
    }
  }

  void Boardcast(const std::string& msg) { boardcast_f(mid_, msg); }
  void Tell(const uint64_t pid, const std::string& msg) { tell_f(mid_, pid, msg); }
  std::string At(const uint64_t pid) { return at_f(mid_, pid); }

private:
  const uint64_t mid_;
  const std::unique_ptr<GameEnv> game_env_;
  const std::unique_ptr<Stage<StageEnum, GameEnv>> main_stage_;
  bool is_over_;
  std::optional<std::vector<int64_t>> scores_;
};

