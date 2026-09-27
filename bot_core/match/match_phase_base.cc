// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "bot_core/match/match_phase_base.h"

#include <memory>

#include "utility/empty_func.h"

namespace lgtbot {
namespace core {
namespace match {

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

MatchPhaseBase::MatchPhaseBase(const MatchContext& ctx, MatchMessaging* messaging,
        std::map<UserID, MatchParticipantUser>& users)
        : ctx_(ctx)
        , messaging_(messaging)
        , users_(users)
{}

bool MatchPhaseBase::UserIsActive(const MatchParticipantUser& user) const noexcept
{
    return user.presence_.load(std::memory_order_acquire) != UserPresence::LEFT;
}

HostMsgSenderBase& MatchPhaseBase::BoardcastMsgSender()
{
    if (HostMsgSenderBase* const group = GroupSenderOrNull_()) {
        return *group;
    }
    *messaging_->private_broadcast_scratch = MakePrivateBroadcastSender(users_);
    return **messaging_->private_broadcast_scratch;
}

HostMsgSenderBase& MatchPhaseBase::GroupMsgSender()
{
    if (HostMsgSenderBase* const group = GroupSenderOrNull_()) {
        return *group;
    }
    return HostEmptyMsgSender::Get();
}

HostMsgSenderBase::MsgSenderGuard MatchPhaseBase::Boardcast()
{
    if (HostMsgSenderBase* const group = GroupSenderOrNull_()) {
        return (*group)();
    }
    return HostMsgSenderBase::MsgSenderGuard(MakePrivateBroadcastSender(users_));
}

HostMsgSenderBase::MsgSenderGuard MatchPhaseBase::BoardcastAtAll()
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

size_t MatchPhaseBase::UserNum() const { return users_.size(); }

void MatchPhaseBase::BriefInfo(std::string& out) const
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

void MatchPhaseBase::ShowInfoHeader_(HostMsgSenderBase& reply, const char* status) const
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

} // namespace match
} // namespace core
} // namespace lgtbot
