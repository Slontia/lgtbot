// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <random>
#include <queue>
#include <regex>

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"

using namespace std;

#include "board.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;
const GameProperties k_properties { 
    .name_ = "困兽棋", // the game name which should be unique among all the games
    .developer_ = "铁蛋",
    .description_ = "使自己地盘更大的圈地游戏",
    .shuffled_player_id_ = true,
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 2; }
uint32_t Multiple(const CustomOptions& options) {
    return min(static_cast<int>((GET_OPTION_VALUE(options, 边长) - 3) / 2), 4);
}
const MutableGenericOptions k_default_generic_options{};
const std::vector<RuleCommand> k_rule_commands = {};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() != 2) {
        reply() << "该游戏为双人游戏，必须为2人参加，当前玩家数为 " << generic_options_readonly.PlayerNum();
        return false;
    }
    return true;
}

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("设置游戏配置",
        [] (CustomOptions& game_options, MutableGenericOptions& generic_options, const uint32_t& size, const uint32_t& step)
        {
            GET_OPTION_VALUE(game_options, 边长) = size;
            GET_OPTION_VALUE(game_options, 步数) = step;
            return NewGameMode::MULTIPLE_USERS;
        },
        ArithChecker<uint32_t>(5, 19, "边长"), OptionalDefaultChecker<ArithChecker<uint32_t>>(3, 1, 10, "步数")),
    InitOptionsCommand("独自一人开始游戏",
        [] (CustomOptions& game_options, MutableGenericOptions& generic_options)
        {
            generic_options.bench_computers_to_player_num_ = 2;
            return NewGameMode::SINGLE_USER;
        },
        VoidChecker("单机")),
};

// ========== GAME STAGES ==========

class RoundStage;

class MainStage : public MainGameStage<RoundStage>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility),
            MakeStageCommand(*this, "查看当前游戏进展情况", &MainStage::Status_, VoidChecker("赛况")))
        , round_(0)
        , player_scores_(Global().PlayerNum(), 0)
    {}

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }

    std::vector<int64_t> player_scores_;

    // 回合数 
	int round_;
    // 地图
    Board board;
    // 当前行动玩家
    PlayerID currentPlayer;
    
  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply){
        reply() << Markdown(board.GetMarkdown(round_, currentPlayer), (GAME_OPTION(边长) + 2) * 60 + 100);
        return StageErrCode::OK;
    }

    void FirstStageFsm(SubStageFsmSetter setter) override
    {
        srand((unsigned int)time(NULL));
        currentPlayer = 0;
        board.size = GAME_OPTION(边长);
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            board.players.emplace_back(Global().PlayerName(pid), Global().PlayerAvatar(pid, 40));
        }
        board.Initialize();

        setter.Emplace<RoundStage>(*this, ++round_);
    }

    void NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override
    {
        currentPlayer = 1 - currentPlayer;
        if (player_scores_[0] == 0 && player_scores_[1] == 0) {
            setter.Emplace<RoundStage>(*this, ++round_);
            return;
        }
        Global().Boardcast() << Markdown(board.GetMarkdown(round_, 2), (GAME_OPTION(边长) + 2) * 60 + 150);
    }
};

class RoundStage : public SubGameStage<>
{
   public:
    RoundStage(MainStage& main_stage, const uint64_t round)
            : StageFsm(main_stage, "第 " + std::to_string(round) + " 回合" ,
                MakeStageCommand(*this, "移动并放置墙壁", &RoundStage::MoveAndPlace_, AnyArg("坐标", "A1"), AlterChecker<Direct>(direction_map)),
                MakeStageCommand(*this, "认输", &RoundStage::Concede_, AlterChecker<uint32_t>({{"认输", 0}, {"投降", 0}})))
    {}

