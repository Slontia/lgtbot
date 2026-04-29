
const map<string, int> position_map = {
	{"上", 1}, {"U", 1}, {"s", 1},
	{"下", 2}, {"D", 2}, {"x", 2},
	{"左", 3}, {"L", 3}, {"z", 3},
	{"右", 4}, {"R", 4}, {"y", 4},
};

class Board
{
public:
	
	// 底色（状态 类型）
	const string color[3][4] = {
		{"ECECEC","E0FFE0","E0FFE0", "4363D8"},		// 00 01 02  03
		{"B0E0FF","FFA0A0","000000", "5A5A5A"},		// 10 11 12  13
		{"A0FFA0","FF6868","FF6868", "FF6868"},		// 20 21 **  **
	};
	// 图标（本回合 类型）
	const string icon[2][4] = {
		{"<font size=7>　</font>","<font size=7>+</font>","<font size=7 color=\"FF0000\">★</font>", "<font size=7 color=\"FFE119\">⚛</font>"},						// 00 01 02  03
		{"<font color=\"FF0000\" size=6>○</font>","<font size=7>⊕</font>","<font size=7 color=\"FF0000\">✮</font>", "<font size=7 color=\"FFE119\">⚛</font>"},		// 10 11 12  13
	};
	// 机身相对飞机头坐标位置偏差（方向 机身 XY）
	vector<vector<pair<int, int>>> positions = {{},   // 1上 2下 3左 4右
		{{-2,1}, {-1,1}, {0,1}, {1,1}, {2,1}, {0,2}, {-1,3}, {0,3}, {1,3}},
		{{-2,-1}, {-1,-1}, {0,-1}, {1,-1}, {2,-1}, {0,-2}, {-1,-3}, {0,-3}, {1,-3}},
		{{1,2}, {1,1}, {1,0}, {1,-1}, {1,-2}, {2,0}, {3,1}, {3,0}, {3,-1}},
 		{{-1,2}, {-1,1}, {-1,0}, {-1,-1}, {-1,-2}, {-2,0}, {-3,1}, {-3,0}, {-3,-1}},
	};
	// 多要害模式其他飞机头相对主飞机头位置偏差
	vector<vector<pair<int, int>>> positions2 = {{}, {}, {}, {}, {}};
	int crucial_num = 1;
	pair<int, int> main_crucial{-1, -1};
	// 飞机边界相对飞机头的最大偏差
	int boundary[5] = {-1, 0, 3, 2, 2};

