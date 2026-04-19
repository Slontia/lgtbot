// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

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
    .name_ = "六贯棋", // the game name which should be unique among all the games
    .developer_ = "铁蛋",
    .description_ = "在由六边形组成的棋盘上轮流落子，联通两侧底盘来获胜",
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 2; } // 0 indicates no max-player limits
uint32_t Multiple(const CustomOptions& options) {
    if (GET_OPTION_VALUE(options, 边长) <= 11) return 1;
    if (GET_OPTION_VALUE(options, 边长) <= 14) return 2;
    if (GET_OPTION_VALUE(options, 边长) <= 17) return 3;
    return 4;
}
const MutableGenericOptions k_default_generic_options;
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
    InitOptionsCommand("独自一人开始游戏",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options)
            {
                generic_options.bench_computers_to_player_num_ = 2;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
    InitOptionsCommand("设置游戏边长",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options, const int& size)
            {
                GET_OPTION_VALUE(game_options, 边长) = size;
                return NewGameMode::MULTIPLE_USERS;
            },
            ArithChecker<uint32_t>(7, 19, "边长")),
};

// ========== GAME STAGES ==========

class RoundStage;

class MainStage : public MainGameStage<RoundStage>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility), MakeStageCommand(*this, "查看当前游戏进展情况", &MainStage::Status_, VoidChecker("赛况")))
        , round_(0)
        , player_scores_(Global().PlayerNum(), 0)
    {}

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }

    std::vector<int64_t> player_scores_;

    // 当前行动玩家
    PlayerID currentPlayer;
    // 棋盘
    Board board;
    // 回合数 
	int round_;

  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << Markdown(board.GetUI(currentPlayer));
        // Returning |OK| means the game stage
        return StageErrCode::OK;
    }

    void FirstStageFsm(SubStageFsmSetter setter) override
    {
        // 随机先后手
        srand((unsigned int)time(NULL));
        currentPlayer = rand() % 2;

        board.size = GAME_OPTION(边长);
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            board.name[pid] = Global().PlayerName(pid);
            if (board.name[pid][0] == '<') {
                board.name[pid] = board.name[pid].substr(1, board.name[pid].size() - 2);
            }
            board.player_color[pid] = pid + 1;
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
        Global().Boardcast() << Markdown(board.GetUI(-1));
    }
};

class RoundStage : public SubGameStage<>
{
   public:
    RoundStage(MainStage& main_stage, const uint64_t round)
            : StageFsm(main_stage, "第 " + std::to_string(round) + " 回合" ,
                MakeStageCommand(*this, "落子", &RoundStage::PlaceChess_, ArithChecker<uint32_t>(1, 361, "位置")),
                MakeStageCommand(*this, "在第 4 回合，后手玩家可以选择是否交换", &RoundStage::SwapColor_, VoidChecker("交换")),
                MakeStageCommand(*this, "认输", &RoundStage::Concede_, AlterChecker<uint32_t>({{"认输", 0}, {"投降", 0}})))
    {}

    bool swap = false;

    virtual void OnStageBegin() override
    {
        Global().SetReady(1 - Main().currentPlayer);
        Global().Boardcast() << Markdown(Main().board.GetUI(Main().currentPlayer));
        if (Main().round_ == 4) {
            Global().Boardcast() << "请 " << color_ch[Main().board.player_color[Main().currentPlayer]] << "方" << At(Main().currentPlayer) << " 行动，本回合您有2种选择：\n"
                                 << "1. 发送「交换」：双方玩家交换棋子颜色，然后由您的对手落子\n"
                                 << "2. 选择位置落子：不交换颜色，落子后继续游戏\n"
                                 << "时限 " << GAME_OPTION(时限) << " 秒，超时未行动判负";
        } else {
            Global().Boardcast() << "请 " << color_ch[Main().board.player_color[Main().currentPlayer]] << "方" << At(Main().currentPlayer) << " 选择位置落子。"
                                 << "时限 " << GAME_OPTION(时限) << " 秒，超时未行动判负";
        }
        Global().StartTimer(GAME_OPTION(时限));
    }

   private:
    AtomReqErrCode PlaceChess_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int num)
    {
        if (Global().IsReady(pid)) {
            reply() << "[错误] 本回合并非您的回合";
            return StageErrCode::FAILED;
        }
        string result = Main().board.PlaceChess(num, Main().currentPlayer);
        if (result != "OK") {
            reply() << result;
            return StageErrCode::FAILED;
        }
        swap = false;
        return StageErrCode::READY;
    }

    AtomReqErrCode SwapColor_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (Main().round_ != 4) {
            reply() << "[错误] 当前并非第4回合，无法执行交换操作";
            return StageErrCode::FAILED;
        }
        if (swap) {
            reply() << "[错误] 本局游戏颜色已经发生交换，无法再次交换！";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 您本局为先手方，无法执行交换操作";
            return StageErrCode::FAILED;
        }
        Main().board.SwapColor();
        Global().Boardcast() << "[提示] 后手选择交换，双方玩家颜色互换！";
        swap = true;
        return StageErrCode::READY;
    }

    AtomReqErrCode Concede_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const uint32_t null) {
        Main().player_scores_[pid] = -1;
        Global().Boardcast() << color_ch[Main().board.player_color[pid]] << "方" << At(PlayerID(pid)) << "认输，游戏结束。";
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
        Global().Boardcast() << color_ch[Main().board.player_color[pid]] << "方" << At(PlayerID(pid)) << " 强退认输。";
        Main().player_scores_[pid] = -1;
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        if (swap) {
            Main().currentPlayer = 1 - Main().currentPlayer;
            Global().ClearReady(Main().currentPlayer);
            Global().Boardcast() << Markdown(Main().board.GetUI(Main().currentPlayer));
            Global().Boardcast() << "请 " << color_ch[Main().board.player_color[Main().currentPlayer]] << "方" << At(Main().currentPlayer) << " 选择位置落子。"
                                 << "时限 " << GAME_OPTION(时限) << " 秒，超时未行动判负";
            Global().StartTimer(GAME_OPTION(时限));
            return StageErrCode::CONTINUE;
        }

        // 判断当前胜负
        int winner = Main().board.WinCheck();
        if (winner != -1) {
            Main().player_scores_[Main().currentPlayer] = 1;
            Global().Boardcast() << color_ch[Main().board.player_color[Main().currentPlayer]] << "方" << At(Main().currentPlayer) << " 棋子联通了地图，获得胜利！";
        }
        return StageErrCode::CHECKOUT;
    }
    
    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        if (pid == Main().currentPlayer) {
            string result;
            while (result != "OK") {
                int num = rand() % (Main().board.size * Main().board.size);
                result = Main().board.PlaceChess(num, pid);
            }
        }
        return StageErrCode::READY;
    }
};

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot

