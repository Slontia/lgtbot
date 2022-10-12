#include <array>
#include <functional>
#include <memory>
#include <set>

#include <map>
#include <vector>
#include <string>
#include <algorithm>

#ifndef PROBLEMS_
#define PROBLEMS_

using namespace std;

class Player
{
public:
	Player()
	{
		lastSelect = 0;
		select = 0;
		lastScore = 0;
		score = 0;
	}
	
	int lastSelect;
	int select;
	
	// the real last score. change after score calc is over.
	double realLastScore;
	
	// not last score. DO NOT use.
	double lastScore;
	double score;
	
	string getScore()
	{
		return to_string(score);
	}
};



//---------------------------------------------------------------------------//
double dec2(double x)
{
    x = x * 100;
    if(x > 0) x += 0.5;
    else x -= 0.5;
    x = (int) x;
    x = x / 100;

    return x;
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
//---------------------------------------------------------------------------//

class Question
{
public:
	
	Question()
	{
		vars.clear();
	} 
	
	int id;
	string author;
	string title; 
	vector<string> texts;
	vector<string> options;
	vector<string> expects;
	
	map<string,double> vars;
	
	
	double playerNum;
	double maxScore;
	double minScore;
	vector<double> optionCount; 
	double maxSelect;
	double minSelect;
	vector<double> tempScore;
	
	
	// init playerNum
	void init(vector<Player>& players)
	{
		playerNum = players.size();
	}
	
	// init texts and options. This function must be overloaded
	virtual void initTexts(){}
	virtual void initOptions(){}
	virtual void initExpects(){}
	
	
	
	// init calc before call it.
	void initCalc(vector<Player>& players)
	{
		maxScore = -99999;
		minScore = 99999;
		maxSelect = -99999;
		minSelect = 99999;
		optionCount.clear();
		
		for(int i = 0; i < options.size(); i++) optionCount.push_back(0);
		for(int i = 0; i < options.size(); i++) tempScore.push_back(0);
		
		for(int i = 0; i < playerNum; i++)
		{
			maxScore = max(maxScore, players[i].score);
			minScore = min(minScore, players[i].score);
			optionCount[players[i].select] += 1;
		}
		
		for(int i = 0; i < optionCount.size(); i++)
		{
			maxSelect = max(maxSelect, optionCount[i]);
			minSelect = min(minSelect, optionCount[i]);
		}
	}
	
	// calc function. This function must be overloaded.
	virtual void calc(vector<Player>& players){}
	
	// quick calc.
	void quickScore(vector<Player>& players)
	{
		for(int i = 0; i < players.size(); i++)
		{
			players[i].score += tempScore[players[i].select];
		}
	}
	
	string Markdown()
	{
		string md = "";
	    md += "<font size=7>";
	    for(auto text:texts)
	    {
	        md += "　";
	        md += text;
	        md += "<br>";
	    }
	    md += "</font>";
	    
	    md += "<table style=\"text-align:center\"><tbody>";
	    md += "<tr><td><font size=6>　　</font></td>";
	    md += "<td><font size=6>　　　　　　　　　　　　　　　　　　　　　　　　</font></td>";
	    int count = 0;
	    for(auto option:options)
	    {
	        md += "<tr><td bgcolor=\"#FFE4E1\"><font size=6>";
	        md += (char)(count + 'A');
	        count++;
	        md += "</font></td>";
			md += "<td bgcolor=\"#F5F5F5\"><font size=6>";
	        md += option;
	        md += "</font></td>";
	    }
	
	    return md;
	}
};

class QE : public Question
{
public:
	QE()
	{
		id = -1;
		author = "NULL";
		title = "测试题目";
	}
	
	virtual void initTexts() override
	{
		texts.push_back("我是texts");
	}
	virtual void initOptions() override
	{
		vars["v1"] = 1.36;
		options.push_back("我是选项1，选我获得" + str(vars["v1"]) + "分");
		options.push_back("我是选项2，选我获得2.17分");
	}
	virtual void initExpects() override
	{
		expects.push_back("a");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = vars["v1"];
		tempScore[1] = 2.17;
	}
};

class Q1 : public Question
{
public:
	Q1()
	{
		id = 1;
		author = "Mutsuki";
		title = "A会比B少吗？";
	}
	
	virtual void initTexts() override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
		options.push_back("如果选这项的人数比 B 少，+3 分。");
		options.push_back("如果选这项的人数最多，+2 分。");
		options.push_back("+1分。如果选这项的人最多，这项改为 -1。");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaabbbcc");
	}
	virtual void calc(vector<Player>& players) override
	{
		if(optionCount[0] < optionCount[1]) tempScore[0] = 3;
		if(optionCount[1] == maxSelect) tempScore[1] = 2;
		tempScore[2] = 1;
		if(optionCount[2] == maxSelect) tempScore[2] = -1;
	}
};

