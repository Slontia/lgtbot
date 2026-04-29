
enum class Direct { UP = 0, DOWN = 1, LEFT = 2, RIGHT = 3 };

const map<string, Direct> direction_map = {
    {"上", Direct::UP}, {"U", Direct::UP}, {"s", Direct::UP},
	{"下", Direct::DOWN}, {"D", Direct::DOWN}, {"x", Direct::DOWN},
	{"左", Direct::LEFT}, {"L", Direct::LEFT}, {"z", Direct::LEFT},
	{"右", Direct::RIGHT}, {"R", Direct::RIGHT}, {"y", Direct::RIGHT},
};

class Player
{
  public:
    Player(const string &name, const string &avatar) : name(name), avatar(avatar) {}

    void SetPos(const pair<int, int> pos) {
        this->x = pos.first;
        this->y = pos.second;
    }

    const string name;
    const string avatar;
    int x, y;
};


class Grid
{
  public:
    template <Direct direct>
    void SetWall(bool has_wall) { wall[static_cast<int>(direct)] = has_wall; }

    template <Direct direct>
    bool Wall() const { return wall[static_cast<int>(direct)]; }

  private:
	// 四周墙面（上/下/左/右）
	bool wall[4] = {false, false, false, false};
};


class Board
{
  public:
    // 地图大小
    int size = 5;
	// 玩家
    vector<Player> players;
    // 地图
    vector<vector<Grid>> grid_map;

    // 初始化地图
    void Initialize()
    {
        players[0].SetPos({0, size - 1});
        players[1].SetPos({size - 1, 0});
        grid_map.resize(size);
        for (int i = 0; i < size; i++) {
            grid_map[i].resize(size);
        }
        InitializeBoundary();   // 初始化边界
    }

    // 获取玩家和地图的markdown字符串
    string GetMarkdown(const int round, const PlayerID currentPlayer) const
    {
        return GetPlayerTable(round, currentPlayer) + GetBoard();
    }

    // 获取地图html
    string GetBoard() const
    {
        html::Table map(size * 2 + 3, size * 2 + 3);
        map.SetTableStyle("align=\"center\" cellpadding=\"0\" cellspacing=\"0\" style=\"border-collapse: collapse;\"");
        // 坐标
        for (int i = 2; i < size * 2 + 2; i = i + 2) {
            int n = i / 2 - 1;
            map.Get(i, 0).SetStyle("class=\"pos\"").SetContent(char('A' + n));
            map.Get(i, size * 2 + 2).SetStyle("class=\"pos\"").SetContent(char('A' + n));
            map.Get(0, i).SetStyle("class=\"pos\"").SetContent(to_string(n + 1));
            map.Get(size * 2 + 2, i).SetStyle("class=\"pos\"").SetContent(to_string(n + 1));
        }
        // 方格和玩家
        for (int x = 2; x < size * 2 + 1; x = x + 2) {
            for (int y = 2; y < size * 2 + 1; y = y + 2) {
                int gridX = x/2-1;
                int gridY = y/2-1;
                map.Get(x, y).SetStyle("class=\"grid\"");
                for (int pid = 0; pid < 2; pid++) {
                    if (players[pid].x == gridX && players[pid].y == gridY) {
                        if (players[pid].name == "机器人0号") {
                            map.Get(x, y).SetContent("电脑");
                        } else {
                            map.Get(x, y).SetContent(players[pid].avatar);
                        }
                    }
                }
            }
        }
        // 纵向围墙
        for (int x = 2; x < size * 2 + 1; x = x + 2) {
            for (int y = 1; y < size * 2; y = y + 2) {
                if (grid_map[x/2-1][y/2].Wall<Direct::LEFT>()) {
                    map.Get(x, y).SetStyle("class=\"wall-col\"").SetColor("black");
                }
            }
            if (grid_map[x/2-1][size-1].Wall<Direct::RIGHT>()) {
                map.Get(x, size*2+1).SetStyle("class=\"wall-col\"").SetColor("black");
            }
        }
        // 横向围墙
        for (int y = 2; y < size * 2 + 1; y = y + 2) {
            for (int x = 1; x < size * 2; x = x + 2) {
                if (grid_map[x/2][y/2-1].Wall<Direct::UP>()) {
                    map.Get(x, y).SetStyle("class=\"wall-row\"").SetColor("black");
                }
            }
            if (grid_map[size-1][y/2-1].Wall<Direct::DOWN>()) {
                map.Get(size*2+1, y).SetStyle("class=\"wall-row\"").SetColor("black");
            }
        }
        // 角落方块
        for (int x = 1; x < size * 2 + 2; x = x + 2) {
            for (int y = 1; y < size * 2 + 2; y = y + 2) {
                map.Get(x, y).SetStyle("class=\"corner\"").SetColor("black");
            }
        }
        return style + map.ToString();
    }

