// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <algorithm>
#include <ranges>
#include <cmath>

#include "bot_core/message_handlers.h"

#include "utility/msg_checker.h"
#include "utility/html.h"
#include "utility/log.h"
#include "bot_core/bot_core.h"
#include "bot_core/db_manager.h"
#include "bot_core/match.h"
#include "bot_core/image.h"
#include "bot_core/options.h"

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

static uint32_t DefaultMultiple(const GameHandle& game_handle)
{
    return game_handle.Info().multiple_fn_(game_handle.DefaultGameOptions().Lock()->game_options_.get());
}

static uint32_t DefaultMaxPlayer(const GameHandle& game_handle)
{
    return game_handle.Info().max_player_num_fn_(game_handle.DefaultGameOptions().Lock()->game_options_.get());
}

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
                          MsgSenderBase& reply)
{
    MsgReader reader(msg);
    const auto ret = HandleRequest(bot, uid, gid, reader, reply, meta_cmds);
    if (ret == EC_REQUEST_NOT_FOUND) {
        reply() << "[错误] 未预料的元指令，您可以通过「" META_COMMAND_SIGN "帮助」查看所有支持的元指令";
    }
    return ret;
}

ErrCode HandleAdminRequest(BotCtx& bot, const UserID uid, const std::optional<GroupID>& gid, const std::string& msg,
                           MsgSenderBase& reply)
{
    MsgReader reader(msg);
    const auto ret = HandleRequest(bot, uid, gid, reader, reply, admin_cmds);
    if (ret == EC_REQUEST_NOT_FOUND) {
        reply() << "[错误] 未预料的管理指令，您可以通过「" ADMIN_COMMAND_SIGN "帮助」查看所有支持的管理指令";
    }
    return ret;
}

static ErrCode show_gamelist(BotCtx& bot, const UserID uid, const std::optional<GroupID>& gid,
                             MsgSenderBase& reply, const bool show_text)
{
    int i = 0;
    if (bot.game_handles().empty()) {
        reply() << "未载入任何游戏";
        return EC_OK;
    }
    if (show_text) {
        auto sender = reply();
        sender << "游戏列表：";
        for (const auto& [name, game_handle] : bot.game_handles()) {
            sender << "\n" << (++i) << ". " << name;
            if (DefaultMultiple(game_handle) == 0) {
                sender << "（试玩）";
            }
        }
    } else {
        html::Table table(0, 5);
        table.SetTableStyle(" align=\"center\" border=\"1px solid #ccc\" cellpadding=\"5\" cellspacing=\"1\" ");
        const auto game_handles_range = std::views::transform(bot.game_handles(), [](const auto& p) { return &p; });
        auto game_handles = std::vector(std::ranges::begin(game_handles_range), std::ranges::end(game_handles_range));
        std::ranges::sort(game_handles,
                [](const auto& _1, const auto& _2) { return _1->second.Activity() > _2->second.Activity(); });
        constexpr uint32_t k_max_game_in_one_image = 20;
        uint32_t i = 0;
        auto send_image =
            [&, img_i = 0, img_num = (game_handles.size() + k_max_game_in_one_image - 1) / k_max_game_in_one_image]() mutable
            {
                reply() << Markdown("## 游戏列表\n\n" + table.ToString() + "<br>第 " + std::to_string(++img_i) + " 张 / 共 " +
                        std::to_string(img_num) + " 张", 800);
            };
        for (const auto& p : game_handles) {
            if (i++ == k_max_game_in_one_image) {
                send_image();
                table.ResizeRow(0);
                i = 1;
            }
            const auto& name = p->first;
            const auto& game_handle = p->second;
            const auto locked_options = game_handle.DefaultGameOptions().Lock();
            const auto default_multiple =
                locked_options->generic_options_.is_formal_ ? game_handle.Info().multiple_fn_(locked_options->game_options_.get())
                                                            : 0;
            const auto default_max_player = game_handle.Info().max_player_num_fn_(locked_options->game_options_.get());
            table.AppendRow();
            table.AppendRow();
            table.MergeDown(table.Row() - 2, 0, 2);
            table.MergeDown(table.Row() - 2, 4, 2);
            table.MergeRight(table.Row() - 1, 1, 3);
            table.Get(table.Row() - 2, 0).SetContent("<font size=\"5\"> **" + name + "**</font>\n\n热度：" + std::to_string(game_handle.Activity()));
            table.Get(table.Row() - 2, 1).SetContent("开发者：" + game_handle.Info().developer_);
            table.Get(table.Row() - 2, 2).SetContent(default_max_player == 0 ? "无玩家数限制" :
                    ("最多 " HTML_COLOR_FONT_HEADER(blue) "**" + std::to_string(default_max_player) + "**" HTML_FONT_TAIL " 名玩家"));
            table.Get(table.Row() - 2, 3).SetContent(default_multiple == 0 ? "默认不计分" :
                    ("默认 " HTML_COLOR_FONT_HEADER(blue) "**" + std::to_string(default_multiple) + "**" HTML_FONT_TAIL " 倍分数"));
            table.Get(table.Row() - 2, 4).SetContent("<img src=\"file:///" +
                    (std::filesystem::absolute(bot.game_path()) / game_handle.Info().module_name_ / "icon.png").string() +
                    "\" style=\"width:70px; height:70px; vertical-align: middle;\">");
            table.Get(table.Row() - 1, 1).SetContent("<font size=\"3\"> " + game_handle.Info().description_ + "</font>");
        }
        send_image();
    }
    return EC_OK;
}

static ErrCode new_game(BotCtx& bot, const UserID uid, const std::optional<GroupID>& gid, MsgSenderBase& reply,
                        const std::string& gamename, const std::vector<std::string>& args)
{
    const auto it = bot.game_handles().find(gamename);
    if (it == bot.game_handles().end()) {
        reply() << "[错误] 创建失败：未知的游戏名，请通过「" META_COMMAND_SIGN "游戏列表」查看游戏名称";
        return EC_REQUEST_UNKNOWN_GAME;
    }
    if (gid.has_value()) {
        const auto running_match = bot.match_manager().GetMatch(*gid);
        ErrCode rc = EC_OK;
        if (running_match && (rc = running_match->Terminate(false /*is_force*/)) != EC_OK) {
            reply() << "[错误] 创建失败：该房间已经开始游戏";
            return rc;
        }
    }
    const std::string binded_args = std::accumulate(args.begin(), args.end(), std::string{},
            [](std::string&& s, const std::string& arg) { return std::move(s) + arg + " "; });
    return bot.match_manager().NewMatch(it->second, binded_args, uid, gid, reply);
}

