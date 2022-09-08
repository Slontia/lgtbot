// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <array>
#include <functional>
#include <memory>
#include <set>

#include "game_framework/game_main.h"
#include "game_framework/game_stage.h"
#include "game_framework/game_options.h"
#include "utility/msg_checker.h"
#include "utility/html.h"

const std::string k_game_name = "天下无贼"; // the game name which should be unique among all the games
const uint64_t k_max_player = 99; // 0 indicates no max-player limits
const uint64_t k_multiple = 1; // the default score multiple for the game, 0 for a testing game,
//1 for a formal game, 2 or 3 for a long formal game

std::string str(int x)
{
    std::string y="";
    if(x==0)
    {
        y+='0';
    }

    while(x)
    {
        y+=x%10+'0';
        x/=10;
    }
    int n=y.length();

    std::string z="";
    for(int i=n-1;i>=0;i--)
    {
        z+= y[i];
    }
    return z;
}

std::string GameOption::StatusInfo() const
{
    return "共 " + std::to_string(GET_VALUE(回合数)) + " 回合，每回合超时时间 " + std::to_string(GET_VALUE(时限)) + " 秒";
}

bool GameOption::ToValid(MsgSenderBase& reply)
{
    if (PlayerNum() < 3) {
        reply() << "该游戏至少 3 人参加，当前玩家数为 " << PlayerNum();
        return false;
    }
    return true;
}

uint64_t GameOption::BestPlayerNum() const { return 6; }

// ========== GAME STAGES ==========

class RoundStage;

class MainStage : public MainGameStage<RoundStage>
{
  public:
    MainStage(const GameOption& option, MatchBase& match)
        : GameStage(option, match, MakeStageCommand("查看当前游戏进展情况", &MainStage::Status_, VoidChecker("赛况")))
        , round_(0)
        , alive_(0)
        , player_scores_(option.PlayerNum(), 0)
        , player_hp_(option.PlayerNum(), 10)
        , player_select_(option.PlayerNum(), 'N')
        , player_target_(option.PlayerNum(), 0)
        , player_eli_(option.PlayerNum(), 0)
    {
    }

    virtual VariantSubStage OnStageBegin() override;
    virtual VariantSubStage NextSubStage(RoundStage& sub_stage, const CheckoutReason reason) override;

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }
    std::vector<int64_t> player_scores_;

    //生命值
    std::vector<int64_t> player_hp_;
    //最后选择
    std::vector<char> player_select_;
    //选择的玩家
    std::vector<int64_t> player_target_;
    //已经被淘汰
    std::vector<int64_t> player_eli_;

    int alive_;
    std::string Pic="";

  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
//        reply() << "当前游戏情况：未设置";

        std::string PreBoard="";
        PreBoard+="本局玩家序号如下：\n";
        for(int i=0;i<option().PlayerNum();i++)
        {
            PreBoard+=std::to_string(i+1)+" 号："+PlayerName(i);

            if(i!=(int)option().PlayerNum()-1)
            {
                PreBoard+="\n";
            }
        }

        Boardcast() << PreBoard;

        std::string WordSitu="";
        WordSitu+="当前玩家状态如下：\n";
        for(int i=0;i<option().PlayerNum();i++)
        {
            WordSitu+=std::to_string(i+1)+" 号：";

            WordSitu+=std::to_string(player_hp_[i])+"生命 ";
            if(player_select_[i]=='N') WordSitu+="民";
            if(player_select_[i]=='S') WordSitu+="贼  "+str(player_target_[i]);
            if(player_select_[i]=='P') WordSitu+="警  "+str(player_target_[i]);


            if(i!=(int)option().PlayerNum()-1)
            {
                WordSitu+="\n";
            }
        }

        Boardcast() << WordSitu;

        // Returning |OK| means the game stage
        return StageErrCode::OK;
    }

    int round_;
};


class RoundStage : public SubGameStage<>
{
  public:
    RoundStage(MainStage& main_stage, const uint64_t round)
        : GameStage(main_stage, "第 " + std::to_string(round) + " 回合"
                , MakeStageCommand("选择民"
                                   , &RoundStage::Select_N_
                                   , VoidChecker("民"))
                , MakeStageCommand("选择贼"
                                   , &RoundStage::Select_S_
                                   , VoidChecker("贼")
                                   , ArithChecker<int64_t>(1, main_stage.option().PlayerNum(), "target")
                                   )
                , MakeStageCommand("选择警"
                                   , &RoundStage::Select_P_
                                   , VoidChecker("警")
                                   , ArithChecker<int64_t>(1, main_stage.option().PlayerNum(), "target")
                                   )
                    )
    {
    }

    virtual void OnStageBegin() override
    {
        StartTimer(option().GET_VALUE(时限));

        Boardcast() << name() << "，请玩家私信选择身份。";
    }

