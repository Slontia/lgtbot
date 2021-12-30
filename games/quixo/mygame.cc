// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <array>
#include <map>
#include <functional>
#include <memory>
#include <vector>
#include <random>

#include "game_framework/game_main.h"
#include "game_framework/game_stage.h"
#include "game_framework/game_options.h"
#include "utility/msg_checker.h"
#include "utility/html.h"
#include "game_util/quixo.h"

const std::string k_game_name = "你推我挤";
const uint64_t k_max_player = 2; /* 0 means no max-player limits */
const uint64_t k_multiple = 1;

std::string GameOption::StatusInfo() const
{
    return "每手棋" + std::to_string(GET_VALUE(局时)) + "秒超时，超时即判负，最多" + std::to_string(GET_VALUE(回合数)) + "回合";
}

bool GameOption::ToValid(MsgSenderBase& reply)
{
    if (PlayerNum() != 2) {
        reply() << "该游戏为双人游戏，必须为2人参加，当前玩家数为" << PlayerNum();
        return false;
    }
    return true;
}

uint64_t GameOption::BestPlayerNum() const { return 2; }

// ========== GAME STAGES ==========

// Player 1 use fork, player 0 use circle
class MainStage : public MainGameStage<>
{
  public:
    MainStage(const GameOption& option, MatchBase& match)
        : GameStage(option, match,
                MakeStageCommand("查看盘面情况，可用于图片重发", &MainStage::Info_, VoidChecker("赛况")),
                MakeStageCommand("移动棋子", &MainStage::Set_,
                    ArithChecker<uint32_t>(0, 15, "移动前位置"), ArithChecker<uint32_t>(0, 15, "移动后位置")))
        , first_turn_(rand() % 2)
        , board_(option.ResourceDir())
        , round_(0)
    {
    }

    virtual void OnStageBegin()
    {
        SetReady(1 - cur_pid());
        StartTimer(option().GET_VALUE(局时));
        Boardcast() << Markdown(ShowInfo_());
        Boardcast() << "请" << At(cur_pid()) << "行动，" << option().GET_VALUE(局时)
                    << "秒未行动自动判负\n格式：移动前位置 移动后位置";
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply)
    {
        if (pid != cur_pid()) {
            return StageErrCode::OK;
        }
        while (true) {
            const uint32_t src = rand() % 16;
            const auto valid_dsts = board_.ValidDsts(src);
            const uint32_t dst = valid_dsts[rand() % valid_dsts.size()];
            const auto ret = board_.Push(src, dst, cur_type());
            if (ret == quixo::ErrCode::OK) {
                Boardcast() << At(pid) << "将 " << src << " 位置的棋子取出，从 " << dst << " 位置重新推入";
                break;
            }
        }
        return StageErrCode::READY;
    }

    int64_t PlayerScore(const PlayerID pid) const
    {
        return winner_ == pid;
    }

