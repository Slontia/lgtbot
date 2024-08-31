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
    .name_ = "此路不通", // the game name which should be unique among all the games
    .developer_ = "铁蛋",
    .description_ = "猜测并阻止对手移动棋子，使棋子达成3连的游戏",
};
uint64_t MaxPlayerNum(const MyGameOptions& options) { return 2; } // 0 indicates no max-player limits
uint32_t Multiple(const MyGameOptions& options) { return 1; } // the default score multiple for the game, 0 for a testing game, 1 for a formal game, 2 or 3 for a long formal game
const MutableGenericOptions k_default_generic_options;
const std::vector<RuleCommand> k_rule_commands = {};

bool AdaptOptions(MsgSenderBase& reply, MyGameOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() != 2) {
        reply() << "该游戏为双人游戏，必须为2人参加，当前玩家数为 " << generic_options_readonly.PlayerNum();
        return false;
    }
    return true;
}

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [] (MyGameOptions& game_options, MutableGenericOptions& generic_options)
            {
                generic_options.bench_computers_to_player_num_ = 2;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
};

// ========== GAME STAGES ==========

class StartStage;
class RoundStage;

class MainStage : public MainGameStage<StartStage, RoundStage>
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
        reply() << Markdown(board.GetUI(false));
        // Returning |OK| means the game stage
        return StageErrCode::OK;
    }

    void FirstStageFsm(SubStageFsmSetter setter)
    {
        // 随机先后手
        srand((unsigned int)time(NULL));
        currentPlayer = rand() % 2;

        board.targetScore = GAME_OPTION(目标);
        board.size = GAME_OPTION(边长);
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            board.name[pid] = Global().PlayerName(pid);
            if (board.name[pid][0] == '<') {
                board.name[pid] = board.name[pid].substr(1, board.name[pid].size() - 2);
            }
            board.score[pid] = 0;
        }
        board.Initialize();

        setter.Emplace<StartStage>(*this, ++round_);
    }

    void NextStageFsm(StartStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
    {
        // 超时直接结束游戏
        if (reason == CheckoutReason::BY_TIMEOUT || reason == CheckoutReason::BY_LEAVE) {
            Global().Boardcast() << Markdown(board.GetUI(false));
            return;
        }
        currentPlayer = 1- currentPlayer;
        if ((++round_) <= (GAME_OPTION(棋子) - 2) * 2) {
            setter.Emplace<StartStage>(*this, round_);
            return;
        }
        round_ = 1;
        setter.Emplace<RoundStage>(*this, round_);
    }

    void NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
    {
        currentPlayer = 1- currentPlayer;
        if ((++round_) <= GAME_OPTION(回合数) && player_scores_[0] == 0 && player_scores_[1] == 0) {
            setter.Emplace<RoundStage>(*this, round_);
            return;
        }
        if (round_ > GAME_OPTION(回合数)) {
            Global().Boardcast() << "回合数已达上限，游戏平局！";
        }
        Global().Boardcast() << Markdown(board.GetUI(false));
    }
};

class StartStage : public SubGameStage<>
{
   public:
    StartStage(MainStage& main_stage, const uint64_t round)
            : StageFsm(main_stage, "落子阶段",
                    MakeStageCommand(*this, "为对手落子", &StartStage::PlaceChess_, AnyArg("位置", "A1")))
    {}

    virtual void OnStageBegin() override
    {
        Global().Boardcast() << Markdown(Main().board.GetUI(true));
        Main().board.ClearLast();
        if (Main().round_ == 1) {
            Global().Boardcast() << "游戏开始！\n" << At(Main().currentPlayer) << " 先手，执" << color_ch[Main().currentPlayer] << "。请为对手放置棋子";
        } else {
            Global().Boardcast() << "请 " << color_ch[Main().currentPlayer] << "方" << At(Main().currentPlayer) << " 行动，为对手放置棋子";
        }
        Global().SetReady(1 - Main().currentPlayer);
        Global().StartTimer(GAME_OPTION(时限));
    }

