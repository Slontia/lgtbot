
using namespace std;

class Board
{
public:
	
    // 底色（状态 类型）
	const string color[3][3] = {
		{"ECECEC","E0FFE0","E0FFE0"},	// 00 01 02
		{"B0E0FF","FFA0A0","000000"},	// 10 11 12
		{"A0FFA0","FF6868"}				// 20 21 **
	};
	// 图标（本回合 类型）
	const string icon[3][3] = {
		{"<font size=7>　</font>","<font size=7>+</font>","<font size=7 color=\"#FF0000\">★</font>"},	// 00 01 02
		{"<font color=\"#FF0000\" size=6>○</font>","<font size=7>⊕</font>","<font size=7 color=\"#FF0000\">✮</font>"}    // 10 11 12
	};
    // 机身相对飞机头坐标位置偏差（方向 机身 XY）
    const int position[5][9][2] = {{},   // 1上 2下 3左 4右
        {{-2,1}, {-1,1}, {0,1}, {1,1}, {2,1}, {0,2}, {-1,3}, {0,3}, {1,3}},
        {{-2,-1}, {-1,-1}, {0,-1}, {1,-1}, {2,-1}, {0,-2}, {-1,-3}, {0,-3}, {1,-3}},
        {{1,2}, {1,1}, {1,0}, {1,-1}, {1,-2}, {2,0}, {3,1}, {3,0}, {3,-1}},
        {{-1,2}, {-1,1}, {-1,0}, {-1,-1}, {-1,-2}, {-2,0}, {-3,1}, {-3,0}, {-3,-1}}
    };

	// 地图大小 
	int sizeX, sizeY;
	/*
            ***  地图区域  ***
		  - 状态 -   | - 类型 -
        0 - 未被打击 | 0 - 空地
        1 - 已被打击 | 1 - 机身
        2 - 侦察点   | 2 - 飞机头
    */ 
    int map[20][20][2];

    /*
        保存飞机的参数（移除功能用）
		如果为飞机头，则保存方向数字
        如果为机身，则保存叠加层数（重叠功能用）
    */ 
    int body[20][20];
    /*
        保存标记数据
		飞机头标记 - 2
        机身标记 - 1
    */ 
    int mark[20][20];
	
    /*
        是否为本回合行动
		0 - 否 1 - 是
    */ 
	int this_turn[20][20];

	// 地图markdown格式 
	string grid[20][20];

    // 玩家昵称
    string MapName;
	// 飞机数量
	int planeNum;
    // 剩余要害数
    int alive;

    // 是否为准备回合
    int prepare;
    // 保存首要害坐标
    int firstX, firstY;

    // 初始化地图
    void InitializeMap()
    {
        for(int j = 0; j <= sizeY; j++) {
			for(int i = 0; i <= sizeX; i++) {
				map[i][j][0] = map[i][j][1] = body[i][j] = mark[i][j] = this_turn[i][j] = 0;
                if ((i == 1) + (j == 1) + (i == sizeX) + (j == sizeY) == 2) {
                    map[i][j][0] = 1;
                }
            }
        }
    }
	