    virtual CheckoutErrCode OnTimeout() override
    {
//        Boardcast() << name() << "超时结束";

        for(int i=0;i<option().PlayerNum();i++)
        {
//             Boardcast()<<std::to_string(IsReady(i))<<std::to_string(main_stage().player_eli_[i]);
             if(IsReady(i) == false && !main_stage().player_eli_[i])
             {
                 Boardcast() << "玩家 "<<main_stage().PlayerName(i)<<" 超时仍未行动，已被淘汰";
                 main_stage().player_hp_[i]=0;
                 main_stage().player_select_[i]='N';
                 main_stage().player_target_[i]=0;
             }
        }


        RoundStage::calc();
        // Returning |CHECKOUT| means the current stage will be over.
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        int i=pid;
        Boardcast() << PlayerName(i) << "退出游戏";
        main_stage().player_hp_[i]=0;
        main_stage().player_select_[i]='N';
        main_stage().player_target_[i]=0;
        // Returning |CONTINUE| means the current stage will be continued.
        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        return Selected_(pid, reply, 'N', 0);
    }

    virtual void OnAllPlayerReady() override
    {
        Boardcast() << "所有玩家选择完成，下面公布赛况。";
        RoundStage::calc();
    }

    void calc()
    {
        bool exist_P=0;

        //check if police exist
        for(int i=0;i<option().PlayerNum();i++)
        {
            if(main_stage().player_select_[i]=='P')
            {
                exist_P=1;
            }
        }
        //first calculate thives
        for(int i=0;i<option().PlayerNum();i++)
        {
            if(main_stage().player_select_[i]=='S')
            {

                // S gains 1 hp
                main_stage().player_hp_[i]+=1;
                int tar=main_stage().player_target_[i]-1;


                if(main_stage().player_select_[tar]!='P')
                {
                    main_stage().player_hp_[tar]-=2;

                    // Steal success. S get 1 score.
                    main_stage().player_scores_[i]+=1;
                }
                else
                {
                    // Now the P will gains 1 hp when someone steal him
                    main_stage().player_hp_[tar]-=2;
                    main_stage().player_hp_[i]=0;

                    // Police success. P get 1 score.
                    main_stage().player_scores_[tar]+=1;
                }
            }
        }

        //then calculate polices
        for(int i=0;i<option().PlayerNum();i++)
        {
            if(main_stage().player_select_[i]=='P')
            {
                main_stage().player_hp_[i]-=2;
                int tar=main_stage().player_target_[i]-1;


                if(main_stage().player_select_[tar]=='S')
                {

                    // OK catch a thief!
                    main_stage().player_hp_[i]+=3;
                    main_stage().player_hp_[tar]=0;

                    // Gain 2 score
                     main_stage().player_scores_[i]+=2;
                }
            }
        }

        //finally calculate commons
        for(int i=0;i<option().PlayerNum();i++)
        {
            if(main_stage().player_select_[i]=='N')
            {
                if(exist_P==0)
                {
                    main_stage().player_hp_[i]-=2;
                }
            }
        }

        for(int i=0;i<option().PlayerNum();i++)
        {
            if(main_stage().player_hp_[i]<0) main_stage().player_hp_[i]=0;
        }

        std::string x="<tr>";
        for(int i=0;i<option().PlayerNum();i++)
        {
//            if(i%2)
//                x+="<td>";
//            else
                x+="<td bgcolor=\"#DCDCDC\">";

            if(main_stage().player_eli_[i]==1)
            {
                x+=" ";
            }
            else
            {
                if(main_stage().player_select_[i]=='N') x+="民";
                if(main_stage().player_select_[i]=='S') x+="贼  "+str(main_stage().player_target_[i]);
                if(main_stage().player_select_[i]=='P') x+="警  "+str(main_stage().player_target_[i]);
            }
            x+="</td>";
        }
        x+="</tr>";
        x+="<tr>";
        for(int i=0;i<option().PlayerNum();i++)
        {
//            if(i%2)
                x+="<td>";
//            else
//                x+="<td bgcolor=\"#DCDCDC\">";

            if(main_stage().player_eli_[i]==0)
            {
                x+=std::to_string(main_stage().player_hp_[i]);
            }

            x+="</td>";
        }
        x+="</tr>";
        main_stage().Pic+=x;
        Boardcast() << Markdown(main_stage().Pic+"</table>");


        int maxHP=0;
        for(int i=0;i<option().PlayerNum();i++)
        {
            maxHP=std::max(maxHP,(int)main_stage().player_hp_[i]);
        }

        main_stage().alive_=0;
        for(int i=0;i<option().PlayerNum();i++)
        {
            if(main_stage().player_hp_[i]<=0)
            {
                main_stage().player_select_[i]=' ';

                if(main_stage().player_eli_[i]==0)
                {
                    main_stage().player_eli_[i]=1;
                    Eliminate(i);
                }
            }
            else
            {
                // Everyone gain 1 score for his survival
                main_stage().alive_++;
                main_stage().player_scores_[i]++;
            }
        }
    }

