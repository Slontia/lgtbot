// Copyright (c) 2018-present, ZhengYang Xu <github.com/jeffxzy>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <array>
#include <functional>
#include <memory>
#include <set>

#include <map>
#include <vector>
#include <set>
#include <string>
#include <algorithm>

#include "game_framework/game_main.h"
#include "game_framework/game_stage.h"
#include "game_framework/game_options.h"
#include "utility/msg_checker.h"
#include "utility/html.h"

#include "problems.h"


using namespace std;


const string k_game_name = "乌合之众"; // the game name which should be unique among all the games
const uint64_t k_max_player = 0; // 0 indicates no max-player limits
const uint64_t k_multiple = 0; // the default score multiple for the game, 0 for a testing game,
//1 for a formal game, 2 or 3 for a long formal game
const std::string k_developer = "睦月";
const std::string k_description = "选择恰当选项，尽可能获取分数的游戏";




string GameOption::StatusInfo() const
{
    return "共 " + to_string(GET_VALUE(回合数)) + " 回合，每回合超时时间 " + to_string(GET_VALUE(时限)) + " 秒";
}

bool GameOption::ToValid(MsgSenderBase& reply)
{
    if (PlayerNum() < 4) {
        reply() << "该游戏至少 4 人参加，当前玩家数为 " << PlayerNum();
        return false;
    }
    return true;
}

uint64_t GameOption::BestPlayerNum() const { return 10; }

// ========== GAME STAGES ==========

class RoundStage;

class MainStage : public MainGameStage<RoundStage>
{
  public:
    MainStage(const GameOption& option, MatchBase& match)
        : GameStage(option, match, MakeStageCommand("查看当前游戏进展情况", &MainStage::Status_, VoidChecker("赛况")))
        , round_(0)
        , player_scores_(option.PlayerNum(), 0)
    {
    }

    virtual VariantSubStage OnStageBegin() override;
    virtual VariantSubStage NextSubStage(RoundStage& sub_stage, const CheckoutReason reason) override;

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }

    vector<int64_t> player_scores_;

//---------------------------------------------------------------------------//

    vector<Player> players;
    Question* question;
    set<int> used;

//---------------------------------------------------------------------------//

  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply){return StageErrCode::OK;}

    int round_;
};


class RoundStage : public SubGameStage<>
{
  public:
    RoundStage(MainStage& main_stage, const uint64_t round)
        : GameStage(main_stage, "第 " + to_string(round) + " 回合",
                MakeStageCommand("选择", &RoundStage::Submit_, AnyArg("提交", "提交")))
    {
    }

    virtual void OnStageBegin() override
    {
        Question *& q = main_stage().question;


        int r = -1;
        int count = 0;
        while(r == -1 || main_stage().used.find(r) != main_stage().used.end())
        {
            r = rand() % 30 + 1;
            if(count++ > 1000) break;
        }
        main_stage().used.insert(r);


        if(option().GET_VALUE(测试) != 0)
            r = option().GET_VALUE(测试);
        if(r > 30)
            r = 1;

        if(r == 1) q = new Q1();
        if(r == 2) q = new Q2();
        if(r == 3) q = new Q3();
        if(r == 4) q = new Q4();
        if(r == 5) q = new Q5();
        if(r == 6) q = new Q6();
        if(r == 7) q = new Q7();
        if(r == 8) q = new Q8();
        if(r == 9) q = new Q9();
        if(r == 10) q = new Q10();
        if(r == 11) q = new Q11();
        if(r == 12) q = new Q12();
        if(r == 13) q = new Q13();
        if(r == 14) q = new Q14();
        if(r == 15) q = new Q15();
        if(r == 16) q = new Q16();
        if(r == 17) q = new Q17();
        if(r == 18) q = new Q18();
        if(r == 19) q = new Q19();
        if(r == 20) q = new Q20();
        if(r == 21) q = new Q21();
        if(r == 22) q = new Q22();
        if(r == 23) q = new Q23();
        if(r == 24) q = new Q24();
        if(r == 25) q = new Q25();
        if(r == 26) q = new Q26();
        if(r == 27) q = new Q27();
        if(r == 28) q = new Q28();
        if(r == 29) q = new Q29();
        if(r == 30) q = new Q30();

        if(q == NULL)
        {
            Boardcast() << "Q == NULL in RoundStage -> OnStageBegin where r == " + to_string(r);
            Boardcast() << " 发生了不可预料的错误。请中断游戏。";
            StartTimer(option().GET_VALUE(时限));
            return;
        }

        q -> init(main_stage().players);
        q -> initTexts();
        q -> initOptions();
        q -> initExpects();

        Boardcast() << Markdown(q -> Markdown());

        StartTimer(option().GET_VALUE(时限));
    }

    virtual CheckoutErrCode OnTimeout() override
    {
        // Returning |CHECKOUT| means the current stage will be over.

        for(int i=0;i<option().PlayerNum();i++)
        {
             if(IsReady(i) == false)
             {
                 main_stage().players[i].select = 0;
             }
        }

        calc();

        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        // Returning |CONTINUE| means the current stage will be continued.
        main_stage().players[pid].select = 0;

        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        string x = "A";
        Question *& q = main_stage().question;

        if(q -> expects.size() == 0 || q -> expects[0].length() == 0)
            return SubmitInternal_(pid, reply, x);

        x[0] = q -> expects[0][rand() % (q -> expects[0].length())];
        if(x[0] <= 'z' && x[0] >= 'a') x[0] = x[0] - 'a' + 'A';

        return SubmitInternal_(pid, reply, x);
    }