    // 获取玩家信息
    string GetPlayerTable(const int round, const PlayerID currentPlayer) const
    {
        html::Table playerTable(2, 3);
        playerTable.SetTableStyle("align=\"center\" cellpadding=\"2\"");
        if (currentPlayer == 0) playerTable.Get(0, 1).SetColor("gainsboro");
        if (currentPlayer == 1) playerTable.Get(1, 1).SetColor("gainsboro");
        for (int pid = 0; pid < 2; pid++) {
            playerTable.Get(pid, 0).SetStyle("style=\"width:40px;\"").SetContent(players[pid].avatar);
            playerTable.Get(pid, 1).SetStyle("style=\"width:300px; text-align:left;\"").SetContent(players[pid].name);
            playerTable.Get(pid, 2).SetStyle("style=\"width:40px;\"");
            if (currentPlayer == 2) {
                playerTable.Get(pid, 2).SetColor("gainsboro").SetContent(to_string(pid == 0 ? GetAreaSize().first : GetAreaSize().second));
            }
        }
        return "### 第 " + to_string(round) + " 回合" + playerTable.ToString();
    }

    // 检查坐标是否合法
	static string CheckCoordinate(string &s)
	{
		// 长度必须为2
		if (s.length() != 2 && s.length() != 3) {
			return "[错误] 输入的坐标长度只能为 2 或 3，如：A1";
		}
		// 大小写不敏感 
		if (s[0] <= 'z' && s[0] >= 'a') {
			s[0] = s[0] - 'a' + 'A';
		}
		// 检查是否为合法输入 
		if (s[0] > 'Z' || s[0] < 'A' || s[1] > '9' || s[1] < '0') {
			return "[错误] 请输入合法的坐标（字母+数字），如：A1";
		}
		if (s.length() == 3 && (s[2] > '9' || s[2] < '0')) {
			return "[错误] 请输入合法的坐标（字母+数字），如：A1";
		}
		return "OK";
	}

	// 将字符串转为一个位置pair。必须确保字符串是合法的再执行这个操作。 
	static pair<int, int> TranString(string s)
	{
		int nowX = s[0] - 'A', nowY = s[1] - '0' - 1; 
		if (s.length() == 3)
		{
			nowY = (s[1] - '0') * 10 + s[2] - '0' - 1;
		}
		pair<int, int> ret;
		ret.first = nowX;
		ret.second = nowY;
		return ret;
	}

