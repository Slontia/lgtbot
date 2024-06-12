
const string color_ch[2] = {"红", "黑"};
const string direction_ch[5] = {{}, "上", "下", "左", "右"};

class Board
{
public:

    // 底色 0正常 1本回合行动
    const string color[2] = {"#ECECEC", "#FFEEAD"};
    // 棋盘大小 
	int size = 4;
    // 棋盘
    int chess[20][20];
    // 上一枚落子 
	int lastX1, lastY1, lastX2, lastY2;
    // 棋盘的markdown格式 
	string grid[20][30];

	// 玩家昵称
	string name[2];
	// 玩家分数及目标分
	int score[2], targetScore;

    // 初始化棋盘
    void Initialize()
    {
        lastX1 = lastX2 = lastY1 = lastY2 = -1;
        for(int j = 0; j <= size; j++)
			for(int i = 0; i <= size; i++)
				chess[i][j] = 0;
		// 随机生成开局棋子
		int temp = 0;
		while (temp < 4) {
			int X = rand() % size + 1;
			int Y = rand() % size + 1;
			if (chess[X][Y] == 0) {
				chess[X][Y] = temp / 2 + 1;
				temp++;
			}
		}
    }

	void ClearLast()
	{
		lastX1 = lastX2 = lastY1 = lastY2 = -1;
	}

    // 图形界面
	string GetUI(bool show_bg)
    {
        // 初始化 
		for(int i = 0; i <= size + 2; i++)
			for(int j = 0; j <= size + 2; j++)
				grid[i][j] = "";

        // 用来填充的字符串 
		string fill = "<font size=3>　</font>";

        // 上下字母
		for(int i = 1; i <= size; i++)
		{
			grid[i][0] += "<td bgcolor=\"#FFFFFF\"><font size=5>";
			grid[i][0] += char(i + 'A' - 1);
			grid[i][0] += "</font></td>";
			
			grid[i][size + 1] += "<td bgcolor=\"#FFFFFF\"><font size=5>";
			grid[i][size + 1] += char(i + 'A' - 1);
			grid[i][size + 1] += "</font></td>";
			
			// 最右侧的填充格 
			grid[i][size + 2] += "<td bgcolor=\"#FFFFFF\">";
			grid[i][size + 2] += fill;
			grid[i][size + 2] += "</td>";
		}
		
		// 左右数字 
		for(int j = 1; j <= size; j++)
		{
			grid[0][j] += "<td bgcolor=\"#FFFFFF\"><font size=5>";
			grid[0][j] += to_string(j);
			grid[0][j] += "</font></td>";
			
			grid[size + 1][j] += "<td bgcolor=\"#FFFFFF\"><font size=5>";
			grid[size + 1][j] += to_string(j);
			grid[size + 1][j] += "</font></td>";
			
			// 最下方的填充格 
			grid[size + 2][j] += "<td bgcolor=\"#FFFFFF\">";
			grid[size + 2][j] += fill;
			grid[size + 2][j] += "</td>";
		}

        // 棋盘边缘
		grid[0][0] = grid[0][size + 1] = grid[0][size + 2] = 
		grid[size + 1][0] = grid[size + 1][size + 1] = grid[size + 1][size + 2] = 
		grid[size + 2][0] = grid[size + 2][size + 1] = grid[size + 2][size + 2] = 
		"<td bgcolor=\"#FFFFFF\"><font size=7>" + fill + "</font></td>";

        // 中间棋盘
		for(int j = 1; j <= size; j++)
		{
	 		for(int i = 1; i <= size; i++)
	 		{
			 	grid[i][j] += "<td style=\"width: 60px; height: 60px;\" bgcolor=\"";
			 	
			 	// 背景色
			 	if(((i == lastX1 && j == lastY1) || (i == lastX2 && j == lastY2)) && show_bg) {
					grid[i][j] += color[1];
				} else {
                    grid[i][j] += color[0];
                }
			 	
			 	// 棋子
			 	grid[i][j] += "\"><font size=7>";
			 	
			 	string ch;
			 	if(chess[i][j] == 0)
				 	ch = fill;
			 	if(chess[i][j] == 1)
					ch = "<svg height=\"60px\" width=\"60px\"><circle cx=\"30\" cy=\"30\" r=\"18\" fill=\"#FF0000\"/></svg>";
			 	if(chess[i][j] == 2)
					ch = "<svg height=\"60px\" width=\"60px\"><circle cx=\"30\" cy=\"30\" r=\"18\" fill=\"#000000\"/></svg>";
			 	grid[i][j] += ch;
			 	
			 	grid[i][j] += "</font></td>";
			}
		}

        // 构造UI 
		string UI = "";

		// 玩家0 信息
		UI += "<table style=\"text-align:center;margin:auto;\"><tbody>";
        UI += "<tr><td>" + fill + "</td></tr>";
        UI += "<tr><td style=\"width:500px\" bgcolor=\"#ECECEC\"><font size=4>" + name[0] + "</font></td></tr>";
		UI += "<tr><td style=\"width:500px\" bgcolor=\"#ECECEC\"><font size=5><font color=\"FF0000\"><b>红方</b></font>分数：";
		UI += "<font color=\"0000FF\">" + to_string(score[0]) + " / " + to_string(targetScore) + "</font></font></td></tr>";
		UI += "</table>";

		UI += "<table style=\"text-align:center;margin:auto;\"><tbody>";
		
		// 为保持对称，开头要额外增加一行，并且每行下方都额外增加一格
		UI += "<tr><td bgcolor=\"#FFFFFF\">" + fill + "</td></tr>";
		for(int j = 0; j <= size + 2; j++)
		{
			UI += "<tr>";
			
			// 额外增加一格 
			UI += "<td bgcolor=\"#FFFFFF\">" + fill + "</td>";
			
	 		for(int i = 0; i <= size + 2; i++)
	 		{
	 			UI += grid[i][j];
	 		}
	 		UI += "</tr>";
		}
		UI += "</table>";

		// 玩家1 信息
		UI += "<table style=\"text-align:center;margin:auto;\"><tbody>";
		UI += "<tr><td style=\"width:500px\" bgcolor=\"#ECECEC\"><font size=5><b>黑方</b>分数：";
		UI += "<font color=\"0000FF\">" + to_string(score[1]) + " / " + to_string(targetScore) + "</font></font></td></tr>";
        UI += "<tr><td style=\"width:500px\" bgcolor=\"#ECECEC\"><font size=4>" + name[1] + "</font></td></tr>";
		UI += "<tr><td>" + fill + "</td></tr>";
		UI += "</table>";
		
		return UI;
    }

