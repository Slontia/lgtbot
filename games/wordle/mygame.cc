// Copyright (c) 2018-present, ZhengYang Xu <github.com/jeffxzy>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <array>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <stdlib.h>
#include <cstring>

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"

using namespace std;

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;

const GameProperties k_properties {
    .name_ = "wordle", // the game name which should be unique among all the games
    .developer_ = "睦月",
    .description_ = "猜测英文单词的游戏",
};

uint64_t MaxPlayerNum(const CustomOptions& options) { return 2; } // 0 indicates no max-player limits
uint32_t Multiple(const CustomOptions& options) { return 1; } // the default score multiple for the game, 0 for a testing game, 1 for a formal game, 2 or 3 for a long formal game
const MutableGenericOptions k_default_generic_options{
    .is_formal_{false},
};
const std::vector<RuleCommand> k_rule_commands = {};

// Give it 2 strings, returns how many letters are the same.
int cmpString(string a,string b)
{
    if(a.length() != b.length()) return -1;

    int n = a.length();
    int ret = 0;
    for(int i = 0; i < n; i++)
    {
        if(a[i] == b[i])
            ret++;
    }
    return ret;
}

// Give it two srtrings, it returns you the wordle result. E.g. abcd and acbe returns 2110.
string cmpWordle(string a,string b)
{

    int n = a.length();
    string ret = "";
    for(int i = 0; i < n; i++) ret += "0";


    if(a.length() != b.length()) return ret;

    multiset<char> s;
    s.clear();

    for(int i = 0; i < n; i++)
    {
        if(a[i] == b[i])
        {
            ret[i] = '2';
            b[i] = ' ';
        }
        else
        {
            s.insert(a[i]);
        }
    }

    for(int i = 0; i < n; i++)
    {
        if(ret[i] == '2') continue;

        if(s.find(b[i]) != s.end())
        {
            ret[i] = '1';
            s.erase(s.find(b[i]));
        }
        else
        {
            ret[i] = '0';
        }
    }

    return ret;
}


bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() != 2) {
        reply() << "该游戏必须 2 人参加，当前玩家数为 " << generic_options_readonly.PlayerNum();
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

class RoundStage;

class MainStage : public MainGameStage<RoundStage>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility), MakeStageCommand(*this, "查看当前游戏进展情况", &MainStage::Status_, VoidChecker("赛况")))
        , round_(0)
        , player_scores_(Global().PlayerNum(), 0)
        , player_word_(Global().PlayerNum(), "")
        , player_now_(Global().PlayerNum(), "")
        , player_used_(Global().PlayerNum(), "")
    {
    }

    virtual void FirstStageFsm(SubStageFsmSetter setter) override;
    virtual void NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }

    // The standard score
    vector<int64_t> player_scores_;

    // player word
    vector<string> player_word_;

    // player guess
    vector<string> player_now_;

    // player used
    vector<string> player_used_;


    /*
     * The initial of (All words), (gameEnd) is in function -> OnStageBegin()
     */

    // All words
    set<string> wordList[15];

    // check if game ends.
    bool gameEnd;

    // the word length
    int wordLength;

    // UI
    string UI;

    // game round
    int round_;


  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
//        reply() << "这里输出当前游戏情况";
        // Returning |OK| means the game stage

        string s1, s2, g1, g2;
        s1 = player_word_[0];
        s2 = player_word_[1];
        g1 = player_now_[0];
        g2 = player_now_[1];
        Global().Boardcast() << g1 << " " << g2 ;
        Global().Boardcast() << cmpWordle(s2, g1) << " " << cmpWordle(s1, g2);


        return StageErrCode::OK;
    }


};


class RoundStage : public SubGameStage<>
{
  public:
    RoundStage(MainStage& main_stage, const uint64_t round)
        : StageFsm(main_stage, "第 " + to_string(round) + " 回合",
                MakeStageCommand(*this, "猜测", &RoundStage::Submit_, AnyArg("提交", "提交")))
    {
    }

