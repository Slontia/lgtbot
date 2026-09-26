// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <atomic>
#include <filesystem>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#include "bot_core/bot_core.h"
#include "bot_core/id.h"
#include "bot_core/msg_sender.h"
#include "game_framework/game_main.h"
#include "match_process/match_ipc.pb.h"
#include "bot_core/subprocess.h"
#include "utility/lock_wrapper.h"

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
    uint64_t ipc_id;
    lgtbot::ipc::ReplyResp reply;
};
struct ResultFrame {
    uint64_t ipc_id;
    lgtbot::ipc::ResultResp::Stage stage;
};

using PushFrame = std::variant<PostFrame, PlayerStateFrame, GameOverFrame>;
using ResponseFrame = std::variant<ReplyFrame, ResultFrame>;
using ChildFrame = std::variant<PostFrame, PlayerStateFrame, GameOverFrame, ReplyFrame, ResultFrame>;

using ChildIpcPushHandler = std::function<void(PushFrame)>;
using ChildIpcEofHandler = std::function<void(bool unexpected)>;

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

    [[nodiscard]] std::optional<std::future<IpcStage>> SendSetOption(const std::string& text);

    [[nodiscard]] std::optional<std::future<IpcStage>> SendStart(uint64_t match_id, uint32_t user_num,
                                                                 const std::vector<lgtbot::ipc::PlayerInfo>& players);

    [[nodiscard]] std::optional<std::future<ErrCode>> SendExecute(PlayerID player_id, bool is_public,
                                                                  const std::string& text, MsgSender& reply);

    [[nodiscard]] std::optional<std::future<IpcStage>> SendLeave(PlayerID player_id);

    // Apply preset commands (init_options from "#新游戏 <game> <args>") to the subprocess game options.
    // The future resolves to STAGE_OK if the args matched a preset command.
    [[nodiscard]] std::optional<std::future<IpcStage>> SendApplyInitOptions(const std::string& args);

    [[nodiscard]] std::optional<std::future<IpcStage>> FetchHelp(bool text_mode, MsgSenderBase& reply_sender);

  private:
    friend std::unique_ptr<MatchChildClient> MakeMatchChildClient(std::filesystem::path runner_exe,
                                                                  std::filesystem::path game_library,
                                                                  const RuntimeOptions& options,
                                                                  const ChildIpcPushHandler& dispatch_push,
                                                                  const ChildIpcEofHandler& on_eof);

    MatchChildClient(Subprocess proc, const ChildIpcPushHandler& dispatch_push, const ChildIpcEofHandler& on_eof);

    class PendingRequests
    {
      public:
        struct Entry {
            MsgSenderBase& reply_sender;
            std::function<void(IpcStage)> on_result;
        };

        PendingRequests() = default;
        ~PendingRequests();

        PendingRequests(const PendingRequests&) = delete;
        PendingRequests& operator=(const PendingRequests&) = delete;

        void Emplace(uint64_t ipc_id, Entry entry);
        void DispatchReply(uint64_t ipc_id, const lgtbot::ipc::ReplyResp& reply);
        void DispatchResult(uint64_t ipc_id, IpcStage stage);

      private:
        using EntryIt = std::map<uint64_t, Entry>::iterator;

        [[nodiscard]] std::optional<EntryIt> FindEntry_(uint64_t ipc_id);

        std::map<uint64_t, Entry> entries_;
    };

    [[nodiscard]] std::optional<std::future<IpcStage>> SendInit_(const RuntimeOptions& options);
    template<typename T, typename Handler>
    [[nodiscard]] std::optional<std::future<T>> SendIpc_(lgtbot::ipc::GameRequest&& req, MsgSenderBase& reply_sender,
                                                         Handler handler);
    [[nodiscard]] std::optional<std::future<IpcStage>> SendIpcStage_(lgtbot::ipc::GameRequest&& req,
                                                                     MsgSenderBase& reply_sender);
    [[nodiscard]] std::optional<std::future<ErrCode>> SendIpcErrCode_(lgtbot::ipc::GameRequest&& req,
                                                                      MsgSenderBase& reply_sender);
    [[nodiscard]] bool WriteProto_(lgtbot::ipc::GameRequest req);
    uint64_t AllocIpcId_();

    void RunReadLoop_(std::stop_token stop, const ChildIpcPushHandler& dispatch_push,
                      const ChildIpcEofHandler& on_eof);

    // Member declaration order controls ~MatchChildClient teardown (reverse order).
    // proc_ is declared first so it is destroyed last; the read thread can use proc_ until it has been joined.
    Subprocess proc_;
    std::mutex request_mutex_;
    mutex_protect_wrapper<PendingRequests> pending_;
    std::atomic<uint64_t> next_ipc_id_{1};
    std::jthread read_thread_;
};

[[nodiscard]] std::unique_ptr<MatchChildClient> MakeMatchChildClient(std::filesystem::path runner_exe,
                                                                     std::filesystem::path game_library,
                                                                     const MatchChildClient::RuntimeOptions& options,
                                                                     const ChildIpcPushHandler& dispatch_push,
                                                                     const ChildIpcEofHandler& on_eof);
