// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "bot_core/bot_core.h"
#include "bot_core/id.h"
#include "bot_core/msg_sender.h"
#include "game_framework/game_main.h"
#include "match_process/match_ipc.pb.h"
#include "bot_core/subprocess.h"

namespace lgtbot::core {
class BotCtx;
class GameHandle;
} // namespace lgtbot::core

namespace lgtbot {
namespace core {
namespace match {

class Match;
struct ChildMessageHandler;

using PushFrame = std::variant<
    std::reference_wrapper<const lgtbot::ipc::PostResp>,
    std::reference_wrapper<const lgtbot::ipc::PlayerStateResp>,
    std::reference_wrapper<const lgtbot::ipc::GameOverResp>,
    std::reference_wrapper<const lgtbot::ipc::TimerStartResp>,
    std::reference_wrapper<const lgtbot::ipc::TimerStopResp>
>;
using PushHandler = std::function<void(const PushFrame&)>;

struct ChildMessageHandler
{
    virtual ~ChildMessageHandler() = default;
    virtual void HandleText(const std::string& text) = 0;
    virtual void HandleAtPlayerId(PlayerID pid) = 0;
    virtual void HandleUserId(UserID uid) = 0;
    virtual void HandleImagePath(const std::string& path) = 0;
    virtual void HandleMarkdown(const std::string& text, uint32_t width) = 0;
};

struct DoNothingHandler final : ChildMessageHandler
{
    static DoNothingHandler& Get();
    void HandleText(const std::string&) override {}
    void HandleAtPlayerId(PlayerID) override {}
    void HandleUserId(UserID) override {}
    void HandleImagePath(const std::string&) override {}
    void HandleMarkdown(const std::string&, uint32_t) override {}
};

class MatchRunning;

class ChildMessageReplyHandler final : public ChildMessageHandler
{
  public:
    ChildMessageReplyHandler(HostMsgSenderBase& sender, const MatchRunning* running);
    void HandleText(const std::string& text) override;
    void HandleAtPlayerId(PlayerID pid) override;
    void HandleUserId(UserID uid) override;
    void HandleImagePath(const std::string& path) override;
    void HandleMarkdown(const std::string& text, uint32_t width) override;

  private:
    HostMsgSenderBase& sender_;
    std::optional<HostMsgSenderBase::MsgSenderGuard> g_;
    const MatchRunning* running_;

    HostMsgSenderBase::MsgSenderGuard& Guard();
};

class ExtractTextHandler final : public ChildMessageHandler
{
  public:
    explicit ExtractTextHandler(std::string& out);
    void HandleText(const std::string& text) override;
    void HandleAtPlayerId(PlayerID) override {}
    void HandleUserId(UserID) override {}
    void HandleImagePath(const std::string&) override {}
    void HandleMarkdown(const std::string&, uint32_t) override {}

  private:
    std::string& out_;
};

void AppendMsgItem(ChildMessageHandler& handler, const lgtbot::ipc::MsgItem& item);

std::filesystem::path ResolveRunnerExe();

std::filesystem::path GameLibraryPath(const BotCtx& bot, const GameHandle& gh);

class MatchChildClient
{
  public:
    using IpcStage = lgtbot::ipc::ResultResp::Stage;

    struct RuntimeOptions
    {
        struct ResourceHolder
        {
            std::string resource_dir_;
            std::string saved_image_dir_;
        };

        ResourceHolder resource_holder_;
        lgtbot::game::GenericOptions generic_options_;
        bool public_timer_alert_{false};
    };

    ~MatchChildClient();

    MatchChildClient(const MatchChildClient&) = delete;
    MatchChildClient& operator=(const MatchChildClient&) = delete;

    [[nodiscard]] std::optional<IpcStage> SendSetOption(const std::string& text);

    // All Send* below accept a PushHandler that the child's push frames are routed
    // through — typically constructed by MakeCallback(MatchRunning&), or by
    // MakeCallback(std::function<MatchRunning&()>) at the SendStart caller's site, where
    // MatchRunning is constructed lazily on the first push frame via the get_running
    // callback (which transitions the phase from MatchLobby and emplaces MatchRunning).
    [[nodiscard]] std::optional<IpcStage> SendStart(uint64_t match_id, uint32_t user_num,
                                                    const std::vector<lgtbot::ipc::PlayerInfo>& players,
                                                    uint32_t bench, bool is_formal,
                                                    const PushHandler& on_push);

    // SendExecute additionally takes a reply handler because ExecuteReq's response
    // may carry a Reply frame that must be delivered synchronously to the caller.
    [[nodiscard]] std::optional<ErrCode> SendExecute(PlayerID player_id, bool is_public,
                                                     const std::string& text,
                                                     ChildMessageHandler& reply_handler,
                                                     const PushHandler& on_push);

    [[nodiscard]] std::optional<IpcStage> SendLeave(PlayerID player_id, const PushHandler& on_push);

    [[nodiscard]] std::optional<IpcStage> SendTimeout(const PushHandler& on_push);

    [[nodiscard]] std::optional<IpcStage> SendAlert(uint64_t remaining_sec, const PushHandler& on_push);

    [[nodiscard]] std::optional<IpcStage> FetchHelp(bool text_mode, std::string& text_out);

    // Idempotent, thread-safe. Interrupts any in-flight Send*_ by terminating the child
    // subprocess without waiting. Safe to call from any thread even while another thread
    // is blocked in a Send*_ call; that call will return an error and its caller must
    // proceed with cleanup.
    void Cancel() noexcept { proc_.Cancel(); }

  private:
    friend std::unique_ptr<MatchChildClient> MakeMatchChildClient(std::filesystem::path runner_exe,
                                                                  std::filesystem::path game_library,
                                                                  const RuntimeOptions& options);

    explicit MatchChildClient(Subprocess proc);

    [[nodiscard]] std::optional<IpcStage> SendInit_(const RuntimeOptions& options);

    // Core: write request, then read frames until ResultFrame. Returns the result stage.
    // Push frames are dispatched to on_push. Reply frames are delivered via handler.
    [[nodiscard]] std::optional<IpcStage> SendRequestAndRead_(lgtbot::ipc::GameRequest req,
                                                              ChildMessageHandler& handler,
                                                              const PushHandler& on_push);

    [[nodiscard]] bool WriteProto_(lgtbot::ipc::GameRequest req);

    Subprocess proc_;
    std::mutex write_mutex_;
};

[[nodiscard]] std::unique_ptr<MatchChildClient> MakeMatchChildClient(std::filesystem::path runner_exe,
                                                                     std::filesystem::path game_library,
                                                                     const MatchChildClient::RuntimeOptions& options);

} // namespace match
} // namespace core
} // namespace lgtbot
