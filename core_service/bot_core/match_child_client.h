// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <condition_variable>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "bot_core/bot_core.h"
#include "bot_core/id.h"
#include "game_framework/game_main.h"
#include "match_process/match_ipc.pb.h"
#include "bot_core/subprocess.h"
#include "nlohmann/json.hpp"

class Match;
class MsgSender;

// ---------------------------------------------------------------------------
// Typed frame variants — hold proto message objects directly
// ---------------------------------------------------------------------------

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
struct AckFrame {
    bool ok;
};
// A "push" frame is one the subprocess sends spontaneously (not in reply to a request).
using PushFrame = std::variant<PostFrame, PlayerStateFrame, GameOverFrame>;

// A "response" frame arrives in direct reply to a request written by the parent.
using ResponseFrame = std::variant<ReplyFrame, ResultFrame, AckFrame>;

// Union of all frames.
using ChildFrame = std::variant<PostFrame, PlayerStateFrame, GameOverFrame,
                                ReplyFrame, ResultFrame, AckFrame>;

// ---------------------------------------------------------------------------
// Interface for push-frame callbacks from the read loop.
// Called on the read thread — implementations must take their own lock if
// needed, but must NOT hold MatchChildClient's internal pending_mutex_.
// ---------------------------------------------------------------------------
struct IMatchChildCallbacks {
    virtual ~IMatchChildCallbacks() = default;
    virtual void OnPost(const PostFrame&) = 0;
    virtual void OnPlayerState(const PlayerStateFrame&) = 0;
    virtual void OnGameOver(const GameOverFrame&) = 0;
    // unexpected=true when the subprocess died without being asked to stop.
    virtual void OnEof(bool unexpected) = 0;
};

// ---------------------------------------------------------------------------
// MatchChildClient
//
// Owns the game subprocess (match_game_runner) and the stdin/stdout IPC channel.
//
// Read-thread model:
//   The caller (Match) is responsible for running the read loop:
//
//     game_child_ = make_unique<MatchChildClient>(...);
//     // ... SendInit / SendSetOption / SendStart ...
//     read_thread_ = std::thread([&]{ game_child_->RunReadLoop(cbs); });
//
//   To stop cleanly (without holding the Match mutex):
//     game_child_->SignalStop();   // wakes WaitForResponse_, signals child
//     read_thread_.join();
//     game_child_.reset();
//
// Concurrency:
//   SendExecute / SendLeave / FetchHelp write stdin and then block in
//   WaitForResponse_(). They must be called WITHOUT holding any lock that the
//   push-frame callbacks also need (otherwise deadlock: the callback blocks on
//   the mutex while the caller blocks in WaitForResponse_).
// ---------------------------------------------------------------------------
class MatchChildClient
{
  public:
    MatchChildClient(std::filesystem::path runner_exe, std::filesystem::path game_library);
    ~MatchChildClient();

    MatchChildClient(const MatchChildClient&) = delete;
    MatchChildClient& operator=(const MatchChildClient&) = delete;

    [[nodiscard]] bool ok() const { return static_cast<bool>(proc_); }

    // --- Pre-game synchronous calls (no read thread yet) ---

    [[nodiscard]] bool SendInit(const std::string& resource_dir, const std::string& saved_image_dir,
                                bool public_timer_alert, uint32_t bench, uint32_t is_formal);

    [[nodiscard]] bool SendSetOption(const std::string& text);

    // Sends "start", reads until "ack".  Push frames collected during start are
    // returned in push_frames_out for the caller to dispatch while it still holds
    // its own lock.  Call RunReadLoop() in a separate thread after this returns true.
    [[nodiscard]] bool SendStart(uint64_t match_id, uint32_t user_num,
                                 const std::vector<lgtbot::ipc::PlayerInfo>& players,
                                 std::vector<PushFrame>& push_frames_out);

    // --- Read loop: run in a dedicated thread owned by Match ---

    // Blocking loop. Reads frames from the subprocess stdout until EOF or
    // SignalStop() is called.  Push frames are dispatched via cbs.
    // Response frames wake any thread blocked in WaitForResponse_().
    void RunReadLoop(IMatchChildCallbacks& cbs);

    // Signal the read loop to stop:
    //   1. Wakes any thread blocked in WaitForResponse_() (returns nullopt).
    //   2. Sends SIGTERM to the child so the pipe EOF unblocks ReadFrame.
    // Does NOT join the read thread — the caller does that.
    void SignalStop();

    // Close the write end of the stdin pipe so the subprocess gets EOF and exits.
    // The subprocess will close its stdout, causing ReadFrame in the read thread to return
    // false (EOF) and the read loop to exit.  Safe even if the subprocess is already dead.
    // Use this for clean game-over teardown.
    void CloseInput();

    // --- Post-game-start calls (call without holding the Match mutex) ---

    [[nodiscard]] ErrCode SendExecute(PlayerID player_id, bool is_public, const std::string& text,
                                      std::function<void(const lgtbot::ipc::ReplyResp&)> reply_cb);

    [[nodiscard]] bool SendLeave(PlayerID player_id);

    [[nodiscard]] bool FetchHelp(bool text_mode, std::string& text_out);

  private:
    static std::optional<ChildFrame> ParseFrame(const std::string& raw);
    [[nodiscard]] bool WriteProto(const lgtbot::ipc::GameRequest& req);
    void SetPendingFrame_(ResponseFrame frame);
    std::optional<ResponseFrame> WaitForResponse_();

    std::unique_ptr<Subprocess> proc_;
    FILE* child_in_{nullptr};
    FILE* child_out_{nullptr};

    std::mutex pending_mutex_;
    std::condition_variable pending_cv_;
    std::optional<ResponseFrame> pending_frame_;
    bool pending_ready_{false};
    bool stopped_{false};
};