template <typename Fn>
static auto handle_match_by_user(BotCtx& bot, const UserID uid, const std::optional<GroupID>& gid, MsgSenderBase& reply,
        const Fn& fn, const char* const action_name = "")
{
    const auto match = bot.match_manager().GetMatch(uid);
    if (!match) {
        reply() << "[错误] " << action_name << "失败：您未加入游戏";
        return EC_MATCH_USER_NOT_IN_MATCH;
    }
    if (gid.has_value() && match->gid() != *gid) {
        reply() << "[错误] " << action_name << "失败：您是在其他房间创建的游戏，若您忘记该房间，可以尝试私信裁判";
        return EC_MATCH_NOT_THIS_GROUP;
    }
    return fn(match);
}

static ErrCode set_bench_to(BotCtx& bot, const UserID uid, const std::optional<GroupID>& gid, MsgSenderBase& reply,
        const uint32_t bench_to_player_num)
{
    return handle_match_by_user(bot, uid, gid, reply,
            [&](const auto& match) { return match->SetBenchTo(uid, reply, bench_to_player_num); }, "配置");
}

static ErrCode set_formal(BotCtx& bot, const UserID uid, const std::optional<GroupID>& gid, MsgSenderBase& reply,
        const bool is_formal)
{
    return handle_match_by_user(bot, uid, gid, reply,
            [&](const auto& match) { return match->SetFormal(uid, reply, is_formal); }, "配置");
}

static ErrCode set_multiple(BotCtx& bot, const UserID uid, const std::optional<GroupID>& gid, MsgSenderBase& reply,
        const uint32_t multiple)
{
    reply() << "[错误] 配置失败：机器人已经不再支持自定义倍率，但您现在可以通过「" META_COMMAND_SIGN "计分 开启」和"
               "「" META_COMMAND_SIGN "计分 关闭」控制该场比赛是否计分";
    return EC_INVALID_ARGUMENT;
}

static ErrCode start_game(BotCtx& bot, const UserID uid, const std::optional<GroupID>& gid, MsgSenderBase& reply)
{
    return handle_match_by_user(bot, uid, gid, reply,
            [&](const auto& match) { return match->GameStart(uid, reply); }, "开始");
}

static ErrCode leave(BotCtx& bot, const UserID uid, const std::optional<GroupID>& gid, MsgSenderBase& reply,
        const bool force)
{
    return handle_match_by_user(bot, uid, gid, reply,
            [&](const auto& match) { return match->Leave(uid, reply, force); }, "退出");
}

