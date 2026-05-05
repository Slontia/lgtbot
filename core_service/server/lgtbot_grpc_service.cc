#include "lgtbot_grpc_service.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <unordered_map>

#include <grpcpp/grpcpp.h>

#include "bot_core/bot_ctx.h"
#include "bot_core/game_handle.h"
#include "bot_core/id.h"
#include "bot_core/match.h"
#ifdef WITH_SQLITE
#include "bot_core/db_manager.h"
#endif

namespace {

thread_local std::string g_tls_platform;

std::string RoomLabel(const Match& m)
{
    if (const auto og = m.gid(); og.has_value()) {
        return og->GetStr();
    }
    return std::to_string(m.MatchId());
}

std::string MakeCacheKey(const std::string& platform, const char* user_id)
{
    return platform + "|" + (user_id ? user_id : "");
}

bool SafeGameModuleSegment(const std::string& m)
{
    if (m.empty() || m == "." || m == "..") {
        return false;
    }
    if (m.find("..") != std::string::npos) {
        return false;
    }
    for (const char ch : m) {
        const unsigned char uc = static_cast<unsigned char>(ch);
        if (std::isalnum(uc) != 0) {
            continue;
        }
        if (ch == '_' || ch == '-' || ch == '.') {
            continue;
        }
        return false;
    }
    return true;
}

std::filesystem::path ResolvedGamePluginsRoot(const std::string& game_path)
{
    namespace fs = std::filesystem;
    std::error_code ec;
    const fs::path abs = fs::absolute(fs::path(game_path), ec);
    if (ec) {
        return fs::path(game_path);
    }
    const fs::path canon = fs::weakly_canonical(abs, ec);
    return ec ? abs : canon;
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

std::string LgtbotGrpcService::CacheKey(const std::string& platform, const std::string& uid) const
{
    return MakeCacheKey(platform, uid.c_str());
}

std::string LgtbotGrpcService::AvatarRelUrlForUser(const std::string& platform, const std::string& uid) const
{
    if (uid.empty() || uid.rfind("computer:", 0) == 0) {
        return "";
    }
    std::lock_guard<std::mutex> lk(cache_mu_);
    const auto it = user_cache_.find(MakeCacheKey(platform, uid.c_str()));
    if (it != user_cache_.end()) {
        return it->second.avatar_rel_url;
    }
    return "";
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
    std::unordered_map<std::string, std::pair<uint32_t, uint32_t>> live_counts;
    namespace fs = std::filesystem;
    for (const auto& m : bot.match_manager().Matches()) {
        if (!m) {
            continue;
        }
        const std::string gn(m->GameName());
        auto& p = live_counts[gn];
        const auto st = m->state();
        if (st == Match::NOT_STARTED) {
            ++p.first;
        } else if (st == Match::IS_STARTED) {
            ++p.second;
        }
    }
    const fs::path game_root = ResolvedGamePluginsRoot(game_path_);
    for (const auto& [display_name, gh] : bot.game_handles()) {
        auto* g = resp->add_games();
        g->set_name(display_name);
        g->set_module_name(gh.Info().module_name_);
        g->set_developer(gh.Info().developer_);
        g->set_description(gh.Info().description_);
        const uint64_t max_p = gh.CachedMaxPlayer();
        g->set_min_players(1);
        if (max_p == 0 || max_p > UINT32_MAX) {
            g->set_max_players(20);
        } else {
            g->set_max_players(static_cast<uint32_t>(max_p));
        }
        const auto itc = live_counts.find(display_name);
        if (itc != live_counts.end()) {
            g->set_live_not_started(itc->second.first);
            g->set_live_in_progress(itc->second.second);
        } else {
            g->set_live_not_started(0);
            g->set_live_in_progress(0);
        }
        const fs::path icon_path = game_root / gh.Info().module_name_ / "icon.png";
        std::error_code fec;
        if (fs::is_regular_file(icon_path, fec)) {
            g->set_icon_url(std::string("/api/game-modules/") + gh.Info().module_name_ + "/icon");
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

grpc::Status LgtbotGrpcService::WebListGameMatches(grpc::ServerContext* /*ctx*/,
        const lgtbot::WebListGameMatchesRequest* req, lgtbot::WebListGameMatchesResponse* resp)
{
    if (!bot_) {
        return grpc::Status::OK;
    }
    auto& bot = *static_cast<BotCtx*>(bot_);
    const std::string& game_name = req->game_name();
    const std::string& platform = req->platform();
    std::vector<std::shared_ptr<Match>> filtered;
    for (const auto& m : bot.match_manager().Matches()) {
        if (m && m->GameName() == game_name) {
            filtered.push_back(m);
        }
    }
    std::sort(filtered.begin(), filtered.end(),
            [](const std::shared_ptr<Match>& a, const std::shared_ptr<Match>& b) { return a->MatchId() > b->MatchId(); });
    resp->set_total(static_cast<uint32_t>(filtered.size()));
    const uint32_t off = req->offset();
    const uint32_t lim = req->limit() ? req->limit() : 50;
    const uint32_t end = std::min<uint32_t>(static_cast<uint32_t>(filtered.size()), off + lim);
    for (uint32_t i = off; i < end; ++i) {
        const auto& m = filtered[i];
        auto* b = resp->add_matches();
        b->set_match_id(m->MatchId());
        b->set_state(static_cast<int32_t>(m->state()));
        b->set_room_label(RoomLabel(*m));
        b->set_config_text(m->ConfigSummaryText());
        b->set_room_created_unix(m->RoomCreatedUnixSec());
        b->set_game_started_unix(m->GameStartedUnixSec());
        std::vector<Match::WebPlayerRow> rows;
        m->ListPlayersForWeb(bot, rows);
        for (const auto& row : rows) {
            auto* p = b->add_players();
            p->set_platform_user_id(row.platform_user_id);
            p->set_display_name(row.display_name);
            p->set_avatar_url(AvatarRelUrlForUser(platform, row.platform_user_id));
        }
    }
    return grpc::Status::OK;
}

grpc::Status LgtbotGrpcService::WebGetGameRule(
        grpc::ServerContext* /*ctx*/, const lgtbot::WebGetGameRuleRequest* req, lgtbot::WebGetGameRuleResponse* resp)
{
    if (!bot_) {
        return grpc::Status::OK;
    }
    auto& bot = *static_cast<BotCtx*>(bot_);
    const auto it = bot.game_handles().find(req->game_name());
    if (it != bot.game_handles().end()) {
        resp->set_pure_rule_markdown(it->second.Info().pure_rule_);
    }
    return grpc::Status::OK;
}

grpc::Status LgtbotGrpcService::WebGetGameAchievements(grpc::ServerContext* /*ctx*/,
        const lgtbot::WebGetGameAchievementsRequest* req, lgtbot::WebGetGameAchievementsResponse* resp)
{
    if (!bot_) {
        return grpc::Status::OK;
    }
    auto& bot = *static_cast<BotCtx*>(bot_);
    const auto it = bot.game_handles().find(req->game_name());
    if (it == bot.game_handles().end()) {
        return grpc::Status::OK;
    }
    const UserID viewer_id(req->viewer_platform_user_id());
    for (const auto& ach : it->second.Info().achievements_) {
        auto* item = resp->add_items();
        item->set_name(ach.name_);
        item->set_description(ach.description_);
        uint64_t global = 0;
#ifdef WITH_SQLITE
        if (bot.db_manager()) {
            global = bot.db_manager()
                             ->GetAchievementStatistic(viewer_id, req->game_name(), ach.name_)
                             .achieved_user_num_;
        }
#endif
        item->set_achieved_user_count(global);
    }
    return grpc::Status::OK;
}

grpc::Status LgtbotGrpcService::WebGetGameRankings(grpc::ServerContext* /*ctx*/,
        const lgtbot::WebGetGameRankingsRequest* req, lgtbot::WebGetGameRankingsResponse* resp)
{
    if (!bot_) {
        return grpc::Status::OK;
    }
    auto& bot = *static_cast<BotCtx*>(bot_);
    const int tr = static_cast<int>(req->time_range());
    if (tr < 0 || tr > 2) {
        return grpc::Status::OK;
    }
    const std::string& platform = req->platform();
    const uint32_t top_n = req->top_n() ? req->top_n() : 20;
#ifdef WITH_SQLITE
    if (!bot.db_manager()) {
        return grpc::Status::OK;
    }
    const auto rank_info =
            bot.db_manager()->GetLevelScoreRank(req->game_name(), k_time_range_begin_datetimes[static_cast<size_t>(tr)],
                    k_time_range_end_datetimes[static_cast<size_t>(tr)]);
    auto add_level_row = [&](const UserID& uid, double score) {
        auto* e = resp->add_level_score();
        e->set_platform_user_id(uid.GetStr());
        e->set_display_name(bot.GetUserName(uid.GetCStr(), nullptr));
        e->set_avatar_url(AvatarRelUrlForUser(platform, uid.GetStr()));
        e->set_score_value(score);
        e->set_match_count_value(0);
    };
    auto add_weight_row = [&](const UserID& uid, double score) {
        auto* e = resp->add_weighted_level_score();
        e->set_platform_user_id(uid.GetStr());
        e->set_display_name(bot.GetUserName(uid.GetCStr(), nullptr));
        e->set_avatar_url(AvatarRelUrlForUser(platform, uid.GetStr()));
        e->set_score_value(score);
        e->set_match_count_value(0);
    };
    auto add_match_count_row = [&](const UserID& uid, int64_t mc) {
        auto* e = resp->add_match_count();
        e->set_platform_user_id(uid.GetStr());
        e->set_display_name(bot.GetUserName(uid.GetCStr(), nullptr));
        e->set_avatar_url(AvatarRelUrlForUser(platform, uid.GetStr()));
        e->set_score_value(0);
        e->set_match_count_value(mc);
    };
    uint32_t n = 0;
    for (const auto& p : rank_info.level_score_rank_) {
        if (n >= top_n) {
            break;
        }
        add_level_row(p.first, p.second);
        ++n;
    }
    n = 0;
    for (const auto& p : rank_info.weight_level_score_rank_) {
        if (n >= top_n) {
            break;
        }
        add_weight_row(p.first, p.second);
        ++n;
    }
    n = 0;
    for (const auto& p : rank_info.match_count_rank_) {
        if (n >= top_n) {
            break;
        }
        add_match_count_row(p.first, p.second);
        ++n;
    }
#else
    (void)req;
    (void)resp;
    (void)bot;
    (void)platform;
    (void)top_n;
    (void)tr;
#endif
    return grpc::Status::OK;
}

grpc::Status LgtbotGrpcService::WebUserProfile(
        grpc::ServerContext* /*ctx*/, const lgtbot::WebUserProfileRequest* req, lgtbot::WebUserProfileResponse* resp)
{
    if (!bot_) {
        return grpc::Status::OK;
    }
    auto& bot = *static_cast<BotCtx*>(bot_);
    const int tr = static_cast<int>(req->time_range());
    if (tr < 0 || tr > 2) {
        return grpc::Status::OK;
    }
#ifdef WITH_SQLITE
    if (!bot.db_manager()) {
        return grpc::Status::OK;
    }
    const UserID uid(req->platform_user_id());
    const auto profile = bot.db_manager()->GetUserProfile(
            uid, k_time_range_begin_datetimes[static_cast<size_t>(tr)], k_time_range_end_datetimes[static_cast<size_t>(tr)]);
    resp->set_total_zero_sum_score(profile.total_zero_sum_score_);
    resp->set_total_top_score(profile.total_top_score_);
    resp->set_match_count(profile.match_count_);
    resp->set_birth_time(profile.birth_time_);
    for (const auto& gl : profile.game_level_infos_) {
        auto* r = resp->add_game_levels();
        r->set_game_name(gl.game_name_);
        r->set_count(gl.count_);
        r->set_total_level_score(gl.total_level_score_);
    }
    for (const auto& m : profile.recent_matches_) {
        auto* r = resp->add_recent_matches();
        r->set_game_name(m.game_name_);
        r->set_finish_time(m.finish_time_);
        r->set_user_count(m.user_count_);
        r->set_multiple(m.multiple_);
        r->set_game_score(m.game_score_);
        r->set_zero_sum_score(m.zero_sum_score_);
        r->set_top_score(m.top_score_);
        r->set_level_score(m.level_score_);
        r->set_rank_score(m.rank_score_);
    }
    for (const auto& h : profile.recent_honors_) {
        auto* r = resp->add_recent_honors();
        r->set_id(h.id_);
        r->set_description(h.description_);
        r->set_time(h.time_);
    }
    for (const auto& a : profile.recent_achievements_) {
        auto* r = resp->add_recent_achievements();
        r->set_game_name(a.game_name_);
        r->set_achievement_name(a.achievement_name_);
        r->set_time(a.time_);
        std::string desc;
        const auto git = bot.game_handles().find(a.game_name_);
        if (git != bot.game_handles().end()) {
            for (const auto& def : git->second.Info().achievements_) {
                if (def.name_ == a.achievement_name_) {
                    desc = def.description_;
                    break;
                }
            }
        }
        r->set_achievement_description(std::move(desc));
    }
#else
    (void)req;
    (void)resp;
    (void)bot;
    (void)tr;
#endif
    return grpc::Status::OK;
}