    virtual void OnStageBegin() override
    {
        Global().SetReady(1 - Main().currentPlayer);
        Global().Boardcast() << Markdown(Main().board.GetMarkdown(Main().round_, Main().currentPlayer), (GAME_OPTION(边长) + 2) * 60 + 150);
        Global().Boardcast() << "请 " << At(Main().currentPlayer) << " 选择坐标移动和放置墙壁，格式例如：B3 上。"
                             << "时限 " << GAME_OPTION(时限) << " 秒，超时未行动判负";
        Global().StartTimer(GAME_OPTION(时限));
    }

   private:
    AtomReqErrCode MoveAndPlace_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const string str, const Direct direction)
    {
        if (pid != Main().currentPlayer) {
            reply() << "[错误] 本回合并非您的回合";
            return StageErrCode::FAILED;
        }
        string s = str;
        string result = Board::CheckCoordinate(s);
		if (result != "OK") {
            reply() << result;
            return StageErrCode::FAILED;
        }
		auto pos = Board::TranString(s);
        int step = Main().board.CheckMoveStep(pid, pos);
        if (step == -1) {
            reply() << "[错误] 移动失败：目标位置无法到达";
            return StageErrCode::FAILED;
        }
        if (step > GAME_OPTION(步数)) {
            reply() << "[错误] 移动失败：所需步数 " << step << "步 超出最大步数限制 " << GAME_OPTION(步数) << "步";
            return StageErrCode::FAILED;
        }
        const string direct_str[4] = {"上", "下", "左", "右"};
        if (Main().board.CheckWall(pos, direction)) {
            reply() << "[错误] 放置墙壁失败：仅能放置于空位，目标位置" << direct_str[static_cast<int>(direction)] << "方已有墙壁";
            return StageErrCode::FAILED;
        }

        Main().board.MoveAndPlace(pid, pos, direction);

        return StageErrCode::READY;
    }

    AtomReqErrCode Concede_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const uint32_t null) {
        Main().player_scores_[pid] = -1;
        Global().Boardcast() << "玩家 " << At(PlayerID(pid)) << " 认输，游戏结束。";
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        if (!Global().IsReady(0)) {
            Main().player_scores_[0] = -1;
            Global().Boardcast() << "玩家 " << At(PlayerID(0)) << " 超时判负";
        } else if (!Global().IsReady(1)) {
            Main().player_scores_[1] = -1;
            Global().Boardcast() << "玩家 " << At(PlayerID(1)) << " 超时判负";
        }
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Global().Boardcast() << "玩家 " <<  At(PlayerID(pid)) << " 强退认输。";
        Main().player_scores_[pid] = -1;
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        if (Main().board.IsGameOver()) {
            auto [area0, area1] = Main().board.GetAreaSize();
            if (area0 > area1) {
                Main().player_scores_[0] = 1;
                Global().Boardcast() << "游戏结束，玩家 " <<  At(PlayerID(0)) << " 获胜！";
            } else if (area0 < area1) {
                Main().player_scores_[1] = 1;
                Global().Boardcast() << "游戏结束，玩家 " <<  At(PlayerID(1)) << " 获胜！";
            } else {
                Main().player_scores_[Main().currentPlayer] = 1;
                Global().Boardcast() << "游戏结束，双方棋子所在区域大小相同，判定最后行动玩家 " <<  At(Main().currentPlayer) << " 获胜！";
            }
        }
        return StageErrCode::CHECKOUT;
    }
    
    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        pair<int, int> pos;
        int direction, step;
        int attempt = 0;
        do {
            pos.first = rand() % GAME_OPTION(边长);
            pos.second = rand() % GAME_OPTION(边长);
            direction = rand() % 4;
            step = Main().board.CheckMoveStep(pid, pos);
            attempt++;
        } while ((step > GAME_OPTION(步数) || step == -1 || Main().board.CheckWall(pos, static_cast<Direct>(direction))) && attempt <= 1000);
        if (attempt <= 1000) {
            Main().board.MoveAndPlace(pid, pos, static_cast<Direct>(direction));
        }
        return StageErrCode::READY;
    }
};

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot

