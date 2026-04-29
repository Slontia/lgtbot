// 本文档仅保留出题会用到的常用函数定义，完整源码请查看`problems.h`

/* 玩家结构 */
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
	
	int lastSelect; // 上回合选择
	int select;     // 本回合选择
	double realLastScore;   // 上回合的分数，calc计算结束后变化
	double score;   // 本回合分数
	
	string getScore()
	{
		return to_string(score);
	}
};

class Question
{
public:
	
	Question()
	{
		vars.clear();
	} 

    virtual ~Question() {};
	
    // 问题相关配置
	int id;
	string author;
	string title; 
	vector<string> texts;
	vector<string> options;
	vector<string> expects;

    // **【重点看这里】**
	// 可以当作全局参数
	map<string,double> vars;
    // 可快速调用的变量
	double playerNum;       // 玩家人数
	double maxScore;        // 最高分数
	double minScore;        // 最低分数
	double medianScore;     // 分数中位数
	vector<double> optionCount;     // 每个选项选择的人数数量
	double maxSelect;       // 选择最多选项的人数
	double minSelect;       // 选择最少选项的人数
	double nonZero_minSelect;   // 选择最少选项的人数（不包括0）
	vector<double> tempScore;   // 记录这个选项可以获得的分数，算分专用
	
	// 各种初始化
	void init(vector<Player>& players);
	virtual void initTexts(vector<Player>& players){}
	virtual void initOptions(){}
	virtual void initExpects(){}
	void initCalc(vector<Player>& players);
    // 图片UI相关（无需使用）
    string Markdown();
    string String();
};

// double保留两位小数
double dec2(double x);
// double转字符串
string str(double x);



/* 您仅需实现如下部分 */
class Q114 : public Question
{
public:
	Q114()
	{
		id = 114;           // 题目ID，和类名相同
		author = "NULL";    // 题目作者名称
		title = "测试题目";  // 题目名称
	}
	
	virtual void initTexts(vector<Player>& players) override    // 题干文本
	{
		texts.push_back("我是第一行文本");
        texts.push_back("我是第二行文本");   // 大部分题目仅需要一行文本，如需多行再添加
	}
	virtual void initOptions() override     // 选项文本
	{
		vars["v1"] = 1.36;	// 将变量保存至vars中，可以在其他函数（如calc）中重复使用
		options.push_back("我是选项【A】，选我获得" + str(vars["v1"]) + "分");
		options.push_back("我是选项【B】，选我获得2.17分");
		// 下方可以增加更多选项（最多26个）
	}
	virtual void initExpects() override     // 电脑伪随机概率
	{
		// 每个电脑玩家都会从下方字符串中随机一个字母
		expects.push_back("abbb");		// 展示的例子为电脑1/4概率选A，3/4概率选B
	}
	virtual void calc(vector<Player>& players) override     // 分数计算实现
	{
		// 基础分数
		// 将选项的得分保存在tempScore中，每个选择该选项的玩家都会按照这个分数变动
		tempScore[0] = vars["v1"];		// A选项得分记为 vars["v1"] （从0编号）
		tempScore[1] = 2.17;			// B选项得分记为 2.17
	}
};



/* 下方为一般题目实现的具体例子和细节说明 */

/* 条件判断（条件类、均分类题目写法例子） */
// Q2：B选项
"战争：如果恰有两名玩家选择这个选项，则+6分。"
// 实现为：
if(optionCount[1] == 2) tempScore[1] = 6;	// optionCount[1]获取B选项的人数

// Q4：E选项
vars["E"] = (int)playerNum / 2 + 1;		// E的分数根据人数动态实现：人数/2+1
"公正：选择这项的人平分 " + str(vars["E"]) + " 分。"
// 实现为：
tempScore[4] = vars["E"] / optionCount[4];	// E选项得分/E选项人数

// Q6
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
// 实现为：
virtual void calc(vector<Player>& players) override
{
	if(optionCount[0] == maxSelect)		// 选项A人数最多
	{
		tempScore[0] -= 1;
		tempScore[1] -= 3;
	}
	if(optionCount[1] == maxSelect)		// 选项B人数最多
	{
		tempScore[1] += 2;
		tempScore[2] += 1;
	}
	if(optionCount[2] == maxSelect)		// 选项C人数最多
	{
		tempScore[2] -= 0.5;
	}
}



/* 关于高级操作（涉及单个玩家的分数检测或变化） */
// Q7：A选项
"均衡：如果有 3 或更多名玩家选择本项，则得分最高的玩家 -3"
// 实现为：
if(optionCount[0] >= 3)		// 当A选项人数大于3时
{
	for(int i = 0; i < playerNum; i++)		// 遍历所有玩家
	{
		if(players[i].score == maxScore)	// 如果玩家的分数等于最高分数
		{
			players[i].score -= 3;			// 这个玩家的分数直接-3
		}
	}
}

// Q21：B选项
"强硬：如果所有玩家全部选择该项，则所有玩家分数取反。"
// 实现为：
if(optionCount[1] == playerNum)		// 如果B选项人数等于玩家总人数（所有人选择此项）
{
	for(int i = 0; i < playerNum; i++)			// 遍历所有玩家
	{
		players[i].score = -players[i].score;	// 玩家分数变为相反数
	}
}

// Q23：A选项
"同化：+1，然后所有选择此项的玩家会均分他们的分数。"
// 实现为：
double sum = 0;
for(int i = 0; i < playerNum; i++)		// 遍历所有玩家
{
	if(players[i].select == 0)			// 如果玩家选择了A选项
	{
		sum += players[i].score;		// 计算玩家的总分
		players[i].score = 0; 			// 先清零此玩家的分数
	}
}
tempScore[0] = 1 + sum / optionCount[0];	// 最终结果存入 tempScore[0]

// Q30：D选项（人数最多时触发）
"摧毁：所有人得分变为 0 。然后你 -30"
// 实现为：
if(optionCount[3] == maxSelect)		// 如果D选项等于人数最多的选项
{
	for(int i = 0; i < playerNum; i++)
	{
		players[i].score = 0;			// 玩家分数清零
		if(players[i].select == 3)		// 如果该玩家选择D选项
			players[i].score -= 30;			// 额外-30分
	}
}


/* 部分特殊题目需要专门针对于题目编写逻辑（离谱题目可以选择先吃灰） */
// Q53
virtual void initTexts(vector<Player>& players) override
{
	int large_med_count = 0;
	vars["med"] = ceil(medianScore);		// C选项分数限制
	for (int i = 0; i < playerNum; i++) {
		if (players[i].score >= vars["med"]) {
			large_med_count++;
		}
	}
	vars["limit"] = large_med_count / 2 + 1;	// C选项人数限制
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
	for(int i = 0; i < playerNum; i++) {		// 统计达标人数
		if (players[i].score >= vars["med"] && players[i].select == 2) {
			count++;
		}
	}
	if (count >= vars["limit"]) {		// 判断C选项是否成功
		tempScore[0] = -1;
		tempScore[1] = -3;
		tempScore[2] = 1;
	}
}





// 感谢您阅读本文档，更多内容请查看题库文件`problems.h`，然后参考已有题目
// 如遇到问题请联系铁蛋
