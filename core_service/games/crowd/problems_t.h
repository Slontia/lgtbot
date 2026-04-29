// 39题囚徒困境修改前两版备份（目前废弃）
class Qt1 : public Question
{
public:
	Qt1()
	{
		id = 1;
		author = "纤光";
		title = "囚徒困境第1版【废弃】";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
        options.push_back("认罪：-1，但若所有人都认罪，改为分数最高的人分数清零");
        options.push_back("不认罪：-2，但若所有人都不认罪，改为+2");
	}
	virtual void initExpects() override
	{
		expects.push_back("ab");
	}
	virtual void calc(vector<Player>& players) override
	{
        if (optionCount[0] == players.size()) {
            const double max_score = MaxPlayerScore(players);
            for (auto& player : players) {
                if (player.score == max_score) {
                    player.score = 0;
                }
            }
        } else if (optionCount[1] == players.size()) {
            for (auto& player : players) {
                player.score += 2;
            }
        } else {
            tempScore[0] = -1;
            tempScore[1] = -2;
        }
	}
};

class Qt2 : public Question
{
public:
	Qt2()
	{
		id = 2;
		author = "纤光";
		title = "囚徒困境第2版【废弃】";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
        options.push_back("认罪：-1，结算完成后，若你的分数大于选此项的人数，则分数清零");
        options.push_back("不认罪：-2，结算完成后，若你的分数小于选此项的人数，则分数清零");
	}
	virtual void initExpects() override
	{
		expects.push_back("ab");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = -1;
        tempScore[1] = -2;
		for (auto& player : players) {
			if (player.select == 0 && player.score > optionCount[0]) {
				player.score = 1;
			}
			if (player.select == 1 && player.score < optionCount[1]) {
				player.score = 2;
			}
		}
	}
};

class Qt3 : public Question
{
public:
	Qt3()
	{
		id = 3;
		author = "大梦我先觉";
		title = "战场的厮杀【废弃】";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("在战场上，一念之差都会扭转战局。阵营AB对立，CD对立，各阵营人数多的一方获胜，胜利者可以获得 [失败一方人数/2] 的分数");
	}
	virtual void initOptions() override
	{
        options.push_back("劫营");
        options.push_back("守营");
        options.push_back("抢粮");
		options.push_back("守粮");
	}
	virtual void initExpects() override
	{
		expects.push_back("abcd");
	}
	virtual void calc(vector<Player>& players) override
	{
        if (optionCount[0] > optionCount[1]) {
            tempScore[0] = optionCount[1] / 2;
			// tempScore[1] = -0.5;
        }
		if (optionCount[0] < optionCount[1]) {
            tempScore[1] = optionCount[0] / 2;
			// tempScore[0] = -0.5;
        }
		if (optionCount[2] > optionCount[3]) {
            tempScore[2] = optionCount[3] / 2;
			// tempScore[3] = -0.5;
        }
		if (optionCount[2] < optionCount[3]) {
            tempScore[3] = optionCount[2] / 2;
			// tempScore[2] = -0.5;
        }
	}
};

class Qt4 : public Question
{
public:
	Qt4()
	{
		id = 4;
		author = "蔡徐坤";
		title = "鉴宝师和收藏家";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		string half = "";
		if (playerNum > 5) {
			half = "的 一半 ";
		}
		texts.push_back("将选择对应选项玩家数" + half + "分别记为 A B C D");
	}
	virtual void initOptions() override
	{
        options.push_back("鉴定为假：+2D，若D>C，将所有选择A的玩家的积分平分给选择D的玩家，然后选A玩家分数清0");
        options.push_back("鉴定为真：若C>D，你-2，否则你+D");
		options.push_back("带来赝品：若B>A，你+2，否则你-A");
		options.push_back("带来真品：若A>B，你-2，否则你+B");
	}
	virtual void initExpects() override
	{
		expects.push_back("abcd");
	}
	virtual void calc(vector<Player>& players) override
	{
		vector<double> pnum;
		for (int i = 0; i < playerNum; i++) {
			if (playerNum > 5) {
				pnum.push_back(optionCount[i] / 2);
			} else {
				pnum.push_back(optionCount[i]);
			}
		}
		tempScore[0] = 2 * pnum[3];
		if (pnum[3] > pnum[2]){
			double sum_score = 0;
			for (int i = 0; i < playerNum; i++) {
				if (players[i].select == 0) {
					sum_score = sum_score + players[i].score + tempScore[0];
					players[i].score = 0;
				}
			}
			tempScore[0] = 0;
			tempScore[3] = sum_score / optionCount[3];
		}
		if (pnum[2] > pnum[3]) {
			tempScore[1] -= 2;
		} else {
			tempScore[1] += pnum[3];
		}
		if (pnum[1] > pnum[0]) {
			tempScore[2] += 2;
		} else {
			tempScore[2] -= pnum[0];
		}
		if (pnum[0] > pnum[1]) {
			tempScore[3] -= 2;
		} else {
			tempScore[3] += pnum[1];
		}
	}
};

