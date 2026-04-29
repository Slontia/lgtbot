#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <math.h>
#include <numeric>

#include "problems.h"   // 此文件应放在一个文件夹内

REGISTER_QUESTION(125, Q125)
REGISTER_QUESTION(126, Q126)
REGISTER_QUESTION(127, Q127)
REGISTER_QUESTION(128, Q128)
REGISTER_QUESTION(129, Q129)
REGISTER_QUESTION(130, Q130)

int getRandomChoiceBasedOnExpect(const std::string& expect) {
	//std::cout << expect << std::endl;
	int k = std::rand();
	int n = expect.size();
	int randChoice = k % n;
	//std::cout << k << " " << n << " " << randChoice << std::endl;
    return expect[randChoice] - 'a';
}

int main() {
	std::srand(static_cast<unsigned int>(std::time(0)));
	int playerNum;
	std::cout << "请输入玩家人数：";
	if (!(std::cin >> playerNum) || playerNum <= 0) {
		std::cerr << "无效的玩家人数。" << std::endl;
		return 1;
	}
	
	std::cout << "请输入题目号：";
	int questionNum;
	if (!(std::cin >> questionNum) || questionNum < 125 || questionNum > 130) {
		std::cerr << "找不到对应题目。" << std::endl;
		return 1;
	}
	
	std::vector<Player> players;
	Player tempP;
	for (int i = 0; i < playerNum; i++) {
		players.push_back(tempP);
		players[i].lastSelect = -1;
	}

	const int totalRounds = 5;
    for (int round = 1; round <= totalRounds; ++round) {
        Question* q = QuestionFactory::get().create(questionNum);
        if (!q) {
            std::cerr << "找不到对应题目。" << std::endl;
            return 1;
        }
        q->init(players);
        q->initTexts(players);
        q->initOptions();
        q->initExpects();

        std::cout << q->String() << std::endl;

        // 输入是否启用随机模式
        int randomModeInput;
        std::cout << "第 " << round << " 回合，请选择是否启用随机模式（0: 不启用，1: 启用）：";
        std::cin >> randomModeInput;
        bool randomMode = (randomModeInput == 1);
		std::cout << "第 " << round << " 回合，请输入每位玩家的选择（用空格分隔）：\n";
        for (int i = 0; i < playerNum; ++i) {
            if (randomMode) {
                const std::string& expect = q->expects[0];
                players[i].select = getRandomChoiceBasedOnExpect(expect);
                std::cout << "玩家 " << i + 1 << " 随机选择了选项: " << (char)(players[i].select + 'a') << std::endl;
            } else {
                char ch;
                std::cin >> ch;
                if (ch >= 'A' && ch < 'A' + q->options.size()) {
                    players[i].select = ch - 'A';
                } else if (ch >= 'a' && ch < 'a' + q->options.size()) {
                    players[i].select = ch - 'a';
                } else {
                    std::cerr << "无效选项 '" << ch << "'，使用默认A。" << std::endl;
                    players[i].select = 0;
                }
            }
        }

        for (int i = 0; i < playerNum; ++i)
            players[i].lastSelect = players[i].select;
        for (int i = 0; i < playerNum; ++i)
            players[i].realLastScore = players[i].lastScore;
        for (int i = 0; i < playerNum; ++i)
            players[i].lastScore = players[i].score;

        q->initCalc(players);
        q->calc(players);
        q->quickScore(players);

        std::cout << "\n第 " << round << " 回合分数：\n";
        for (int i = 0; i < playerNum; ++i)
            std::cout << i << " " << players[i].score << std::endl;

        delete q;
    }

	std::cout << "游戏结束。" << std::endl;
	return 0;
}
