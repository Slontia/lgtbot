#pragma once
#include <cassert>
#include <iostream>
#include <vector>
#include <memory>
#include <functional>
#include "game_stage.h"
#include "msg_checker.h"
#include "game_base.h"
#include "game.h"

class Player;

Game::Game(void* const match, const uint64_t player_num, std::unique_ptr<Stage>&& main_stage, const std::function<int64_t(uint64_t)>& player_score_f)
  : match_(match), player_num_(player_num), main_stage_(std::move(main_stage)), player_score_f_(player_score_f), is_over_(false), timer_(nullptr), is_busy_(false),
  help_cmd_(MakeCommand<void(const std::function<void(const std::string&)>)>("查看游戏帮助", BindThis(this, &Game::Help), std::make_unique<VoidChecker>("帮助")))
{
  main_stage_->Init(match, std::bind(&Game::Time, this, std::placeholders::_1));
}

/* Return true when is_over_ switch from false to true */
void __cdecl Game::HandleRequest(const uint64_t pid, const bool is_public, const char* const msg)
{
  std::unique_lock<std::mutex> lk(mutex_);
  cv_.wait(lk, [&is_busy = is_busy_]() { return !is_busy.exchange(true); });
  const auto reply = [this, pid, is_public](const std::string& msg) { is_public ? boardcast_f(match_, at_f(match_, pid) + msg) : tell_f(match_, pid, msg); };
  if (is_over_) { reply("[错误] 差一点点，游戏已经结束了哦~"); }
  else
  {
    assert(msg);
    MsgReader reader(msg);
    if (!help_cmd_->CallIfValid(reader, std::tuple{ reply }) && !main_stage_->HandleRequest(reader, pid, is_public, reply)) { reply("[错误] 未预料的游戏请求"); }
    if (main_stage_->IsOver()) { OnGameOver(); }
  }
  is_busy_.store(false);
  lk.unlock();
  cv_.notify_one();
}

std::unique_ptr<Timer, std::function<void(Timer*)>> Game::Time(const uint64_t sec)
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

void Game::Help(const std::function<void(const std::string&)>& reply)
{
  std::stringstream ss;
  ss << "[当前可使用游戏命令]";
  ss << std::endl << "[1] " << help_cmd_->Info();
  main_stage_->CommandInfo(1, ss);
  reply(ss.str());
}

void Game::OnGameOver()
{
  is_over_ = true;
  std::vector<int64_t> scores(player_num_);
  for (uint64_t pid = 0; pid < player_num_; ++pid)
  {
    scores[pid] = player_score_f_(pid);
  }
  game_over_f(match_, scores);
}