class Qt5 : public Question
{
public:
	Qt5()
	{
		id = 5;
		author = "大梦我先觉";
		title = "焦灼的击剑【废弃】";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("a和b两人击剑时进行进攻和躲闪。a和b的最终行为分别由选择人数最多项确定（如果人数相等分数不变）。");
		texts.push_back("若两人均进攻，则选进攻者-2分；两人均躲闪，则选躲闪者-1分。若一人进攻一人躲闪，则进攻方选进攻者+2，躲闪方分数不变。");
	}
	virtual void initOptions() override
	{
        options.push_back("a选择进攻");
        options.push_back("a选择躲闪");
        options.push_back("b选择进攻");
		options.push_back("b选择躲闪");
	}
	virtual void initExpects() override
	{
		expects.push_back("abcd");
	}
	virtual void calc(vector<Player>& players) override
	{
		int a_action = (optionCount[0] > optionCount[1])? 1 : (optionCount[1] > optionCount[0])? 0 : -1;
		int b_action = (optionCount[2] > optionCount[3])? 1 : (optionCount[3] > optionCount[2])? 0 : -1;
		if (a_action == b_action) {
			if (a_action == 1) {
				tempScore[0] = tempScore[2] = -2;
			} else if (a_action == 0) {
				tempScore[1] = tempScore[3] = -1;
			}
		} else {
			// a无行动时，b攻击>躲闪加分，否则都不加
			if (a_action > b_action && a_action == 1) {
				tempScore[0] = 2;
			}
			if (b_action > a_action && b_action == 1) {
				tempScore[2] = 2;
			}
		}
	}
};

class Qt6 : public Question
{
public:
	Qt6()
	{
		id = 6;
		author = "剩菜剩饭";
		title = "HP杀";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择一项，若死亡则不得分，但仍执行攻击效果");
	}
	virtual void initOptions() override
	{
        options.push_back("大狼：+1分，每有一只小狼+1分。若大狼只有 1 只，神职死亡");
        options.push_back("小狼：+1分，选择的人数比神职多时平民死亡，若大狼存活，额外+1分");
		options.push_back("神职：+1分，选择人数比小狼多时，大狼死亡，此项额外+1分");
		options.push_back("平民：+3分，若选择的人数比神职多，平民死亡");
		options.push_back("第三方：+0分，若狼人方和好人方都有阵亡消息，你+4分 ");
	}
	virtual void initExpects() override
	{
		expects.push_back("aabbbcccddde");
	}
	virtual void calc(vector<Player>& players) override
	{
		bool A_die, B_die, C_die, D_die;
		A_die = B_die = C_die = D_die = false;
		tempScore[0] = 1 + optionCount[1];
		tempScore[1] = 1;
		tempScore[2] = 1;
		tempScore[3] = 3;
		if (optionCount[0] == 1) {
			C_die = true;
		}
		if (optionCount[1] > optionCount[2] || optionCount[3] > optionCount[2]) {
			D_die = true;
		}
		if (optionCount[2] > optionCount[1]) {
			A_die = true;
			tempScore[2] += 1;
		}
		if (A_die) tempScore[0] = 0; else tempScore[1] += 1;
		// if (B_die) tempScore[1] = 0;
		if (C_die) tempScore[2] = 0;
		if (D_die) tempScore[3] = 0;
		if ((A_die || B_die) && (C_die || D_die)) {
			tempScore[4] = 4;
		}
	}
};

class Qt7 : public Question
{
public:
	Qt7()
	{
		id = 7;
		author = "An idle brain";
		title = "未命名";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选择一项。");
	}
	virtual void initOptions() override
	{
        options.push_back("+1，若有人选B，则改为获得 [D-B] 人数的分数");
        options.push_back("+1.5，若有人选C，则改为获得 [A-C] 人数的分数");
		options.push_back("+2，若有人选D，则改为获得 [B-D] 人数的分数");
		options.push_back("+2.5，若有人选A，则改为获得 [C-A] 人数的分数");
	}
	virtual void initExpects() override
	{
		expects.push_back("abbcdd");
	}
	virtual void calc(vector<Player>& players) override
	{
		tempScore[0] = 1;
		if (optionCount[1] > 0) {
			tempScore[0] = optionCount[3] - optionCount[1];
		}
		tempScore[1] = 1.5;
		if (optionCount[2] > 0) {
			tempScore[1] = optionCount[0] - optionCount[2];
		}
		tempScore[2] = 2;
		if (optionCount[3] > 0) {
			tempScore[2] = optionCount[1] - optionCount[3];
		}
		tempScore[3] = 2.5;
		if (optionCount[0] > 0) {
			tempScore[3] = optionCount[2] - optionCount[0];
		}
	}
};

