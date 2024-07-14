// 39题囚徒困境修改前两版备份（目前已移除）
class Q39 : public Question
{
public:
	Q39()
	{
		id = 39;
		author = "纤光";
		title = "囚徒困境";
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
class Q39 : public Question
{
public:
	Q39()
	{
		id = 39;
		author = "纤光";
		title = "囚徒困境";
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

class Qrm01 : public Question
{
public:
	Q45()
	{
		id = 45;
		author = "大梦我先觉";
		title = "战场的厮杀";
	}
	
	virtual void initTexts() override
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

class Qrm02 : public Question
{
public:
	Q58()
	{
		id = 58;
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

class Qrm03 : public Question
{
public:
	Q61()
	{
		id = 61;
		author = "大梦我先觉";
		title = "焦灼的击剑";
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

class Qrm04 : public Question
{
public:
	Q62()
	{
		id = 62;
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

class Qrm05 : public Question
{
public:
	Q64()
	{
		id = 64;
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

class Qrm06 : public Question
{
public:
	Q72()
	{
		id = 72;
		author = "圣墓上的倒吊人";
		title = "让步";
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

class Qrm07 : public Question
{
public:
	Q81()
	{
		id = 81;
		author = "Q群管家";
		title = "奇偶1";
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

class Qrm08 : public Question
{
public:
	Q83()
	{
		id = 83;
		author = "Q群管家";
		title = "奇偶3";
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

class Qrm09 : public Question
{
public:
	Q84()
	{
		id = 84;
		author = "Q群管家";
		title = "奇偶4";
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