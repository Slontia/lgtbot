
#include <random>

class Player
{
public:
	Player(PlayerID pid, string playerName, string playerAvatar)
	{
        id = pid;
        name = playerName;
        avatar = playerAvatar;
		current = 0;
        head = 0;
	}

    // 玩家信息
    PlayerID id;
    string name;
    string avatar;

    // 玩家手牌
	vector<int> hand;

    // 本回合出牌
    int current;
    // 持有牛头数
    int head;
	
};

class Table
{
public:
    // 游戏文件夹
    string ResourceDir;
    
    // 玩家数量
    int playerNum;
    // 手牌数量
    int handNum;
    // 牌阵行数
    int lineNum;
    // 每行上限
    int lineLimit;
    // 卡牌倍数数据
    struct cardMultiple {
        int head, multiple;
    } headMultiple[5];

    // 牌阵数据
    vector<vector<int>> tableStatus;
    

    // 初始化游戏
    void Initialize(const char* dir, const int num, const int hand, const int lines, const int limit, const vector<int> multiple)
    {
        ResourceDir = dir;
        playerNum = num;
        handNum = hand;
        lineNum = lines;
        lineLimit = limit;
        tableStatus.resize(lineNum);
        const int head_num[5] = {2, 3, 5, 7, 10};
        for (int i = 0; i < 5; i++) {
            headMultiple[i].head = head_num[i];
            headMultiple[i].multiple = multiple[i];
        }
        sort(headMultiple, headMultiple + 5, [](const cardMultiple a, const cardMultiple b) {
            if (a.multiple != b.multiple) {
                return a.multiple > b.multiple;
            } else {
                return a.head > b.head;
            }
        });
    }

    // 开局发牌
    void ShuffleCards(vector<Player>& players, const int TotalCards)
    {
        vector<int> deck(TotalCards);
        for (int i = 0; i < TotalCards; i++) {
            deck[i] = i + 1;
        }
        random_device rd;
        mt19937 g(rd());
        shuffle(deck.begin(), deck.end(), g);
        int cardIndex = 0;
        for (int pid = 0; pid < playerNum; pid++) {
            for (int i = 0; i < handNum; i++) {
                players[pid].hand.push_back(deck[cardIndex++]);
            }
            sort(players[pid].hand.begin(), players[pid].hand.end());
        }
        vector<int> tmp;
        for (int i = 0; i < lineNum; i++) {
            tmp.push_back(deck[cardIndex++]);
        }
        sort(tmp.begin(), tmp.end());
        for (int i = 0; i < lineNum; i++) {
            tableStatus[i].push_back(tmp[i]);
        }
    }

    // 获取公屏赛况
    string GetTable(const bool showPlaceing, const vector<Player> players, const vector<Player> current_players) const
    {
        // 桌面状态
        html::Table desk(lineNum, lineLimit);
        desk.SetTableStyle("align=\"center\" cellpadding=\"2\"");
        for (int i = 0; i < lineNum; i++) {
            for (int j = 0; j < lineLimit; j++) {
                if (j == lineLimit - 1) {
                    desk.Get(i, j).SetContent("![](file:///" + ResourceDir + "warn.png)");
                } else if (tableStatus[i].size() <= j) {
                    desk.Get(i, j).SetContent("![](file:///" + ResourceDir + "empty.png)");
                } else {
                    desk.Get(i, j).SetContent(Card(tableStatus[i][j]));
                }
            }
        }
        // 当前等待放置的玩家
        html::Table current(3, current_players.size());
        current.SetTableStyle("align=\"center\"");
        if (showPlaceing && !current_players.empty()) {
            current.Get(0, 0).SetStyle("colspan=\"" + to_string(current_players.size()) + "\" style=\"text-align:left;\"");
            current.Get(0, 0).SetContent("<font size=\"6\">本回合玩家放置顺序：</font>");
            for (int pid = 0; pid < current_players.size(); pid++) {
                current.Get(1, pid).SetContent(Card(current_players[pid].current));
                current.Get(2, pid).SetContent(to_string(current_players[pid].id + 1) + "号" + current_players[pid].avatar);
            }
        }
        return GetPlayerTable(players) + desk.ToString() + current.ToString();
    }

