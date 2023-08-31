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

#include "game_framework/game_stage.h"
#include "utility/html.h"

#include "problems.h"
#include "rules.h"

using namespace std;

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = GameStage<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = GameStage<void, SubStages...>;

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
        : GameStage(option, match, MakeStageCommand("查看当前游戏进展情况", &MainStage::Status_, VoidChecker("赛况"),
            OptionalDefaultChecker<BoolChecker>(true, "图片", "文字")))
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
    vector<string> finalBoard;
    Question* question;
    set<int> used;

    int round_;
//---------------------------------------------------------------------------//

  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const bool show_image){

         string s = "";
         s += "乌合之众: \n第" + str(round_) + " / " + str(GET_OPTION_VALUE(option(), 回合数)) + " 回合\n\n";
         s += specialRule(players, GET_OPTION_VALUE(option(), 特殊规则), "gameStart");
         if(GET_OPTION_VALUE(option(), 特殊规则) == 0) s += "无";
         s += "\n\n当前题目\n";
         s += "题号：" + str(question -> id) + "\n";
         s += "出题者：" + (question -> author) + "\n";
         s += "题目：" + (question -> title) + "\n";
         Boardcast() << s;

         if (show_image) {
            Boardcast() << Markdown(question -> Markdown());
         } else {
            Boardcast() << question -> String();
         }
         
        return StageErrCode::OK;
    }

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
        constexpr static uint32_t k_question_num = 41;

        Question *& q = main_stage().question;


        int r = -1;
        int count = 0;
        while(r == -1 || main_stage().used.find(r) != main_stage().used.end())
        {
            r = rand() % k_question_num;
            if(count++ > 1000) break;
        }
        main_stage().used.insert(r);


        if(GET_OPTION_VALUE(option(), 测试) != 0)
            r = GET_OPTION_VALUE(option(), 测试) - 1;

        static const std::array<Question*(*)(), k_question_num> create_question{
            []() -> Question* { return new Q1(); },
            []() -> Question* { return new Q2(); },
            []() -> Question* { return new Q3(); },
            []() -> Question* { return new Q4(); },
            []() -> Question* { return new Q5(); },
            []() -> Question* { return new Q6(); },
            []() -> Question* { return new Q7(); },
            []() -> Question* { return new Q8(); },
            []() -> Question* { return new Q9(); },
            []() -> Question* { return new Q10(); },
            []() -> Question* { return new Q11(); },
            []() -> Question* { return new Q12(); },
            []() -> Question* { return new Q13(); },
            []() -> Question* { return new Q14(); },
            []() -> Question* { return new Q15(); },
            []() -> Question* { return new Q16(); },
            []() -> Question* { return new Q17(); },
            []() -> Question* { return new Q18(); },
            []() -> Question* { return new Q19(); },
            []() -> Question* { return new Q20(); },
            []() -> Question* { return new Q21(); },
            []() -> Question* { return new Q22(); },
            []() -> Question* { return new Q23(); },
            []() -> Question* { return new Q24(); },
            []() -> Question* { return new Q25(); },
            []() -> Question* { return new Q26(); },
            []() -> Question* { return new Q27(); },
            []() -> Question* { return new Q28(); },
            []() -> Question* { return new Q29(); },
            []() -> Question* { return new Q30(); },
            []() -> Question* { return new Q31(); },
            []() -> Question* { return new Q32(); },
            []() -> Question* { return new Q33(); },
            []() -> Question* { return new Q34(); },
            []() -> Question* { return new Q35(); },
            []() -> Question* { return new Q36(); },
            []() -> Question* { return new Q37(); },
            []() -> Question* { return new Q38(); },
            []() -> Question* { return new Q39(); },
            []() -> Question* { return new Q40(); },
            []() -> Question* { return new Q41(); },
        };

        q = create_question[r]();

        if(q == NULL)
        {
            Boardcast() << "Q == NULL in RoundStage -> OnStageBegin where r == " + to_string(r);
            Boardcast() << "发生了不可预料的错误。请中断游戏。";
            StartTimer(GET_OPTION_VALUE(option(), 时限));
            return;
        }

        q -> init(main_stage().players);
        q -> initTexts();
        q -> initOptions();
        q -> initExpects();

        Boardcast() << Markdown(q -> Markdown());

        StartTimer(GET_OPTION_VALUE(option(), 时限));
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

    void calc()
    {
        Question *& q = main_stage().question;
        vector<Player> & p = main_stage().players;

        for(int i = 0; i < option().PlayerNum(); i++)
            p[i].realLastScore = p[i].lastScore;
        for(int i = 0; i < option().PlayerNum(); i++)
            p[i].lastScore = p[i].score;


        q -> initCalc(p);
        q -> calc(p);
        q -> quickScore(p);

        for(int i = 0; i < option().PlayerNum(); i++)
            std::cout << i << " " << p[i].score << std::endl;

        int specialRule_ = GET_OPTION_VALUE(option(), 特殊规则);
        specialRule(p, specialRule_, "roundEnd");


        vector<string> & fb = main_stage().finalBoard;
        string b = "";
        b += "<table style=\"text-align:center\"><tbody>";
        b += "<tr><td><font size=7>　　　　　　　　　　</font></td><td><font size=7>　　</font></td>";
        b += "<td><font size=7>　　　　</font></td><td><font size=7>　　　　</font></td></tr>";
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


            fb[i] += (string)""
                    + "<td " + color + "><font size=7>" + (char)(p[i].select + 'A') + "</font></td>"
                    + "<td " + color + "><font size=7>"
                    + (p[i].score >= p[i].lastScore ? "+ " : "- " )
                    + str(abs(p[i].score - p[i].lastScore))
                    + "</font></td>";


        }
        b += "</table>";


        if(specialRule_ != 1)
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
        finalBoard.push_back("");
    }

    for(int i = 0; i < players.size(); i++)
    {
        finalBoard[i] += (string)"<tr>" + "<td bgcolor=\"#D2F4F4\"><font size=7>" + strName(PlayerName(i)) + "</font></td>";
    }

    int specialRule_ = GET_OPTION_VALUE(option(), 特殊规则);
    if(specialRule_ != 0)
    {
        Boardcast() << specialRule(players, specialRule_, "gameStart");
    }

    return make_unique<RoundStage>(*this, ++round_);
}