	// 图形界面
	string Getmap(int show_planes, int crucial_mode)
	{
		// 初始化 
		for(int i = 0; i <= sizeX + 2; i++)
			for(int j = 0; j <= sizeY + 2; j++)
				grid[i][j] = "";
		
		// 用来填充的字符串 
		string fill = "<font size=7>　</font><font size=5>　</font>";
		
		// 上下字母
		for(int i = 1; i <= sizeX; i++)
		{
			grid[i][0] += "<td bgcolor=\"#FFFFFF\"><font size=7>";
			grid[i][0] += char(i + 'A' - 1);
			grid[i][0] += "</font></td>";
			
			grid[i][sizeY + 1] += "<td bgcolor=\"#FFFFFF\"><font size=7>";
			grid[i][sizeY + 1] += char(i + 'A' - 1);
			grid[i][sizeY + 1] += "</font></td>";
			
			// 最右侧的填充格 
			grid[i][sizeY + 2] += "<td bgcolor=\"#FFFFFF\"><font size=7>";
			grid[i][sizeY + 2] += fill;
			grid[i][sizeY + 2] += "</font></td>";
		}
		
		// 左右数字 
		for(int j = 1; j <= sizeY; j++)
		{
			grid[0][j] += "<td bgcolor=\"#FFFFFF\"><font size=7>";
			grid[0][j] += to_string(j);
			grid[0][j] += "</font></td>";
			
			grid[sizeX + 1][j] += "<td bgcolor=\"#FFFFFF\"><font size=7>";
			grid[sizeX + 1][j] += to_string(j);
			grid[sizeX + 1][j] += "</font></td>";
			
			// 最下方的填充格 
			grid[sizeX + 2][j] += "<td bgcolor=\"#FFFFFF\"><font size=7>";
			grid[sizeX + 2][j] += fill;
			grid[sizeX + 2][j] += "</font></td>";
		}
		
		// 地图边缘
		grid[0][0] = grid[0][sizeY + 1] = grid[0][sizeY + 2] = 
		grid[sizeX + 1][0] = grid[sizeX + 1][sizeY + 1] = grid[sizeX + 1][sizeY + 2] = 
		grid[sizeX + 2][0] = grid[sizeX + 2][sizeY + 1] = grid[sizeX + 2][sizeY + 2] = 
		"<td bgcolor=\"#FFFFFF\"><font size=7>" + fill + "</font></td>";
		
		// 中间地图
		for(int j = 1; j <= sizeY; j++)
		{
	 		for(int i = 1; i <= sizeX; i++)
	 		{
			 	grid[i][j] += "<td bgcolor=\"";
			 	
			 	// 背景色
				if (show_planes || map[i][j][0] > 0) {
                    if (map[i][j][1] == 2 && !show_planes && (crucial_mode == 1 || (!(firstX == i && firstY == j) && crucial_mode == 2))) {
					    grid[i][j] += color[map[i][j][0]][1];
                    } else {
                        grid[i][j] += color[map[i][j][0]][map[i][j][1]];
                    }
				} else {
					grid[i][j] += color[0][0];
				}

                grid[i][j] += "\">";
			 	
			 	// 地图符号
			 	string m;
			 	if(map[i][j][0] == 2 && map[i][j][1] == 0) {
					m = "<font size=7>-</font>";
				} else {
					if (show_planes || map[i][j][0] > 0) {
                        if (map[i][j][1] == 2 && !show_planes && (crucial_mode == 1 || (!(firstX == i && firstY == j) && crucial_mode == 2))) {
                            m = icon[this_turn[i][j]][1];
                        } else {
                            m = icon[this_turn[i][j]][map[i][j][1]];
                        }
					} else {
                        // 空地标记
                        if (mark[i][j] == 1) {
                            m = "<font size=5 color=\"#505050\">—</font>";
                        } else if (mark[i][j] == 2) {
                            m = "<font size=7 color=\"#505050\">☆</font>";
                        } else {
                            m = fill;
                        }
					}
				}
			 	grid[i][j] += m;
			 	
			 	grid[i][j] += "</td>";
			}
		}

		// 构造地图
		string mapString = "";
		
		// 玩家昵称
		mapString += "<table style=\"text-align:center;margin:auto;\"><tbody>";
        mapString += "<tr><td>" + fill + "</td></tr>";
        if (MapName == "机器人0号") {
            mapString += "<tr><td style=\"width:850px\" bgcolor=\"#ECECEC\"><font size=6><div>无限火力BOSS战</div>拥有多种特殊技能，每回合都可能随机发动</font></td></tr>";
        } else {
            mapString += "<tr><td style=\"width:850px\" bgcolor=\"#ECECEC\"><font size=6>" + MapName + "的地图</font></td></tr>";
        }
		mapString += "</table>";

		mapString += "<table style=\"text-align:center;margin:auto;\"><tbody>";
		
		// 为保持对称，开头要额外增加一行，并且每行下方都额外增加一格
		mapString += "<tr><td>" + fill + "</td></tr>";
		for(int j = 0; j <= sizeY + 2; j++)
		{
			mapString += "<tr>";
			
			// 额外增加一格 
			mapString += "<td><font size=7>" + fill + "</font></td>";
			
	 		for(int i = 0; i <= sizeX + 2; i++)
	 		{
	 			mapString += grid[i][j];
	 		}
	 		mapString += "</tr>";
		}
		mapString += "</table>";
		
		// 要害数量显示 
		mapString += "<table style=\"text-align:center;margin:auto;\"><tbody>";
        if (prepare) {
            mapString += "<tr><td><font size=7>剩余飞机数：" + to_string(planeNum - alive) + "</font></td></tr>";
        } else {
            if (crucial_mode == 0) {
                mapString += "<tr><td><font size=7>命中要害：" + to_string(planeNum - alive) + "</font></td></tr>";
            } else {
                mapString += "<tr><td><font size=7>命中要害：？？？</font></td></tr>";
            }
        }
        mapString += "<tr><td>" + fill + "</td></tr>";
		mapString += "</table>";
		
		return mapString; 
	}

