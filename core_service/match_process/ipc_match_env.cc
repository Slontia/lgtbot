#include "match_process/ipc_match_env.h"

#include <algorithm>
#include <ranges>

#include "bot_core/bot_core.h"
#include "match_process/child_session.h"
#include "utility/log.h"

IpcMatchEnv::IpcMatchEnv(ChildGameSession& session)
    : session_(session)
{
    using Ch = lgtbot::ipc::PostResp::Channel;
    broadcast_sender_ = std::make_unique<IpcMsgSender>(*this, Ch::PostResp_Channel_BROADCAST, 0);
    group_sender_     = std::make_unique<IpcMsgSender>(*this, Ch::PostResp_Channel_GROUP, 0);
    ai_sender_        = std::make_unique<IpcMsgSender>(*this, Ch::PostResp_Channel_AI, 0);
}

MsgSenderBase& IpcMatchEnv::BoardcastMsgSender() { return *broadcast_sender_; }

MsgSenderBase& IpcMatchEnv::BoardcastAiInfoMsgSender() { return *ai_sender_; }

MsgSenderBase& IpcMatchEnv::GroupMsgSender() { return *group_sender_; }

MsgSenderBase& IpcMatchEnv::TellMsgSender(const PlayerID pid)
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

const char* IpcMatchEnv::PlayerAvatar(const PlayerID& pid, const int32_t /*size*/)
{
    const auto i = pid.Get();
    if (i < player_avatars_.size()) {
        return player_avatars_[i].c_str();
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

void IpcMatchEnv::IpcMsgSender::SaveText(const char* const data, const uint64_t len)
{
    lgtbot::ipc::MsgItem item;
    item.set_text(std::string(data, data + len));
    items_.push_back(std::move(item));
}

void IpcMatchEnv::IpcMsgSender::SaveUser(const UserID& id, const bool /*is_at*/)
{
    lgtbot::ipc::MsgItem item;
    item.set_user_id(id.GetStr());
    items_.push_back(std::move(item));
}

void IpcMatchEnv::IpcMsgSender::SavePlayer(const PlayerID& id, const bool /*is_at*/)
{
    lgtbot::ipc::MsgItem item;
    item.set_at_player_id(id.Get());
    items_.push_back(std::move(item));
}

void IpcMatchEnv::IpcMsgSender::SaveImage(const char* const path)
{
    lgtbot::ipc::MsgItem item;
    item.set_image_path(path);
    items_.push_back(std::move(item));
}

void IpcMatchEnv::IpcMsgSender::SaveMarkdown(const char* const markdown, const uint32_t width)
{
    lgtbot::ipc::MsgItem item;
    auto* md = item.mutable_markdown();
    md->set_text(markdown);
    md->set_width(width);
    items_.push_back(std::move(item));
}

void IpcMatchEnv::IpcMsgSender::Flush()
{
    if (!items_.empty()) {
        env_.SendPostFrame(channel_, target_pid_, std::move(items_));
        items_.clear();
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

namespace {

static const uint64_t kMinAlertSec = 10;

} // namespace

void IpcMatchEnv::TimerCtl::Start(IpcMatchEnv& env, const uint64_t sec, void* alert_arg, void(*alert_cb)(void*, uint64_t))
{
    if (sec == 0) {
        return;
    }
    Stop(env);
    timer_is_over_ = std::make_shared<bool>(false);

    const auto timeout_handler = [timer_is_over = timer_is_over_, &env](const uint64_t /*sec*/)
        {
            if (!*timer_is_over) {
                if (env.session().main_stage()) {
                    env.session().main_stage()->HandleTimeout();
                }
                env.session().Routine();
            }
        };

    const auto alert_handler = [alert_cb, alert_arg, timer_is_over = timer_is_over_, &env](const uint64_t alert_sec)
        {
            if (!*timer_is_over) {
                alert_cb(alert_arg, alert_sec);
            }
        };

    Timer::TaskSet timeup_tasks;
    if (kMinAlertSec > sec / 2) {
        timeup_tasks.emplace_front(sec, timeout_handler);
    } else {
        timeup_tasks.emplace_front(kMinAlertSec, timeout_handler);
        uint64_t sum_alert_sec = kMinAlertSec;
        for (uint64_t alert_sec = kMinAlertSec; sum_alert_sec < sec / 2; sum_alert_sec += alert_sec, alert_sec *= 2) {
            timeup_tasks.emplace_front(alert_sec, alert_handler);
        }
        timeup_tasks.emplace_front(sec - sum_alert_sec, g_empty_func);
    }
    timer_ = std::make_unique<Timer>(std::move(timeup_tasks));
}

void IpcMatchEnv::TimerCtl::Stop(const IpcMatchEnv& /*env*/)
{
    if (timer_is_over_ == nullptr) {
        return;
    }
    *timer_is_over_ = true;
    timer_is_over_ = nullptr;
    timer_ = nullptr;
}

void IpcMatchEnv::StartTimer(const uint64_t sec, void* const alert_arg, void (*alert_cb)(void*, uint64_t))
{
    timer_cntl_.Start(*this, sec, alert_arg, alert_cb);
}

void IpcMatchEnv::StopTimer() { timer_cntl_.Stop(*this); }

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
