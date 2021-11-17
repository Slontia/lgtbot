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

std::string GameOption::StatusInfo() const
{
    return "每手棋" + std::to_string(GET_VALUE(局时)) + "秒超时，超时即判负，最多" + std::to_string(GET_VALUE(回合数)) + "回合";
}

bool GameOption::IsValid(MsgSenderBase& reply) const
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
        , board_(option.ResourceDir())
        , round_(0)
        , turn_(rand() % 2)
    {
    }

    virtual void OnStageBegin()
    {
        SetReady(1 - turn_);
        StartTimer(option().GET_VALUE(局时));
        Boardcast() << Markdown(ShowInfo_(false));
        Boardcast() << "请" << At(turn_) << "行动，" << option().GET_VALUE(局时)
                    << "秒未行动自动判负\n格式：移动前位置 移动后位置";
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply)
    {
        if (pid != turn_) {
            return StageErrCode::OK;
        }
        while (true) {
            const uint32_t src = rand() % 16;
            const auto valid_dsts = board_.ValidDsts(src);
            const uint32_t dst = valid_dsts[rand() % valid_dsts.size()];
            const auto ret = board_.Push(src, dst, pid2type_(pid));
            if (ret == quixo::ErrCode::OK) {
                Boardcast() << At(pid) << "将 " << src << " 位置的棋子移动至 " << dst;
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
        if (pid != turn_) {
            reply() << "在您的对手行动完毕前，您无法行动";
            return StageErrCode::FAILED;
        }
        const auto ret = board_.Push(src, dst, pid2type_(turn_));
        if (ret == quixo::ErrCode::INVALID_SRC) {
            reply() << "您无法移动该棋子，您只能移动空白或本方棋子";
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
        Boardcast() << At(pid) << "将 " << src << " 位置的棋子移动至 " << dst;
        if (const auto win_type = board_.CheckWin(); win_type != quixo::Type::_) {
            winner_.emplace(type2pid_(win_type));
        }
        return StageErrCode::READY;
    }

    AtomReqErrCode Info_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << Markdown(ShowInfo_(false));
        return StageErrCode::OK;
    }

    std::string ShowInfo_(const bool is_finish) const
    {
        std::string str = "## 第" + std::to_string(round_ / 2) + "回合\n\n";
        Table player_table(1, 4);
        player_table.SetTableStyle(" align=\"center\" cellpadding=\"1\" cellspacing=\"1\" ");
        const auto print_player = [&](const PlayerID pid)
            {
                player_table.Get(0, 0 + pid * 2).SetContent(
                        std::string("![](file://") + option().ResourceDir() +
                        (pid == turn_ || is_finish ? std::string("box_") + static_cast<char>(pid2type_(pid)) : "empty") +
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
        if (winner_.has_value()) {
            Boardcast() << Markdown(ShowInfo_(true));
            Boardcast() << At(turn_) << "连成了直线，取得胜利";
        } else if ((++round_) / 2 > option().GET_VALUE(回合数)) {
            Boardcast() << Markdown(ShowInfo_(true));
            Boardcast() << "达到最大回合数，游戏平局";
        } else {
            turn_ = 1 - turn_;
            ClearReady(turn_);
            StartTimer(option().GET_VALUE(局时));
            Boardcast() << Markdown(ShowInfo_(false));
            Boardcast() << "请" << At(turn_) << "行动，" << option().GET_VALUE(局时)
                        << "秒未行动自动判负\n格式：移动前位置 移动后位置";
        }
    }

    CheckoutErrCode OnTimeout()
    {
        winner_.emplace(1 - turn_);
        Boardcast() << At(turn_) << "超时判负";
        return StageErrCode::CHECKOUT;
    }

    PlayerID type2pid_(const quixo::Type type) const { return type == quixo::Type::X ? 1 : 0; }
    quixo::Type pid2type_(const PlayerID pid) const { return pid == 1 ? quixo::Type::X : quixo::Type::O; }

    quixo::Board board_;
    uint32_t round_;
    PlayerID turn_;
    std::optional<PlayerID> winner_;
};

MainStageBase* MakeMainStage(MsgSenderBase& reply, const GameOption& options, MatchBase& match)
{
    if (!options.IsValid(reply)) {
        return nullptr;
    }
    return new MainStage(options, match);
}
