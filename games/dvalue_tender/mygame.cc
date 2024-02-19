// Copyright (c) 2018-present, ZhengYang Xu <github.com/jeffxzy>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <array>
#include <functional>
#include <memory>
#include <set>

#include "game_framework/game_stage.h"
#include "utility/html.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = GameStage<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = GameStage<void, SubStages...>;

const std::string k_game_name = "差值投标"; // the game name which should be unique among all the games
const uint64_t k_max_player = 2; // 0 indicates no max-player limits
const uint64_t k_multiple = 1; // the default score multiple for the game, 0 for a testing game, 1 for a formal game, 2 or 3 for a long formal game
const std::string k_developer = "睦月";
const std::string k_description = "简单的拼点游戏";
const std::vector<RuleCommand> k_rule_commands = {};

// a function to give one-digit number two space in its left, in order to align correctly in text broadcast.
std::string myToStrL(int x)
{
    std::string ret = "";
    if(x < 10) ret += "  ";
    ret += std::to_string(x);

    return ret;
}
// The same, but to the right.
std::string myToStrR(int x)
{
    std::string ret = "";
    ret += std::to_string(x);
    if(x < 10) ret += "  ";

    return ret;
}




std::string GameOption::StatusInfo() const
{
    return "共 " + std::to_string(GET_VALUE(回合数)) + " 回合，每回合超时时间 " + std::to_string(GET_VALUE(时限)) + " 秒";
}

bool GameOption::ToValid(MsgSenderBase& reply)
{
    if (PlayerNum() != 2) {
        reply() << "该游戏必须 2 人参加，当前玩家数为 " << PlayerNum();
        return false;
    }
    return true;
}

uint64_t GameOption::BestPlayerNum() const { return 2; }

// ========== GAME STAGES ==========

class RoundStage;

class MainStage : public MainGameStage<RoundStage>
{
  public:
    MainStage(const GameOption& option, MatchBase& match)
        : GameStage(option, match, MakeStageCommand("查看当前游戏进展情况", &MainStage::Status_, VoidChecker("赛况")))
        , round_(0)
        , player_scores_(option.PlayerNum(), 0)
        , player_coins_(option.PlayerNum(), 0)
        , player_wins_(option.PlayerNum(), 0)
        , player_now_(option.PlayerNum(), 0)
    {
    }

    virtual VariantSubStage OnStageBegin() override;
    virtual VariantSubStage NextSubStage(RoundStage& sub_stage, const CheckoutReason reason) override;

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }

    // The standard score
    std::vector<int64_t> player_scores_;

    // remain coins
    std::vector<int64_t> player_coins_;

    // count of winned rounds
    std::vector<int64_t> player_wins_;

    // This round coin
    std::vector<int64_t> player_now_;

    std::string roundBoard;
    int round_;

  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
//        reply() << "这里输出当前游戏情况";
        // Returning |OK| means the game stage

        Boardcast() << roundBoard;

        std::string mdBoard = "";

        mdBoard += "<p>";

        int len = roundBoard.length();
        for(int i = 0;i < len;i++){

            if(roundBoard[i] == '\n'){
                mdBoard += "<br>";
            }
            else if(roundBoard[i] == ' '){
                mdBoard += "&emsp;";
                while(i+1 < len && roundBoard[i+1] == ' ') i++;
            }
            else if(roundBoard[i] == '|'){
                mdBoard += "";
            }
            else{
                mdBoard += roundBoard[i];
            }

        }

        mdBoard += "<p>";

        Boardcast() << Markdown(mdBoard);
        return StageErrCode::OK;
    }


};


class RoundStage : public SubGameStage<>
{
  public:
    RoundStage(MainStage& main_stage, const uint64_t round)
        : GameStage(main_stage, "第 " + std::to_string(round) + " 回合",
                MakeStageCommand("下分", &RoundStage::GiveCoin_, ArithChecker<int64_t>(0, 1000000, "金币")))
    {
    }

    virtual void OnStageBegin() override
    {
        StartTimer(GET_OPTION_VALUE(option(), 时限));
//        Boardcast() << name() << "开始";
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
//        Boardcast() << name() << "超时结束";
        // Returning |CHECKOUT| means the current stage will be over.

        for(int i=0;i<option().PlayerNum();i++)
        {
//             Boardcast()<<std::to_string(IsReady(i))<<std::to_string(main_stage().player_eli_[i]);
             if(IsReady(i) == false)
             {
                 main_stage().player_now_[i]=0;
             }
        }


        RoundStage::calc();

        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
//        Boardcast() << PlayerName(pid) << "退出游戏";
        // Returning |CONTINUE| means the current stage will be continued.

        main_stage().player_now_[pid]=0;

        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        int c1 = main_stage().player_coins_[0];
        int c2 = main_stage().player_coins_[1];
        int w1 = main_stage().player_wins_[0];
        int w2 = main_stage().player_wins_[1];
        int r = main_stage().round_;
        int f = 0;


        if(c1 == 0)
        {
            if(c2 == 0) return GiveCoinInternal_(pid, reply, 0);
            return GiveCoinInternal_(pid, reply, 1);
        }

        if(r <= 9)
        {
            if((c1 + 1) * (10 - r) <= c2)
            {
                f = c1 + 1;
            }
            else
            {
                if(w1 == 0)
                {
                    f = rand() % 9;
                }
                if(w1 == 1)
                {
                    if(rand() % 3) f = rand() % 3 + 3;
                    else if(rand() % 2) f = rand() % 2 + 9;
                    else f = 0;
                }
                if(w1 == 2)
                {
                    if(rand() % 2) f = rand() % 3;
                    else if(rand() % 2) f = rand() % 2 + 5;
                    else f = rand() % 2 + 9;
                }
                if(w1 == 3)
                {
                    if(rand() % 2) f = rand() % 3 + 2;
                    else f = rand() % 2 + 10;
                }
                if(w1 == 4)
                {
                    if(rand() % 2)
                    {
                        f = 1;
                        if(rand() % 2) f = 3;
                    }
                    else
                    {
                        f = c1 + 1;
                        if(rand() % 2 && c1 > 5) f -= rand() % (c1/2);
                    }
                }
            }
        }
        else
        {
            if((c1 + 1) * (13 - r) <= c2)
            {
                f = c1 + 1;
            }
            else
            {
                f = rand() % (c2 + 1);
            }
        }

        if(f < 0) f = 0;
        if(f > c2) f = c2;

        return GiveCoinInternal_(pid, reply, f);
    }

