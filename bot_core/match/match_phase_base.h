// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "bot_core/match/match_common.h"

namespace lgtbot {
namespace core {
namespace match {

class MatchPhaseBase
{
  public:
    MatchPhaseBase(const MatchContext& ctx, MatchMessaging* messaging,
            std::map<UserID, MatchParticipantUser>& users);

    HostMsgSenderBase& BoardcastMsgSender();
    HostMsgSenderBase& GroupMsgSender();

    HostMsgSenderBase::MsgSenderGuard Boardcast();
    HostMsgSenderBase::MsgSenderGuard BoardcastAtAll();

    size_t UserNum() const;
    void BriefInfo(std::string& out) const;

    virtual ErrCode UserInterrupt(const UserID uid, HostMsgSenderBase& reply, const bool cancel) = 0;
    virtual void ShowInfo(HostMsgSenderBase& reply) const = 0;
    virtual bool SwitchHost() = 0;
    virtual ErrCode Terminate(const bool is_force) = 0;
    virtual UserID HostUserId() const = 0;

  protected:
    bool UserIsActive(const MatchParticipantUser& user) const noexcept;

    HostMsgSenderBase::MsgSenderGuard MakeGroupGuard_() { return GroupMsgSender()(); }

    void ShowInfoHeader_(HostMsgSenderBase& reply, const char* status) const;

    virtual HostMsgSenderBase* GroupSenderOrNull_() = 0;
    virtual MatchVariantID HostVariantId_() const = 0;
    virtual uint32_t ComputerNumImpl_() const = 0;
    virtual const MatchRuntimeOptions& RuntimeOptions_() const = 0;

    const MatchContext& ctx_;
    MatchMessaging* messaging_{nullptr};
    std::map<UserID, MatchParticipantUser>& users_;
};

} // namespace match
} // namespace core
} // namespace lgtbot