    virtual void OnStageBegin() override
    {
        Global().StartTimer(GAME_OPTION(时限));
//        Global().Boardcast() << Name() << "开始";
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
//        Global().Boardcast() << Name() << "超时结束";
        // Returning |CHECKOUT| means the current stage will be over.

        for(int i=0;i<Global().PlayerNum();i++)
        {
//             Global().Boardcast()<<to_string(Global().IsReady(i))<<to_string(Main().player_eli_[i]);
             if(Global().IsReady(i) == false)
             {
                 // Here is for one who committed nothing
                 Main().player_now_[i]="";
                 for(int j = 0; j < Main().wordLength; j++)
                 {
                     Main().player_now_[i]+=" ";
                 }
             }
        }

        RoundStage::calc();

        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
//        Global().Boardcast() << Global().PlayerName(pid) << "退出游戏";
        // Returning |CONTINUE| means the current stage will be continued.



        // Here is for one who committed nothing
        Main().player_now_[pid]=" ";
        for(int j = 0; j < Main().wordLength; j++)
        {
            Main().player_now_[pid]+=" ";
        }

        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        string s = "";
        int l = Main().wordLength;
        int r = Main().round_;
        string a = Main().player_word_[0];

        for(int i = 0; i < l; i++) s += ' ';

        if(l == 2)
        {
            if(r == 1) s = "nm";
            if(r == 2) s = "as";
            if(r == 3) s = "er";
            if(r == 4) s = "ok";
            if(r == 5) s = "th";
            if(r == 6) s = "li";
            if(r == 7) s = "fg";
            if(r == 8) s = "wy";
            if(r == 9) s = "pu";
            if(r == 10) s = a;
        }
        if(l == 3)
        {
            if(r == 1) s = "one";
            if(r == 2) s = "car";
            if(r == 3) s = "bug";
            if(r == 4) s = "mod";
            if(r == 5) s = "tax";
            if(r == 6) s = "why";
            if(r == 7) s = "zip";
            if(r == 8) s = a;
        }
        if(l == 4)
        {
            if(r == 1) s = "dust";
            if(r == 2) s = "grow";
            if(r == 3) s = "evil";
            if(r == 4) s = "back";
            if(r == 5) s = "hymn";
            if(r == 6) s = a;
        }
        if(l == 5)
        {
            if(r == 1) s = "crane";
            if(r == 2) s = "pilot";
            if(r == 3) s = "dusky";
            if(r == 4) s = a;
        }
        if(l == 6)
        {
            if(r == 1) s = "sniper";
            if(r == 2) s = "fourth";
            if(r == 3) s = "cymbal";
            if(r == 4) s = "adzuki";
            if(r == 5) s = a;
        }
        if(l == 7)
        {
            if(r == 1) s = "keyword";
            if(r == 2) s = "implant";
            if(r == 3) s = "cloughs";
            if(r == 4) s = "farebox";
            if(r == 5) s = "aquiver";
            if(r == 6) s = a;
        }
        if(l == 8)
        {
            if(r == 1) s = "equation";
            if(r == 2) s = "abruptly";
            if(r == 3) s = "biconvex";
            if(r == 4) s = "goldfish";
            if(r == 5) s = "waymarks";
            if(r == 6) s = "jarovize";
            if(r == 7) s = a;
        }
        if(l == 9)
        {
            if(r == 1) s = "excellent";
            if(r == 2) s = "colourful";
            if(r == 3) s = "abolished";
            if(r == 4) s = "overtrump";
            if(r == 5) s = "quickwork";
            if(r == 6) s = "jargonize";
            if(r == 7) s = "keybutton";
            if(r == 8) s = a;
        }
        if(l == 10)
        {
            if(r == 1) s = "philosophy";
            if(r == 2) s = "adjudgment";
            if(r == 3) s = "workaholic";
            if(r == 4) s = "verifiable";
            if(r == 5) s = "equestrian";
            if(r == 6) s = "execration";
            if(r == 7) s = "zoological";
            if(r == 8) s = a;
        }
        if(l == 11)
        {
            if(r == 1) s = "abstraction";
            if(r == 2) s = "dependingly";
            if(r == 3) s = "chairmaking";
            if(r == 4) s = "injunctions";
            if(r == 5) s = "quoteworthy";
            if(r == 6) s = "effectively";
            if(r == 7) s = "externalize";
            if(r == 8) s = a;
        }


        return SubmitInternal_(pid, reply, s);
    }

