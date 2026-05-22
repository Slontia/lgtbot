// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "match_process/child_config_session.h"

#include <fstream>

#include "match_process/ipc_frame.h"
#include "match_process/match_ipc.pb.h"
#include "utility/log.h"
#include "nlohmann/json.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

ChildConfigSession::ChildConfigSession(FILE* const in, FILE* const out)
    : in_(in)
    , out_(out)
{}

ChildConfigSession::~ChildConfigSession()
{
#ifdef _WIN32
    if (module_.mod_) {
        FreeLibrary(module_.mod_);
    }
#else
    if (module_.mod_) {
        dlclose(module_.mod_);
    }
#endif
}

bool ChildConfigSession::LoadModule(const std::string& lib_path, const std::string& conf_path, std::string& error_out)
{
#ifdef _WIN32
    const HMODULE mod = LoadLibraryA(lib_path.c_str());
#else
    void* const mod = dlopen(lib_path.c_str(), RTLD_NOW);
#endif
    if (!mod) {
#if defined(__linux__) || defined(__APPLE__)
        // dlerror() must be called at most once per failure — a second call may return nullptr.
        const char* const err = dlerror();
        error_out = err ? err : "dlopen failed";
#else
        error_out = "LoadLibrary failed";
#endif
        return false;
    }
    module_.mod_ = mod;
    const auto sym = [&mod, &error_out](const char* const name) -> void*
        {
#ifdef _WIN32
            void* const p = reinterpret_cast<void*>(GetProcAddress(mod, name));
#else
            void* const p = dlsym(mod, name);
#endif
            if (!p) {
                error_out = std::string("missing symbol ") + name;
            }
            return p;
        };
    error_out.clear();
    module_.alloc_opt_ = reinterpret_cast<game_options_allocator>(sym("NewGameOptions"));
    if (!error_out.empty()) { return false; }
    module_.del_opt_ = reinterpret_cast<game_options_deleter>(sym("DeleteGameOptions"));
    if (!error_out.empty()) { return false; }
    module_.max_player_num_ = reinterpret_cast<max_player_num_handler>(sym("MaxPlayerNum"));
    if (!error_out.empty()) { return false; }
    module_.multiple_ = reinterpret_cast<multiple_handler>(sym("Multiple"));
    if (!error_out.empty()) { return false; }
    module_.init_options_ = reinterpret_cast<init_options_command_handler>(sym("HandleInitOptionsCommand"));
    if (!error_out.empty()) { return false; }
    module_.handle_rule_command_ = reinterpret_cast<rule_command_handler>(sym("HandleRuleCommand"));
    if (!error_out.empty()) { return false; }
    module_.get_game_info_ = reinterpret_cast<get_game_info_handler>(sym("GetGameInfo"));
    if (!error_out.empty()) { return false; }

    default_options_ = game_options_ptr(module_.alloc_opt_(), module_.del_opt_);
    if (!default_options_) {
        error_out = "NewGameOptions returned null";
        return false;
    }

    // Seed default_is_formal_ from the game's compile-time k_default_generic_options
    // (mygame.cc declares e.g. `.is_formal_ = false`). Without this, the static
    // declaration was silently ignored and matches always defaulted to formal.
    {
        lgtbot::game::GameInfo game_info{};
        module_.get_game_info_(&game_info);
        default_is_formal_ = game_info.default_generic_options_.is_formal_;
    }

    // Restore persisted default options from config file
    if (!conf_path.empty()) {
        std::ifstream f(conf_path);
        if (f) {
            try {
                nlohmann::json j;
                f >> j;
                for (const auto& [opt_name, value] : j["options"].items()) {
                    const std::string opt_str = opt_name + " " + value.get<std::string>();
                    default_options_->SetOption(opt_str.c_str());
                }
            } catch (...) {
                // Ignore config parse errors; proceed with defaults
            }
        }
    }

    return true;
}

void ChildConfigSession::SendProto(const lgtbot::ipc::ConfigResponse& resp)
{
    std::string buf;
    if (!resp.SerializeToString(&buf)) {
        ErrorLog() << "ConfigResponse::SerializeToString failed";
        return;
    }
    if (!WriteFrame(out_, buf)) {
        ErrorLog() << "ChildConfigSession::SendProto WriteFrame failed";
    }
}