class Qt8 : public Question
{
public:
	Qt8()
	{
		id = 8;
		author = "圣墓上的倒吊人";
		title = "让步【废弃】";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("如果所有人都选择一个选项，那只执行它。否则执行被选的加分最少的选项，且选择该选项玩家+2，其他人-2。");
	}
	virtual void initOptions() override
	{
        options.push_back("+0");
        options.push_back("+1");
		options.push_back("+2");
		options.push_back("+3");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaabbccd");
	}
	virtual void calc(vector<Player>& players) override
	{
		int selected = 0;
		bool add_point = true;
		for (int i = 0; i < 4; i++) {
			if (optionCount[i] > 0) {
				if (add_point) {
					tempScore[i] = i + 2;
					add_point = false;
				} else {
					tempScore[i] = -2;
				}
				selected++;
			}
		}
		if (selected == 1) {
			for (int i = 0; i < 4; i++) {
				if (optionCount[i] > 0) {
					tempScore[i] -= 2;
				}
			}
		}
	}
};

class Qt9 : public Question
{
public:
	Qt9()
	{
		id = 9;
		author = "Q群管家";
		title = "奇偶1【废弃】";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("对应选项选择人数奇数取反，偶数取正。");
	}
	virtual void initOptions() override
	{
        options.push_back("+3");
        options.push_back("0");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaaab");
	}
	virtual void calc(vector<Player>& players) override
	{
		if (int(optionCount[0]) % 2 == 1) {
			tempScore[0] = -3;
		} else {
			tempScore[0] = 3;
		}
	}
};

class Qt10 : public Question
{
public:
	Qt10()
	{
		id = 10;
		author = "Q群管家";
		title = "奇偶3【废弃】";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("选项得分与选择人数之和若为奇数则得分取反。");
	}
	virtual void initOptions() override
	{
        options.push_back("+1");
        options.push_back("+2");
		options.push_back("+3");
	}
	virtual void initExpects() override
	{
		expects.push_back("aaabbc");
	}
	virtual void calc(vector<Player>& players) override
	{
		for (int i = 0; i < 3; i++) {
			if (int(optionCount[i] + i + 1) % 2 == 1) {
				tempScore[i] = -(i + 1);
			} else {
				tempScore[i] = i + 1;
			}
		}
	}
};

class Qt11 : public Question
{
public:
	Qt11()
	{
		id = 11;
		author = "Q群管家";
		title = "奇偶4【废弃】";
	}
	
	virtual void initTexts(vector<Player>& players) override
	{
		texts.push_back("一张正面是+2，背面是-2的卡牌，初始正面向上，最终状态是本题得分，你选择：");
	}
	virtual void initOptions() override
	{
        options.push_back("翻面，你加对应的分数");
        options.push_back("不翻面，你减对应的分数");
	}
	virtual void initExpects() override
	{
		expects.push_back("aab");
	}
	virtual void calc(vector<Player>& players) override
	{
		if (int(optionCount[0]) % 2 == 0) {
			tempScore[0] = 2;
			tempScore[1] = -2;
		} else {
			tempScore[0] = -2;
			tempScore[1] = 2;
		}
	}
};

class Qt12 : public Question   // [待修改]分数增减幅度太大    平衡 1
{
public:
	Qt12()
	{
		id = 12;
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

class Qt13 : public Question   // [待修改]选项不平衡，选D居多
{
public:
	Qt13()
	{
		id = 13;
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

class Qt14 : public Question   // [待修改]分数增减幅度太大
{
public:
	Qt14()
	{
		id = 14;
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

class Qt15 : public Question   // [待修改]选择性问题，大部分情况玩家可能单选B
{
public:
	Qt15()
	{
		id = 15;
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

class Qt16 : public Question   // [测试]奇偶存在较大的运气因素，多人时分数增减幅度太大
{
public:
	Qt16()
	{
		id = 16;
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

class Qt17 : public Question   // [待修改]分数增减幅度太大
{
public:
	Qt17()
	{
		id = 17;
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

class Qt18 : public Question   // [待修改]触发E选项取反概率太高
{
public:
	Qt18()
	{
		id = 18;
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

class Qt19 : public Question   // [待修改]人多时分数增减幅度太大
{
public:
	Qt19()
	{
		id = 19;
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

class Qt20 : public Question   // [待修改]分数增减幅度太大
{
public:
	Qt20()
	{
		id = 20;
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

class Qt21 : public Question   // [待修改]分数增减幅度太大，A选项风险太高
{
public:
	Qt21()
	{
		id = 21;
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

class Qt22: public Question   // [待修改]多人时分数增减幅度太大
{
public:
	Qt22()
	{
		id = 22;
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
		options.push_back("获得 a×1.5-b 分");
		options.push_back("获得 a×2-c 分");
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

class Qt23 : public Question   // [待修改]人多时分数增减幅度太大
{
public:
	Qt23()
	{
		id = 23;
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

class Qt24 : public Question   // [测试]随机性太高
{
public:
	Qt24()
	{
		id = 24;
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
        options.push_back("+2，有 [(B+C) ×" + str(vars["percent"]) + "]% 的概率-4");
        options.push_back("+1，有 [(A-B) ×" + str(vars["percent"]) + "]% 的概率使C+2");
		options.push_back("-2，有 [(A+D) ×" + str(vars["percent"]) + "]% 的概率+6");
		options.push_back("-3，有 [(A+B-D) ×" + str(vars["percent"]) + "]% 的概率使A和B-4，D+5");
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