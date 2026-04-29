// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include "bot_core/game_handle.h"
#include "game_framework/game_main.h"
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

// Owns dlopen'd game options and handles config-related IPC requests in the child process.
class ChildConfigSession
{
  public:
    ChildConfigSession(FILE* in, FILE* out);
    ~ChildConfigSession();

    ChildConfigSession(const ChildConfigSession&) = delete;
    ChildConfigSession& operator=(const ChildConfigSession&) = delete;

    [[nodiscard]] bool LoadModule(const std::string& lib_path, const std::string& conf_path, std::string& err);

    int RunLoop();

  private:
    using game_options_allocator = lgtbot::game::GameOptionsBase*(*)();
    using game_options_deleter = void(*)(const lgtbot::game::GameOptionsBase*);
    using max_player_num_handler = uint64_t(*)(const lgtbot::game::GameOptionsBase*);
    using multiple_handler = uint32_t(*)(const lgtbot::game::GameOptionsBase*);
    using init_options_command_handler = lgtbot::game::InitOptionsResult(*)(const char*, lgtbot::game::GameOptionsBase*, lgtbot::game::MutableGenericOptions*);
    using rule_command_handler = const char*(*)(const char*);

    struct ModuleFns
    {
        DynModule mod_{};
        game_options_allocator alloc_opt_{};
        game_options_deleter del_opt_{};
        max_player_num_handler max_player_num_{};
        multiple_handler multiple_{};
        init_options_command_handler init_options_{};
        rule_command_handler handle_rule_command_{};
    };

    using game_options_ptr = std::unique_ptr<lgtbot::game::GameOptionsBase,
                                             game_options_deleter>;

    void SendProto(const lgtbot::ipc::ConfigResponse& resp);

    bool HandleQueryOptionInfo(const lgtbot::ipc::QueryOptionInfoReq& req);
    bool HandleSetDefaultOption(const lgtbot::ipc::SetDefaultOptionReq& req);
    bool HandleSetFormal(const lgtbot::ipc::SetFormalReq& req);
    bool HandleQueryFormal();
    bool HandleGetAppliedLog();
    bool HandleInitOptions(const lgtbot::ipc::InitOptionsReq& req);
    bool HandleRuleCommand(const lgtbot::ipc::HandleRuleCommandReq& req);

    lgtbot::ipc::ConfigResponse MakeOptionUpdateResp(bool ok) const;

    FILE* const in_;
    FILE* const out_;

    ModuleFns module_;
    game_options_ptr default_options_{nullptr, [](const lgtbot::game::GameOptionsBase*) {}};
    std::vector<std::string> applied_options_log_; // Options set via set_default_option
    bool default_is_formal_{true}; // Tracks the formal/informal default set via set_formal
};
