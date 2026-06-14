// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "bot_core/match_internal.h"

#include <cstdlib>
#include <filesystem>

#ifndef MATCH_GAME_RUNNER_PATH
#define MATCH_GAME_RUNNER_PATH "match_game_runner"
#endif

void AppendMsgItem(MsgSenderBase::MsgSenderGuard& g, const lgtbot::ipc::MsgItem& item)
{
    switch (item.content_case()) {
    case lgtbot::ipc::MsgItem::kText:       g << item.text(); break;
    case lgtbot::ipc::MsgItem::kAtPlayerId: g << At(PlayerID{item.at_player_id()}); break;
    case lgtbot::ipc::MsgItem::kUserId:     g << Name(UserID{item.user_id()}); break;
    case lgtbot::ipc::MsgItem::kImagePath:  g << Image{item.image_path()}; break;
    case lgtbot::ipc::MsgItem::kMarkdown:   g << Markdown{item.markdown().text(), item.markdown().width()}; break;
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

void HelpTextCollector::Flush(std::vector<MsgFragment>&& messages) const
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

PrivateBroadcastSender::PrivateBroadcastSender(std::vector<const MsgSenderBase*> targets)
    : targets_(std::move(targets))
{}

void PrivateBroadcastSender::Flush(std::vector<MsgFragment>&& messages) const
{
    if (messages.empty() || targets_.empty()) {
        return;
    }
    for (size_t i = 0; i + 1 < targets_.size(); ++i) {
        targets_[i]->DeliverMessages(std::vector<MsgFragment>(messages));
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
