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

/*
 *
 * The question is stored in /resources/questions.txt, with format like:
 * #
 * Select an option! ...
 * #
 * Option 1
 * Option 2
 * Option 3
 * ...
 * #
 * Code 1
 * Code 2
 * Code 3
 * ...
 * #
 *
 *
 * And the code also has strict format, like:
 *
 * # make X a var, init with 0
 * var X
 *
 *
 * # give X the value Y.
 * X = Y
 *
 *
 * # let a = b+c
 * a = b c
 *
 * # Tips: [ - * / ] are also the same.
 *
 *
 * # make P.1 a var. You can interpret " . " as array symbol. That is, P.1 -> P[1] in C++.
 * var P.1
 *
 *
 * # make P.x a var, please notice that x must be initialized first.
 * var P.x
 *
 *
 * # if a == b , jump to line +3/-3/x
 * jumpif a == b P3
 * jumpif a == b N3
 * jumpif a == b x
 *
 * #Tips, [ != < <= > >= ] are also the same.
 * #Please notice that we use P3 as +3, N3 as -3.
 *
 *
 * # jump to +3
 * jump P3
 * jump 3
 *
 *
 *
 * # if ... then
 * if a < b
 *
 *
 * # if ... then ,else jump x
 * if a < b else x
 *
 *
 *
 * Also, there are some init vars for one to code easier.
 * Score.i -> the score of player[i]
 * Now.i -> the option player[i] chose this turn
 * MaxScoreNum/MinScoreNum -> the highest/lowest score now
 * Select.i -> the number of players who chose option[i]
 * MaxSelectNum/MinSelectNum -> the hightest/lowest number in Selected.i
 *
 *
 *
 *
 * In this way, a question can be easily added without changing the code.
 * Also, online editors my be possible if one would like to add problem by talking to the bot.
 *
 *
 */

using namespace std;


const string k_game_name = "乌合之众"; // the game name which should be unique among all the games
const uint64_t k_max_player = 0; // 0 indicates no max-player limits
const uint64_t k_multiple = 0; // the default score multiple for the game, 0 for a testing game, 1 for a formal game, 2 or 3 for a long formal game


// a RetCode is a status code for program to know what's happening. e.g: status = "ok" / "failed" / "jump"
class RetCode
{
public:
    RetCode(){
        status = "ok";
        number = 0;
    }
    string status;
    int number;
};

// the class of player
class Player
{
public:
    Player(){
        lastscore = 0;
        score = 0;
        nowC = 'A';
        nowN = 0;
        selected = "";
    }
public:

    int lastscore;
    int score;
    char nowC;
    int nowN;
    string selected;
};


// the class of question
class Question
{

public:
    Question(){
        texts.clear();
        options.clear();
        codes.clear();
        md = "";
    }

    vector<string> texts;
    vector<string> options;
    vector<string> codes;

    string md;
    void createMarkdown();
    string getMd();

    RetCode ret;

    // call this function to run the code
    void RunCode(vector<Player> &players, MsgSenderBase& sender);

    // run a string as code
    void RunLine(vector<Player> &players, map<string,int> &var, string code, int line, MsgSenderBase& sender);


    int getValue(string s, map<string,int> &var);
    string getRealVar(MsgSenderBase& sender, int line, string s, map<string,int> &var);

    bool checkLength(MsgSenderBase& sender, int line, string s, int l1, int l2);
    bool checkVar(MsgSenderBase& sender, int line, string s, map<string,int> &var);
    bool checkNotVar(MsgSenderBase& sender, int line, string s, map<string,int> &var);
    bool checkNumber(string s);
    bool checkValue(MsgSenderBase& sender, int line, string s, map<string,int> &var);
    bool checkCompareSymbol(MsgSenderBase& sender, int line, string s, map<string,int> &var);

};


