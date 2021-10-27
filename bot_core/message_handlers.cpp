#include "bot_core/message_handlers.h"

#include "utility/msg_checker.h"

#include "bot_core/bot_core.h"
#include "bot_core/db_manager.h"
#include "bot_core/log.h"
#include "bot_core/match.h"
#include "bot_core/image.h"

// para func can appear only once
#define RETURN_IF_FAILED(func)                                 \
    do {                                                       \
        if (const auto ret = (func); ret != EC_OK) return ret; \
    } while (0);

static MetaCommand make_command(const char* const description, const auto& cb, auto&&... checkers)
{
    return MetaCommand(description, cb, std::move(checkers)...);
};

namespace {

struct MetaCommandDesc
{
    bool is_common_;
    MetaCommand cmd_;
};

struct MetaCommandGroup
{
    std::string group_name_;
    std::vector<MetaCommand> desc_;
};

struct ShowCommandOption
{
    bool only_common_ = true;
    bool with_html_color_ = false;
    bool with_example_ = true;
};

}

extern const std::vector<MetaCommandGroup> meta_cmds;
extern const std::vector<MetaCommandGroup> admin_cmds;

static ErrCode help_internal(BotCtx& bot, MsgSenderBase& reply, const std::vector<MetaCommandGroup>& cmd_groups,
        const ShowCommandOption& option, const std::string& type_name)
{
    std::string outstr = "## 可使用的" + type_name + "指令";
    for (const MetaCommandGroup& cmd_group : cmd_groups) {
        int i = 0;
        outstr += "\n\n### " + cmd_group.group_name_;
        for (const MetaCommand& cmd : cmd_group.desc_) {
            outstr += "\n" + std::to_string(++i) + ". " + cmd.Info(option.with_example_, option.with_html_color_);
        }
    }
    if (option.with_html_color_) {
        reply() << Markdown(outstr);
    } else {
        reply() << outstr;
    }
    return EC_OK;
}

template <bool IS_ADMIN = false>
static ErrCode help(BotCtx& bot, const UserID uid, const std::optional<GroupID> gid, MsgSenderBase& reply, const bool show_text) {
    
    return help_internal(
            bot, reply, IS_ADMIN ? admin_cmds : meta_cmds,
            ShowCommandOption{.only_common_ = show_text, .with_html_color_ = !show_text, .with_example_ = !show_text},
            IS_ADMIN ? "管理" : "元");
}

ErrCode HandleRequest(BotCtx& bot, const UserID uid, const std::optional<GroupID>& gid, MsgReader& reader,
                      MsgSenderBase& reply, const std::vector<MetaCommandGroup>& cmd_groups)
{
    reader.Reset();
    for (const MetaCommandGroup& cmd_group : cmd_groups) {
        for (const MetaCommand& cmd : cmd_group.desc_) {
            const std::optional<ErrCode> errcode = cmd.CallIfValid(reader, bot, uid, gid, reply);
            if (errcode.has_value()) {
                return *errcode;
            }
        }
    }
    return EC_REQUEST_NOT_FOUND;
}

ErrCode HandleMetaRequest(BotCtx& bot, const UserID uid, const std::optional<GroupID>& gid, const std::string& msg,
                          MsgSender& reply)
{
    MsgReader reader(msg);
    const auto ret = HandleRequest(bot, uid, gid, reader, reply, meta_cmds);
    if (ret == EC_REQUEST_NOT_FOUND) {
        reply() << "[错误] 未预料的元指令，您可以通过\"#帮助\"查看所有支持的元指令";
    }
    return ret;
}

ErrCode HandleAdminRequest(BotCtx& bot, const UserID uid, const std::optional<GroupID>& gid, const std::string& msg,
                           MsgSender& reply)
{
    MsgReader reader(msg);
    const auto ret = HandleRequest(bot, uid, gid, reader, reply, admin_cmds);
    if (ret == EC_REQUEST_NOT_FOUND) {
        reply() << "[错误] 未预料的管理指令，您可以通过\"%帮助\"查看所有支持的管理指令";
    }
    return ret;
}

static ErrCode show_gamelist(BotCtx& bot, const UserID uid, const std::optional<GroupID>& gid,
                             MsgSenderBase& reply)
{
    int i = 0;
    if (bot.game_handles().empty()) {
        reply() << "未载入任何游戏";
        return EC_OK;
    }
    auto sender = reply();
    sender << "游戏列表：";
    for (const auto& [name, info] : bot.game_handles()) {
        sender << "\n" << (++i) << ". " << name;
        if (info->game_id() == 0) {
            sender << "（试玩）";
        }
    }
    return EC_OK;
}

