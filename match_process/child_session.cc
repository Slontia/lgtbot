#include "match_process/child_session.h"

#include <stdexcept>

#include "match_process/ipc_frame.h"
#include "match_process/match_ipc.pb.h"
#include "utility/log.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace {

class ErrCollector final : public MsgSenderBase
{
  public:
    void SetMatch(const Match* const) override {}

  protected:
    void SaveText(const char* const data, const uint64_t len) override { text_.append(data, data + len); }
    void SaveUser(const UserID&, const bool) override {}
    void SavePlayer(const PlayerID&, const bool) override {}
    void SaveImage(const char* const) override {}
    void SaveMarkdown(const char* const, const uint32_t) override {}
    void Flush() override {}

  public:
    std::string text_;
};

class ReplySender final : public MsgSenderBase
{
  public:
    explicit ReplySender(ChildGameSession& session)
        : session_(session)
    {}

    void SetMatch(const Match* const) override {}

  protected:
    void SaveText(const char* const data, const uint64_t len) override
    {
        auto* item = reply_.add_items();
        item->set_text(std::string(data, data + len));
    }

    void SaveUser(const UserID& id, const bool /*is_at*/) override
    {
        auto* item = reply_.add_items();
        item->set_user_id(id.GetStr());
    }

    void SavePlayer(const PlayerID& id, const bool /*is_at*/) override
    {
        auto* item = reply_.add_items();
        item->set_at_player_id(id.Get());
    }

    void SaveImage(const char* const path) override
    {
        auto* item = reply_.add_items();
        item->set_image_path(path);
    }

    void SaveMarkdown(const char* const markdown, const uint32_t width) override
    {
        auto* item = reply_.add_items();
        auto* md = item->mutable_markdown();
        md->set_text(markdown);
        md->set_width(width);
    }

    void Flush() override
    {
        if (reply_.items_size() == 0) {
            return;
        }
        lgtbot::ipc::GameResponse resp;
        *resp.mutable_reply() = reply_;
        session_.SendProto(resp);
        reply_.Clear();
    }

  public:
    void flush_out() { Flush(); }

  private:
    ChildGameSession& session_;
    lgtbot::ipc::ReplyResp reply_;
};

static lgtbot::ipc::ResultResp::Stage StageErrToProto(const StageErrCode rc)
{
    using S = lgtbot::ipc::ResultResp;
    switch (rc) {
    case StageErrCode::OK:        return S::STAGE_OK;
    case StageErrCode::CHECKOUT:  return S::STAGE_CHECKOUT;
    case StageErrCode::FAILED:    return S::STAGE_FAILED;
    case StageErrCode::CONTINUE:  return S::STAGE_CONTINUE;
    case StageErrCode::NOT_FOUND: return S::STAGE_NOT_FOUND;
    default:                      return S::STAGE_UNKNOWN;
    }
}

} // namespace

ChildGameSession::ChildGameSession(FILE* const in, FILE* const out)
    : in_(in)
    , out_(out)
{}

ChildGameSession::~ChildGameSession()
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