    virtual CheckoutErrCode OnStageOver() override
    {
//        Global().Boardcast() << "所有玩家准备完成";

        RoundStage::calc();
        return StageErrCode::CHECKOUT;
    }


    string AddKeyLetter(int player, char now)
    {
        if(player == -1)
        {
            string ret = "";
            string A;
            if(now == 'X') A = "　";
            ret += "<th bgcolor=\"#FFFFFF\"><font size=5 color=\"#FFFFFF\">" + A + "</font></th>";
            return ret;
        }

        string color;
        char x = Main().player_used_[player][now - 'a'];
        if(x == '0') color = "#EEEEEE";
        if(x == '-') color = "#F8F8F8";
        if(x == '2') color = "#ADFF2F";
        if(x == '1') color = "#FAFA7D";

        string ret = "";
        ret += "<th bgcolor=\"" + color + "\">";

        ret += "<font size=5>";
        if(x != '-')
            ret += now - 'a' + 'A';
        ret += "</font>";

        ret += "</th>";
        return ret;
    }

    // draw keyboard
    string AddKeyboard(string s1, string s2, string g1, string g2, string r1, string r2)
    {
        int l = Main().wordLength;
        for(int i = 0; i < l; i++)
        {
            if(g1[i] <= 'z' && g1[i] >= 'a')
            {
                int app = 0;
                for(int j = 0; j < l; j++)
                {
                    if(g1[i] == s2[j])
                    {
                        app = 1;
                    }
                }
                if(app == 0)
                    Main().player_used_[0][g1[i] - 'a'] = '-';


                if(r1[i] == '2')
                    Main().player_used_[0][g1[i] - 'a'] = '2';
                if(r1[i] == '1' && Main().player_used_[0][g1[i] - 'a'] != '2')
                    Main().player_used_[0][g1[i] - 'a'] = '1';
            }

            if(g2[i] <= 'z' && g2[i] >= 'a')
            {
                int app = 0;
                for(int j = 0; j < l; j++)
                {
                    if(g2[i] == s1[j])
                    {
                        app = 1;
                    }
                }
                if(app == 0)
                    Main().player_used_[1][g2[i] - 'a'] = '-';

                if(r2[i] == '2')
                    Main().player_used_[1][g2[i] - 'a'] = '2';
                if(r2[i] == '1' && Main().player_used_[1][g2[i] - 'a'] != '2')
                    Main().player_used_[1][g2[i] - 'a'] = '1';
            }
        }

        string k1,k2,k3; int l1,l2,l3;
        k1 = "qwertyuiop"; l1=k1.length();
        k2 = "asdfghjkl"; l2=k2.length();
        k3 = "zxcvbnm"; l3=k3.length();

        string ret =" ";

        ret += "</table>";

        ret += "<br><br>";

        ret += "<table style=\"text-align:center;margin:auto;\"><tbody><tr>";
        for(int i = 0; i < l1 + 4 + l1; i++) ret += AddKeyLetter(-1, 'X'); ret += "</tr><tr>";
        for(int i = 0; i < l1; i++) ret += AddKeyLetter(0, k1[i]);
        for(int i = 0; i < 4; i++) ret += AddKeyLetter(-1, 'X');
        for(int i = 0; i < l1; i++) ret += AddKeyLetter(1, k1[i]);
        ret += "</tr></table>";

        ret += "<table style=\"text-align:center;margin:auto;\"><tbody><tr>";
        for(int i = 0; i < l2 + 5 + l2; i++) ret += AddKeyLetter(-1, 'X'); ret += "</tr><tr>";
        for(int i = 0; i < l2; i++) ret += AddKeyLetter(0, k2[i]);
        for(int i = 0; i < 5; i++) ret += AddKeyLetter(-1, 'X');
        for(int i = 0; i < l2; i++) ret += AddKeyLetter(1, k2[i]);
        ret += "</tr></table>";

        ret += "<table style=\"text-align:center;margin:auto;\"><tbody><tr>";
        for(int i = 0; i < l3 + 7 + l3; i++) ret += AddKeyLetter(-1, 'X'); ret += "</tr><tr>";
        for(int i = 0; i < l3; i++) ret += AddKeyLetter(0, k3[i]);
        for(int i = 0; i < 7; i++) ret += AddKeyLetter(-1, 'X');
        for(int i = 0; i < l3; i++) ret += AddKeyLetter(1, k3[i]);
        ret += "</tr></table>";

        return ret;
    }