// run the question code to calc the score of every player
void Question::RunCode(vector<Player> &players, MsgSenderBase& sender)
{
    map<string,int> var;
    var.clear();
    int mn = 10000, mx = -10000;
    var["PlayerNum"] = players.size();
    for(int i = 0; i < players.size(); i++)
    {
        var["Score." + to_string(i)] = players[i].score;
        var["Now." + to_string(i)] = players[i].nowN;
        mn = min(mn, players[i].score);
        mx = max(mx, players[i].score);
    }
    var["MinScoreNum"] = mn;
    var["MaxScoreNum"] = mx;
    for(int i = 0; i < options.size(); i++)
    {
        var["Select." + to_string(i)] = 0;
    }
    for(int i = 0; i < players.size();i++)
    {
        var["Select." + to_string(players[i].nowN)]++;
    }
    mn = 10000, mx = -10000;
    for(int i = 0; i < options.size(); i++)
    {
        mn = min(mn, var["Select." + to_string(i)]);
        mx = max(mx, var["Select." + to_string(i)]);
    }
    var["MinSelectNum"] = mn;
    var["MaxSelectNum"] = mx;


    int lastLine = 0;
    int counts = 0;
    for(int i = 0; i < codes.size(); i++)
    {
//        sender() << "Now at Line " + to_string(i + 1);

        counts++;
        if(counts > 50000)
        {
            sender() << "Error on line " + to_string(i) +
                        " : " + "infinity loop on " + to_string(lastLine);
            sender() << " --> | " + codes[lastLine];
            break;
        }

        RunLine(players, var, codes[i], i+1, sender);

        if(ret.status == "failed")
        {
            sender() << " --> | " + codes[i];
        }

        if(ret.status == "jump")
        {
            i += ret.number;

            if(i < 0)
            {
                sender() << "Error on line " + to_string(i) +
                            " : " + "unexpected jump from " + to_string(i - ret.number) + " to " + to_string(i);
                sender() << " --> | " + codes[i - ret.number];
                break;
            }

            i--;
        }
        if(ret.status == "failed") return;
    }

    for(int i = 0; i < players.size(); i++)
    {
        players[i].score = var["Score." + to_string(i)];

    }
    return;
}


// check whether a char is a symbol. this func is only called in readString(string s, int *pos)
bool checkSymbol(char c)
{
    if(c == '=' || c == '!' || c == '>' || c == '<' ||
            c == '+' || c == '-' || c == '*' || c == '/')
    return true;

    return false;
}

// read an element in one line code. this function is only called in readline to get its elements
string readString(string s, int *pos)
{
    int len = s.length();
    string ret = "";

    int i;
    for(i = *pos; i < len; i++)
    {
        if(s[i] != '\n' && s[i] != ' ' && s[i] != ',')
        {
            break;
        }
    }

    if(i == len)
    {
        *pos = i;
        return ret;
    }

    int type = checkSymbol(s[i]);

    for(i = i; i < len; i++)
    {
        if(s[i] == '\n' || s[i] == ' ' || s[i] == ',')
        {
            break;
        }

        if(checkSymbol(s[i]) != type) break;

        ret += s[i];
    }

    *pos = i;

    return ret;
}


