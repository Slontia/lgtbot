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

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"

#include "problems.h"
#include "rules.h"

using namespace std;

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;

const string k_game_name = "乌合之众"; // the game name which should be unique among all the games
uint64_t MaxPlayerNum(const MyGameOptions& options) { return 0; } // 0 indicates no max-player limits
uint32_t Multiple(const MyGameOptions& options) {
    return GET_OPTION_VALUE(options, 测试模式) || GET_OPTION_VALUE(options, 测试) ? 0 : 1;
} // the default score multiple for the game, 0 for a testing game, 1 for a formal game, 2 or 3 for a long formal game
const std::string k_developer = "睦月";
const std::string k_description = "选择恰当选项，尽可能获取分数的游戏";
const MutableGenericOptions k_default_generic_options;


// formal questions
constexpr static uint32_t k_question_num = 68;
// with test questions
constexpr static uint32_t all_question_num = 108;


static const std::array<Question*(*)(), all_question_num> create_question{
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
    []() -> Question* { return new Q42(); },
    []() -> Question* { return new Q43(); },
    []() -> Question* { return new Q44(); },
    []() -> Question* { return new Q45(); },
    []() -> Question* { return new Q46(); },
    []() -> Question* { return new Q47(); },
    []() -> Question* { return new Q48(); },
    []() -> Question* { return new Q49(); },
    []() -> Question* { return new Q50(); },
    []() -> Question* { return new Q51(); },
    []() -> Question* { return new Q52(); },
    []() -> Question* { return new Q53(); },
    []() -> Question* { return new Q54(); },
    []() -> Question* { return new Q55(); },
    []() -> Question* { return new Q56(); },
    []() -> Question* { return new Q57(); },
    []() -> Question* { return new Q58(); },
    []() -> Question* { return new Q59(); },
    []() -> Question* { return new Q60(); },
    []() -> Question* { return new Q61(); },
    []() -> Question* { return new Q62(); },
    []() -> Question* { return new Q63(); },
    []() -> Question* { return new Q64(); },
    []() -> Question* { return new Q65(); },
    []() -> Question* { return new Q66(); },
    []() -> Question* { return new Q67(); },
    []() -> Question* { return new Q68(); },
    // test questions
    []() -> Question* { return new Q69(); },
    []() -> Question* { return new Q70(); },
    []() -> Question* { return new Q71(); },
    []() -> Question* { return new Q72(); },
    []() -> Question* { return new Q73(); },
    []() -> Question* { return new Q74(); },
    []() -> Question* { return new Q75(); },
    []() -> Question* { return new Q76(); },
    []() -> Question* { return new Q77(); },
    []() -> Question* { return new Q78(); },
    []() -> Question* { return new Q79(); },
    []() -> Question* { return new Q80(); },
    []() -> Question* { return new Q81(); },
    []() -> Question* { return new Q82(); },
    []() -> Question* { return new Q83(); },
    []() -> Question* { return new Q84(); },
    []() -> Question* { return new Q85(); },
    []() -> Question* { return new Q86(); },
    []() -> Question* { return new Q87(); },
    []() -> Question* { return new Q88(); },
    []() -> Question* { return new Q89(); },
    []() -> Question* { return new Q90(); },
    []() -> Question* { return new Q91(); },
    []() -> Question* { return new Q92(); },
    []() -> Question* { return new Q93(); },
    []() -> Question* { return new Q94(); },
    []() -> Question* { return new Q95(); },
    []() -> Question* { return new Q96(); },
    []() -> Question* { return new Q97(); },
    []() -> Question* { return new Q98(); },
    []() -> Question* { return new Q99(); },
    []() -> Question* { return new Q100(); },
    []() -> Question* { return new Q101(); },
    []() -> Question* { return new Q102(); },
    []() -> Question* { return new Q103(); },
    []() -> Question* { return new Q104(); },
    []() -> Question* { return new Q105(); },
    []() -> Question* { return new Q106(); },
    []() -> Question* { return new Q107(); },
    []() -> Question* { return new Q108(); },
};


string init_question(int id)
{
    vector<Player> players;
    Player tempP;
    for (int i = 0; i < 10; i++) {
        players.push_back(tempP);
    }
    Question* question = create_question[id - 1]();
    question -> init(players);
    question -> initTexts(players);
    question -> initOptions();
    thread_local static string str;
    str = "";
    str += "题号：#" + to_string(question -> id) + "\n";
    str += "出题者：" + (question -> author) + "\n";
    if (id > k_question_num) {
        str += "[测试] ";
    }
    str += "题目：" + (question -> title) + "\n\n";
    str += question -> String();
    delete question;
    return str;
}

