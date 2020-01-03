#pragma once
#include "msg_checker.h"
#include <optional>
#include <cassert>

enum StageEnum;
class Game;

using GameMsgCommand = MsgCommand<std::string(const uint64_t, const bool)>;

class Stage
{
public:
  Stage(const StageEnum stage_id, std::vector<std::unique_ptr<GameMsgCommand>>&& commands, Game& game);
  virtual ~Stage();
  virtual bool HandleRequest(MsgReader& reader, const uint64_t player_id, const bool is_public, const std::function<void(const std::string&)>& reply) = 0;
  virtual void HandleTimeout() = 0;
  bool IsOver() const { return is_over_; }
  StageEnum StageID() const { return stage_id_; }

protected:
  virtual std::string Name() const = 0;
  virtual void OnOver() = 0;
  void Over();

private:
  const StageEnum stage_id_;
  std::vector<std::unique_ptr<GameMsgCommand>> commands_;
  bool is_over_;
  Game& game_;
};

class CompStage : public Stage
{
public:
  CompStage(const StageEnum stage_id, std::vector<std::unique_ptr<GameMsgCommand>>&& commands, Game& game, std::unique_ptr<Stage>&& sub_stage);
  virtual ~CompStage();
  virtual std::unique_ptr<Stage> NextSubStage(const StageEnum cur_stage) const = 0;
  virtual bool HandleRequest(MsgReader& reader, const uint64_t player_id, const bool is_public, const std::function<void(const std::string&)>& reply);
  virtual void HandleTimeout();
  void CheckoutSubStage();

private:
  std::unique_ptr<Stage> sub_stage_;
};

class AtomStage : public Stage
{
public:
  using Stage::Stage;
  virtual ~AtomStage() {}
  virtual uint32_t TimerSec() const = 0;
  using Stage::HandleRequest;
  virtual void HandleTimeout() override final { Over(); }
  using Stage::Over; // to public
};
