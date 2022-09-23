// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <array>
#include <functional>
#include <map>
#include <memory>

#include "game_framework/game_main.h"
#include "game_framework/game_options.h"
#include "game_framework/game_stage.h"
#include "nerduel_core.h"
#include "utility/msg_checker.h"

const std::string k_game_name = "Nerduel";
const uint64_t k_max_player = 2; /* 0 means no max-player limits */
const uint64_t k_multiple = 1;

std::string GameOption::StatusInfo() const {
  std::stringstream ss;
  ss << "等式长度为" << GET_VALUE(等式长度) << "；等式限制：" << (GET_VALUE(允许负号) ? "" : "不")
     << "允许负号。";
  return ss.str();
}

bool GameOption::ToValid(MsgSenderBase& reply) {
  if (PlayerNum() != 2) {
    reply() << "该游戏为双人游戏，必须为2人参加，当前玩家数为" << PlayerNum();
    return false;
  }
  return true;
}

uint64_t GameOption::BestPlayerNum() const { return 2; }

// ========== GAME STAGES ==========

class RoundStage;

class MainStage : public MainGameStage<RoundStage> {
 public:
  MainStage(const GameOption& option, MatchBase& match);
  virtual VariantSubStage OnStageBegin() override;
  virtual VariantSubStage NextSubStage(RoundStage& sub_stage, const CheckoutReason reason) override;
  int64_t PlayerScore(const PlayerID pid) const;

 private:
  bool JudgeOver();
  void Info_() { Boardcast() << Markdown("## 第" + std::to_string(round_) + "回合\n\n"); }

  int round_;
  std::vector<std::pair<PlayerID, std::string>> targets_;
  std::map<PlayerID, int64_t> score_;
};

class SettingStage : public SubGameStage<> {
 public:
  SettingStage(MainStage& main_stage)
      : GameStage(main_stage, "设置等式阶段",
                  MakeStageCommand("设置等式", &SettingStage::Set_, AnyArg("等式", "1+2+3=6"))) {}

  virtual void OnStageBegin() override {
    Boardcast() << "请双方设置等式";
    StartTimer(180);
  }

 private:
  AtomReqErrCode Set_(const PlayerID pid, const bool is_public, MsgSenderBase& reply,
                      std::string str) {
    reply() << "收到了字符串：" << str << "\n评估结果：" << evaluate(str)
            << "\n[错误] 后续阶段尚未实现";
    return StageErrCode::FAILED;
  }
};

class RoundStage : public SubGameStage<SettingStage> {
 public:
  RoundStage(MainStage& main_stage, const uint64_t round)
      : GameStage(main_stage, "第" + std::to_string(round) + "回合") {}

  virtual VariantSubStage OnStageBegin() override {
    return std::make_unique<SettingStage>(main_stage());
  }

  virtual VariantSubStage NextSubStage(SettingStage& sub_stage,
                                       const CheckoutReason reason) override {
    return std::make_unique<SettingStage>(main_stage());
  }

 private:
};

MainStage::MainStage(const GameOption& option, MatchBase& match)
    : GameStage(option, match), round_(1) {}

MainStage::VariantSubStage MainStage::OnStageBegin() {
  return std::make_unique<RoundStage>(*this, round_);
}

MainStage::VariantSubStage MainStage::NextSubStage(RoundStage& sub_stage,
                                                   const CheckoutReason reason) {
  if (JudgeOver()) {
    return {};
  }
  return std::make_unique<RoundStage>(*this, round_);
}

int64_t MainStage::PlayerScore(const PlayerID pid) const { return score_.at(pid); }

bool MainStage::JudgeOver() {
  Info_();
  return false;
}

MainStageBase* MakeMainStage(MsgSenderBase& reply, GameOption& options, MatchBase& match) {
  if (!options.ToValid(reply)) {
    return nullptr;
  }
  return new MainStage(options, match);
}