bool ChildGameSession::LoadModule(const std::string& lib_path, std::string& error_out)
{
#ifdef _WIN32
    const HMODULE mod = LoadLibraryA(lib_path.c_str());
#else
    void* const mod = dlopen(lib_path.c_str(), RTLD_NOW);
#endif
    if (!mod) {
#if defined(__linux__) || defined(__APPLE__)
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
    module_.alloc_opt_ = reinterpret_cast<GameHandle::game_options_allocator>(sym("NewGameOptions"));
    if (!error_out.empty()) {
        return false;
    }
    module_.del_opt_ = reinterpret_cast<GameHandle::game_options_deleter>(sym("DeleteGameOptions"));
    if (!error_out.empty()) {
        return false;
    }
    module_.alloc_stage_ = reinterpret_cast<GameHandle::main_stage_allocator>(sym("NewMainStage"));
    if (!error_out.empty()) {
        return false;
    }
    module_.del_stage_ = reinterpret_cast<GameHandle::main_stage_deleter>(sym("DeleteMainStage"));
    if (!error_out.empty()) {
        return false;
    }
    const auto get_info = reinterpret_cast<void (*)(lgtbot::game::GameInfo*)>(sym("GetGameInfo"));
    if (!error_out.empty() || !get_info) {
        return false;
    }
    lgtbot::game::GameInfo info;
    get_info(&info);
    game_title_ = info.properties_ ? info.properties_->name_ : "game";
    return true;
}

void ChildGameSession::SendProto(const lgtbot::ipc::GameResponse& resp)
{
    std::string buf;
    if (!resp.SerializeToString(&buf)) {
        ErrorLog() << "GameResponse::SerializeToString failed";
        return;
    }
    const std::lock_guard lock(write_mutex_);
    if (!WriteFrame(out_, buf)) {
        ErrorLog() << "WriteFrame failed";
    }
}

void ChildGameSession::SendGameOver()
{
    if (!main_stage_ || !env_) {
        return;
    }
    lgtbot::ipc::GameResponse resp;
    auto* go = resp.mutable_game_over();
    for (uint32_t i = 0; i < env_->PlayerCount(); ++i) {
        const PlayerID pid{i};
        auto* entry = go->add_scores();
        entry->set_pid(i);
        entry->set_score(static_cast<int32_t>(main_stage_->PlayerScore(pid)));
        const char* const* ap = main_stage_->VerdictateAchievements(pid);
        if (ap) {
            for (; *ap; ++ap) {
                entry->add_achievements(*ap);
            }
        }
    }
    SendProto(resp);
}

void ChildGameSession::DrainAfterStageWork()
{
    SendGameOver();
    main_stage_.reset();
}

void ChildGameSession::Routine()
{
    if (!main_stage_ || !env_) {
        return;
    }
    if (main_stage_->IsOver()) {
        DrainAfterStageWork();
        return;
    }
    const uint64_t computer_num = env_->ComputerNum();
    uint64_t ok_count = 0;
    const uint32_t n = env_->PlayerCount();
    if (n == 0) {
        return;
    }
    for (uint64_t p = 0; !main_stage_->IsOver() && ok_count < computer_num; p = (p + 1) % n) {
        const uint32_t pi = static_cast<uint32_t>(p);
        if (!env_->IsComputerAt(pi)) {
            continue;
        }
        if (env_->IsEliminatedAt(pi) || StageErrCode::OK == main_stage_->HandleComputerAct(pi, false)) {
            ++ok_count;
        } else {
            ok_count = 0;
        }
    }
    if (main_stage_->IsOver()) {
        DrainAfterStageWork();
    }
}

bool ChildGameSession::HandleInit(const lgtbot::ipc::InitReq& req, std::string& err)
{
    resource_dir_ = req.resource_dir();
    saved_image_dir_ = req.saved_image_dir();
    game_options_ = GameHandle::game_options_ptr(module_.alloc_opt_(), module_.del_opt_);
    if (!game_options_) {
        err = "NewGameOptions returned null";
        lgtbot::ipc::GameResponse resp;
        resp.mutable_ack()->set_ok(false);
        SendProto(resp);
        return false;
    }
    lgtbot::game::ImmutableGenericOptions imm{};
    imm.public_timer_alert_ = req.public_timer_alert();
    imm.resource_dir_ = resource_dir_.c_str();
    imm.saved_image_dir_ = saved_image_dir_.c_str();
    imm.user_num_ = 0;
    lgtbot::game::MutableGenericOptions mut{};
    mut.bench_computers_to_player_num_ = req.bench();
    mut.is_formal_ = req.is_formal();
    generic_options_ = lgtbot::game::GenericOptions(imm, mut);
    lgtbot::ipc::GameResponse resp;
    resp.mutable_ack()->set_ok(true);
    SendProto(resp);
    return true;
}

bool ChildGameSession::HandleSetOption(const lgtbot::ipc::SetOptionReq& req, std::string& /*err*/)
{
    const bool ok = game_options_ && game_options_->SetOption(req.text().c_str());
    lgtbot::ipc::GameResponse resp;
    resp.mutable_ack()->set_ok(ok);
    SendProto(resp);
    return true;
}

bool ChildGameSession::HandleStart(const lgtbot::ipc::StartReq& req, std::string& err)
{
    std::vector<std::string> names;
    std::vector<std::string> avatars;
    std::vector<bool> computer;
    names.reserve(req.players_size());
    avatars.reserve(req.players_size());
    computer.reserve(req.players_size());
    for (const auto& p : req.players()) {
        const bool is_computer = p.computer();
        computer.push_back(is_computer);
        if (is_computer) {
            names.push_back("机器人" + std::to_string(p.computer_id()) + "号");
            avatars.push_back("");
        } else {
            names.push_back(p.display_name());
            avatars.push_back(p.avatar());
        }
    }
    env_ = std::make_unique<IpcMatchEnv>(*this);
    env_->SetMeta(req.match_id(), game_title_, std::move(names), std::move(avatars), std::move(computer));

    lgtbot::game::ImmutableGenericOptions imm{};
    imm.public_timer_alert_ = generic_options_.public_timer_alert_;
    imm.user_num_ = req.user_num();
    imm.resource_dir_ = resource_dir_.c_str();
    imm.saved_image_dir_ = saved_image_dir_.c_str();
    lgtbot::game::MutableGenericOptions mut{};
    mut.bench_computers_to_player_num_ = generic_options_.bench_computers_to_player_num_;
    mut.is_formal_ = generic_options_.is_formal_;
    generic_options_ = lgtbot::game::GenericOptions(imm, mut);

    ErrCollector err_reply;
    main_stage_ = GameHandle::main_stage_ptr(
            module_.alloc_stage_(&err_reply, game_options_.get(), &generic_options_, env_.get()),
            module_.del_stage_);
    if (!main_stage_) {
        err = err_reply.text_.empty() ? "NewMainStage failed" : err_reply.text_;
        lgtbot::ipc::GameResponse resp;
        resp.mutable_ack()->set_ok(false);
        SendProto(resp);
        env_.reset();
        return false;
    }
    main_stage_->HandleStageBegin();
    Routine();
    lgtbot::ipc::GameResponse resp;
    resp.mutable_ack()->set_ok(true);
    SendProto(resp);
    return true;
}

bool ChildGameSession::HandleExecute(const lgtbot::ipc::ExecuteReq& req, std::string& /*err*/)
{
    if (!main_stage_) {
        lgtbot::ipc::GameResponse resp;
        resp.mutable_result()->set_stage(lgtbot::ipc::ResultResp::STAGE_FAILED);
        SendProto(resp);
        return true;
    }
    ReplySender rep(*this);
    const auto stage_rc = main_stage_->HandleRequest(
            req.text().c_str(),
            req.player_id(),
            req.is_public(),
            rep);
    rep.flush_out();
    Routine();
    lgtbot::ipc::GameResponse resp;
    resp.mutable_result()->set_stage(StageErrToProto(stage_rc));
    SendProto(resp);
    return true;
}

bool ChildGameSession::HandleLeave(const lgtbot::ipc::LeaveReq& req, std::string& /*err*/)
{
    if (!main_stage_) {
        lgtbot::ipc::GameResponse resp;
        resp.mutable_ack()->set_ok(false);
        SendProto(resp);
        return true;
    }
    main_stage_->HandleLeave(PlayerID{req.player_id()});
    Routine();
    lgtbot::ipc::GameResponse resp;
    resp.mutable_ack()->set_ok(true);
    SendProto(resp);
    return true;
}

bool ChildGameSession::HandleHelp(const lgtbot::ipc::HelpReq& req, std::string& /*err*/)
{
    const bool text_mode = req.text_mode();
    std::string out;
    if (main_stage_) {
        out += "## 阶段信息\n\n";
        out += main_stage_->StageInfoC();
        out += "\n\n";
    }
    out += "## 当前可使用的游戏命令\n\n### 查看信息\n1. 帮助";
    if (main_stage_) {
        out += main_stage_->CommandInfoC(text_mode);
    } else if (game_options_) {
        out += "\n\n ### 配置选项";
        out += game_options_->Info(true, !text_mode);
    }
    lgtbot::ipc::GameResponse resp;
    auto* item = resp.mutable_reply()->add_items();
    item->set_text(std::move(out));
    SendProto(resp);
    return true;
}

int ChildGameSession::RunLoop()
{
    for (;;) {
        std::string raw;
        if (!ReadFrame(in_, raw)) {
            return 0;
        }
        lgtbot::ipc::GameRequest req;
        if (!req.ParseFromString(raw)) {
            lgtbot::ipc::GameResponse err_resp;
            err_resp.mutable_ack()->set_ok(false);
            SendProto(err_resp);
            continue;
        }
        std::string err;
        switch (req.req_case()) {
        case lgtbot::ipc::GameRequest::kShutdown:
            return 0;
        case lgtbot::ipc::GameRequest::kInit:
            HandleInit(req.init(), err);
            break;
        case lgtbot::ipc::GameRequest::kSetOption:
            HandleSetOption(req.set_option(), err);
            break;
        case lgtbot::ipc::GameRequest::kStart:
            HandleStart(req.start(), err);
            break;
        case lgtbot::ipc::GameRequest::kExecute:
            HandleExecute(req.execute(), err);
            break;
        case lgtbot::ipc::GameRequest::kLeave:
            HandleLeave(req.leave(), err);
            break;
        case lgtbot::ipc::GameRequest::kHelp:
            HandleHelp(req.help(), err);
            break;
        default: {
            lgtbot::ipc::GameResponse resp;
            resp.mutable_ack()->set_ok(false);
            SendProto(resp);
            break;
        }
        }
    }
}