lgtbot::ipc::ConfigResponse ChildConfigSession::MakeOptionUpdateResp(const bool ok) const
{
    lgtbot::ipc::ConfigResponse resp;
    auto* upd = resp.mutable_option_update();
    upd->set_ok(ok);
    if (default_options_) {
        upd->set_max_player(module_.max_player_num_(default_options_.get()));
        upd->set_multiple(module_.multiple_(default_options_.get()));
    }
    return resp;
}

bool ChildConfigSession::HandleQueryOptionInfo(const lgtbot::ipc::QueryOptionInfoReq& req)
{
    lgtbot::ipc::ConfigResponse resp;
    auto* info = resp.mutable_option_info();
    info->set_ok(true);
    if (default_options_) {
        info->set_text(default_options_->Info(true, !req.text_mode()));
    }
    SendProto(resp);
    return true;
}

bool ChildConfigSession::HandleQueryMatchOptionInfo(const lgtbot::ipc::QueryMatchOptionInfoReq& req)
{
    lgtbot::ipc::ConfigResponse resp;
    auto* info = resp.mutable_option_info();
    game_options_ptr opts(module_.alloc_opt_(), module_.del_opt_);
    if (!opts) {
        info->set_ok(false);
        SendProto(resp);
        return true;
    }
    for (const auto& opt : req.applied_options_log()) {
        opts->SetOption(opt.c_str());
    }
    if (!req.init_options_args().empty() && module_.init_options_) {
        lgtbot::game::MutableGenericOptions generic_options{};
        generic_options.is_formal_ = default_is_formal_;
        // The result is intentionally ignored: invalid init args were already
        // rejected at NewMatch time, so anything we receive here was previously
        // accepted and is safe to re-apply.
        module_.init_options_(req.init_options_args().c_str(), opts.get(), &generic_options);
    }
    info->set_ok(true);
    info->set_text(opts->Info(true, !req.text_mode()));
    SendProto(resp);
    return true;
}

bool ChildConfigSession::HandleSetDefaultOption(const lgtbot::ipc::SetDefaultOptionReq& req)
{
    const bool ok = default_options_ && default_options_->SetOption(req.text().c_str());
    if (ok) {
        applied_options_log_.push_back(req.text());
    }
    SendProto(MakeOptionUpdateResp(ok));
    return true;
}

bool ChildConfigSession::HandleTryMatchOption(const lgtbot::ipc::TryMatchOptionReq& req)
{
    lgtbot::ipc::ConfigResponse resp;
    auto* upd = resp.mutable_option_update();
    game_options_ptr opts(module_.alloc_opt_(), module_.del_opt_);
    if (!opts) {
        upd->set_ok(false);
        SendProto(resp);
        return true;
    }
    for (const auto& opt : req.applied_options_log()) {
        opts->SetOption(opt.c_str());
    }
    const bool ok = opts->SetOption(req.text().c_str());
    upd->set_ok(ok);
    if (ok) {
        upd->set_max_player(module_.max_player_num_(opts.get()));
        upd->set_multiple(module_.multiple_(opts.get()));
    }
    SendProto(resp);
    return true;
}

bool ChildConfigSession::HandleSetFormal(const lgtbot::ipc::SetFormalReq& req)
{
    default_is_formal_ = req.value();
    lgtbot::ipc::ConfigResponse resp;
    resp.mutable_ack()->set_ok(true);
    SendProto(resp);
    return true;
}

bool ChildConfigSession::HandleQueryFormal()
{
    lgtbot::ipc::ConfigResponse resp;
    auto* f = resp.mutable_formal();
    f->set_ok(true);
    f->set_is_formal(default_is_formal_);
    SendProto(resp);
    return true;
}

bool ChildConfigSession::HandleGetAppliedLog()
{
    lgtbot::ipc::ConfigResponse resp;
    auto* log = resp.mutable_applied_log();
    log->set_ok(true);
    for (const auto& entry : applied_options_log_) {
        log->add_log(entry);
    }
    SendProto(resp);
    return true;
}

