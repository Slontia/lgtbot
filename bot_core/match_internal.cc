// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "bot_core/match_internal.h"

#include <cstdlib>
#include <filesystem>

#include "bot_core/match_phase_common.h"

#ifndef MATCH_GAME_RUNNER_PATH
#define MATCH_GAME_RUNNER_PATH "match_game_runner"
#endif

void AppendMsgItem(HostMsgSenderBase::MsgSenderGuard& g, const lgtbot::ipc::MsgItem& item,
                   const MatchPhaseCommon* const phase)
{
    switch (item.content_case()) {
    case lgtbot::ipc::MsgItem::kText:      g << item.text(); break;
    case lgtbot::ipc::MsgItem::kAtPlayerId: {
        const auto pid = PlayerID{item.at_player_id()};
        g << "[" << pid.Get() << "号：";
        if (phase) {
            const auto id = phase->ConvertPid(pid);
            if (const auto pval = std::get_if<ComputerID>(&id)) {
                g << "机器人" << pval->Get() << "号";
            } else {
                g << At(std::get<UserID>(id));
            }
        } else {
            g << "玩家";
        }
        g << "]";
        break;
    }
    case lgtbot::ipc::MsgItem::kUserId:    g << Name(UserID{item.user_id()}); break;
    case lgtbot::ipc::MsgItem::kImagePath: g << Image{item.image_path()}; break;
    case lgtbot::ipc::MsgItem::kMarkdown:  g << Markdown{item.markdown().text(), item.markdown().width()}; break;
    default: break;
    }
}

std::filesystem::path ResolveRunnerExe()
{
    if (const char* const e = std::getenv("LGTBOT_MATCH_RUNNER")) {
        return e;
    }
    return std::filesystem::path(MATCH_GAME_RUNNER_PATH);
}

void HelpTextCollector::Flush(std::vector<HostMsgFragment>&& messages) const
{
    if (!out_.empty()) {
        return;
    }
    for (const auto& frag : messages) {
        if (const auto* text = std::get_if<std::string>(&frag)) {
            out_ = *text;
            return;
        }
    }
}

PrivateBroadcastSender::PrivateBroadcastSender(std::vector<const HostMsgSenderBase*> targets)
    : targets_(std::move(targets))
{}

void PrivateBroadcastSender::Flush(std::vector<HostMsgFragment>&& messages) const
{
    if (messages.empty() || targets_.empty()) {
        return;
    }
    for (size_t i = 0; i + 1 < targets_.size(); ++i) {
        targets_[i]->DeliverMessages(std::vector<HostMsgFragment>(messages));
    }
    targets_.back()->DeliverMessages(std::move(messages));
}

std::filesystem::path GameLibraryPath(const BotCtx& bot, const GameHandle& gh)
{
    const auto base = std::filesystem::absolute(bot.game_path()) / gh.Info().module_name_;
#if defined(_WIN32)
    return base / "libgame.dll";
#elif defined(__APPLE__)
    return base / "libgame.dylib";
#else
    return base / "libgame.so";
#endif
}
