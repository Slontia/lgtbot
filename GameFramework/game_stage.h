#pragma once
#include "msg_checker.h"
#include <optional>
#include <cassert>
#include "timer.h"

using GameUserFuncType = std::string(const uint64_t, const bool);
using GameMsgCommand = MsgCommand<GameUserFuncType>;

template <typename T, typename Ret, typename ...Args>
static std::function<Ret(Args...)> BindThis(T* p, Ret (T::*func)(Args...))
{
  return [p, func](Args... args) { return (p->*func)(args...); };
}

template <typename Stage, typename ...Args, typename ...Checkers>
static std::shared_ptr<GameMsgCommand> MakeStageCommand(Stage* stage, std::string (Stage::*cb)(Args...), Checkers&&... checkers)
{
  return MakeCommand<GameUserFuncType>(BindThis(stage, cb), std::move(checkers)...);
}

template <typename StageEnum, typename GameEnv> class Game;

template <typename StageEnum, typename GameEnv>
class Stage
{
public:
  Stage(const StageEnum stage_id, std::vector<std::shared_ptr<GameMsgCommand>>&& commands, Game<StageEnum, GameEnv>& game)
    : stage_id_(stage_id), commands_(std::move(commands)), is_over_(false), game_(game) {}
  virtual void HandleTimeout() = 0;
  bool IsOver() const { return is_over_; }
  StageEnum StageID() const { return stage_id_; }

  ~Stage() {}

  virtual bool HandleRequest(MsgReader& reader, const uint64_t player_id, const bool is_public, const std::function<void(const std::string&)>& reply)
  {
    for (const std::shared_ptr<GameMsgCommand>& command : commands_)
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
  virtual void Over() { is_over_ = true; }
  Game<StageEnum, GameEnv>& game_;

private:
  const StageEnum stage_id_;
  std::vector<std::shared_ptr<GameMsgCommand>> commands_;
  bool is_over_;
};

template <typename StageEnum, typename GameEnv>
class CompStage : public Stage<StageEnum, GameEnv>
{
public:
  virtual std::unique_ptr<Stage<StageEnum, GameEnv>> NextSubStage(const StageEnum cur_stage) = 0;
  
  CompStage(const StageEnum stage_id, std::vector<std::shared_ptr<GameMsgCommand>>&& commands, Game<StageEnum, GameEnv>& game, std::unique_ptr<Stage<StageEnum, GameEnv>>&& sub_stage)
    : Stage<StageEnum, GameEnv>(stage_id, std::move(commands), game), sub_stage_(std::move(sub_stage)) {}

  ~CompStage() {}

  virtual bool HandleRequest(MsgReader& reader, const uint64_t player_id, const bool is_public, const std::function<void(const std::string&)>& reply) override
  {
    if (Stage<StageEnum, GameEnv>::HandleRequest(reader, player_id, is_public, reply)) { return true; }
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
  AtomStage(const StageEnum stage_id, std::vector<std::shared_ptr<GameMsgCommand>>&& commands, Game<StageEnum, GameEnv>& game, const uint64_t sec = 0)
    : Stage<StageEnum, GameEnv>(stage_id, std::move(commands), game), timer_(game.Time(sec)) {}
  virtual ~AtomStage() {}
  virtual void HandleTimeout() override final { Stage<StageEnum, GameEnv>::Over(); }
  virtual void Over() override final
  {
    timer_ = nullptr;
    Stage<StageEnum, GameEnv>::Over();
  }
private:
  std::unique_ptr<Timer, std::function<void(Timer*)>> timer_;
};