static ErrCode user_interrupt_game(BotCtx& bot, const UserID uid, const std::optional<GroupID>& gid, MsgSenderBase& reply,
        const bool cancel)
{
    return handle_match_by_user(bot, uid, gid, reply,
            [&](const auto& match) { return match->UserInterrupt(uid, reply, cancel); }, "中断");
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

static ErrCode show_matches(BotCtx& bot, const UserID uid, const std::optional<GroupID> gid, MsgSenderBase& reply)
{
    const auto matches = bot.match_manager().Matches();
    const auto show_table = [&](const bool is_private)
    {
        html::Table table(1, 6);
        table.SetTableStyle(" align=\"center\" border=\"1px solid #ccc\" cellpadding=\"1\" cellspacing=\"1\" width=\"650\"");
        table.Get(0, 0).SetContent("**序号**");
        table.Get(0, 1).SetContent(is_private ? "**ID**" : "**房间**");
        table.Get(0, 2).SetContent("**房主**");
        table.Get(0, 3).SetContent("**游戏**");
        table.Get(0, 4).SetContent("**人数**");
        table.Get(0, 5).SetContent("**状态**");
        for (const auto& match : matches) {
            const auto gid = match->gid();
            if (is_private ^ gid.has_value()) {
                table.AppendRow();
                table.GetLastRow(0).SetContent(std::to_string(table.Row() - 1));
                if (gid.has_value()) {
                    table.GetLastRow(1).SetContent(gid->GetStr());
                } else {
                    table.GetLastRow(1).SetContent(std::to_string(match->MatchId()));
                }
                const auto uid = match->HostUserId();
                table.GetLastRow(2).SetContent(bot.GetUserAvatar(uid.GetCStr(), 25) + HTML_ESCAPE_SPACE + bot.GetUserName(uid.GetCStr(), nullptr));
                table.GetLastRow(3).SetContent(match->game_handle().Info().name_);
                table.GetLastRow(4).SetContent(std::to_string(match->UserNum()));
                const auto state = match->state();
                table.GetLastRow(5).SetContent(
                        state == Match::NOT_STARTED ? HTML_COLOR_FONT_HEADER(red) "未开始" HTML_FONT_TAIL :
                        state == Match::IS_STARTED  ? HTML_COLOR_FONT_HEADER(green) "进行中" HTML_FONT_TAIL :
                                                      HTML_COLOR_FONT_HEADER(blue) "已结束" HTML_FONT_TAIL);
            }
        }
        return table.Row() > 1 ? table.ToString() : "当前无比赛";
    };
    reply() << Markdown("## 赛事列表\n\n### 公开赛事\n\n" + show_table(false) + "\n\n### 私密赛事\n\n" + show_table(true), 700);
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
    match->ShowInfo(reply);
    return EC_OK;
}

static ErrCode show_rule(BotCtx& bot, const UserID uid, const std::optional<GroupID> gid, MsgSenderBase& reply,
                         const std::string& gamename, const bool show_text)
{
    const auto it = bot.game_handles().find(gamename);
    if (it == bot.game_handles().end()) {
        reply() << "[错误] 查看失败：未知的游戏名，请通过「" META_COMMAND_SIGN "游戏列表」查看游戏名称";
        return EC_REQUEST_UNKNOWN_GAME;
    };
    if (!show_text) {
        reply() << Markdown(it->second.Info().rule_);
        return EC_OK;
    }
    auto sender = reply();
    sender << "最多可参加人数：";
    if (const auto max_player = DefaultMaxPlayer(it->second); max_player == 0) {
        sender << "无限制";
    } else {
        sender << max_player;
    }
    sender << "人\n";
    sender << "详细规则：\n";
    sender << it->second.Info().rule_;
    return EC_OK;
}

static ErrCode show_custom_rule(BotCtx& bot, const UserID uid, const std::optional<GroupID> gid, MsgSenderBase& reply,
                         const std::string& gamename, const std::vector<std::string>& args)
{
    const auto it = bot.game_handles().find(gamename);
    if (it == bot.game_handles().end()) {
        reply() << "[错误] 查看失败：未知的游戏名，请通过「" META_COMMAND_SIGN "游戏列表」查看游戏名称";
        return EC_REQUEST_UNKNOWN_GAME;
    };
    std::string s;
    for (const auto& arg : args) {
        s += arg;
        s += " ";
    }
    const char* const result = it->second.Info().handle_rule_command_fn_(s.c_str());
    if (!result) {
        reply() << "[错误] 查看失败：未知的规则指令，请通过「" META_COMMAND_SIGN "规则 " << gamename << "」查看具体规则指令";
        return EC_INVALID_ARGUMENT;
    }
    reply() << result;
    return EC_OK;
}

static ErrCode show_achievement(BotCtx& bot, const UserID uid, const std::optional<GroupID> gid, MsgSenderBase& reply,
                         const std::string& gamename)
{
    if (!bot.db_manager()) {
        reply() << "[错误] 查看失败：未连接数据库";
        return EC_DB_NOT_CONNECTED;
    }
    const auto it = bot.game_handles().find(gamename);
    if (it == bot.game_handles().end()) {
        reply() << "[错误] 查看失败：未知的游戏名，请通过「" META_COMMAND_SIGN "游戏列表」查看游戏名称";
        return EC_REQUEST_UNKNOWN_GAME;
    };
    if (it->second.Info().achievements_.empty()) {
        reply() << "该游戏没有任何成就";
        return EC_OK;
    }
    html::Table table(0, 3);
    table.SetTableStyle(" align=\"center\" border=\"1px solid #ccc\" cellpadding=\"5\" cellspacing=\"1\" ");
    for (const auto& [achievement_name, description] : it->second.Info().achievements_) {
        const auto statistic = bot.db_manager()->GetAchievementStatistic(uid, gamename, achievement_name);
        const std::string color_header = statistic.count_ > 0 ? HTML_COLOR_FONT_HEADER(green) : HTML_COLOR_FONT_HEADER(black);
        table.AppendRow();
        table.GetLastRow(0).SetContent("<nobr>" + color_header + "<font size=\"5\"> **" + achievement_name + "**</font>\n\n达成人数：" +
                std::to_string(statistic.achieved_user_num_) + HTML_FONT_TAIL);
        if (statistic.count_ > 0) {
            table.GetLastRow(1).SetContent(color_header + " 首次达成时间：**" + statistic.first_achieve_time_ + "**" HTML_FONT_TAIL);
            table.GetLastRow(2).SetContent(color_header + " 达成次数：**" + std::to_string(statistic.count_) + "**" HTML_FONT_TAIL);
            table.AppendRow();
            table.MergeDown(table.Row() - 2, 0, 2);
        }
        table.MergeRight(table.Row() - 1, 1, 2);
        table.GetLastRow(1).SetContent(color_header + "<font size=\"3\"> " + description + "</font>" HTML_FONT_TAIL);
    }
    reply() << Markdown(std::string("## ") + bot.GetUserAvatar(uid.GetCStr(), 40) + HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE +
        bot.GetUserName(uid.GetCStr(), gid.has_value() ? gid->GetCStr() : nullptr) + "——" HTML_COLOR_FONT_HEADER(blue) +
        gamename + HTML_FONT_TAIL "成就一览\n\n" + table.ToString(), 800);
    return EC_OK;
}

static ErrCode show_game_options(BotCtx& bot, const UserID uid, const std::optional<GroupID> gid, MsgSenderBase& reply,
        const std::string& gamename, const bool text_mode)
{
    const auto it = bot.game_handles().find(gamename);
    if (it == bot.game_handles().end()) {
        reply() << "[错误] 查看失败：未知的游戏名，请通过「" META_COMMAND_SIGN "游戏列表」查看游戏名称";
        return EC_REQUEST_UNKNOWN_GAME;
    };
    const std::string outstr = std::string("### 「") + gamename + "」配置选项" +
        it->second.DefaultGameOptions().Lock()->game_options_->Info(true, !text_mode, (ADMIN_COMMAND_SIGN "配置 " + gamename + " ").c_str());
    if (text_mode) {
        reply() << outstr;
    } else {
        reply() << Markdown(outstr);
    }
    return EC_OK;
}

static ErrCode about(BotCtx& bot, const UserID uid, const std::optional<GroupID> gid, MsgSenderBase& reply)
{
    auto sender = reply();
    sender << "LGTBot ";
    sender << LGTBot_Version();
    sender << "\n"
              "\n作者：森高（QQ：654867229）"
              "\nGitHub：http://github.com/slontia/lgtbot"
              "\n"
              "\n若您使用中遇到任何 BUG 或其它问题，欢迎私信作者，或前往 GitHub 主页提 issue"
              "\n本项目仅供娱乐和技术交流，请勿用于商业用途，健康游戏，拒绝赌博";
    return EC_OK;
}

static ErrCode show_profile(BotCtx& bot, const UserID uid, const std::optional<GroupID> gid,
                            MsgSenderBase& reply, const TimeRange time_range)
{
    if (!bot.db_manager()) {
        reply() << "[错误] 查看失败：未连接数据库";
        return EC_DB_NOT_CONNECTED;
    }
    const auto profile = bot.db_manager()->GetUserProfile(uid,
            k_time_range_begin_datetimes[time_range.ToUInt()], k_time_range_end_datetimes[time_range.ToUInt()]);  // TODO: pass sender

    const auto colored_text = [](const auto score, std::string text)
        {
            std::string s;
            if (score < 0) {
                s = HTML_COLOR_FONT_HEADER(red);
            } else if (score > 0) {
                s = HTML_COLOR_FONT_HEADER(green);
            }
            s += std::move(text);
            if (score != 0) {
                s += HTML_FONT_TAIL;
            }
            return s;
        };

    std::string html = std::string("## ") + bot.GetUserAvatar(uid.GetCStr(), 40) + HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE +
        bot.GetUserName(uid.GetCStr(), gid.has_value() ? gid->GetCStr() : nullptr) + "\n";

    html += "\n- **注册时间**：" + (profile.birth_time_.empty() ? "无" : profile.birth_time_) + "\n";

    // title: season score info
    html += "\n<h3 align=\"center\">" HTML_COLOR_FONT_HEADER(blue);
    html += time_range.ToString();
    html += HTML_FONT_TAIL "赛季</h3>\n";

    // score info

    html += "\n- **游戏局数**：" + std::to_string(profile.match_count_);
    html += "\n- **零和总分**：" + colored_text(profile.total_zero_sum_score_, std::to_string(profile.total_zero_sum_score_));
    html += "\n- **头名总分**：" + colored_text(profile.total_top_score_, std::to_string(profile.total_top_score_));

    // game level score info

    html += "\n- **各游戏等级总分**：\n\n";
    if (profile.game_level_infos_.empty()) {
        html += "<p align=\"center\">您本赛季还没有参与过游戏</p>\n\n";
    } else {
        static constexpr const size_t k_level_score_table_num = 2;
        html::Table level_score_table_outside(1, k_level_score_table_num);
        level_score_table_outside.SetTableStyle(" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" width=\"800\" ");
        level_score_table_outside.SetRowStyle(" valign=\"top\" ");
        std::array<html::Table, k_level_score_table_num> level_score_table;
        const size_t game_level_info_num_each_table = (profile.game_level_infos_.size() + k_level_score_table_num - 1) / k_level_score_table_num;
        for (size_t i = 0; i < k_level_score_table_num; ++i) {
            auto& table = level_score_table[i];
            table.SetTableStyle(" align=\"center\" border=\"1px solid #ccc\" cellpadding=\"1\" cellspacing=\"1\" ");
            table.AppendRow();
            table.AppendColumn();
            table.Get(0, 0).SetContent("**序号**");
            table.AppendColumn();
            table.Get(0, 1).SetContent("**游戏名称**");
            table.AppendColumn();
            table.Get(0, 2).SetContent("**局数**");
            table.AppendColumn();
            table.Get(0, 3).SetContent("**等级总分**");
            table.AppendColumn();
            table.Get(0, 4).SetContent("**加权等级总分**");
            table.AppendColumn();
            table.Get(0, 5).SetContent("**评级**");
        }
        for (size_t i = 0; i < profile.game_level_infos_.size(); ++i) {
            const auto& info = profile.game_level_infos_[i];
            const int32_t total_level_score_ = static_cast<int32_t>(info.total_level_score_);
            auto& table = level_score_table[i / game_level_info_num_each_table];
            table.AppendRow();
            table.GetLastRow(0).SetContent(colored_text(total_level_score_ / 100,  std::to_string(i + 1)));
            table.GetLastRow(1).SetContent(colored_text(total_level_score_ / 100, info.game_name_));
            table.GetLastRow(2).SetContent(colored_text(total_level_score_ / 100, std::to_string(info.count_)));
            table.GetLastRow(3).SetContent(colored_text(total_level_score_ / 100, std::to_string(info.total_level_score_)));
            table.GetLastRow(4).SetContent(colored_text(total_level_score_ / 100, std::to_string(std::sqrt(info.count_) * info.total_level_score_)));
            table.GetLastRow(5).SetContent(colored_text(total_level_score_ / 100,
                        total_level_score_ <= -300 ? "E"  :
                        total_level_score_ <= -100 ? "D"  :
                        total_level_score_ < 100   ? "C"  :
                        total_level_score_ < 300   ? "B"  :
                        total_level_score_ < 500   ? "A"  :
                        total_level_score_ < 700   ? "S"  :
                        total_level_score_ < 1000  ? "SS" : "SSS"));
        }
        for (size_t i = 0; i < k_level_score_table_num; ++i) {
            level_score_table_outside.Get(0, i).SetContent(level_score_table[i].ToString());
        }
        html += "\n\n" + level_score_table_outside.ToString() + "\n\n";
    }

    // title: recent info

    html += "\n<h3 align=\"center\">近期战绩</h3>\n";

    // show recent matches

    html += "\n- **近十场游戏记录**：\n\n";
    if (profile.recent_matches_.empty()) {
        html += "<p align=\"center\">您还没有参与过游戏</p>\n\n";
    } else {
        html::Table recent_matches_table(1, 9);
        recent_matches_table.SetTableStyle(" align=\"center\" border=\"1px solid #ccc\" cellpadding=\"1\" cellspacing=\"1\" width=\"800\" ");
        recent_matches_table.Get(0, 0).SetContent("**序号**");
        recent_matches_table.Get(0, 1).SetContent("**游戏名称**");
        recent_matches_table.Get(0, 2).SetContent("**结束时间**");
        recent_matches_table.Get(0, 3).SetContent("**等价排名**");
        recent_matches_table.Get(0, 4).SetContent("**倍率**");
        recent_matches_table.Get(0, 5).SetContent("**游戏得分**");
        recent_matches_table.Get(0, 6).SetContent("**零和得分**");
        recent_matches_table.Get(0, 7).SetContent("**头名得分**");
        recent_matches_table.Get(0, 8).SetContent("**等级得分**");

        for (uint32_t i = 0; i < profile.recent_matches_.size(); ++i) {
            recent_matches_table.AppendRow();
            const auto match_profile = profile.recent_matches_[i];
            recent_matches_table.Get(i + 1, 0).SetContent(colored_text(match_profile.top_score_, std::to_string(i + 1)));
            recent_matches_table.Get(i + 1, 1).SetContent(colored_text(match_profile.top_score_, match_profile.game_name_));
            recent_matches_table.Get(i + 1, 2).SetContent(colored_text(match_profile.top_score_, match_profile.finish_time_));
            recent_matches_table.Get(i + 1, 3).SetContent(colored_text(match_profile.top_score_, [&match_profile]()
                        {
                            std::stringstream ss;
                            ss.precision(2);
                            ss << (match_profile.user_count_ - float(match_profile.rank_score_) / 2 + 0.5) << " / " << match_profile.user_count_;
                            return ss.str();
                        }()));
            recent_matches_table.Get(i + 1, 4).SetContent(colored_text(match_profile.top_score_, std::to_string(match_profile.multiple_) + " 倍"));
            recent_matches_table.Get(i + 1, 5).SetContent(colored_text(match_profile.top_score_, std::to_string(match_profile.game_score_)));
            recent_matches_table.Get(i + 1, 6).SetContent(colored_text(match_profile.top_score_, std::to_string(match_profile.zero_sum_score_)));
            recent_matches_table.Get(i + 1, 7).SetContent(colored_text(match_profile.top_score_, std::to_string(match_profile.top_score_)));
            recent_matches_table.Get(i + 1, 8).SetContent(colored_text(match_profile.top_score_, std::to_string(match_profile.level_score_)));
        }
        html += recent_matches_table.ToString() + "\n\n";
    }

    // show recent honors

    html += "\n- **近十次荣誉记录**：\n\n";
    if (profile.recent_honors_.empty()) {
        html += "<p align=\"center\">您还没有获得过荣誉</p>\n\n";
    } else {
        html::Table recent_honors_table(1, 3);
        recent_honors_table.SetTableStyle(" align=\"center\" border=\"1px solid #ccc\" cellpadding=\"1\" cellspacing=\"1\" width=\"800\" ");
        recent_honors_table.Get(0, 0).SetContent("**ID**");
        recent_honors_table.Get(0, 1).SetContent("**荣誉**");
        recent_honors_table.Get(0, 2).SetContent("**获得时间**");
        for (const auto& info : profile.recent_honors_) {
            recent_honors_table.AppendRow();
            recent_honors_table.GetLastRow(0).SetContent(std::to_string(info.id_));
            recent_honors_table.GetLastRow(1).SetContent(info.description_);
            recent_honors_table.GetLastRow(2).SetContent(info.time_);
        }
        html += recent_honors_table.ToString() + "\n\n";
    }

    // show recent achievements

    html += "\n- **近十次成就记录**：\n\n";
    if (profile.recent_achievements_.empty()) {
        html += "<p align=\"center\">您还没有获得过成就</p>\n\n";
    } else {
        html::Table recent_honors_table(1, 5);
        recent_honors_table.SetTableStyle(" align=\"center\" border=\"1px solid #ccc\" cellpadding=\"1\" cellspacing=\"1\" width=\"800\" ");
        recent_honors_table.Get(0, 0).SetContent("**序号**");
        recent_honors_table.Get(0, 1).SetContent("**游戏名称**");
        recent_honors_table.Get(0, 2).SetContent("**成就名称**");
        recent_honors_table.Get(0, 3).SetContent("**成就描述**");
        recent_honors_table.Get(0, 4).SetContent("**获得时间**");
        for (size_t i = 0; i < profile.recent_achievements_.size(); ++i) {
            const auto& info = profile.recent_achievements_[i];
            recent_honors_table.AppendRow();
            recent_honors_table.GetLastRow(0).SetContent(std::to_string(i + 1));
            recent_honors_table.GetLastRow(1).SetContent(info.game_name_);
            recent_honors_table.GetLastRow(2).SetContent(info.achievement_name_);
            if (const auto it = bot.game_handles().find(info.game_name_); it == bot.game_handles().end()) {
                recent_honors_table.GetLastRow(3).SetContent("???");
            } else {
                for (const auto& [name, description] : it->second.Info().achievements_) {
                    if (name == info.achievement_name_) {
                        recent_honors_table.GetLastRow(3).SetContent(description);
                        break;
                    }
                }
            }
            recent_honors_table.GetLastRow(4).SetContent(info.time_);
        }
        html += recent_honors_table.ToString() + "\n\n";
    }

    // reply image

    reply() << Markdown(html, 850);

    return EC_OK;
}

static ErrCode clear_profile(BotCtx& bot, const UserID uid, const std::optional<GroupID> gid, MsgSenderBase& reply)
{
    static constexpr const uint32_t k_required_match_num = 3;
    if (!bot.db_manager()) {
        reply() << "[错误] 重来失败：未连接数据库";
        return EC_DB_NOT_CONNECTED;
    }
    if (!bot.db_manager()->Suicide(uid, k_required_match_num)) {
        reply() << "[错误] 重来失败：清除战绩，需最近三局比赛均取得正零和分的收益";
        return EC_USER_SUICIDE_FAILED;
    }
    reply() << bot.GetUserName(uid.GetCStr(), gid.has_value() ? gid->GetCStr() : nullptr) << "，凋零！";
    return EC_OK;
}

template <typename V>
static std::string print_score(BotCtx& bot, const V& vec, const std::optional<GroupID> gid, const std::string_view& unit = "分")
{
    std::string s;
    for (uint64_t i = 0; i < vec.size(); ++i) {
        s += "\n" + std::to_string(i + 1) + "位：" +
                bot.GetUserName(vec[i].first.GetCStr(), gid.has_value() ? gid->GetCStr() : nullptr) +
                "【" + std::to_string(vec[i].second) + " " + unit.data() + "】";
    }
    return s;
};

template <typename V>
static std::string print_score_in_table(BotCtx& bot, const std::string_view& score_name, const V& vec,
        const std::optional<GroupID> gid, const std::string_view& unit = "分")
{
    html::Table table(2 + vec.size(), 3);
    table.SetTableStyle(" align=\"center\" border=\"1px solid #ccc\" cellpadding=\"1\" cellspacing=\"1\" width=\"400\"");
    table.MergeRight(0, 0, 3);
    table.Get(0, 0).SetContent(std::string("**" HTML_COLOR_FONT_HEADER(blue)) + score_name.data() +
            HTML_FONT_TAIL "排行**");
    table.Get(1, 0).SetContent("**排名**");
    table.Get(1, 1).SetContent("**用户**");
    table.Get(1, 2).SetContent(std::string("**") + score_name.data() + "**");
    for (uint64_t i = 0; i < vec.size(); ++i) {
        const auto uid_cstr = vec[i].first.GetCStr();
        table.Get(2 + i, 0).SetContent(std::to_string(i + 1) + " 位");
        table.Get(2 + i, 1).SetContent(
                "<p align=\"left\">" HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE + bot.GetUserAvatar(uid_cstr, 30) +
                HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE + bot.GetUserName(uid_cstr, gid.has_value() ? gid->GetCStr() : nullptr) +
                "</p>");
        table.Get(2 + i, 2).SetContent(std::to_string(vec[i].second) + " " + unit.data());
    }
    return table.ToString();
};

static ErrCode show_rank(BotCtx& bot, const UserID uid, const std::optional<GroupID> gid, MsgSenderBase& reply)
{
    if (!bot.db_manager()) {
        reply() << "[错误] 查看失败：未连接数据库";
        return EC_DB_NOT_CONNECTED;
    }
    std::string s;
    for (const auto time_range : TimeRange::Members()) {
        const auto info = bot.db_manager()->GetRank(
                k_time_range_begin_datetimes[time_range.ToUInt()], k_time_range_end_datetimes[time_range.ToUInt()]);
        s += "\n<h2 align=\"center\">" HTML_COLOR_FONT_HEADER(blue);
        s += time_range.ToString();
        s += HTML_FONT_TAIL "赛季排行</h2>\n";
        html::Table table(1, 3);
        table.SetTableStyle(" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" width=\"1250\" ");
        table.Get(0, 0).SetContent(print_score_in_table(bot, "零和总分", info.zero_sum_score_rank_, gid));
        table.Get(0, 1).SetContent(print_score_in_table(bot, "头名总分", info.top_score_rank_, gid));
        table.Get(0, 2).SetContent(print_score_in_table(bot, "游戏局数", info.match_count_rank_, gid, "场"));
        s += "\n\n" + table.ToString() + "\n\n";
    }
    reply() << Markdown(s, 1300);
    return EC_OK;
}

static ErrCode show_rank_time_range(BotCtx& bot, const UserID uid, const std::optional<GroupID> gid, MsgSenderBase& reply,
        const TimeRange time_range)
{
    if (!bot.db_manager()) {
        reply() << "[错误] 查看失败：未连接数据库";
        return EC_DB_NOT_CONNECTED;
    }
    const auto info = bot.db_manager()->GetRank(
            k_time_range_begin_datetimes[time_range.ToUInt()], k_time_range_end_datetimes[time_range.ToUInt()]);
    reply() << "## 零和得分排行（" << time_range << "赛季）：\n" << print_score(bot, info.zero_sum_score_rank_, gid);
    reply() << "## 头名得分排行（" << time_range << "赛季）：\n" << print_score(bot, info.top_score_rank_, gid);
    reply() << "## 游戏局数排行（" << time_range << "赛季）：\n" << print_score(bot, info.match_count_rank_, gid, "场");
    return EC_OK;
}

static ErrCode show_game_rank(BotCtx& bot, const UserID uid, const std::optional<GroupID> gid, MsgSenderBase& reply,
        const std::string& game_name)
{
    if (!bot.db_manager()) {
        reply() << "[错误] 查看失败：未连接数据库";
        return EC_DB_NOT_CONNECTED;
    }
    if (bot.game_handles().find(game_name) == bot.game_handles().end()) {
        reply() << "[错误] 查看失败：未知的游戏名，请通过「" META_COMMAND_SIGN "游戏列表」查看游戏名称";
        return EC_REQUEST_UNKNOWN_GAME;
    }
    std::string s;
    for (const auto time_range : TimeRange::Members()) {
        const auto info = bot.db_manager()->GetLevelScoreRank(game_name,
                k_time_range_begin_datetimes[time_range.ToUInt()], k_time_range_end_datetimes[time_range.ToUInt()]);
        s += "\n<h2 align=\"center\">" HTML_COLOR_FONT_HEADER(blue);
        s += time_range.ToString();
        s += HTML_FONT_TAIL "赛季";
        s += HTML_COLOR_FONT_HEADER(blue);
        s += game_name;
        s += HTML_FONT_TAIL "排行</h2>\n";
        html::Table table(1, 3);
        table.SetTableStyle(" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" width=\"1250\" ");
        table.Get(0, 0).SetContent(print_score_in_table(bot, "等级总分", info.level_score_rank_, gid));
        table.Get(0, 1).SetContent(print_score_in_table(bot, "加权等级总分", info.weight_level_score_rank_, gid));
        table.Get(0, 2).SetContent(print_score_in_table(bot, "游戏局数", info.match_count_rank_, gid, "场"));
        s += "\n\n" + table.ToString() + "\n\n";
    }
    reply() << Markdown(s, 1300);
    return EC_OK;
}

static ErrCode show_game_rank_range_time(BotCtx& bot, const UserID uid, const std::optional<GroupID> gid, MsgSenderBase& reply,
        const std::string& game_name, const TimeRange time_range)
{
    if (!bot.db_manager()) {
        reply() << "[错误] 查看失败：未连接数据库";
        return EC_DB_NOT_CONNECTED;
    }
    if (bot.game_handles().find(game_name) == bot.game_handles().end()) {
        reply() << "[错误] 查看失败：未知的游戏名，请通过「" META_COMMAND_SIGN "游戏列表」查看游戏名称";
        return EC_REQUEST_UNKNOWN_GAME;
    }
    const auto info = bot.db_manager()->GetLevelScoreRank(game_name, k_time_range_begin_datetimes[time_range.ToUInt()],
            k_time_range_end_datetimes[time_range.ToUInt()]);
    reply() << "## 等级得分排行（" << time_range << "赛季）：\n" << print_score(bot, info.level_score_rank_, gid);
    reply() << "## 加权等级得分排行（" << time_range << "赛季）：\n" << print_score(bot, info.weight_level_score_rank_, gid);
    reply() << "## 游戏局数排行（" << time_range << "赛季）：\n" << print_score(bot, info.match_count_rank_, gid, "场");
    return EC_OK;
}

static ErrCode show_honors(BotCtx& bot, const UserID uid, const std::optional<GroupID> gid, MsgSenderBase& reply,
        const std::string& keyword)
{
    static constexpr const uint32_t k_limit = 20;
    if (!bot.db_manager()) {
        reply() << "[错误] 查看失败：未连接数据库";
        return EC_DB_NOT_CONNECTED;
    }
    html::Table table(1, 4);
    table.SetTableStyle(" align=\"center\" border=\"1px solid #ccc\" cellpadding=\"3\" cellspacing=\"3\" ");
    table.Get(0, 0).SetContent("**ID**");
    table.Get(0, 1).SetContent("**用户**");
    table.Get(0, 2).SetContent("**荣誉**");
    table.Get(0, 3).SetContent("**获得时间**");
    for (const auto& info : bot.db_manager()->GetHonors(keyword, k_limit)) {
        table.AppendRow();
        table.GetLastRow(0).SetContent(std::to_string(info.id_));
        table.GetLastRow(1).SetContent(bot.GetUserAvatar(info.uid_.GetCStr(), 25) + HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE +
                bot.GetUserName(info.uid_.GetCStr(), gid.has_value() ? gid->GetCStr() : nullptr));
        table.GetLastRow(2).SetContent(info.description_);
        table.GetLastRow(3).SetContent(info.time_);
    }
    reply() << Markdown("## 荣誉列表\n\n" + table.ToString() + "\n\n只显示前 " + std::to_string(k_limit) + " 条荣誉信息", 800);
    return EC_OK;
}

const std::vector<MetaCommandGroup> meta_cmds = {
    {
        "信息查看", { // GAME INFO: can be executed at any time
            make_command("查看帮助", help<false>, VoidChecker(META_COMMAND_SIGN "帮助"),
                        OptionalDefaultChecker<BoolChecker>(false, "文字", "图片")),
            make_command("查看游戏列表", show_gamelist, VoidChecker(META_COMMAND_SIGN "游戏列表"),
                        OptionalDefaultChecker<BoolChecker>(false, "文字", "图片")),
            make_command("查看游戏规则（游戏名称可以通过「" META_COMMAND_SIGN "游戏列表」查看）", show_rule, VoidChecker(META_COMMAND_SIGN "规则"),
                        AnyArg("游戏名称", "猜拳游戏"), OptionalDefaultChecker<BoolChecker>(false, "文字", "图片")),
            make_command("查看游戏具体游戏规则（游戏名称可以通过「" META_COMMAND_SIGN "游戏列表」查看）", show_custom_rule, VoidChecker(META_COMMAND_SIGN "规则"),
                        AnyArg("游戏名称", "猜拳游戏"), RepeatableChecker<AnyArg>("规则指令", "某指令")),
            make_command("查看游戏成就（游戏名称可以通过「" META_COMMAND_SIGN "游戏列表」查看）", show_achievement, VoidChecker(META_COMMAND_SIGN "成就"),
                        AnyArg("游戏名称", "猜拳游戏")),
            make_command("查看游戏配置信息（游戏名称可以通过「" META_COMMAND_SIGN "游戏列表」查看）", show_game_options, VoidChecker(META_COMMAND_SIGN "配置"),
                        AnyArg("游戏名称", "猜拳游戏"), OptionalDefaultChecker<BoolChecker>(false, "文字", "图片")),
            make_command("查看已加入，或该房间正在进行的比赛信息", show_match_info, VoidChecker(META_COMMAND_SIGN "游戏信息")),
            make_command("查看比赛列表", show_matches, VoidChecker(META_COMMAND_SIGN "赛事列表")),
            make_command("关于机器人", about, VoidChecker(META_COMMAND_SIGN "关于")),
        }
    },
    {
        "战绩情况", { // SCORE INFO: can be executed at any time
            make_command("查看个人战绩", show_profile, VoidChecker(META_COMMAND_SIGN "战绩"),
                    OptionalDefaultChecker<EnumChecker<TimeRange>>(TimeRange::总)),
            make_command("清除个人战绩", clear_profile, VoidChecker(META_COMMAND_SIGN "人生重来算了")),
            make_command("查看排行榜", show_rank, VoidChecker(META_COMMAND_SIGN "排行大图")),
            make_command("查看某个赛季粒度排行榜", show_rank_time_range, VoidChecker(META_COMMAND_SIGN "排行"),
                    OptionalDefaultChecker<EnumChecker<TimeRange>>(TimeRange::年)),
            make_command("查看单个游戏等级积分排行榜", show_game_rank, VoidChecker(META_COMMAND_SIGN "排行大图"),
                    AnyArg("游戏名称", "猜拳游戏")),
            make_command("查看单个游戏某个赛季粒度等级积分排行榜", show_game_rank_range_time, VoidChecker(META_COMMAND_SIGN "排行"),
                    AnyArg("游戏名称", "猜拳游戏"), OptionalDefaultChecker<EnumChecker<TimeRange>>(TimeRange::年)),
            make_command("查看所有荣誉", show_honors, VoidChecker(META_COMMAND_SIGN "荣誉列表"), OptionalDefaultChecker<AnyArg>("", "关键词")),
        }
    },
    {
        "新建游戏", { // NEW GAME: can only be executed by host
            make_command("在当前房间建立公开游戏，或私信 bot 以建立私密游戏（游戏名称可以通过「" META_COMMAND_SIGN "游戏列表」查看）",
                        new_game, VoidChecker(META_COMMAND_SIGN "新游戏"), AnyArg("游戏名称", "猜拳游戏"),
                        RepeatableChecker<AnyArg>("预设指令", "某指令")),
            make_command("房主设置参与游戏的AI数量，使得玩家不低于一定数量（属于配置变更，会使得全部玩家退出游戏）",
                        set_bench_to, VoidChecker(META_COMMAND_SIGN "替补至"), ArithChecker<uint32_t>(2, 32, "数量")),
            make_command("（已废弃）房主调整分数倍率",
                        set_multiple, VoidChecker(META_COMMAND_SIGN "倍率"), ArithChecker<uint32_t>(0, 3, "倍率")),
            make_command("房主调整该局比赛是否计分",
                        set_formal, VoidChecker(META_COMMAND_SIGN "计分"), BoolChecker("开启", "关闭")),
            make_command("房主开始游戏", start_game, VoidChecker(META_COMMAND_SIGN "开始")),
        }
    },
    {
        "参与游戏", { // JOIN/LEAVE GAME: can only be executed by player
            make_command("加入当前房间的公开游戏", join_public, VoidChecker(META_COMMAND_SIGN "加入")),
            make_command("私信bot以加入私密游戏（可通过「" META_COMMAND_SIGN "私密游戏列表」查看比赛编号）", join_private, VoidChecker(META_COMMAND_SIGN "加入"),
                        BasicChecker<MatchID>("私密比赛编号", "1")),
            make_command("退出游戏（若附带了「强制」参数，则可以在游戏进行中退出游戏，需注意退出后无法继续参与原游戏）",
                        leave, VoidChecker(META_COMMAND_SIGN "退出"),  OptionalDefaultChecker<BoolChecker>(false, "强制", "常规")),
            make_command("在游戏中发起中断比赛", user_interrupt_game, VoidChecker(META_COMMAND_SIGN "中断"), OptionalDefaultChecker<BoolChecker>(false, "取消", "确定")),
        }
    }
};

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
    match->Terminate(true /*is_force*/);
    reply() << "中断成功";
    return EC_OK;
}