bool ChildConfigSession::HandleInitOptions(const lgtbot::ipc::InitOptionsReq& req)
{
    // Create a fresh options object, then replay all default option settings
    // so that init_options sees the current default configuration.
    game_options_ptr opts(module_.alloc_opt_(), module_.del_opt_);
    if (!opts) {
        lgtbot::ipc::ConfigResponse resp;
        auto* r = resp.mutable_init_options();
        r->set_ok(false);
        r->set_start_mode(lgtbot::ipc::InitOptionsResp::INVALID);
        SendProto(resp);
        return true;
    }
    for (const auto& opt : applied_options_log_) {
        opts->SetOption(opt.c_str());
    }

    lgtbot::game::MutableGenericOptions generic_options{};
    generic_options.is_formal_ = default_is_formal_;
    const auto result = module_.init_options_(req.args().c_str(), opts.get(), &generic_options);

    lgtbot::ipc::ConfigResponse resp;
    auto* r = resp.mutable_init_options();
    if (result == lgtbot::game::INVALID_INIT_OPTIONS_COMMAND) {
        r->set_ok(false);
        r->set_start_mode(lgtbot::ipc::InitOptionsResp::INVALID);
    } else {
        r->set_ok(true);
        r->set_start_mode(result == lgtbot::game::NEW_SINGLE_USER_MODE_GAME
                              ? lgtbot::ipc::InitOptionsResp::SINGLE
                              : lgtbot::ipc::InitOptionsResp::MULTIPLE);
        r->set_max_player(module_.max_player_num_(opts.get()));
        r->set_multiple(module_.multiple_(opts.get()));
        r->set_bench(generic_options.bench_computers_to_player_num_);
        r->set_is_formal(static_cast<uint32_t>(generic_options.is_formal_));
    }
    SendProto(resp);
    return true;
}

bool ChildConfigSession::HandleRuleCommand(const lgtbot::ipc::HandleRuleCommandReq& req)
{
    lgtbot::ipc::ConfigResponse resp;
    auto* r = resp.mutable_rule_command();
    const char* const result = module_.handle_rule_command_ ? module_.handle_rule_command_(req.args().c_str()) : nullptr;
    r->set_ok(result != nullptr);
    if (result) {
        r->set_text(result);
    }
    SendProto(resp);
    return true;
}

int ChildConfigSession::RunLoop()
{
    for (;;) {
        std::string raw;
        if (!ReadFrame(in_, raw)) {
            return 0;
        }
        lgtbot::ipc::ConfigRequest req;
        if (!req.ParseFromString(raw)) {
            lgtbot::ipc::ConfigResponse err_resp;
            err_resp.mutable_ack()->set_ok(false);
            SendProto(err_resp);
            continue;
        }
        switch (req.req_case()) {
        case lgtbot::ipc::ConfigRequest::kShutdown:
            return 0;
        case lgtbot::ipc::ConfigRequest::kQueryOptionInfo:
            HandleQueryOptionInfo(req.query_option_info());
            break;
        case lgtbot::ipc::ConfigRequest::kQueryMatchOptionInfo:
            HandleQueryMatchOptionInfo(req.query_match_option_info());
            break;
        case lgtbot::ipc::ConfigRequest::kSetDefaultOption:
            HandleSetDefaultOption(req.set_default_option());
            break;
        case lgtbot::ipc::ConfigRequest::kTryMatchOption:
            HandleTryMatchOption(req.try_match_option());
            break;
        case lgtbot::ipc::ConfigRequest::kSetFormal:
            HandleSetFormal(req.set_formal());
            break;
        case lgtbot::ipc::ConfigRequest::kQueryFormal:
            HandleQueryFormal();
            break;
        case lgtbot::ipc::ConfigRequest::kGetAppliedLog:
            HandleGetAppliedLog();
            break;
        case lgtbot::ipc::ConfigRequest::kInitOptions:
            HandleInitOptions(req.init_options());
            break;
        case lgtbot::ipc::ConfigRequest::kHandleRuleCommand:
            HandleRuleCommand(req.handle_rule_command());
            break;
        default: {
            lgtbot::ipc::ConfigResponse err_resp;
            err_resp.mutable_ack()->set_ok(false);
            SendProto(err_resp);
            break;
        }
        }
    }
}