   private:
    AtomReqErrCode PlaceChess_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const string str)
    {
        if (pid != Main().currentPlayer) {
            reply() << "[错误] 当前不是您的回合";
            return StageErrCode::FAILED;
        }
        
        string result = Main().board.PlaceChess(str, 1 - Main().currentPlayer);
        if (result != "OK") {
            reply() << result;
            return StageErrCode::FAILED;
        }
        
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        Global().Boardcast() << color_ch[Main().currentPlayer] << "方" << At(Main().currentPlayer) << " 超时判负。";
        Main().player_scores_[Main().currentPlayer] = -1;
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Global().Boardcast() << color_ch[pid] << "方" << At(PlayerID(pid)) << " 强退认输。";
        Main().player_scores_[pid] = -1;
        return StageErrCode::CHECKOUT;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (pid == Main().currentPlayer) {
            string result;
            while (result != "OK") {
                int X = rand() % Main().board.size + 1;
                int Y = rand() % Main().board.size + 1;
                result = Main().board.PlaceChess(string(1, 'A' + X - 1) + to_string(Y), 1 - Main().currentPlayer);
            }
            return StageErrCode::READY;
        }
        return StageErrCode::OK;
    }
};

class RoundStage : public SubGameStage<>
{
   public:
    RoundStage(MainStage& main_stage, const uint64_t round)
            : StageFsm(main_stage, "第 " + std::to_string(round) + " 回合" ,
                MakeStageCommand(*this, "移动棋子", &RoundStage::MoveChess_, AnyArg("起始位置", "A1"), AnyArg("结束位置", "A2")),
                MakeStageCommand(*this, "猜测对手的移动方向", &RoundStage::GuessMove_, AlterChecker<int>({{"上", 1}, {"下", 2}, {"左", 3}, {"右", 4}})))
    {}

    int guess_;

    virtual void OnStageBegin() override
    {
        Global().Boardcast() << Markdown(Main().board.GetUI(true));
        Global().Boardcast() << "请 " << color_ch[Main().currentPlayer] << "方" << At(Main().currentPlayer) << " 私信裁判移动棋子\n"
                             << "请 " << At(PlayerID(1 - Main().currentPlayer)) << " 设定本回合的禁走方向\n"
                             << "时限 " << GAME_OPTION(时限) << " 秒，超时未行动即判负";
        Global().Tell(0) << Markdown(Main().board.GetUI(false));
        Global().Tell(1) << Markdown(Main().board.GetUI(false));
        Global().StartTimer(GAME_OPTION(时限));
    }