char* find_question(const string& keywords)
{
    thread_local static string find_str;
    thread_local static string notfind_str;
    notfind_str = "[错误] 未找到包含“" + keywords + "”的题目";
    bool flag = false;
    for (int i = 1; i <= all_question_num; i++) {
        find_str = init_question(i);
        string::size_type position = find_str.find(keywords);
        if (position != std::string::npos) {
            flag = true;
            break;
        }
    }
    if (flag) {
        return (char*)find_str.c_str();
    } else {
        return (char*)notfind_str.c_str();
    }
}

const std::vector<RuleCommand> k_rule_commands = {
    RuleCommand("根据 关键字或题号 查找题目",
        [](const string& keywords) { return find_question(keywords); },
        AnyArg("关键字 / #题号", "#30")),
};

bool AdaptOptions(MsgSenderBase& reply, MyGameOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() < 4) {
        reply() << "该游戏至少 4 人参加，当前玩家数为 " << generic_options_readonly.PlayerNum();
        return false;
    }
    return true;
}

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [] (MyGameOptions& game_options, MutableGenericOptions& generic_options)
            {
                generic_options.bench_computers_to_player_num_ = 10;
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
        : StageFsm(std::move(utility), MakeStageCommand(*this, "查看当前游戏进展情况", &MainStage::Status_, VoidChecker("赛况"),
            OptionalDefaultChecker<BoolChecker>(true, "图片", "文字")))
        , round_(0)
        , player_scores_(Global().PlayerNum(), 0)
    {
    }

    virtual void FirstStageFsm(SubStageFsmSetter setter) override;
    virtual void NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;

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
         s += "乌合之众: \n第" + str(round_) + " / " + str(GAME_OPTION(回合数)) + " 回合\n\n";
         s += specialRule(players, GAME_OPTION(特殊规则), "gameStart");
         if(GAME_OPTION(特殊规则) == 0) s += "无";
         s += "\n\n当前题目\n";
         s += "题号：" + str(question -> id) + "\n";
         s += "出题者：" + (question -> author) + "\n";
         s += "题目：" + (question -> title) + "\n";
         Global().Boardcast() << s;

         if (show_image) {
            Global().Boardcast() << Markdown(question -> Markdown());
         } else {
            Global().Boardcast() << question -> String();
         }
         
        return StageErrCode::OK;
    }

};


class RoundStage : public SubGameStage<>
{
  public:
    RoundStage(MainStage& main_stage, const uint64_t round)
        : StageFsm(main_stage, "第 " + to_string(round) + " 回合",
                MakeStageCommand(*this, "选择", &RoundStage::Submit_, AnyArg("提交", "提交")))
    {
    }

    virtual void OnStageBegin() override
    {
        Question *& q = Main().question;

        int r = -1;
        int count = 0;
        while(r == -1 || Main().used.find(r) != Main().used.end())
        {
            r = rand() % k_question_num;
            if (GAME_OPTION(测试模式)) {
                r = rand() % (all_question_num - k_question_num) + k_question_num;
            }
            if(count++ > 1000) {
                Main().used.clear();
                break;
            }
        }
        Main().used.insert(r);

        if(GAME_OPTION(测试) != 0)
            r = GAME_OPTION(测试) - 1;
        if(r >= all_question_num){
            Global().Boardcast() << "question not found! RoundStage -> OnStageBegin where r == " + to_string(r);
            r = 0;
        }

        q = create_question[r]();

        if(q == NULL)
        {
            Global().Boardcast() << "Q == NULL in RoundStage -> OnStageBegin where r == " + to_string(r);
            Global().Boardcast() << "发生了不可预料的错误。请中断游戏。";
            Global().StartTimer(GAME_OPTION(时限));
            return;
        }

        q -> init(Main().players);
        q -> initTexts(Main().players);
        q -> initOptions();
        q -> initExpects();

        Global().Boardcast() << Markdown(q -> Markdown());

        Global().StartTimer(GAME_OPTION(时限));
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        // Returning |CHECKOUT| means the current stage will be over.
        
        for(int i=0;i<Global().PlayerNum();i++)
        {
            if(Global().IsReady(i) == false)
            {
                Main().players[i].select = 0;
                if (GAME_OPTION(特殊规则) == 9 && Main().players[i].lastSelect == 0) {
                    Main().players[i].select = 1;
                }
            }
        }
        Global().HookUnreadyPlayers();

        calc();

        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        // Returning |CONTINUE| means the current stage will be continued.
        Main().players[pid].select = 0;

        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        string x = "A";
        Question *& q = Main().question;

        if(q -> expects.size() == 0 || q -> expects[0].length() == 0)
            return SubmitInternal_(pid, reply, x);

        x[0] = q -> expects[0][rand() % (q -> expects[0].length())];
        if(x[0] <= 'z' && x[0] >= 'a') x[0] = x[0] - 'a' + 'A';

        return SubmitInternal_(pid, reply, x);
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        for(int i=0;i<Global().PlayerNum();i++)
        {
            if(Global().IsReady(i) == false)
            {
                Main().players[i].select = 0;
                if (GAME_OPTION(特殊规则) == 9 && Main().players[i].lastSelect == 0) {
                    Main().players[i].select = 1;
                }
            }
        }

        calc();
        
        return StageErrCode::CHECKOUT;
    }

