#pragma once

#include <cstdio>
#include <memory>
#include <mutex>
#include <string>

#include "bot_core/game_handle.h"
#include "game_framework/game_main.h"
#include "match_process/ipc_match_env.h"
#include "match_process/match_ipc.pb.h"

#ifdef _WIN32
#define GAME_DYNLIB_SUFFIX ".dll"
#elif defined(__APPLE__)
#define GAME_DYNLIB_SUFFIX ".dylib"
#else
#define GAME_DYNLIB_SUFFIX ".so"
#endif

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
using DynModule = HMODULE;
#else
using DynModule = void*;
#endif

class IpcMatchEnv;

// Owns dlopen'd game, main stage, and IPC loop for one match in the child process.
class ChildGameSession
{
    friend class IpcMatchEnv;

  public:
    ChildGameSession(FILE* in, FILE* out);
    ~ChildGameSession();

    ChildGameSession(const ChildGameSession&) = delete;
    ChildGameSession& operator=(const ChildGameSession&) = delete;

    int RunLoop();

    [[nodiscard]] bool LoadModule(const std::string& lib_path, std::string& err);

    void SendProto(const lgtbot::ipc::GameResponse& resp);

    [[nodiscard]] std::mutex& write_mutex() { return write_mutex_; }
    [[nodiscard]] FILE* out() const { return out_; }

    void Routine();
    [[nodiscard]] lgtbot::game::MainStageBase* main_stage() const { return main_stage_.get(); }

  private:
    using init_options_command_handler =
            lgtbot::game::InitOptionsResult(*)(const char*, lgtbot::game::GameOptionsBase*,
                                               lgtbot::game::MutableGenericOptions*);

    struct ModuleFns
    {
        DynModule mod_{};
        GameHandle::game_options_allocator alloc_opt_{};
        GameHandle::game_options_deleter del_opt_{};
        GameHandle::main_stage_allocator alloc_stage_{};
        GameHandle::main_stage_deleter del_stage_{};
        init_options_command_handler init_options_{};
    };

    bool HandleInit(const lgtbot::ipc::InitReq& req, std::string& err);
    bool HandleSetOption(const lgtbot::ipc::SetOptionReq& req, std::string& err);
    bool HandleApplyInitOptions(const lgtbot::ipc::ApplyInitOptionsReq& req, std::string& err);
    bool HandleStart(const lgtbot::ipc::StartReq& req, std::string& err);
    bool HandleExecute(const lgtbot::ipc::ExecuteReq& req, std::string& err);
    bool HandleLeave(const lgtbot::ipc::LeaveReq& req, std::string& err);
    bool HandleHelp(const lgtbot::ipc::HelpReq& req, std::string& err);
    void SendGameOver();
    void DrainAfterStageWork();

    FILE* const in_;
    FILE* const out_;
    std::mutex write_mutex_;

    std::string game_title_;
    ModuleFns module_;
    GameHandle::game_options_ptr game_options_{nullptr, [](const lgtbot::game::GameOptionsBase*) {}};
    std::string resource_dir_;
    std::string saved_image_dir_;
    lgtbot::game::GenericOptions generic_options_{};
    GameHandle::main_stage_ptr main_stage_{nullptr, nullptr};
    std::unique_ptr<IpcMatchEnv> env_;
};