MainStage::VariantSubStage MainStage::NextSubStage(RoundStage& sub_stage, const CheckoutReason reason)
{
    if ((++round_) <= GET_OPTION_VALUE(option(), 回合数)) {
        return make_unique<RoundStage>(*this, round_);
    }


    // OK game ends here

    int specialRule_ = GET_OPTION_VALUE(option(), 特殊规则);
    specialRule(players, specialRule_, "gameEnd");
    for(int i = 0; i < players.size(); i++)
    {
        finalBoard[i] += "<td bgcolor=\"#E1FFFF\"><font size=7>" + str(players[i].score) + "</font></td></tr>";
    }

    string fb = "<table style=\"text-align:center\"><tbody>";
    fb += "<tr><td><font size=7>　　　　　　　　　　</font></td>";
    for(int i = 1; i < round_; i++)
        fb += "<td><font size=7>　　</font></td><td><font size=7>　　　</font></td>";
    fb += "<td><font size=7>　　　　</font></td></tr>";
    for(int i = 0; i < finalBoard.size(); i++)
    {
        fb += finalBoard[i];
    }
    fb += "</table>";
    Boardcast() << Markdown(fb);




    for(int i = 0; i < option().PlayerNum(); i++)
    {
        player_scores_[i] = dec2(players[i].score) * 100;
    }


    return {};
}

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

#include "game_framework/make_main_stage.h"
