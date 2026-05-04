#include "lgtbot_grpc_service.h"

#include <cstring>
#include <filesystem>
#include <fstream>

#include <grpcpp/grpcpp.h>

#include "bot_core/bot_ctx.h"

namespace {

thread_local std::string g_tls_platform;

std::string MakeCacheKey(const std::string& platform, const char* user_id)
{
    return platform + "|" + (user_id ? user_id : "");
}

void FillProtoMessage(const char* id, int is_to_user, const LGTBot_Message* messages, size_t size,
        lgtbot::PushEvent* ev)
{
    auto* msg = ev->mutable_message();
    for (size_t i = 0; i < size; ++i) {
        auto* item = msg->add_items();
        switch (messages[i].type_) {
        case LGTBOT_MSG_TEXT:
        case LGTBOT_MSG_USER_NAME:
            item->set_text(messages[i].str_ ? messages[i].str_ : "");
            break;
        case LGTBOT_MSG_USER_MENTION:
            item->set_at_platform_user_id(messages[i].str_ ? messages[i].str_ : "");
            break;
        case LGTBOT_MSG_IMAGE: {
            auto* img = item->mutable_image();
            std::string url = "/static/images/";
            url += (messages[i].str_ ? messages[i].str_ : "");
            img->set_url(std::move(url));
            img->set_delete_after_send(false);
            break;
        }
        default:
            item->set_text("");
            break;
        }
    }
    if (is_to_user) {
        ev->set_platform_user_id(id ? id : "");
    } else {
        ev->set_platform_group_id(id ? id : "");
    }
}

} // namespace

extern "C" {

static void cb_get_user_name(void* handler, char* buffer, size_t size, const char* user_id)
{
    static_cast<LgtbotGrpcService*>(handler)->OnGetUserName(buffer, size, user_id);
}

static void cb_get_user_name_in_group(
        void* handler, char* buffer, size_t size, const char* group_id, const char* user_id)
{
    static_cast<LgtbotGrpcService*>(handler)->OnGetUserNameInGroup(buffer, size, group_id, user_id);
}

static int cb_download_user_avatar(void* handler, const char* user_id, const char* dest_filename)
{
    return static_cast<LgtbotGrpcService*>(handler)->OnDownloadUserAvatar(user_id, dest_filename);
}

static void cb_handle_messages(
        void* handler, const char* id, const int is_to_user, const LGTBot_Message* messages, const size_t size)
{
    static_cast<LgtbotGrpcService*>(handler)->OnHandleMessages(id, is_to_user, messages, size);
}

} // extern "C"

void FillLgtbotCallbacks(LGTBot_Callback* out)
{
    out->get_user_name = &cb_get_user_name;
    out->get_user_name_in_group = &cb_get_user_name_in_group;
    out->download_user_avatar = &cb_download_user_avatar;
    out->handle_messages = &cb_handle_messages;
}

LgtbotGrpcService::LgtbotGrpcService(std::string image_root, std::string game_path)
    : image_root_(std::move(image_root))
    , game_path_(std::move(game_path))
{
}

LgtbotGrpcService::~LgtbotGrpcService()
{
    std::lock_guard<std::mutex> lk(queues_mu_);
    for (auto& [_, q] : push_queues_) {
        q->Close();
    }
}

PushQueue* LgtbotGrpcService::QueueForPlatform(const std::string& platform)
{
    std::lock_guard<std::mutex> lk(queues_mu_);
    const std::string key = platform.empty() ? "default" : platform;
    auto& slot = push_queues_[key];
    if (!slot) {
        slot = std::make_unique<PushQueue>();
    }
    return slot.get();
}

void LgtbotGrpcService::OnGetUserName(char* buffer, size_t size, const char* user_id)
{
    if (!buffer || size == 0) {
        return;
    }
    buffer[0] = '\0';
    const std::string plat = g_tls_platform.empty() ? "web" : g_tls_platform;
    std::lock_guard<std::mutex> lk(cache_mu_);
    const auto it = user_cache_.find(MakeCacheKey(plat, user_id));
    if (it != user_cache_.end() && !it->second.display_name.empty()) {
        std::strncpy(buffer, it->second.display_name.c_str(), size - 1);
        buffer[size - 1] = '\0';
        return;
    }
    if (user_id) {
        std::strncpy(buffer, user_id, size - 1);
        buffer[size - 1] = '\0';
    }
}

void LgtbotGrpcService::OnGetUserNameInGroup(
        char* buffer, size_t size, const char* /*group_id*/, const char* user_id)
{
    OnGetUserName(buffer, size, user_id);
}

int LgtbotGrpcService::OnDownloadUserAvatar(const char* user_id, const char* dest_filename)
{
    if (!user_id || !dest_filename) {
        return 0;
    }
    const std::string plat = g_tls_platform.empty() ? "web" : g_tls_platform;
    namespace fs = std::filesystem;
    const fs::path src = fs::path(image_root_) / "avatar" / plat / (std::string(user_id) + ".png");
    std::error_code ec;
    if (!fs::exists(src, ec)) {
        return 0;
    }
    fs::create_directories(fs::path(dest_filename).parent_path(), ec);
    fs::copy_file(src, dest_filename, fs::copy_options::overwrite_existing, ec);
    return ec ? 0 : 1;
}

void LgtbotGrpcService::OnHandleMessages(
        const char* id, const int is_to_user, const LGTBot_Message* messages, const size_t size)
{
    if (!messages || size == 0) {
        return;
    }
    const std::string plat = g_tls_platform.empty() ? "web" : g_tls_platform;
    lgtbot::PushEvent ev;
    ev.set_event_sign(++event_seq_);
    FillProtoMessage(id, is_to_user, messages, size, &ev);
    QueueForPlatform(plat)->Push(std::move(ev));
}

