// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include "bot_core/id.h"

class MsgSenderBase;

// Cross-module class interface for game level.
class MatchBase
{
  public:
    MatchBase() {}
    virtual ~MatchBase() {}

    // message senders
    virtual MsgSenderBase& BoardcastMsgSender() = 0;
    virtual MsgSenderBase& TellMsgSender(const PlayerID pid) = 0;
    virtual MsgSenderBase& GroupMsgSender() = 0;
    virtual MsgSenderBase& BoardcastAiInfoMsgSender() = 0;

    // player info
    virtual const char* PlayerName(const PlayerID& pid) = 0;
    virtual const char* PlayerAvatar(const PlayerID& pid, const int32_t size) = 0;

    // timer operation
    virtual void StartTimer(const uint64_t sec, void* p, void(*cb)(void*, uint64_t)) = 0;
    virtual void StopTimer() = 0;

    // player operation
    virtual void Eliminate(const PlayerID pid) = 0;
    virtual void Hook(const PlayerID pid) = 0;
    virtual void Activate(const PlayerID pid) = 0;

    // match info
    virtual bool IsInDeduction() const = 0;
    virtual uint64_t MatchId() const = 0;
    virtual const char* GameName() const = 0;
};