  private:
    AtomReqErrCode Set_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const uint32_t src, const uint32_t dst)
    {
        if (pid != cur_pid()) {
            reply() << "在您的对手行动完毕前，您无法行动";
            return StageErrCode::FAILED;
        }
        const auto ret = board_.Push(src, dst, cur_type());
        if (ret == quixo::ErrCode::INVALID_SRC) {
            reply() << "您无法移动该棋子，您只能移动空白或本方相同样式的棋子";
            return StageErrCode::FAILED;
        }
        if (ret == quixo::ErrCode::INVALID_DST) {
            const auto valid_dsts = board_.ValidDsts(src);
            auto sender = reply();
            sender << "非法的移动后位置，需要是和移动前同行或同列，且处于边缘的不同位置，这样才能造成棋子推动\n对于移动前位置 "
                   << src << "，其合法的移动后位置有：";
            for (const auto dst : valid_dsts) {
                sender << dst << " ";
            }
            return StageErrCode::FAILED;
        }
        reply() << "移动成功！";
        Boardcast() << At(pid) << "将 " << src << " 位置的棋子取出，从 " << dst << " 位置重新推入";
        return StageErrCode::READY;
    }

    AtomReqErrCode Info_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << Markdown(ShowInfo_());
        return StageErrCode::OK;
    }

    std::string ShowInfo_() const
    {
        std::string str = "## 第" + std::to_string(round_ / 2 + 1) + "回合\n\n";
        html::Table player_table(1, 4);
        player_table.SetTableStyle(" align=\"center\" cellpadding=\"1\" cellspacing=\"1\" ");
        const auto print_player = [&](const PlayerID pid)
            {
                player_table.Get(0, 0 + pid * 2).SetContent(
                        std::string("![](file://") + option().ResourceDir() +
                        (pid == cur_pid() ? std::string("box_") + static_cast<char>(cur_type()) : "empty") +
                        ".png)");
                player_table.Get(0, 1 + pid * 2).SetContent("**" + PlayerName(pid) + "**");
            };
        print_player(0);
        print_player(1);
        str += player_table.ToString();
        str += "\n\n";
        str += board_.ToHtml();
        return str;
    }

    virtual void OnAllPlayerReady()
    {
        const auto ret = board_.LineCount();
        if (ret[1 - static_cast<uint32_t>(cur_symbol())]) {
            Boardcast() << Markdown(ShowInfo_());
            Boardcast() << At(cur_pid()) << "帮助对手达成了直线，于是，输掉了比赛";
            winner_.emplace(1 - cur_pid());
        } else if (ret[static_cast<uint32_t>(cur_symbol())]) {
            Boardcast() << Markdown(ShowInfo_());
            Boardcast() << At(cur_pid()) << "达成了直线，于是，赢得了比赛";
            winner_.emplace(cur_pid());
        } else if ((++round_) / 2 >= option().GET_VALUE(回合数)) {
            Boardcast() << Markdown(ShowInfo_());
            Boardcast() << "后手方" << At(PlayerID(1 - first_turn_)) << "坚持到了最大回合数，于是，赢得了比赛";
            winner_.emplace(1 - first_turn_);
        } else if (!board_.CanPush(cur_type())) {
            Boardcast() << Markdown(ShowInfo_());
            Boardcast() << At(cur_pid()) << "没有可取出的棋子，于是，输掉了比赛";
            winner_.emplace(1 - cur_pid());
        } else {
            Boardcast() << Markdown(ShowInfo_());
            ClearReady(cur_pid());
            StartTimer(option().GET_VALUE(局时));
            Boardcast() << "请" << At(cur_pid()) << "行动，" << option().GET_VALUE(局时)
                        << "秒未行动自动判负\n格式：取出位置 推入位置";
        }
    }

    CheckoutErrCode OnTimeout()
    {
        winner_.emplace(1 - cur_pid());
        Boardcast() << At(cur_pid()) << "超时判负";
        return StageErrCode::CHECKOUT;
    }

    PlayerID cur_pid() const { return round_ % 2 ? PlayerID(1 - first_turn_) : first_turn_; }
    quixo::Type cur_type() const
    {
        static const std::array<quixo::Type, 4> types{quixo::Type::O1, quixo::Type::X1, quixo::Type::O2, quixo::Type::X2};
        return types[round_ % (option().GET_VALUE(模式) ? 2 : 4)];
    }
    quixo::Symbol cur_symbol() const { return round_ % 2 ? quixo::Symbol::X : quixo::Symbol::O; }

    const PlayerID first_turn_;
    quixo::Board board_;
    uint32_t round_;
    std::optional<PlayerID> winner_;
};

MainStageBase* MakeMainStage(MsgSenderBase& reply, GameOption& options, MatchBase& match)
{
    if (!options.ToValid(reply)) {
        return nullptr;
    }
    return new MainStage(options, match);
}
