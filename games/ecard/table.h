
const vector<vector<string>> CardName = {
    {"citizen", "king", "slave"},
    {"special_citizen", "special_king", "special_slave"},
};
const vector<string> CardName_ch = {{"市民", "皇帝", "奴隶"}};
const string mark[2] = {"<svg height=\"16px\" width=\"16px\"><circle cx=\"8\" cy=\"8\" r=\"5\" stroke=\"red\" stroke-width=\"1\" fill-opacity=\"0\"/></svg>", "<font color=\"red\" size=\"3\">×</font>"};
const string circle[3] = {
    "<svg height=\"60px\" width=\"60px\"><circle cx=\"30\" cy=\"30\" r=\"20\" fill=\"#BD413F\"/></svg>",
    "<svg height=\"60px\" width=\"60px\"><circle cx=\"30\" cy=\"30\" r=\"20\" fill=\"#00E118\"/></svg>",
    "<svg height=\"60px\" width=\"60px\"><circle cx=\"30\" cy=\"30\" r=\"20\" stroke=\"grey\" stroke-width=\"1\" fill-opacity=\"0\"/></svg>",
};

class Table
{
public:
    // 游戏模式
    int GameMode;
    // 游戏文件夹
    const char* ResourceDir;
    // 彩蛋触发
    int special;

    // 玩家昵称
	string name[2];
    // 玩家分数 [人生模式保存生命值和失分]
    int playerPoint[2];
    // 开局是否交换先后手 [不用于人生模式]
    bool swapPlayer = false;

    // [人生模式]
    // 开局设定的目标分
    int targetScore[2];
    // 进攻方ID & 防守方ID 默认为1 0（用于其他模式）
    PlayerID attacker = 0;
    PlayerID defender = 1;
    /*
        记录表格数据 -1为空数据
        1、胜负：0胜利 1失败
        2、下分：下注数值
        3、合计：仅失败记录累计损失血量
    */ 
    int LiveModeRecord[3][13];

    // [点球模式]胜负记录表
    int shootoutRecord[2][55];

