// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "bot_core/bot_ctx.h"
#include "bot_core/game_handle.h"
#include "bot_core/msg_sender.h"
#include "match_process/match_ipc.pb.h"

void AppendMsgItem(MsgSenderBase::MsgSenderGuard& g, const lgtbot::ipc::MsgItem& item);

std::filesystem::path ResolveRunnerExe();

class HelpTextCollector final : public MsgSenderBase
{
  public:
    explicit HelpTextCollector(std::string& out) : out_(out) {}

  private:
    void SetMatch(std::weak_ptr<const class Match> /*match*/) override {}

    void Flush(std::vector<MsgFragment>&& messages) const override;

    std::string& out_;
};

class PrivateBroadcastSender final : public MsgSenderBase
{
  public:
    explicit PrivateBroadcastSender(std::vector<const MsgSenderBase*> targets);

  private:
    void SetMatch(std::weak_ptr<const class Match> /*match*/) override {}

    void Flush(std::vector<MsgFragment>&& messages) const override;

    std::vector<const MsgSenderBase*> targets_;
};

std::filesystem::path GameLibraryPath(const BotCtx& bot, const GameHandle& gh);