class Q2 : public Question
{
public:
	Q2()
	{
		id = 2;
		author = "ShenHuXiaoDe";
		title = "和平与战争";
	}
	
	virtual void initTexts() override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
		options.push_back("和平：+2分");
		options.push_back("战争：如果恰有两名玩家选择这个选项，则+6分。");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaaaab");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = 2;
		if(optionCount[1] == 2) tempScore[1] = 6;
	}
};

class Q3 : public Question
{
public:
	Q3()
	{
		id = 3;
		author = "Mutsuki";
		title = "中立与激进";
	}
	
	virtual void initTexts() override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
		vars["A"] = playerNum/4;
		options.push_back("中立：获得 " + str(vars["A"]) + " 分。");
		options.push_back("激进：获得 [ 选择 A 选项的玩家个数 / 2 ] 分。");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaaabbbbb");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = vars["A"];
		tempScore[1] = optionCount[0] / 2;
	}
};

class Q4 : public Question
{
public:
	Q4()
	{
		id = 4;
		author = "ShenHuXiaoDe";
		title = "五美德";
	}
	
	virtual void initTexts() override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
		vars["E"] = (int)playerNum / 2 + 1;
		options.push_back("谨慎：+1分。");
		options.push_back("团结：如果选择这项的人数最多，+3.5分。");
		options.push_back("智慧：如果选择这项的人数最少，+3分。");
		options.push_back("勇敢：如果只有一人选择这项，+5分。");
		options.push_back("公正：选择这项的人平分 " + str(vars["E"]) + " 分。");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaaabbbbbbbbbbbcccccdddddddeeeeeeeeeeeeeee");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = 1;
		if(optionCount[1] == maxSelect) tempScore[1] = 3.5;
		if(optionCount[2] == minSelect) tempScore[2] = 3;
		if(optionCount[3] == 1) tempScore[3] = 5;
		tempScore[4] = vars["E"] / optionCount[4];
	}
};

class Q5 : public Question
{
public:
	Q5()
	{
		id = 5;
		author = "Mutsuki";
		title = "就餐时间";
	}
	
	virtual void initTexts() override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
		options.push_back("独享：如果只有 1 人选择此项，获得 4 分。");
		options.push_back("聚餐：如果有 2 或 3 人选择此项，获得 3 分。");
		options.push_back("盛宴：如果有 4 或 5 人选择此项，获得 2 分。");
		options.push_back("闭户：获得 1 分。");
	}
	virtual void initExpects() override
	{
		expects.push_back("aabbbbbcccccccccddddddd");
	}
	virtual void calc(vector<Player>& players) override
	{
		if(optionCount[0] == 1) tempScore[0] = 4;
		if(optionCount[1] == 2 || optionCount[1] == 3) tempScore[1] = 3;
		if(optionCount[2] == 4 || optionCount[2] == 5) tempScore[2] = 2;
		tempScore[3] = 1;
	}
};

class Q6 : public Question
{
public:
	Q6()
	{
		id = 6;
		author = "Mutsuki";
		title = "破坏与合作";
	}
	
	virtual void initTexts() override
	{
		texts.push_back("选择一项。");
		texts.push_back(" [ 只有选择人数最多的选项会生效 ]"); 
	}
	virtual void initOptions() override
	{
		options.push_back("破坏：-1，然后使选 B 的玩家 -3");
		options.push_back("合作：+2，然后使选 C 的玩家 +1");
		options.push_back("平衡：-0.5。");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaaaabbbcccc");
	}
	virtual void calc(vector<Player>& players) override
	{
		if(optionCount[0] == maxSelect)
		{
			tempScore[0] -= 1;
			tempScore[1] -= 3;
		}
		if(optionCount[1] == maxSelect)
		{
			tempScore[1] += 2;
			tempScore[2] += 1;
		}
		if(optionCount[2] == maxSelect)
		{
			tempScore[2] -= 0.5;
		}
	}
};

class Q7 : public Question
{
public:
	Q7()
	{
		id = 7;
		author = "Mutsuki";
		title = "均衡-3";
	}
	
	virtual void initTexts() override
	{
		texts.push_back("选择一项。先执行 A ，后执行 B 。");
	}
	virtual void initOptions() override
	{
		options.push_back("均衡：如果有 3 或更多名玩家选择本项，则得分最高的玩家 -3");
		options.push_back("观望：+1");
	}
	virtual void initExpects() override
	{
		expects.push_back("abbbb");
	}
	virtual void calc(vector<Player>& players) override
	{
		if(optionCount[0] >= 3)
		{
			for(int i = 0; i < playerNum; i++)
			{
				if(players[i].score == maxScore)
				{
					players[i].score -= 3;
				}
			}
		}
		tempScore[1] = 1;
	}
};