    // Add a letter to UI.
    void AddUI(char x, char type)
    {
        string color;
        if(type == '0') color="#F5FFFA"; // White
        if(type == '1') color="#FAFA7D"; // Yellow
        if(type == '2') color="#ADFF2F"; // Green
        if(type == '3') color="#BBBBBB"; // Grey

        string now = Main().UI;

        now += "<th bgcolor=\"" + color + "\">";

        if(x <= 'z' && x >= 'a')
            x = x - 'a' + 'A';



        now += "<font size=6>";
        now += x;
        now += "</font>";

        now += "</th>";


        Main().UI = now;
    }

    // calc() is where the code calculate results after players made their choice.
    void calc()
    {
        // s->string g->guess r->result
        string s1, s2, g1, g2, r1, r2;
        int l = Main().wordLength;
        s1 = Main().player_word_[0];
        s2 = Main().player_word_[1];
        g1 = Main().player_now_[0];
        g2 = Main().player_now_[1];
        r1 = cmpWordle(s2, g1);
        r2 = cmpWordle(s1, g2);

        Main().UI += "<tr>";

        Main().UI += "<td bgcolor=\"#FFFFFF\"><font size=7>　</font></td>";

        for(int i = 0; i < l; i++)
        {
            AddUI(g1[i],r1[i]);
        }
        AddUI(' ','3');
        for(int i = 0; i < l; i++)
        {
            AddUI(g2[i],r2[i]);
        }

        Main().UI += "<td bgcolor=\"#FFFFFF\"><font size=7>　</font></td>";

        Main().UI += "</tr>";

        string keyboardUI = AddKeyboard(s1, s2, g1, g2, r1, r2);

        // Global().Boardcast the result. Note that the table need an end mark </table>
        Global().Boardcast() << Markdown(Main().UI + keyboardUI);

        if(s2 == g1)
        {
            Main().player_scores_[0] = 1;
            Main().gameEnd = 1;
        }
        if(s1 == g2)
        {
            Main().player_scores_[1] = 1;
            Main().gameEnd = 1;
        }


    }

  private:
    AtomReqErrCode Submit_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, string submission)
    {
        if (Global().IsReady(pid)) {
            reply() << "[错误] 您本回合已经完成提交。";
            return StageErrCode::FAILED;
        }


        int l = Main().wordLength;

        for(int i = 0; i < l; i++)
        {
            if(submission[i] <= 'Z' && submission[i] >= 'A')
            {
                submission[i] = submission[i] - 'A' + 'a';
            }
        }

        // When single game, the game can end whenever the player want
        // These codes were temporary annotated because I can't find a way to check whether it is a single game.
//        if(submission == "end" && Global().PlayerNum() == 1)
//        {
//            Main().gameEnd = 1;

//            string temp="";
//            for(int i = 0; i < l; i++)
//            {
//                temp += " ";
//            }
//            return SubmitInternal_(pid, reply, temp);
//        }


        if(l != submission.length()) {
            reply() << "[错误] 单词长度不正确。";
            return StageErrCode::FAILED;
        }


        if(Main().wordList[l].find(submission) == Main().wordList[l].end()) {
            reply() << "[错误] 这不是一个有效的单词。";
            return StageErrCode::FAILED;
        }



        return SubmitInternal_(pid, reply, submission);
    }

    AtomReqErrCode SubmitInternal_(const PlayerID pid, MsgSenderBase& reply, const string submission)
    {
//        auto& player_score = Main().player_scores_[pid];
//        player_score += score;

        Main().player_now_[pid] = submission;

        reply() << "提交成功。";
        // Returning |READY| means the player is ready. The current stage will be over when all surviving players are ready.
        return StageErrCode::READY;
    }

};