    virtual void OnAllPlayerReady() override
    {
        calc();
    }

    string str(double x)
    {
        string ret = "";
        x = dec2(x);
        string sx = to_string(x);
        int n = sx.length();
        for(int i = 0; i < n; i++)
        {
            if(sx[i] == '.')
            {
                if(i + 2 < n && (sx[i + 1] != '0' || sx[i + 2] != '0'))
                {
                    ret += sx[i];
                    ret += sx[i + 1];
                    if(sx[i + 2] != '0')
                        ret += sx[i + 2];
                }
                break;
            }
            ret += sx[i];
        }
        return ret;
    }

    double dec2(double x)
    {
        x = x * 100;
        if(x > 0) x += 0.5;
        else x -= 0.5;
        x = (int) x;
        x = x / 100;

        return x;
    }

    string strName(string x)
    {
        string ret = "";
        int n = x.length();
        if(n == 0) return ret;

        int l = 0;
        int r = n - 1;

        if(x[0] == '<') l++;
        if(x[r] == '>')
        {
            while(r >= 0 && x[r] != '(')
                r--;
            r--;
        }
        for(int i = l; i <= r; i++)
            ret += x[i];

        return ret;
    }
    void calc()
    {
        Question *& q = main_stage().question;
        vector<Player> & p = main_stage().players;


        for(int i = 0; i < option().PlayerNum(); i++)
            p[i].lastScore = p[i].score;


        q -> initCalc(p);
        q -> calc(p);
        q -> quickScore(p);



        string b = "";
        b += "<table style=\"text-align:center\"><tbody>";
        b += "<tr><td><font size=7>　　　　　　　　　　</font></td><td><font size=7>　　</font></td>";
        b += "<td><font size=7>　　　　</font></td><td><font size=7>　　　　</font></td><tr>";
        for(int i = 0; i < option().PlayerNum(); i++)
        {
            string color;
            if(main_stage().players[i].lastScore < main_stage().players[i].score) color = "bgcolor=\"#90EE90\"";
            if(main_stage().players[i].lastScore == main_stage().players[i].score) color = "bgcolor=\"#F5DEB3\"";
            if(main_stage().players[i].lastScore > main_stage().players[i].score) color = "bgcolor=\"#FFA07A\"";

            b += (string)""
                    + "<tr>"
                    + "<td bgcolor=\"#D2F4F4\"><font size=7>" + strName(PlayerName(i)) + "</font></td>"
                    + "<td " + color + "><font size=7>" + (char)(p[i].select + 'A') + "</font></td>"
                    + "<td " + color + "><font size=7>"
                    + (p[i].score >= p[i].lastScore ? "+ " : "- " )
                    + str(abs(p[i].score - p[i].lastScore))
                    + "</font></td>"
                    + "<td bgcolor=\"#E1FFFF\"><font size=7>" + str(p[i].score) + "</font></td>"
                    + "</tr>";
        }
        b += "</table>";

        Boardcast() << Markdown(b);


//        std::this_thread::sleep_for(std::chrono::seconds(5));


        for(int i = 0; i < option().PlayerNum(); i++)
        {
            main_stage().player_scores_[i] = dec2(p[i].score) * 100;
        }


        delete q;
        q = NULL;
    }


  private:
    AtomReqErrCode Submit_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, string submission)
    {
        if (is_public) {
            reply() << "[错误] 请私信裁判选择";
            return StageErrCode::FAILED;
        }
        if (IsReady(pid)) {
            reply() << "[错误] 您本回合已经选择过了";
            return StageErrCode::FAILED;
        }
        if(submission.length() != 1)
        {
            reply() << "[错误] 请提交单个字母。";
            return StageErrCode::FAILED;
        }

        if(submission[0] <= 'z' && submission[0] >= 'a')
        {
            submission[0] = submission[0] - 'a' + 'A';
        }

        if(submission[0] > 'Z' || submission[0] < 'A')
        {
            reply() << "[错误] 请提交选项首字母。";
            return StageErrCode::FAILED;
        }
        if(submission[0] - 'A' + 1 > main_stage().question -> options.size())
        {
            reply() << "[错误] 选项不存在。";
            return StageErrCode::FAILED;
        }

        return SubmitInternal_(pid, reply, submission);
    }

    AtomReqErrCode SubmitInternal_(const PlayerID pid, MsgSenderBase& reply, string submission)
    {
        main_stage().players[pid].select = submission[0] - 'A';
        return StageErrCode::READY;
    }
};

MainStage::VariantSubStage MainStage::OnStageBegin()
{
    srand((unsigned int)time(NULL));

    Player tempP;
    for(int i = 0; i < option().PlayerNum(); i++)
    {
        players.push_back(tempP);
    }


    return make_unique<RoundStage>(*this, ++round_);
}

MainStage::VariantSubStage MainStage::NextSubStage(RoundStage& sub_stage, const CheckoutReason reason)
{
    if ((++round_) <= option().GET_VALUE(回合数)) {
        return make_unique<RoundStage>(*this, round_);
    }

    return {};
}

MainStageBase* MakeMainStage(MsgSenderBase& reply, GameOption& options, MatchBase& match)
{
    if (!options.ToValid(reply)) {
        return nullptr;
    }
    return new MainStage(options, match);
}
