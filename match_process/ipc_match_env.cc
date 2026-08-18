#include "match_process/ipc_match_env.h"

#include <algorithm>
#include <ranges>
#include <string_view>

#include "bot_core/bot_core.h"
#include "match_process/child_session.h"
#include "match_process/msg_fragment_ipc.h"
#include "utility/log.h"

namespace {

std::string ResizeAvatarHtml(std::string html, const int32_t size)
{
    if (size <= 0) {
        return html;
    }
    const std::string size_px = std::to_string(size) + "px";
    const auto replace_px_value = [&](const std::string_view key)
    {
        const auto key_pos = html.find(key);
        if (key_pos == std::string::npos) {
            return;
        }
        const auto value_pos = html.find_first_not_of(" \t", key_pos + key.size());
        if (value_pos == std::string::npos) {
            return;
        }
        const auto end_pos = html.find("px", value_pos);
        if (end_pos == std::string::npos) {
            return;
        }
        html.replace(value_pos, end_pos - value_pos + 2, size_px);
    };
    replace_px_value("width:");
    replace_px_value("height:");
    return html;
}

} // namespace

IpcMatchEnv::IpcMatchEnv(ChildGameSession& session)
    : session_(session)
{
    using Ch = lgtbot::ipc::PostResp::Channel;
    broadcast_sender_ = std::make_unique<IpcMsgSender>(*this, Ch::PostResp_Channel_BROADCAST, 0);
    group_sender_     = std::make_unique<IpcMsgSender>(*this, Ch::PostResp_Channel_GROUP, 0);
}

ChildMsgSenderBase& IpcMatchEnv::BoardcastMsgSender() { return *broadcast_sender_; }

ChildMsgSenderBase& IpcMatchEnv::GroupMsgSender() { return *group_sender_; }

ChildMsgSenderBase& IpcMatchEnv::TellMsgSender(const PlayerID pid)
{
    const auto it = tell_senders_.find(pid);
    if (it != tell_senders_.end()) {
        return *it->second;
    }
    auto [placed, _] = tell_senders_.emplace(pid,
        std::make_unique<IpcMsgSender>(*this, lgtbot::ipc::PostResp::Channel::PostResp_Channel_TELL, pid.Get()));
    return *placed->second;
}

const char* IpcMatchEnv::PlayerName(const PlayerID& pid)
{
    thread_local static std::string buf;
    const auto i = pid.Get();
    if (i < player_names_.size()) {
        return player_names_[i].c_str();
    }
    buf = "player_" + std::to_string(i);
    return buf.c_str();
}

const char* IpcMatchEnv::PlayerAvatar(const PlayerID& pid, const int32_t size)
{
    thread_local static std::string buf;
    const auto i = pid.Get();
    if (i < player_avatars_.size()) {
        return (buf = ResizeAvatarHtml(player_avatars_[i], size)).c_str();
    }
    return "";
}

void IpcMatchEnv::SetMeta(const uint64_t match_id, std::string game_name, std::vector<std::string> player_names, std::vector<std::string> player_avatars,
        std::vector<bool> slot_is_computer)
{
    match_id_ = match_id;
    game_name_ = std::move(game_name);
    player_names_ = std::move(player_names);
    player_avatars_ = std::move(player_avatars);
    players_.clear();
    players_.reserve(slot_is_computer.size());
    for (const bool c : slot_is_computer) {
        PlayerSlot slot;
        slot.is_computer_ = c;
        players_.push_back(slot);
    }
}

void IpcMatchEnv::WritePlayerStateToParent(const PlayerID pid, const char* const state)
{
    lgtbot::ipc::GameResponse resp;
    auto* ps = resp.mutable_player_state();
    ps->set_pid(pid.Get());
    ps->set_state(state);
    session_.SendProto(resp);
}

void IpcMatchEnv::SendPostFrame(lgtbot::ipc::PostResp::Channel channel, uint32_t target_pid,
                                std::vector<lgtbot::ipc::MsgItem> items)
{
    lgtbot::ipc::GameResponse resp;
    auto* post = resp.mutable_post();
    post->set_channel(channel);
    post->set_target_pid(target_pid);
    for (auto& item : items) {
        *post->add_items() = std::move(item);
    }
    session_.SendProto(resp);
}

void IpcMatchEnv::IpcMsgSender::Flush(std::vector<ChildMsgFragment>&& messages) const
{
    if (!messages.empty()) {
        env_.SendPostFrame(channel_, target_pid_, lgtbot::ipc::MsgFragmentsToItems(std::move(messages)));
    }
}

void IpcMatchEnv::Eliminate(const PlayerID pid)
{
    if (pid.Get() >= players_.size()) {
        return;
    }
    auto& slot = players_[pid.Get()];
    if (std::exchange(slot.state_, PlayerSlot::State::ELIMINATED) != PlayerSlot::State::ELIMINATED) {
        TellMsgSender(pid)() << "很遗憾，您被淘汰了，可以通过「" META_COMMAND_SIGN "退出」以退出游戏";
        is_in_deduction_ = std::ranges::all_of(players_, [](const PlayerSlot& p) { return p.is_computer_ || p.state_ == PlayerSlot::State::ELIMINATED; });
        WritePlayerStateToParent(pid, "eliminated");
    }
}

void IpcMatchEnv::Hook(const PlayerID pid)
{
    if (pid.Get() >= players_.size()) {
        return;
    }
    auto& slot = players_[pid.Get()];
    if (slot.state_ == PlayerSlot::State::ACTIVE) {
        TellMsgSender(pid)() << "您已经进入挂机状态，若其他玩家已经行动完成，裁判将不再继续等待您，执行任意游戏请求可恢复至原状态";
        slot.state_ = PlayerSlot::State::HOOKED;
        WritePlayerStateToParent(pid, "hooked");
    }
}

void IpcMatchEnv::Activate(const PlayerID pid)
{
    if (pid.Get() >= players_.size()) {
        return;
    }
    auto& slot = players_[pid.Get()];
    if (slot.state_ == PlayerSlot::State::HOOKED) {
        TellMsgSender(pid)() << "挂机状态已取消";
        slot.state_ = PlayerSlot::State::ACTIVE;
        WritePlayerStateToParent(pid, "active");
    }
}

void IpcMatchEnv::StartTimer(const uint64_t sec, void* const alert_arg, void (*alert_cb)(void*, uint64_t))
{
    alert_arg_ = alert_arg;
    alert_cb_  = alert_cb;
    lgtbot::ipc::GameResponse resp;
    resp.mutable_timer_start()->set_duration_sec(sec);
    session_.SendProto(resp);
}

void IpcMatchEnv::StopTimer()
{
    alert_arg_ = nullptr;
    alert_cb_  = nullptr;
    lgtbot::ipc::GameResponse resp;
    resp.mutable_timer_stop();
    session_.SendProto(resp);
}

uint32_t IpcMatchEnv::ComputerNum() const
{
    return static_cast<uint32_t>(std::ranges::count_if(players_, [](const PlayerSlot& p) { return p.is_computer_; }));
}

bool IpcMatchEnv::IsComputerAt(const uint32_t index) const
{
    return index < players_.size() && players_[index].is_computer_;
}

bool IpcMatchEnv::IsEliminatedAt(const uint32_t index) const
{
    return index < players_.size() && players_[index].state_ == PlayerSlot::State::ELIMINATED;
}