void MainStage::FirstStageFsm(SubStageFsmSetter setter)
{

    // Most init steps are in this function.

    // 0. init vars in case unexpected seg error happens.
    wordLength = 5;
    player_used_[0] = "00000000000000000000000000+";
    player_used_[1] = "00000000000000000000000000+";

    // 1. Read all given words.
    FILE *fp=fopen((string(Global().ResourceDir())+("words.txt")).c_str(),"r");
    if(fp == NULL)
    {
        Global().Boardcast() << "[错误] 单词列表不存在。(W)";
        gameEnd = 1;
        setter.Emplace<RoundStage>(*this, ++round_);
        return;
    }

    int hard = GAME_OPTION(高难);
    int mode = GAME_OPTION(随机);
    if(hard == 1)
    {
        fclose(fp);
        fp = fopen((string(Global().ResourceDir())+("wordsGuess.txt")).c_str(),"r");
        if(fp == NULL)
        {
            Global().Boardcast() << "[错误] 单词列表不存在。(G)";
            gameEnd = 1;
            setter.Emplace<RoundStage>(*this, ++round_);
            return;
        }
    }

    if(hard == 2)
    {
        fclose(fp);
        fp = fopen((string(Global().ResourceDir())+("wordsHard.txt")).c_str(),"r");
        if(fp == NULL)
        {
            Global().Boardcast() << "[错误] 单词列表不存在。(H)";
            gameEnd = 1;
            setter.Emplace<RoundStage>(*this, ++round_);
            return;
        }
    }

    char word[50];
    while(fscanf(fp, "%s", word) != EOF)
    {
        string addS = "";
        int len = strlen(word);

        if(len < 2 || len > 11) continue;

        for(int i = 0; i < len; i++)
        {
            addS += word[i];
        }

        wordList[len].insert(addS);
    }
    fclose(fp);



    // 2. Choose random words for players
    srand((unsigned int)time(NULL));
    int fin = 1;
    while(fin != 0 && fin < 100)
    {

        wordLength = GAME_OPTION(长度);

        if((wordLength >= 2 && wordLength <= 4 || wordLength >= 9 && wordLength <= 11) && hard == 1)
        {
            Global().Boardcast() << "[警告] 配置「高难 1」不支持此单词长度，将自动在5-8中随机单词长度";
            wordLength = 0;
        }

        if(wordLength == 0)
        {
            int r = rand() % 100 + 1;
            if(hard == 2 || mode == 2)
            {
                if(r <= -1);
                else if(r <= 5) wordLength = 2;
                else if(r <= 13) wordLength = 3;
                else if(r <= 25) wordLength = 4;
                else if(r <= 55) wordLength = 5;
                else if(r <= 70) wordLength = 6;
                else if(r <= 80) wordLength = 7;
                else if(r <= 88) wordLength = 8;
                else if(r <= 93) wordLength = 9;
                else if(r <= 97) wordLength = 10;
                else if(r <= 100) wordLength = 11;
            }
            else
            {
                if(r <= -1);
                else if(r <= 40) wordLength = 5;
                else if(r <= 57) wordLength = 6;
                else if(r <= 83) wordLength = 7;
                else if(r <= 100) wordLength = 8;
            }
        }

        // l=length
        int l = wordLength;

        string s1,s2;
        int r1,r2,n2;


        if(wordList[l].size()==0)
            continue;

        r1=rand()%wordList[l].size();

        // random select a word or player 0
        for(auto v:wordList[l])
        {
            if(r1==0)
            {
                s1=v;
                break;
            }
            r1--;
        }

        n2 = 0;
        for(auto v:wordList[l])
        {
            int same=cmpString(v,s1);
            if(same != -1 && same != 0 && same != l)
            {
                n2++;
            }
        }

        // cannot find enough situable word for s1
        if(n2 <= 10)
        {
            fin++;
            continue;
        }

        // find a correct s2 for s1
        r2 = rand()%n2;
        r2++;
        for(auto v:wordList[l])
        {
            int same=cmpString(v,s1);
            if(same > 0 && same != l)
            {
                r2--;
                if(r2 == 0)
                {
                    s2 = v;
                    break;
                }
            }
        }

        if(rand() % 2 == 0)
        {
            string temp;
            temp = s1;
            s1 = s2;
            s2 = temp;
        }

        player_word_[0] = s1;
        player_word_[1] = s2;
        fin = 0;
    }

    // 3. init UI

    UI = "<table style=\"text-align:center;margin:0 auto\"><tbody>";
    UI += "<tr>";
    UI += "<td bgcolor=\"#FFFFFF\"><font size=7>　</font></td>";
    for(int i = 0; i < wordLength; i++)
    {
        UI += "<td bgcolor=\"#D2F4F4\"><font size=7>　</font></td>";
    }
    UI += "<td bgcolor=\"#BBBBBB\"><font size=7>　　</font></td>";
    for(int i = 0; i < wordLength; i++)
    {
        UI += "<td bgcolor=\"#D2F4F4\"><font size=7>　</font></td>";
    }
    UI += "<td bgcolor=\"#FFFFFF\"><font size=7>　</font></td>";
    UI += "</tr>";



    // 4. init variables
    gameEnd = 0;



    // 5. extend wordlist
    if(hard == 0)
    {
        if(mode == 1)
        {
            fp = fopen((string(Global().ResourceDir())+("wordsGuess.txt")).c_str(),"r");
            if(fp == NULL)
            {
                Global().Boardcast() << "[错误] 单词列表不存在。(G)";
                gameEnd = 1;
                setter.Emplace<RoundStage>(*this, ++round_);
                return;
            }
        }

        if(mode == 2 || (wordLength >= 2 && wordLength <= 4 || wordLength >= 9 && wordLength <= 11))
        {
            fp = fopen((string(Global().ResourceDir())+("wordsHard.txt")).c_str(),"r");
            if(fp == NULL)
            {
                Global().Boardcast() << "[错误] 单词列表不存在。(H)";
                gameEnd = 1;
                setter.Emplace<RoundStage>(*this, ++round_);
                return;
            }
        }

    //    char word[50];
        while(fscanf(fp, "%s", word) != EOF)
        {
            string addS = "";
            int len = strlen(word);

            if(len < 2 || len > 11) continue;

            for(int i = 0; i < len; i++)
            {
                addS += word[i];
            }

            wordList[len].insert(addS);
        }
        fclose(fp);
    }



    // 5. Global().Boardcast the game start
    Global().Boardcast() << "本局游戏参与玩家： \n" << Global().PlayerName(0) << "\n" << Global().PlayerName(1);
    Global().Boardcast() << "本局游戏双方单词长度： " + to_string(wordLength) + (wordLength <= 4? "\n本局不告知双方单词":"");

    if(player_word_[0] == "" || player_word_[1] == "")
    {
        Global().Boardcast() << "[错误] 获取双方玩家单词失败，游戏将自动结束。";
        gameEnd = 1;
        setter.Emplace<RoundStage>(*this, ++round_);
        return;
    }

    if(wordLength > 4){
        Global().Tell(0) << "你的单词是：" << player_word_[0];
        Global().Tell(1) << "你的单词是：" << player_word_[1];
    }

    setter.Emplace<RoundStage>(*this, ++round_);
    return;
}

void MainStage::NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    round_++;

//    Global().Boardcast()<<to_string(round_)<<to_string(player_wins_[0])<<to_string(player_wins_[1]);

    if (round_ <= GAME_OPTION(回合数) && !gameEnd ) {
        setter.Emplace<RoundStage>(*this, round_);
        return;
    }

    Global().Boardcast() << "本局游戏双方单词: \n"
                << Global().PlayerName(0) << ":" << player_word_[0] << "\n"
                << Global().PlayerName(1) << ":" << player_word_[1];

}

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