  private:
    AtomReqErrCode Select_N_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (is_public) {
            reply() << "[错误] 请私信裁判进行选择。";
            return StageErrCode::FAILED;
        }
        if (IsReady(pid)) {
            reply() << "[错误] 您本回合已经进行过选择了。";
            return StageErrCode::FAILED;
        }
        return Selected_(pid, reply, 'N', 0);
    }

    AtomReqErrCode Select_S_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int64_t target)
    {
        if (is_public) {
            reply() << "[错误] 请私信裁判进行选择。";
            return StageErrCode::FAILED;
        }
        if (IsReady(pid)) {
            reply() << "[错误] 您本回合已经进行过选择了。";
            return StageErrCode::FAILED;
        }
        if(main_stage().player_select_[pid]=='P'){
            reply() << "[错误] 不能选择这一职业。";
            return StageErrCode::FAILED;
        }
        if(target<1 || target>option().PlayerNum()||target-1==pid||main_stage().player_eli_[target-1]==1){
            reply() << "[错误] 目标无效。";
            return StageErrCode::FAILED;
        }
        return Selected_(pid, reply, 'S', target);
    }

    AtomReqErrCode Select_P_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int64_t target)
    {
        if (is_public) {
            reply() << "[错误] 请私信裁判进行选择。";
            return StageErrCode::FAILED;
        }
        if (IsReady(pid)) {
            reply() << "[错误] 您本回合已经进行过选择了。";
            return StageErrCode::FAILED;
        }
        if(main_stage().player_select_[pid]=='S'){
            reply() << "[错误] 不能选择这一职业。";
            return StageErrCode::FAILED;
        }
        if(target<1 || target>option().PlayerNum()||target-1==pid||main_stage().player_eli_[target-1]==1){
            reply() << "[错误] 目标无效。";
            return StageErrCode::FAILED;
        }
        return Selected_(pid, reply, 'P', target);
    }

    AtomReqErrCode Selected_(const PlayerID pid, MsgSenderBase& reply, char type, const int64_t target)
    {
        main_stage().player_select_[pid]=type;
        main_stage().player_target_[pid]=target;

//        reply() << "设置成功。";
        // Returning |READY| means the player is ready. The current stage will be over when all surviving players are ready.
        return StageErrCode::READY;
    }
};

MainStage::VariantSubStage MainStage::OnStageBegin()
{
    Pic+="<table style=\"text-align:center\"><tbody>";
    Pic+="<tr>";
    for(int i=0;i<option().PlayerNum();i++)
    {
//        if(i%2)
//            Pic+="<th>";
//        else
            Pic+="<th bgcolor=\"#FFE4C4\">";

        //Please note that this is not 2 spaces but a special symbol.
        Pic+="　"+std::to_string(i+1)+" 号　";
        Pic+="</th>";
    }
    Pic+="</tr>";

    Pic+="<tr>";
    for(int i=0;i<option().PlayerNum();i++)
    {
//        if(i%2)
            Pic+="<td>";
//        else
//            Pic+="<td bgcolor=\"#DCDCDC\">";
        Pic+="10";
        Pic+="</td>";
    }
    Pic+="</tr>";

    std::string PreBoard="";
    PreBoard+="本局玩家序号如下：\n";
    for(int i=0;i<option().PlayerNum();i++)
    {
        PreBoard+=std::to_string(i+1)+" 号："+PlayerName(i);

        if(i!=(int)option().PlayerNum()-1)
        {
            PreBoard+="\n";
        }
    }

    Boardcast() << PreBoard;

    return std::make_unique<RoundStage>(*this, ++round_);
}

MainStage::VariantSubStage MainStage::NextSubStage(RoundStage& sub_stage, const CheckoutReason reason)
{
//    Boardcast()<<"alive"<<std::to_string(alive_);
    if ((++round_) <= option().GET_VALUE(回合数)&&alive_>2) {
        return std::make_unique<RoundStage>(*this, round_);
    }
//    Boardcast() << "游戏结束";
    int maxHP=0;
    for(int i=0;i<option().PlayerNum();i++)
    {
        maxHP=std::max(maxHP,(int)player_hp_[i]);
    }
    for(int i=0;i<option().PlayerNum();i++)
    {
        if(maxHP==player_hp_[i])
        {
            // The last survival gain 5 scores.
            player_scores_[i]+=5;
        }
        else if(player_eli_[i]==0)
        {
            // The second last survival
            player_scores_[i]+=3;
        }
    }
    // Returning empty variant means the game will be over.
    return {};
}
MainStageBase* MakeMainStage(MsgSenderBase& reply, GameOption& options, MatchBase& match)
{
    if (!options.ToValid(reply)) {
        return nullptr;
    }
    return new MainStage(options, match);
}