class Q8 : public Question
{
public:
	Q8()
	{
		id = 8;
		author = "Mutsuki";
		title = "幽灵平分";
	}
	
	virtual void initTexts() override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
		vars["A"] = playerNum;
		vars["B1"] = (int)(playerNum / 4);
		vars["B2"] = (int)(playerNum * 1.5); 
		options.push_back("均势：选择本项的人平分" + str(vars["A"]) + "分。");
		options.push_back("幽灵：选择本项的人，按人数+" + str(vars["B1"]) + "计算，平分"
		 + str(vars["B2"]) + "分。");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaaabbbbb");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = vars["A"] / optionCount[0];
		tempScore[1] = vars["B2"] / (optionCount[1] + vars["B1"]);
	}
};

class Q9 : public Question
{
public:
	Q9()
	{
		id = 9;
		author = "Mutsuki";
		title = "好学生";
	}
	
	virtual void initTexts() override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
		options.push_back("老师：如果好学生比坏学生多，+2");
		options.push_back("好学生：如果比坏学生多，+1");
		options.push_back("坏学生：如果比好学生多，老师 -2");
		options.push_back("校霸：如果坏学生比好学生多，+0.5");
	}
	virtual void initExpects() override
	{
		expects.push_back("aabbbccccdddd");
	}
	virtual void calc(vector<Player>& players) override
	{
		if(optionCount[1] > optionCount[2]) tempScore[0] += 2;
		if(optionCount[1] > optionCount[2]) tempScore[1] += 1;
		if(optionCount[1] < optionCount[2]) tempScore[0] -= 2;
		if(optionCount[1] < optionCount[2]) tempScore[3] += 0.5;
	}
};

class Q10 : public Question
{
public:
	Q10()
	{
		id = 10;
		author = "Mutsuki";
		title = "背叛者";
	}
	
	virtual void initTexts() override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
		options.push_back("老实人：+3。");
		options.push_back("背叛者：-1。如果任何人选择此项，则选 A 的玩家改为 -2。");
		options.push_back("旁观者：-0.5。");
		options.push_back("狂热者：-0.6。这个选项有极小的概率变为 +77。");
	}
	virtual void initExpects() override
	{
		expects.push_back("aabbcccccdd");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] += 3;
		tempScore[1] -= 1;
		tempScore[2] -= 0.5;
		if(optionCount[1] > 0) tempScore[0] = -2;
		
		tempScore[3] -= 0.6;
		if(rand() % 1000 < 9)
		{
			tempScore[3] = 77;
		} 
	}
};

class Q11 : public Question
{
public:
	Q11()
	{
		id = 11;
		author = "Mutsuki";
		title = "制度";
	}
	
	virtual void initTexts() override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
		options.push_back("苦工：+1。");
		options.push_back("老板：如果存在苦工，+2。否则 -1 。");
		options.push_back("高官：如果存在老板，+2.5。");
		options.push_back("制度：如果存在高官，+3。");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaabbbbcddddddddddddddddddddddd");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] += 1;
		if(optionCount[0] > 0) tempScore[1] = 2;
		else tempScore[1] = -1;
		
		if(optionCount[1] > 0) tempScore[2] += 2.5;
		if(optionCount[2] > 0) tempScore[3] += 3;
	}
};

class Q12 : public Question
{
public:
	Q12()
	{
		id = 12;
		author = "Mutsuki";
		title = "好运！";
	}
	
	virtual void initTexts() override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
		vars["D"] = (int)(playerNum / 3) + 2;
		options.push_back("智慧：人数比 B 少则 +3");
		options.push_back("体能：人数比 A 多则 +2");
		options.push_back("坚持：+1");
		options.push_back("好运：如果恰有 " + str(vars["D"]) + " 玩家选择这个选项，+" + 
		str(vars["D"]));
	}
	virtual void initExpects() override
	{
		expects.push_back("abbccccccccccccddddd");
	}
	virtual void calc(vector<Player>& players) override
	{
		if(optionCount[0] < optionCount[1]) tempScore[0] = 3;
		if(optionCount[0] < optionCount[1]) tempScore[1] = 2;
		tempScore[2] = 1;
		if(optionCount[3] == vars["D"]) tempScore[3] = vars["D"];
	}
};

class Q13 : public Question
{
public:
	Q13()
	{
		id = 13;
		author = "Dva";
		title = "顾虑与背叛";
	}
	
	virtual void initTexts() override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
		options.push_back("信任：+3 分");
		options.push_back("怀疑：+2 分");
		options.push_back("顾虑：-1 分");
		options.push_back("背叛：失去等同于选择该选项人数的分数。\
如果有超过 1 玩家选择本选项，则 A 改为 -4，B改为 -2 。");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaaabccccccccccdddd");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = 3;
		tempScore[1] = 2;
		tempScore[2] = -1;
		tempScore[3] = -optionCount[3];
		
