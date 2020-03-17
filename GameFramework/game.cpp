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
  : match_(match), player_num_(player_num), main_stage_(std::move(main_stage)), player_score_f_(player_score_f), is_over_(false),
  help_cmd_(MakeCommand<void(const std::function<void(const std::string&)>)>("查看游戏帮助", BindThis(this, &Game::Help), std::make_unique<VoidChecker>("帮助")))
{
  main_stage_->Init(match, std::bind(start_timer_f, match_, std::placeholders::_1), std::bind(stop_timer_f, match_));
}

/* Return true when is_over_ switch from false to true */
void __cdecl Game::HandleRequest(const uint64_t pid, const bool is_public, const char* const msg)
{
  std::lock_guard<SpinLock> l(lock_);
  const auto reply = [this, pid, is_public](const std::string& msg) { is_public ? boardcast_f(match_, at_f(match_, pid) + msg) : tell_f(match_, pid, msg); };
  if (is_over_) { reply("[错误] 差一点点，游戏已经结束了哦~"); }
  else
  {
    assert(msg);
    MsgReader reader(msg);
    if (!help_cmd_->CallIfValid(reader, std::tuple{ reply }) && !main_stage_->HandleRequest(reader, pid, is_public, reply)) { reply("[错误] 未预料的游戏请求"); }
    if (main_stage_->IsOver()) { OnGameOver(); }
  }
}

void Game::HandleTimeout(const bool* const stage_is_over)
{
  std::lock_guard<SpinLock> l(lock_);
  if (!*stage_is_over) { main_stage_->HandleTimeout(); }
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