	// 检查一个字符串是否是合法的位置 
	string CheckCoordinate(string &s)
	{
		// 长度必须为2或3 
		if (s.length() != 2 && s.length() != 3)
		{
			return "[错误] 坐标只能是长度不超过 3 的字符串，如：A1";
		}
		// 大小写不敏感 
		if (s[0] <= 'z' && s[0] >= 'a')
		{
			s[0] = s[0] - 'a' + 'A';
		}
		// 检查是否为合法输入 
		if (s[0] > 'Z' || s[0] < 'A' || s[1] > '9' || s[1] < '0' )
		{
			return "[错误] 请输入合法的字符串（字母+数字），如：A1";
		}
		if (s.length() == 3 && (s[2] > '9' || s[2] < '0'))
		{
			return "[错误] 请输入合法的字符串（字母+数字），如：A1";
		}
		// 转化
        auto pos = TranString(s);
		int nowY = pos.first, nowX = pos.second; 
		// 检查是否越界
		if (nowX < 1 || nowX > size || nowY < 1 || nowY > size) 
		{
			return "[错误] 你选择的位置超出了棋盘的大小";
		}

		return "OK";
	}

    // 将字符串转为一个位置pair。必须确保字符串是合法的再执行这个操作。 
	static pair<int, int> TranString(string s)
	{
		int nowX = s[0] - 'A' + 1, nowY = s[1] - '0'; 
		if (s.length() == 3)
		{
			nowY = (s[1] - '0') * 10 + s[2] - '0';
		}
		pair<int, int> ret;
		ret.first = nowX;
		ret.second = nowY;
		return ret;
	}

