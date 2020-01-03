#pragma once
#include <optional>
#include <cassert>
#include "stdafx.h"
#include "msg_checker.h"
#include "game_stage.h"
#include "game.h"

enum StageEnum;
class Game;

using GameMsgCommand = MsgCommand<std::string(const uint64_t, const bool)>;

Stage::Stage(const StageEnum stage_id, std::vector<std::unique_ptr<GameMsgCommand>>&& commands, Game& game)
  : stage_id_(stage_id), commands_(std::move(commands)), is_over_(false), game_(game) {}

Stage::~Stage() {}

bool Stage::HandleRequest(MsgReader& reader, const uint64_t player_id, const bool is_public, const std::function<void(const std::string&)>& reply)
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
  
void Stage::Over()
{
  if (!is_over_)
  {
    OnOver();
    is_over_ = true;
  }
}

CompStage::CompStage(const StageEnum stage_id, std::vector<std::unique_ptr<GameMsgCommand>>&& commands, Game& game, std::unique_ptr<Stage>&& sub_stage)
  : Stage(stage_id, std::move(commands), game), sub_stage_(std::move(sub_stage)) {}

CompStage::~CompStage() {}

bool CompStage::HandleRequest(MsgReader& reader, const uint64_t player_id, const bool is_public, const std::function<void(const std::string&)>& reply)
{
  if (Stage::HandleRequest(reader, player_id, is_public, reply)) { return true; }
  const bool sub_stage_handled = sub_stage_->HandleRequest(reader, player_id, is_public, reply);
  if (sub_stage_->IsOver()) { CheckoutSubStage(); }
  return sub_stage_handled;
}

void CompStage::HandleTimeout()
{
  sub_stage_->HandleTimeout();
  if (sub_stage_->IsOver()) { CheckoutSubStage(); }
}

void CompStage::CheckoutSubStage()
{
  sub_stage_ = NextSubStage(sub_stage_->StageID());
  if (!sub_stage_) { Over(); } // no more substages
}
