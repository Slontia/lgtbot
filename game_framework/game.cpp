#include <cassert>
#include <iostream>
#include <vector>
#include <memory>
#include <functional>
#include "game_stage.h"
#include "utility/msg_checker.h"
#include "game_base.h"
#include "game.h"

class Player;

Game::Game(void* const match)
  : match_(match), main_stage_(nullptr), is_over_(false),
  help_cmd_(MakeCommand<void(const replier_t)>("查看游戏帮助", BindThis(this, &Game::Help_), VoidChecker("帮助")))
{}

bool Game::StartGame(const bool is_public, const uint64_t player_num)
{
  assert(main_stage_ == nullptr);
  assert(k_max_player == 0 || player_num <= k_max_player);
  const replier_t reply = [this, is_public]() -> MsgSenderWrapper<MsgSenderForGame>
  {
    if (is_public)
    {
      auto sender = ::Boardcast(match_);
      sender << AtMsg(0) << "\n";
      return sender;
    }
    return ::Tell(match_, 0);
  }; // we must convert lambda to std::function to pass it into CallIfValid

  options_.SetPlayerNum(player_num);
  if (main_stage_ = MakeMainStage(reply, options_))
  {
    g_game_prepare_cb(match_);
    main_stage_->Init(match_, std::bind(g_start_timer_cb, match_, std::placeholders::_1), std::bind(g_stop_timer_cb, match_));
    return true;
  }
  return false;
}

/* Return true when is_over_ switch from false to true */
ErrCode /*__cdecl*/ Game::HandleRequest(const uint64_t pid, const bool is_public, const char* const msg)
{
  using namespace std::string_literals;
  std::lock_guard<SpinLock> l(lock_);
  const replier_t reply = [this, pid, is_public]() -> MsgSenderWrapper<MsgSenderForGame>
  {
    if (is_public)
    {
      auto sender = ::Boardcast(match_);
      sender << AtMsg(pid) << "\n";
      return sender;
    }
    return ::Tell(match_, pid);
  }; // we must convert lambda to std::function to pass it into CallIfValid

  ErrCode rc = EC_GAME_REQUEST_OK;
  if (is_over_)
  {
    reply() << "[错误] 差一点点，游戏已经结束了哦~";
    rc = EC_MATCH_ALREADY_OVER;
  }
  else
  {
    assert(msg);
    MsgReader reader(msg);
    if (help_cmd_->CallIfValid(reader, std::tuple{ reply })) { return EC_GAME_REQUEST_OK; }
    if (main_stage_)
    {
      switch (main_stage_->HandleRequest(reader, pid, is_public, reply))
      {
        case StageBase::StageErrCode::OK:
          rc = EC_GAME_REQUEST_OK;
          break;
        case StageBase::StageErrCode::CHECKOUT:
          rc = EC_GAME_REQUEST_CHECKOUT;
          break;
        case StageBase::StageErrCode::FAILED:
          rc = EC_GAME_REQUEST_FAILED;
          break;
        case StageBase::StageErrCode::NOT_FOUND:
        {
          reply() << "[错误] 未预料的游戏指令，您可以通过\"帮助\"（不带#号）查看所有支持的游戏指令\n"
                     "若您想执行元指令，请尝试在请求前加\"#\"，或通过\"#帮助\"查看所有支持的元指令";
          rc = EC_GAME_REQUEST_NOT_FOUND;
          break;
        }
        default:
          reply() << "[错误] 游戏请求执行失败，未知的错误，请联系管理员";
          rc = EC_GAME_REQUEST_UNKNOWN;
          break;
      }
      if (main_stage_->IsOver()) { OnGameOver_(); }
    }
    else if (!options_.SetOption(reader))
    {
      reply() << "[错误] 未预料的游戏设置，您可以通过\"帮助\"（不带#号）查看所有支持的游戏设置\n"
                 "若您想执行元指令，请尝试在请求前加\"#\"，或通过\"#帮助\"查看所有支持的元指令";
      rc = EC_GAME_REQUEST_NOT_FOUND;
    }
    else { reply() << "设置成功！目前配置："s << OptionInfo(); }
  }
  return rc;
}

void Game::HandleTimeout(const bool* const stage_is_over)
{
  std::lock_guard<SpinLock> l(lock_);
  if (!*stage_is_over)
  {
    main_stage_->HandleTimeout();
    if (main_stage_->IsOver()) { OnGameOver_(); }
  }
}

void Game::Help_(const replier_t reply)
{
  auto sender = reply();
  if (main_stage_)
  {
    sender << "\n[当前阶段]\n";
    main_stage_->StageInfo(sender);
    sender << "\n\n";
  }
  sender << "[当前可使用游戏命令]";
  sender << "\n[1] " << help_cmd_->Info();
  if (main_stage_)
  {
    main_stage_->CommandInfo(1, sender);
  }
  else
  {
    uint32_t i = 1;
    for (const std::string& option_info : options_.Infos()) { sender << "\n[" << (++i) << "] " << option_info; }
  }
}

const char* Game::OptionInfo() const
{
  thread_local static std::string s;
  s = options_.StatusInfo();
  return s.c_str();
}

void Game::OnGameOver_()
{
  is_over_ = true;
  std::vector<int64_t> scores(options_.PlayerNum());
  for (uint64_t pid = 0; pid < options_.PlayerNum(); ++pid)
  {
    scores[pid] = main_stage_->PlayerScore(pid);
  }
  g_game_over_cb(match_, scores.data());
}