static ErrCode set_game_default_formal(BotCtx& bot, const UserID uid, const std::optional<GroupID> gid,
        MsgSenderBase& reply, const std::string& gamename, const bool is_formal)
{
    const auto it = bot.game_handles().find(gamename);
    if (it == bot.game_handles().end()) {
        reply() << "[错误] 查看失败：未知的游戏名，请通过「" META_COMMAND_SIGN "游戏列表」查看游戏名称";
        return EC_REQUEST_UNKNOWN_GAME;
    };
    it->second.DefaultGameOptions().Lock()->generic_options_.is_formal_ = is_formal;
    reply() << "设置成功，游戏默认" << (is_formal ? "开启" : "关闭") << "计分";
    bot.UpdateGameDefaultFormal(gamename, is_formal);
    return EC_OK;
}

static ErrCode show_others_profile(BotCtx& bot, const UserID uid, const std::optional<GroupID> gid,
        MsgSenderBase& reply, const std::string& others_uid, const TimeRange time_range)
{
    return show_profile(bot, others_uid, gid, reply, time_range);
}

static ErrCode clear_others_profile(BotCtx& bot, const UserID uid, const std::optional<GroupID> gid,
        MsgSenderBase& reply, const std::string& others_uid, const std::string& reason)
{
    if (!bot.db_manager()) {
        reply() << "[错误] 清除失败：未连接数据库";
        return EC_DB_NOT_CONNECTED;
    }
    if (!bot.db_manager()->Suicide(UserID{others_uid}, 0)) {
        reply() << "[错误] 清除失败：未知原因";
        return EC_USER_SUICIDE_FAILED;
    }
    bot.MakeMsgSender(UserID{others_uid})() << "非常抱歉，您的战绩已被强制清空，理由为「" << reason << "」\n如有疑问，请联系管理员";
    reply() << "战绩删除成功，且已通知该玩家！";
    return EC_OK;
}

static ErrCode set_bot_option(BotCtx& bot, const UserID uid, const std::optional<GroupID> gid,
        MsgSenderBase& reply, const std::string& option_name, const std::vector<std::string>& option_args)
{
    MsgReader reader(option_args);
    auto locked_option = bot.option().Lock(); // lock until updated config to ensure atomic write
    if (!locked_option->SetOption(option_name, reader)) {
        reply() << "[错误] 设置配置项失败，请通过「" ADMIN_COMMAND_SIGN "全局配置」确认配置项是否存在";
        return EC_INVALID_ARGUMENT;
    }
    reply() << "设置成功";
    bot.UpdateBotConfig(option_name, option_args);
    return EC_OK;
}

static ErrCode set_game_option(BotCtx& bot, const UserID uid, const std::optional<GroupID> gid,
        MsgSenderBase& reply, const std::string& game_name, const std::string& option_name,
        const std::vector<std::string>& option_args)
{
    const auto game_handle_it = bot.game_handles().find(game_name);
    if (game_handle_it == bot.game_handles().end()) {
        reply() << "[错误] 设置失败：未知的游戏名，请通过「" META_COMMAND_SIGN "游戏列表」查看游戏名称";
        return EC_REQUEST_UNKNOWN_GAME;
    }
    std::string option_str = option_name;
    for (const auto& option_arg : option_args) {
        option_str += " " + option_arg;
    }
    auto locked_option = game_handle_it->second.DefaultGameOptions().Lock(); // lock until updated config to ensure atomic write
    if (!locked_option->game_options_->SetOption(option_str.c_str())) {
        reply() << "[错误] 设置配置项失败，请通过「" META_COMMAND_SIGN "配置 " << game_name << "」确认配置项是否存在";
        return EC_INVALID_ARGUMENT;
    }
    reply() << "设置成功";
    bot.UpdateGameConfig(game_name, option_name, option_args);
    return EC_OK;
}