    // 检查移动（返回所需要的步数，无法抵达返回-1）
    int CheckMoveStep(const PlayerID pid, const pair<int, int> target) const
    {
        int sx = players[pid].x, sy = players[pid].y;
        int ex = target.first, ey = target.second;
        const int dx[4] = {-1, 1, 0, 0};
        const int dy[4] = {0, 0, -1, 1};
    
        // 某格是否有玩家
        auto hasPlayer = [&](int x, int y) {
            for (int i = 0; i < 2; i++) {
                if (i != pid && players[i].x == x && players[i].y == y) return true;
            }
            return false;
        };
    
        vector<vector<bool>> vis(size, vector<bool>(size, false));
        queue<tuple<int, int, int>> q;
        q.emplace(sx, sy, 0);
        vis[sx][sy] = true;
    
        auto mod = [&](int v) {
            return (v + size) % size;
        };
    
        while (!q.empty()) {
            auto [x, y, d] = q.front(); q.pop();
            if (x == ex && y == ey) return d;
    
            for (int dir = 0; dir < 4; dir++) {
                int nx = mod(x + dx[dir]);
                int ny = mod(y + dy[dir]);
                if (!IsConnected(x, y, dx[dir], dy[dir])) continue;
    
                if (!hasPlayer(nx, ny)) {
                    if (!vis[nx][ny]) {
                        vis[nx][ny] = true;
                        q.emplace(nx, ny, d + 1);
                    }
                } else {
                    int jx = mod(nx + dx[dir]);
                    int jy = mod(ny + dy[dir]);
                    if (IsConnected(nx, ny, dx[dir], dy[dir]) && !hasPlayer(jx, jy)) {
                        if (!vis[jx][jy]) {
                            vis[jx][jy] = true;
                            q.emplace(jx, jy, d + 1);
                        }
                    } else {
                        vector<int> perp;
                        if (dir < 2) perp = {2, 3};
                        else perp = {0, 1};
    
                        for (int pd : perp) {
                            int px = mod(nx + dx[pd]);
                            int py = mod(ny + dy[pd]);
                            if (IsConnected(nx, ny, dx[pd], dy[pd]) && !hasPlayer(px, py)) {
                                if (!vis[px][py]) {
                                    vis[px][py] = true;
                                    q.emplace(px, py, d + 1);
                                }
                            }
                        }
                    }
                }
            }
        }
        return -1;
    }
    
    // 检查是否存在墙壁
    bool CheckWall(const pair<int, int> pos, const Direct direction) const
    {
        switch (direction) {
            case Direct::UP:    return grid_map[pos.first][pos.second].Wall<Direct::UP>();
            case Direct::DOWN:  return grid_map[pos.first][pos.second].Wall<Direct::DOWN>();
            case Direct::LEFT:  return grid_map[pos.first][pos.second].Wall<Direct::LEFT>();
            case Direct::RIGHT: return grid_map[pos.first][pos.second].Wall<Direct::RIGHT>();
            default: return true;
        }
    }

    // 执行移动和放置墙壁
    void MoveAndPlace(const PlayerID pid, const pair<int, int> pos, const Direct direction)
    {
        players[pid].SetPos(pos);
        switch (direction) {
            case Direct::UP:
                grid_map[pos.first][pos.second].SetWall<Direct::UP>(true);
                grid_map[pos.first - 1][pos.second].SetWall<Direct::DOWN>(true);
                break;
            case Direct::DOWN:
                grid_map[pos.first][pos.second].SetWall<Direct::DOWN>(true);
                grid_map[pos.first + 1][pos.second].SetWall<Direct::UP>(true);
                break;
            case Direct::LEFT:
                grid_map[pos.first][pos.second].SetWall<Direct::LEFT>(true);
                grid_map[pos.first][pos.second - 1].SetWall<Direct::RIGHT>(true);
                break;
            case Direct::RIGHT:
                grid_map[pos.first][pos.second].SetWall<Direct::RIGHT>(true);
                grid_map[pos.first][pos.second + 1].SetWall<Direct::LEFT>(true);
                break;
            default:;
        }
    }