    // 检查坐标是否合法
    string CheckCoordinate(string &s) {
        // 长度必须为2或3 
		if (s.length() != 2 && s.length() != 3)
		{
			return "[错误] 输入的坐标长度只能为 2 或 3，如：A1";
		}
		// 大小写不敏感 
		if (s[0] <= 'z' && s[0] >= 'a')
		{
			s[0] = s[0] - 'a' + 'A';
		}
		// 检查是否为合法输入 
		if (s[0] > 'Z' || s[0] < 'A' || s[1] > '9' || s[1] < '0')
		{
			return "[错误] 请输入合法的坐标（字母+数字），如：A1";
		}
		if (s.length() == 3 && (s[2] > '9' || s[2] < '0'))
		{
			return "[错误] 请输入合法的坐标（字母+数字），如：A1";
		}
		return "OK";
    }
	
	// 添加一架飞机
	string AddPlane(string s, int direction, bool overlap)
	{
        string result = CheckCoordinate(s);
        if (result != "OK") {
            return result;
        }
		// 转化
		int X = s[0] - 'A' + 1, Y = s[1] - '0'; 
		if (s.length() == 3)
		{
			Y = (s[1] - '0') * 10 + s[2] - '0';
		}
		// 检查地图边界
		if (direction == 1 && (X < 3 || X > sizeX-2 || Y < 1 || Y > sizeY-3) ||
            direction == 2 && (X < 3 || X > sizeX-2 || Y < 4 || Y > sizeY) ||
            direction == 3 && (X < 1 || X > sizeX-3 || Y < 3 || Y > sizeY-2) ||
            direction == 4 && (X < 4 || X > sizeX || Y < 3 || Y > sizeY-2)
        ) {
			return "[错误] 放置的飞机超出了地图范围，请检查坐标和方向是否正确";
		}
		// 检查是否是空地
		if (map[X][Y][1] != 0)
		{
			return "[错误] 飞机头只能放置于空地，不能与机身或其他飞机头重叠";
		}
        // 检查是否是侦察点
		if (map[X][Y][0] == 2)
		{
			return "[错误] 飞机头不能放置于侦察点";
		}
        // 检查飞机是否重叠
        for (int i = 0; i < 9; i++) {
            if (map[X + position[direction][i][0]][Y + position[direction][i][1]][1] == 2) {
                return "[错误] 无法放置于此位置：机身不能与其他飞机头重叠";
            }
            if (map[X + position[direction][i][0]][Y + position[direction][i][1]][1] == 1 && !overlap) {
                return "[错误] 无法放置于此位置：当前规则飞机机身之间不允许重叠";
            }
        }
        // 放置飞机
        map[X][Y][1] = 2;
        body[X][Y] = direction;
		for (int i = 0; i < 9; i++) {
            map[X + position[direction][i][0]][Y + position[direction][i][1]][1] = 1;
            body[X + position[direction][i][0]][Y + position[direction][i][1]] += 1;
        }
        alive++;

		return "OK";
	}