    // 初始化游戏
    void Initialize(const int mode, const char* dir)
    {
        GameMode = mode;
        ResourceDir = dir;
        special = 0;
#ifndef TEST_BOT
        swapPlayer = rand() % 2 == 1 ? true : false;
#endif
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 13; j++) {
                LiveModeRecord[i][j] = -1;
            }
        }
        for (int i = 0; i < 2; i++) {
            for (int j = 1; j <= 50; j++) {
                shootoutRecord[i][j] = 2;
            }
        }
    }

    // 出牌阶段赛况[全模式通用]
    string GetCardTable(const int card1, const int card2, const int left_card, const int total_num) const
    {
        // 双方玩家信息
        html::Table name0(1, 1), name1(1, 1);
        name0.SetTableStyle("align=\"center\" cellpadding=\"5\"");
        name1.SetTableStyle("align=\"center\" cellpadding=\"5\"");
        if (GameMode == 2) {
            name0.Get(0, 0).SetContent("<font size=\"5\">" + name[0] + "</font>");
            name1.Get(0, 0).SetContent("<font size=\"5\">" + name[1] + "</font>");
        } else {
            name0.Get(0, 0).SetContent("<font size=\"5\">" + name[0] + "（得分：<font color=\"blue\">" + to_string(playerPoint[0]) + "</font>）</font>");
            name1.Get(0, 0).SetContent("<font size=\"5\">" + name[1] + "（得分：<font color=\"blue\">" + to_string(playerPoint[1]) + "</font>）</font>");
        }
        string nameTable[2] = {name0.ToString(), name1.ToString()};
        // 玩家0手牌
        html::Table back0(1, total_num);
        back0.SetTableStyle("align=\"center\"");
        for (int i = 0; i < total_num; i++) {
            if (i < left_card) {
                back0.Get(0, i).SetContent(std::string("![](file:///") + ResourceDir + "back.jpg)");
            } else {
                back0.Get(0, i).SetContent(std::string("![](file:///") + ResourceDir + "empty.jpg)");
            }
        }
        // 双方玩家出牌
        html::Table cards(2, 1);
        cards.SetTableStyle("align=\"center\" cellpadding=\"10\" cellspacing=\"10\"");
        cards.Get(0, 0).SetContent(std::string("![](file:///") + ResourceDir + CardName[special][card1] + ".jpg)");
        cards.Get(1, 0).SetContent(std::string("![](file:///") + ResourceDir + CardName[special][card2] + ".jpg)");
        // 玩家1手牌
        html::Table back1(1, total_num);
        back1.SetTableStyle("align=\"center\"");
        for (int i = 0; i < total_num; i++) {
            if (i >= total_num - left_card) {
                back1.Get(0, i).SetContent(std::string("![](file:///") + ResourceDir + "back.jpg)");
            } else {
                back1.Get(0, i).SetContent(std::string("![](file:///") + ResourceDir + "empty.jpg)");
            }
        }
        if (left_card == 0) {
            return nameTable[defender] + cards.ToString() + nameTable[attacker];
        }
        return nameTable[defender] + back0.ToString() + cards.ToString() + back1.ToString() + nameTable[attacker];
    }

    // [人生模式]记录表
    string GetLiveModeTable() const
    {
        // 双方玩家信息
        html::Table name0(1, 1);
        name0.SetTableStyle("align=\"center\" cellpadding=\"5\"");
        string status0;
        if(attacker == 0) {
            status0 = " 的生命：<font color=\"red\">" + to_string(playerPoint[0]) + "</font> / 30";
        } else {
            status0 = " 失分：<font color=\"#FFC900\">" + to_string(playerPoint[0]) + "</font> / <font color=\"blue\">" + to_string(targetScore[1]) + "</font>";
        }
        name0.Get(0, 0).SetContent("<font size=\"3\">" + name[0] + status0 + "</font>");
        html::Table name1(1, 1);
        name1.SetTableStyle("align=\"center\" cellpadding=\"5\"");
        string status1;
        if(attacker == 1) {
            status1 = " 的生命：<font color=\"red\">" + to_string(playerPoint[1]) + "</font> / 30";
        } else {
            status1 = " 失分：<font color=\"#FFC900\">" + to_string(playerPoint[1]) + "</font> / <font color=\"blue\">" + to_string(targetScore[0]) + "</font>";
        }
        name1.Get(0, 0).SetContent("<font size=\"3\">" + name[1] + status1 + "</font>");
        string nameTable[2] = {name0.ToString(), name1.ToString()};

        html::Table table(5, 13);
        table.SetTableStyle("align=\"center\" border=\"1px solid #ccc\" cellpadding=\"1\" cellspacing=\"1\"");
        // 回合
        table.Get(0, 0).SetStyle("width=\"65px\"");
        table.Get(0, 0).SetContent("<font size=\"3\">回合</font>");
        for (int i = 1; i <= 12; i++) {
            if (i <= 3 || (i >= 7 && i <= 9)) {
                table.Get(2, i).SetColor("#FFFDE4");
                table.Get(3, i).SetColor("#FFFDE4");
                table.Get(4, i).SetColor("#FFFDE4");
            } else {
                table.Get(2, i).SetColor("#FFDEE2");
                table.Get(3, i).SetColor("#FFDEE2");
                table.Get(4, i).SetColor("#FFDEE2");
            }
            table.Get(0, i).SetStyle("width=\"30px\"");
            table.Get(0, i).SetContent("<font color=\"blue\" size=\"3\">" + to_string(i) + "</font>");
        }
        // 阵营
        table.Get(1, 0).SetContent("<font size=\"3\">阵营</font>");
        table.MergeRight(1, 1, 3); table.Get(1, 1).SetColor("#FFFDE4");
        table.MergeRight(1, 4, 3); table.Get(1, 4).SetColor("#FFDEE2");
        table.MergeRight(1, 7, 3); table.Get(1, 7).SetColor("#FFFDE4");
        table.MergeRight(1, 10, 3); table.Get(1, 10).SetColor("#FFDEE2");
        table.Get(1, 1).SetContent("<font size=\"3\">**" + CardName_ch[1] + "**</font>");
        table.Get(1, 4).SetContent("<font size=\"3\">**" + CardName_ch[2] + "**</font>");
        table.Get(1, 7).SetContent("<font size=\"3\">**" + CardName_ch[1] + "**</font>");
        table.Get(1, 10).SetContent("<font size=\"3\">**" + CardName_ch[2] + "**</font>");
        // 胜负
        table.Get(2, 0).SetContent("<font size=\"3\">胜负</font>");
        for (int i = 1; i <= 12; i++) {
            table.Get(2, i).SetContent(LiveModeRecord[0][i] == -1 ? " " : (LiveModeRecord[0][i] == 1 ? mark[0] : mark[1]));
        }
        // 下分
        table.Get(3, 0).SetContent("<font size=\"3\">下分</font>");
        for (int i = 1; i <= 12; i++) {
            string bid = LiveModeRecord[1][i] == -1 ? " " : to_string(LiveModeRecord[1][i]);
            table.Get(3, i).SetContent("<font size=\"3\">" + bid + "</font>");
        }
        // 合计
        table.Get(4, 0).SetContent("<font size=\"3\">合计</font>");
        for (int i = 1; i <= 12; i++) {
            string total = LiveModeRecord[2][i] == -1 ? " " : to_string(LiveModeRecord[2][i]);
            table.Get(4, i).SetContent("<font color=\"red\" size=\"3\">" + total + "</font>");
        }

        return nameTable[defender] + table.ToString() + nameTable[attacker];
    }

    // [点球模式]记录表
    string GetShootoutModeTable(const int round) const
    {
        // 双方玩家信息
        html::Table name0(1, 1), name1(1, 1);
        name0.SetTableStyle("align=\"center\" cellpadding=\"5\"");
        name1.SetTableStyle("align=\"center\" cellpadding=\"5\"");
        name0.Get(0, 0).SetContent("<font size=\"5\">" + name[0] + "（得分：<font color=\"blue\">" + to_string(playerPoint[0]) + "</font>）</font>");
        name1.Get(0, 0).SetContent("<font size=\"5\">" + name[1] + "（得分：<font color=\"blue\">" + to_string(playerPoint[1]) + "</font>）</font>");
        const string nameTable[2] = {name0.ToString(), name1.ToString()};
        // 双方玩家记录表格
        const int column = ceil(round / 2.0) < 5 ? 5 : ceil(round / 2.0);
        html::Table table1(1, column), table2(1, column);
        table1.SetTableStyle("align=\"center\" border=\"1px solid #ccc\" cellpadding=\"1\" cellspacing=\"1\"");
        table2.SetTableStyle("align=\"center\" border=\"1px solid #ccc\" cellpadding=\"1\" cellspacing=\"1\"");
        for (int i = 0; i < column; i++) {
            if (i == 5) {
                table1.Get(0, i).SetStyle("style=\"border-left:2px solid #000000\"");
                table2.Get(0, i).SetStyle("style=\"border-left:2px solid #000000\"");
            }
            table1.Get(0, i).SetColor("#F6E8DD");
            table1.Get(0, i).SetContent(circle[shootoutRecord[defender][i + 1]]);
            table2.Get(0, i).SetColor("#F6E8DD");
            table2.Get(0, i).SetContent(circle[shootoutRecord[attacker][i + 1]]);
        }
        return nameTable[defender] + table1.ToString() + "<p/>" + table2.ToString() + nameTable[attacker];
    }

    // [点球模式]获取玩家剩余回合数（胜负判定用）
    int GetPlayerLeftRound(PlayerID player) const
    {
        for (int i = 0; i < 5; i++) {
            if (shootoutRecord[player][i + 1] == 2) {
                return 5 - i;
            }
        }
        return 0;
    }
};