    // 获取玩家信息
    string GetPlayerTable(const vector<Player> players) const
    {
        html::Table playerTable(playerNum, 1);
        playerTable.SetTableStyle("align=\"center\" cellpadding=\"2\"");
        for (int pid = 0; pid < playerNum; pid++) {
            playerTable.Get(pid, 0).SetContent(GetPlayerInfo(players[pid]));
        }
        return playerTable.ToString();
    }

    // 获取手牌
    string GetHand(const PlayerID pid, const vector<Player> players, const vector<Player> current_players) const
    {
        int line = ceil(players[pid].hand.size() / 5.0);
        html::Table handTable(line, 5);
        handTable.SetTableStyle("align=\"center\"");
        int handIndex = 0;
        for (int i = 0; i < line; i++) {
            for (int j = 0; j < 5; j++) {
                handTable.Get(i, j).SetContent(Card(players[pid].hand[handIndex++]));
                if (handIndex == players[pid].hand.size()) {
                    break;
                }
            }
        }
        return GetTable(false, players, current_players) + GetPlayerInfo(players[pid]) + handTable.ToString();
    }

    string GetPlayerInfo(const Player player) const
    {
        html::Table playerTable(1, 6);
        playerTable.SetTableStyle("align=\"center\" cellpadding=\"2\"");
        playerTable.Get(0, 0).SetStyle("style=\"width:60px; text-align:right;\"");
        playerTable.Get(0, 0).SetContent(to_string(player.id + 1) + "号：");
        playerTable.Get(0, 1).SetStyle("style=\"width:40px;\"");
        playerTable.Get(0, 1).SetContent(player.avatar);
        playerTable.Get(0, 2).SetStyle("style=\"width:250px; text-align:left;\"");
        playerTable.Get(0, 2).SetContent(player.name);
        playerTable.Get(0, 3).SetContent("![](file:///" + ResourceDir + "favicon.png)");
        playerTable.Get(0, 4).SetContent("![](file:///" + ResourceDir + "mark.png)");
        playerTable.Get(0, 5).SetStyle("style=\"width:50px; text-align:left;\"");
        playerTable.Get(0, 5).SetContent("<font size=\"6\" color=\"#7f5093\"><b>" + to_string(player.head) + "</b></font>");
        return playerTable.ToString();
    }

    // 检测玩家是否需要手动操作
    int CheckPlayerNeedPlace(const PlayerID pid, const vector<Player> players) const
    {
        int card = players[pid].current;
        int maxIndex = 0;
        bool automatic = false;
        for (int i = 0; i < lineNum; i++) {
            if (card > tableStatus[i][tableStatus[i].size() - 1]) {
                if (!automatic) {
                    automatic = true;
                    maxIndex = i;
                    continue;
                }
                if (tableStatus[i][tableStatus[i].size() - 1] > tableStatus[maxIndex][tableStatus[maxIndex].size() - 1]) {
                    maxIndex = i;
                }
            }
        }
        if (!automatic) return -1;
        else return maxIndex;
    }

    // 玩家放置卡牌
    int PlaceCard(Player &player, const int line, vector<Player> &current_players)
    {
        int card = player.current;
        int current_place = tableStatus[line].size() - 1;
        current_players.erase(current_players.begin());
        // 放置成功
        if (card > tableStatus[line][current_place] && current_place + 2 < lineLimit) {
            tableStatus[line].push_back(card);
            return 0;
        }
        // 放置失败
        int gain_head = GetLineHead(line);
        tableStatus[line].clear();
        tableStatus[line].push_back(card);
        player.head += gain_head;
        return gain_head;
    }

    int GetLineHead(const int line) const
    {
        int gain_head = 0;
        for (int i = 0; i < tableStatus[line].size(); i++) {
            if (tableStatus[line][i] % headMultiple[0].multiple == 0) gain_head += headMultiple[0].head;
            else if (tableStatus[line][i] % headMultiple[1].multiple == 0) gain_head += headMultiple[1].head;
            else if (tableStatus[line][i] % headMultiple[2].multiple == 0) gain_head += headMultiple[2].head;
            else if (tableStatus[line][i] % headMultiple[3].multiple == 0) gain_head += headMultiple[3].head;
            else if (tableStatus[line][i] % headMultiple[4].multiple == 0) gain_head += headMultiple[4].head;
            else gain_head += 1;
        }
        return gain_head;
    }

    bool CheckPlayerHead(const int num, const vector<Player> players) const
    {
        for (int pid = 0; pid < playerNum; pid++) {
            if (players[pid].head >= num) {
                return true;
            }
        }
        return false;
    }

    // 生成卡牌
    string Card(const int num) const
    {
        if (num % headMultiple[0].multiple == 0) return HeadToImage(headMultiple[0].head, num);
        else if (num % headMultiple[1].multiple == 0) return HeadToImage(headMultiple[1].head, num);
        else if (num % headMultiple[2].multiple == 0) return HeadToImage(headMultiple[2].head, num);
        else if (num % headMultiple[3].multiple == 0) return HeadToImage(headMultiple[3].head, num);
        else if (num % headMultiple[4].multiple == 0) return HeadToImage(headMultiple[4].head, num);
        else return HeadToImage(1, num);
    }

    string HeadToImage(const int headNum, const int num) const
    {
        if (headNum == 10) return CardImage("b10", to_string(num), "#ff0000");
        if (headNum == 7) return CardImage("b7", to_string(num), "#f8e65f");
        if (headNum == 5) return CardImage("b5", to_string(num), "#ffbb52");
        if (headNum == 3) return CardImage("b3", to_string(num), "#72caff");
        if (headNum == 2) return CardImage("b2", to_string(num), "#ffcc00");
        if (headNum == 1) return CardImage("b1", to_string(num), "#42397b");
        return "[ERROR] 数量有误，无法生成卡牌图片，请联系管理员中断游戏！";
    }

    string CardImage(const string name, const string text, const string color) const
    {
        // 字体style
        const string corner_font = "Arial Rounded MT Bold";
        const string middle_font = "Algerian";
        string style = "<style>@font-face{font-family:\'" + corner_font + "\'; src:url(\"file:///" + ResourceDir + "ARLRDBD.ttf\");}@font-face{font-family:\'" + middle_font + "\'; src:url(\"file:///" + ResourceDir + "ALGER.ttf\");}</style>";
        
        string BackgroundImage = "<div style=\"background:url('file:///" + ResourceDir + name + ".png') no-repeat; width:120px; height:170px; line-height:170px;\">";

        html::Table num_table(3, 2);
        num_table.SetTableStyle("style=\"font-family:\'" + corner_font + "\'; color:" + color + "; width:115px; height:170px; margin-left:3px\"");
        num_table.Get(0, 0).SetStyle("style=\"font-size:12px; text-align:left;\"");
        num_table.Get(0, 0).SetContent(text);
        num_table.Get(0, 1).SetStyle("style=\"font-size:12px; text-align:right;\"");
        num_table.Get(0, 1).SetContent(text);
        num_table.Get(1, 0).SetStyle("colspan=\"2\" style=\"font-family:\'" + middle_font + "\'; font-size:60px; text-align:center; height:120px;\"");
        num_table.Get(1, 0).SetContent(text);
        num_table.Get(2, 0).SetStyle("style=\"font-size:12px; text-align:right; transform:rotate(180deg);\"");
        num_table.Get(2, 0).SetContent(text);
        num_table.Get(2, 1).SetStyle("style=\"font-size:12px; text-align:left; transform:rotate(180deg);\"");
        num_table.Get(2, 1).SetContent(text);

        return style + BackgroundImage + num_table.ToString() + "</div>";
    }

};