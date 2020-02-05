#pragma once
#include <cassert>
#include <iostream>
#include <vector>
#include <memory>
#include <functional>
#include "game_stage.h"
#include "msg_checker.h"
#include "game_base.h"

class Player;
struct GameEnv;

static std::function<void(const uint64_t, const std::string&)> boardcast_f;
static std::function<void(const uint64_t, const uint64_t, const std::string&)> tell_f;
static std::function<std::string(const uint64_t, const uint64_t)> at_f;
static std::function<void(const uint64_t, const std::vector<int64_t>&)> game_over_f;

template <typename StageEnum, typename GameEnv>
class Game : public GameBase
{
public:
  Game(const uint64_t mid, std::unique_ptr<GameEnv>&& game_env)
    : mid_(mid), game_env_(std::move(game_env)), is_over_(false), timer_(nullptr), is_busy_(false) {}
  virtual ~Game() {}

  /* Return true when is_over_ switch from false to true */
  virtual void __cdecl HandleRequest(const uint64_t pid, const bool is_public, const char* const msg) override
  {
    std::unique_lock<std::mutex> lk(mutex_);
    cv_.wait(lk, [&is_busy = is_busy_]() { return !is_busy.exchange(true); });
    const auto reply = [this, pid, is_public](const std::string& msg) { is_public ? Boardcast(At(pid) + msg) : Tell(pid, msg); };
    if (is_over_)
    {
      assert(msg);
      MsgReader reader(msg);
      if (!main_stage_->HandleRequest(reader, pid, is_public, reply)) { reply("[错误] 未预料的游戏请求"); }
      if (main_stage_->IsOver())
      {
        is_over_ = true;
        game_over_f(mid_, game_env_->PlayerScores());
      }
    }
    else { reply("[错误] 差一点点，游戏已经结束了哦~"); }
    is_busy_.store(false);
    lk.unlock();
    cv_.notify_one();
  }

  std::unique_ptr<Timer, std::function<void(Timer*)>> Time(const uint64_t sec)
  {
    if (sec == 0) { return nullptr; }
    std::shared_ptr<std::atomic<bool>> state_over = std::make_shared<std::atomic<bool>>(false);
    const auto deleter = [state_over, &mutex = mutex_, &cv = cv_](Timer* timer)
    {
      {
        std::lock_guard<std::mutex> lk(mutex);
        state_over->store(true);
      }
      cv.notify_one();
      delete timer;
    };
    return std::unique_ptr<Timer, std::function<void(Timer*)>>(new Timer(sec, [this, state_over]()
    {
      std::unique_lock<std::mutex> lk(mutex_);
      cv_.wait(lk, [&state_over, &is_busy = is_busy_]() { return state_over->load() || !is_busy.exchange(true); });
      if (!state_over->load())
      {
        main_stage_->HandleTimeout();
        is_busy_.store(false);
      }
      lk.unlock();
      cv_.notify_one();
    }), std::move(deleter));
  }

  void SetMainStage(std::unique_ptr<Stage<StageEnum, GameEnv>>&& main_stage) { main_stage_ = std::move(main_stage); }
  void Boardcast(const std::string& msg) { boardcast_f(mid_, msg); }
  void Tell(const uint64_t pid, const std::string& msg) { tell_f(mid_, pid, msg); }
  std::string At(const uint64_t pid) { return at_f(mid_, pid); }

private:
  const uint64_t mid_;
  const std::unique_ptr<GameEnv> game_env_;
  std::unique_ptr<Stage<StageEnum, GameEnv>> main_stage_;
  bool is_over_;
  std::optional<std::vector<int64_t>> scores_;
  std::unique_ptr<Timer> timer_;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::atomic<bool> is_busy_;
};

