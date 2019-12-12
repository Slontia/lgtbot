#pragma once
#include "msg_checker.h"
#include <optional>
#include <cassert>

enum StageEnum;

class Stage
{
public:
  Stage(const StageEnum stage_id, std::vector<std::unique_ptr<MsgCommand>>&& commands)
    : stage_id_(stage_id), commands_(std::move(commands)) {}
  virtual ~Stage() {}
  virtual bool HandleRequest(MsgReader& reader) = 0
  {
    for (const std::unique_ptr<MsgCommand>& command : commands_)
    {
      if (command->CallIfValid(reader)) { return true; }
    }
    return false;
  }
  virtual void HandleTimeout() = 0;
  bool IsOver() const { return is_over_; }
  StageEnum StageID() const { return stage_id_; }

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
  std::vector<std::unique_ptr<MsgCommand>> commands_;
  bool is_over_;
};

class CompStage : public Stage
{
public:
  CompStage(const StageEnum stage_id, std::vector<std::unique_ptr<MsgCommand>>&& commands, std::unique_ptr<Stage>&& sub_stage)
    : Stage(stage_id, std::move(commands)), sub_stage_(std::move(sub_stage)) {}
  virtual ~CompStage() {}
  virtual std::unique_ptr<Stage> NextSubStage(const StageEnum cur_stage) const = 0;
  virtual bool HandleRequest(MsgReader& reader) override final
  {
    if (Stage::HandleRequest(reader)) { return true; }
    else if (sub_stage_->HandleRequest(reader))
    {
      if (sub_stage_->IsOver()) { CheckoutSubStage(); }
      return true;
    }
    return false;
  }
  virtual void HandleTimeout() override final
  {
    sub_stage_->HandleTimeout();
    if (sub_stage_->IsOver()) { CheckoutSubStage(); }
  }
  void CheckoutSubStage()
  {
    sub_stage_ = NextSubStage(sub_stage_->StageID());
    if (!sub_stage_) { Over(); } // no more substages
  }

private:
  std::unique_ptr<Stage> sub_stage_;
};

class AtomStage : public Stage
{
public:
  using Stage::Stage;
  virtual ~AtomStage() {}
  virtual uint32_t TimerSec() const = 0;
  virtual bool HandleRequest(MsgReader& reader) override final { Stage::HandleRequest(reader); }
  virtual void HandleTimeout() override final { Over(); }
  using Stage::Over; // to public
};