    // 开局为对手落子（此处已为对手代号）
	string PlaceChess(string s, int player)
	{
        string ret = CheckCoordinate(s);
        if (ret != "OK") return ret;

		player += 1;
		auto pos = TranString(s);
		int x = pos.first, y = pos.second; 
		if(chess[x][y] != 0)
		{
			return "[错误] 这个位置已经有棋子了";
		}
		
		lastX1 = pos.first, lastY1 = pos.second;
		lastX2 = -1, lastY2 = -1;
		chess[pos.first][pos.second] = player;

		if (WinCheck() != -1) {
			ClearLast();
			chess[pos.first][pos.second] = 0;
			return "[错误] 落子失败：当前在为对手落子，此位置会让对手达成三连";
		}
		
		return "OK"; 
	}

    // 记录棋子移动前后位置（尝试移动回合）
    string RecordChessMove(string s1, string s2, int player)
    {
        string ret1 = CheckCoordinate(s1);
		string ret2 = CheckCoordinate(s2);
        if (ret1 != "OK") return ret1;
		if (ret2 != "OK") return ret2;

        player += 1;
        auto pos1 = TranString(s1);
		auto pos2 = TranString(s2);
        int x1 = pos1.first, x2 = pos2.first, y1 = pos1.second, y2 = pos2.second;

        if ((x1 != x2 && y1 != y2) || (x1 == x2 && y1 == y2))
		{
			return "[错误] 这不是一个合法的移动";
		}
        if (chess[x1][y1] == 0)
		{
			return "[错误] 这个位置没有棋子";
		}
        if (chess[x1][y1] != player)
        {
            return "[错误] 这个位置不是您的棋子";
        }

        int addx = 0, addy = 0;
		if (x1 < x2) addx = 1;
		if (x1 > x2) addx = -1;
		if (y1 < y2) addy = 1;
		if (y1 > y2) addy = -1;

		while(x1 != x2 || y1 != y2)
		{
			x1 = x1 + addx;
			y1 = y1 + addy;
			if (chess[x1][y1] != 0)
			{
				return "[错误] 移动的路径上有别的棋子";
			}
		}

		x1 = pos1.first; x2 = pos2.first; y1 = pos1.second; y2 = pos2.second;
        // last1移动前位置 last2移动后位置
        lastX1 = x1, lastY1 = y1;
		lastX2 = x2, lastY2 = y2;

        return "OK"; 
	}

	// 返回当回合猜测的结果
	int GuessResult(int direction)
	{
		int addx = 0, addy = 0;
		if (lastX1 < lastX2) addx = 1;
		if (lastX1 > lastX2) addx = -1;
		if (lastY1 < lastY2) addy = 1;
		if (lastY1 > lastY2) addy = -1;

		if (addy < 0) {
			if (direction == 1) return 0;
			else return 1;
		}
		if (addy > 0) {
			if (direction == 2) return 0;
			else return 2;
		}
		if (addx < 0) {
			if (direction == 3) return 0;
			else return 3;
		}
		if (addx > 0) {
			if (direction == 4) return 0;
			else return 4;
		}

		return 0;
	}

	// 检查是否有玩家三连胜利
    int WinCheck() {
		int winner = -1;
		auto c = chess;
		for (int i = 1; i <= size; i++)
		{
			for (int j = 1; j <= size; j++)
			{
				if (c[i][j] == 0) continue;

				if ((c[i][j] == c[i + 1][j] && c[i][j] == c[i + 2][j]) ||
					(c[i][j] == c[i][j + 1] && c[i][j] == c[i][j + 2]) ||
					(c[i][j] == c[i + 1][j + 1] && c[i][j] == c[i + 2][j + 2]))
				{
					winner = c[i][j] - 1; 
				}
				// 特判，不然可能访问到负数位置 
				if (j >= 3)
				{
					if (c[i][j] == c[i + 1][j - 1] && c[i][j] == c[i + 2][j - 2])
					{
						winner = c[i][j] - 1; 
					}
				}
			}
		}
		return winner;
	}
};