static ErrCode new_game(BotCtx& bot, const UserID uid, const std::optional<GroupID>& gid, MsgSenderBase& reply,
                        const std::string& gamename)
{
    const auto it = bot.game_handles().find(gamename);
    if (it == bot.game_handles().end()) {
        reply() << "[错误] 创建失败：未知的游戏名，请通过\"#游戏列表\"查看游戏名称";
        return EC_REQUEST_UNKNOWN_GAME;
    }
    return bot.match_manager().NewMatch(*it->second, uid, gid, reply);
}

static ErrCode set_bench_to(BotCtx& bot, const UserID uid, const std::optional<GroupID>& gid, MsgSenderBase& reply,
        const uint64_t bench_to_player_num)
{
    const auto match = bot.match_manager().GetMatch(uid);
    if (!match) {
        reply() << "[错误] 开始失败：您未加入游戏";
        return EC_MATCH_USER_NOT_IN_MATCH;
    }
    if (gid.has_value() && match->gid() != gid) {
        reply() << "[错误] 开始失败，您是在其他房间创建的游戏，若您忘记该房间，可以尝试私信裁判";
        return EC_MATCH_NOT_THIS_GROUP;
    }
    return match->SetBenchTo(uid, reply, bench_to_player_num);
}

static ErrCode start_game(BotCtx& bot, const UserID uid, const std::optional<GroupID>& gid, MsgSenderBase& reply)
{
    const auto match = bot.match_manager().GetMatch(uid);
    if (!match) {
        reply() << "[错误] 开始失败：您未加入游戏";
        return EC_MATCH_USER_NOT_IN_MATCH;
    }
    if (gid.has_value() && match->gid() != gid) {
        reply() << "[错误] 开始失败，您是在其他房间创建的游戏，若您忘记该房间，可以尝试私信裁判";
        return EC_MATCH_NOT_THIS_GROUP;
    }
    return match->GameStart(uid, gid.has_value(), reply);
}

static ErrCode leave(BotCtx& bot, const UserID uid, const std::optional<GroupID>& gid, MsgSenderBase& reply,
        const bool force)
{
    const auto match = bot.match_manager().GetMatch(uid);
    if (!match) {
        reply() << "[错误] 退出失败：您未加入游戏";
        return EC_MATCH_USER_NOT_IN_MATCH;
    }
    if (gid.has_value() && match->gid() != *gid) {
        reply() << "[错误] 退出失败：您未加入本房间游戏，您可以尝试私信裁判\"#退出\"以退出您所在的游戏";
        return EC_MATCH_NOT_THIS_GROUP;
    }
    RETURN_IF_FAILED(match->Leave(uid, reply, force));
    return EC_OK;
}

static ErrCode join_private(BotCtx& bot, const UserID uid, const std::optional<GroupID>& gid,
                            MsgSenderBase& reply, const MatchID mid)
{
    if (gid.has_value()) {
        reply() << "[错误] 加入失败：请私信裁判加入私密游戏，或去掉比赛ID以加入当前房间游戏";
        return EC_MATCH_NEED_REQUEST_PRIVATE;
    }
    const auto match = bot.match_manager().GetMatch(mid);
    if (!match) {
        reply() << "[错误] 加入失败：游戏ID不存在";
        return EC_MATCH_NOT_EXIST;
    }
    if (!match->IsPrivate()) {
        reply() << "[错误] 加入失败：该游戏属于公开比赛，请前往房间加入游戏";
        return EC_MATCH_NEED_REQUEST_PUBLIC;
    }
    RETURN_IF_FAILED(match->Join(uid, reply));
    return EC_OK;
}

static ErrCode join_public(BotCtx& bot, const UserID uid, const std::optional<GroupID>& gid, MsgSenderBase& reply)
{
    if (!gid.has_value()) {
        reply() << "[错误] 加入失败：若要加入私密游戏，请指明比赛ID";
        return EC_MATCH_NEED_ID;
    }
    const auto match = bot.match_manager().GetMatch(*gid);
    if (!match) {
        reply() << "[错误] 加入失败：该房间未进行游戏";
        return EC_MATCH_GROUP_NOT_IN_MATCH;
    }
    assert(!match->IsPrivate());
    RETURN_IF_FAILED(match->Join(uid, reply));
    return EC_OK;
}