    // 移除一架飞机
    string RemovePlane(string s)
	{
        string result = CheckCoordinate(s);
        if (result != "OK") {
            return result;
        }
		// 转化
		int X = s[0] - 'A' + 1, Y = s[1] - '0'; 
		if (s.length() == 3)
		{
			Y = (s[1] - '0') * 10 + s[2] - '0';
		}
        if (map[X][Y][1] != 2) {
            return "[错误] 移除失败：此处不存在飞机头，请输入飞机头坐标";
        }
        int direction = body[X][Y];
        map[X][Y][1] = body[X][Y] = 0;
        for (int i = 0; i < 9; i++) {
            body[X + position[direction][i][0]][Y + position[direction][i][1]] -= 1;
            // 如果此位置已没有机身，则设为空地（存在重叠则不移除）
            if (body[X + position[direction][i][0]][Y + position[direction][i][1]] == 0) {
                map[X + position[direction][i][0]][Y + position[direction][i][1]][1] = 0;
            }
        }
        alive--;

		return "OK";
	}

	// 清空全部飞机
    void RemoveAllPlanes()
	{
		for(int i = 1; i <= sizeX; i++) {
			for(int j = 1; j <= sizeY; j++) {
                map[i][j][1] = 0;
                body[i][j] = 0;
            }
        }
        alive = 0;
	}

    // 地图被进攻（对方操作）
    string Attack(string s)
    {
        string result = CheckCoordinate(s);
        if (result != "OK") {
            return result;
        }
		// 转化
		int X = s[0] - 'A' + 1, Y = s[1] - '0'; 
		if (s.length() == 3)
		{
			Y = (s[1] - '0') * 10 + s[2] - '0';
		}
        // 检查地图边界
		if (X < 1 || X > sizeX || Y < 1 || Y > sizeY) {
			return "[错误] 攻击的坐标超出了地图的范围";
		}
        if (map[X][Y][0] != 0) {
            return "[错误] 无法对已被打击的区域或侦察点发射导弹";
        }
        map[X][Y][0] = 1;
        this_turn[X][Y] = 1;
        if (map[X][Y][1] == 0) {
            return "0";
        }
        if (map[X][Y][1] == 1){
            return "1";
        }
        if (map[X][Y][1] == 2){
            alive--;
            return "2";
        }
        return "Empty Return";
    }

    // 添加飞机标记
	string AddMark(string s, int direction)
	{
        string result = CheckCoordinate(s);
        if (result != "OK") {
            return result;
        }
		// 转化
		int X = s[0] - 'A' + 1, Y = s[1] - '0'; 
		if (s.length() == 3)
		{
			Y = (s[1] - '0') * 10 + s[2] - '0';
		}
		// 检查地图边界
		if (direction == 1 && (X < 3 || X > sizeX-2 || Y < 1 || Y > sizeY-3) ||
            direction == 2 && (X < 3 || X > sizeX-2 || Y < 4 || Y > sizeY) ||
            direction == 3 && (X < 1 || X > sizeX-3 || Y < 3 || Y > sizeY-2) ||
            direction == 4 && (X < 4 || X > sizeX || Y < 3 || Y > sizeY-2)
        ) {
			return "[错误] 标记的飞机位置超出了地图范围，请检查坐标和方向是否正确";
		}
        // 设置标记
        mark[X][Y] = 2;
		for (int i = 0; i < 9; i++) {
            mark[X + position[direction][i][0]][Y + position[direction][i][1]] = 1;
        }

		return "OK";
	}

    // 清空全部标记
    void RemoveAllMark()
	{
		for(int i = 1; i <= sizeX; i++) {
			for(int j = 1; j <= sizeY; j++) {
                mark[i][j] = 0;
            }
        }
	}

};