		if(optionCount[3] > 1)
		{
			tempScore[0] = -4;
			tempScore[1] = -2;
		}
	}
};

class Q14 : public Question
{
public:
	Q14()
	{
		id = 14;
		author = "Dva";
		title = "抽屉原理";
	}
	
	virtual void initTexts() override
	{
		texts.push_back("选择一项。对于每个选项，如果所选人数小于等于其人数，\
则选择该选项的人获得对应分数。");
	}
	virtual void initOptions() override
	{
		vars["A"] = 1;
		vars["B"] = (int)(playerNum * 2 / 10) + 1;
		vars["C"] = (int)(playerNum * 3 / 10) + 1;
		vars["D"] = (int)(playerNum * 4 / 10);
		options.push_back(str(vars["A"]) + "人，3分");
		options.push_back(str(vars["B"]) + "人，2分");
		options.push_back(str(vars["C"]) + "人，1分");
		options.push_back(str(vars["D"]) + "人，2分");
	}
	virtual void initExpects() override
	{
		expects.push_back("abbcccdddd");
	}
	virtual void calc(vector<Player>& players) override
	{
		if(optionCount[0] <= vars["A"]) tempScore[0] = 3;
		if(optionCount[1] <= vars["B"]) tempScore[1] = 2;
		if(optionCount[2] <= vars["C"]) tempScore[2] = 1;
		if(optionCount[3] <= vars["D"]) tempScore[3] = 2;
	}
};

class Q15 : public Question
{
public:
	Q15()
	{
		id = 15;
		author = "Guest";
		title = "自负与平庸";
	}
	
	virtual void initTexts() override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
		options.push_back("自负：如若选C的人最多，+2。如若选C的人最少，-1。");
		options.push_back("叛逆：如若选C的人最少，+3。如若选C的人最多，-1。");
		options.push_back("平庸：+1。");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaaabbccccccccc");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[2] = 1;
		
		if(optionCount[2] == maxSelect)
		{
			tempScore[0] += 2;
			tempScore[1] -= 1;
		}
		if(optionCount[2] == minSelect)
		{
			tempScore[0] -= 1;
			tempScore[1] += 3;
		} 
	}
};

class Q16 : public Question
{
public:
	Q16()
	{
		id = 16;
		author = "403";
		title = "AB都有吗？";
	}
	
	virtual void initTexts() override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
		options.push_back("如果选择该项的人数比 B 多，-1 分。");
		options.push_back("如果选择该项的人数比 A 多，-1 分。");
		options.push_back("如若前两个选项都有人选择，+0。否则 -4 。");
	}
	virtual void initExpects() override
	{
		expects.push_back("aabbccccc");
	}
	virtual void calc(vector<Player>& players) override
	{
		if(optionCount[0] > optionCount[1]) tempScore[0] = -1;
		if(optionCount[1] > optionCount[0]) tempScore[1] = -1;
		
		if(!optionCount[0] || !optionCount[1]) tempScore[2] = -4;
	}
};

class Q17 : public Question
{
public:
	Q17()
	{
		id = 17;
		author = "Guest";
		title = "内卷与摸鱼";
	}
	
	virtual void initTexts() override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
		options.push_back("内卷：+3 ，如果选择人数大于另外两项之和，改为 -3");
		options.push_back("正常：+1");
		options.push_back("摸鱼：-1 ，如果选择人数小于另外两项之和，改为 +2");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaaaabcccc");
	}
	virtual void calc(vector<Player>& players) override
	{
		if(optionCount[0] > optionCount[1] + optionCount[2]) tempScore[0] = -3;
		else tempScore[0] = 3;
		if(optionCount[2] < optionCount[1] + optionCount[0]) tempScore[2] = +2;
		else tempScore[2] = -1;
		
		tempScore[1] = 1;
	}
};

class Q18 : public Question
{
public:
	Q18()
	{
		id = 18;
		author = "Dva";
		title = "战力平分";
	}
	
	virtual void initTexts() override
	{
		vars["S1"] = (int)(playerNum / 3);
		vars["S2"] = (int)(playerNum / 2);
		texts.push_back("选择一个阵营加入，每个阵营内部平分 " + 
		str(vars["S1"]) + " 分，然后战斗力最高的阵营平分 " + str(vars["S2"]) + " 分");
	}
	virtual void initOptions() override
	{
		options.push_back("团结：每有一个加入者，+3战力。");
		options.push_back("科技：战力固定为 " + str(playerNum) + "。");
		options.push_back("经济：战力为 -1 。该阵营有 2 个或更多加入者时，不平分 " + 
		str(vars["S1"]) + " 分，而是 " + str(vars["S2"]) + " 分。");
	}
	virtual void initExpects() override
	{
		expects.push_back("abc");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] += vars["S1"] / optionCount[0];
		tempScore[1] += vars["S1"] / optionCount[1];
		tempScore[2] += vars["S1"] / optionCount[2];
		