// run a string as code
void Question::RunLine(vector<Player> &players, map<string,int> &var, string code, int line, MsgSenderBase& sender)
{
    vector<string> items;
    items.clear();
    int now = 0;
    string item = readString(code, &now);

    while(item != "")
    {
//        sender() << item;
        items.push_back(item);
        item = readString(code, &now);
    }

    ret.status = "ok";
    ret.number = 0;

    if(items.size() == 0)
        return;

    for(int i = 0; i < items.size(); i++)
    {
        items[i] = getRealVar(sender, line, items[i], var);
        if(ret.status == "failed") return;
    }

    int S = items.size();

    // this if is useless
    if(items[0] == "test")
    {


    }
    // [ var x ] to definite a var
    else if(S > 0 && items[0] == "var")
    {
        for(int i = 1; i < items.size(); i++)
        {
            if(checkNumber(items[i]))
            {
                sender() << "Error in line " + to_string(line) +
                            ": " + items[i] + "\" is a number, which cannot be declared as var.";
                return;
            }
            if(!checkNotVar(sender, line, items[i], var)) return;

            var[items[i]] = 0;
        }
    }
    // [ a = b ] to let a queals to b
    else if(S > 1 && S < 4 && items[1] == "=")
    {
        if(!checkLength(sender, line, "+", 3, items.size())) return;
        if(!checkVar(sender, line, items[0], var)) return;
        if(!checkValue(sender, line, items[2], var)) return;

        var[items[0]] = getValue(items[2], var);

    }
    // [ a = b + c ] to let a queals to b plus c.
    else if(S > 3 && items[1] == "=" && items[3] == "+")
    {
        if(!checkLength(sender, line, "+", 5, items.size())) return;
        if(!checkVar(sender, line, items[0], var)) return;
        if(!checkValue(sender, line, items[2], var)) return;
        if(!checkValue(sender, line, items[4], var)) return;
        var[items[0]] = getValue(items[2], var) + getValue(items[4], var);
    }
    else if(S > 3 && items[1] == "=" && items[3] == "-")
    {
        if(!checkLength(sender, line, "+", 5, items.size())) return;
        if(!checkVar(sender, line, items[0], var)) return;
        if(!checkValue(sender, line, items[2], var)) return;
        if(!checkValue(sender, line, items[4], var)) return;
        var[items[0]] = getValue(items[2], var) - getValue(items[4], var);
    }
    else if(S > 3 && items[1] == "=" && items[3] == "*")
    {
        if(!checkLength(sender, line, "+", 5, items.size())) return;
        if(!checkVar(sender, line, items[0], var)) return;
        if(!checkValue(sender, line, items[2], var)) return;
        if(!checkValue(sender, line, items[4], var)) return;
        var[items[0]] = getValue(items[2], var) * getValue(items[4], var);
    }
    else if(S > 3 && items[1] == "=" && items[3] == "/")
    {
        if(!checkLength(sender, line, "+", 5, items.size())) return;
        if(!checkVar(sender, line, items[0], var)) return;
        if(!checkValue(sender, line, items[2], var)) return;
        if(!checkValue(sender, line, items[4], var)) return;
        var[items[0]] = getValue(items[2], var) / getValue(items[4], var);
    }
    // [ a ++ ] to let a = a + 1
    else if(S > 1 && items[1] == "++")
    {
        if(!checkLength(sender, line, "++", 2, items.size())) return;
        if(!checkVar(sender, line, items[0], var)) return;

        var[items[0]] = getValue(items[0], var) + 1;
    }
    else if(S > 1 && items[1] == "--")
    {
        if(!checkLength(sender, line, "--", 2, items.size())) return;
        if(!checkVar(sender, line, items[0], var)) return;

        var[items[0]] = getValue(items[0], var) - 1;
    }
    // [ jumpif x == y P5 ] if x==y, then goto line +5. !=, <,<=,>,>= can also be used. Use N to go back such as N5 -> -5.
    else if(S > 0 && items[0] == "jumpif")
    {
        if(!checkLength(sender, line, "jumpif", 5, items.size())) return;
        if(!checkValue(sender, line, items[1], var)) return;
        if(!checkCompareSymbol(sender, line, items[2], var)) return;
        if(!checkValue(sender, line, items[3], var)) return;
        if(!checkValue(sender, line, items[4], var)) return;

        if((items[2] == "<" && getValue(items[1], var) < getValue(items[3], var))||
           (items[2] == "<=" && getValue(items[1], var) <= getValue(items[3], var))||
           (items[2] == "==" && getValue(items[1], var) == getValue(items[3], var))||
           (items[2] == ">" && getValue(items[1], var) > getValue(items[3], var))||
           (items[2] == ">=" && getValue(items[1], var) >= getValue(items[3], var))||
           (items[2] == "!=" && getValue(items[1], var) != getValue(items[3], var)))
        {

            ret.status = "jump";
            ret.number = getValue(items[4], var);
        }

    }
    // [ jump P5 ] goto line +5
    else if(S > 0 && items[0] == "jump")
    {
        if(!checkLength(sender, line, "jump", 2, items.size())) return;
        if(!checkValue(sender, line, items[1], var)) return;

        ret.status = "jump";
        ret.number = getValue(items[1], var);

    }
    // [ if a > b ] if a>b then ... , else skip the next line. [ if a > b P6 ] skip to the 6th line.
    else if(S > 0 && items[0] == "if")
    {
        if(items.size() != 4 && items.size() != 6)
        {
            checkLength(sender, line, "if", 4, items.size());
            return;
        }

        if(!checkValue(sender, line, items[1], var)) return;
        if(!checkCompareSymbol(sender, line, items[2], var)) return;
        if(!checkValue(sender, line, items[3], var)) return;

        if((items[2] == "<" && getValue(items[1], var) < getValue(items[3], var))||
           (items[2] == "<=" && getValue(items[1], var) <= getValue(items[3], var))||
           (items[2] == "==" && getValue(items[1], var) == getValue(items[3], var))||
           (items[2] == ">" && getValue(items[1], var) > getValue(items[3], var))||
           (items[2] == ">=" && getValue(items[1], var) >= getValue(items[3], var))||
           (items[2] == "!=" && getValue(items[1], var) != getValue(items[3], var)))
        {

        }
        else
        {
            int jumpto = 2;
            if(items.size() == 6)
            {
                if(!checkValue(sender, line, items[5], var)) return;
                jumpto = getValue(items[5], var);
            }
            ret.status = "jump";
            ret.number = jumpto;
        }


    }
    // [ // comment ] use // to comment
    else if(S > 0 && items[0].length() >= 2 && items[0][0] == '/' && items[0][1] == '/')
    {
        return;
    }
    // [ end ] use end to end the code immediately. The code will also end when reach the last line.
    else if(S > 0 && items[0] == "end")
    {
        ret.status = "end";
        return;
    }
    // No valid code was found.
    else
    {
        sender() << "Error in line " + to_string(line) + " : Invalid code \"" + code + "\" was not declared.";
        ret.status = "failed";

    }



    return;
}



//----------------------------------------------------------------------------//
//                                                                            //
//             These codes are checkers for interpreter.                      //
//                                                                            //
//----------------------------------------------------------------------------//


// get the value of a string. It can be a either number or var.
int Question::getValue(string s, map<string,int> &var)
{
    if(var.count(s))
    {
        return var[s];
    }

    int ret = 0, sym = 1;
    int len = s.length();
    for(int i = 0; i < len; i++)
    {
        if(i == 0 && s[i] == 'N')
        {
            sym = -1;
            continue;
        }
        if(i == 0 && s[i] == 'P')
        {
            sym = 1;
            continue;
        }
        if(s[i] > '9' || s[i] < '0') return 0;
        ret = ret * 10;
        ret += s[i] - '0';
    }
    ret *= sym;
    return ret;
}


// get the real var like: x=3, p.x -> p.3
string Question::getRealVar(MsgSenderBase& sender, int line, string s, map<string,int> &var)
{
    string a = "", b = "";
    int len = s.length();
    int i = 0;
    for(i = 0; i < len; i++)
    {
        a += s[i];
        if(s[i] == '.')
            break;
    }
    if(i != len)
    {
        for(i = i + 1; i < len; i++)
        {
            b += s[i];
        }

        // first, .b -> b must be a var
        if(!checkValue(sender, line, b, var))
        {
            ret.status = "failed";
            sender() << "Error in line " + to_string(line) +
                        ": var \"" + b + "\" was not declared when finding \"" + s + "\"";
        }

        // then, a.b must be a var
        string c = a + to_string(getValue(b, var));
        return c;

    }


    return s;
}

// check whether l1 == l2
bool Question::checkLength(MsgSenderBase& sender, int line, string s, int l1, int l2)
{
    if(l1 == l2) return true;
    sender() << "Error in line " + to_string(line) +
                ": Operator \"" + s + "\" expects " + to_string(l1) +" elements, but found " + to_string(l2);
    ret.status = "failed";
    return false;
}

// check whether a string is a var
bool Question::checkVar(MsgSenderBase& sender, int line, string s, map<string,int> &var)
{
    if(var.count(s)) return true;
    sender() << "Error in line " + to_string(line) +
                ": var \"" + s + "\" was not declared ";
    ret.status = "failed";
    return false;
}


// check whether a string is not a var
bool Question::checkNotVar(MsgSenderBase& sender, int line, string s, map<string,int> &var)
{
    if(!var.count(s)) return true;
    sender() << "Error in line " + to_string(line) +
                ": var \"" + s + "\" has already been declared. ";
    ret.status = "failed";
    return false;
}

// check whether a string is a number
bool Question::checkNumber(string s)
{
    int len = s.length();
    for(int i = 0; i < len; i++)
    {
        if(i == 0 && s[i] == 'P') continue;
        if(i == 0 && s[i] == 'N') continue;
        if(s[i] > '9' || s[i] < '0') return false;
    }
    return true;
}


