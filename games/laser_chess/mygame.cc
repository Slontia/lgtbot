// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <array>
#include <map>
#include <functional>
#include <memory>
#include <vector>
#include <random>

#include "game_maps.h"

#include "game_framework/game_main.h"
#include "game_framework/game_stage.h"
#include "game_framework/game_options.h"
#include "utility/msg_checker.h"
#include "utility/html.h"
#include "utility/coding.h"
#include "game_util/laser_chess.h"

const std::string k_game_name = "镭射象棋";
const uint64_t k_max_player = 2; /* 0 means no max-player limits */
const uint64_t k_multiple = 0;

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

using namespace laser;

enum class Choise { UP, RIGHT, DOWN, LEFT, RIGHT_UP, LEFT_UP, RIGHT_DOWN, LEFT_DOWN, CLOCKWISE, ANTICLOCKWISE, _MAX };

static std::ostream& operator<<(std::ostream& os, const Coor& coor) { return os << ('A' + coor.m_) << coor.n_; }

std::array<Board(*)(std::string), 3> game_map_initers = {InitAceBoard, InitCuriosityBoard, InitGeniusBoard};

// Player 1 use fork, player 0 use circle
class MainStage : public MainGameStage<>
{
  public:
    MainStage(const GameOption& option, MatchBase& match)
        : GameStage(option, match,
                MakeStageCommand("查看盘面情况，可用于图片重发", &MainStage::Info_, VoidChecker("赛况")),
                MakeStageCommand("移动棋子", &MainStage::Set_,
                    AnyArg("棋子当前位置"), AlterChecker<Choise>(
                            {{ "上", Choise::UP },
                            { "右", Choise::RIGHT },
                            { "下", Choise::DOWN },
                            { "左", Choise::LEFT },
                            { "右上", Choise::RIGHT_UP },
                            { "左上", Choise::LEFT_UP },
                            { "右下", Choise::RIGHT_DOWN },
                            { "左下", Choise::LEFT_DOWN },
                            { "顺", Choise::CLOCKWISE },
                            { "逆", Choise::ANTICLOCKWISE }}
                        )))
        , first_turn_(rand() % 2)
        , board_(option.GET_VALUE(地图) == GameMap::ACE ? InitAceBoard(option.ResourceDir()) :
                 option.GET_VALUE(地图) == GameMap::CURIOSITY ? InitCuriosityBoard(option.ResourceDir()) :
                 option.GET_VALUE(地图) == GameMap::GENIUS ? InitGeniusBoard(option.ResourceDir()) :
                     (*game_map_initers[rand() % game_map_initers.size()])(option.ResourceDir()))
        , round_(0)
    {
    }

    virtual void OnStageBegin()
    {
        SetReady(1 - cur_pid());
        StartTimer(option().GET_VALUE(局时));
        Boardcast() << Markdown(ShowInfo_());
        Boardcast() << "请" << At(cur_pid()) << "行动，" << option().GET_VALUE(局时)
                    << "秒未行动自动判负\n格式：棋子位置 行动方式";
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply)
    {
        if (pid != cur_pid()) {
            return StageErrCode::OK;
        }
        const Coor coor = [&]() -> Coor
            {
                while (true) {
                    const Coor coor(rand() % board_.max_m(), rand() % board_.max_n());
                    if (board_.IsMyChess(coor, pid)) {
                        return coor;
                    }
                }
                return {};
            }();
        while (!Act_(pid, coor, static_cast<Choise>(rand() % static_cast<uint32_t>(Choise::_MAX)), EmptyMsgSender::Get()));
        return StageErrCode::READY;
    }

    int64_t PlayerScore(const PlayerID pid) const
    {
        return winner_ == pid;
    }

