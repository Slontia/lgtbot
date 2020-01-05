#pragma once
#include "../util/msg_checker.h"
#include <optional>
#include <cassert>

using GameMsgCommand = MsgCommand<std::string(const uint64_t, const bool)>;
template <typename StageEnum, typename GameEnv> class Game;

template <typename StageEnum, typename GameEnv>
class Stage
{
public:
  Stage(const StageEnum stage_id, std::vector<std::unique_ptr<GameMsgCommand>>&& commands, Game<StageEnum, GameEnv>& game)
    : stage_id_(stage_id), commands_(std::move(commands)), is_over_(false), game_(game) {}
  virtual void HandleTimeout() = 0;
  bool IsOver() const { return is_over_; }
  StageEnum StageID() const { return stage_id_; }

  ~Stage() {}

  virtual bool HandleRequest(MsgReader& reader, const uint64_t player_id, const bool is_public, const std::function<void(const std::string&)>& reply) = 0
  {
    for (const std::unique_ptr<GameMsgCommand>& command : commands_)
    {
      if (std::optional<std::string> response = command->CallIfValid(reader, std::tuple{ player_id, is_public }))
      {
        if (!response->empty()) { reply(*response); }
        return true;
      }
    }
    return false;
  }

protected:
  virtual std::string Name() const = 0;
  virtual void OnOver() = 0;
  void Over()
  {
    if (!is_over_)
    {
      OnOver();
      is_over_ = true;
    }
  }
private:
  const StageEnum stage_id_;
  std::vector<std::unique_ptr<GameMsgCommand>> commands_;
  bool is_over_;
  Game<StageEnum, GameEnv>& game_;
};

template <typename StageEnum, typename GameEnv>
class CompStage : public Stage<StageEnum, GameEnv>
{
public:
  virtual std::unique_ptr<Stage<StageEnum, GameEnv>> NextSubStage(const StageEnum cur_stage) const = 0;
  
  CompStage(const StageEnum stage_id, std::vector<std::unique_ptr<GameMsgCommand>>&& commands, Game<StageEnum, GameEnv>& game, std::unique_ptr<Stage<StageEnum, GameEnv>>&& sub_stage)
    : Stage(stage_id, std::move(commands), game), sub_stage_(std::move(sub_stage)) {}

  ~CompStage() {}

  virtual bool HandleRequest(MsgReader& reader, const uint64_t player_id, const bool is_public, const std::function<void(const std::string&)>& reply) override
  {
    if (Stage::HandleRequest(reader, player_id, is_public, reply)) { return true; }
    const bool sub_stage_handled = sub_stage_->HandleRequest(reader, player_id, is_public, reply);
    if (sub_stage_->IsOver()) { CheckoutSubStage(); }
    return sub_stage_handled;
  }

  virtual void HandleTimeout() override
  {
    sub_stage_->HandleTimeout();
    if (sub_stage_->IsOver()) { CheckoutSubStage(); }
  }

  void CheckoutSubStage()
  {
    sub_stage_ = NextSubStage(sub_stage_->StageID());
    if (!sub_stage_) { Stage<StageEnum, GameEnv>::Over(); } // no more substages
  }
private:
  std::unique_ptr<Stage<StageEnum, GameEnv>> sub_stage_;
};

template <typename StageEnum, typename GameEnv>
class AtomStage : public Stage<StageEnum, GameEnv>
{
public:
  using Stage<StageEnum, GameEnv>::Stage;
  virtual ~AtomStage() {}
  virtual uint32_t TimerSec() const = 0;
  using Stage<StageEnum, GameEnv>::HandleRequest;
  virtual void HandleTimeout() override final { Over(); }
  using Stage<StageEnum, GameEnv>::Over; // to public
};
