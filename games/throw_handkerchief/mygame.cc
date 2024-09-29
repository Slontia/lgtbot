// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"

using namespace std;

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;

const GameProperties k_properties {
    .name_ = "丢手帕",
    .developer_ = "铁蛋",
    .description_ = "玩家分别丢手帕和回头看，抓住最合适的时机才能获胜",
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 2; }
uint32_t Multiple(const CustomOptions& options) {
    return min(3U, max(GET_OPTION_VALUE(options, 模式) == 0 && GET_OPTION_VALUE(options, 生命) == 300 ? 1 : GET_OPTION_VALUE(options, 生命) / 150, 1U));
}
const MutableGenericOptions k_default_generic_options;
const std::vector<RuleCommand> k_rule_commands = {};

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("设定游戏模式",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options, const uint32_t& mode)
            {
                if (mode == 100) {
                    generic_options.bench_computers_to_player_num_ = 2;
                    return NewGameMode::SINGLE_USER;
                } else {
                    GET_OPTION_VALUE(game_options, 模式) = mode;
                    return NewGameMode::MULTIPLE_USERS;
                }
            },
            AlterChecker<uint32_t>({{"快速", 0}, {"高级", 1}, {"实时", 2}, {"单机", 100}})),
};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() != 2) {
        reply() << "该游戏为双人游戏，必须为2人参加，当前玩家数为 " << generic_options_readonly.PlayerNum();
        return false;
    }
    if (GET_OPTION_VALUE(game_options, 模式) == 1 && GET_OPTION_VALUE(game_options, 生命) == 180) {
        GET_OPTION_VALUE(game_options, 生命) = 300;
    }
    if (GET_OPTION_VALUE(game_options, 模式) == 2) {
        GET_OPTION_VALUE(game_options, 时限) = 60;
    }
    return true;
}

// ========== GAME STAGES ==========