static ErrCode show_bot_options(BotCtx& bot, const UserID uid, const std::optional<GroupID> gid,
        MsgSenderBase& reply, const bool text_mode)
{
    const std::string outstr = "### 全局配置选项" + bot.option().Lock()->Info(true, !text_mode, ADMIN_COMMAND_SIGN "全局配置 ");
    if (text_mode) {
        reply() << outstr;
    } else {
        reply() << Markdown(outstr);
    }
    return EC_OK;
}

static ErrCode add_honor(BotCtx& bot, const UserID uid, const std::optional<GroupID> gid, MsgSenderBase& reply,
        const std::string& honor_uid, const std::string honor_desc)
{
    if (!bot.db_manager()) {
        reply() << "[错误] 添加失败：未连接数据库";
        return EC_DB_NOT_CONNECTED;
    }
    if (!bot.db_manager()->AddHonor(honor_uid, honor_desc)) {
        reply() << "[错误] 添加失败：未知原因";
        return EC_HONOR_ADD_FAILED;
    }
    reply() << "添加荣誉成功，恭喜" << At(UserID{honor_uid}) << "荣获「" << honor_desc << "」";
    return EC_OK;
}

static ErrCode delete_honor(BotCtx& bot, const UserID uid, const std::optional<GroupID> gid, MsgSenderBase& reply,
        const int32_t id)
{
    if (!bot.db_manager()) {
        reply() << "[错误] 删除失败：未连接数据库";
        return EC_DB_NOT_CONNECTED;
    }
    if (!bot.db_manager()->DeleteHonor(id)) {
        reply() << "[错误] 删除失败：未知原因";
        return EC_HONOR_ADD_FAILED;
    }
    reply() << "删除荣誉成功";
    return EC_OK;
}

