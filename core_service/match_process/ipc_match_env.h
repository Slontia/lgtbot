#pragma once

#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "bot_core/id.h"
#include "bot_core/match_base.h"
#include "bot_core/msg_sender.h"
#include "bot_core/timer.h"
#include "game_framework/game_main.h"
#include "match_process/match_ipc.pb.h"
#include "utility/empty_func.h"

class ChildGameSession;

// MatchBase implementation inside the game child process: forwards outbound messages as proto frames.
class IpcMatchEnv final : public MatchBase
{
  public:
    explicit IpcMatchEnv(ChildGameSession& session);

    MsgSenderBase& BoardcastMsgSender() override;
    MsgSenderBase& BoardcastAiInfoMsgSender() override;
    MsgSenderBase& TellMsgSender(const PlayerID pid) override;
    MsgSenderBase& GroupMsgSender() override;

    const char* PlayerName(const PlayerID& pid) override;
    const char* PlayerAvatar(const PlayerID& pid, const int32_t size) override;

    void StartTimer(const uint64_t sec, void* alert_arg, void(*alert_cb)(void*, uint64_t)) override;
    void StopTimer() override;

    void Eliminate(const PlayerID pid) override;
    void Hook(const PlayerID pid) override;
    void Activate(const PlayerID pid) override;

    bool IsInDeduction() const override { return is_in_deduction_; }
    uint64_t MatchId() const override { return match_id_; }
    const char* GameName() const override { return game_name_.c_str(); }

    void SetMeta(const uint64_t match_id, std::string game_name, std::vector<std::string> player_names, std::vector<std::string> player_avatars,
            std::vector<bool> slot_is_computer);

    void WritePlayerStateToParent(const PlayerID pid, const char* state);

    ChildGameSession& session() { return session_; }

    [[nodiscard]] uint32_t PlayerCount() const { return static_cast<uint32_t>(players_.size()); }
    [[nodiscard]] uint32_t ComputerNum() const;
    [[nodiscard]] bool IsComputerAt(const uint32_t index) const;
    [[nodiscard]] bool IsEliminatedAt(const uint32_t index) const;

  private:
    struct PlayerSlot
    {
        enum class State { ACTIVE, ELIMINATED, HOOKED };
        State state_{State::ACTIVE};
        bool is_computer_{false};
    };

    void SendPostFrame(lgtbot::ipc::PostResp::Channel channel, uint32_t target_pid,
                       std::vector<lgtbot::ipc::MsgItem> items);

    struct TimerCtl
    {
        void Start(IpcMatchEnv& env, const uint64_t sec, void* alert_arg, void(*alert_cb)(void*, uint64_t));
        void Stop(const IpcMatchEnv& env);

        std::shared_ptr<bool> timer_is_over_;
        std::unique_ptr<Timer> timer_;
    };

    ChildGameSession& session_;
    uint64_t match_id_{0};
    std::string game_name_;
    std::vector<std::string> player_names_;
    std::vector<std::string> player_avatars_;
    std::vector<PlayerSlot> players_;
    std::atomic<bool> is_in_deduction_{false};

    class IpcMsgSender final : public MsgSenderBase
    {
      public:
        IpcMsgSender(IpcMatchEnv& env, lgtbot::ipc::PostResp::Channel channel, const uint32_t target_pid)
            : env_(env)
            , channel_(channel)
            , target_pid_(target_pid)
        {}

        void SetMatch(const Match* const) override {}

      protected:
        void SaveText(const char* const data, const uint64_t len) override;
        void SaveUser(const UserID& id, const bool is_at) override;
        void SavePlayer(const PlayerID& id, const bool is_at) override;
        void SaveImage(const char* const path) override;
        void SaveMarkdown(const char* const markdown, const uint32_t width) override;
        void Flush() override;

      private:
        IpcMatchEnv& env_;
        lgtbot::ipc::PostResp::Channel channel_;
        uint32_t target_pid_;
        std::vector<lgtbot::ipc::MsgItem> items_;
    };

    std::unique_ptr<IpcMsgSender> broadcast_sender_;
    std::unique_ptr<IpcMsgSender> group_sender_;
    std::unique_ptr<IpcMsgSender> ai_sender_;
    std::map<PlayerID, std::unique_ptr<IpcMsgSender>> tell_senders_;

    TimerCtl timer_cntl_;
};
