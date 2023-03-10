// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/game_stage.h"
#include "utility/html.h"
#include "utility/coding.h"
#include "game_util/othello.h"

using namespace lgtbot::game_util::othello;

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

const std::string k_game_name = "决胜奥赛罗"; // the game name which should be unique among all the games
const uint64_t k_max_player = 2; // 0 indicates no max-player limits
const uint64_t k_multiple = 1; // the default score multiple for the game, 0 for a testing game, 1 for a formal game, 2 or 3 for a long formal game
const std::string k_developer = "森高";
const std::string k_description = "双方一起落子的奥赛罗游戏";

std::string GameOption::StatusInfo() const
{
    return "每回合超时时间 " + std::to_string(GET_VALUE(时限)) + " 秒";
}

bool GameOption::ToValid(MsgSenderBase& reply)
{
    if (PlayerNum() < 2) {
        reply() << "该游戏至少 2 人参加，当前玩家数为 " << PlayerNum();
        return false;
    }
    return true;
}

uint64_t GameOption::BestPlayerNum() const { return 2; }

// ========== GAME STAGES ==========

class MainStage : public MainGameStage<>
{
  public:
    MainStage(const GameOption& option, MatchBase& match)
        : GameStage(option, match,
                MakeStageCommand("查看当前游戏进展情况", &MainStage::Status_, VoidChecker("赛况")),
                MakeStageCommand("落子", &MainStage::Place_, AnyArg("位置", "A0")))
        , round_(0)
        , player_scores_{2, 2} // the initial chess count
        , board_(option.ResourceDir())
    {
    }

    virtual void OnStageBegin() override
    {
        StartTimer(GET_OPTION_VALUE(option(), 时限));
        Boardcast() << Markdown(ToHtml_());
        Boardcast() << "游戏开始，请私信裁判坐标以落子（如 C3），时限 " << GET_OPTION_VALUE(option(), 时限) << " 秒，超时未落子即判负";

    }

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        player_scores_[pid] = 0;
        player_scores_[1 - pid] = 1;
        Boardcast() << "玩家" << At(pid) << "强退判负";
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnTimeout() override
    {
        if (!IsReady(0) && !IsReady(1)) {
            Boardcast() << "双方均超时，游戏立刻结束，以当前盘面结算成绩";
        } else if (IsReady(0)) {
            player_scores_[0] = 1;
            player_scores_[1] = 0;
            Boardcast() << "玩家" << At(PlayerID{1}) << "超时判负";
        } else {
            player_scores_[1] = 1;
            player_scores_[0] = 0;
            Boardcast() << "玩家" << At(PlayerID{0}) << "超时判负";
        }
        return StageErrCode::CHECKOUT;
    }

  private:
    AtomReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << Markdown(ToHtml_());
        return StageErrCode::OK;
    }

    AtomReqErrCode Place_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::string& coor_str)
    {
        if (is_public) {
            reply() << "落子失败：请私信裁判落子";
            return StageErrCode::FAILED;
        }
        const auto decode_result = DecodePos(coor_str);
        if (const auto* errstr = std::get_if<std::string>(&decode_result)) {
            reply() << "落子失败：" << *errstr;
            return StageErrCode::FAILED;
        }
        const auto raw_coor = std::get<std::pair<uint32_t, uint32_t>>(decode_result);
        if (!board_.Place(Coor{static_cast<int32_t>(raw_coor.first), static_cast<int32_t>(raw_coor.second)},
                    PlayerIDToChessType_(pid))) {
            auto sender = reply();
            sender << "落子失败：您无法在此处落子，";
            const auto placable_positions = board_.PlacablePositions(PlayerIDToChessType_(pid));
            if (placable_positions.empty()) {
                sender << "您没有合法的落子点，本回合您无需落子";
            } else {
                sender << "合法的落子点包括";
                for (const Coor coor : placable_positions) {
                    sender << " " << std::string(1, 'A' + coor.row_) << coor.col_;
                }
            }
            return StageErrCode::FAILED;
        }
        reply() << "落子成功";
        if (!IsReady(1 - pid)) {
        }
        return StageErrCode::READY;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        const auto chess_type = PlayerIDToChessType_(pid);
        const auto avaliable_placements = board_.PlacablePositions(chess_type);
        if (!avaliable_placements.empty()) {
            const auto ret = board_.Place(avaliable_placements[std::rand() % avaliable_placements.size()], chess_type);
            assert(ret);
        }
        return StageErrCode::READY;
    }

    virtual void OnAllPlayerReady() override
    {
        const auto result = board_.Settlement();
        const auto update_player = [&](const PlayerID pid)
            {
                player_scores_[pid] = result[static_cast<uint8_t>(PlayerIDToChessType_(pid))];
                if (!board_.PlacablePositions(PlayerIDToChessType_(pid)).empty()) {
                    ClearReady(pid);
                }
            };
        update_player(0);
        update_player(1);
        Boardcast() << "双方落子成功";
        ++round_;
        Boardcast() << Markdown(ToHtml_());
        if (!IsReady(0) || !IsReady(1)) {
            StartTimer(GET_OPTION_VALUE(option(), 时限));
            Boardcast() << "请继续私信裁判坐标以落子（如 C3），时限 " << GET_OPTION_VALUE(option(), 时限) << " 秒，超时未落子即判负";
        }
    }

    std::string ToHtml_() const
    {
        html::Table player_table(2, 4);
        player_table.MergeDown(0, 0, 2);
        player_table.MergeDown(0, 2, 2);
        player_table.SetTableStyle(" align=\"center\" cellpadding=\"1\" cellspacing=\"1\" ");
        const auto print_player = [&](const PlayerID pid)
            {
                player_table.Get(0, 0 + pid * 2).SetContent(std::string("![](file://") + option().ResourceDir() +
                        (PlayerIDToChessType_(pid) == ChessType::BLACK ? "black" : "white") + ".png)");
                player_table.Get(0, 1 + pid * 2).SetContent("**" + PlayerName(pid) + "**");
                player_table.Get(1, 1 + pid * 2).SetContent("棋子数量： " HTML_COLOR_FONT_HEADER(yellow) + std::to_string(player_scores_[pid]) + HTML_FONT_TAIL);
            };
        print_player(0);
        print_player(1);
        return "## 第 " + std::to_string(round_) + " 回合\n\n" + player_table.ToString() + "\n\n" + board_.ToHtml();
    }

    static ChessType PlayerIDToChessType_(const PlayerID pid) { return pid ? ChessType::BLACK : ChessType::WHITE; }

    int round_;
    std::array<int64_t, 2> player_scores_;
    Board board_;
};

} // namespace lgtbot

} // namespace game

} // namespace lgtbot

#include "game_framework/make_main_stage.h"
