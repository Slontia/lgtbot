#pragma once
#include "msg_checker.h"
#include <optional>
#include <cassert>
#include "timer.h"

template <typename RetType>
using GameUserFuncType = RetType(const uint64_t, const bool, const std::function<void(const std::string&)>);
template <typename RetType>
using GameMsgCommand = MsgCommand<GameUserFuncType<RetType>>;

template <typename Stage, typename RetType, typename ...Args, typename ...Checkers>
static std::shared_ptr<GameMsgCommand<RetType>> MakeStageCommand(Stage* stage, std::string&& description, RetType (Stage::*cb)(Args...), Checkers&&... checkers)
{
  return MakeCommand<GameUserFuncType<RetType>>(std::move(description), BindThis(stage, cb), std::move(checkers)...);
}

template <typename StageEnum, typename GameEnv> class Game;

template <typename StageEnum, typename GameEnv>
class Stage
{
public:
  Stage(
    Game<StageEnum, GameEnv>& game,
    const StageEnum stage_id,
    std::string&& name)
    : stage_id_(stage_id), name_(std::move(name)), is_over_(false), game_(game)
  {}

  ~Stage() {}

  virtual void HandleTimeout() = 0;
  virtual uint64_t CommandInfo(uint64_t i, std::stringstream& ss) const = 0;
  bool IsOver() const { return is_over_; }
  StageEnum StageID() const { return stage_id_; }

  virtual bool HandleRequest(MsgReader& reader, const uint64_t player_id, const bool is_public, const std::function<void(const std::string&)>& reply) = 0;

protected:
  uint64_t CommandInfo(uint64_t i, std::stringstream& ss, const std::shared_ptr<MsgCommandBase>& cmd) const
  {
    ss << std::endl << "[" << (++i) << "] " << cmd->Info();
    return i;
  }
  virtual void Over() { is_over_ = true; }
  Game<StageEnum, GameEnv>& game_;
  const std::string name_;

private:
  const StageEnum stage_id_;
  bool is_over_;
};

template <typename StageEnum, typename GameEnv>
class CompStage : public Stage<StageEnum, GameEnv>
{
public:
  virtual std::unique_ptr<Stage<StageEnum, GameEnv>> NextSubStage(const StageEnum cur_stage) = 0;
  
  CompStage(
    Game<StageEnum, GameEnv>& game,
    const StageEnum stage_id,
    std::string&& name,
    std::vector<std::shared_ptr<GameMsgCommand<void>>>&& commands,
    std::unique_ptr<Stage<StageEnum, GameEnv>>&& sub_stage)
    : Stage<StageEnum, GameEnv>(game, stage_id, std::move(name)), sub_stage_(std::move(sub_stage)), commands_(std::move(commands)) {}

  ~CompStage() {}

  virtual bool HandleRequest(MsgReader& reader, const uint64_t player_id, const bool is_public, const std::function<void(const std::string&)>& reply) override
  {
    for (const std::shared_ptr<GameMsgCommand<void>>& command : commands_)
    {
      if (command->CallIfValid(reader, std::tuple{ player_id, is_public, reply })) { return true; }
    }
    const bool sub_stage_handled = sub_stage_->HandleRequest(reader, player_id, is_public, reply);
    if (sub_stage_->IsOver()) { CheckoutSubStage(); }
    return sub_stage_handled;
  }

  virtual void HandleTimeout() override
  {
    sub_stage_->HandleTimeout();
    if (sub_stage_->IsOver()) { CheckoutSubStage(); }
  }

  virtual uint64_t CommandInfo(uint64_t i, std::stringstream& ss) const override
  {
    for (const auto& cmd : commands_) { i = Stage<StageEnum, GameEnv>::CommandInfo(i, ss, cmd); }
    return sub_stage_->CommandInfo(i, ss);
  }

  void CheckoutSubStage()
  {
    StageEnum last_stage = sub_stage_->StageID();
    sub_stage_.release(); // ensure previous substage is released before next substage built
    sub_stage_ = NextSubStage(last_stage);
    if (!sub_stage_) { Stage<StageEnum, GameEnv>::Over(); } // no more substages
  }
private:
  using Stage<StageEnum, GameEnv>::Over;
  std::unique_ptr<Stage<StageEnum, GameEnv>> sub_stage_;
  std::vector<std::shared_ptr<GameMsgCommand<void>>> commands_;
};

template <typename StageEnum, typename GameEnv>
class AtomStage : public Stage<StageEnum, GameEnv>
{
public:
  AtomStage(
    Game<StageEnum, GameEnv>& game,
    const StageEnum stage_id,
    std::string&& name,
    std::vector<std::shared_ptr<GameMsgCommand<bool>>>&& commands,
    const uint64_t sec = 0)
    : Stage<StageEnum, GameEnv>(game, stage_id, std::move(name)), timer_(game.Time(sec)), commands_(std::move(commands)) {}
  virtual ~AtomStage() {}
  virtual void HandleTimeout() override final { Stage<StageEnum, GameEnv>::Over(); }
  virtual bool HandleRequest(MsgReader& reader, const uint64_t player_id, const bool is_public, const std::function<void(const std::string&)>& reply) override
  {
    for (const std::shared_ptr<GameMsgCommand<bool>>& command : commands_)
    {
      if (std::optional<bool> over = command->CallIfValid(reader, std::tuple{ player_id, is_public, reply }); over.has_value())
      { 
        if (*over) { Over(); }
        return true;
      }
    }
    return false;
  }
  virtual uint64_t CommandInfo(uint64_t i, std::stringstream& ss) const override
  {
    for (const auto& cmd : commands_) { i = Stage<StageEnum, GameEnv>::CommandInfo(i, ss, cmd); }
    return i;
  }
private:
  virtual void Over() override final
  {
    timer_ = nullptr;
    Stage<StageEnum, GameEnv>::Over();
  }
  std::unique_ptr<Timer, std::function<void(Timer*)>> timer_;
  std::vector<std::shared_ptr<GameMsgCommand<bool>>> commands_;
};
