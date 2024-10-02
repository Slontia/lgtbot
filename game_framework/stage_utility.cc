// Copyright (c) 2024-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/stage_utility.h"

#include "game_framework/util.h"

#ifndef GAME_MODULE_NAME
#error GAME_MODULE_NAME is not defined
#endif

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

namespace internal {

PublicStageUtility::PublicStageUtility(const CustomOptions& game_options, const lgtbot::game::GenericOptions& generic_options, MatchBase& match)
    : game_options_{game_options}
    , generic_options_(generic_options)
    , match_(match)
    , masker_(match.MatchId(), match.GameName(), generic_options.PlayerNum())
    , achievement_counts_(generic_options.PlayerNum())
{
    std::ranges::for_each(achievement_counts_, [](AchievementCounts& counts) { std::ranges::fill(counts, 0); });
}

MsgSenderBase& PublicStageUtility::BoardcastMsgSender() const
{
    return IsInDeduction() ? EmptyMsgSender::Get() : match_.BoardcastMsgSender();
}

MsgSenderBase& PublicStageUtility::TellMsgSender(const PlayerID pid) const
{
    return IsInDeduction() ? EmptyMsgSender::Get() : match_.TellMsgSender(pid);
}

MsgSenderBase& PublicStageUtility::GroupMsgSender() const
{
    return IsInDeduction() ? EmptyMsgSender::Get() : match_.GroupMsgSender();
}

MsgSenderBase& PublicStageUtility::BoardcastAiInfoMsgSender() const
{
    return IsInDeduction() ? EmptyMsgSender::Get() : match_.BoardcastAiInfoMsgSender();
}

void PublicStageUtility::BoardcastAiInfo(nlohmann::json j)
{
    BoardcastAiInfoMsgSender()() << nlohmann::json{
            { "match_id", match_.MatchId() },
            { "info_id", bot_message_id_++ },
            { "info", std::move(j) },
        }.dump();
}

int PublicStageUtility::SaveMarkdown(const std::string& markdown, const uint32_t width)
{
    const std::filesystem::path path = std::filesystem::path(generic_options_.saved_image_dir_) /
        ("match_saved_" + std::to_string(saved_image_no_ ++) + ".png");
    return MarkdownToImage(markdown, path.string(), width);
}

void PublicStageUtility::Eliminate(const PlayerID pid)
{
    match_.Eliminate(pid);
    Leave(pid);
}

void PublicStageUtility::HookUnreadyPlayers()
{
    for (PlayerID pid = 0; pid < PlayerNum(); ++pid) {
        if (!IsReady(pid)) {
            Hook(pid);
        }
    }
}

void PublicStageUtility::Hook(const PlayerID pid)
{
    match_.Hook(pid);
    masker_.SetTemporaryInactive(pid);
}

void PublicStageUtility::StartTimer(const uint64_t sec)
{
    timer_finish_time_ = std::chrono::steady_clock::now() + std::chrono::seconds(sec);
    // cannot pass substage pointer because substage may has been released when alert
    match_.StartTimer(sec, this,
            generic_options_.public_timer_alert_ ? TimerCallbackPublic_ : TimerCallbackPrivate_);
}

void PublicStageUtility::StopTimer()
{
    match_.StopTimer();
    timer_finish_time_ = std::nullopt;
}

void PublicStageUtility::Leave(const PlayerID pid)
{
    masker_.SetPermanentInactive(pid);
    if (IsInDeduction()) {
        match_.GroupMsgSender()() << "所有玩家都失去了行动能力，于是游戏将直接推演至终局";
    }
}

void PublicStageUtility::TimerCallbackPublic_(void* const p, const uint64_t alert_sec)
{
    auto& utility = *static_cast<PublicStageUtility*>(p);
    auto sender = utility.Boardcast();
    sender << "剩余时间" << (alert_sec / 60) << "分" << (alert_sec % 60) <<
        "秒\n\n以下玩家还未选择，要抓紧了，机会不等人\n";
    for (PlayerID pid = 0; pid < utility.PlayerNum(); ++pid) {
        if (!utility.masker_.IsReady(pid) && !utility.masker_.IsInactive(pid)) {
            sender << At(pid);
        }
    }
}

void PublicStageUtility::TimerCallbackPrivate_(void* const p, const uint64_t alert_sec)
{
    auto& utility = *static_cast<PublicStageUtility*>(p);
    utility.Boardcast() << "剩余时间" << (alert_sec / 60) << "分" << (alert_sec % 60) << "秒";
    for (PlayerID pid = 0; pid < utility.PlayerNum(); ++pid) {
        if (!utility.masker_.IsReady(pid) && !utility.masker_.IsInactive(pid)) {
            utility.Tell(pid) << "您还未选择，要抓紧了，机会不等人";
        }
    }
}

} // namespace internal

uint8_t StageUtility::AchievementCount(const PlayerID pid, const Achievement& achievement) const
{
    return achievement_counts_[pid][achievement.ToUInt()];
}

void StageUtility::Activate(const PlayerID pid)
{
    match_.Activate(pid);
    masker_.SetActive(pid);
    assert(!masker_.IsInactive(pid));
}

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot
