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

class Match;

struct PostFrame {
    lgtbot::ipc::PostResp post;
};
struct PlayerStateFrame {
    PlayerID pid;
    std::string state;
};
struct GameOverFrame {
    lgtbot::ipc::GameOverResp game_over;
};
struct ReplyFrame {
    lgtbot::ipc::ReplyResp reply;
};
struct ResultFrame {
    lgtbot::ipc::ResultResp::Stage stage;
};

using PushFrame = std::variant<PostFrame, PlayerStateFrame, GameOverFrame>;
using PushHandler = std::function<void(const PushFrame&)>;

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

    [[nodiscard]] std::optional<IpcStage> SendStart(uint64_t match_id, uint32_t user_num,
                                                    const std::vector<lgtbot::ipc::PlayerInfo>& players,
                                                    const PushHandler& on_push);

    [[nodiscard]] std::optional<ErrCode> SendExecute(PlayerID player_id, bool is_public,
                                                     const std::string& text, MsgSender& reply,
                                                     const PushHandler& on_push);

    [[nodiscard]] std::optional<IpcStage> SendLeave(PlayerID player_id, const PushHandler& on_push);

    [[nodiscard]] std::optional<IpcStage> FetchHelp(bool text_mode, MsgSenderBase& reply_sender);

  private:
    friend std::unique_ptr<MatchChildClient> MakeMatchChildClient(std::filesystem::path runner_exe,
                                                                  std::filesystem::path game_library,
                                                                  const RuntimeOptions& options);

    explicit MatchChildClient(Subprocess proc);

    [[nodiscard]] std::optional<IpcStage> SendInit_(const RuntimeOptions& options);

    // Core: write request, then read frames until ResultFrame. Returns the result stage.
    // Push frames are dispatched to on_push. Reply frames are sent to reply_sender.
    [[nodiscard]] std::optional<IpcStage> SendRequestAndRead_(lgtbot::ipc::GameRequest req,
                                                              MsgSenderBase& reply_sender,
                                                              const PushHandler& on_push);

    [[nodiscard]] bool WriteProto_(lgtbot::ipc::GameRequest req);

    Subprocess proc_;
    std::mutex write_mutex_;
};

[[nodiscard]] std::unique_ptr<MatchChildClient> MakeMatchChildClient(std::filesystem::path runner_exe,
                                                                     std::filesystem::path game_library,
                                                                     const MatchChildClient::RuntimeOptions& options);