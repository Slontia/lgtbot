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

#include "game_framework/game_main.h"
#include "game_framework/game_stage.h"
#include "game_framework/game_options.h"
#include "utility/msg_checker.h"
#include "utility/html.h"

using namespace std;



const string k_game_name = "wordle"; // the game name which should be unique among all the games
const uint64_t k_max_player = 2; // 0 indicates no max-player limits
const uint64_t k_multiple = 0; // the default score multiple for the game, 0 for a testing game, 1 for a formal game, 2 or 3 for a long formal game

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


string GameOption::StatusInfo() const
{
    return "共 " + to_string(GET_VALUE(回合数)) + " 回合，每回合超时时间 " + to_string(GET_VALUE(时限)) + " 秒";
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
        , player_word_(option.PlayerNum(), "")
        , player_now_(option.PlayerNum(), "")
        , player_used_(option.PlayerNum(), "")
    {
    }

    virtual VariantSubStage OnStageBegin() override;
    virtual VariantSubStage NextSubStage(RoundStage& sub_stage, const CheckoutReason reason) override;

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
    set<string> wordList[10];

    // check if game ends.
    bool gameEnd;

    // the word length
    int wordLength;

    // UI
    string UI;


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
        Boardcast() << g1 << " " << g2 ;
        Boardcast() << cmpWordle(s2, g1) << " " << cmpWordle(s1, g2);


        return StageErrCode::OK;
    }

    int round_;

};


class RoundStage : public SubGameStage<>
{
  public:
    RoundStage(MainStage& main_stage, const uint64_t round)
        : GameStage(main_stage, "第 " + to_string(round) + " 回合",
                MakeStageCommand("猜测", &RoundStage::Submit_, AnyArg("提交", "提交")))
    {
    }

    virtual void OnStageBegin() override
    {
        StartTimer(option().GET_VALUE(时限));
//        Boardcast() << name() << "开始";
    }

    virtual CheckoutErrCode OnTimeout() override
    {
//        Boardcast() << name() << "超时结束";
        // Returning |CHECKOUT| means the current stage will be over.

        for(int i=0;i<option().PlayerNum();i++)
        {
//             Boardcast()<<to_string(IsReady(i))<<to_string(main_stage().player_eli_[i]);
             if(IsReady(i) == false)
             {
                 // Here is for one who committed nothing
                 main_stage().player_now_[i]="";
                 for(int j = 0; j < main_stage().wordLength; j++)
                 {
                     main_stage().player_now_[i]+=" ";
                 }
             }
        }

        RoundStage::calc();

        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
//        Boardcast() << PlayerName(pid) << "退出游戏";
        // Returning |CONTINUE| means the current stage will be continued.



        // Here is for one who committed nothing
        main_stage().player_now_[pid]=" ";
        for(int j = 0; j < main_stage().wordLength; j++)
        {
            main_stage().player_now_[pid]+=" ";
        }

        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        string s="";
        for(int j = 0; j < main_stage().wordLength; j++)
        {
            s+=" ";
        }
        return SubmitInternal_(pid, reply, s);
    }

    virtual void OnAllPlayerReady() override
    {
//        Boardcast() << "所有玩家准备完成";

        RoundStage::calc();
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
        char x = main_stage().player_used_[player][now - 'a'];
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
        int l = main_stage().wordLength;
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
                    main_stage().player_used_[0][g1[i] - 'a'] = '-';


                if(r1[i] == '2')
                    main_stage().player_used_[0][g1[i] - 'a'] = '2';
                if(r1[i] == '1' && main_stage().player_used_[0][g1[i] - 'a'] != '2')
                    main_stage().player_used_[0][g1[i] - 'a'] = '1';
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
                    main_stage().player_used_[1][g2[i] - 'a'] = '-';

                if(r2[i] == '2')
                    main_stage().player_used_[1][g2[i] - 'a'] = '2';
                if(r2[i] == '1' && main_stage().player_used_[1][g2[i] - 'a'] != '2')
                    main_stage().player_used_[1][g2[i] - 'a'] = '1';
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

        string now = main_stage().UI;

        now += "<th bgcolor=\"" + color + "\">";

        if(x <= 'z' && x >= 'a')
            x = x - 'a' + 'A';



        now += "<font size=6>";
        now += x;
        now += "</font>";

        now += "</th>";


        main_stage().UI = now;
    }

    // calc() is where the code calculate results after players made their choice.
    void calc()
    {
        // s->string g->guess r->result
        string s1, s2, g1, g2, r1, r2;
        int l = main_stage().wordLength;
        s1 = main_stage().player_word_[0];
        s2 = main_stage().player_word_[1];
        g1 = main_stage().player_now_[0];
        g2 = main_stage().player_now_[1];
        r1 = cmpWordle(s2, g1);
        r2 = cmpWordle(s1, g2);

        main_stage().UI += "<tr>";

        main_stage().UI += "<td bgcolor=\"#FFFFFF\"><font size=7>　</font></td>";

        for(int i = 0; i < l; i++)
        {
            AddUI(g1[i],r1[i]);
        }
        AddUI(' ','3');
        for(int i = 0; i < l; i++)
        {
            AddUI(g2[i],r2[i]);
        }

        main_stage().UI += "<td bgcolor=\"#FFFFFF\"><font size=7>　</font></td>";

        main_stage().UI += "</tr>";

        string keyboardUI = AddKeyboard(s1, s2, g1, g2, r1, r2);

        // Boardcast the result. Note that the table need an end mark </table>
        Boardcast() << Markdown(main_stage().UI + keyboardUI);

        if(s2 == g1)
        {
            main_stage().player_scores_[0] = 1;
            main_stage().gameEnd = 1;
        }
        if(s1 == g2)
        {
            main_stage().player_scores_[1] = 1;
            main_stage().gameEnd = 1;
        }


    }

