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

    virtual ~Question() {};
	
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
	double nonZero_minSelect;
	vector<double> tempScore;
	
	
	// init playerNum
	void init(vector<Player>& players)
	{
		playerNum = players.size();
	}
	
	// init texts and options. This function must be overloaded
	virtual void initTexts(vector<Player>& players){}
	virtual void initOptions(){}
	virtual void initExpects(){}
	
	
	
	// init calc before call it.
	void initCalc(vector<Player>& players)
	{
		maxScore = -99999;
		minScore = 99999;
		maxSelect = -99999;
		minSelect = 99999;
		nonZero_minSelect = 99999;
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
			if (optionCount[i] > 0) {
				nonZero_minSelect = min(nonZero_minSelect, optionCount[i]);
			}
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
		md += "</tr></table>";
	
	    return md;
	}
	
	string String(){
		string str = "";
		for(auto text:texts)
		{
		    str += "　";
		    str += text;
			str += "\n";
		}
		str += "\n";
		int count = 0;
		for(auto option:options)
		{
		    str += (char)(count + 'A');
			str += ": ";
		    count++;
		    str += option;
			str += "\n";
		}
		return str;
	}
	
    static double MaxPlayerScore(const std::vector<Player>& players) {
        return std::ranges::max_element(players, [](const Player& p1, const Player& p2) { return p1.score < p2.score; })->score;
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
	
	virtual void initTexts(vector<Player>& players) override
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
	
	virtual void initTexts(vector<Player>& players) override
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
	
	virtual void initTexts(vector<Player>& players) override
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
	
	virtual void initTexts(vector<Player>& players) override
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
	
	virtual void initTexts(vector<Player>& players) override
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
	
	virtual void initTexts(vector<Player>& players) override
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
	
	virtual void initTexts(vector<Player>& players) override
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
	
	virtual void initTexts(vector<Player>& players) override
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
	
	virtual void initTexts(vector<Player>& players) override
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
	
	virtual void initTexts(vector<Player>& players) override
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
	
	virtual void initTexts(vector<Player>& players) override
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
	
	virtual void initTexts(vector<Player>& players) override
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
	
	virtual void initTexts(vector<Player>& players) override
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
	
	virtual void initTexts(vector<Player>& players) override
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
	
	virtual void initTexts(vector<Player>& players) override
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
	
	virtual void initTexts(vector<Player>& players) override
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
	
	virtual void initTexts(vector<Player>& players) override
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
	
	virtual void initTexts(vector<Player>& players) override
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
	
	virtual void initTexts(vector<Player>& players) override
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
	
	virtual void initTexts(vector<Player>& players) override
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
	
	virtual void initTexts(vector<Player>& players) override
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
	
	virtual void initTexts(vector<Player>& players) override
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
	
	virtual void initTexts(vector<Player>& players) override
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
	
	virtual void initTexts(vector<Player>& players) override
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
	
	virtual void initTexts(vector<Player>& players) override
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
	
	virtual void initTexts(vector<Player>& players) override
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
	
	virtual void initTexts(vector<Player>& players) override
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
	
	virtual void initTexts(vector<Player>& players) override
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
	
	virtual void initTexts(vector<Player>& players) override
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
	
	virtual void initTexts(vector<Player>& players) override
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
	
	virtual void initTexts(vector<Player>& players) override
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
	
	virtual void initTexts(vector<Player>& players) override
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
	
	virtual void initTexts(vector<Player>& players) override
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
	
	virtual void initTexts(vector<Player>& players) override
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
	
	virtual void initTexts(vector<Player>& players) override
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

class Q35 : public Question
{
public:
	Q35()
	{
		id = 35;
		author = "纤光";
		title = "选举";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
        win_score_ = double(playerNum) / double(2);
		options.push_back("支持票：平分 " + str(win_score_) + " 分，+1");
		options.push_back("反对票：反对票不少于支持票时，A 项得分取反，反之 -2");
		options.push_back("弃权：弃权票为最多之一时，-1");
	}
	virtual void initExpects() override
	{
		expects.push_back("aabbbc");
	}
	virtual void calc(vector<Player>& players) override
	{
        if (optionCount[2] == maxSelect) {
            tempScore[2] = -1;
        }
        if (optionCount[1] >= optionCount[0]) {
            tempScore[0] = -win_score_ / double(optionCount[0]) - 1;
        } else {
            tempScore[0] = win_score_ / double(optionCount[0]) + 1;
            tempScore[1] = -2;
        }
	}

private:
    double win_score_;
};

class Q36 : public Question
{
public:
	Q36()
	{
		id = 36;
		author = "纤光";
		title = "种姓制度";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
		options.push_back("首陀罗：+0.5");
		options.push_back("吠舍：如果有人选A，+1");
		options.push_back("刹帝利：如果选A和B的人数不少于一半，+1.5");
		options.push_back("婆罗门：如果只有你一人选此项，+2");
	}
	virtual void initExpects() override
	{
		expects.push_back("abbbccd");
	}
	virtual void calc(vector<Player>& players) override
	{
        tempScore[0] = 0.5;
        if (optionCount[0] > 0) {
            tempScore[1] = 1;
        }
        if ((optionCount[0] + optionCount[1]) * 2 >= playerNum) {
            tempScore[2] = 1.5;
        }
        if (optionCount[3] == 1) {
            tempScore[3] = 2;
        }
	}
};

class Q37 : public Question
{
public:
	Q37()
	{
		id = 37;
		author = "纤光";
		title = "魔镜";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
		options.push_back("恰有两人选此项，你们的分数互换");
		options.push_back("恰有一人选此项，将你的分数取绝对值");
        options.push_back("恰有一人选此项，在结算完上述两个选项后，分数最高者-2");
		options.push_back("+0.5");
	}
	virtual void initExpects() override
	{
		expects.push_back("abcddddd");
	}
	virtual void calc(vector<Player>& players) override
	{
        if (optionCount[0] == 2) {
            Player* p = nullptr;
			for (int i = 0; i < playerNum; i++) {
                if (players[i].select != 0) {
                    continue;
                }
                if (p == nullptr) {
                    p = &players[i];
                } else {
                    std::swap(p->score, players[i].score);
                }
            }
        }
        if (optionCount[1] == 1) {
			for (int i = 0; i < playerNum; i++) {
                if (players[i].select == 1 && players[i].score < 0) {
                    players[i].score *= -1;
                }
            }
        }
        if (optionCount[2] == 1) {
            const double max_score = MaxPlayerScore(players);
            for (auto& player : players) {
                if (player.score == max_score) {
                    player.score -= 2;
                }
            }
        }
        tempScore[3] = 0.5;
	}
};

class Q38 : public Question
{
public:
	Q38()
	{
		id = 38;
		author = "纤光";
		title = "丘比特之箭";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
		options.push_back("恰有两人选此项，+2，否则-1");
		options.push_back("+0.5，结算完成后，不考虑其他选项的结算，若你是分数最高的人，再-2");
		options.push_back("若恰有两个选项选择人数相同，+2");
	}
	virtual void initExpects() override
	{
		expects.push_back("abc");
	}
	virtual void calc(vector<Player>& players) override
	{
        tempScore[0] = optionCount[0] == 2 ? 2 : -1;
        for (auto& player : players) {
            if (player.select == 1) {
                player.score += 0.5;
            }
        }
        const double max_score = MaxPlayerScore(players);
        for (auto& player : players) {
            if (player.select == 1 && player.score == max_score) {
                player.score -= 2;
            }
        }
        if (optionCount[0] == optionCount[1] || optionCount[1] == optionCount[2] || optionCount[0] == optionCount[2]) {
            tempScore[2] = 2;
        }
	}
};

class Q39 : public Question
{
public:
	Q39()
	{
		id = 39;
		author = "纤光";
		title = "台风";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
        options.push_back("-4");
        options.push_back("-3，选此项的人多于4人时，改为-6");
		options.push_back("-2，选此项的人多于3人时，改为-6");
		options.push_back("-1，选此项的人多于2人时，改为-6");
	}
	virtual void initExpects() override
	{
		expects.push_back("aabbbbcccdd");
	}
	virtual void calc(vector<Player>& players) override
	{
        tempScore[0] = -4;
		tempScore[1] = optionCount[1] > 4 ? -6 : -3;
		tempScore[2] = optionCount[2] > 3 ? -6 : -2;
		tempScore[3] = optionCount[3] > 2 ? -6 : -1;
	}
};

class Q40 : public Question
{
public:
	Q40()
	{
		id = 40;
		author = "纤光";
		title = "大乐透";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
        options.push_back("头奖：当BD没有人选且AC项都有人选时，+4");
        options.push_back("二等奖：当仅一人选此项时，+2");
        options.push_back("三等奖：当无人选A 时，+1");
        options.push_back("安慰奖：+0.5");
	}
	virtual void initExpects() override
	{
		expects.push_back("abcccdddd");
	}
	virtual void calc(vector<Player>& players) override
	{
        if (optionCount[0] > 0 && optionCount[1] == 0 && optionCount[2] > 0 && optionCount[3] == 0) {
            tempScore[0] = 4;
        }
        if (optionCount[1] == 1) {
            tempScore[1] = 2;
        }
        if (optionCount[0] == 0) {
            tempScore[2] = 1;
        }
        tempScore[3] = 0.5;
	}
};

class Q41 : public Question
{
public:
	Q41()
	{
		id = 41;
		author = "纤光";
		title = "狼人杀";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
        options.push_back("平民：若无人选此项，所有狼人+1");
        options.push_back("狼人：当狼人最多时，所有狼人+1");
        options.push_back("预言家：当狼人最多时，+2");
        options.push_back("女巫：恰有一人选此项，所有狼人-2");
        options.push_back("猎人：-2，此外恰有一人选此项，本题所有得分取反");
	}
	virtual void initExpects() override
	{
		expects.push_back("abbbbccccde");
	}
	virtual void calc(vector<Player>& players) override
	{
        if (optionCount[0] == 0) {
            tempScore[1] += 1;
        }
        const bool wolf_is_max_count = optionCount[1] == *std::ranges::max_element(optionCount);
        if (wolf_is_max_count) {
            tempScore[1] += 1;
            tempScore[2] = 2;
        }
        if (optionCount[3] == 1) {
            tempScore[1] -= 2;
        }
        tempScore[4] = -2;
        if (optionCount[4] == 1) {
            for (auto& temp_score : tempScore) {
                temp_score *= -1;
            }
        }
	}
};

class Q42 : public Question
{
public:
	Q42()
	{
		id = 42;
		author = "大梦我先觉";
		title = "饭堂的斗争";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("学校的午饭时间到了，请选择下列一项。");
	}
	virtual void initOptions() override
	{
        options.push_back("饭堂阿姨：+2，若人数多于B则改为-2");
        options.push_back("吃饭堂：-0.5，若人数为最多则改为+1.5");
        options.push_back("吃外卖：-1，然后平分选择A人数的积分");
	}
	virtual void initExpects() override
	{
		expects.push_back("aabbbc");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = optionCount[0] > optionCount[1] ? -2 : 2;
		tempScore[1] = optionCount[1] == maxSelect ? 1.5 : -0.5;
		tempScore[2] = -1 + optionCount[0] / optionCount[2];
	}
};

class Q43 : public Question
{
public:
	Q43()
	{
		id = 43;
		author = "大梦我先觉";
		title = "出差方式的选择";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		if (playerNum > 5) {
			vars["win1"] = 8;
			vars["win2"] = 5;
			vars["B"] = playerNum / 2 - 0.5;
		} else {
			vars["win1"] = 6;
			vars["win2"] = 3;
			vars["B"] = playerNum / 2 + 0.5;
		}
		vars["C"] = round(playerNum / 3);
		texts.push_back("最先到达的队伍平分 " + str(vars["win1"]) + " 分");
		texts.push_back("第二到达的队伍平分 " + str(vars["win2"]) + " 分");
		texts.push_back("最后到达的队伍视为迟到，每人-1分");
		texts.push_back("如果有队伍同时到达，则按照 飞机→拼车→高铁 排列");
	}
	virtual void initOptions() override
	{
        options.push_back("高铁：2h，每多一人时间增加0.5h");
        options.push_back("拼车：" + str(vars["B"]) + "h，每多一人时间减少0.5h");
        options.push_back("飞机：1h，若人数小于 " + str(vars["C"]) + " 则不出发");
	}
	virtual void initExpects() override
	{
		expects.push_back("abbbcc");
	}
	virtual void calc(vector<Player>& players) override
	{
		vector<int> time = {17, int(vars["B"]*10+6), 10};
		time[0] += optionCount[0] * 5;
		time[1] -= optionCount[1] * 5;
		time[2] = (optionCount[2] < vars["C"]) ? 999 : time[2];
		for (int i = 0; i < 3; i++) {
			if (optionCount[i] == 0) {
				time[i] = 999;
			}
		}
		auto minmax = minmax_element(time.begin(), time.end());
		int win1 = distance(time.begin(), minmax.first);
		int lose = distance(time.begin(), minmax.second);
		int win2 = 3 - win1 - lose;

		tempScore[win1] = vars["win1"] / optionCount[win1];
		tempScore[win2] = vars["win2"] / optionCount[win2];
		tempScore[lose] = -1;
	}
};

class Q44 : public Question
{
public:
	Q44()
	{
		id = 44;
		author = "大梦我先觉";
		title = "串联与并联";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("物理学上电路的串联和并联分别意味着社会层面的合作和竞争，请选择你的身份。");
	}
	virtual void initOptions() override
	{
		if (playerNum <= 10) {
			vars["A"] = 6;
			vars["B"] = 4;
		} else {
			vars["A"] = 8;
			vars["B"] = 5;
		}
        options.push_back("串联：2人及以上选择时平分 " + str(vars["A"]) + " 分。");
        options.push_back("并联：获得 2<sup>" + str(vars["B"]) + "-N</sup> 分（N为选择此选项的人数）");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaabb");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = optionCount[0] >= 2 ? vars["A"] / optionCount[0] : 0;
		tempScore[1] = pow(2, vars["B"] - optionCount[1]);
	}
};

class Q45 : public Question
{
public:
	Q45()
	{
		id = 45;
		author = "大梦我先觉";
		title = "助学金";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("僧丫助学金风波再起，请选择你的身份");
	}
	virtual void initOptions() override
	{
        options.push_back("真贫困生：-1，若人数小于等于 D 时改为+2");
        options.push_back("假贫困生：+1.5，若人数大于等于 D 则改为-2");
        options.push_back("小康生：+0.5");
		options.push_back("资助者：每个资助者都能资助大于等于2个贫困生时，+3分");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaabbbccd");
	}
	virtual void calc(vector<Player>& players) override
	{
        tempScore[0] = optionCount[0] <= optionCount[3] ? 2 : -1;
		tempScore[1] = optionCount[1] >= optionCount[3] ? -2 : 1.5;
		tempScore[2] = 0.5;
		tempScore[3] = (optionCount[0] + optionCount[1]) / optionCount[3] >= 2 ? 3 : 0;
	}
};

class Q46 : public Question
{
public:
	Q46()
	{
		id = 46;
		author = "H3PO4";
		title = "和平的道路";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择一条通往和平的道路。");
	}
	virtual void initOptions() override
	{
        options.push_back("相互理解：如果人数多于B，+3；否则-2");
        options.push_back("武力威慑：+1");
	}
	virtual void initExpects() override
	{
		expects.push_back("aabbb");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = optionCount[0] > optionCount[1] ? 3 : -2;
		tempScore[1] = 1;
	}
};

class Q47 : public Question
{
public:
	Q47()
	{
		id = 47;
		author = "大梦我先觉";
		title = "争议的考试";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("正所谓，大学一堆水课线上课。考试的时候你发现周围的同学正在作弊。你选择");
	}
	virtual void initOptions() override
	{
        options.push_back("举报：+3，但额外失去 [选择 C 选项人数/2] 的分数");
        options.push_back("效仿：-1，但额外获得 [选择 C 选项人数] 的分数");
        options.push_back("漠视：+1");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaabbbcc");
	}
	virtual void calc(vector<Player>& players) override
	{
        tempScore[0] = 3 - optionCount[2] / 2;
		tempScore[1] = -1 + optionCount[2];
		tempScore[2] = 1;
	}
};

class Q48 : public Question
{
public:
	Q48()
	{
		id = 48;
		author = "神户小德";
		title = "网络博弈";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("有个见光死的小圈子，如果 乐子人 人数超过 圈内管理 则出圈被封，请在下列选项中选择一个");
	}
	virtual void initOptions() override
	{
        options.push_back("圈内人：+1.5，如果圈子被封不加分");
        options.push_back("圈内管理：+2，如果圈子被封改为-1");
        options.push_back("乐子人：+1");
		options.push_back("审核：如果圈子被封，+2；反之-2");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaabbccddd");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = 1.5;
		tempScore[1] = 2;
		tempScore[2] = 1;
		tempScore[3] = -2;
		if (optionCount[2] > optionCount[1]) {
			tempScore[0] = 0;
			tempScore[1] = -1;
			tempScore[3] = 2;
		}
	}
};

class Q49 : public Question
{
public:
	Q49()
	{
		id = 49;
		author = "FishToucher";
		title = "人与群分";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("按照人数最多的一组执行");
	}
	virtual void initOptions() override
	{
        options.push_back("乐善好施：本组每人+1分，人数最少的一组每人+3分");
        options.push_back("人人平等：所有人获得1分");
		options.push_back("众志成城：如果本组人数大于等于其他组的总和，每人+3分；否则每人-3分");
		options.push_back("雨露均沾：本组每人+2分，其他组每组平分3分");
		options.push_back("反向赌博：本组每人-5分");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaabcddde");
	}
	virtual void calc(vector<Player>& players) override
	{
		if (optionCount[0] == maxSelect) {
			tempScore[0] += 1;
			for (int i = 0; i < 5; i++) {
				if (optionCount[i] == nonZero_minSelect) {
					tempScore[i] += 3;
				}
			}
		}
		if (optionCount[1] == maxSelect) {
			for (int i = 0; i < 5; i++) {
				tempScore[i] += 1;
			}
		}
		if (optionCount[2] == maxSelect) {
			if (optionCount[2] >= optionCount[0] + optionCount[1] + optionCount[3] + optionCount[4]) {
				tempScore[2] += 3;
			} else {
				tempScore[2] -= 3;
			}
		}
		if (optionCount[3] == maxSelect) {
			tempScore[3] += 2;
			tempScore[0] += 3 / optionCount[0];
			tempScore[1] += 3 / optionCount[1];
			tempScore[2] += 3 / optionCount[2];
			tempScore[4] += 3 / optionCount[4];
		}
		if (optionCount[4] == maxSelect) {
			tempScore[4] -= 5;
		}
	}
};

class Q50 : public Question
{
public:
	Q50()
	{
		id = 50;
		author = "H3PO4";
		title = "流浪地球";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择一项身份。");
	}
	virtual void initOptions() override
	{
        options.push_back("飞船派：如果人数在有人选择的选项里最少，+4");
        options.push_back("地球派：如果选择人数不是最少，+3，然后让选择 A 的玩家+1");
        options.push_back("反叛军：-1，如果选择人数比A和B都多，使选A和B的玩家-4");
		options.push_back("数字生命：获得 [A+B-C] 人数的分数");
	}
	virtual void initExpects() override
	{
		expects.push_back("abbcccdd");
	}
	virtual void calc(vector<Player>& players) override
	{
        if (optionCount[0] == nonZero_minSelect) {
			tempScore[0] += 4;
		}
		if (optionCount[1] > minSelect) {
			tempScore[1] += 3;
			tempScore[0] += 1;
		}
		tempScore[2] = -1;
		if (optionCount[2] > optionCount[1] && optionCount[2] > optionCount[0]) {
			tempScore[0] -= 4;
			tempScore[1] -= 4;
		}
		tempScore[3] = optionCount[0] + optionCount[1] - optionCount[2];
	}
};

class Q51 : public Question
{
public:
	Q51()
	{
		id = 51;
		author = "大梦我先觉";
		title = "名著的价值";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("四大名著之一的红楼梦，至今续本已佚，若你当时获得了她，你作何选择？");
	}
	virtual void initOptions() override
	{
        options.push_back("保存：+3，然后获得 [C-B] 人数的分数");
        options.push_back("上交朝廷：+1.5");
        options.push_back("销毁：平分 X 分（X为选择B选项的人数）");
	}
	virtual void initExpects() override
	{
		expects.push_back("abbbcc");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = 3 + optionCount[2] - optionCount[1];
		tempScore[1] = 1.5;
		tempScore[2] = optionCount[1] / optionCount[2];
	}
};

class Q52 : public Question
{
public:
	Q52()
	{
		id = 52;
		author = "纸团OvO";
		title = "乌合之众";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择一项，人数最多的一项额外-2分（如有并列则一起减）");
	}
	virtual void initOptions() override
	{
        options.push_back("+1");
        options.push_back("+0");
		options.push_back("-0.4");
	}
	virtual void initExpects() override
	{
		expects.push_back("aabbbc");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = 1;
		tempScore[2] = -0.4;
		for (int i = 0; i < 3; i++) {
			if (optionCount[i] == maxSelect) {
				tempScore[i] -= 2;
			}
		}
	}
};

class Q53 : public Question
{
public:
	Q53()
	{
		id = 53;
		author = "神户小德";
		title = " 战争爆发";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		vector<double> tmp_score;
		int large_med_count = 0;
		for (int i = 0; i < playerNum; i++) {
			tmp_score.push_back(players[i].score);
		}
		sort(tmp_score.begin(),tmp_score.end());
		vars["med"] = int(tmp_score[int(playerNum / 2 + 1)]);
		for (int i = 0; i < playerNum; i++) {
			if (players[i].score >= vars["med"]) {
				large_med_count++;
			}
		}
		vars["limit"] = large_med_count / 2 + 1;
		texts.push_back("战争爆发，你的策略是？");
	}
	virtual void initOptions() override
	{
        options.push_back("和平：+1。但如果核武器研制成功，改为-1");
        options.push_back("人海战术：+2。但如果核武器研制成功，改为-3");
        options.push_back("研究核武器：如果当前分数达到 " + str(vars["med"]) + " 的玩家至少有 " + str(vars["limit"]) + " 人选择此选项，则研制成功，所有选择此选项的玩家+1，否则-2");
		options.push_back("投降：-0.5");
	}
	virtual void initExpects() override
	{
		expects.push_back("aabcccdd");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = 1;
		tempScore[1] = 2;
		tempScore[2] = -2;
		tempScore[3] = -0.5;
		int count = 0;
		for(int i = 0; i < playerNum; i++) {
			if (players[i].score >= vars["med"] && players[i].select == 2) {
				count++;
			}
		}
		if (count >= vars["limit"]) {
			tempScore[0] = -1;
			tempScore[1] = -3;
			tempScore[2] = 1;
		}
	}
};

class Q54 : public Question
{
public:
	Q54()
	{
		id = 54;
		author = "saiwei";
		title = "空中楼阁";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("如果你盖的楼能盖的起来，不是空中楼阁，则你加所盖楼层的分数，否则你扣所盖楼层的分数。");
	}
	virtual void initOptions() override
	{
        options.push_back("盖1楼");
        options.push_back("盖2楼");
		options.push_back("盖3楼");
		options.push_back("盖4楼");
	}
	virtual void initExpects() override
	{
		expects.push_back("abcccdd");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = 1;
		tempScore[1] = -2;
		tempScore[2] = -3;
		tempScore[3] = -4;
		if (optionCount[0]) {
			tempScore[1] = 2;
			if (optionCount[1]) {
				tempScore[2] = 3;
				if (optionCount[2]) {
					tempScore[3] = 4;
				}
			}
		}
	}
};

class Q55 : public Question
{
public:
	Q55()
	{
		id = 55;
		author = "铁蛋";
		title = "天下无贼";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		vars["num"] = int(playerNum / 3);
		texts.push_back("选择一个身份。");
	}
	virtual void initOptions() override
	{
        options.push_back("民：如果没有警，-2");
        options.push_back("贼：+2");
		options.push_back("警：如果选贼的人数不少于 " + str(vars["num"]) + "：警+1，并使选贼的玩家改为-2；否则警-2");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaabbcc");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = optionCount[2] == 0 ? -2 : 0;
		tempScore[1] = 2;
		tempScore[2] = -2;
		if (optionCount[2] > 0 && optionCount[1] >= vars["num"]) {
			tempScore[2] = 1;
			tempScore[1] = -2;
		}
	}
};

class Q56 : public Question
{
public:
	Q56()
	{
		id = 56;
		author = "H3PO4";
		title = "电车难题";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("所有人被绑在了电车轨道上，每个人身边有一个拉杆。");
		texts.push_back("必须有人拉下拉杆才能停下电车，否则 【所有人分数归0】！。");
	}
	virtual void initOptions() override
	{
        options.push_back("拉下拉杆：+3，但如果人数大于等于一半，改为-3");
        options.push_back("不拉拉杆：+0");
	}
	virtual void initExpects() override
	{
		expects.push_back("aabbb");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = optionCount[0] >= playerNum / 2 ? -3 : 3;
		if (optionCount[0] == 0) {
			for (int i = 0; i < playerNum; i++) {
				players[i].score = 0;
			}
		}
	}
};

class Q57 : public Question
{
public:
	Q57()
	{
		id = 57;
		author = "圣墓上的倒吊人";
		title = "市场";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择选项的数字最大的玩家减去相应的分数，其他人加所选的选项分数。");
	}
	virtual void initOptions() override
	{
        options.push_back("0");
		options.push_back("1");
        options.push_back("2");
		options.push_back("3");
		options.push_back("+4，如果只有一人选此项，则改为仅有此选项+4。");
	}
	virtual void initExpects() override
	{
		expects.push_back("aabbbccde");
	}
	virtual void calc(vector<Player>& players) override
	{
		if (optionCount[4] == 1) {
			tempScore[4] = 4;
		} else {
			bool minus_point = true;
			for (int i = 4; i > 0; i--) {
				if (optionCount[i] > 0 && minus_point) {
					tempScore[i] = -i;
					minus_point = false;
				} else {
					tempScore[i] = i;
				}
			}
		}
	}
};

class Q58: public Question
{
public:
	Q58()
	{
		id = 58;
		author = "西二旗四惠东";
		title = "Who is max?";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("如果A人数最多，AB两项得分互换；如果C人数最多，CD两项得分互换。");
	}
	virtual void initOptions() override
	{
		options.push_back("+3");
		options.push_back("-3");
		options.push_back("+1");
		options.push_back("-1");
	}
	virtual void initExpects() override
	{
		expects.push_back("aabbcd");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = 3;
		tempScore[1] = -3;
		tempScore[2] = 1;
		tempScore[3] = -1;
		if (optionCount[0] == maxSelect) {
			tempScore[0] = -3;
			tempScore[1] = 3;
		}
		if (optionCount[2] == maxSelect) {
			tempScore[2] = -1;
			tempScore[3] = 1;
		}
	}
};

class Q59 : public Question
{
public:
	Q59()
	{
		id = 59;
		author = "妙蛙种子";
		title = "同步麻将";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("你的选择是？");
	}
	virtual void initOptions() override
	{
        options.push_back("役满：如果没有人选择B，+3.2");
        options.push_back("速攻：+0.5，如果有人选择A，改为+1.5");
		options.push_back("平进：+1，如果B的人数最多，则B改为-1");
		options.push_back("九种九牌：-4，如果任何人选择此项，则使其他选项无效；但如果有人选择C，此选项分数改为+2");
		options.push_back("天选之人：1.6%的概率获得64分");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaabbbccdeeee");
	}
	virtual void calc(vector<Player>& players) override
	{
		if (optionCount[3] == 0){
			if (optionCount[1] == 0) {
				tempScore[0] = 3.2;
			}
			tempScore[1] = 0.5;
			if (optionCount[0] > 0) {
				tempScore[1] = 1.5;
			}
			tempScore[2] = 1;
			if (optionCount[2] > 0 && optionCount[1] == maxSelect) {
				tempScore[1] = -1;
			}
			if (rand() % 1000 < 16) {
				tempScore[4] = 64;
			}
		} else {
			tempScore[3] = -4;
			if (optionCount[2] > 0) {
				tempScore[3] = 2;
			}
		}
	}
};

class Q60 : public Question
{
public:
	Q60()
	{
		id = 60;
		author = "剩菜剩饭";
		title = "多数服从少数";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("最少的一项执行效果（不包括0人），若存在并列则最多的一项执行效果，仍有并列则只有D+7");
	}
	virtual void initOptions() override
	{
        options.push_back("B+2分，C+1分");
        options.push_back("A-2分");
		options.push_back("-1分，B-3分");
		options.push_back("A+3分，B和C+2分");
	}
	virtual void initExpects() override
	{
		expects.push_back("aabbbcd");
	}
	virtual void calc(vector<Player>& players) override
	{
		int minIndex = 0;
		int maxIndex = 0;
		bool multipleMinSelect = false;
		bool multipleMaxSelect = false;
		for (int i = 1; i < options.size(); i++) {
			if (optionCount[i] < optionCount[minIndex] && optionCount[i] > 0) {
				minIndex = i;
				multipleMinSelect = false;
			} else if (optionCount[i] == optionCount[minIndex]) {
				multipleMinSelect = true;
			}
			if (optionCount[i] > optionCount[maxIndex]) {
				maxIndex = i;
				multipleMaxSelect = false;
			} else if (optionCount[i] == optionCount[maxIndex]) {
				multipleMaxSelect = true;
			}
		}

		int win;
		if (!multipleMinSelect) {
			win = minIndex;
		} else if (!multipleMaxSelect) {
			win = maxIndex;
		} else {
			win = -1;
		}
		switch (win) {
			case 0:
				tempScore[1] = 2;
				tempScore[2] = 1;
				break;
			case 1:
				tempScore[0] = -2;
				break;
			case 2:
				tempScore[2] = -1;
				tempScore[1] = -3;
				break;
			case 3:
				tempScore[0] = 3;
				tempScore[1] = 2;
				tempScore[2] = 2;
				break;
			case -1: tempScore[3] = 7;
		}
	}
};

class Q61 : public Question
{
public:
	Q61()
	{
		id = 61;
		author = "九九归一";
		title = "一人一个";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("下列每个选项只有一人选择时生效");
	}
	virtual void initOptions() override
	{
		vars["num"] = playerNum > 20 ? 20 : playerNum;
		for (int i = 0; i < vars["num"]; i++) {
        	options.push_back("+" + str(i * 0.5 + 0.5));
		}
	}
	virtual void initExpects() override
	{
		string expect = "";
		for (int i = 0; i < vars["num"]; i++) {
        	expect += 'a' + i;
		}
		expects.push_back(expect);
	}
	virtual void calc(vector<Player>& players) override
	{
		for (int i = 0; i < vars["num"]; i++) {
        	if (optionCount[i] == 1) {
				tempScore[i] = i * 0.5 + 0.5;
			}
		}
	}
};

class Q62 : public Question
{
public:
	Q62()
	{
		id = 62;
		author = "圣墓上的倒吊人";
		title = "碰碰车";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("只有一个玩家选择的最小选项+2。若有大于等于3个玩家选择同一选项，扣除 [此选项人数*0.5] 分。");
	}
	virtual void initOptions() override
	{
		if (playerNum < 5) {
			vars["num"] = 4;
		} else {
			vars["num"] = round(4 + (playerNum - 5) / 2);
		}
		for (int i = 1; i <= vars["num"]; i++) {
			options.push_back(to_string(i) + "号");
		}
	}
	virtual void initExpects() override
	{
		string expect = "";
		for (int i = 0; i < vars["num"]; i++) {
        	expect += 'a' + i;
		}
		expects.push_back(expect);
	}
	virtual void calc(vector<Player>& players) override
	{
		bool min = true;
		for (int i = 0; i < vars["num"]; i++) {
			if (optionCount[i] == 1 && min) {
				tempScore[i] = 2;
				min = false;
			}
			if (optionCount[i] > 2) {
				tempScore[i] = -optionCount[i] * 0.5;
			}
		}
	}
};

class Q63: public Question
{
public:
	Q63()
	{
		id = 63;
		author = "Li2CO3";
		title = "寻宝猎人";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("如果某个怪物选项被2+人选择，则每种怪物均分 [A选项人数] 的分数，且A改为-1。");
	}
	virtual void initOptions() override
	{
		vars["leave"] = ceil(playerNum / 3);
		options.push_back("继续：均分 " + str(playerNum) + " 分");
		options.push_back("撤离：均分 " + str(vars["leave"]) + " 分，除不尽的部分无法被均分");
		options.push_back("地狱犬（怪物）：-0.5");
		options.push_back("美杜莎（怪物）：-1");
		options.push_back("不死鸟（怪物）：-1.5");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaaabbccdde");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = playerNum / optionCount[0];
		tempScore[1] = int(vars["leave"] / optionCount[1]);
		tempScore[2] = -0.5;
		tempScore[3] = -1;
		tempScore[4] = -1.5;
		if (optionCount[2] >= 2 || optionCount[3] >= 2 || optionCount[4] >= 2) {
			tempScore[2] += optionCount[0] / optionCount[2];
			tempScore[3] += optionCount[0] / optionCount[3];
			tempScore[4] += optionCount[0] / optionCount[4];
			tempScore[0] = -1;
		}
	}
};

class Q64: public Question
{
public:
	Q64()
	{
		id = 64;
		author = "飞机鸭卖蛋";
		title = "我逃避";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
		options.push_back("+2，若人数最多则改为-1");
		options.push_back("+1，若人数最多则改为-1");
		options.push_back("0");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaabbc");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = optionCount[0] == maxSelect ? -1 : 2;
		tempScore[1] = optionCount[1] == maxSelect ? -1 : 1;
	}
};

class Q65 : public Question
{
public:
	Q65()
	{
		id = 65;
		author = "铁蛋";
		title = "面包危机";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("获取：可以获得对应的分数，但如果被抢夺，分值会变为相反数");
		texts.push_back("抢夺：如果有人获取相同的分数，抢夺成功，获得抢夺的分数；失败则失去对应的分数");
	}
	virtual void initOptions() override
	{
        options.push_back("获取 1");
        options.push_back("获取 2");
		options.push_back("获取 3");
		options.push_back("抢夺 1");
		options.push_back("抢夺 2");
		options.push_back("抢夺 3");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaabbcdeeff");
	}
	virtual void calc(vector<Player>& players) override
	{
		for (int i = 0; i < 3; i++) {
			tempScore[i] = i + 1;
			if (optionCount[i] > 0 && optionCount[i+3] > 0) {
				tempScore[i+3] = tempScore[i];
				tempScore[i] = -tempScore[i];
			} else {
				tempScore[i+3] = -tempScore[i];
			}
		}
	}
};

class Q66: public Question
{
public:
	Q66()
	{
		id = 66;
		author = "西二旗四惠东";
		title = "有人吗？";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("你选择一个数字X。如果人数大于0的选项至少有X个，你+X分。");
	}
	virtual void initOptions() override
	{
		vars["num"] = int(playerNum / 2) + 2;
		for (int i = 1; i <= vars["num"]; i++) {
			options.push_back(to_string(i));
		}
	}
	virtual void initExpects() override
	{
		string expect = "";
		for (int i = 0; i < vars["num"]; i++) {
        	expect += 'a' + i;
		}
		expects.push_back(expect);
	}
	virtual void calc(vector<Player>& players) override
	{
		int count = 0;
		for (int i = 0; i < vars["num"]; i++) {
        	if (optionCount[i] > 0) {
				count++;
			}
		}
		for (int i = 0; i < vars["num"]; i++) {
        	if (count >= i + 1) {
				tempScore[i] = i + 1;
			}
		}
	}
};

class Q67: public Question
{
public:
	Q67()
	{
		id = 67;
		author = "西二旗四惠东";
		title = "螳螂捕蝉";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
		options.push_back("+0");
		options.push_back("如果人数少于A，+2；否则-1");
		options.push_back("如果人数少于B，+4；否则-2");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaabbc");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[1] = optionCount[1] < optionCount[0] ? 2 : -1;
		tempScore[2] = optionCount[2] < optionCount[1] ? 4 : -2;
	}
};

class Q68: public Question
{
public:
	Q68()
	{
		id = 68;
		author = "齐齐";
		title = "高考与工作";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
		options.push_back("高考：-1，然后均分 " + str(playerNum) + " 分");
		options.push_back("工作：+1");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaabb");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = -1 + playerNum / optionCount[0];
		tempScore[1] = 1;
	}
};

// ——————————测试题目——————————

class Q69 : public Question   // [待修改]存在50%概率影响
{
public:
	Q69()
	{
		id = 69;
		author = "齐齐";
		title = "月下人狼";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
		options.push_back("村人：如果 村人+狂人＞人狼+妖狐，+1，否则-2");
		options.push_back("人狼：如果村人扣分，+1，否则-2");
		options.push_back("狂人：如果村人扣分，+4，否则-2");
		options.push_back("妖狐：50%概率+2，50%概率-2（所有妖狐得分情况相同）");
	}
	virtual void initExpects() override
	{
		expects.push_back("aabccddd");
	}
	virtual void calc(vector<Player>& players) override
	{
		if (optionCount[0] + optionCount[2] > optionCount[1] + optionCount[3]) {
			tempScore[0] = 1;
		} else {
			tempScore[0] = -2;
		}
		if (tempScore[0] < 0) {
			tempScore[1] = 1;
			tempScore[2] = 4;
		} else {
			tempScore[1] = -2;
			tempScore[2] = -2;
		}
		if (rand() % 2 == 0) {
			tempScore[3] = 2;
		} else {
			tempScore[3] = -2;
		}
	}
};

class Q70 : public Question   // [待修改]选项不平衡，选A居多
{
public:
	Q70()
	{
		id = 70;
		author = "匿名";
		title = "选举";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择人数最多的选项生效，且该选项的玩家额外+1.5。若平票，优先结算序号在后面的选项。");
	}
	virtual void initOptions() override
	{
        options.push_back("分最高的人-3，分最低的人+2，其他人+1");
        options.push_back("分最高的人+3，负分的玩家分数加至0");
		options.push_back("选择该项的玩家增加等同于人数的分数，40%概率改为减少");
		options.push_back("0");
	}
	virtual void initExpects() override
	{
		expects.push_back("aabbbccd");
	}
	virtual void calc(vector<Player>& players) override
	{
		if (optionCount[3] == maxSelect) {
			tempScore[3] += 1.5;
		} else if (optionCount[2] == maxSelect) {
			tempScore[2] = optionCount[2];
			if (rand() % 10 < 4) {
				tempScore[2] = -tempScore[2];
			}
			tempScore[2] += 1.5;
		} else if (optionCount[1] == maxSelect) {
			for (int i = 0; i < playerNum; i++) {
				if (players[i].score == maxScore) {
					players[i].score += 3;
				}
				if (players[i].score < 0) {
					players[i].score = 0;
				}
			}
			tempScore[1] += 1.5;
		} else if (optionCount[0] == maxSelect) {
			for (int i = 0; i < playerNum; i++) {
				if (players[i].score == maxScore) {
					players[i].score -= 3;
				} else if (players[i].score == minScore) {
					players[i].score += 2;
				} else {
					players[i].score += 1;
				}
			}
			tempScore[0] += 1.5;
		}
	}
};

class Q71 : public Question   // 备选题目   平衡 4
{
public:
	Q71()
	{
		id = 71;
		author = "xiaogt";
		title = "投资";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		vars["total"] = round(playerNum * 1.6);
		texts.push_back("请选择一项投资，若总投资达到 " + str(vars["total"]) + " 则投资成功，每人返还 3.5 积分，投资金额最多的选项额外返还 2 积分。你选择：");
	}
	virtual void initOptions() override
	{
        options.push_back("拒绝投资");
        options.push_back("投资 1 积分");
		options.push_back("投资 2 积分");
		options.push_back("投资 3 积分");
		options.push_back("投资 4 积分");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaabbbccdde");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = 0;
		tempScore[1] = -1;
		tempScore[2] = -2;
		tempScore[3] = -3;
		tempScore[4] = -4;
		if (optionCount[1] + optionCount[2]*2 + optionCount[3]*3 + optionCount[4]*4 >= vars["total"]) {
			int max = 0;
			for (int i = 1; i < 5; i++) {
				tempScore[i] += 3.5;
				if (optionCount[i] > 0) {
					max = i;
				}
			}
			tempScore[max] += 2;
		}
	}
};

class Q72 : public Question   // [待修改]人多时分数增减幅度太大
{
public:
	Q72()
	{
		id = 72;
		author = "圣墓上的倒吊人";
		title = "税收";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
        options.push_back("减45%分数");
        options.push_back("减 [B选项人数] 的分数");
		options.push_back("减 [C选项人数*1.5] 的分数");
	}
	virtual void initExpects() override
	{
		expects.push_back("abbbcc");
	}
	virtual void calc(vector<Player>& players) override
	{
		for (int i = 0; i < playerNum; i++) {
			if (players[i].select == 0) {
				if (players[i].score > 0) {
					players[i].score -= players[i].score * 0.45;
				} else {
					players[i].score += players[i].score * 0.45;
				}
			}
		}
		tempScore[1] = -optionCount[1];
		tempScore[2] = -optionCount[2] * 1.5;
	}
};

class Q73 : public Question   // [待修改]人多时分数增减幅度太大
{
public:
	Q73()
	{
		id = 73;
		author = "圣墓上的倒吊人";
		title = "云顶之巢";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
        options.push_back("速攻：如果没有中速，+3");
        options.push_back("中速：+1.5");
		options.push_back("贪贪贪：如果没有速攻，+5；反之-3");
	}
	virtual void initExpects() override
	{
		expects.push_back("abbbcc");
	}
	virtual void calc(vector<Player>& players) override
	{
		if (optionCount[1] == 0) {
			tempScore[0] = 3;
		}
		tempScore[1] = 1.5;
		if (optionCount[0] == 0) {
			tempScore[2] = 5;
		} else {
			tempScore[2] = -3;
		}
	}
};

class Q74 : public Question   // [待定]暂时无法判断选项平衡性    平衡 1
{
public:
	Q74()
	{
		id = 74;
		author = "Chance";
		title = "未命名";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择下面一项，人数最多和最少的选项都成立");
	}
	virtual void initOptions() override
	{
        options.push_back("A+1，B+2");
        options.push_back("A-2，C-3");
		options.push_back("C+2，D+2");
		options.push_back("D-1，B-2");
	}
	virtual void initExpects() override
	{
		expects.push_back("aabcccd");
	}
	virtual void calc(vector<Player>& players) override
	{
		if (optionCount[0] == maxSelect || optionCount[0] == minSelect) {
			tempScore[0] += 1;
			tempScore[1] += 2;
		}
		if (optionCount[1] == maxSelect || optionCount[1] == minSelect) {
			tempScore[0] -= 2;
			tempScore[2] -= 3;
		}
		if (optionCount[2] == maxSelect || optionCount[2] == minSelect) {
			tempScore[2] += 2;
			tempScore[3] += 2;
		}
		if (optionCount[3] == maxSelect || optionCount[3] == minSelect) {
			tempScore[3] -= 1;
			tempScore[1] -= 2;
		}
	}
};

class Q75 : public Question   // 备选题目    平衡 1
{
public:
	Q75()
	{
		id = 75;
		author = "Chance";
		title = "未命名";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择下面一项，人数最少的一项成立");
	}
	virtual void initOptions() override
	{
        options.push_back("B+2");
        options.push_back("C-0.5");
		options.push_back("D-1");
		options.push_back("A+1");
	}
	virtual void initExpects() override
	{
		expects.push_back("aabbbccd");
	}
	virtual void calc(vector<Player>& players) override
	{
		if (optionCount[0] == minSelect) {
			tempScore[1] += 2;
		}
		if (optionCount[1] == minSelect) {
			tempScore[2] -= 0.5;
		}
		if (optionCount[2] == minSelect) {
			tempScore[3] -= 1;
		}
		if (optionCount[3] == minSelect) {
			tempScore[0] += 1;
		}
	}
};

class Q76 : public Question   // [待修改]题目过于复杂
{
public:
	Q76()
	{
		id = 76;
		author = "纤光";
		title = "平行时空";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("有一项选择人数唯一最多时，此项生效");
		texts.push_back("有两项选择人数同时最多时，第三项生效");
		texts.push_back("有三项选择人数同时最多时，均不生效，所有人分数清零。");
	}
	virtual void initOptions() override
	{
        options.push_back("若有人选B，选C的人+1");
        options.push_back("若有人选C，选A的人+2");
		options.push_back("若有人选A，选B的人+3");
	}
	virtual void initExpects() override
	{
		expects.push_back("abc");
	}
	virtual void calc(vector<Player>& players) override
	{
		int maxCount = 0;
		int win[2];
		for (int i = 0; i < 3; i++) {
			if (optionCount[i] == maxSelect) {
				maxCount++;
				win[0] = i;
			} else {
				win[1] = i;
			}
		}
		if (maxCount == 3) {
			for (int i = 0; i < playerNum; i++) {
				players[i].score = 0;
			}
		} else {
			if (win[maxCount - 1] == 0 && optionCount[1] > 0) {
				tempScore[2] = 1;
			}
			if (win[maxCount - 1] == 1 && optionCount[2] > 0) {
				tempScore[0] = 2;
			}
			if (win[maxCount - 1] == 2 && optionCount[0] > 0) {
				tempScore[1] = 3;
			}
		}
	}
};

class Q77 : public Question   // [待修改]题目过于复杂
{
public:
	Q77()
	{
		id = 77;
		author = "纤光";
		title = "差值投标";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择人数最多的两项对战（多项相同时取靠近A的两项），对战者中靠近A的称作败者，靠近D的称作胜者。另外两项与败者得分相同但额外-0.5。");
		texts.push_back("败者和胜者选项分别生效，随后胜者+4。");
	}
	virtual void initOptions() override
	{
        options.push_back("-0");
        options.push_back("-2");
		options.push_back("-4");
		options.push_back("-6");
	}
	virtual void initExpects() override
	{
		expects.push_back("abcd");
	}
	virtual void calc(vector<Player>& players) override
	{
		vector<double> optionSort = optionCount;
		vector<int> orig_indexes(optionCount.size());
		iota(orig_indexes.begin(), orig_indexes.end(), 0);
		partial_sort(orig_indexes.begin(), orig_indexes.begin() + 2, orig_indexes.end(), [&optionSort](int i, int j) {
			return optionSort[i] > optionSort[j];
		});

		int o1 = orig_indexes[0];
		int o2 = orig_indexes[1];
		if (o1 > o2) swap(o1, o2);
		tempScore[o1] = -o1 * 2;
		tempScore[o2] = -o2 * 2 + 4;
		for (int i = 0; i < 4; i++) {
			if (i != o1 && i != o2) {
				tempScore[i] = tempScore[o1] - 0.5;
			}
		}
	}
};

class Q78 : public Question   // [测试]随机性太高
{
public:
	Q78()
	{
		id = 78;
		author = "Chance";
		title = "我是大土块";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
		if (playerNum > 8) {
			vars["percent"] = 10;
		} else {
			vars["percent"] = 16;
		}
        options.push_back("+2，有 [(B+C) *" + str(vars["percent"]) + "]% 的概率-4");
        options.push_back("+1，有 [(A-B) *" + str(vars["percent"]) + "]% 的概率使C+2");
		options.push_back("-2，有 [(A+D) *" + str(vars["percent"]) + "]% 的概率+6");
		options.push_back("-3，有 [(A+B-D) *" + str(vars["percent"]) + "]% 的概率使A和B-4，D+5");
	}
	virtual void initExpects() override
	{
		expects.push_back("abcd");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = 2;
		tempScore[1] = 1;
		tempScore[2] = -2;
		tempScore[3] = -3;
		if (rand() % 100 < (optionCount[1] + optionCount[2]) * vars["percent"]) {
			tempScore[0] -= 4;
		}
		if (rand() % 100 < (optionCount[0] - optionCount[1]) * vars["percent"]) {
			tempScore[2] += 2;
		}
		if (rand() % 100 < (optionCount[0] + optionCount[3]) * vars["percent"]) {
			tempScore[2] += 6;
		}
		if (rand() % 100 < (optionCount[0] + optionCount[1] - optionCount[3]) * vars["percent"]) {
			tempScore[0] -= 4;
			tempScore[1] -= 4;
			tempScore[2] += 5;
		}
	}
};

class Q79 : public Question   // [待修改]分数增减幅度太大    平衡 1
{
public:
	Q79()
	{
		id = 79;
		author = "圣墓上的倒吊人";
		title = "资源配置";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		vars["start"] = playerNum * 2;
		vars["limit"] = int(playerNum * 3.2);
		texts.push_back("桌上有 " + str(vars["start"]) + " 个资源，一资源一分，如果投入后总资源超过了 " + str(vars["limit"]) + " ，那获得（你的减分/所有玩家的减分）比例的总资源。如果投入后分数为负数，则选择无效。");
	}
	virtual void initOptions() override
	{
        options.push_back("0");
        options.push_back("-1");
		options.push_back("-2");
		options.push_back("-4");
		options.push_back("-6");
		options.push_back("1.6%获得投入后总资源，否则-15");
	}
	virtual void initExpects() override
	{
		expects.push_back("abbcccddddeef");
	}
	virtual void calc(vector<Player>& players) override
	{
		int total = vars["start"];
		total = total + optionCount[1] * 1 + optionCount[2] * 2 + optionCount[3] * 4 + optionCount[4] * 6;
		tempScore[1] = -1;
		tempScore[2] = -2;
		tempScore[3] = -4;
		tempScore[4] = -6;
		for (int i = 0; i < playerNum; i++) {
			if (players[i].score < -tempScore[players[i].select]) {
				total += tempScore[players[i].select];
				players[i].score -= tempScore[players[i].select];
			}
		}
		if (rand() % 1000 < 16) {
			tempScore[5] = total;
		} else {
			if (total > vars["limit"]) {
				for (int i = 0; i < playerNum; i++) {
					if (players[i].score >= -tempScore[players[i].select]) {
						players[i].score += -tempScore[players[i].select] / (total - vars["start"]) * total;
					}
				}
			}
			tempScore[5] = -15;
		}
	}
};

class Q80 : public Question   // [待修改]选项不平衡，选D居多
{
public:
	Q80()
	{
		id = 80;
		author = "飘渺";
		title = "电车难题";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("请选择你的位置。拉杆只有2个状态，即拉动偶数次会还原拉杆");
	}
	virtual void initOptions() override
	{
        options.push_back("乘客：+0.5，但每有一个人拉动拉杆-0.5");
        options.push_back("多数派：+0.5，如果拉杆最终没有拉动，改为-2");
		options.push_back("少数派：+0.5，如果拉杆最终拉动，改为-2.5");
		options.push_back("-0.5，每一个选择此项的人都会拉动一次拉杆");
	}
	virtual void initExpects() override
	{
		expects.push_back("abbcdddd");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = 0.5 - optionCount[3] * 0.5;
		tempScore[1] = 0.5;
		if (int(optionCount[3]) % 2 == 0) {
			tempScore[1] = -2;
		}
		tempScore[2] = 0.5;
		if (int(optionCount[3]) % 2 == 1) {
			tempScore[2] = -2.5;
		}
		tempScore[3] = -0.5;
	}
};

class Q81 : public Question   // [待修改]分数增减幅度太大
{
public:
	Q81()
	{
		id = 81;
		author = "Q群管家";
		title = "均分1";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
        options.push_back("当前总积分×0.5");
        options.push_back("选择此选项的玩家均分他们的分数");
		options.push_back("当前总积分×（-0.5）");
	}
	virtual void initExpects() override
	{
		expects.push_back("abbbbbc");
	}
	virtual void calc(vector<Player>& players) override
	{
		double sum = 0;
		for (int i = 0; i < playerNum; i++) {
        	if (players[i].select == 1) {
				sum += players[i].score;
			}
		}
		for (int i = 0; i < playerNum; i++) {
        	if (players[i].select == 0) {
				players[i].score = players[i].score * 0.5;
			}
			if (players[i].select == 1) {
				players[i].score = sum / optionCount[1];
			}
			if (players[i].select == 2) {
				players[i].score = -players[i].score * 0.5;
			}
		}
	}
};

class Q82 : public Question   // [待修改]选择性问题，大部分情况玩家可能单选B
{
public:
	Q82()
	{
		id = 82;
		author = "Q群管家";
		title = "均分2";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
        options.push_back("选择此选项的分数最高与最低的玩家均分他们的分数");
        options.push_back("正分的-2，负分的+2");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaabb");
	}
	virtual void calc(vector<Player>& players) override
	{
		double maxS = -99999;
		double minS = 99999;
		for(int i = 0; i < playerNum; i++)
		{
			if (players[i].select == 0) {
				maxS = max(maxS, players[i].score);
				minS = min(minS, players[i].score);
			}
		}
		double sum = 0;
		int count = 0;
		for(int i = 0; i < playerNum; i++)
		{
			if (players[i].select == 0 && (players[i].score == minS || players[i].score == maxS)) {
				sum += players[i].score;
				count++;
			}
		}
		for (int i = 0; i < playerNum; i++) {
        	if (players[i].select == 0 && (players[i].score == minS || players[i].score == maxS)) {
				players[i].score = sum / count;
			}
			if (players[i].select == 1) {
				if (players[i].score > 0) {
					players[i].score -= 2;
				}
				if (players[i].score < 0) {
					players[i].score += 2;
				}
			}
		}
	}
};

class Q83 : public Question   // [测试]奇偶存在较大的运气因素
{
public:
	Q83()
	{
		id = 83;
		author = "Q群管家";
		title = "奇偶2";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
        options.push_back("+3");
        options.push_back("-1，若此选项选择人数为奇数，A变为-3");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaab");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = int(optionCount[1]) % 2 == 1 ? -3 : 3;
		tempScore[1] = -1;
	}
};

class Q84 : public Question   // [测试]奇偶存在较大的运气因素，多人时分数增减幅度太大
{
public:
	Q84()
	{
		id = 84;
		author = "Q群管家";
		title = "奇偶5";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
        options.push_back("若选择人数为奇数，扣除 [选择本选项人数/2] 的分数；否则获得相应的分数");
        options.push_back("0");
	}
	virtual void initExpects() override
	{
		expects.push_back("aab");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = int(optionCount[0]) % 2 == 1 ? -optionCount[0] / 2 : optionCount[0] / 2;
	}
};

class Q85 : public Question   // [待修改]分数增减幅度太大
{
public:
	Q85()
	{
		id = 85;
		author = "胡扯弟";
		title = "云顶之巢";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择你的云巢身份。");
	}
	virtual void initOptions() override
	{
        options.push_back("YAMI：总是能+2分的大佬，但是有5%的可能性失手-1");
        options.push_back("飘渺：10%+24的赌博爱好者");
		options.push_back("黑桃3：+13，但每有一个飘渺-1.75分");
		options.push_back("飞机：+12，但有YAMI就会被gank");
		options.push_back("西东：+A+B-C+D");
		options.push_back("胡扯：-114514，但有2.5%的可能性改为+1919810");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaaabbbccdeeeef");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = rand() % 100 < 5 ? -1 : 2;
		tempScore[1] = rand() % 100 < 10 ? 24 : 0;
		tempScore[2] = 13 - optionCount[1] * 1.75;
		tempScore[3] = optionCount[0] == 0 ? 12 : 0;
		tempScore[4] = optionCount[0] + optionCount[1] - optionCount[2] + optionCount[3];
		tempScore[5] = rand() % 1000 < 25 ? 1919810 : -114514;
	}
};

class Q86 : public Question   // [待修改]触发E选项取反概率太高
{
public:
	Q86()
	{
		id = 86;
		author = "圣墓上的倒吊人";
		title = "未来计划";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("（X）代表有且只有 X 人选择该选项时，该选项才能执行。执行顺序为序号顺序。无选项执行时执行E");
	}
	virtual void initOptions() override
	{
        options.push_back("+4（1）");
        options.push_back("+2（2）");
		options.push_back("-3（3）");
		options.push_back("-1（≥2）");
		options.push_back("所有玩家的分数变为相反数（4）");
	}
	virtual void initExpects() override
	{
		expects.push_back("abbbcdee");
	}
	virtual void calc(vector<Player>& players) override
	{
		for(int i = 0; i < playerNum; i++) {
			if (players[i].select == 0 && optionCount[0] == 1) {
				players[i].score += 4;
			}
			if (players[i].select == 1 && optionCount[1] == 2) {
				players[i].score += 2;
			}
			if (players[i].select == 2 && optionCount[2] == 3) {
				players[i].score -= 3;
			}
			if (players[i].select == 3 && optionCount[3] >= 2) {
				players[i].score -= 1;
			}
		}
		if ((optionCount[0] != 1 && optionCount[1] != 2 && optionCount[2] != 3 && optionCount[3] < 2) || optionCount[4] == 4) {
			for(int i = 0; i < playerNum; i++) {
				players[i].score = -players[i].score;
			}
		}
	}
};

class Q87 : public Question   // [待定]题目阅读时间较长
{
public:
	Q87()
	{
		id = 87;
		author = "圣墓上的倒吊人";
		title = "坐等反转";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("甲 / 乙，被选中的选项执行乙，否则执行甲。选项从前往后结算。");
	}
	virtual void initOptions() override
	{
        options.push_back("选择人数最少的选项+2 / 选择人数最多的选项-2");
        options.push_back("使 D 选项无效化 / 0");
		options.push_back("D-2 / D+2");
		options.push_back("所有玩家均分自己的分数 / -1");
	}
	virtual void initExpects() override
	{
		expects.push_back("aabbcd");
	}
	virtual void calc(vector<Player>& players) override
	{
		for(int i = 0; i < playerNum; i++) {
			if (optionCount[0] == 0 && optionCount[players[i].select] == minSelect) {
				players[i].score += 2;
			}
			if (optionCount[0] > 0 && optionCount[players[i].select] == maxSelect) {
				players[i].score -= 2;
			}
			if (optionCount[2] == 0 && players[i].select == 3) {
				players[i].score -= 2;
			}
			if (optionCount[2] > 0 && players[i].select == 3) {
				players[i].score += 2;
			}
		}
		if (optionCount[1] > 0) {
			if (optionCount[3] == 0) {
				double sum = 0;
				for(int i = 0; i < playerNum; i++) {
					sum += players[i].score;
				}
				for(int i = 0; i < playerNum; i++) {
					players[i].score = sum / playerNum;
				}
			} else {
				tempScore[3] -= 1;
			}
		}
	}
};

class Q88 : public Question   // [待定]积分不够分配时，A选项玩家得分太高
{
public:
	Q88()
	{
		id = 88;
		author = "圣墓上的倒吊人";
		title = "报销";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		vars["point"] = playerNum * 1.5;
		texts.push_back("场上有 " + str(vars["point"]) + " 的积分，玩家从中获得和选项数值一致的积分，如果积分不够分配至所有玩家，选 A 的玩家平分所有积分");
	}
	virtual void initOptions() override
	{
        options.push_back("0");
        options.push_back("1");
		options.push_back("2");
		options.push_back("3");
		options.push_back("4");
	}
	virtual void initExpects() override
	{
		expects.push_back("aabccddde");
	}
	virtual void calc(vector<Player>& players) override
	{
		double sum = optionCount[1]*1 + optionCount[2]*2 + optionCount[3]*3 + optionCount[4]*4;
		if (sum <= vars["point"]) {
			tempScore[1] = 1;
			tempScore[2] = 2;
			tempScore[3] = 3;
			tempScore[4] = 4;
		} else {
			tempScore[0] = vars["point"] / optionCount[0];
		}
	}
};

class Q89 : public Question   // 备选题目   平衡 1
{
public:
	Q89()
	{
		id = 89;
		author = "圣墓上的倒吊人";
		title = "奇珍异宝";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择选项的玩家平分选项对应的分数");
	}
	virtual void initOptions() override
	{
        options.push_back("珍珠手链（2）");
        options.push_back("银质餐具（3）");
		options.push_back("国王宝球（5）");
		options.push_back("奇怪的书（选择该选项的玩家数的平方-2）");
	}
	virtual void initExpects() override
	{
		expects.push_back("abbccccdd");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = 2 / optionCount[0];
		tempScore[1] = 3 / optionCount[1];
		tempScore[2] = 5 / optionCount[2];
		tempScore[3] = (optionCount[3] * optionCount[3] - 2) / optionCount[3];
	}
};

class Q90 : public Question   // [待定]暂时无法判断选项平衡性
{
public:
	Q90()
	{
		id = 90;
		author = "圣墓上的倒吊人";
		title = "站位";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("赢的玩家+2，输的玩家-2。");
		texts.push_back("同时胜利或无人胜利时B胜利，AC输");
	}
	virtual void initOptions() override
	{
		vars["num"] = int(playerNum / 2);
        options.push_back("B人数大于 " + str(vars["num"]) + " 则胜利");
        options.push_back("中立");
		options.push_back("A人数大于B则胜利，否则输");
	}
	virtual void initExpects() override
	{
		expects.push_back("aabbbc");
	}
	virtual void calc(vector<Player>& players) override
	{
		bool a_win = optionCount[1] > vars["num"];
		bool c_win = optionCount[0] > optionCount[1];
		if (a_win && !c_win) {
			tempScore[0] = 2;
			tempScore[2] = -2;
		} else if (!a_win && c_win) {
			tempScore[0] = -2;
			tempScore[2] = 2;
		} else {
			tempScore[0] = -2;
			tempScore[1] = 2;
			tempScore[2] = -2;
		}
	}
};

class Q91 : public Question   // [待修改]题目过于复杂
{
public:
	Q91()
	{
		id = 91;
		author = "纸团OvO";
		title = "差值投标";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择人数最多的两项+0.5分，且该两项进行对弈。（如果存在并列，仅选中选项靠前的）");
		texts.push_back("双方的筹码等级将决定两方的加分（筹码大的在右边）：");
		texts.push_back("多1级时 0:2");
		texts.push_back("多2级时 0:0");
		texts.push_back("多3级时 2:0");
	}
	virtual void initOptions() override
	{
        options.push_back("奴隶流：0级筹码");
        options.push_back("草民流：1级筹码");
		options.push_back("市民流：2级筹码");
		options.push_back("皇帝流：3级筹码");
		options.push_back("悠悠流：0或1级筹码（当对手是奴隶流时为1级，否则为0级）");
	}
	virtual void initExpects() override
	{
		expects.push_back("abcde");
	}
	virtual void calc(vector<Player>& players) override
	{
		vector<double> optionSort = optionCount;
		vector<int> orig_indexes(optionCount.size());
		iota(orig_indexes.begin(), orig_indexes.end(), 0);
		partial_sort(orig_indexes.begin(), orig_indexes.begin() + 2, orig_indexes.end(), [&optionSort](int i, int j) {
			return optionSort[i] > optionSort[j];
		});
		int o1 = orig_indexes[0];
		int o2 = orig_indexes[1];
		if (o1 > o2) swap(o1, o2);

		tempScore[o1] = 0.5;
		tempScore[o2] = 0.5;
		int level1 = o1;
		int level2 = o2;
		if (level2 == 4) {
			if (level1 == 0 || level1 == 3) {
				tempScore[o2] += 2;
			} else if(level1 == 1){
				tempScore[o1] += 2;
			}
		} else {
			if (abs(level2 - level1) == 1) {
				tempScore[o2] += 2;
			}
			if (abs(level2 - level1) == 3) {
				tempScore[o1] += 2;
			}
		}
	}
};

class Q92 : public Question   // [待修改]题目过于复杂   平衡 1
{
public:
	Q92()
	{
		id = 92;
		author = "xiaogt";
		title = "债务危机";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		vars["debt"] = playerNum * 1.5;
		texts.push_back("有 " + str(vars["debt"]) + " 负债需要分摊，玩家每+1分，负债+1，玩家每-1分，负债-1。每轮将以从上往下的顺序结算。若所有选项均结算完成，将开始新一轮结算，直到负债归零。你选择：");
	}
	virtual void initOptions() override
	{
		options.push_back("-1分");
		options.push_back("+1分，第二轮起每轮-3分");
		options.push_back("-2分");
		options.push_back("0分，第二轮起每轮-3分");
		options.push_back("-1.5分，第二轮起每轮+0分");
		options.push_back("-6分，若在第一轮就分摊完毕，改为+1分");
	}
	virtual void initExpects() override
	{
		expects.push_back("aabbbcddeff");
	}
	virtual void calc(vector<Player>& players) override
	{
		int debt = vars["debt"];
		tempScore[0] -= 1;
		tempScore[1] += 1;
		tempScore[2] -= 2;
		tempScore[4] -= 1.5;
		tempScore[5] -= 6;
		debt = debt - optionCount[0] + optionCount[1] - optionCount[2]*2 - optionCount[4]*3 - optionCount[5]*6;
		if (debt <= 0) {
			tempScore[5] = 1;
		}
		int minus[6] = {1, 3, 2, 3, 0, 6};
		while (debt > 0) {
			for (int i = 0; i < 6; i++) {
				tempScore[i] -= minus[i];
				debt -= optionCount[i] * minus[i];
				if (debt <= 0) break;
			}
		}
	}
};

class Q93 : public Question   // [待修改]D选项不好理解，选项不平衡，AD人数居多
{
public:
	Q93()
	{
		id = 93;
		author = "大梦我先觉";
		title = "夺宝奇兵";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
        options.push_back("如果此选项的人数<=3，平分 6 分的宝藏");
        options.push_back("如果此选项的人数>=4，平分 10 分的宝藏");
		options.push_back("地雷：如果仅 1 人选择，使A和B的得分变为相反数，你获得 [A+B] 人数的分数");
		options.push_back("战术弃权：每有一人选择此选项，则使选择A的人数减少一人（最低不低于1），使选择B的人数增加一人");
	}
	virtual void initExpects() override
	{
		expects.push_back("aabbbcd");
	}
	virtual void calc(vector<Player>& players) override
	{
		int gap;
		optionCount[0] -= optionCount[3];
		if (optionCount[0] <= 0) {
			gap = 1 - optionCount[0];
			optionCount[0] = 1;
		}
		optionCount[1] += optionCount[3];
		if (optionCount[0] <= 3) {
			tempScore[0] = 6 / optionCount[0];
		}
		if (optionCount[1] >= 4) {
			tempScore[1] = 10 / optionCount[1];
		}
		if (optionCount[2] == 1) {
			tempScore[0] = -tempScore[0];
			tempScore[1] = -tempScore[1];
			tempScore[2] = optionCount[0] + optionCount[1] - gap;
		}
	}
};

class Q94 : public Question   // 备选题目
{
public:
	Q94()
	{
		id = 94;
		author = "剩菜剩饭";
		title = "未命名";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("人数最多的选项和其下一项改为扣等量的分数（D下面是A）");
		texts.push_back("如果有多个选项并列，仅结算其中分值最大的选项");
	}
	virtual void initOptions() override
	{
        options.push_back("+1");
        options.push_back("+2");
		options.push_back("+3");
		options.push_back("+4");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaaabcccdd");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = 1;
		tempScore[1] = 2;
		tempScore[2] = 3;
		tempScore[3] = 4;
		for (int i = 3; i >= 0; i--) {
			if (optionCount[i] == maxSelect) {
				tempScore[i] = -tempScore[i];
				if (i == 3) {
					tempScore[0] = -tempScore[0];
				} else {
					tempScore[i + 1] = -tempScore[i + 1];
				}
				break;
			}
		}
	}
};

class Q95: public Question   // [待修改]分数增减幅度太大
{
public:
	Q95()
	{
		id = 95;
		author = "齐齐";
		title = "HP杀";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
		options.push_back("杀手：当A>B时，+2，否则-2");
		options.push_back("平民：当B>A时，+2，否则-2");
		options.push_back("内奸：当A=B时，+2，否则-2");
		options.push_back("特工：当A=B时，+10，否则-10");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaaaabbbbbcccd");
	}
	virtual void calc(vector<Player>& players) override
	{
		if (optionCount[0] > optionCount[1]) {
			tempScore[0] = 2;
			tempScore[1] = -2;
			tempScore[2] = -2;
			tempScore[3] = -10;
		} else if (optionCount[0] < optionCount[1]) {
			tempScore[0] = -2;
			tempScore[1] = 2;
			tempScore[2] = -2;
			tempScore[3] = -10;
		} else {
			tempScore[0] = -2;
			tempScore[1] = -2;
			tempScore[2] = 2;
			tempScore[3] = 10;
		}
	}
};

class Q96: public Question   // [待定]暂时无法判断选项平衡性
{
public:
	Q96()
	{
		id = 96;
		author = "Chance";
		title = "未命名";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
		options.push_back("平分 [B选项人数-5绝对值] 的分数");
		options.push_back("平分 [A选项人数-6绝对值] 的分数");
		options.push_back("平分 [人数最多的选项人数] 的分数");
	}
	virtual void initExpects() override
	{
		expects.push_back("aabbbcc");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = abs(optionCount[1] - 5) / optionCount[0];
		tempScore[1] = abs(optionCount[0] - 6) / optionCount[1];
		tempScore[2] = maxSelect / optionCount[2];
	}
};

class Q97: public Question   // [待定]暂时无法判断选项平衡性
{
public:
	Q97()
	{
		id = 97;
		author = "Chance";
		title = "未命名";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("人数大于1的选项在获得分数后-2。");
	}
	virtual void initOptions() override
	{
		options.push_back("+1.25");
		options.push_back("+1.5");
		options.push_back("+1.75");
		options.push_back("+2");
		options.push_back("+2.25");
		options.push_back("+2.5");
		options.push_back("+2.75");
		options.push_back("+3");
	}
	virtual void initExpects() override
	{
		expects.push_back("abcdefgh");
	}
	virtual void calc(vector<Player>& players) override
	{
		double score[8] = {1.25, 1.5, 1.75, 2, 2.25, 2.5, 2.75, 3};
		for (int i = 0; i < 8; i++) {
			tempScore[i] = score[i];
			if (optionCount[i] > 1) {
				tempScore[i] -= 2;
			}
		}
	}
};

class Q98 : public Question   // [待修改]人多时分数增减幅度太大
{
public:
	Q98()
	{
		id = 98;
		author = "圣墓上的倒吊人";
		title = "真理";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("真理掌握在？");
	}
	virtual void initOptions() override
	{
        options.push_back("少数人：+3，如果此项选择人数最多，则改为减 [选择该项人数] 的分数");
        options.push_back("多数人：-2");
	}
	virtual void initExpects() override
	{
		expects.push_back("aabbb");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = optionCount[0] == maxSelect ? -optionCount[0] : 3;
		tempScore[1] = -2;
	}
};

class Q99 : public Question   // [待修改]分数增减幅度太大
{
public:
	Q99()
	{
		id = 99;
		author = "丘陵萨满";
		title = "伊甸园";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		vector<double> tmp_score;
		for (int i = 0; i < playerNum; i++) {
			tmp_score.push_back(players[i].score);
		}
		sort(tmp_score.begin(),tmp_score.end());
		vars["score"] = tmp_score[2];
		texts.push_back("获胜的选项获得 [选择非此选项人数] 的分数，失败的选项失去 [选择获胜选项人数] 的分数");
	}
	virtual void initOptions() override
	{
        options.push_back("红苹果：如果分数小于等于 " + str(vars["score"]) + " 的玩家都选择红苹果，红苹果获胜。但如果所有人都选择此项，全员分数取反");
        options.push_back("银苹果：如果选择此项的人数少于 C，且红苹果没有获胜，银苹果获胜。");
		options.push_back("金苹果：如果选择此项的人数少于等于 B，且红苹果没有获胜，金苹果获胜。");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaaabc");
	}
	virtual void calc(vector<Player>& players) override
	{
		int win, l1, l2;
		bool red_win = true;
		bool is_invert = false;
		win = l1 = l2 = 0;
		for (int i = 0; i < playerNum; i++) {
			if (players[i].score <= vars["score"] && players[i].select != 0) {
				red_win = false; break;
			}
		}
		if (red_win) {
			if (optionCount[0] == playerNum) {
				is_invert = true;
				for (int i = 0; i < playerNum; i++) {
					players[i].score = -players[i].score;
				}
			} else {
				win = 0;
				l1 = 1; l2 = 2;
			}
		} else {
			if (optionCount[1] < optionCount[2]) {
				win = 1;
				l1 = 0; l2 = 2;
			} else {
				win = 2;
				l1 = 0; l2 = 1;
			}
		}
		if (!is_invert) {
			tempScore[win] = optionCount[l1] + optionCount[l2];
			tempScore[l1] = -optionCount[win];
			tempScore[l2] = -optionCount[win];
		}
	}
};

class Q100 : public Question   // [待修改]A选项不平衡，无人选择
{
public:
	Q100()
	{
		id = 100;
		author = "xiaogt";
		title = "怪物糖果";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		vars["point"] = playerNum * 2.5;
		texts.push_back("有 " + str(vars["point"]) + " 分可供瓜分，你可以选择拿取的分数，然后从小到大进行拿取操作。但当剩余分数不够某个选项拿取时，该组及更高分组-1分。");
	}
	virtual void initOptions() override
	{
        options.push_back("1分");
        options.push_back("2分");
		options.push_back("3分");
		options.push_back("4分");
	}
	virtual void initExpects() override
	{
		expects.push_back("aabcccdd");
	}
	virtual void calc(vector<Player>& players) override
	{
		int need[4] = {1, 2, 3, 4};
		int point = vars["point"];
		for(int i = 0; i < 4; i++)
		{
			if(point >= optionCount[i] * need[i])
			{
				point -= optionCount[i] * need[i];
				tempScore[i] = need[i];
			} else {
				tempScore[i] -= 1;
				for (int j = i; j < 4; j++) {
					need[j] -= 1;
				}
			}
		}
	}
};

class Q101: public Question   // [待定]对于为玩过伊甸园的玩家读题略有困难
{
public:
	Q101()
	{
		id = 101;
		author = "九九归一";
		title = "伊甸园";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择下列一项：");
	}
	virtual void initOptions() override
	{
		options.push_back("金苹果：若选择此项的人数大于选择B的人数，则平分 [未选择此项的人数] 分，否则减1分。");
		options.push_back("银苹果：若选择此项的人数大于选择A的人数，则平分 [未选择此项的人数] 分，否则减1分。");
		options.push_back("红苹果：若选择A的人数等于选择B的人数，则平分 [未选择此项的人数] 分，否则减1分。");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaabbc");
	}
	virtual void calc(vector<Player>& players) override
	{
		int win, l1, l2;
		if (optionCount[0] > optionCount[1]) {
			win = 0;
			l1 = 1; l2 = 2;
		} else if (optionCount[1] > optionCount[0]) {
			win = 1;
			l1 = 0; l2 = 2;
		} else {
			win = 2;
			l1 = 0; l2 = 1;
		}
		tempScore[win] = (optionCount[l1] + optionCount[l2]) / optionCount[win];
		tempScore[l1] = tempScore[l2] = -1;
	}
};

class Q102: public Question   // [待定]清0仍需考虑
{
public:
	Q102()
	{
		id = 102;
		author = "剩菜剩饭";
		title = "寻宝猎人";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
		vars["num"] = playerNum * 2 - 1;
		options.push_back("宝藏：平分 " + str(vars["num"]) + " 分");
		options.push_back("珍宝：如果选择人数小于等于3，+2");
		options.push_back("奇遇：选择A，B的玩家减 [选择C的人数] 的分数");
		options.push_back("怪物：+1分，如果没有人选本项，选C的玩家分数归0");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaaabbccd");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = vars["num"] / optionCount[0];
		if (optionCount[1] <= 3) {
			tempScore[1] = 2;
		}
		tempScore[0] -= optionCount[2];
		tempScore[1] -= optionCount[2];
		tempScore[3] = 1;
		if (optionCount[3] == 0) {
			for(int i = 0; i < playerNum; i++) {
				if (players[i].select == 2) {
					players[i].score = 0;
				}
			}
		}
	}
};

class Q103 : public Question   // [待修改]反向执行的结算容易引发歧义
{
public:
	Q103()
	{
		id = 103;
		author = "栗子";
		title = "利他之众";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选的人数最多的执行，最少（包括0人）反向执行");
	}
	virtual void initOptions() override
	{
        options.push_back("B每人1分");
        options.push_back("C平分3分");
		options.push_back("AD平分3分");
		options.push_back("AC平分3分");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaabccdd");
	}
	virtual void calc(vector<Player>& players) override
	{
		if (optionCount[0] == maxSelect) {
			tempScore[1] += 1;
		}
		if (optionCount[1] == maxSelect) {
			tempScore[2] += 3 / optionCount[2];
		}
		if (optionCount[2] == maxSelect) {
			tempScore[0] += 3 / optionCount[0];
			tempScore[3] += 3 / optionCount[3];
		}
		if (optionCount[3] == maxSelect) {
			tempScore[0] += 3 / optionCount[0];
			tempScore[2] += 3 / optionCount[2];
		}

		if (optionCount[0] == minSelect) {
			tempScore[1] -= 1;
		}
		if (optionCount[1] == minSelect) {
			tempScore[2] -= 3 / optionCount[2];
		}
		if (optionCount[2] == minSelect) {
			tempScore[0] -= 3 / optionCount[0];
			tempScore[3] -= 3 / optionCount[3];
		}
		if (optionCount[3] == minSelect) {
			tempScore[0] -= 3 / optionCount[0];
			tempScore[2] -= 3 / optionCount[2];
		}
	}
};

class Q104 : public Question   // [待修改]分数增减幅度太大，A选项风险太高
{
public:
	Q104()
	{
		id = 104;
		author = "圣墓上的倒吊人";
		title = "战争中的贵族";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("你的国家与敌国正在战争中，选择一项行动。");
	}
	virtual void initOptions() override
	{
        options.push_back("继续抵抗：+5，如果有玩家选择了B，那选择此项的人分数归零");
        options.push_back("有条件投降：-3");
		options.push_back("润出国外：如果有人选 A，则-2");
	}
	virtual void initExpects() override
	{
		expects.push_back("aabccccc");
	}
	virtual void calc(vector<Player>& players) override
	{
		if (optionCount[1] == 0) {
			tempScore[0] = 5;
		} else {
			for (int i = 0; i < playerNum; i++) {
				if (players[i].select == 0) {
					players[i].score = 0;
				}
			}
		}
		tempScore[1] = -3;
		if (optionCount[0] > 0) {
			tempScore[2] = -2;
		}
	}
};

class Q105: public Question   // [待修改]多人时分数增减幅度太大
{
public:
	Q105()
	{
		id = 105;
		author = "飞机鸭卖蛋";
		title = "太多挂机A了…？";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("分别记选择A、B、C选项的人数为a、b、c：");
	}
	virtual void initOptions() override
	{
		options.push_back("获得 a 分");
		options.push_back("获得 a*1.5-b 分");
		options.push_back("获得 a*2-c 分");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaaabbccc");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = optionCount[0];
		tempScore[1] = optionCount[0] * 1.5 - optionCount[1];
		tempScore[2] = optionCount[0] * 2 - optionCount[2];
	}
};

class Q106: public Question   // [待修改]存在50%概率影响
{
public:
	Q106()
	{
		id = 106;
		author = "飞机鸭卖蛋";
		title = "听力练习";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("耳机电量不佳，背书还是听力？");
	}
	virtual void initOptions() override
	{
		options.push_back("背书：+1，噪音值+1");
		options.push_back("耳机标准模式：+3-噪音值");
		options.push_back("耳机降噪模式：+3，但50%概率中途关机，改为+0");
	}
	virtual void initExpects() override
	{
		expects.push_back("aabbbcc");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = 1;
		tempScore[1] = 3 - optionCount[0];
		tempScore[2] = rand() % 2 ? 3 : 0;
	}
};

class Q107: public Question   // [待定]暂时无法判断选项平衡性
{
public:
	Q107()
	{
		id = 107;
		author = "飞机鸭卖蛋";
		title = "有情人终成眷属";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
		options.push_back("若恰有两人选择，-2，否则+2");
		options.push_back("若恰有两人选择，-2，否则+2");
		options.push_back("若选择人数最多，-2，否则+3");
	}
	virtual void initExpects() override
	{
		expects.push_back("aabbccc");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = optionCount[0] == 2 ? -2 : 2;
		tempScore[1] = optionCount[1] == 2 ? -2 : 2;
		tempScore[2] = optionCount[2] == maxSelect ? -2 : 3;
	}
};

class Q108: public Question   // [待定]暂时无法判断选项平衡性
{
public:
	Q108()
	{
		id = 108;
		author = "小黄鸭";
		title = "E卡抉择";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择一项，依次生效。");
	}
	virtual void initOptions() override
	{
		options.push_back("皇帝：+2分");
		options.push_back("市民：若A比C多则-1分；否则+1分。");
		options.push_back("奴隶：若有人选择此项且没有人选择B，则+1分且所有选A的玩家的总得分变为0；否则-1分。");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaabccc");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[1] = optionCount[0] > optionCount[2] ? -1 : 1;
		if (optionCount[2] > 0 && optionCount[1] == 0) {
			tempScore[2] = 1;
			for (int i = 0; i < playerNum; i++) {
				if (players[i].select == 0) {
					players[i].score = 0;
				}
			}
		} else {
			tempScore[0] = 2;
			tempScore[2] = -1;
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

FishToucher (已添加 #49人与群分)
按照人数最多的一组执行
A．	乐善好施（本组每人+1分，人数最少的一组每人+3分）
B．	人人平等（所有人获得1分）
C．	众志成城（如果本组人数大于等于其他组的总和，每人+3分；否则每人-3分）
D．	雨露均沾（本组每人+2分，其他组每组平分3分）
E．	反向赌博（本组每人-5分）


*/ 


#endif
