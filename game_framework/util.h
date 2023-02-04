// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once
#include <string>

#include "game_framework/game_main.h"
#include "bot_core/msg_sender.h"

// interfaces provided by bot_core
extern MsgSenderBase& BoardcastMsgSender(void* match_p);
extern MsgSenderBase& TellMsgSender(void* match_p, const uint64_t pid);
extern void StartTimer(void* match, const uint64_t sec);
extern void StopTimer(void* match);
extern const char* Rule();
