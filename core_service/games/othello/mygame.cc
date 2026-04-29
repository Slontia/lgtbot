// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"
#include "utility/coding.h"
#include "game_util/othello.h"

using namespace lgtbot::game_util::othello;

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;
const GameProperties k_properties { 
    .name_ = "决胜奥赛罗", // the game name which should be unique among all the games
    .developer_ = "森高",
    .description_ = "双方一起落子的奥赛罗游戏",
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 2; } // 0 indicates no max-player limits
uint32_t Multiple(const CustomOptions& options) { return 2; } // the default score multiple for the game, 0 for a testing game, 1 for a formal game, 2 or 3 for a long formal game
const MutableGenericOptions k_default_generic_options;
const std::vector<RuleCommand> k_rule_commands = {};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() < 2) {
        reply() << "该游戏至少 2 人参加，当前玩家数为 " << generic_options_readonly.PlayerNum();
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

class MainStage : public MainGameStage<>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility),
                MakeStageCommand(*this, "查看当前游戏进展情况", &MainStage::Status_, VoidChecker("赛况")),
                MakeStageCommand(*this, "落子", &MainStage::Place_, AnyArg("位置", "A0")))
        , round_(0)
        , player_scores_{2, 2} // the initial chess count
        , board_(Global().ResourceDir())
    {
    }

    virtual void OnStageBegin() override
    {
        Global().StartTimer(GAME_OPTION(时限));
        Global().Group() << Markdown(ToHtml_());
        Global().Tell(0) << Markdown(ToHtml_());
        Global().Tell(1) << Markdown(ToHtml_());
        Global().Boardcast() << "游戏开始，请私信裁判坐标以落子（如 C3），时限 " << GAME_OPTION(时限) << " 秒，超时未落子即判负";
    }

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        player_scores_[pid] = 0;
        player_scores_[1 - pid] = 1;
        Global().Boardcast() << "玩家" << At(pid) << "强退判负";
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        if (!Global().IsReady(0) && !Global().IsReady(1)) {
            Global().Boardcast() << "双方均超时，游戏立刻结束，以当前盘面结算成绩";
        } else if (Global().IsReady(0)) {
            player_scores_[0] = 1;
            player_scores_[1] = 0;
            Global().Boardcast() << "玩家" << At(PlayerID{1}) << "超时判负";
        } else {
            player_scores_[1] = 1;
            player_scores_[0] = 0;
            Global().Boardcast() << "玩家" << At(PlayerID{0}) << "超时判负";
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
        if (Global().IsReady(pid)) {
            reply() << "落子失败：您已经落子过了";
            return StageErrCode::FAILED;
        }
        const auto decode_result = DecodePos(coor_str);
        if (const auto* errstr = std::get_if<std::string>(&decode_result)) {
            reply() << "落子失败：" << *errstr;
            return StageErrCode::FAILED;
        }
        const auto raw_coor = std::get<std::pair<uint32_t, uint32_t>>(decode_result);
        if (!board_.Place(Coor{static_cast<int32_t>(raw_coor.second), static_cast<int32_t>(raw_coor.first)},
                    PlayerIDToChessType_(pid))) {
            auto sender = reply();
            sender << "落子失败：您无法在此处落子，";
            const auto placable_positions = board_.PlacablePositions(PlayerIDToChessType_(pid));
            if (placable_positions.empty()) {
                sender << "您没有合法的落子点，本回合您无需落子";
            } else {
                sender << "合法的落子点包括";
                for (const Coor coor : placable_positions) {
                    sender << " " << std::string(1, 'A' + coor.col_) << coor.row_;
                }
            }
            return StageErrCode::FAILED;
        }
        reply() << "落子成功";
        placed_coors_[pid] = raw_coor;
        return StageErrCode::READY;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        const auto chess_type = PlayerIDToChessType_(pid);
        const auto avaliable_placements = board_.PlacablePositions(chess_type);
        if (!avaliable_placements.empty()) {
            const auto coor = avaliable_placements[std::rand() % avaliable_placements.size()];
            placed_coors_[pid] = std::pair{static_cast<uint32_t>(coor.row_), static_cast<uint32_t>(coor.col_)};
            const auto ret = board_.Place(coor, chess_type);
            assert(ret);
        }
        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        nlohmann::json json_array;
        const auto result = board_.Settlement();
        for (auto pid : {PlayerID{0}, PlayerID{1}}) {
            auto& coor = placed_coors_[pid];
            if (coor.has_value()) {
                json_array.push_back(std::string(1, 'A' + coor->first) + std::to_string(coor->second));
                coor.reset();
            } else {
                json_array.push_back(nullptr);
            }
            player_scores_[pid] = result[static_cast<uint8_t>(PlayerIDToChessType_(pid))];
            if (!board_.PlacablePositions(PlayerIDToChessType_(pid)).empty()) {
                Global().ClearReady(pid);
            }
        }
        Global().Boardcast() << "双方落子成功";
        Global().BoardcastAiInfo(nlohmann::json{
                    { "player_coordinates", std::move(json_array) },
                    { "board", board_.ToString() }
                });
        ++round_;
        Global().Group() << Markdown(ToHtml_());
        Global().Tell(0) << Markdown(ToHtml_());
        Global().Tell(1) << Markdown(ToHtml_());
        if (!Global().IsReady(0) || !Global().IsReady(1)) {
            Global().StartTimer(GAME_OPTION(时限));
            Global().Boardcast() << "请继续私信裁判坐标以落子（如 C3），时限 " << GAME_OPTION(时限) << " 秒，超时未落子即判负";
            return StageErrCode::CONTINUE;
        }
        return StageErrCode::CHECKOUT;
    }

    std::string ToHtml_() const
    {
        html::Table player_table(2, 4);
        player_table.MergeDown(0, 0, 2);
        player_table.MergeDown(0, 2, 2);
        player_table.SetTableStyle(" align=\"center\" cellpadding=\"1\" cellspacing=\"1\" ");
        const auto print_player = [&](const PlayerID pid)
            {
                player_table.Get(0, 0 + pid * 2).SetContent(std::string("![](file:///") + Global().ResourceDir() +
                        (PlayerIDToChessType_(pid) == ChessType::BLACK ? "black" : "white") + ".png)");
                player_table.Get(0, 1 + pid * 2).SetContent("**" + Global().PlayerName(pid) + "**");
                player_table.Get(1, 1 + pid * 2).SetContent("棋子数量： " HTML_COLOR_FONT_HEADER(yellow) + std::to_string(player_scores_[pid]) + HTML_FONT_TAIL);
            };
        print_player(0);
        print_player(1);
        return "## 第 " + std::to_string(round_) + " 回合\n\n" + player_table.ToString() + "\n\n" + board_.ToHtml();
    }

    static ChessType PlayerIDToChessType_(const PlayerID pid) { return pid ? ChessType::BLACK : ChessType::WHITE; }

    int round_;
    std::array<int64_t, 2> player_scores_;
    std::array<std::optional<std::pair<uint32_t, uint32_t>>, 2> placed_coors_;
    Board board_;
};

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot

