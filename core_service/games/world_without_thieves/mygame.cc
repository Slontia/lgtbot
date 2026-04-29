// Copyright (c) 2018-present, ZhengYang Xu <github.com/jeffxzy>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <array>
#include <functional>
#include <memory>
#include <set>

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;

const GameProperties k_properties {
    .name_ = "天下无贼", // the game name which should be unique among all the games
    .developer_ = "睦月",
    .description_ = "通过在民/警/贼身份中切换，尽可能活到最后的游戏",
    .shuffled_player_id_ = true,
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 0; } // 0 indicates no max-player limits
uint32_t Multiple(const CustomOptions& options) { return 2; } // the default score multiple for the game, 0 for a testing game, 1 for a formal game, 2 or 3 for a long formal game
const MutableGenericOptions k_default_generic_options;
const std::vector<RuleCommand> k_rule_commands = {};

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

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() < 3) {
        reply() << "该游戏至少 3 人参加，当前玩家数为 " << generic_options_readonly.PlayerNum();
        return false;
    }
    return true;
}

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options)
            {
                generic_options.bench_computers_to_player_num_ = 6;
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
        : StageFsm(std::move(utility), MakeStageCommand(*this, "查看当前游戏进展情况", &MainStage::Status_, VoidChecker("赛况")))
        , round_(0)
        , alive_(0)
        , player_scores_(Global().PlayerNum(), 0)
        , player_hp_(Global().PlayerNum(), 10)
        , player_select_(Global().PlayerNum(), 'N')
        , player_last_(Global().PlayerNum(), 'N')
        , player_target_(Global().PlayerNum(), 0)
        , player_eli_(Global().PlayerNum(), 0)
    {
    }

    virtual void FirstStageFsm(SubStageFsmSetter setter) override;
    virtual void NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }
    std::vector<int64_t> player_scores_;

    //生命值
    std::vector<int64_t> player_hp_;
    //选择
    std::vector<char> player_select_;
    //上次选择
    std::vector<char> player_last_;
    //选择的玩家
    std::vector<int64_t> player_target_;
    //已经被淘汰
    std::vector<int64_t> player_eli_;

    int alive_;
    std::string Pic="";
    std::string GetName(std::string x);

  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
//        reply() << "当前游戏情况：未设置";

        std::string PreBoard="";
        PreBoard+="本局玩家序号如下：\n";
        for(int i=0;i<Global().PlayerNum();i++)
        {
            PreBoard+=std::to_string(i+1)+" 号："+Global().PlayerName(i);

            if(i!=(int)Global().PlayerNum()-1)
            {
                PreBoard+="\n";
            }
        }

        Global().Boardcast() << PreBoard;

        std::string WordSitu="";
        WordSitu+="当前玩家状态如下：\n";
        for(int i=0;i<Global().PlayerNum();i++)
        {
            WordSitu+=std::to_string(i+1)+" 号：";

            WordSitu+=std::to_string(player_hp_[i])+"生命 ";
            if(player_select_[i]=='N') WordSitu+="民";
            if(player_select_[i]=='S') WordSitu+="贼  "+str(player_target_[i]);
            if(player_select_[i]=='P') WordSitu+="警  "+str(player_target_[i]);


            if(i!=(int)Global().PlayerNum()-1)
            {
                WordSitu+="\n";
            }
        }

//        select is updated unexpectedly, which may cause error, so use Pic instead now.
//        Global().Boardcast() << WordSitu;

        Global().Boardcast() << Markdown(Pic+"</table>");

        // Returning |OK| means the game stage
        return StageErrCode::OK;
    }

    int round_;
};


class RoundStage : public SubGameStage<>
{
  public:
    RoundStage(MainStage& main_stage, const uint64_t round)
        : StageFsm(main_stage, "第 " + std::to_string(round) + " 回合"
                , MakeStageCommand(*this, "选择民"
                                   , &RoundStage::Select_N_
                                   , VoidChecker("民"))
                , MakeStageCommand(*this, "选择贼"
                                   , &RoundStage::Select_S_
                                   , VoidChecker("贼")
                                   , ArithChecker<int64_t>(1, main_stage.Global().PlayerNum(), "target")
                                   )
                , MakeStageCommand(*this, "选择警"
                                   , &RoundStage::Select_P_
                                   , VoidChecker("警")
                                   , ArithChecker<int64_t>(1, main_stage.Global().PlayerNum(), "target")
                                   )
                    )
    {
    }

