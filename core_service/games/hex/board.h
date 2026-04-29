#include <stack>

const string color_en[3] = {"gainsboro", "red", "#0078FF"};
const string color_ch[3] = {"灰", "红", "蓝"};

class Board
{
public:
	// 玩家昵称
	string name[2];
	// 颜色对应的玩家编号
	PlayerID player_color[2];
	// 棋盘边长
	int size = 9;
	// 棋盘
	int chess[30][30];

	void Initialize() {
		for(int i = 0; i < size; i++) {
			for(int j = 0; j < size; j++) {
				chess[i][j] = 0;
			}
		}
	}

	// 六边形样式
	inline static const string style = R"EOF(
<style>
    .hex {
		width: 50px;
		height: 86.6px;
		position: relative;
		margin: -1px 13px;
		display: block;
		border-top: 1px solid black;
		border-bottom: 1px solid black;
	}
	.hex::after {
		position: absolute;
		content: '';
		width: 50px;
		height: 86.6px;
		transform: rotate(60deg);
		border-top: 1px solid black;
		border-bottom: 1px solid black;
		background-color: inherit;
	}
	.hex::before {
		position: absolute;
		content: '';
		width: 50px;
		height: 86.6px;
		transform: rotate(-60deg);
		border-top: 1px solid black;
		border-bottom: 1px solid black;
		background-color: inherit;
	}
	.content {
		position: absolute;
		top: 50%;
		left: 50%;
		transform: translate(-50%, -50%);
		z-index: 1;
		font-size: 35px;
		color: black;
	}
    .c0{
        background-color: gainsboro;
    }
    .c1{
        background-color: red;
    }
    .c2{
        background-color: #0078FF;
    }
    .board {
        display: flex;
        margin: 30px;
    }
    .col {
        display: flex;
        flex-direction: column;
        justify-content: center;
    }
</style>)EOF";

	// 构造UI 
	string GetUI(int currentPlayer) {
		string UI = style;

		// 玩家信息
		string bg0 = "", bg1 = "";
		if (currentPlayer == 0) bg0 = " bgcolor=\"" + color_en[0] + "\"";
		if (currentPlayer == 1) bg1 = " bgcolor=\"" + color_en[0] + "\"";
		UI += "<table style=\"text-align:center;margin:auto;\"><tr>";
		UI += "<td style=\"width:50px; height:50px; background-color:" + color_en[player_color[0]] + ";\">　</td><td" + bg0 + "><font size=3>　</font><font size=6>" + name[0] + "</font></td>";
		UI += "<td style=\"width:80px;\">　</td>";
		UI += "<td style=\"width:50px; height:50px; background-color:" + color_en[player_color[1]] + ";\">　</td><td" + bg1 + "><font size=3>　</font><font size=6>" + name[1] + "</font></td></tr>";
		UI += "</table>";

		// 棋盘
		UI += "<div class=\"board\">";
		UI += "<div class=\"col\">";
		UI += "<div class=\"hex c2\"></div>";
		UI += "</div>";
		UI += "<div class=\"col\">";
		UI += "<div class=\"hex c1\"></div>";
		UI += "<div class=\"hex c2\"></div>";
		UI += "</div>";
		// 根据对角线遍历
		int num = 1;
		for (int d = 0; d < 2 * size - 1; ++d) {
			UI += "<div class=\"col\">";
			if (d <= size - 1) {
				UI += "<div class=\"hex c1\"></div>";
			} else {
				UI += "<div class=\"hex c2\"></div>";
			}
			for (int i = 0; i <= d; ++i) {
				int j = d - i;
				if (i < size && j < size) {
					UI += "<div class=\"hex c" + to_string(chess[i][j]) + "\"><div class=\"content\">" + to_string(num++) + "</div></div>";
				}
			}
			if (d < size - 1) {
				UI += "<div class=\"hex c2\"></div>";
			} else {
				UI += "<div class=\"hex c1\"></div>";
			}
			UI += "</div>";
		}
		UI += "<div class=\"col\">";
		UI += "<div class=\"hex c2\"></div>";
		UI += "<div class=\"hex c1\"></div>";
		UI += "</div>";
		UI += "<div class=\"col\">";
		UI += "<div class=\"hex c2\"></div>";
		UI += "</div>";
		UI += "</div>";

		return UI;
	}

	// 将数字位置转为一个位置pair
	pair<int, int> TranNum(int num)
	{
		int c = 1;
		for (int d = 0; d < 2 * size - 1; ++d) {
			for (int i = 0; i <= d; ++i) {
				int j = d - i;
				if (i < size && j < size) {
					if (c == num) {
						return {i, j};
					}
					c++;
				}
			}
		}
		return {-1, -1};
	}

	string PlaceChess(int num, PlayerID player)
	{
		if (num > size * size || num < 1) {
			return "[错误] 数字位置超出棋盘大小";
		}
		auto pos = TranNum(num);
		int x = pos.first, y = pos.second;
		if (x == -1) {
			return "[错误] 数字坐标转换时发生了未知错误，请联系管理员中断游戏！";
		}
		if(chess[x][y] != 0) {
			return "[错误] 这个位置已经有棋子了";
		}
		chess[x][y] = player_color[player];
		return "OK"; 
	}

	void SwapColor() {
		player_color[0] = 3 - player_color[0];
		player_color[1] = 3 - player_color[1];
	}

	int WinCheck() {
        // 检查 1红色 是否连通上下两端
        for (int col = 0; col < size; ++col) {
            if (chess[0][col] == 1 && dfs(1, 0, col)) {
                return 1;
            }
        }
        // 检查 2蓝色 是否连通左右两端
        for (int row = 0; row < size; ++row) {
            if (chess[row][0] == 2 && dfs(2, row, 0)) {
                return 2;
            }
        }
        return -1;
    }

    bool dfs(int player, int x, int y) {
        bool visited[30][30] = {false};
        stack<pair<int, int>> s;
        s.push({x, y});
        visited[x][y] = true;

        while (!s.empty()) {
            auto [cx, cy] = s.top(); s.pop();

            // 检查是否到达目标边界
            if (player == 1 && cx == size - 1) return true; // 1红色 连接上下两端
            if (player == 2 && cy == size - 1) return true; // 2蓝色 连接左右两端

            // 遍历六个相邻格子
            for (auto [nx, ny] : getNeighbors(cx, cy)) {
                if (!visited[nx][ny] && chess[nx][ny] == player) {
                    visited[nx][ny] = true;
                    s.push({nx, ny});
                }
            }
        }

        return false;
    }

    vector<pair<int, int>> getNeighbors(int x, int y) {
        vector<pair<int, int>> neighbors;
        vector<pair<int, int>> directions = {
            {-1, 0}, {1, 0}, {0, -1}, {0, 1}, {-1, 1}, {1, -1}
        };
        for (auto [dx, dy] : directions) {
            int nx = x + dx, ny = y + dy;
            if (nx >= 0 && nx < size && ny >= 0 && ny < size) {
                neighbors.push_back({nx, ny});
            }
        }
        return neighbors;
    }

};