// check whether a string is a var or a number
bool Question::checkValue(MsgSenderBase& sender, int line, string s, map<string,int> &var)
{
    if(var.count(s)) return true;
    if(checkNumber(s)) return true;

    sender() << "Error in line " + to_string(line) +
                ": var \"" + s + "\" was not declared ";
    ret.status = "failed";
    return false;
}


// check whether a string is ">", "<", "==", "<=", ">=", or "!="
bool Question::checkCompareSymbol(MsgSenderBase& sender, int line, string s, map<string,int> &var)
{
    if(s == ">" || s == ">=" || s == "<" || s == "<=" || s == "==" || s == "!=") return true;

    sender() << "Error in line " + to_string(line) +
                ": symbol \"" + s + "\" not correct, you can only use [ >  <  == ] here. ";
    ret.status = "failed";
    return false;
}




//----------------------------------------------------------------------------//
//                                                                            //
//                       Interpreter ends here.                               //
//                                                                            //
//----------------------------------------------------------------------------//


void Question::createMarkdown()
{
    md = "";
    md += "<font size=7>";
    for(auto text:texts)
    {
        md += "　";
        md += text;
        md += "<br>";
    }
    md += "<br>";
    md += "</font>";
    md += "<font size=6>";
    int count = 0;
    for(auto option:options)
    {
        md += "　";
        md += (char)(count + 'A');
        count++;
        md += ". ";
        md += option;
        md += "<br>";
    }
    md += "<br>";
    md += "</font>";

    md += "<table><tr><td><font size=6>　　　　　　　　　　　　　　　　　　　　</font></td></tr></table>";

    return;
}


string Question::getMd()
{
    createMarkdown();
    return md;
}










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

    // player data
    vector<Player> players;
    // question data. read when game init
    vector<Question> questions;
    // store the used questions id to prevent from reusing
    set<int> used;
    int now;

  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