class MainStage : public MainGameStage<>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility),
                MakeStageCommand(*this, "查看当前游戏进展情况", &MainStage::Status_, VoidChecker("赛况")),
                MakeStageCommand(*this, "设定 丢手帕/回头 的时间点", &MainStage::SetTime_, ArithChecker<int64_t>(0, 60, "时间")),
                MakeStageCommand(*this, "【仅实时模式】在最合适的时机进行行动！", &MainStage::RealTimeMode_, AlterChecker<int>({{"回", 0}, {"回头", 0}, {"回头看", 0}, {"丢", 1}, {"丢手帕", 1}, {"丢出手帕", 1}})))
        , round_(1)
        , lookback_player(0)
        , player_scores_(Global().PlayerNum(), 0)
        , player_time_(Global().PlayerNum(), 0)
        , player_fault_(Global().PlayerNum(), 0)
        , player_select_(Global().PlayerNum(), 0)
    {
    }

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }

    // 终局分数
    std::vector<int64_t> player_scores_;
    // 回合数
    int round_;
    // 剩余时间（秒）
    std::vector<int64_t> player_time_;
    // 失误次数
    std::vector<int64_t> player_fault_;
    // 当前回头玩家（初始时间更长）
    PlayerID lookback_player;
    // 当前回合的选择
    std::vector<int64_t> player_select_;

    pair<string, string> ZeroPadding(int64_t time) const
    {
        string mm, ss;
        int m = time / 60;
        int s = time % 60;
        if (m < 10) {
            mm = "0" + to_string(m);
        } else {
            mm = to_string(m);
        }
        if (s < 10) {
            ss = "0" + to_string(s);
        } else {
            ss = to_string(s);
        }
        return make_pair(mm, ss);
    }

    string GetTable()
    {
        auto time0 = ZeroPadding(player_time_[0]);
        auto time1 = ZeroPadding(player_time_[1]);
        const string fault_warning = "<font color=\"red\">（再次失误判负）</font>";
        bool show_warning[2] = {false, false};
        if (GAME_OPTION(生命) < 300) {
            if (player_fault_[0] == 2) show_warning[0] = true;
            if (player_fault_[1] == 2) show_warning[1] = true;
        } else {
            if ((player_fault_[0] == 4 && player_fault_[0] != player_fault_[1] + 3) || player_fault_[0] == player_fault_[1] + 2) show_warning[0] = true;
            if ((player_fault_[1] == 4 && player_fault_[1] != player_fault_[0] + 3) || player_fault_[1] == player_fault_[0] + 2) show_warning[1] = true;
        }
        html::Table table(6, 1);
        table.SetTableStyle("width=\"500px\" align=\"center\" cellpadding=\"8\"");
        table.Get(0, 0).SetColor("#ECECEC");
        table.Get(0, 0).SetContent("<font size=\"5\">" + Global().PlayerName(0) + "</font>");
        table.Get(1, 0).SetContent("<font size=\"5\" color=\"#75A7EC\">受到惩罚：<font color=\"red\">" + to_string(player_fault_[0]) + "</font>次" + (show_warning[0] ? fault_warning : "") + "</font>");
        table.Get(2, 0).SetContent("<font size=\"6\" color=\"blue\">剩余生命 - <font color=\"red\">" + time0.first + "：" + time0.second + "</font></font>");
        table.Get(2, 0).SetStyle("style=\"border-bottom:2px solid #000000\"");
        table.Get(3, 0).SetContent("<font size=\"6\" color=\"blue\">剩余生命 - <font color=\"red\">" + time1.first + "：" + time1.second + "</font></font>");
        table.Get(4, 0).SetContent("<font size=\"5\" color=\"#75A7EC\">受到惩罚：<font color=\"red\">" + to_string(player_fault_[1]) + "</font>次" + (show_warning[1] ? fault_warning : "") + "</font>");
        table.Get(5, 0).SetContent("<font size=\"5\">" + Global().PlayerName(1) + "</font>");
        table.Get(5, 0).SetColor("#ECECEC");
        return table.ToString();
    }

  private:
    virtual void OnStageBegin() override
    {
#ifndef TEST_BOT
        lookback_player = rand() % 2;
#endif
        player_time_[lookback_player] = GAME_OPTION(生命);
        player_time_[1 - lookback_player] = GAME_OPTION(生命) - 15;
        player_select_[0] = player_select_[1] = 61;

        Global().Boardcast() << Markdown(GetTable());
        if (GAME_OPTION(模式) != 2) {
            Global().Boardcast() << "第 " << round_ << " 回合开始\n"
                                 << At(PlayerID(1 - lookback_player)) << " 选择时机丢出手帕\n"
                                 << At(PlayerID(lookback_player)) << " 选择回头看的时间";
            Global().Tell(1 - lookback_player) << "请选择时间点[丢出手帕]";
            Global().Tell(lookback_player) << "请选择时间点[回头看]";
        } else {
            Global().Boardcast() << "[提示] 本局游戏为实时模式，改为统计玩家发送消息时的真实秒数。如超时未操作，视为在第60秒时行动。\n"
                                 << "请注意：在该模式下，双方均行动之前，倒计时提示**永远**会警告双方玩家，即使您已经行动完成。\n"
                                 << "建议使用**文字指令**代替数字来防止延迟造成的问题";
#ifndef TEST_BOT
            std::this_thread::sleep_for(std::chrono::seconds(5));
#endif
            Global().Boardcast() << "第 " << round_ << " 回合准备\n"
                                 << At(PlayerID(1 - lookback_player)) << " 准备丢出手帕\n"
                                 << At(PlayerID(lookback_player)) << " 准备回头看\n"
                                 << "时限60秒，如您已经行动完成，请*忽略*倒计时警告消息\n"
                                 << "【本回合将在 10 秒后开始】";
#ifndef TEST_BOT
            std::this_thread::sleep_for(std::chrono::seconds(10));
#endif
            Global().Tell(1 - lookback_player) << "【实时模式】请在合适的时机发送消息[丢出手帕]，行动指令：丢 / 丢手帕";
            Global().Boardcast() << "第 " << round_ << " 回合开始！\n"
                                 << At(PlayerID(lookback_player)) << " 请在合适的时机发送消息[回头看]，行动指令：回 / 回头";
        }
        Global().StartTimer(GAME_OPTION(时限));
    }

    AtomReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << Markdown(GetTable());
        return StageErrCode::OK;
    }

    // 实时模式指令
    AtomReqErrCode RealTimeMode_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int64_t type)
    {
        if (GAME_OPTION(模式) != 2) {
            reply() << "[错误] 当前模式非实时模式，请直接发送数字代表行动的时机";
            return StageErrCode::FAILED;
        }
        if (type == 0 && pid != lookback_player) {
            reply() << "[错误] 行动失败：本回合您需要丢出手帕";
            return StageErrCode::FAILED;
        }
        if (type == 1 && pid == lookback_player) {
            reply() << "[错误] 行动失败：本回合您需要回头看";
            return StageErrCode::FAILED;
        }
        return SetTime_(pid, is_public, reply, 0);
    }

    AtomReqErrCode SetTime_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int64_t time)
    {
        if ((is_public && GAME_OPTION(模式) != 2) || (is_public && GAME_OPTION(模式) == 2 && pid != lookback_player)) {
            reply() << "[错误] 请私信裁判选择行动时间";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid) || player_select_[pid] != 61) {
            reply() << "[错误] 您本回合已经行动过了";
            return StageErrCode::FAILED;
        }

        // 实时模式结算
        if (GAME_OPTION(模式) == 2) {
            player_select_[pid] = 59 - std::chrono::duration_cast<std::chrono::seconds>(*Global().TimerFinishTime() - std::chrono::steady_clock::now()).count();
            if (player_select_[pid] < 0) player_select_[pid] = 0;
            if (pid == lookback_player) {
                reply() << "您在此时选择了回头，当前时间为：" << player_select_[pid];
            } else {
                reply() << "您在此时丢出了手帕，当前时间为：" << player_select_[pid];
            }
            if (player_select_[lookback_player] != 61) {
                if (player_select_[1 - lookback_player] == 61) {
                    Global().Tell(1 - lookback_player) << "[提示] 对手已经回头，本回合结束，您无需行动";
                }
                Global().SetReady(0); Global().SetReady(1);
            }
            return StageErrCode::OK;
        }

        player_select_[pid] = time;
        if (pid == lookback_player) {
            reply() << "本回合回头时间为：" << time;
        } else {
            reply() << "本回合丢手帕时间为：" << time;
        }
        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        if (GAME_OPTION(模式) == 2) {
            for (PlayerID i = 0; i < Global().PlayerNum(); ++i) {
                if (player_select_[i] == 61) {
                    player_select_[i] = 60;
                    Global().Tell(i) << "您超时未行动，默认在第 60 秒时行动";
                }
            }
            return RoundOver();
        }
        if (!Global().IsReady(0) && !Global().IsReady(1)) {
            Global().Boardcast() << "双方均超时，游戏平局";
        } else if (!Global().IsReady(0)) {
            player_scores_[0] = -1;
            Global().Boardcast() << "玩家 " << At(PlayerID(0)) << " 超时判负";
        } else {
            player_scores_[1] = -1;
            Global().Boardcast() << "玩家 " << At(PlayerID(1)) << " 超时判负";
        }
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Global().Boardcast() << At(PlayerID(pid)) << " 强退认输。";
        player_scores_[pid] = -1;
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        return RoundOver();
    }

    // 回合结束
    virtual CheckoutErrCode RoundOver()
    {
        bool game_end = false;
        if (player_select_[lookback_player] > player_select_[1 - lookback_player]) {
            // 回头看成功，减少剩余生命
            int gap_time = player_select_[lookback_player] - player_select_[1 - lookback_player];
            if (player_time_[lookback_player] >= gap_time) {
                player_time_[lookback_player] -= gap_time;
            } else {
                gap_time = player_time_[lookback_player];
                player_time_[lookback_player] = 0;
            }
            Global().Boardcast() << At(PlayerID(1 - lookback_player)) << " 于" << player_select_[1 - lookback_player] << "秒丢出手帕\n"
                                 << At(PlayerID(lookback_player)) << " 于" << player_select_[lookback_player] << "秒回头\n"
                                 << At(PlayerID(lookback_player)) << " 时间减少" << gap_time << "秒";
        } else if (player_select_[lookback_player] < player_select_[1 - lookback_player]) {
            // 回头看失败，受到惩罚
            player_fault_[lookback_player]++;
            Global().Boardcast() << At(PlayerID(lookback_player)) << " 于" << player_select_[lookback_player] << "秒回头\n"
                                 << At(PlayerID(1 - lookback_player)) << " 在此期间未丢手帕\n"
                                 << At(PlayerID(lookback_player)) << " 受到惩罚！";
            // 失误次数到达上限
            if ((GAME_OPTION(生命) < 300 && player_fault_[lookback_player] >= 3) ||
                (GAME_OPTION(生命) >= 300 && (player_fault_[lookback_player] >= 5 || player_fault_[lookback_player] >= player_fault_[1 - lookback_player] + 3))) {
                player_scores_[1 - lookback_player] = player_time_[1 - lookback_player];
                game_end = true;
                Global().Boardcast() << At(PlayerID(lookback_player)) << " 失误次数达到上限，" << At(PlayerID(1 - lookback_player)) << " 获得胜利！";
            }
        } else {
            // 完美回头
            if (GAME_OPTION(完美)) {
                player_scores_[lookback_player] = player_time_[lookback_player];
                game_end = true;
                Global().Boardcast() << At(PlayerID(1 - lookback_player)) << " 于" << player_select_[1 - lookback_player] << "秒丢出手帕\n"
                                     << At(PlayerID(lookback_player)) << " 于" << player_select_[lookback_player] << "秒回头\n"
                                     << At(PlayerID(lookback_player)) << " 完美回头获胜！";
            } else {
                player_time_[lookback_player]--;
                Global().Boardcast() << At(PlayerID(1 - lookback_player)) << " 于" << player_select_[1 - lookback_player] << "秒丢出手帕\n"
                                     << At(PlayerID(lookback_player)) << " 于" << player_select_[lookback_player] << "秒回头\n"
                                     << At(PlayerID(lookback_player)) << " 完美回头！时间减少1秒";
            }
        }
        // 剩余时间归零
        if (player_time_[lookback_player] <= 0 && !game_end) {
            player_scores_[1 - lookback_player] = player_time_[1 - lookback_player];
            game_end = true;
            Global().Boardcast() << At(PlayerID(lookback_player)) << " 剩余时间归零，" << At(PlayerID(1 - lookback_player)) << " 获得胜利！";
        }
        Global().Boardcast() << Markdown(GetTable());
        if (game_end) {
            return StageErrCode::CHECKOUT;
        }
        round_++;
        lookback_player = 1 - lookback_player;
        player_select_[0] = player_select_[1] = 61;
        Global().ClearReady(0); Global().ClearReady(1);
        if (GAME_OPTION(模式) != 2) {
            Global().Boardcast() << "第 " << round_ << " 回合开始\n"
                                 << At(PlayerID(1 - lookback_player)) << " 选择时机丢出手帕\n"
                                 << At(PlayerID(lookback_player)) << " 选择回头看的时间";
            Global().Tell(1 - lookback_player) << "请选择时间点[丢出手帕]";
            Global().Tell(lookback_player) << "请选择时间点[回头看]";
        } else {
            Global().Boardcast() << "第 " << round_ << " 回合准备\n"
                                 << At(PlayerID(1 - lookback_player)) << " 准备丢出手帕\n"
                                 << At(PlayerID(lookback_player)) << " 准备回头看\n"
                                 << "时限60秒，如您已经行动完成，请*忽略*倒计时警告消息\n"
                                 << "【本回合将在 10 秒后开始】";
#ifndef TEST_BOT
            std::this_thread::sleep_for(std::chrono::seconds(10));
#endif
            Global().Tell(1 - lookback_player) << "【实时模式】请在合适的时机发送消息[丢出手帕]，行动指令：丢 / 丢手帕";
            Global().Boardcast() << "第 " << round_ << " 回合开始！\n"
                                 << At(PlayerID(lookback_player)) << " 请在合适的时机发送消息[回头看]，行动指令：回 / 回头";
        }
        Global().StartTimer(GAME_OPTION(时限));
        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        player_select_[pid] = rand() % 61;
        if (rand() % 5 == 0) {
            if (pid == lookback_player) {
                player_select_[pid] = rand() % 3 + 58;
            } else {
                player_select_[pid] = rand() % 3;
            }
        }
        return StageErrCode::READY;
    }
};

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot

