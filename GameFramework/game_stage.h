#pragma once
#include "msg_checker.h"
#include <optional>
#include <cassert>
#include <variant>
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

template <typename GameEnv> class Game;

template <typename GameEnv>
class Stage
{
public:
  Stage(
    Game<GameEnv>& game, std::string&& name) : name_(std::move(name)), is_over_(false), game_(game)
  {}

  ~Stage() {}

  virtual void HandleTimeout() = 0;
  virtual uint64_t CommandInfo(uint64_t i, std::stringstream& ss) const = 0;
  bool IsOver() const { return is_over_; }

  virtual bool HandleRequest(MsgReader& reader, const uint64_t player_id, const bool is_public, const std::function<void(const std::string&)>& reply) = 0;

protected:
  uint64_t CommandInfo(uint64_t i, std::stringstream& ss, const std::shared_ptr<MsgCommandBase>& cmd) const
  {
    ss << std::endl << "[" << (++i) << "] " << cmd->Info();
    return i;
  }
  virtual void Over() { is_over_ = true; }
  Game<GameEnv>& game_;
  const std::string name_;

private:
  bool is_over_;
};

template <typename SubStage, typename RetType>
class SubStageCheckoutHelper
{
 public:
  virtual RetType NextSubStage(SubStage&) = 0;
};

template < typename GameEnv, typename ...SubStages>
class CompStage : public Stage<GameEnv>,
  public SubStageCheckoutHelper<SubStages, std::variant<std::unique_ptr<SubStages>...>>...
{
public:
  using VariantSubStage = std::variant<std::unique_ptr<SubStages>...>;
  using SubStageCheckoutHelper<SubStages, std::variant<std::unique_ptr<SubStages>...>>::NextSubStage...;

  CompStage(
    Game<GameEnv>& game,
    std::string&& name,
    std::vector<std::shared_ptr<GameMsgCommand<void>>>&& commands,
    VariantSubStage&& sub_stage)
    : Stage<GameEnv>(game, std::move(name)), sub_stage_(std::move(sub_stage)), commands_(std::move(commands)) {}

  ~CompStage() {}

  virtual bool HandleRequest(MsgReader& reader, const uint64_t player_id, const bool is_public, const std::function<void(const std::string&)>& reply) override
  {
    for (const std::shared_ptr<GameMsgCommand<void>>& command : commands_)
    {
      if (command->CallIfValid(reader, std::tuple{ player_id, is_public, reply })) { return true; }
    }
    return std::visit([this, &reader, player_id, is_public, &reply](auto&& sub_stage)
    {
      const bool sub_stage_handled = sub_stage->HandleRequest(reader, player_id, is_public, reply);
      if (sub_stage->IsOver()) { this->CheckoutSubStage(); }
      return sub_stage_handled;
    }, sub_stage_);
  }

  virtual void HandleTimeout() override
  {
    std::visit([this](auto&& sub_stage)
    {
      sub_stage->HandleTimeout();
      if (sub_stage->IsOver()) { this->CheckoutSubStage(); }
    }, sub_stage_);
  }

  virtual uint64_t CommandInfo(uint64_t i, std::stringstream& ss) const override
  {
    for (const auto& cmd : commands_) { i = Stage<GameEnv>::CommandInfo(i, ss, cmd); }
    return std::visit([&i, &ss](auto&& sub_stage) { return sub_stage->CommandInfo(i, ss); }, sub_stage_);
  }

  void CheckoutSubStage()
  {
    // ensure previous substage is released before next substage built
    sub_stage_ = std::visit([this](auto&& sub_stage)
    {   
      VariantSubStage new_sub_stage = NextSubStage(*sub_stage);
      sub_stage.release();
      return std::move(new_sub_stage);
    }, sub_stage_);
    if (std::visit([](auto&& sub_stage) { return sub_stage == nullptr; }, sub_stage_)) { Stage<GameEnv>::Over(); } // no more substages
  }
private:
  using Stage<GameEnv>::Over;
  VariantSubStage sub_stage_;
  std::vector<std::shared_ptr<GameMsgCommand<void>>> commands_;
};

template <typename GameEnv>
class AtomStage : public Stage<GameEnv>
{
public:
  AtomStage(
    Game<GameEnv>& game,
    std::string&& name,
    std::vector<std::shared_ptr<GameMsgCommand<bool>>>&& commands,
    const uint64_t sec = 0)
    : Stage<GameEnv>(game, std::move(name)), timer_(game.Time(sec)), commands_(std::move(commands)) {}
  virtual ~AtomStage() {}
  virtual void HandleTimeout() override final { Stage<GameEnv>::Over(); }
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
    for (const auto& cmd : commands_) { i = Stage<GameEnv>::CommandInfo(i, ss, cmd); }
    return i;
  }
private:
  virtual void Over() override final
  {
    timer_ = nullptr;
    Stage<GameEnv>::Over();
  }
  std::unique_ptr<Timer, std::function<void(Timer*)>> timer_;
  std::vector<std::shared_ptr<GameMsgCommand<bool>>> commands_;
};