		if(optionCount[0] * 3 >= playerNum) tempScore[0] += vars["S2"] / optionCount[0];
		if(optionCount[0] * 3 <= playerNum) tempScore[1] += vars["S2"] / optionCount[1];
		if(optionCount[2] >= 2) tempScore[2] = vars["S2"] / optionCount[2];
	}
};

class Q19 : public Question
{
public:
	Q19()
	{
		id = 19;
		author = "Guest";
		title = "传谣与思考";
	}
	
	virtual void initTexts() override
	{
		texts.push_back("面对谣言，你选择：");
	}
	virtual void initOptions() override
	{
		vars["A"] = (int)(playerNum / 3);
		vars["B"] = (int)(playerNum * 3 / 5);
		options.push_back("传谣：若选择人数<= " + str(vars["A"]) + "，+2。否则 -1。");
		options.push_back("沉默：+0。");
		options.push_back("思考：若选择人数>= " + str(vars["B"]) + "，+1。否则 -2。");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaaabbbbbcc");
	}
	virtual void calc(vector<Player>& players) override
	{
		if(optionCount[0] <= vars["A"]) tempScore[0] = 2;
		else tempScore[0] = -1;
		
		tempScore[1] = 0;
		
		if(optionCount[2] >= vars["B"]) tempScore[2] = 1;
		else tempScore[2] = -2;
	}
};

class Q20 : public Question
{
public:
	Q20()
	{
		id = 20;
		author = "403";
		title = "+1，+0，+3";
	}
	
	virtual void initTexts() override
	{
		texts.push_back("选择一项");
	}
	virtual void initOptions() override
	{
		options.push_back("+1，然后均分 [ 选择 B 选项人数 ] 的分数。");
		options.push_back("+0，然后均分 [ 选择 C 选项人数 ] 的分数。");
		options.push_back("+3。");
	}
	virtual void initExpects() override
	{
		expects.push_back("abbbccccccccc");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = optionCount[1] / optionCount[0] + 1;
		tempScore[1] = optionCount[2] / optionCount[1] + 0;
		tempScore[2] = 3;
	}
};

class Q21 : public Question
{
public:
	Q21()
	{
		id = 21;
		author = "Mutsuki";
		title = "委婉与强硬";
	}
	
	virtual void initTexts() override
	{
		texts.push_back("选择一项");
	}
	virtual void initOptions() override
	{
		options.push_back("委婉：-2");
		options.push_back("强硬：如果所有玩家全部选择该项，则所有玩家分数取反。");
	}
	virtual void initExpects() override
	{
		expects.push_back("abbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = -2;
		
		if(optionCount[1] == playerNum)
		{
			for(int i = 0; i < playerNum; i++)
			{
				players[i].score = -players[i].score;
			}
		}
	}
};

class Q22 : public Question
{
public:
	Q22()
	{
		id = 22;
		author = "luna";
		title = "负分大转盘";
	}
	
	virtual void initTexts() override
	{
		texts.push_back("负分大转盘，选择一项。");
		texts.push_back("扣分最多的玩家获胜，获得（1 + 没有选择该选项玩家的数量 / 2）分。");
	}
	virtual void initOptions() override
	{
		vars["E"] = - 4 - playerNum / 4;
		options.push_back("+0");
		options.push_back("-1");
		options.push_back("-2");
		options.push_back("-3");
		options.push_back(str(vars["E"]));
	}
	virtual void initExpects() override
	{
		expects.push_back("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbcccccdddeeeeeeee");
	}
	virtual void calc(vector<Player>& players) override
	{
		for(int i = 4; i >= 0; i--)
		{
			if(optionCount[i] != 0)
			{
				tempScore[i] = 1 + (playerNum - optionCount[i]) / 2;
				break;
			}
		}
		tempScore[0] += 0;
		tempScore[1] += -1;
		tempScore[2] += -2;
		tempScore[3] += -3;
		tempScore[4] += vars["E"];
	}
};

class Q23 : public Question
{
public:
	Q23()
	{
		id = 23;
		author = "YAMI";
		title = "同化";
	}
	
	virtual void initTexts() override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
		options.push_back("同化：+1，然后所有选择此项的玩家会均分他们的分数。");
		options.push_back("排斥：+0。");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaaaaaaaaaaaaaaaab");
	}
	virtual void calc(vector<Player>& players) override
	{
		double sum = 0;
		for(int i = 0; i < playerNum; i++)
		{
			if(players[i].select == 0)
			{
				sum += players[i].score;
				players[i].score = 0; 
			}
		}
		
		tempScore[0] = 1 + sum / optionCount[0];
	}
};