  private:
    bool Act_(const PlayerID pid, const Coor& coor, const Choise choise, MsgSenderBase& reply)
    {
        assert(board_.IsValidCoor(coor));
        const auto coor_to_str = [](const Coor& coor) { return char('A' + coor.m_) + std::to_string(coor.n_); };
        if (!board_.IsMyChess(coor, pid)) {
            reply() << "行动失败：该位置并非本方棋子";
            return false;
        }
        if (choise == Choise::CLOCKWISE || choise == Choise::ANTICLOCKWISE) {
            if (!board_.Rotate(coor, choise == Choise::CLOCKWISE)) {
                reply() << "行动失败：无法如此旋转该棋子";
                return false;
            }
            Boardcast() << At(pid) << "将 " << coor_to_str(coor) << " 位置的棋子"
                        << (choise == Choise::CLOCKWISE ? "顺" : "逆") << "时针旋转";
        } else {
            const auto new_coor = coor + (
                        choise == Choise::UP ? Coor{-1, 0} :
                        choise == Choise::DOWN ? Coor{1, 0} :
                        choise == Choise::LEFT ? Coor{0, 1} :
                        choise == Choise::RIGHT ? Coor{0, -1} :
                        choise == Choise::LEFT_UP ? Coor{-1, 1} :
                        choise == Choise::RIGHT_UP ? Coor{-1, -1} :
                        choise == Choise::LEFT_DOWN ? Coor{1, -1} :
                        choise == Choise::RIGHT_DOWN ? Coor{1, 1} : (assert(false), Coor())
                    );
            if (!board_.IsValidCoor(new_coor)) {
                reply() << "行动失败：您无法将棋子移动至棋盘外";
                return false;
            }
            if (!board_.Move(coor, new_coor)) {
                reply() << "行动失败：您无法移动该棋子，或目标位置已有棋子";
                return false;
            }
            Boardcast() << At(pid) << "将 " << coor_to_str(coor) << " 位置的棋子移动至 " << coor_to_str(new_coor) << " 位置";
        }
        reply() << "行动成功";
        board_.Shoot(pid);
        return true;
    }

    AtomReqErrCode Set_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::string& coor_str, const Choise choise)
    {
        if (pid != cur_pid()) {
            reply() << "在您的对手行动完毕前，您无法行动";
            return StageErrCode::FAILED;
        }
        const auto coor_ret = DecodePos(coor_str);
        if (const auto* const s = std::get_if<std::string>(&coor_ret)) {
            reply() << "行动失败：" << *s;
            return StageErrCode::FAILED;
        }
        const auto& coor_pair = std::get<std::pair<uint32_t, uint32_t>>(coor_ret);
        if (!Act_(pid, Coor{static_cast<int32_t>(coor_pair.first), static_cast<int32_t>(coor_pair.second)}, choise, reply)) {
            return StageErrCode::FAILED;
        }
        return StageErrCode::READY;
    }

    AtomReqErrCode Info_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << Markdown(ShowInfo_());
        return StageErrCode::OK;
    }

    std::string ShowInfo_() const
    {
        std::string str = "## 第 " + std::to_string(round_ / 2 + 1) + " / " + std::to_string(option().GET_VALUE(回合数)) + " 回合\n\n";
        html::Table player_table(1, 4);
        player_table.SetTableStyle(" align=\"center\" cellpadding=\"1\" cellspacing=\"1\" ");
        const auto print_player = [&](const PlayerID pid)
            {
                player_table.Get(0, 0 + pid * 2).SetContent(
                        std::string("![](file://") + option().ResourceDir() +
                        (pid == cur_pid() ? std::string("king_") + bool_to_rb(pid) : "empty") +
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
        Boardcast() << Markdown(ShowInfo_());
        const auto settle_ret = board_.Settle();

        if (settle_ret.king_dead_num_[cur_pid()]) {
            winner_.emplace(1 - cur_pid());
            Boardcast() << "玩家" << At(cur_pid()) << "命中了己方的王，输掉了比赛";
        } else if (settle_ret.king_dead_num_[1 - cur_pid()]) {
            winner_.emplace(cur_pid());
            Boardcast() << "玩家" << At(cur_pid()) << "命中了敌方的王，赢得了比赛";
        } else if ((++round_) / 2 >= option().GET_VALUE(回合数)) { // cur_pid changed
            Boardcast() << "达到最大回合数，游戏平局";
        } else {
            auto sender = Boardcast();
            if (settle_ret.chess_dead_num_[0] || settle_ret.chess_dead_num_[1]) {
                sender << "玩家" << At(PlayerID{1 - cur_pid()}) << "命中了敌方 " << settle_ret.chess_dead_num_[cur_pid()]
                       << " 枚棋子，命中了己方 " << settle_ret.chess_dead_num_[1 - cur_pid()] << " 枚棋子\n\n";
            }
            ClearReady(cur_pid());
            StartTimer(option().GET_VALUE(局时));
            sender << "请" << At(cur_pid()) << "行动，" << option().GET_VALUE(局时)
                   << "秒未行动自动判负\n格式：棋子位置 行动方式";
        }
        Boardcast() << Markdown(ShowInfo_());
    }

    CheckoutErrCode OnTimeout()
    {
        winner_.emplace(1 - cur_pid());
        Boardcast() << At(cur_pid()) << "超时判负";
        return StageErrCode::CHECKOUT;
    }

    PlayerID cur_pid() const { return round_ % 2 ? PlayerID(1 - first_turn_) : first_turn_; }

    const PlayerID first_turn_;
    laser::Board board_;
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
