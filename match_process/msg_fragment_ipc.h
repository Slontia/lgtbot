// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <vector>

#include "bot_core/msg_sender.h"
#include "match_process/match_ipc.pb.h"

namespace lgtbot::ipc {

std::vector<MsgItem> MsgFragmentsToItems(std::vector<MsgFragment> fragments);

} // namespace lgtbot::ipc