class Q24 : public Question
{
public:
	Q24()
	{
		id = 24;
		author = "Mutsuki";
		title = "慈善家";
	}
	
	virtual void initTexts() override
	{
		texts.push_back("选择一项。");
		texts.push_back("只有人数最多的选项会产生效果。");
	}
	virtual void initOptions() override
	{
		options.push_back("乞丐：-2。");
		options.push_back("慈善家：选 A 或 C 的玩家 +2。");
		options.push_back("乞丐：-2。");
		options.push_back("路人：-0.5。");
	}
	virtual void initExpects() override
	{
		expects.push_back("aabbbccddd");
	}
	virtual void calc(vector<Player>& players) override
	{
		if(optionCount[0] == maxSelect) tempScore[0] += -2;
		if(optionCount[2] == maxSelect) tempScore[2] += -2;
		if(optionCount[1] == maxSelect)
		{
			tempScore[0] += 2;
			tempScore[2] += 2;
		}
		if(optionCount[3] == maxSelect) tempScore[3] += -0.5;
	}
};

class Q25 : public Question
{
public:
	Q25()
	{
		id = 25;
		author = "Mutsuki";
		title = "美人投票";
	}
	
	virtual void initTexts() override
	{
		texts.push_back("选择一项。");
		texts.push_back("选择完毕后，会统计所有玩家所选数字的平均值（下取整）。\
然后选择了数字为平均值选项的玩家，获得该数字的分数。");
	}
	virtual void initOptions() override
	{
		options.push_back("0");
		options.push_back("1");
		options.push_back("2");
		options.push_back("3");
		options.push_back("4");
		options.push_back("5");
		options.push_back("6");
		options.push_back("7");
	}
	virtual void initExpects() override
	{
		expects.push_back("abbbccccccccccccccccccccccccccccddddddddddddddddddddddddeeeeffgh");
	}
	virtual void calc(vector<Player>& players) override
	{
		int sum = 0, ave = 0;
		for(int i = 0; i < playerNum; i++)
		{
			sum += players[i].select;
		}
		ave = (int)(sum / playerNum);
		
		tempScore[ave] = ave;
	}
};

class Q26 : public Question
{
public:
	Q26()
	{
		id = 26;
		author = "Mutsuki";
		title = "过半效果";
	}
	
	virtual void initTexts() override
	{
		texts.push_back("选择一项。");
		texts.push_back("超过一半人选择某一选项时，该选项会改为不同的（括号中的）效果。");
	}
	virtual void initOptions() override
	{
		options.push_back("拔河：-1 ( +1.5 )");
		options.push_back("过桥：+1 ( +0 )");
		options.push_back("坐船：+2 ( -2 )");
	}
	virtual void initExpects() override
	{
		expects.push_back("aabbbbbbbccccc");
	}
	virtual void calc(vector<Player>& players) override
	{
		if(optionCount[0] > playerNum / 2) tempScore[0] = 1.5;
		else tempScore[0] = -1;
		
		if(optionCount[1] > playerNum / 2) tempScore[1] = 0;
		else tempScore[1] = 1;
				
		if(optionCount[2] > playerNum / 2) tempScore[2] = -2;
		else tempScore[2] = 2;
	}
};

class Q27 : public Question
{
public:
	Q27()
	{
		id = 27;
		author = "Saiwei";
		title = "危楼";
	}
	
	virtual void initTexts() override
	{
		texts.push_back("所有玩家被逼上一座危楼，请选择到几层。");
		texts.push_back("选择人数最高的楼层会垮塌，使得其得分改为 -1。最底层不会垮塌。");
		texts.push_back("同时，垮塌楼层的下方一层额外 -0.5。");
	}
	virtual void initOptions() override
	{
		options.push_back("第四层：+1.5");
		options.push_back("第三层：+0.5");
		options.push_back("第二层：+1");
		options.push_back("最底层：+0");
	}
	virtual void initExpects() override
	{
		expects.push_back("abcd");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = 1.5;
		tempScore[1] = 0.5;
		tempScore[2] = 1;
		tempScore[3] = 0;
		for(int i = 2; i >= 0; i--)
		{
			if(optionCount[i] == maxSelect)
			{
				tempScore[i] = -1;
				tempScore[i + 1] -= 0.5;
			}
		}
	}
};

class Q28 : public Question
{
public:
	Q28()
	{
		id = 28;
		author = "Neverlandre";
		title = "+2与-1.5";
	}
	
