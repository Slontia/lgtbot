// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "bot_core/match_phase_common.h"

#include <memory>

#include "utility/empty_func.h"
#include "bot_core/match_internal.h"

namespace {

std::unique_ptr<HostMsgSenderBase> MakePrivateBroadcastSender(const std::map<UserID, MatchParticipantUser>& users)
{
    std::vector<const HostMsgSenderBase*> targets;
    targets.reserve(users.size());
    for (const auto& [_, user] : users) {
        if (user.presence_.load(std::memory_order_acquire) != UserPresence::LEFT) {
            targets.push_back(&user.sender_);
        }
    }
    return std::make_unique<PrivateBroadcastSender>(std::move(targets));
}

} // namespace

MatchPhaseCommon::MatchPhaseCommon(const MatchContext& ctx, MatchMessaging* messaging,
        std::map<UserID, MatchParticipantUser>& users, std::vector<MatchPlayer>& players)
        : ctx_(ctx)
        , messaging_(messaging)
        , users_(users)
        , players_(players)
{}

bool MatchPhaseCommon::UserIsActive(const MatchParticipantUser& user) const noexcept
{
    return user.presence_.load(std::memory_order_acquire) != UserPresence::LEFT;
}

HostMsgSenderBase& MatchPhaseCommon::BoardcastMsgSender()
{
    if (HostMsgSenderBase* const group = GroupSenderOrNull_()) {
        return *group;
    }
    *messaging_->private_broadcast_scratch = MakePrivateBroadcastSender(users_);
    return **messaging_->private_broadcast_scratch;
}

HostMsgSenderBase& MatchPhaseCommon::TellMsgSender(const PlayerID pid)
{
    const auto& id = ConvertPid(pid);
    const auto pval = std::get_if<UserID>(&id);
    if (!pval) {
        return HostEmptyMsgSender::Get();
    }
    if (const auto it = users_.find(*pval); it != users_.end() && UserIsActive(it->second)) {
        return it->second.sender_;
    }
    return HostEmptyMsgSender::Get();
}

HostMsgSenderBase& MatchPhaseCommon::GroupMsgSender()
{
    if (HostMsgSenderBase* const group = GroupSenderOrNull_()) {
        return *group;
    }
    return HostEmptyMsgSender::Get();
}

HostMsgSenderBase::MsgSenderGuard MatchPhaseCommon::Boardcast()
{
    if (HostMsgSenderBase* const group = GroupSenderOrNull_()) {
        return (*group)();
    }
    return HostMsgSenderBase::MsgSenderGuard(MakePrivateBroadcastSender(users_));
}

HostMsgSenderBase::MsgSenderGuard MatchPhaseCommon::BoardcastAtAll()
{
    if (ctx_.gid.has_value()) {
        auto sender = Boardcast();
        for (const auto& [uid, user] : users_) {
            if (UserIsActive(user)) {
                sender << At(uid);
            }
        }
        sender << "\n";
        return sender;
    }
    return Boardcast();
}

const char* MatchPhaseCommon::PlayerName(const PlayerID& pid)
{
    thread_local static std::string str;
    const auto& id = ConvertPid(pid);
    if (const auto pval = std::get_if<ComputerID>(&id)) {
        return (str = "机器人" + std::to_string(pval->Get()) + "号").c_str();
    }
    return (str = ctx_.bot.GetUserName(std::get<UserID>(id).GetCStr(),
                              ctx_.gid.has_value() ? ctx_.gid->GetCStr() : nullptr))
            .c_str();
}

const char* MatchPhaseCommon::PlayerAvatar(const PlayerID& pid, const int32_t size)
{
    thread_local static std::string str;
    const auto& id = ConvertPid(pid);
    if (std::get_if<ComputerID>(&id)) {
        return "";
    }
    return (str = ctx_.bot.GetUserAvatar(std::get<UserID>(id).GetCStr(), size)).c_str();
}

MatchVariantID MatchPhaseCommon::ConvertPid(const PlayerID pid) const
{
    if (!pid.IsValid()) {
        return HostVariantId_();
    }
    return players_[pid.Get()].id_;
}

size_t MatchPhaseCommon::UserNum() const { return users_.size(); }

void MatchPhaseCommon::BriefInfo(std::string& out) const
{
    const auto multiple = ctx_.Multiple();
    const char* const name = ctx_.game_handle.Info().name_;
    const auto& options = RuntimeOptions_();
    const auto is_formal = options.generic_options_.is_formal_;
    const auto usize = users_.size();
    const auto cnum = ComputerNumImpl_();
    out = std::string("游戏名称：") + name +
        "\n- 倍率：" +
        (is_formal || multiple == 0 ? std::to_string(multiple) :
                                      "0（开启计分后为 " + std::to_string(multiple) + "）") +
        "\n- 当前用户数：" + std::to_string(usize) +
        "\n- 当前电脑数：" + std::to_string(cnum);
}

void MatchPhaseCommon::ShowInfoHeader_(HostMsgSenderBase& reply, const char* status) const
{
    auto sender = reply();
    sender << "游戏名称：" << ctx_.game_handle.Info().name_ << "\n";
    sender << "配置信息：" << ctx_.OptionInfo() << "\n";
    sender << "电脑数量：" << ComputerNumImpl_() << "\n";
    sender << "游戏状态：" << status << "\n";
    sender << "房间号：";
    if (ctx_.gid.has_value()) {
        sender << (*ctx_.gid) << "\n";
    } else {
        sender << "私密游戏" << "\n";
    }
    sender << "最多可参加人数：";
    if (const auto max_player = ctx_.MaxPlayerNum(); max_player == 0) {
        sender << "无限制";
    } else {
        sender << max_player;
    }
    sender << "人\n房主：";
    const auto host_id = HostVariantId_();
    if (const auto* uid = std::get_if<UserID>(&host_id)) {
        sender << Name(*uid);
    } else {
        sender << "机器人";
    }
}