grpc::Status LgtbotGrpcService::HandlePrivateRequest(
        grpc::ServerContext* /*ctx*/, const lgtbot::PrivateRequest* req, lgtbot::PrivateResponse* resp)
{
    if (!bot_) {
        resp->set_err_code(EC_NOT_INIT);
        resp->set_err_msg("bot not bound");
        return grpc::Status::OK;
    }
    g_tls_platform = req->platform();
    const ErrCode ec =
            LGTBot_HandlePrivateRequest(bot_, req->platform_user_id().c_str(), req->text().c_str());
    g_tls_platform.clear();
    resp->set_err_code(static_cast<int32_t>(ec));
    resp->set_err_msg(std::string(errcode2str(ec)));
    return grpc::Status::OK;
}

grpc::Status LgtbotGrpcService::Subscribe(
        grpc::ServerContext* ctx, const lgtbot::SubscribeRequest* req, grpc::ServerWriter<lgtbot::PushEvent>* writer)
{
    PushQueue* q = QueueForPlatform(req->platform());
    lgtbot::PushEvent ev;
    while (!ctx->IsCancelled() && q->PopBlocking(ev)) {
        if (!writer->Write(ev)) {
            break;
        }
    }
    return grpc::Status::OK;
}

grpc::Status LgtbotGrpcService::GetCommands(
        grpc::ServerContext* /*ctx*/, const lgtbot::GetCommandsRequest* /*req*/, lgtbot::GetCommandsResponse* /*resp*/)
{
    return grpc::Status::OK;
}

grpc::Status LgtbotGrpcService::GetUserInfo(
        grpc::ServerContext* /*ctx*/, const lgtbot::GetUserInfoRequest* req, lgtbot::GetUserInfoResponse* resp)
{
    std::lock_guard<std::mutex> lk(cache_mu_);
    const auto it = user_cache_.find(MakeCacheKey(req->platform(), req->platform_user_id().c_str()));
    if (it != user_cache_.end()) {
        resp->set_display_name(it->second.display_name);
        resp->set_avatar_url(it->second.avatar_rel_url);
    } else {
        resp->set_display_name(req->platform_user_id());
    }
    return grpc::Status::OK;
}

grpc::Status LgtbotGrpcService::GetMatchHistory(grpc::ServerContext* /*ctx*/,
        const lgtbot::GetMatchHistoryRequest* /*req*/, grpc::ServerWriter<lgtbot::Message>* /*writer*/)
{
    return grpc::Status::OK;
}

grpc::Status LgtbotGrpcService::UpdateUserInfo(
        grpc::ServerContext* /*ctx*/, const lgtbot::UpdateUserInfoRequest* req, lgtbot::UpdateUserInfoResponse* resp)
{
    namespace fs = std::filesystem;
    const std::string key = MakeCacheKey(req->platform(), req->platform_user_id().c_str());
    UserCacheEntry entry;
    entry.display_name = req->display_name();
    const fs::path rel = fs::path("avatar") / req->platform() / (req->platform_user_id() + ".png");
    const fs::path dest = fs::path(image_root_) / rel;
    std::error_code ec;
    fs::create_directories(dest.parent_path(), ec);
    if (!req->avatar_png().empty()) {
        std::ofstream out(dest, std::ios::binary);
        out.write(req->avatar_png().data(), static_cast<std::streamsize>(req->avatar_png().size()));
    }
    entry.avatar_rel_url = std::string("/static/images/") + rel.generic_string();
    {
        std::lock_guard<std::mutex> lk(cache_mu_);
        user_cache_[key] = std::move(entry);
    }
    (void)resp;
    return grpc::Status::OK;
}

grpc::Status LgtbotGrpcService::RecordChatMessage(grpc::ServerContext* /*ctx*/,
        const lgtbot::RecordChatMessageRequest* /*req*/, lgtbot::RecordChatMessageResponse* /*resp*/)
{
    return grpc::Status::OK;
}

grpc::Status LgtbotGrpcService::GetGameList(
        grpc::ServerContext* /*ctx*/, const lgtbot::GetGameListRequest* /*req*/, lgtbot::GetGameListResponse* resp)
{
    if (!bot_) {
        return grpc::Status::OK;
    }
    auto& bot = *static_cast<BotCtx*>(bot_);
    for (const auto& [display_name, gh] : bot.game_handles()) {
        auto* g = resp->add_games();
        g->set_name(display_name);
        g->set_developer(gh.Info().developer_);
        g->set_description(gh.Info().description_);
        const uint64_t max_p = gh.CachedMaxPlayer();
        g->set_min_players(1);
        if (max_p == 0 || max_p > UINT32_MAX) {
            g->set_max_players(20);
        } else {
            g->set_max_players(static_cast<uint32_t>(max_p));
        }
    }
    return grpc::Status::OK;
}

grpc::Status LgtbotGrpcService::StartGame(
        grpc::ServerContext* /*ctx*/, const lgtbot::StartGameRequest* req, lgtbot::StartGameResponse* resp)
{
    if (!bot_) {
        resp->set_err_code(EC_NOT_INIT);
        resp->set_err_msg("bot not bound");
        return grpc::Status::OK;
    }
    g_tls_platform = req->platform();
    std::string cmd = std::string(META_COMMAND_SIGN) + "新游戏 " + req->game_name();
    if (req->solo()) {
        cmd += " 单机";
    }
    const ErrCode ec = LGTBot_HandlePrivateRequest(bot_, req->platform_user_id().c_str(), cmd.c_str());
    g_tls_platform.clear();
    resp->set_err_code(static_cast<int32_t>(ec));
    resp->set_err_msg(std::string(errcode2str(ec)));
    return grpc::Status::OK;
}