    virtual void OnStageBegin() override
    {
        Global().StartTimer(GAME_OPTION(时限));

        Global().Boardcast() << Name() << "，请玩家私信选择身份。";
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
//        Global().Boardcast() << Name() << "超时结束";

        for(int i=0;i<Global().PlayerNum();i++)
        {
//             Global().Boardcast()<<std::to_string(Global().IsReady(i))<<std::to_string(Main().player_eli_[i]);
             if(Global().IsReady(i) == false && !Main().player_eli_[i])
             {
                 Main().player_hp_[i]=0;
                 Main().player_select_[i]='N';
                 Main().player_target_[i]=0;
             }
        }


        Global().Boardcast() << "有玩家超时仍未行动，已被淘汰";
        RoundStage::calc();
        // Returning |CHECKOUT| means the current stage will be over.
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        int i=pid;
//        Global().Boardcast() << Global().PlayerName(i) << "退出游戏";
        Main().player_hp_[i]=0;
        Main().player_select_[i]='N';
        Main().player_target_[i]=0;
        // Returning |CONTINUE| means the current stage will be continued.
        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        auto select = Main().player_last_;
        auto eli = Main().player_eli_;
        int N = Global().PlayerNum();

        char S = 'N';
        int T = 0;

        int now = pid;
        if(select[now] == 'S')
        {
            int r = rand() % 100 + 1;
            if(r <= 12) S = 'S';
            else if(r <= 100) S = 'N';
        }
        if(select[now] == 'N')
        {
            int r = rand() % 100 + 1;
            if(r <= 15) S = 'S';
            else if(r <= 85) S = 'N';
            else if(r <= 100) S = 'P';
        }
        if(select[now] == 'P')
        {
            int r = rand() % 100 + 1;
            if(r <= 85) S = 'N';
            else if(r <= 100) S = 'P';
        }

        std::multiset<int> s[3];
        s[0].clear();s[1].clear();s[2].clear();
        for(int i = 0; i < N; i++)
        {
            if(eli[i] == 1 || i == now) continue;
            if(select[i] == 'S') s[0].insert(i);
            if(select[i] == 'N') s[1].insert(i);
            if(select[i] == 'P') s[2].insert(i);
        }
        if(S == 'S')
        {
            int to = 0;
            if(s[0].size() == 0 && s[1].size() == 0)
            {
                S = 'N';
                return Selected_(pid, reply, S, T + 1);
            }
            int r = rand() % 100 + 1;
            if(r <= 75) to = 0;
            else if(r <= 90) to = 1;
            else if(r <= 100) to = 2;

            if(to == 0 && s[0].size() == 0) to = 1;
            if(to == 1 && s[1].size() == 0) to = 0;
            if(to == 2 && s[2].size() == 0) return Selected_(pid, reply, S, T + 1);
            if(s[to].size() == 0)
            {
                Global().Boardcast() << "Unexpected error on f -> OnComputerAct -> Select == 'S' -> s[to].size() == 0";
                S = 'N';
                return Selected_(pid, reply, S, T + 1);
            }
            r = rand() % s[to].size();
            while(r != 0)
            {
                r--;
                s[to].erase(s[to].find(*s[to].begin()));
            }
            T = *s[to].begin();
        }
        if(S == 'P')
        {
            int to = 0;
            if(s[0].size() == 0 && s[1].size() == 0)
            {
                S = 'N';
                return Selected_(pid, reply, S, T + 1);
            }

            int r = rand() % 100 + 1;
            if(r <= 30) to = 0;
            else if(r <= 100) to = 1;

            if(to == 0 && s[0].size() == 0) to = 1;
            if(to == 1 && s[1].size() == 0) to = 0;

            if(s[to].size() == 0)
            {
                Global().Boardcast() << "Unexpected error on f -> OnComputerAct -> Select == 'P' -> s[to].size() == 0";
                S = 'N';
                return Selected_(pid, reply, S, T + 1);
            }
            r = rand() % s[to].size();
            while(r != 0)
            {
                r--;
                s[to].erase(s[to].find(*s[to].begin()));
            }
            T = *s[to].begin();
        }

        return Selected_(pid, reply, S, T + 1);
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        Global().Boardcast() << "所有玩家选择完成，下面公布赛况。";
        RoundStage::calc();
        return StageErrCode::CHECKOUT;
    }

