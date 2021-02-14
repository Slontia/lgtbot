#pragma once
#include <cassert>
#include <iostream>
#include <vector>
#include <memory>
#include <functional>
#include <mutex>
#include <atomic>
#include "utility/msg_checker.h"
#include "game_base.h"
#include "game_stage.h"
#include "utility/timer.h"
#include "utility/spinlock.h"
#include "game_options.h"

class StageBase;

class Game : public GameBase
{
public:
  Game(void* const match);
  virtual ~Game() {}
  /* Return true when is_over_ switch from false to true */
  virtual bool /*__cdecl*/ StartGame(const bool is_public, const uint64_t player_num) override;
  virtual ErrCode /*__cdecl*/ HandleRequest(const uint64_t pid, const bool is_public, const char* const msg) override;
  virtual void /*__cdecl*/ HandleTimeout(const bool* const stage_is_over) override;
  virtual const char* /*__cdecl*/ OptionInfo() const override;

private:
  void Help_(const replier_t reply);
  template <typename SenderRef> requires std::is_same_v<std::decay_t<SenderRef>, MsgSenderWrapper>
  void HelpInternal_(SenderRef&& sender);
  void OnGameOver_();

  void* const match_;
  uint64_t player_num_;
  std::unique_ptr<MainStageBase> main_stage_;
  bool is_over_;
  std::optional<std::vector<int64_t>> scores_;
  SpinLock lock_;
  const std::shared_ptr<MsgCommand<void(const replier_t)>> help_cmd_;
  GameOption options_;
};