//        reply() << "这里输出当前游戏情况";
        // Returning |OK| means the game stage
        return StageErrCode::OK;
    }

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
        int &now = main_stage().now ;
        now = -1;
        if(main_stage().questions.size() == 0)
        {
            Boardcast() << "[错误] 问题列表不存在(RoundStage -> OnStageBegin -> questions.size())";
            StartTimer(option().GET_VALUE(时限));
            return;
        }
        int count = 0;
        while(now == -1 || main_stage().used.find(now) != main_stage().used.end())
        {
            now = rand() % main_stage().questions.size();

            if(count++ > 1000) break;
        }


        int TestQ = option().GET_VALUE(测试);
        if(TestQ != 99999)
        {
            if(TestQ < main_stage().questions.size())
            {
                now = TestQ;
            }
            else
            {
                Boardcast() << "[错误] 测试题号不存在(RoundStage -> OnStageBegin)";
                now = 0;
                StartTimer(option().GET_VALUE(时限));
                return;
            }
        }


        main_stage().used.insert(now);

        Boardcast() << "题号：" << to_string(now);
        Boardcast() << Markdown(main_stage().questions[now].getMd());

        StartTimer(option().GET_VALUE(时限));
    }

    virtual CheckoutErrCode OnTimeout() override
    {
        // Returning |CHECKOUT| means the current stage will be over.

        if(main_stage().questions[main_stage().now].options.size() == 0)
            return StageErrCode::CHECKOUT;

        for(int i=0;i<option().PlayerNum();i++)
        {
//             Boardcast()<<std::to_string(IsReady(i))<<std::to_string(main_stage().player_eli_[i]);
             if(IsReady(i) == false)
             {
                 int x = rand() % main_stage().questions[main_stage().now].options.size();
                 main_stage().players[i].nowC = x + 'A';
                 main_stage().players[i].nowN = x;
             }
        }

        calc();

        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        // Returning |CONTINUE| means the current stage will be continued.
        main_stage().players[pid].nowC = 'A';
        main_stage().players[pid].nowN = 0;


        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if(main_stage().questions[main_stage().now].options.size() == 0)
            return SubmitInternal_(pid, reply, "A");

        int x = rand() % main_stage().questions[main_stage().now].options.size();
        string ret = "A";
        ret[0] += x;
        return SubmitInternal_(pid, reply, ret);
    }

    virtual void OnAllPlayerReady() override
    {
//        Boardcast() << "所有玩家准备完成";

        if(main_stage().questions[main_stage().now].options.size() == 0)
            return;

        if(main_stage().now == -1)
        {
            return;
        }


        calc();
    }

    string str(string x)
    {
        int l = x.length();
        for(int i = 0; i < l; i++)
        {
            if(x[i] == '<' || x[i] == '>') x[i] = ' ';
        }
        return x;
    }

    // calc scores
    void calc()
    {
        for(int i = 0; i < option().PlayerNum(); i++)
        {
            main_stage().players[i].lastscore = main_stage().players[i].score;
        }


        main_stage().questions[main_stage().now].RunCode(main_stage().players, BoardcastMsgSender());



        string b = "";
        b += "<table style=\"text-align:center\"><tbody>";
        b += "<tr><td><font size=7>　　</font></td><td><font size=7>　　</font></td><td><font size=7>　　</font></td><tr>";
        for(int i = 0; i < option().PlayerNum(); i++)
        {
            string color;
            if(main_stage().players[i].lastscore < main_stage().players[i].score) color = "bgcolor=\"#90EE90\"";
            if(main_stage().players[i].lastscore == main_stage().players[i].score) color = "bgcolor=\"#F5DEB3\"";
            if(main_stage().players[i].lastscore > main_stage().players[i].score) color = "bgcolor=\"#FFA07A\"";

            b += (string)""
                    + "<tr>"
                    + "<td bgcolor=\"#D2F4F4\"><font size=7>" + str(PlayerName(i)) + "</font></td>"
                    + "<td " + color + "><font size=7>" + main_stage().players[i].nowC + "</font></td>"
                    + "<td " + color + "><font size=7>" + to_string(main_stage().players[i].score) + "</font></td>"
                    + "</tr>";
        }
        b += "</table>";

        Boardcast() << Markdown(b);


        std::this_thread::sleep_for(std::chrono::seconds(5));

        return;
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
        if(submission[0] - 'A' + 1 > main_stage().questions[main_stage().now].options.size())
        {
            reply() << "[错误] 选项不存在。";
            return StageErrCode::FAILED;
        }

        return SubmitInternal_(pid, reply, submission);
    }

    AtomReqErrCode SubmitInternal_(const PlayerID pid, MsgSenderBase& reply, string submission)
    {
        main_stage().players[pid].nowC = submission[0];
        main_stage().players[pid].nowN = submission[0] - 'A';

        return StageErrCode::READY;
    }
};

MainStage::VariantSubStage MainStage::OnStageBegin()
{
    srand((unsigned int)time(NULL));

    FILE *fp=fopen((string(option().ResourceDir())+("problems.txt")).c_str(),"r");
    if(fp == NULL)
    {
        Boardcast() << "[错误] 题目列表不存在。(P)";
        return make_unique<RoundStage>(*this, ++round_);
    }

    string s = "";
    int status = 0;
    Question q;

    while(1)
    {

        bool ok = false;
        s = "";
        char c = 0;
        while(fscanf(fp, "%c", &c) != EOF)
        {
            if(c == '\n')
            {
                ok = true;
                break;
            }
            s += c;
        }

        if(ok == false) break;

        if(s.length() != 0 && s[0] == '#')
        {
            status++;
            if(status == 4)
            {
                questions.push_back(q);
                status = 0;
            }
            continue;
        }

        if(status == 0)
        {
            q.texts.clear();
            q.options.clear();
            q.codes.clear();
        }
        if(status == 1)
        {
            q.texts.push_back(s);
        }
        if(status == 2)
        {
            if(s == "") continue;
            q.options.push_back(s);
        }
        if(status == 3)
        {
            q.codes.push_back(s);
        }
    }

//    for(int i = 0; i < questions.size(); i++)
//    {
//        Boardcast() << "问题" << to_string(i);
//        for(auto v:questions[i].texts) Boardcast() << v;
//        for(auto v:questions[i].options) Boardcast() << v;
//        for(auto v:questions[i].codes) Boardcast() << v;
//    }

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


    for(int i = 0; i < option().PlayerNum(); i++)
    {
        player_scores_[i] = players[i].score;
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