    virtual CheckoutErrCode OnStageOver() override
    {
//        Boardcast() << "所有玩家准备完成";

        RoundStage::calc();
        return StageErrCode::CHECKOUT;
    }

    void calc()
    {
        int now0 = main_stage().player_now_[0],
            now1 = main_stage().player_now_[1];
        std::string symbolW = "=";
        if(now0 > now1){
            main_stage().player_wins_[0]++;
            main_stage().player_coins_[0] -= (now0 - now1);
            symbolW = ">";
        }
        else if(now0 < now1){
            main_stage().player_wins_[1]++;
            main_stage().player_coins_[1] -= (now1 - now0);
            symbolW = "<";
        }

        main_stage().roundBoard+=
                "\n"
                + std::to_string(main_stage().player_wins_[0])
                + " |   "
                + myToStrL(main_stage().player_coins_[0])
                + "    "
                + myToStrL(main_stage().player_now_[0])
                + " "
                + symbolW
                + " "
                + myToStrR(main_stage().player_now_[1])
                + "    "
                + myToStrR(main_stage().player_coins_[1])
                + "   | "
                + std::to_string(main_stage().player_wins_[1])
                ;

        Boardcast() << main_stage().roundBoard;
    }

  private:
    AtomReqErrCode GiveCoin_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int64_t coins)
    {
        if (is_public) {
            reply() << "[错误] 请私信裁判选择";
            return StageErrCode::FAILED;
        }
        if (IsReady(pid)) {
            reply() << "[错误] 您本回合已经做出过选择";
            return StageErrCode::FAILED;
        }

        if(coins > main_stage().player_coins_[pid]){
            reply() << "[错误] 金币数量不够。";
            return StageErrCode::FAILED;
        }
        if(coins < 0 || coins > GET_OPTION_VALUE(option(), 金币)){
            reply() << "[错误] 不合法的金币数。";
            return StageErrCode::FAILED;
        }


        return GiveCoinInternal_(pid, reply, coins);
    }

    AtomReqErrCode GiveCoinInternal_(const PlayerID pid, MsgSenderBase& reply, const int64_t coins)
    {
//        auto& player_score = main_stage().player_scores_[pid];
//        player_score += score;

        main_stage().player_now_[pid]=coins;

        reply() << "设置成功。";
        // Returning |READY| means the player is ready. The current stage will be over when all surviving players are ready.
        return StageErrCode::READY;
    }
};

MainStage::VariantSubStage MainStage::OnStageBegin()
{
    srand((unsigned int)time(NULL));

    roundBoard+="当前结果：";
    for(int i = 0; i < option().PlayerNum(); i++){
        player_coins_[i] = GET_OPTION_VALUE(option(), 金币);
    }

    return std::make_unique<RoundStage>(*this, ++round_);
}

MainStage::VariantSubStage MainStage::NextSubStage(RoundStage& sub_stage, const CheckoutReason reason)
{
    round_++;

//    Boardcast()<<std::to_string(round_)<<std::to_string(player_wins_[0])<<std::to_string(player_wins_[1]);

    if (round_ != 10 && round_ != 13) {
        if(
          (round_ < 10 && (player_wins_[0] < 5 && player_wins_[1] < 5) )
          ||(round_ > 10 && (player_wins_[0] < 6 && player_wins_[1] < 6) )
                )
        return std::make_unique<RoundStage>(*this, round_);
    }
    else if(round_ == 10){
        if(player_wins_[0] == player_wins_[1]){
            return std::make_unique<RoundStage>(*this, round_);
        }
    }
    // Returning empty variant means the game will be over.

    if(player_wins_[0] > player_wins_[1])
    player_scores_[0] = 1;

    if(player_wins_[0] < player_wins_[1])
    player_scores_[1] = 1;

    std::string finalBoard = "";
    finalBoard += "游戏结束。\n ";
    finalBoard += std::to_string(player_wins_[0]) + " - " + std::to_string(player_wins_[1]) + "\n";
    if(player_wins_[0] > player_wins_[1])
        finalBoard += PlayerName(0) + "获胜。";
    else if(player_wins_[0] < player_wins_[1])
        finalBoard += PlayerName(1) + "获胜。";
    else
        finalBoard += PlayerName(0) + PlayerName(1) + "双方平局。";

    Boardcast() << finalBoard;

    return {};
}

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

#include "game_framework/make_main_stage.h"