// BOSS技能介绍 + 普攻、技能AI
string BossIntro()
{
    string BOSS_SkillIntro = "<table>";
    BOSS_SkillIntro += "<tr><th style=\"text-align:center;\">无限火力BOSS战</th></tr>";
    BOSS_SkillIntro += "<tr><td>BOSS每回合最多发动一个技能，且都会进行普通打击：随机向地图上发射 3-6 枚导弹。当达到 18 回合时，BOSS将增强普攻至最多 8 枚导弹</td></tr>";
    BOSS_SkillIntro += "<tr><td>技能1：15% 概率发动 [空军指挥]——随机移动所有未被击落的飞机至其他位置，并尽可能避开已侦察区域</td></tr>";
    BOSS_SkillIntro += "<tr><td>技能2：15% 概率发动 [连环轰炸]——随机打击地图上某个坐标的整个十字区域</td></tr>";
    BOSS_SkillIntro += "<tr><td>技能3：15% 概率发动 [雷达扫描]——随机扫描地图上 5*5 的区域，其中的所有飞机（飞机头+机身）会被直接击落</td></tr>";
    BOSS_SkillIntro += "<tr><td>技能4：20% 概率发动 [火力打击]——BOSS会发射一枚高爆导弹，炸毁地图上 3*3 的区域</td></tr>";
    BOSS_SkillIntro += "<tr><td>特殊：当BOSS的一架飞机被击落时，所有技能的触发概率提升 7%！</td></tr>";
    BOSS_SkillIntro += "</table>";
    return BOSS_SkillIntro;
}

string BossNormalAttack(Board (&board)[2], int round_, int (&attack_count)[2])
{
    int X, Y;
    string str;
    int try_count = 0, count = 0;
    int num;
    if (round_ < 18) {
        num = rand() % 4 + 3;
    } else {
        num = rand() % 4 + 5;
    }
    for (int i = 0; i < num; i++) {
        if (try_count > 10000) break;
        X = rand() % board[0].sizeX;
        Y = rand() % board[0].sizeY + 1;
        str = string(1, 'A' + X) + to_string(Y);
        string result = board[0].Attack(str);
        if (result != "0" && result != "1" && result != "2" ) {
            i--; try_count++;
            continue;
        } else {
            count++;
        }
    }
    attack_count[1] = count;
    return "BOSS进行普通打击：共成功发射了 " + to_string(count) + " 枚导弹";
}