    // 判断游戏是否结束
    bool IsGameOver() const
    {
        const int dx[4] = {-1, 1, 0, 0};
        const int dy[4] = {0, 0, -1, 1};

        int sx = players[0].x, sy = players[0].y;
        int tx = players[1].x, ty = players[1].y;

        vector<vector<bool>> vis(size, vector<bool>(size, false));
        queue<pair<int, int>> q;
        q.emplace(sx, sy);
        vis[sx][sy] = true;

        auto mod = [&](int v) { return (v + size) % size; };

        while (!q.empty()) {
            auto [x, y] = q.front(); q.pop();
            if (x == tx && y == ty) {
                return false;
            }
            for (int dir = 0; dir < 4; dir++) {
                int nx = mod(x + dx[dir]);
                int ny = mod(y + dy[dir]);
                if (!vis[nx][ny] && IsConnected(x, y, dx[dir], dy[dir])) {
                    vis[nx][ny] = true;
                    q.emplace(nx, ny);
                }
            }
        }
        return true;
    }

    // 计算双方玩家所在区域大小
    pair<int, int> GetAreaSize() const
    {
        const int dx[4] = {-1, 1, 0, 0};
        const int dy[4] = {0, 0, -1, 1};
        auto mod = [&](int v) { return (v + size) % size; };

        auto bfs_count = [&](int pid) {
            int sx = players[pid].x, sy = players[pid].y;
            vector<vector<bool>> vis(size, vector<bool>(size, false));
            queue<pair<int, int>> q;
            q.emplace(sx, sy);
            vis[sx][sy] = true;
            int cnt = 1;

            while (!q.empty()) {
                auto [x, y] = q.front(); q.pop();
                for (int dir = 0; dir < 4; dir++) {
                    int nx = mod(x + dx[dir]);
                    int ny = mod(y + dy[dir]);
                    if (!vis[nx][ny] && IsConnected(x, y, dx[dir], dy[dir])) {
                        vis[nx][ny] = true;
                        cnt++;
                        q.emplace(nx, ny);
                    }
                }
            }
            return cnt;
        };

        int size0 = bfs_count(0);
        int size1 = bfs_count(1);
        return { size0, size1 };
    }

  private:
    // 初始化边界
    void InitializeBoundary()
    {
        for (int x = 0; x < size; x++) {
            for (int y = 0; y < size; y++) {
                if (y == size - 1) {
                    grid_map[x][y].SetWall<Direct::RIGHT>(true);
                }
                if (y == 0) {
                    grid_map[x][y].SetWall<Direct::LEFT>(true);
                }
                if (x == size - 1) {
                    grid_map[x][y].SetWall<Direct::DOWN>(true);
                }
                if (x == 0) {
                    grid_map[x][y].SetWall<Direct::UP>(true);
                }
            }
        }
    }

    // 判断(x, y)格子沿(dx, dy)方向是否连通
    bool IsConnected(int x, int y, int dx, int dy) const
    {
        int nx = (x + dx + size) % size;
        int ny = (y + dy + size) % size;
        if (dx == -1 && dy == 0)
            return !grid_map[x][y].Wall<Direct::UP>() && !grid_map[nx][ny].Wall<Direct::DOWN>();
        else if (dx == 1 && dy == 0)
            return !grid_map[x][y].Wall<Direct::DOWN>() && !grid_map[nx][ny].Wall<Direct::UP>();
        else if (dx == 0 && dy == -1)
            return !grid_map[x][y].Wall<Direct::LEFT>() && !grid_map[nx][ny].Wall<Direct::RIGHT>();
        else if (dx == 0 && dy == 1)
            return !grid_map[x][y].Wall<Direct::RIGHT>() && !grid_map[nx][ny].Wall<Direct::LEFT>();
        return false;
    }
    
    inline static constexpr const char* style = R"(
<style>
    .grid {
        width: 50px;
        height: 50px;
    }
    .wall-row {
        width: 50px;
        height: 9px;
    }
    .wall-col {
        width: 9px;
        height: 50px;
    }
    .corner {
        width: 9px;
        height: 9px;
    }
    .pos {
        font-size: 25px;
        width: 50px;
        height: 50px;
    }
    .grid, .wall-row, .wall-col, .corner {
        border: 1px solid #000;
        box-sizing: border-box;
    }
</style>
)";
};
