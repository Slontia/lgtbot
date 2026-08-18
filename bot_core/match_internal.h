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

class MatchPhaseCommon;

void AppendMsgItem(HostMsgSenderBase::MsgSenderGuard& g, const lgtbot::ipc::MsgItem& item,
                   const MatchPhaseCommon* phase);

std::filesystem::path ResolveRunnerExe();

// Captures the first plain-text fragment from a game reply, used to extract
// help text from the child process without sending anything to chat.
class HelpTextCollector final : public HostMsgSenderBase
{
  public:
    explicit HelpTextCollector(std::string& out) : out_(out) {}

  private:
    void Flush(std::vector<HostMsgFragment>&& messages) const override;

    std::string& out_;
};

// Fans out a single message to multiple target senders, delivering the same
// fragments to each participant as a private message.
class PrivateBroadcastSender final : public HostMsgSenderBase
{
  public:
    explicit PrivateBroadcastSender(std::vector<const HostMsgSenderBase*> targets);

  private:
    void Flush(std::vector<HostMsgFragment>&& messages) const override;

    std::vector<const HostMsgSenderBase*> targets_;
};

std::filesystem::path GameLibraryPath(const BotCtx& bot, const GameHandle& gh);