    void calc()
    {
        Question *& q = Main().question;
        vector<Player> & p = Main().players;

        for(int i = 0; i < Global().PlayerNum(); i++)
            p[i].lastSelect = p[i].select;

        for(int i = 0; i < Global().PlayerNum(); i++)
            p[i].realLastScore = p[i].lastScore;
        for(int i = 0; i < Global().PlayerNum(); i++)
            p[i].lastScore = p[i].score;

        q -> initCalc(p);
        q -> calc(p);
        q -> quickScore(p);

        for(int i = 0; i < Global().PlayerNum(); i++)
            std::cout << i << " " << p[i].score << std::endl;

        int specialRule_ = GAME_OPTION(特殊规则);
        specialRule(p, specialRule_, "roundEnd");


        vector<string> & fb = Main().finalBoard;
        string b = "";
        b += "<table style=\"text-align:center\"><tbody>";
        b += "<tr><td><font size=7>　　　　　　　　　　</font></td><td><font size=7>　　</font></td>";
        b += "<td><font size=7>　　　　</font></td><td><font size=7>　　　　</font></td></tr>";
        for(int i = 0; i < Global().PlayerNum(); i++)
        {
            string color;
            if(Main().players[i].lastScore < Main().players[i].score) color = "bgcolor=\"#90EE90\"";
            if(Main().players[i].lastScore == Main().players[i].score) color = "bgcolor=\"#F5DEB3\"";
            if(Main().players[i].lastScore > Main().players[i].score) color = "bgcolor=\"#FFA07A\"";

            b += (string)""
                    + "<tr>"
                    + "<td bgcolor=\"#D2F4F4\"><font size=7>" + strName(Global().PlayerName(i)) + "</font></td>"
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
            Global().Boardcast() << Markdown(b);


//        std::this_thread::sleep_for(std::chrono::seconds(5));


        for(int i = 0; i < Global().PlayerNum(); i++)
        {
            Main().player_scores_[i] = dec2(p[i].score) * 100;
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
        if (Global().IsReady(pid)) {
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
        if(submission[0] - 'A' + 1 > Main().question -> options.size())
        {
            reply() << "[错误] 选项不存在。";
            return StageErrCode::FAILED;
        }

        if(GAME_OPTION(特殊规则) == 9 && Main().players[pid].lastSelect == submission[0] - 'A') {
            reply() << "[错误] 特殊规则限制：本回合您不能选择此选项。";
            return StageErrCode::FAILED;
        }

        return SubmitInternal_(pid, reply, submission);
    }

    AtomReqErrCode SubmitInternal_(const PlayerID pid, MsgSenderBase& reply, string submission)
    {
        Main().players[pid].select = submission[0] - 'A';
        return StageErrCode::READY;
    }
};

void MainStage::FirstStageFsm(SubStageFsmSetter setter)
{
    srand((unsigned int)time(NULL));

    Player tempP;
    for(int i = 0; i < Global().PlayerNum(); i++)
    {
        players.push_back(tempP);
        finalBoard.push_back("");
    }

    for(int i = 0; i < players.size(); i++)
    {
        finalBoard[i] += (string)"<tr>" + "<td bgcolor=\"#D2F4F4\"><font size=7>" + strName(Global().PlayerName(i)) + "</font></td>";
    }

    for(int i = 0; i < Global().PlayerNum(); i++)
    {
        players[i].lastSelect = -1;
    }

    int specialRule_ = GAME_OPTION(特殊规则);
    if(specialRule_ != 0)
    {
        Global().Boardcast() << specialRule(players, specialRule_, "gameStart");
    }

    if(GAME_OPTION(测试模式)) {
        Global().Boardcast() << "[警告] 正在进行新题测试！如出现异常，请联系管理员中断游戏。";
    }

    setter.Emplace<RoundStage>(*this, ++round_);
}

void MainStage::NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    if ((++round_) <= GAME_OPTION(回合数)) {
        setter.Emplace<RoundStage>(*this, round_);
        return;
    }


    // OK game ends here

    int specialRule_ = GAME_OPTION(特殊规则);
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
    Global().Boardcast() << Markdown(fb);
    for(int i = 0; i < Global().PlayerNum(); i++)
    {
        player_scores_[i] = dec2(players[i].score) * 100;
    }
}

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