static ErrCode show_private_matches(BotCtx& bot, const UserID uid, const std::optional<GroupID> gid,
                                    MsgSenderBase& reply)
{
    uint64_t count = 0;
    auto sender = reply();
    const auto matches = bot.match_manager().Matches();
    for (const auto& match : matches) {
        if (match->IsPrivate() && match->state() == Match::State::NOT_STARTED) {
            ++count;
            sender << match->game_handle().name_ << " - [房主ID] " << match->host_uid() << " - [比赛ID] "
                   << match->mid() << "\n";
        }
    }
    if (count == 0) {
        sender << "当前无未开始的私密比赛";
    } else {
        sender << "共" << count << "场";
    }
    return EC_OK;
}

static ErrCode show_match_info(BotCtx& bot, const UserID uid, const std::optional<GroupID> gid,
                                 MsgSenderBase& reply)
{
    std::shared_ptr<Match> match;
    if (gid.has_value() && !(match = bot.match_manager().GetMatch(*gid))) {
        reply() << "[错误] 查看失败：该房间未进行游戏";
        return EC_MATCH_GROUP_NOT_IN_MATCH;
    } else if (!gid.has_value() && !(match = bot.match_manager().GetMatch(uid))) {
        reply() << "[错误] 查看失败：该房间未进行游戏";
        return EC_MATCH_GROUP_NOT_IN_MATCH;
    }
    match->ShowInfo(gid, reply);
    return EC_OK;
}

static ErrCode show_rule(BotCtx& bot, const UserID uid, const std::optional<GroupID> gid, MsgSenderBase& reply,
                         const std::string& gamename, const bool show_text)
{
    const auto it = bot.game_handles().find(gamename);
    if (it == bot.game_handles().end()) {
        reply() << "[错误] 查看失败：未知的游戏名，请通过\"#游戏列表\"查看游戏名称";
        return EC_REQUEST_UNKNOWN_GAME;
    };
    if (!show_text) {
        reply() << Markdown(it->second->rule_);
        return EC_OK;
    }
    auto sender = reply();
    sender << "最多可参加人数：";
    if (it->second->max_player_ == 0) {
        sender << "无限制";
    } else {
        sender << it->second->max_player_;
    }
    sender << "人\n";
    sender << "详细规则：\n";
    sender << it->second->rule_;
    return EC_OK;
}

static ErrCode about(BotCtx& bot, const UserID uid, const std::optional<GroupID> gid, MsgSenderBase& reply)
{
    reply() << "LGTBot 内测版本 Beta-v0.1.0"
               "\n"
               "\n作者：森高（QQ：654867229）"
               "\nGitHub：http://github.com/slontia/lgtbot"
               "\n"
               "\n若您使用中遇到任何 BUG 或其它问题，欢迎私信作者，或前往 GitHub 主页提 issue"
               "\n本项目仅供娱乐和技术交流，请勿用于商业用途，健康游戏，拒绝赌博";
    return EC_OK;
}

#ifdef WITH_MYSQL
static ErrCode show_profile(BotCtx& bot, const UserID uid, const std::optional<GroupID> gid,
                            MsgSenderBase& reply)
{
    const std::unique_ptr<DBManager>& db_manager = DBManager::GetDBManager();
    if (db_manager == nullptr) {
        reply() << "[错误] 查看失败：未连接数据库";
        return EC_DB_NOT_CONNECTED;
    }
    db_manager->GetUserProfit(uid, reply);  // TODO: pass sender
    return EC_OK;
}
#endif