    void calc()
    {
        bool exist_P=0;

        //check if police exist
        for(int i=0;i<Global().PlayerNum();i++)
        {
            if(Main().player_select_[i]=='P')
            {
                exist_P=1;
            }
        }
        //first calculate thives
        for(int i=0;i<Global().PlayerNum();i++)
        {
            if(Main().player_select_[i]=='S')
            {

                // S gains 1 hp
                Main().player_hp_[i]+=1;
                int tar=Main().player_target_[i]-1;


                if(Main().player_select_[tar]!='P')
                {
                    Main().player_hp_[tar]-=2;

                    // Steal success. S get 1 score.
                    Main().player_scores_[i]+=1;
                }
                else
                {
                    // Now the P will gains 1 hp when someone steal him
                    Main().player_hp_[tar]+=1;
                    Main().player_hp_[i]=0;

                    // Police success. P get 1 score.
                    Main().player_scores_[tar]+=1;
                }
            }
        }

        //then calculate polices
        for(int i=0;i<Global().PlayerNum();i++)
        {
            if(Main().player_select_[i]=='P')
            {
                Main().player_hp_[i]-=2;
                int tar=Main().player_target_[i]-1;


                if(Main().player_select_[tar]=='S')
                {

                    // OK catch a thief!
                    Main().player_hp_[i]+=3;
                    Main().player_hp_[tar]=0;

                    // Gain 2 score
                     Main().player_scores_[i]+=2;
                }
            }
        }

        //finally calculate commons
        for(int i=0;i<Global().PlayerNum();i++)
        {
            if(Main().player_select_[i]=='N')
            {
                if(exist_P==0)
                {
                    Main().player_hp_[i]-=2;
                }
            }
        }

        for(int i=0;i<Global().PlayerNum();i++)
        {
            if(Main().player_hp_[i]<0) Main().player_hp_[i]=0;
        }

        std::string x="<tr>";
        for(int i=0;i<Global().PlayerNum();i++)
        {
//            if(i%2)
//                x+="<td>";
//            else
                x+="<td bgcolor=\"#DCDCDC\">";

            if(Main().player_eli_[i]==1)
            {
                x+=" ";
            }
            else
            {
                if(Main().player_select_[i]=='N') x+="民";
                if(Main().player_select_[i]=='S') x+="贼  "+str(Main().player_target_[i]);
                if(Main().player_select_[i]=='P') x+="警  "+str(Main().player_target_[i]);
            }
            x+="</td>";
        }
        x+="</tr>";
        x+="<tr>";
        for(int i=0;i<Global().PlayerNum();i++)
        {
//            if(i%2)
                x+="<td>";
//            else
//                x+="<td bgcolor=\"#DCDCDC\">";

            if(Main().player_eli_[i]==0)
            {
                x+=std::to_string(Main().player_hp_[i]);
            }

            x+="</td>";
        }
        x+="</tr>";
        Main().Pic+=x;
        Global().Boardcast() << Markdown(Main().Pic+"</table>");

        for(int i=0;i<Global().PlayerNum();i++)
        {

            if(Main().player_hp_[i]<=0)
            {
                Main().player_select_[i]=' ';

                if(Main().player_eli_[i]==0)
                {
                    Main().player_eli_[i]=1;
                    Global().Eliminate(i);
                    Main().alive_--;
                }
            }
            else
            {
                // Everyone gain 1 score for his survival
                Main().player_scores_[i]++;
            }
        }

        for(int i=0;i<Global().PlayerNum();i++)
            Main().player_last_[i] = Main().player_select_[i];
    }

