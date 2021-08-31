#include "bot_core/message_handlers.h"

#include "utility/msg_checker.h"

#include "bot_core/bot_core.h"
#include "bot_core/db_manager.h"
#include "bot_core/log.h"
#include "bot_core/match.h"

// para func can appear only once
#define RETURN_IF_FAILED(func)                                 \
    do {                                                       \
        if (const auto ret = (func); ret != EC_OK) return ret; \
    } while (0);

static MetaCommand make_command(const char* const description, const auto& cb, auto&&... checkers)
{
    return MetaCommand(description, cb, std::move(checkers)...);
};

static ErrCode help(BotCtx& bot, const UserID uid, const std::optional<GroupID>& gid, MsgSenderBase& reply,
                    const std::vector<MetaCommand>& cmds, const std::string& type)
{
    auto sender = reply();
    sender << "[可使用的" << type << "指令]";
    int i = 0;
    for (const MetaCommand& cmd : cmds) {
        sender << "\n[" << (++i) << "] " << cmd.Info();
    }
    return EC_OK;
}

ErrCode HandleRequest(BotCtx& bot, const UserID uid, const std::optional<GroupID>& gid, MsgReader& reader,
                      MsgSenderBase& reply, const std::vector<MetaCommand>& cmds)
{
    reader.Reset();
    for (const MetaCommand& cmd : cmds) {
        const std::optional<ErrCode> errcode = cmd.CallIfValid(reader, bot, uid, gid, reply);
        if (errcode.has_value()) {
            return *errcode;
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
        if (!info->game_id().has_value()) {
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

static std::variant<ErrCode, std::shared_ptr<Match>> GetMatchByHost(BotCtx& bot,
        const UserID uid, const std::optional<GroupID> gid, MsgSenderBase& reply)
{
    const auto match = bot.match_manager().GetMatch(uid);
    if (!match) {
        reply() << "[错误] 开始失败：您未加入游戏";
        return EC_MATCH_USER_NOT_IN_MATCH;
    }
    if (match->host_uid() != uid) {
        reply() << "[错误] 开始失败：您不是房主，没有开始游戏的权限";
        return EC_MATCH_NOT_HOST;
    }
    if (gid.has_value() && match->gid() != gid) {
        reply() << "[错误] 开始失败，您是在其他房间创建的游戏，若您忘记该房间，可以尝试私信裁判\"#开始\"以开始游戏";
        return EC_MATCH_NOT_THIS_GROUP;
    }
    return match;
}

static ErrCode set_com_num(BotCtx& bot, const UserID uid, const std::optional<GroupID>& gid, MsgSenderBase& reply,
        const uint64_t com_num)
{
    const auto match_or_errcode = GetMatchByHost(bot, uid, gid, reply);
    if (const auto p_errcode = std::get_if<0>(&match_or_errcode)) {
        return *p_errcode;
    }
    const auto match = std::get<1>(match_or_errcode);
    return match->GameSetComNum(reply, com_num);
}

static ErrCode start_game(BotCtx& bot, const UserID uid, const std::optional<GroupID>& gid, MsgSenderBase& reply)
{
    const auto match_or_errcode = GetMatchByHost(bot, uid, gid, reply);
    if (const auto p_errcode = std::get_if<0>(&match_or_errcode)) {
        return *p_errcode;
    }
    const auto match = std::get<1>(match_or_errcode);
    return match->GameStart(gid.has_value(), reply);
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
    // TODO: make it thread safe
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
                         const std::string& gamename)
{
    const auto it = bot.game_handles().find(gamename);
    if (it == bot.game_handles().end()) {
        reply() << "[错误] 查看失败：未知的游戏名，请通过\"#游戏列表\"查看游戏名称";
        return EC_REQUEST_UNKNOWN_GAME;
    };
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

const std::vector<MetaCommand> meta_cmds = {
        // GAME INFO: can be executed at any time
        make_command(
                "查看帮助",
                [](BotCtx& bot, const UserID uid, const std::optional<GroupID> gid, MsgSenderBase& reply) {
                    return help(bot, uid, gid, reply, meta_cmds, "元");
                },
                VoidChecker("#帮助")),
#ifdef WITH_MYSQL
        make_command("查看个人信息", show_profile, VoidChecker("#个人信息")),
#endif
        make_command("查看游戏列表", show_gamelist, VoidChecker("#游戏列表")),
        make_command("查看游戏规则（游戏名称可以通过\"#游戏列表\"查看）", show_rule, VoidChecker("#规则"),
                     AnyArg("游戏名称", "某游戏名")),
        make_command("查看当前所有未开始的私密比赛", show_private_matches, VoidChecker("#私密游戏列表")),
        make_command("查看已加入，或该房间正在进行的比赛信息", show_match_info, VoidChecker("#游戏信息")),

        // NEW GAME: can only be executed by host
        make_command("在当前房间建立公开游戏，或私信bot以建立私密游戏（游戏名称可以通过\"#游戏列表\"查看）",
                     new_game, VoidChecker("#新游戏"), AnyArg("游戏名称", "某游戏名")),
        make_command("设置参与游戏的AI数量", set_com_num, VoidChecker("#电脑数量"), ArithChecker<uint32_t>(0, 12, "数量")),
        make_command("房主开始游戏", start_game, VoidChecker("#开始游戏")),

        // JOIN/LEAVE GAME: can only be executed by player
        make_command("加入当前房间的公开游戏", join_public, VoidChecker("#加入游戏")),
        make_command("私信bot以加入私密游戏（私密比赛编号可以通过\"#私密游戏列表\"查看）", join_private,
                     VoidChecker("#加入游戏"), BasicChecker<MatchID>("私密比赛编号")),
        make_command("退出游戏（若附带了「强制」参数，则可以在游戏进行中退出游戏，需注意退出后无法继续参与原游戏",
                     leave, VoidChecker("#退出游戏"),  BoolChecker<true>("强制", "常规")),
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
    std::optional<uint64_t> game_id = it->second->game_id();
    if (game_id.has_value()) {
        reply() << "[错误] 发布失败：游戏已发布，ID为" << *game_id;
        return EC_GAME_ALREADY_RELEASE;
    }
    const std::unique_ptr<DBManager>& db_manager = DBManager::GetDBManager();
    if (db_manager == nullptr) {
        reply() << "[错误] 发布失败：未连接数据库";
        return EC_DB_NOT_CONNECTED;
    }
    if (!(game_id = db_manager->ReleaseGame(gamename)).has_value()) {
        reply() << "[错误] 发布失败：发布失败，请查看错误日志";
        return EC_DB_RELEASE_GAME_FAILED;
    }
    it->second->set_game_id(game_id);
    reply() << "发布成功，游戏\'" << gamename << "\'的ID为" << *game_id;
    return EC_OK;
}
#endif

static ErrCode interrupt_public(BotCtx& bot, const UserID uid, const std::optional<GroupID> gid,
                                MsgSenderBase& reply)
{
    if (!gid.has_value()) {
        reply() << "[错误] 中断失败：需要在房间中使用该指令";
        return EC_MATCH_NEED_REQUEST_PUBLIC;
    }
    const auto match = bot.match_manager().GetMatch(*gid);
    if (!match) {
        reply() << "[错误] 中断失败：该房间未进行游戏";
        return EC_MATCH_GROUP_NOT_IN_MATCH;
    }
    match->Interrupt();
    reply() <<  "中断成功";
    return EC_OK;
}

static ErrCode interrupt_private(BotCtx& bot, const UserID uid, const std::optional<GroupID> gid,
                                 MsgSenderBase& reply, const MatchID mid)
{
    const auto match = bot.match_manager().GetMatch(mid);
    if (!match) {
        reply() << "[错误] 中断失败：游戏ID不存在";
        return EC_MATCH_NOT_EXIST;
    }
    match->Interrupt();
    reply() << "中断成功";
    return EC_OK;
}

const std::vector<MetaCommand> admin_cmds = {
        make_command(
                "查看帮助",
                [](BotCtx& bot, const UserID uid, const std::optional<GroupID> gid, MsgSenderBase& reply) {
                    return help(bot, uid, gid, reply, admin_cmds, "管理");
                },
                VoidChecker("%帮助")),
#ifdef WITH_MYSQL
        make_command("发布游戏，写入游戏信息到数据库", release_game, VoidChecker("%发布游戏"),
                     AnyArg("游戏名称", "某游戏名")),
#endif
        make_command("强制中断公开比赛", interrupt_public, VoidChecker("%中断游戏")),
        make_command("强制中断私密比赛", interrupt_private, VoidChecker("%中断游戏"),
                     BasicChecker<MatchID>("私密比赛编号")),
};