const std::vector<MetaCommandGroup> meta_cmds = {
    {
        "信息查看", { // GAME INFO: can be executed at any time
            make_command("查看帮助", help<false>, VoidChecker("#帮助"),
                        OptionalDefaultChecker<BoolChecker>(false, "文字", "图片")),
#ifdef WITH_MYSQL
            make_command("查看个人信息", show_profile, VoidChecker("#个人信息")),
#endif
            make_command("查看游戏列表", show_gamelist, VoidChecker("#游戏列表")),
            make_command("查看游戏规则（游戏名称可以通过\"#游戏列表\"查看）", show_rule, VoidChecker("#规则"),
                        AnyArg("游戏名称", "猜拳游戏"), OptionalDefaultChecker<BoolChecker>(false, "文字", "图片")),
            make_command("查看已加入，或该房间正在进行的比赛信息", show_match_info, VoidChecker("#游戏信息")),
            make_command("查看当前所有未开始的私密比赛", show_private_matches, VoidChecker("#私密游戏列表")),
            make_command("关于机器人", about, VoidChecker("#关于")),
        }
    },
    {
        "新建游戏", { // NEW GAME: can only be executed by host
            make_command("在当前房间建立公开游戏，或私信bot以建立私密游戏（游戏名称可以通过\"#游戏列表\"查看）",
                        new_game, VoidChecker("#新游戏"), AnyArg("游戏名称", "猜拳游戏")),
            make_command("房主设置参与游戏的AI数量，使得玩家不低于一定数量（属于配置变更，会使得全部玩家退出游戏）",
                        set_bench_to, VoidChecker("#替补至"), ArithChecker<uint32_t>(2, 12, "数量")),
            make_command("房主开始游戏", start_game, VoidChecker("#开始")),
        }
    },
    {
        "加入游戏", { // JOIN/LEAVE GAME: can only be executed by player
            make_command("加入当前房间的公开游戏", join_public, VoidChecker("#加入")),
            make_command("私信bot以加入私密游戏（可通过「#私密游戏列表」查看比赛编号）", join_private, VoidChecker("#加入"),
                        BasicChecker<MatchID>("私密比赛编号", "1")),
            make_command("退出游戏（若附带了「强制」参数，则可以在游戏进行中退出游戏，需注意退出后无法继续参与原游戏）",
                        leave, VoidChecker("#退出"),  OptionalDefaultChecker<BoolChecker>(false, "强制", "常规")),
        }
    }
};

#ifdef WITH_MYSQL
static ErrCode release_game(BotCtx& bot, const UserID uid, const std::optional<GroupID> gid,
                            MsgSenderBase& reply, const std::string& gamename)
{
    const auto it = bot.game_handles().find(gamename);
    if (it == bot.game_handles().end()) {
        reply() << "[错误] 发布失败：未知的游戏名，请通过\"#游戏列表\"查看游戏名称";
        return EC_REQUEST_UNKNOWN_GAME;
    }
    auto game_id = it->second->game_id();
    if (game_id != 0) {
        reply() << "[错误] 发布失败：游戏已发布，ID为" << game_id;
        return EC_GAME_ALREADY_RELEASE;
    }
    const std::unique_ptr<DBManager>& db_manager = DBManager::GetDBManager();
    if (db_manager == nullptr) {
        reply() << "[错误] 发布失败：未连接数据库";
        return EC_DB_NOT_CONNECTED;
    }
    if ((game_id = db_manager->ReleaseGame(gamename)) == 0) {
        reply() << "[错误] 发布失败：发布失败，请查看错误日志";
        return EC_DB_RELEASE_GAME_FAILED;
    }
    it->second->set_game_id(game_id);
    reply() << "发布成功，游戏\'" << gamename << "\'的ID为" << game_id;
    return EC_OK;
}
#endif

static ErrCode interrupt_game(BotCtx& bot, const UserID uid, const std::optional<GroupID> gid,
                                 MsgSenderBase& reply, const std::optional<MatchID> mid)
{
    std::shared_ptr<Match> match;
    if (mid.has_value() && !(match = bot.match_manager().GetMatch(*mid))) {
        reply() << "[错误] 中断失败：游戏ID不存在";
        return EC_MATCH_NOT_EXIST;
    } else if (!mid.has_value() && gid.has_value() && !(match = bot.match_manager().GetMatch(*gid))) {
        reply() << "[错误] 中断失败：该房间未进行游戏";
        return EC_MATCH_GROUP_NOT_IN_MATCH;
    } else if (!mid.has_value() && !gid.has_value()) {
        reply() << "[错误] 中断失败：需要在房间中使用该指令，或指定比赛ID";
        return EC_MATCH_NEED_REQUEST_PUBLIC;
    }
    match->Terminate();
    reply() << "中断成功";
    return EC_OK;
}

const std::vector<MetaCommandGroup> admin_cmds = {
    {
        "信息查看", {
            make_command("查看帮助", help<true>, VoidChecker("%帮助"),
                        OptionalDefaultChecker<BoolChecker>(false, "文字", "图片")),
        }
    },
    {
        "管理操作", {
#ifdef WITH_MYSQL
            make_command("发布游戏，写入游戏信息到数据库", release_game, VoidChecker("%发布游戏"),
                        AnyArg("游戏名称", "某游戏名")),
#endif
            make_command("强制中断比赛", interrupt_game, VoidChecker("%中断游戏"),
                        OptionalChecker<BasicChecker<MatchID>>("私密比赛编号")),
        }
    },
};