const std::vector<MetaCommandGroup> admin_cmds = {
    {
        "信息查看", {
            make_command("查看帮助", help<true>, VoidChecker(ADMIN_COMMAND_SIGN "帮助"),
                        OptionalDefaultChecker<BoolChecker>(false, "文字", "图片")),
            make_command("查看他人战绩", show_others_profile, VoidChecker(ADMIN_COMMAND_SIGN "战绩"), AnyArg("用户 ID", "123456789"),
                        OptionalDefaultChecker<EnumChecker<TimeRange>>(TimeRange::总)),
        }
    },
    {
        "管理操作", {
            make_command("强制中断比赛", interrupt_game, VoidChecker(ADMIN_COMMAND_SIGN "中断"),
                        OptionalChecker<BasicChecker<MatchID>>("私密比赛编号")),
            make_command("清除他人战绩，并通知其具体理由", clear_others_profile, VoidChecker(ADMIN_COMMAND_SIGN "清除战绩"),
                        AnyArg("用户 ID", "123456789"), AnyArg("理由", "恶意刷分")),
        }
    },
    {
        "配置操作", {
            make_command("设置游戏计分默认开启/关闭", set_game_default_formal, VoidChecker(ADMIN_COMMAND_SIGN "计分"),
                        AnyArg("游戏名称", "猜拳游戏"), BoolChecker("开启", "关闭")),
            make_command("查看所有支持的配置项", show_bot_options, VoidChecker(ADMIN_COMMAND_SIGN "全局配置"),
                        OptionalDefaultChecker<BoolChecker>(false, "文字", "图片")),
            make_command("设置配置项（可通过「" ADMIN_COMMAND_SIGN "配置列表」查看所有支持的配置）", set_bot_option, VoidChecker(ADMIN_COMMAND_SIGN "全局配置"),
                        AnyArg("配置名称", "某配置"), RepeatableChecker<AnyArg>("配置参数", "参数")),
            make_command("设置配置项（可通过「" ADMIN_COMMAND_SIGN "配置列表」查看所有支持的配置）", set_game_option, VoidChecker(ADMIN_COMMAND_SIGN "配置"),
                        AnyArg("游戏名称", "猜拳游戏"), AnyArg("配置名称", "某配置"),
                        RepeatableChecker<AnyArg>("配置参数", "参数")),
        }
    },
    {
        "荣誉操作", {
            make_command("新增荣誉", add_honor, VoidChecker(ADMIN_COMMAND_SIGN "荣誉"), VoidChecker("新增"),
                        AnyArg("用户 ID", "123456789"), AnyArg("荣誉描述", "2022 年度某游戏年赛冠军")),
            make_command("新增荣誉", delete_honor, VoidChecker(ADMIN_COMMAND_SIGN "荣誉"), VoidChecker("删除"),
                        ArithChecker<int32_t>(0, INT32_MAX, "编号")),
        }
    },
};