	virtual void initTexts() override
	{
		texts.push_back("选择一项。选择的人最多的选项，分值变为其相反数");
	}
	virtual void initOptions() override
	{
		options.push_back("+2");
		options.push_back("+1");
		options.push_back("0");
		options.push_back("-1.5");
	}
	virtual void initExpects() override
	{
		expects.push_back("abcd");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = 2;
		tempScore[1] = 1;
		tempScore[2] = 0;
		tempScore[3] = -1.5;
		for(int i = 0; i <= 3; i++)
		{
			if(optionCount[i] == maxSelect)
			{
				tempScore[i] = -tempScore[i];
			}
		}
	}
};

class Q29 : public Question
{
public:
	Q29()
	{
		id = 29;
		author = "Neverlandre";
		title = "血染钟楼";
	}
	
	virtual void initTexts() override
	{
		texts.push_back("请选择一项身份");
	}
	virtual void initOptions() override
	{
		options.push_back("村民 （+0.5分，若场上外来者数比村民多，则村民变为+1分）");
		options.push_back("外来者 （+1.5分，若人数最多，改为-0.5分）");
		options.push_back("爪牙 （+2分，若人数最多，改为-1分）");
		options.push_back("恶魔（如果爪牙比村民和外来者都多，则+4分，否则+0.5分。\
若人数最多，则额外失去 [恶魔数量 / 2] 分。）");
	}
	virtual void initExpects() override
	{
		expects.push_back("abcd");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = 0.5;
		if(optionCount[1] > optionCount[0]) tempScore[0] = 1;
		
		tempScore[1] = 1.5;
		if(optionCount[1] == maxSelect) tempScore[1] = -0.5;
		
		tempScore[2] = 2;
		if(optionCount[2] == maxSelect) tempScore[2] = -1;
		
		tempScore[3] = 0.5;
		if(optionCount[2] > optionCount[0] && optionCount[2] > optionCount[1]) tempScore[3] = 4;
		if(optionCount[3] == maxSelect) tempScore[3] -= optionCount[3] / 2;
		
	}
};

class Q30 : public Question
{
public:
	Q30()
	{
		id = 30;
		author = "Mutsuki";
		title = "时光机";
	}
	
	virtual void initTexts() override
	{
		texts.push_back("请选择一项。");
		texts.push_back("只有选择人数最多的选项会生效。");
		texts.push_back("若有多个选项生效，则依次执行。");
	}
	virtual void initOptions() override
	{
		options.push_back("过去：所有人的得分变为上一回合的得分，然后你 -1");
		options.push_back("未来：没有选择本项的人 +1");
		options.push_back("现在：选择本项的人 -0.5");
		options.push_back("摧毁：所有人得分变为 0 。然后你 -30");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaabbbbbbbcccccccccccccd");
	}
	virtual void calc(vector<Player>& players) override
	{
		if(optionCount[0] == maxSelect)
		{
			for(int i = 0; i < playerNum; i++)
			{
				players[i].score = players[i].realLastScore;
				if(players[i].select == 0)
					players[i].score -= 1;
			}
		}
		if(optionCount[1] == maxSelect)
		{
			for(int i = 0; i < playerNum; i++)
			{
				if(players[i].select != 1)
					players[i].score += 1;
			}
		}
		if(optionCount[2] == maxSelect)
		{
			for(int i = 0; i < playerNum; i++)
			{
				if(players[i].select == 2)
					players[i].score -= 0.5;
			}
		}
		if(optionCount[3] == maxSelect)
		{
			for(int i = 0; i < playerNum; i++)
			{
				players[i].score = 0;
				if(players[i].select == 3)
					players[i].score -= 30;
			}
		}
		
	}
};

class Q31 : public Question
{
public:
	Q31()
	{
		id = 31;
		author = "FishToucher";
		title = "分金币";
	}
	
	virtual void initTexts() override
	{
		vars["coins"] = playerNum * 2.5;
		vars["del"] = playerNum / 2 + 2;
		texts.push_back("金库中有" + str(vars["coins"]) + "金币。");
		texts.push_back("玩家获得的金币将折合成同等的分数。");
	}
	virtual void initOptions() override
	{
		options.push_back("投资：-1。所有投资者将均分金库中的金币。");
		options.push_back("储蓄：+0.5，并使金库中金币 +1。");
		options.push_back("等待：+0，并使金库中金币 -1。");
		options.push_back("盗窃：-2，并使金库中金币 -" + str(vars["del"]));
	}
	virtual void initExpects() override
	{
		expects.push_back("abcd");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = -1;
		tempScore[1] = 0.5;
		tempScore[2] = 0;
		tempScore[3] = -2;
		
		int coins = vars["coins"];
		coins += optionCount[1];
		coins -= optionCount[2];
		coins -= optionCount[3] * vars["del"];
		
		tempScore[0] += coins / optionCount[0];
	}
};

