// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "bot_core/match_env.h"
#include "bot_core/match_types.h"

class MatchPhaseCommon
{
  public:
    MatchPhaseCommon(const MatchContext& ctx, MatchMessaging* messaging,
            std::map<UserID, MatchParticipantUser>& users, std::vector<MatchPlayer>& players);

    MsgSenderBase& BoardcastMsgSender();
    MsgSenderBase& TellMsgSender(const PlayerID pid);
    MsgSenderBase& GroupMsgSender();

    const char* PlayerName(const PlayerID& pid);
    const char* PlayerAvatar(const PlayerID& pid, const int32_t size);

    MsgSenderBase::MsgSenderGuard Boardcast();
    MsgSenderBase::MsgSenderGuard BoardcastAtAll();

    size_t UserNum() const;
    MatchVariantID ConvertPid(const PlayerID pid) const;
    void BriefInfo(std::string& out) const;

    virtual ErrCode UserInterrupt(const UserID uid, MsgSenderBase& reply, const bool cancel) = 0;
    virtual void ShowInfo(MsgSenderBase& reply, const std::weak_ptr<const class Match>& match_wk) const = 0;
    virtual bool SwitchHost() = 0;
    virtual ErrCode Terminate(const bool is_force) = 0;
    virtual UserID HostUserId() const = 0;

    void BindMsgSenderMatch(const std::weak_ptr<const class Match>& match_wk);

  protected:
    bool UserIsActive(const MatchParticipantUser& user) const noexcept;

    MsgSenderBase::MsgSenderGuard MakeTellGuard_(const PlayerID pid) { return TellMsgSender(pid)(); }

    MsgSenderBase::MsgSenderGuard MakeGroupGuard_() { return GroupMsgSender()(); }

    void ShowInfoHeader_(MsgSenderBase& reply, const std::weak_ptr<const class Match>& match_wk,
            const char* status) const;

    virtual MsgSenderBase* GroupSenderOrNull_() = 0;
    virtual MatchVariantID HostVariantId_() const = 0;
    virtual uint32_t ComputerNumImpl_() const = 0;
    virtual const MatchRuntimeOptions& RuntimeOptions_() const = 0;

    const MatchContext& ctx_;
    MatchMessaging* messaging_{nullptr};
    std::map<UserID, MatchParticipantUser>& users_;
    std::vector<MatchPlayer>& players_;
};