	// 地图大小 
	int sizeX, sizeY;
	/*
			***  地图区域  ***
		  - 状态 -   | - 类型 -
		0 - 未被打击 | 0 - 空地
		1 - 已被打击 | 1 - 机身
		2 - 侦察点   | 2 - 飞机头  3 - 特殊要害
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
		特殊机身标记(+) - 300
		飞机头标记(☆) - 200
		机身标记(—) - 0-199（保存机身标记叠加层数）
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
	// 保存需要展示机头的坐标
	vector<pair<int, int>> shown_crucials;

	// 计算自定义形状相关参数
	void CustomizeShape(const vector<string> shape) {
		// 清空positions，第0项保留为空
		for (int i = 1; i <= 4; i++) {
			positions[i].clear();
		}
		// 清空boundary
		for (int i = 1; i <= 4; i++) {
			boundary[i] = 0;
		}
		// 形状为空，返回空结果
		if (shape.size() == 0) {
			return;
		}
		pair<int, int> mainPos{-1, -1};
		int rows = shape.size();
		int cols = shape[0].size();
		// 查找靠近中间上方的 2 作为主飞机头
		for (int r = 0; r < rows; ++r) {
			if (shape[r][2] == '2') { mainPos = {r, 2}; break; }
			if (shape[r][1] == '2') { mainPos = {r, 1}; break; }
			if (shape[r][3] == '2') { mainPos = {r, 3}; break; }
			if (shape[r][0] == '2') { mainPos = {r, 0}; break; }
			if (shape[r][4] == '2') { mainPos = {r, 4}; break; }
		}
		// 如果没有找到 2，返回空结果
		if (mainPos.first == -1) {
			return;
		}
		// 计算相对坐标差
		crucial_num = 0;
		for (int r = 0; r < rows; ++r) {
			for (int c = 0; c < cols; ++c) {
				int dy = r - mainPos.first;
				int dx = c - mainPos.second;
				if (shape[r][c] == '1') {
					positions[1].push_back({dx, dy});	// 上1
					positions[2].push_back({-dx, -dy});	// 下2
					positions[3].push_back({dy, -dx});	// 左3
					positions[4].push_back({-dy, dx});	// 右4
					if (-dy > boundary[1]) boundary[1] = -dy;	// 上1
					if (dy > boundary[2]) boundary[2] = dy;		// 下2
					if (-dx > boundary[3]) boundary[3] = -dx;	// 左3
					if (dx > boundary[4]) boundary[4] = dx;		// 右4
				}
				if (shape[r][c] == '2' && (dx != 0 || dy != 0)) {
					positions2[1].push_back({dx, dy});		// 上1
					positions2[2].push_back({-dx, -dy});	// 下2
					positions2[3].push_back({dy, -dx});		// 左3
					positions2[4].push_back({-dy, dx});		// 右4
					if (-dy > boundary[1]) boundary[1] = -dy;	// 上1
					if (dy > boundary[2]) boundary[2] = dy;		// 下2
					if (-dx > boundary[3]) boundary[3] = -dx;	// 左3
					if (dx > boundary[4]) boundary[4] = dx;		// 右4
				}
			}
			crucial_num += std::count(shape[r].begin(), shape[r].end(), '2');
		}
		if (crucial_num > 1) main_crucial = mainPos;
	}

	// 初始化地图
	void InitializeMap(const bool corner)
	{
		for(int j = 0; j <= sizeY; j++) {
			for(int i = 0; i <= sizeX; i++) {
				map[i][j][0] = map[i][j][1] = body[i][j] = mark[i][j] = this_turn[i][j] = 0;
				if ((i == 1) + (j == 1) + (i == sizeX) + (j == sizeY) == 2 && corner) {
					map[i][j][0] = 1;
				}
			}
		}
	}
	
	// 图形界面
	string GetMap(const int show_planes, const int crucial_mode)
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
				bool is_shown_crucial = std::find(shown_crucials.begin(), shown_crucials.end(), std::make_pair(i, j)) != shown_crucials.end();
				bool hide_crucial = (!is_shown_crucial && crucial_mode > 0);

			 	grid[i][j] += "<td style=\"background-color:#";
			 	
			 	// 背景色
				if (show_planes || map[i][j][0] > 0) {
					if (map[i][j][1] == 2 && !show_planes && hide_crucial) {
						grid[i][j] += color[map[i][j][0]][1];
					} else {
						grid[i][j] += color[map[i][j][0]][map[i][j][1]];
					}
				} else {
					grid[i][j] += color[0][0];
				}

				if (map[i][j][1] == 2 && body[i][j] > 0 && positions2[1].size() > 0 && show_planes) {
					grid[i][j] += "; border: 1px solid red";
				}

				grid[i][j] += ";\">";
			 	
			 	// 地图符号
			 	string m;
			 	if(map[i][j][0] == 2 && map[i][j][1] == 0) {
					m = "<font size=7>-</font>";
				} else {
					if ((show_planes || map[i][j][0] > 0) &&
						(!hide_crucial || mark[i][j] != 200 || map[i][j][0] == 2 || (map[i][j][0] == 1 && map[i][j][1] >= 3)))
					{
						if (map[i][j][1] == 2 && !show_planes && hide_crucial) {
							m = icon[this_turn[i][j]][1];
						} else {
							m = icon[this_turn[i][j]][map[i][j][1]];
						}
					} else {
						// 空地标记
						if (mark[i][j] == 200) {
							m = "<font size=7 color=\"#505050\">☆</font>";
						} else if (mark[i][j] == 300) {
							m = "<font size=7 color=\"#505050\">+</font>";
						} else if (mark[i][j] > 0) {
							m = "<font size=5 color=\"#505050\">—</font>";
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
		string mapString;
		
		// 玩家昵称
		mapString += "<table style=\"text-align:center;margin:auto;\"><tbody>";
		mapString += "<tr><td>" + fill + "</td></tr>";
		mapString += "<tr><td style=\"width:900px\" bgcolor=\"#ECECEC\"><font size=6>" + MapName + "</font></td></tr>";
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
			mapString += "<tr><td><font size=7>剩余飞机数：" + to_string(planeNum - alive / crucial_num) + "</font></td></tr>";
		} else {
			if (crucial_mode == 0) {
				mapString += "<tr><td><font size=7>命中要害：" + to_string(planeNum * crucial_num - alive) + "</font></td></tr>";
			} else {
				mapString += "<tr><td><font size=7>命中要害：？？？</font></td></tr>";
			}
		}
		mapString += "<tr><td>" + fill + "</td></tr>";
		mapString += "</table>";
		
		return mapString; 
	}

	// 检查坐标是否合法
	static string CheckCoordinate(string &s)
	{
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

	// 检查地图边界
	bool CheckMapBoundary(const int X, const int Y, const int direction) const
	{
		if (direction == 1 && (X < 1 + boundary[3] || X > sizeX - boundary[4] || Y < 1 + boundary[1] || Y > sizeY - boundary[2])) return false;
		if (direction == 2 && (X < 1 + boundary[4] || X > sizeX - boundary[3] || Y < 1 + boundary[2] || Y > sizeY - boundary[1])) return false;
		if (direction == 3 && (X < 1 + boundary[1] || X > sizeX - boundary[2] || Y < 1 + boundary[4] || Y > sizeY - boundary[3])) return false;
		if (direction == 4 && (X < 1 + boundary[2] || X > sizeX - boundary[1] || Y < 1 + boundary[3] || Y > sizeY - boundary[4])) return false;
		return true;
	}
	
	// 玩家执行指令添加一架飞机
	string PlayerAddPlane(string s, const int direction, const bool overlap)
	{
		string result = CheckCoordinate(s);
		if (result != "OK") return result;

		auto pos = TranString(s);
		return AddPlane(pos.first, pos.second, direction, overlap);
	}

	// 根据坐标添加飞机
	string AddPlane(int X, int Y, const int direction, const bool overlap)
	{
		// 检查地图边界
		if (!CheckMapBoundary(X, Y, direction)) {
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
		for (auto position : positions[direction]) {
			string position_str = string(1, 'A' + X + position.first - 1) + to_string(Y + position.second);
			if (map[X + position.first][Y + position.second][1] >= 2) {
				return "[错误] 无法放置于此位置：机身不能与其他飞机头重叠。重叠位置：" + position_str;
			}
			if (map[X + position.first][Y + position.second][1] == 1 && !overlap) {
				return "[错误] 无法放置于此位置：当前规则飞机机身之间不允许重叠。重叠位置：" + position_str;
			}
		}
		for (auto position2 : positions2[direction]) {
			string position_str = string(1, 'A' + X + position2.first - 1) + to_string(Y + position2.second);
			if (map[X + position2.first][Y + position2.second][1] != 0) {
				return "[错误] 多要害模式下，附属飞机头不能与机身或其他飞机头重叠。重叠位置：" + position_str;
			}
			if (map[X + position2.first][Y + position2.second][0] == 2) {
				return "[错误] 多要害模式下，附属飞机头不能放置于侦察点。侦察点位置：" + position_str;
			}
		}
		// 放置飞机
		map[X][Y][1] = 2;
		body[X][Y] = direction;
		for (auto position : positions[direction]) {
			map[X + position.first][Y + position.second][1] = 1;
			body[X + position.first][Y + position.second] += 1;
		}
		for (auto position2 : positions2[direction]) {
			map[X + position2.first][Y + position2.second][1] = 2;
		}
		alive += crucial_num;
		return "OK";
	}

	// 玩家执行指令移除一架飞机
	string PlayerRemovePlane(string s)
	{
		string result = CheckCoordinate(s);
		if (result != "OK") return result;

		auto pos = TranString(s);
		return RemovePlane(pos.first, pos.second);
	}

	// 根据坐标移除飞机
	string RemovePlane(int X, int Y)
	{
		if (map[X][Y][1] != 2) {
			return "[错误] 移除失败：此处不存在飞机头，请输入飞机头坐标";
		}
		int direction = body[X][Y];
		if (direction == 0) {
			return "[错误] 移除失败：此位置的飞机头并非主飞机头，请输入主飞机头坐标来移除飞机（已用红框标记）";
		}
		map[X][Y][1] = body[X][Y] = 0;
		for (auto position : positions[direction]) {
			body[X + position.first][Y + position.second] -= 1;
			// 如果此位置已没有机身，则设为空地（存在重叠则不移除）
			if (body[X + position.first][Y + position.second] == 0) {
				map[X + position.first][Y + position.second][1] = 0;
			}
		}
		for (auto position2 : positions2[direction]) {
			map[X + position2.first][Y + position2.second][1] = 0;
		}
		alive -= crucial_num;
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

	// 玩家执行指令使地图被进攻（对方操作）
	string PlayerAttack(string s)
	{
		string result = CheckCoordinate(s);
		if (result != "OK") return result;

		auto pos = TranString(s);
		return Attack(pos.first, pos.second);
	}

	// 根据坐标执行进攻操作
	string Attack(int X, int Y)
	{
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
		// 特殊要害
		if (map[X][Y][1] == 3){
			return "3";
		}
		return "Empty Return";
	}

	// 结算侦察技能
	int Scout()
	{
		int scout_count = 0;
		for(int i = 1; i <= sizeX; i++) {
			for(int j = 1; j <= sizeY; j++) {
				// 如果已被打击且是机头
				if (map[i][j][0] > 0 && map[i][j][1] == 2) {
					scout_count++;
					shown_crucials.push_back({i, j});
				}
			}
		}
		return scout_count;
	}

	// 添加飞机标记
	string AddMark(string s, const int direction)
	{
		string result = CheckCoordinate(s);
		if (result != "OK") return result;
		auto pos = TranString(s);
		int X = pos.first, Y = pos.second;
		// 检查地图边界
		if (!CheckMapBoundary(X, Y, direction)) {
			return "[错误] 标记的飞机位置超出了地图范围，请检查坐标和方向是否正确";
		}
		// 设置标记
		if (mark[X][Y] != 300) mark[X][Y] = 200;
		for (auto position : positions[direction]) {
			if (mark[X + position.first][Y + position.second] < 200) {
				mark[X + position.first][Y + position.second] += 1;
			}
		}
		for (auto position2 : positions2[direction]) {
			if (mark[X + position2.first][Y + position2.second] != 300) {
				mark[X + position2.first][Y + position2.second] = 200;
			}
		}
		return "OK";
	}

	// 移除飞机标记
	string RemoveMark(string s, const int direction)
	{
		string result = CheckCoordinate(s);
		if (result != "OK") return result;
		auto pos = TranString(s);
		int X = pos.first, Y = pos.second;
		// 检查地图边界
		if (!CheckMapBoundary(X, Y, direction)) {
			return "[错误] 移除指定的坐标位置超出了地图范围，请检查坐标和方向是否正确";
		}
		if (mark[X][Y] != 300) mark[X][Y] = 0;
		for (auto position : positions[direction]) {
			if (mark[X + position.first][Y + position.second] < 200 && mark[X + position.first][Y + position.second] > 0) {
				mark[X + position.first][Y + position.second] -= 1;
			}
		}
		for (auto position2 : positions2[direction]) {
			if (mark[X + position2.first][Y + position2.second] != 300) {
				mark[X + position2.first][Y + position2.second] = 0;
			}
		}
		return "OK";
	}

	// 清空全部标记
	void RemoveAllMark()
	{
		for(int i = 1; i <= sizeX; i++) {
			for(int j = 1; j <= sizeY; j++) {
				if (mark[i][j] != 300) {
					mark[i][j] = 0;
				}
			}
		}
	}

	static string GetPlaneTable(const vector<string> shape, const int direction, const pair<int, int> pos = {-1, -1}) {
		const vector<string> components = {
			"<td style=\"background-color:#ECECEC; width:25px;\">　</td>",
			"<td style=\"background-color:#E0FFE0; width:25px;\">+</td>",
			"<td style=\"background-color:#000000; width:25px;\"><font color=\"FF0000\">★</font></td>",
			"<td style=\"background-color:#5A5A5A; width:25px;\"><font color=\"FFE119\">⚛</font></td>",
			"<td style=\"background-color:#E0FFE0; width:25px;\">①</td>",
			"<td style=\"background-color:#E0FFE0; width:25px;\">②</td>",
			"<td style=\"background-color:#E0FFE0; width:25px;\">③</td>",
			"<td style=\"background-color:#E0FFE0; width:25px;\">④</td>",
			"<td style=\"background-color:#E0FFE0; width:25px;\">�</td>",
		};

		string table = "<table style=\"text-align:center; margin:auto\">";
		int rows = shape.size();
		int cols = shape[0].size();
		for (int r = 0; r < rows; ++r) {
			table += "<tr>";
			for (int c = 0; c < cols; ++c) {
				int trans_r, trans_c;
				if (direction == 1) {
					trans_r = r;
					trans_c = c;
				}
				if (direction == 2) {
					trans_r = rows - r - 1;
					trans_c = cols - c - 1;
				}
				if (direction == 3) {
					trans_r = c;
					trans_c = rows - r - 1;
				}
				if (direction == 4) {
					trans_r = cols - c - 1;
					trans_c = r;
				}
				if (trans_r == pos.first && trans_c == pos.second) {
					table += "<td style=\"background-color:#000000; width:25px; border: 1px solid red;\"><font color=\"FF0000\">★</font></td>";
				} else {
					table += components[shape[trans_r][trans_c] - '0'];
				}
			}
			table += "</tr>";
		}
		table += "</table>";

		return table;
	}

	string GetAllDirectionTable(const vector<string> shape) {
		string table = "<table style=\"text-align:center; margin:auto\">";
		table += "<tr>";
		table += "<td>" + GetPlaneTable(shape, 1, main_crucial) + "</td><td><font size=4>　</font></td>";
		table += "<td>" + GetPlaneTable(shape, 2, main_crucial) + "</td>";
		table += "</tr><tr>";
		table += "<td><font size=5>上</font></td><td/>";
		table += "<td><font size=5>下</font></td>";
		table += "</tr>";

		table += "<tr>";
		table += "<td>" + GetPlaneTable(shape, 3, main_crucial) + "</td><td><font size=4>　</font></td>";
		table += "<td>" + GetPlaneTable(shape, 4, main_crucial) + "</td>";
		table += "</tr><tr>";
		table += "<td><font size=5>左</font></td><td/>";
		table += "<td><font size=5>右</font></td>";
		table += "</tr>";
		table += "</table>";
		return table;
	}

};