class Q32 : public Question
{
public:
	Q32()
	{
		id = 32;
		author = "FishToucher";
		title = "排队难题";
	}
	
	virtual void initTexts() override
	{
		texts.push_back("选择一项。");
		texts.push_back("只有选择人数最多的选项会被执行。");
		texts.push_back("有多个选项人数最多时，只执行最靠后的选项。");
	}
	virtual void initOptions() override
	{
		options.push_back("A+1 B+4 C-1 D+1");
		options.push_back("A+2 B-3 C+2 D+3");
		options.push_back("A+2 B+3 C+1 D-1");
		options.push_back("A+1 B+3 C+1 D+0");
	}
	virtual void initExpects() override
	{
		expects.push_back("abcd");
	}
	virtual void calc(vector<Player>& players) override
	{
		if(optionCount[0] == maxSelect)
		{
			tempScore[0] = 1;
			tempScore[1] = 4;
			tempScore[2] = -1;
			tempScore[3] = 1;
		}
		if(optionCount[1] == maxSelect)
		{
			tempScore[0] = 2;
			tempScore[1] = -3;
			tempScore[2] = 2;
			tempScore[3] = 3;
		}
		if(optionCount[2] == maxSelect)
		{
			tempScore[0] = 2;
			tempScore[1] = 3;
			tempScore[2] = 1;
			tempScore[3] = -1;
		}
		if(optionCount[3] == maxSelect)
		{
			tempScore[0] = 1;
			tempScore[1] = 3;
			tempScore[2] = 1;
			tempScore[3] = 0;
		}
	}
};

class Q33 : public Question
{
public:
	Q33()
	{
		id = 33;
		author = "Mutsuki";
		title = "均衡联合体";
	}
	
	virtual void initTexts() override
	{
		texts.push_back("选择一项。");
		texts.push_back("本题目中，将统计人数最多的选项，将其人数记作MAX。同理人数最少的选项为MIN。");
	}
	virtual void initOptions() override
	{
		vars["D"] = playerNum / 3;
		options.push_back("分歧：如若 MAX - MIN > " + str(vars["D"]) + " ，+1");
		options.push_back("秩序：如若 MAX - MIN <= " + str(vars["D"]) + " ，+3");
		options.push_back("完美：如果 MAX - MIN <= 1， +6");
	}
	virtual void initExpects() override
	{
		expects.push_back("abc");
	}
	virtual void calc(vector<Player>& players) override
	{
		if(maxSelect - minSelect > vars["D"]) tempScore[0] = 1;
		if(maxSelect - minSelect <= vars["D"]) tempScore[1] = 3;
		if(maxSelect - minSelect <= 1) tempScore[2] = 6;
	}
};

class Q34 : public Question
{
public:
	Q34()
	{
		id = 34;
		author = "Mutsuki";
		title = "贪心取金";
	}
	
	virtual void initTexts() override
	{
		vars["coins"] = playerNum;
		texts.push_back("选择一项。");
		texts.push_back("金库中一共有 " + str(vars["coins"]) + " 枚金币。");
		texts.push_back("从数字最大的选项开始，所有选择了该项的玩家拿去等量的金币，依次执行下去。");
		texts.push_back("但是，如果金币的数量不够某选项的玩家分，则会跳过这个选项。");
	}
	virtual void initOptions() override
	{
		options.push_back("拿取 3");
		options.push_back("拿取 2");
		options.push_back("拿取 1");
		options.push_back("拿取 0.5");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaaabbbccd");
	}
	virtual void calc(vector<Player>& players) override
	{
		double need[4] = {3, 2, 1, 0.5};
		int coins = vars["coins"];
		for(int i = 0; i < 4; i++)
		{
			if(coins >= optionCount[i] * need[i])
			{
				coins -= optionCount[i] * need[i];
				tempScore[i] = need[i];
			}
		}
	}
};

/*

Neverlandre 2022/10/7 13:35:47
请任意选择一项：（本题结算顺序固定从刚进入本题时的分数 从高到低结算）
a:如果你不是分数最低的人并选择了此项，则你免疫b选项受到的影响。
b:如果你是分数最高的人并选择了此项，则你与最后一名互换分数；如果你不是分数
最高的人并选择了此项，则与你的上一名互换分数。

选择一项，然后选择最高的人全部拿分数，顺次拿，除非拿不了。 

FishToucher 
按照人数最多的一组执行
A．	乐善好施（本组每人+1分，人数最少的一组每人+3分）
B．	人人平等（所有人获得1分）
C．	众志成城（如果本组人数大于等于其他组的总和，每人+3分；否则每人-3分）
D．	雨露均沾（本组每人+2分，其他组每组平分3分）
E．	反向赌博（本组每人-5分）


*/ 


#endif
