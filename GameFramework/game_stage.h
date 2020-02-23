#pragma once
#include "msg_checker.h"
#include <optional>
#include <cassert>
#include <variant>
#include "timer.h"
#include "game_main.h"

template <typename RetType>
using GameUserFuncType = RetType(const uint64_t, const bool, const std::function<void(const std::string&)>);
template <typename RetType>
using GameMsgCommand = MsgCommand<GameUserFuncType<RetType>>;

template <typename Stage, typename RetType, typename ...Args, typename ...Checkers>
static std::shared_ptr<GameMsgCommand<RetType>> MakeStageCommand(Stage* stage, std::string&& description, RetType (Stage::*cb)(Args...), Checkers&&... checkers)
{
  return MakeCommand<GameUserFuncType<RetType>>(std::move(description), BindThis(stage, cb), std::move(checkers)...);
}

class Stage
{
public:
  Stage(std::string&& name) : name_(std::move(name)), is_over_(false) {}
  ~Stage() {}
  virtual void Init(void* const match, const std::function<std::unique_ptr<Timer, std::function<void(Timer*)>>(const uint64_t)>& time_f)
  {
    match_ = match;
    time_f_ = time_f;
  }
  virtual void HandleTimeout() = 0;
  virtual uint64_t CommandInfo(uint64_t i, std::stringstream& ss) const = 0;
  bool IsOver() const { return is_over_; }
  virtual bool HandleRequest(MsgReader& reader, const uint64_t player_id, const bool is_public, const std::function<void(const std::string&)>& reply) = 0;
  void Boardcast(const std::string& msg) { boardcast_f(match_, msg); }
  void Tell(const uint64_t pid, const std::string& msg) { tell_f(match_, pid, msg); }
  std::string At(const uint64_t pid) { return at_f(match_, pid); }

protected:
  uint64_t CommandInfo(uint64_t i, std::stringstream& ss, const std::shared_ptr<MsgCommandBase>& cmd) const
  {
    ss << std::endl << "[" << (++i) << "] " << cmd->Info();
    return i;
  }
  virtual void Over() { is_over_ = true; }
  const std::string name_;
  void* match_;
  std::function<std::unique_ptr<Timer, std::function<void(Timer*)>>(const uint64_t)> time_f_;

private:
  bool is_over_;
};

template <typename SubStage, typename RetType>
class SubStageCheckoutHelper
{
 public:
  virtual RetType NextSubStage(SubStage&) = 0;
};

template <typename ...SubStages>
class CompStage : public Stage,
  public SubStageCheckoutHelper<SubStages, std::variant<std::unique_ptr<SubStages>...>>...
{
public:
  using VariantSubStage = std::variant<std::unique_ptr<SubStages>...>;
  using SubStageCheckoutHelper<SubStages, std::variant<std::unique_ptr<SubStages>...>>::NextSubStage...;

  CompStage(
    std::string&& name,
    std::vector<std::shared_ptr<GameMsgCommand<void>>>&& commands)
    : Stage(std::move(name)), sub_stage_(), commands_(std::move(commands)) {}

  ~CompStage() {}

  virtual VariantSubStage OnStageBegin() = 0;

  virtual void Init(void* const match, const std::function<std::unique_ptr<Timer, std::function<void(Timer*)>>(const uint64_t)>& time_f)
  {
    Stage::Init(match, time_f);
    sub_stage_ = OnStageBegin();
    std::visit([match, time_f](auto&& sub_stage) { sub_stage->Init(match, time_f); }, sub_stage_);
  }

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
    for (const auto& cmd : commands_) { i = Stage::CommandInfo(i, ss, cmd); }
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
    std::visit([this](auto&& sub_stage)
    {
      if (!sub_stage) { Over(); } // no more substages
      else { sub_stage->Init(match_, time_f_); }
    }, sub_stage_);
  }
private:
  using Stage::Over;
  VariantSubStage sub_stage_;
  std::vector<std::shared_ptr<GameMsgCommand<void>>> commands_;
};

class AtomStage : public Stage
{
public:
  AtomStage(
    std::string&& name,
    std::vector<std::shared_ptr<GameMsgCommand<bool>>>&& commands)
    : Stage(std::move(name)), timer_(nullptr), commands_(std::move(commands)) {}
  virtual ~AtomStage() {}
  virtual uint64_t OnStageBegin() { return 0; };
  virtual void Init(void* const match, const std::function<std::unique_ptr<Timer, std::function<void(Timer*)>>(const uint64_t)>& time_f)
  {
    Stage::Init(match, time_f);
    const uint64_t sec = OnStageBegin();
    timer_ = time_f(sec);
  }
  virtual void HandleTimeout() override final { Stage::Over(); }
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
    for (const auto& cmd : commands_) { i = Stage::CommandInfo(i, ss, cmd); }
    return i;
  }
private:
  virtual void Over() override final
  {
    timer_ = nullptr;
    Stage::Over();
  }
  std::unique_ptr<Timer, std::function<void(Timer*)>> timer_;
  std::vector<std::shared_ptr<GameMsgCommand<bool>>> commands_;
};