   private:
    AtomReqErrCode MoveChess_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const string str1, const string str2)
    {
        if (pid != Main().currentPlayer) {
            reply() << "[错误] 本回合您为猜测者，无法移动棋子";
            return StageErrCode::FAILED;
        }
        if (is_public) {
            reply() << "[错误] 请私信裁判移动棋子";
            return StageErrCode::FAILED;
        }
        string result = Main().board.RecordChessMove(str1, str2, Main().currentPlayer);
        if (result != "OK") {
            reply() << result;
            return StageErrCode::FAILED;
        }
        reply() << "行动完成：" << str1 << " → " << str2;
        return StageErrCode::READY;
    }

    AtomReqErrCode GuessMove_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int guess)
    {
        if (pid == Main().currentPlayer) {
            reply() << "[错误] 本回合您为移动者，无法进行猜测";
            return StageErrCode::FAILED;
        }
        if (is_public) {
            reply() << "[错误] 请私信裁判进行猜测";
            return StageErrCode::FAILED;
        }
        guess_ = guess;
        reply() << "行动完成：禁走方向为" << direction_ch[guess];
        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        if (!Global().IsReady(0) && !Global().IsReady(1)) {
            Global().Boardcast() << "双方均超时，游戏平局";
        } else if (!Global().IsReady(0)) {
            Main().player_scores_[0] = -1;
            Global().Boardcast() << "玩家 " << At(PlayerID(0)) << " 超时判负";
        } else {
            Main().player_scores_[1] = -1;
            Global().Boardcast() << "玩家 " << At(PlayerID(1)) << " 超时判负";
        }
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Global().Boardcast() << color_ch[pid] << "方" << At(PlayerID(pid)) << " 强退认输。";
        Main().player_scores_[pid] = -1;
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        string start_pos = string(1, 'A' + Main().board.lastX1 - 1) + to_string(Main().board.lastY1);
        string end_pos = string(1, 'A' + Main().board.lastX2 - 1) + to_string(Main().board.lastY2);
        
        // 结算猜测结果
        int result = Main().board.GuessResult(guess_);
        if (result == 0) {
            // 猜测正确，阻止移动并获得1分
            Main().board.score[1 - Main().currentPlayer]++;

            Global().Boardcast() << Markdown(Main().board.GetUI(true));

            Global().Boardcast() << At(Main().currentPlayer) << " 将棋子从 " << start_pos << " 移动至 " << end_pos << "\n"
                                 << At(PlayerID(1 - Main().currentPlayer)) << " 设定禁止向" << direction_ch[guess_] << "走\n"
                                 << "【猜测成功】阻止移动并获得 1 分，当前分数为 " << Main().board.score[1 - Main().currentPlayer];
        } else {
            // 猜测失败，正常执行移动
            Main().board.chess[Main().board.lastX1][Main().board.lastY1] = 0;
            Main().board.chess[Main().board.lastX2][Main().board.lastY2] = Main().currentPlayer + 1;

            Global().Boardcast() << Markdown(Main().board.GetUI(true));

            Global().Boardcast() << At(Main().currentPlayer) << " 将棋子从 " << start_pos << " 移动至 " << end_pos << "\n"
                                 << At(PlayerID(1 - Main().currentPlayer)) << " 设定禁止向" << direction_ch[guess_] << "走\n"
                                 << "猜测失败，实际移动方向为" << direction_ch[result];
        }

        Main().board.ClearLast();

        // 判断当前胜负
        if (Main().board.score[1 - Main().currentPlayer] == GAME_OPTION(目标)) {
            Main().player_scores_[1 - Main().currentPlayer] = 1;
            Global().Boardcast() << color_ch[1 - Main().currentPlayer] << "方" << At(PlayerID(1 - Main().currentPlayer)) << " 分数达到了目标分，获得胜利！";
        }
        int winner = Main().board.WinCheck();
        if (winner != -1) {
            Main().player_scores_[Main().currentPlayer] = 1;
            Global().Boardcast() << color_ch[Main().currentPlayer] << "方" << At(Main().currentPlayer) << " 棋子达成三连，获得胜利！";
        }

        return StageErrCode::CHECKOUT;
    }
    
    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        if (pid == Main().currentPlayer) {
            int X, Y, addx, addy;
            string result;
            while (result != "OK") {
                int c = rand() % GAME_OPTION(棋子) + 1;
                for (int i = 1; i <= Main().board.size; i++) {
                    for (int j = 1; j <= Main().board.size; j++) {
                        if (Main().board.chess[i][j] == pid + 1 && c >= 0) {
                            c--; X = i; Y = j;
                        }
                    }
                }
                addx = addy = 0;
                if (rand() % 2) {
                    addx = rand() % 2 == 1 ? 1 : -1;
                } else {
                    addy = rand() % 2 == 1 ? 1 : -1;
                }
                string start_pos = string(1, 'A' + X - 1) + to_string(Y);
                string end_pos = string(1, 'A' + X + addx - 1) + to_string(Y + addy);
                result = Main().board.RecordChessMove(start_pos, end_pos, pid);
            }
        } else {
            guess_ = rand() % 4 + 1;
        }
        return StageErrCode::READY;
    }
};

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot

