// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <array>
#include <map>
#include <functional>
#include <memory>
#include <vector>
#include <random>

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"
#include "game_util/quixo.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;
const GameProperties k_properties { 
    .name_ = "你推我挤",
    .developer_ = "森高",
    .description_ = "通过取出并重新放入棋子，先连成五子者获胜的游戏",
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 2; } /* 0 means no max-player limits */
uint32_t Multiple(const CustomOptions& options) { return 2; }
const MutableGenericOptions k_default_generic_options;
const std::vector<RuleCommand> k_rule_commands = {};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() != 2) {
        reply() << "该游戏为双人游戏，必须为2人参加，当前玩家数为" << generic_options_readonly.PlayerNum();
        return false;
    }
    return true;
}

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options)
            {
                generic_options.bench_computers_to_player_num_ = 2;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
};

// ========== GAME STAGES ==========

// Player 1 use fork, player 0 use circle
class MainStage : public MainGameStage<>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility),
                MakeStageCommand(*this, "查看盘面情况，可用于图片重发", &MainStage::Info_, VoidChecker("赛况")),
                MakeStageCommand(*this, "移动棋子", &MainStage::Set_,
                    ArithChecker<uint32_t>(0, 15, "移动前位置"), ArithChecker<uint32_t>(0, 15, "移动后位置")))
        , first_turn_(rand() % 2)
        , board_(Global().ResourceDir())
        , round_(0)
        , scores_{0}
    {
    }

    virtual void OnStageBegin()
    {
        Global().SetReady(1 - cur_pid());
        Global().StartTimer(GAME_OPTION(局时));
        Global().Boardcast() << Markdown(ShowInfo_());
        Global().Boardcast() << "请" << At(cur_pid()) << "行动，" << GAME_OPTION(局时)
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
            if (ret == game_util::quixo::ErrCode::OK) {
                Global().Boardcast() << At(pid) << "将 " << src << " 位置的棋子取出，从 " << dst << " 位置重新推入";
                break;
            }
        }
        return StageErrCode::READY;
    }

    int64_t PlayerScore(const PlayerID pid) const
    {
        return scores_[pid];
    }

  private:
    AtomReqErrCode Set_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const uint32_t src, const uint32_t dst)
    {
        if (pid != cur_pid()) {
            reply() << "在您的对手行动完毕前，您无法行动";
            return StageErrCode::FAILED;
        }
        const auto ret = board_.Push(src, dst, cur_type());
        if (ret == game_util::quixo::ErrCode::INVALID_SRC) {
            reply() << "您无法移动该棋子，您只能移动空白或本方相同样式的棋子";
            return StageErrCode::FAILED;
        }
        if (ret == game_util::quixo::ErrCode::INVALID_DST) {
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
        Global().Boardcast() << At(pid) << "将 " << src << " 位置的棋子取出，从 " << dst << " 位置重新推入";
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
        html::Table player_table(2, 4);
        player_table.MergeDown(0, 0, 2);
        player_table.MergeDown(0, 2, 2);
        player_table.SetTableStyle(" align=\"center\" cellpadding=\"1\" cellspacing=\"1\" ");
        const auto chess_counts = ChessCounts_();
        const auto print_player = [&](const PlayerID pid)
            {
                player_table.Get(0, 0 + pid * 2).SetContent(
                        std::string("![](file:///") + Global().ResourceDir() +
                        (pid == cur_pid() ? std::string("box_") + static_cast<char>(cur_type()) : "empty") +
                        ".png)");
                player_table.Get(0, 1 + pid * 2).SetContent("**" + Global().PlayerName(pid) + "**");
                player_table.Get(1, 1 + pid * 2).SetContent("棋子数量： " HTML_COLOR_FONT_HEADER(red) + std::to_string(chess_counts[pid]) + HTML_FONT_TAIL);
            };
        print_player(0);
        print_player(1);
        str += player_table.ToString();
        str += "\n\n";
        str += board_.ToHtml();
        return str;
    }

    virtual CheckoutErrCode OnStageOver()
    {
        const auto ret = board_.LineCount();
        if (ret[1 - static_cast<uint32_t>(cur_symbol())]) {
            Global().Boardcast() << Markdown(ShowInfo_());
            Global().Boardcast() << At(cur_pid()) << "帮助对手达成了直线，于是，输掉了比赛";
            scores_[1 - cur_pid()] = 1;
        } else if (ret[static_cast<uint32_t>(cur_symbol())]) {
            Global().Boardcast() << Markdown(ShowInfo_());
            Global().Boardcast() << At(cur_pid()) << "达成了直线，于是，赢得了比赛";
            scores_[cur_pid()] = 1;
        } else if ((++round_) / 2 >= GAME_OPTION(回合数)) {
            Global().Boardcast() << Markdown(ShowInfo_());
            const auto chess_counts = ChessCounts_();
            if (chess_counts[0] == chess_counts[1]) {
                Global().Boardcast() << "游戏达到最大回合数，双方棋子数量相同，游戏平局";
            } else if (chess_counts[0] > chess_counts[1]) {
                scores_[1] = 1;
                Global().Boardcast() << "游戏达到最大回合数，玩家" << At(PlayerID(1)) << "棋子数量较少，于是，赢得了比赛";
            } else {
                scores_[0] = 1;
                Global().Boardcast() << "游戏达到最大回合数，玩家" << At(PlayerID(0)) << "棋子数量较少，于是，赢得了比赛";
            }
        } else if (!board_.CanPush(cur_type())) {
            Global().Boardcast() << Markdown(ShowInfo_());
            Global().Boardcast() << At(cur_pid()) << "没有可取出的棋子，于是，输掉了比赛";
            scores_[1 - cur_pid()] = 1;
        } else {
            Global().Boardcast() << Markdown(ShowInfo_());
            Global().ClearReady(cur_pid());
            Global().StartTimer(GAME_OPTION(局时));
            Global().Boardcast() << "请" << At(cur_pid()) << "行动，" << GAME_OPTION(局时)
                        << "秒未行动自动判负\n格式：取出位置 推入位置";
            return StageErrCode::CONTINUE;
        }
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        scores_[1 - cur_pid()] = 1;
        Global().Boardcast() << At(cur_pid()) << "超时判负";
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        scores_[1 - pid] = 1;
        Global().Boardcast() << At(pid) << "中途离开判负";
        return StageErrCode::CHECKOUT;
    }

    PlayerID cur_pid() const { return round_ % 2 ? PlayerID(1 - first_turn_) : first_turn_; }
    game_util::quixo::Type cur_type() const
    {
        static const std::array<game_util::quixo::Type, 4> types{
            game_util::quixo::Type::O1,
            game_util::quixo::Type::X1,
            game_util::quixo::Type::O2,
            game_util::quixo::Type::X2
        };
        return types[round_ % (GAME_OPTION(模式) ? 2 : 4)];
    }
    game_util::quixo::Symbol cur_symbol() const { return round_ % 2 ? game_util::quixo::Symbol::X : game_util::quixo::Symbol::O; }

    std::array<uint32_t, 2> ChessCounts_() const
    {
        const auto board_chess_counts = board_.ChessCounts();
        std::array<uint32_t, 2> chess_counts;
        chess_counts[cur_pid()] = board_chess_counts[static_cast<uint32_t>(cur_symbol())];
        chess_counts[1 - cur_pid()] = board_chess_counts[1 - static_cast<uint32_t>(cur_symbol())];
        return chess_counts;
    }

    const PlayerID first_turn_;
    game_util::quixo::Board board_;
    uint32_t round_;
    std::array<int32_t, 2> scores_;
};

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