  private:
    AtomReqErrCode Select_N_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (is_public) {
            reply() << "[错误] 请私信裁判进行选择。";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
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
        if (Global().IsReady(pid)) {
            reply() << "[错误] 您本回合已经进行过选择了。";
            return StageErrCode::FAILED;
        }
        if(Main().player_select_[pid]=='P'){
            reply() << "[错误] 不能选择这一职业。";
            return StageErrCode::FAILED;
        }
        if(target<1 || target>Global().PlayerNum()||target-1==pid||Main().player_eli_[target-1]==1){
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
        if (Global().IsReady(pid)) {
            reply() << "[错误] 您本回合已经进行过选择了。";
            return StageErrCode::FAILED;
        }
        if(Main().player_select_[pid]=='S'){
            reply() << "[错误] 不能选择这一职业。";
            return StageErrCode::FAILED;
        }
        if(target<1 || target>Global().PlayerNum()||target-1==pid||Main().player_eli_[target-1]==1){
            reply() << "[错误] 目标无效。";
            return StageErrCode::FAILED;
        }
        return Selected_(pid, reply, 'P', target);
    }

    AtomReqErrCode Selected_(const PlayerID pid, MsgSenderBase& reply, char type, const int64_t target)
    {
        if(type != 'S' && type != 'N' && type != 'P')
        {
            Global().Boardcast() << "Wrong input on pid = " << std::to_string(pid) << std::to_string(type) << " in f->Selected";
        }
        Main().player_select_[pid] = type;
        Main().player_target_[pid] = target;
//        reply() << "设置成功。";
        // Returning |READY| means the player is ready. The current stage will be over when all surviving players are ready.
        return StageErrCode::READY;
    }
};

std::string MainStage::GetName(std::string x)
{
    std::string ret = "";
    int n = x.length();
    if(n == 0) return ret;

    int l = 0;
    int r = n - 1;

    if(x[0] == '<') l++;
    if(x[r] == '>')
    {
        while(r >= 0 && x[r] != '(') r--;
        r--;
    }

    for(int i = l; i <= r; i++)
    {
        ret += x[i];
    }
    return ret;
}

void MainStage::FirstStageFsm(SubStageFsmSetter setter)
{
    srand((unsigned int)time(NULL));
    alive_ = Global().PlayerNum();

    Pic += "<table><tr>";
    for(int i=0;i<Global().PlayerNum();i++)
    {
        Pic+="<th >"+std::to_string(i+1)+" 号： "+GetName(Global().PlayerName(i))+"　</th>";
        if(i % 4 == 3) Pic += "</tr><tr>";
    }
    Pic+="</tr><br>";

    Pic+="<table style=\"text-align:center\"><tbody>";
    Pic+="<tr>";
    for(int i=0;i<Global().PlayerNum();i++)
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
    for(int i=0;i<Global().PlayerNum();i++)
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
    for(int i=0;i<Global().PlayerNum();i++)
    {
        PreBoard+=std::to_string(i+1)+" 号："+Global().PlayerName(i);

        if(i!=(int)Global().PlayerNum()-1)
        {
            PreBoard+="\n";
        }
    }

    Global().Boardcast() << PreBoard;
    Global().Boardcast() << Markdown(Pic + "</table>");

    setter.Emplace<RoundStage>(*this, ++round_);
    return;
}

void MainStage::NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
//    Global().Boardcast()<<"alive"<<std::to_string(alive_);
    if ((++round_) <= GAME_OPTION(回合数)&&alive_>2) {
        setter.Emplace<RoundStage>(*this, round_);
        return;
    }
//    Global().Boardcast() << "游戏结束";
    int maxHP=0;
    for(int i=0;i<Global().PlayerNum();i++)
    {
        maxHP=std::max(maxHP,(int)player_hp_[i]);
    }
    for(int i=0;i<Global().PlayerNum();i++)
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
}

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