  private:
    AtomReqErrCode Submit_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, string submission)
    {
        if (IsReady(pid)) {
            reply() << "[错误] 您本回合已经完成提交。";
            return StageErrCode::FAILED;
        }


        int l = main_stage().wordLength;

        for(int i = 0; i < l; i++)
        {
            if(submission[i] <= 'Z' && submission[i] >= 'A')
            {
                submission[i] = submission[i] - 'A' + 'a';
            }
        }

        // When single game, the game can end whenever the player want
        // These codes were temporary annotated because I can't find a way to check whether it is a single game.
//        if(submission == "end" && option().PlayerNum() == 1)
//        {
//            main_stage().gameEnd = 1;

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


        if(main_stage().wordList[l].find(submission) == main_stage().wordList[l].end()) {
            reply() << "[错误] 这不是一个有效的单词。";
            return StageErrCode::FAILED;
        }



        return SubmitInternal_(pid, reply, submission);
    }

    AtomReqErrCode SubmitInternal_(const PlayerID pid, MsgSenderBase& reply, const string submission)
    {
//        auto& player_score = main_stage().player_scores_[pid];
//        player_score += score;

        main_stage().player_now_[pid] = submission;

        reply() << "提交成功。";
        // Returning |READY| means the player is ready. The current stage will be over when all surviving players are ready.
        return StageErrCode::READY;
    }

};

MainStage::VariantSubStage MainStage::OnStageBegin()
{

    // Most init steps are in this function.

    // 0. init vars in case unexpected seg error happens.
    wordLength = 5;
    player_used_[0] = "00000000000000000000000000+";
    player_used_[1] = "00000000000000000000000000+";

    // 1. Read all given words.
    FILE *fp=fopen((string(option().ResourceDir())+("words.txt")).c_str(),"r");
    if(fp == NULL)
    {
        Boardcast() << "[错误] 单词列表不存在。(W)";
        gameEnd = 1;
        return make_unique<RoundStage>(*this, ++round_);
    }

    int hard = option().GET_VALUE(高难);
    if(hard == 1)
    {
        fclose(fp);
        fp = fopen((string(option().ResourceDir())+("wordsGuess.txt")).c_str(),"r");
        if(fp == NULL)
        {
            Boardcast() << "[错误] 单词列表不存在。(GH)";
            gameEnd = 1;
            return make_unique<RoundStage>(*this, ++round_);
        }
    }

    char word[50];
    while(fscanf(fp, "%s", word) != EOF)
    {
        string addS = "";
        int len = strlen(word);

        if(len < 5 || len > 8) continue;

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

        // len : 5 -> 8

        wordLength = option().GET_VALUE(长度);


        if(wordLength == 0)
        {
            int r = rand() % 100;
            if(r <= -1);
            else if(r <= 39) wordLength = 5;
            else if(r <= 56) wordLength = 6;
            else if(r <= 82) wordLength = 7;
            else if(r <= 100) wordLength = 8;
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

    UI = "<table style=\"text-align:center\"><tbody>";
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
        fp = fopen((string(option().ResourceDir())+("wordsGuess.txt")).c_str(),"r");
        if(fp == NULL)
        {
            Boardcast() << "[错误] 单词列表不存在。(G)";
            gameEnd = 1;
            return make_unique<RoundStage>(*this, ++round_);
        }

    //    char word[50];
        while(fscanf(fp, "%s", word) != EOF)
        {
            string addS = "";
            int len = strlen(word);

            if(len < 4 || len > 8) continue;

            for(int i = 0; i < len; i++)
            {
                addS += word[i];
            }

            wordList[len].insert(addS);
        }
        fclose(fp);
    }



    // 5. Boardcast the game start
    Boardcast() << "本局游戏参与玩家： \n" << PlayerName(0) << "\n" << PlayerName(1);
    Boardcast() << "本局游戏双方单词长度： " + to_string(wordLength);

    Tell(0) << "你的单词是：" << player_word_[0];
    Tell(1) << "你的单词是：" << player_word_[1];

    return make_unique<RoundStage>(*this, ++round_);
}

MainStage::VariantSubStage MainStage::NextSubStage(RoundStage& sub_stage, const CheckoutReason reason)
{
    round_++;

//    Boardcast()<<to_string(round_)<<to_string(player_wins_[0])<<to_string(player_wins_[1]);

    if (round_ < option().GET_VALUE(回合数) && !gameEnd ) {
        return make_unique<RoundStage>(*this, round_);
    }

    Boardcast() << "本局游戏双方单词: \n"
                << PlayerName(0) << ":" << player_word_[0] << "\n"
                << PlayerName(1) << ":" << player_word_[1];

    return {};
}

MainStageBase* MakeMainStage(MsgSenderBase& reply, GameOption& options, MatchBase& match)
{
    if (!options.ToValid(reply)) {
        return nullptr;
    }
    return new MainStage(options, match);
}