string BossSkillAttack(Board (&board)[2], bool overlap, int (&timeout)[2])
{
    const int skill_probability[5] = {0, 15, 30, 45, 65};
    int add_p = 0;
    int X, Y;
    string str;

    X = rand() % board[0].sizeX;
    Y = rand() % board[0].sizeY + 1;
    int skill = rand() % 100 + 1;
    if (board[1].alive <= board[1].planeNum - 1) add_p = 7;

    if (skill > 100 - skill_probability[1] - add_p * 1)
    {
        // 15%概率触发移动飞机
        int count = 0;
        for(int j = 1; j <= board[1].sizeY; j++) {
            for(int i = 1; i <= board[1].sizeX; i++) {
                if (board[1].map[i][j][1] == 2 && board[1].map[i][j][0] == 0) {
                    str = string(1, 'A' + i - 1) + to_string(j);
                    board[1].RemovePlane(str);
                    count++;
                }
            }
        }
        int left = count, try_count = 0;
        int direction;
        while (left > 0) {
            X = rand() % board[1].sizeX;
            Y = rand() % board[1].sizeY + 1;
            str = string(1, 'A' + X) + to_string(Y);
            direction = rand() % 4 + 1;
            int found_count = 0;
            for (int i = 0; i < 9; i++) {
                if (board[1].map[X + board[1].position[direction][i][0] + 1][Y + board[1].position[direction][i][1]][0] > 0) {
                    found_count++;
                }
            }
            bool hide = false;
            for (int i = 1; i <= 9; i++) {
                if (found_count <= i && try_count >= (i - 1) * 100) {
                    hide = true;
                }
            }
            if (board[1].map[X + 1][Y][0] == 0 && hide) {
                string result = board[1].AddPlane(str, direction, overlap);
                if (result == "OK") {
                    left--; try_count = 0;
                } else {
                    try_count++;
                }
            }
            if (try_count > 10000) {
                board[1].alive++;
                timeout[1] = 1;
                return "[系统错误] 检查点：逻辑漏洞监测——随机放置飞机失败，已强制中断游戏进程！";
            }
        }
        return "【WARNING】BOSS发动技能 [空军指挥]！已指挥 " + to_string(count) + " 架飞机飞行至地图的其他位置。";
    }
    else if (skill > 100 - skill_probability[2] - add_p * 2)
    {
        // 15%概率触发连环轰炸，打击十字区域
        for (int i = 0; i < board[0].sizeX; i++) {
            str = string(1, 'A' + X) + to_string(i + 1);
            board[0].Attack(str);
            str = string(1, 'A' + i) + to_string(Y);
            board[0].Attack(str);
        }
        return "【WARNING】BOSS发动技能 [连环轰炸]！对地图行列进行打击，打击了地图上 " + string(1, 'A' + X) + to_string(Y) + " 所在的整个十字区域";
    }
    else if (skill > 100 - skill_probability[3] - add_p * 3)
    {
        // 15%概率触发雷达扫描，扫描5*5，击落所有飞机
        int count, affected_count;
        for (int attempt = 1; attempt <= 30; attempt++) {
            count = affected_count = 0;
            X = rand() % board[0].sizeX;
            Y = rand() % board[0].sizeY + 1;
            if (X == 0) X = X + 2;
            if (X == board[0].sizeX - 1) X = X - 2;
            if (Y == 1) Y = Y + 2;
            if (Y == board[0].sizeY) Y = Y - 2;
            if (X == 1) X = X + 1;
            if (X == board[0].sizeX - 2) X = X - 1;
            if (Y == 2) Y = Y + 1;
            if (Y == board[0].sizeY - 1) Y = Y - 1;
            for (int i = -2; i <= 2; i++) {
                for (int j = -2; j <= 2; j++) {
                    if (board[0].map[X + 1 + i][Y + j][0] > 0 && board[0].map[X + 1 + i][Y + j][1] == 0) {
                        affected_count++;
                    }
                }
            }
            if (affected_count <= 6 || attempt == 30) {
                for (int i = -2; i <= 2; i++) {
                    for (int j = -2; j <= 2; j++) {
                        str = string(1, 'A' + X + i) + to_string(Y + j);
                        if (board[0].map[X + 1 + i][Y + j][1] > 0 && board[0].map[X + 1 + i][Y + j][0] == 0) {
                            board[0].Attack(str);
                            count++;
                        }
                    }
                }
                break;
            }
        }
        if (count > 0) {
            return "【WARNING】 BOSS发动技能 [雷达扫描]！使用雷达扫描了 " + string(1, 'A' + X) + to_string(Y) + " 为中心的 5*5 的区域，并发射 " + to_string(count) + " 枚导弹打击了检测到的所有飞机";
        } else {
            return "【WARNING】 BOSS发动技能 [雷达扫描]！使用雷达扫描了 " + string(1, 'A' + X) + to_string(Y) + " 为中心的 5*5 的区域，什么都没有发现";
        }
    }
    else if (skill > 100 - skill_probability[4] - add_p * 4)
    {
        // 20%概率触高爆导弹，打击3*3区域
        int affected_count;
        for (int attempt = 1; attempt <= 30; attempt++) {
            affected_count = 0;
            X = rand() % board[0].sizeX;
            Y = rand() % board[0].sizeY + 1;
            if (X == 0) X++;
            if (X == board[0].sizeX - 1) X--;
            if (Y == 1) Y++;
            if (Y == board[0].sizeY) Y--;
            for (int i = -1; i <= 1; i++) {
                for (int j = -1; j <= 1; j++) {
                    if (board[0].map[X + 1 + i][Y + j][0] > 0) {
                        affected_count++;
                    }
                }
            }
            if (affected_count <= 4 || attempt == 30) {
                for (int i = -1; i <= 1; i++) {
                    for (int j = -1; j <= 1; j++) {
                        str = string(1, 'A' + X + i) + to_string(Y + j);
                        board[0].Attack(str);
                    }
                }
                break;
            }
        }
        return "【WARNING】 BOSS发动技能 [火力打击]！发射了一枚高爆导弹，炸毁了位于 " + string(1, 'A' + X) + to_string(Y) + " 的 3*3 的区域";
    }
    return "";
}