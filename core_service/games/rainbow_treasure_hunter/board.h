
#include <random>
#include <set>

struct Map {
    int size;
    int treasure;
    int bomb;
    int special;
    int hp;
    int limit;
};
const Map maps[] = {
    {  6,  6,  4,  1,  1,  4 },     // 小地图
    {  9,  9, 12,  2,  2,  6 },     // 中地图
    { 12, 18, 18,  4,  2,  9 },     // 大地图
    { 15, 36, 24,  6,  2, 12 },     // 特大地图
};

struct ItemNumOption {
    int treasure;
    int bomb;
    int humidifier = 0;
    int ink = 0;

    int operator[](size_t idx) const
    {
        switch (idx) {
            case 0:  return treasure;
            case 1:  return bomb;
            case 2:  return humidifier;
            case 3:  return ink;
            default: return -1;
        }
    }
};


class Player
{
  public:
    Player(const string &name, const string &avatar, const int &hp) : name(name), avatar(avatar), hp(hp) {}

    const string name;
    const string avatar;
    
    int hp;
    int score = 0;
    int change_hp = 0;
    int change_score = 0;

    bool has_dug = false;
    pair<int, int> current;
};


const int k_color_count = 6;
const int k_item_count = 4;

enum class GridType {
    R, B, Y, O, G, P,
    TREASURE,
    BOMB,
    HUMIDIFIER,
    INK,
};

const vector<string> GridTypeNames = {
    "R", "B", "Y", "O", "G", "P",
    "<font color='#00C8C8'>★</font>",
    "✹",
    "♨",
    "⚗",
};
string toString(GridType type)
{
    int index = static_cast<int>(type);
    if (index >= 0 && index < GridTypeNames.size()) {
        return GridTypeNames[index];
    }
    return "UNKNOWN";
}

class Grid
{
  public:
    string GetClasses(const bool show_all, const bool stain = false) const
    {
        if (!show && !show_all) return "hidden";

        string base = dig_count > 0 ? "current" : (extra_mark ? "extra" : "show");
        if (!IsEmpty()) {
            return base;
        } else if (stain && !show_all) {
            return base + " stain";
        } else {
            return base + " color-" + toString(type);
        }
    }

    string GetContent(const bool show_all, const bool stain = false) const
    {
        if (!show && !show_all) return "";
        
        if (show_all && type == GridType::HUMIDIFIER) {
            return 
                "<div class='item-box'>"
                    "<span>" + toString(type) + "</span>"
                    "<span class='item-badge'>" + to_string(humidifier_add) + "</span>"
                "</div>";
        } else if (!IsEmpty()) {
            return toString(type);
        } else if (stain && !show_all) {
            return "▒▒";
        } else {
            return toString(type) + to_string(num);
        }
    }

    bool IsEmpty() const { return static_cast<int>(type) < k_color_count; }

  private:
	GridType type;
    int num = 0;
    int humidifier_add = 0;

    bool show = false;
    int dig_count = 0;
    bool extra_mark = false;

    friend class Board;
};


class Board
{
  public:
    Board(string ResourceDir, const int playerNum) : resource_path_(std::move(ResourceDir)), playerNum(playerNum) {}
    // 图片资源文件夹
    const string resource_path_;

    // 地图初始配置
    int size;
	// 玩家
    const int playerNum;
    vector<Player> players;
    // 地图
    vector<vector<Grid>> grid_map;

    // 道具模式
    int item_mode;

    // 初始化地图
    void Initialize(
        const uint32_t map_option,
        const vector<int>& weights,
        const int32_t item_mode,
        const int32_t size_option,
        const ItemNumOption& options
    ) {
        ItemNumOption init_options;
        this->item_mode = item_mode;
        size = size_option >= 0 ? size_option : maps[map_option].size;
        init_options.treasure  = (options.treasure >= 0 ? options.treasure : maps[map_option].treasure);
        init_options.bomb      = (options.bomb     >= 0 ? options.bomb     : maps[map_option].bomb);

        if (item_mode == 1) {
            std::vector<Item> items = {
                { GridType::HUMIDIFIER, options.humidifier },
                { GridType::INK,        options.ink        },
            };
            AssignItemCounts(items, maps[map_option].special);
            init_options.humidifier = items[0].actual;
            init_options.ink        = items[1].actual;
        }

        grid_map.resize(size);
        for (int i = 0; i < size; i++) {
            grid_map[i].resize(size);
        }
        GenerateRandomMap(weights, init_options);
    }

    // 获取玩家和地图的markdown字符串
    string GetMarkdown(const int round, const bool show_all = false) const
    {
        return GetHeadTable(round) + GetBoard(show_all) + GetButtomTable();
    }

    // 获取顶部表格
    string GetHeadTable(const int round) const
    {
        html::Table headTable(playerNum + 1, 7);
        headTable.SetTableStyle("align=\"center\" cellpadding=\"2\"");
        headTable.SetRowStyle(0, "bgcolor=\"E7E7E7\" style=\"text-align:center; font-size:12px; font-weight:bold;\"");
        headTable.MergeRight(0, 0, 2);
        headTable.Get(0, 0).SetStyle("style=\"width:280px;\"").SetContent("玩家");
        headTable.Get(0, 2).SetStyle("style=\"width:80px;\"").SetContent(HTML_COLOR_FONT_HEADER(#BD8900) "【总得分】" HTML_FONT_TAIL);
        headTable.MergeRight(0, 3, 2);
        headTable.Get(0, 3).SetStyle("style=\"width:32px;\"").SetContent("生命值");
        headTable.Get(0, 5).SetStyle("style=\"width:40px;\"").SetContent("选择");
        headTable.Get(0, 6).SetStyle("style=\"width:50px;\"").SetContent("状态变化");
        for (int pid = 0; pid < playerNum; pid++) {
            const Player& p = players[pid];
            headTable.Get(pid + 1, 0).SetStyle("style=\"width:40px;\"").SetContent(p.avatar);
            headTable.Get(pid + 1, 1).SetStyle("style=\"width:240px; text-align:left;\"").SetContent(HTML_SIZE_FONT_HEADER(2) + p.name + HTML_FONT_TAIL);
            headTable.Get(pid + 1, 2).SetStyle("style=\"width:80px;\"").SetContent(HTML_COLOR_FONT_HEADER(#BD8900) "<b>" + to_string(p.score) + "</b>" HTML_FONT_TAIL);
            // 生命值
            if (p.hp > 0) {
                headTable.Get(pid + 1, 3).SetStyle("style=\"width:20px; text-align:right;\"").SetContent("<img src='file:///" + resource_path_ + "hp.png'>");
                headTable.Get(pid + 1, 4).SetStyle("style=\"width:12px; text-align:left;\"").SetContent(HTML_COLOR_FONT_HEADER(#EC4646) + to_string(p.hp) + HTML_FONT_TAIL);
            } else {
                headTable.MergeRight(pid + 1, 3, 2);
                headTable.Get(pid + 1, 3).SetStyle("style=\"width:32px;\"").SetColor("#E5E5E5").SetContent("淘汰");
            }
            headTable.Get(pid + 1, 5).SetStyle("style=\"width:40px;\"").SetContent(p.has_dug ? string(1, 'A' + p.current.first) + to_string(p.current.second + 1) : "--");
            // 状态变化
            auto& cell = headTable.Get(pid + 1, 6);
            cell.SetStyle("style=\"width:50px;\"");
            if (p.change_hp != 0) {
                bool isPositive = p.change_hp > 0;
                string color = isPositive ? "#2E7D32" : "#C62828";
                string value = (isPositive ? "+" : "") + to_string(p.change_hp);
                string image = "<img src='file:///" + resource_path_ + "hp.png' width='12'>";
                string html = "<font color=\"" + color + "\">" + value + image + "</font>";
                cell.SetColor(isPositive ? "#D6F5D6" : "#FAD6D6").SetContent(html);
            } else if (p.change_score != 0) {
                bool isPositive = p.change_score > 0;
                string color = isPositive ? "#2E7D32" : "#C62828";
                string value = (isPositive ? "+" : "") + to_string(p.change_score);
                string html = "<font color=\"" + color + "\">" + value + "</font>";
                cell.SetColor(isPositive ? "#D6F5D6" : "#FAD6D6").SetContent(html);
            }
        }
        return "### 第 " + to_string(round) + " 回合" + headTable.ToString();
    }

    // 获取地图html
    string GetBoard(bool show_all = false) const
    {
        html::Table map(size + 2, size + 2);
        map.SetTableStyle("align=\"center\" style=\"border-collapse: collapse;\"");
        // 坐标
        for (int i = 1; i < size + 1; i++) {
            map.Get(i, 0).SetStyle("class=\"pos\"").SetContent(char('A' + i - 1));
            map.Get(i, size + 1).SetStyle("class=\"pos\"").SetContent(char('A' + i - 1));
            map.Get(0, i).SetStyle("class=\"pos\"").SetContent(to_string(i));
            map.Get(size + 1, i).SetStyle("class=\"pos\"").SetContent(to_string(i));
        }
        // 方格
        for (int x = 0; x < size; x++) {
            for (int y = 0; y < size; y++) {
                const Grid grid = grid_map[x][y];
                bool stain = CountSurroundingHiddenInks(x, y) > 0;
                map.Get(x + 1, y + 1).SetStyle("class=\"grid " + grid.GetClasses(show_all, stain) + "\"").SetContent(grid.GetContent(show_all, stain));
            }
        }
        return style + map.ToString();
    }

    // 获取底部图例
    string GetButtomTable() const
    {
        int table_rows = item_mode ? k_color_count + k_item_count - 2 : k_color_count;
        html::Table buttomTable(table_rows, 4);
        buttomTable.SetTableStyle("align=\"center\" style=\"font-size:12px;\"");

        struct InfoColor {
            string fontColor;
            string bgColor;
            string label;
            string description;
        };
        const InfoColor infoColor[] = {
            { "#C91D32", "#FADADE", "R 红色", "统计周围 8 格的炸弹总数" },
            { "#0070C0", "#D9E1F4", "B 蓝色", "统计本行本列的炸弹总数" },
            { "#BD8900", "#FFF3CA", "Y 黄色", "统计周围 8 格的宝藏总数" },
            { "#C55C10", "#F9CBAA", "O 橙色", "统计周围 8 格的宝藏和炸弹总数" },
            { "#588E31", "#E3F2D9", "G 绿色", "统计周围 8 格以及本行本列的宝藏总数（不重复计算）" },
            { "#7030A0", "#F6CCFF", "P 紫色", "统计周围 8 格以及本行本列的炸弹总数（不重复计算）" },
        };
        int row = 0;
        for (int i = 0; i < k_color_count; i++) {
            string html = "<font color='" + infoColor[i].fontColor + "'>" +
                            "<span style='background:" + infoColor[i].bgColor + "'>" + infoColor[i].label + "</span>：" +
                            infoColor[i].description + "</font>";
            buttomTable.Get(row++, 0).SetStyle("style=\"width:350px; text-align:left\"").SetContent(html);
        }
        if (item_mode > 0) {
            string special[] = {
                "♨ 加湿器：周围8格的颜色会增加数值 1-3，且增加数值相同。",
                "⚗ 墨水瓶：<b>挖到的人平分 5 分。</b>周围8格的颜色会被污渍(▒)覆盖，道具不受污渍影响；当墨水瓶被挖走污渍消失。",
            };
            buttomTable.Get(row++, 0).SetStyle("style=\"width:350px; text-align:left\"").SetContent(special[0]);
            buttomTable.Get(row++, 0).SetStyle("style=\"width:350px; text-align:left\"").SetContent(special[1]);
        }

        buttomTable.Get(0, 1).SetStyle("style=\"width:30px;\"").SetContent("<b>图例</b>");
        buttomTable.Get(0, 2).SetStyle("style=\"width:80px;\"").SetContent("<b>名称</b>");
        buttomTable.Get(0, 3).SetStyle("style=\"width:60px;\"").SetContent("<b>剩余数量</b>");
        struct InfoItem {
            string name;
            GridType type;
        };
        const InfoItem infoItem[] = {
            { "宝藏",   GridType::TREASURE   },
            { "炸弹",   GridType::BOMB       },
            { "加湿器", GridType::HUMIDIFIER },
            { "墨水瓶", GridType::INK        }, 
        };
        row = 1;
        for (int i = 0; i < (item_mode ? k_item_count : 2); i++) {
            buttomTable.Get(row, 1).SetStyle("style=\"width:30px;\"").SetContent(toString(infoItem[i].type));
            buttomTable.Get(row, 2).SetStyle("style=\"width:80px;\"").SetContent(infoItem[i].name);
            buttomTable.Get(row, 3).SetStyle("style=\"width:60px;\"").SetContent(to_string(countLeftGridType(infoItem[i].type)));
            row++;
        }
        buttomTable.Get(row, 1).SetStyle("style=\"width:30px;\"").SetContent("<div style=\"width:11px; height:11px; border:2px solid #FF0000;\"></div>");
        buttomTable.MergeRight(row, 2, 2);
        buttomTable.Get(row, 2).SetStyle("style=\"width:140px;\"").SetContent("本回合挖掘位置");
        row++;
        if (item_mode == 1) {
            buttomTable.Get(row, 1).SetStyle("style=\"width:30px;\"").SetContent("<div style=\"width:11px; height:11px; border:2px solid #FF6B00;\"></div>");
            buttomTable.MergeRight(row, 2, 2);
            buttomTable.Get(row, 2).SetStyle("style=\"width:140px;\"").SetContent("本回合额外信息");
            row++;
        }
        
        return buttomTable.ToString();
    }

    // 统计某一个类型的剩余数量
    int countLeftGridType(const GridType type) const
    {
        int count = 0;
        for (int x = 0; x < size; x++)
            for (int y = 0; y < size; y++)
                if (grid_map[x][y].type == type && !grid_map[x][y].show)
                    count++;
        return count;
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

    // 玩家行动
    pair<bool, string> PlayerAction(const PlayerID pid, string s, const int round)
    {
        string result = CheckCoordinate(s);
		if (result != "OK") return make_pair(false, result);
		const auto [X, Y] = TranString(s);
        
        if (X < 0 || X > size - 1 || Y < 0 || Y > size - 1) {
			return make_pair(false, "[错误] 挖掘位置超出了地图的范围");
		}

        return Action(pid, X, Y, round);
    }

    pair<bool, string> Action(const PlayerID pid, const int X, const int Y, const int round) {
        Player& p = players[pid];
        bool stain = CountSurroundingHiddenInks(X, Y) > 0;
        Grid& grid = grid_map[X][Y];
        if (grid.show) {
            switch (grid.type) {
                case GridType::TREASURE:   return {false, "[错误] 想再挖一次就能多一份宝藏？可没那么简单。试试其他位置吧~"};
                case GridType::BOMB:       return {false, "[错误] 这里已经炸成渣了，再挖也只剩焦土。试试其他位置吧~"};
                case GridType::HUMIDIFIER: return {false, "[错误] 加湿器已经在这里努力工作了。换个地方试试吧~"};
                case GridType::INK:        return {false, "[错误] 墨水瓶已经被回收过了，这里已经空无一物。去别处看看吧~"};
                default:                   return {false, "[错误] 这片区域已经被别人翻过啦。试试其他位置吧~"};
            }
        }

        grid.dig_count++;
        p.has_dug = true;
        p.current = {X, Y};
        string reply;
        switch (grid.type) {
            case GridType::TREASURE:
                reply = stain
                    ? "你挖到了一坨污渍，污渍下面是……哇，是【宝藏】！"
                    : "你挖到到了一个【宝藏】！";
                break;
            case GridType::BOMB:
                reply = (stain
                    ? "你挖到了一坨污渍，污渍下面是……糟糕，【炸弹】爆炸了！"
                    : "糟糕！你挖到了【炸弹】！")
                    + string(round <= 2 ? "（前 2 回合炸弹不生效）" : "");
                if (round > 2) { p.hp -= 1; p.change_hp = -1; }
                break;
            case GridType::HUMIDIFIER:
                reply = stain
                    ? "你挖到了一坨污渍，污渍下面是……居然是【加湿器】！显示为【" + to_string(grid.humidifier_add) + "档】"
                    : "你挖到了1个【加湿器】！地下怎么会有这个？！观察到是【" + to_string(grid.humidifier_add) + "档】";
                break;
            case GridType::INK:
                reply = stain
                    ? "你挖到了一坨污渍，污渍下面是……是……怎么是【墨水瓶】？！"
                    : "你挖到了1个【墨水瓶】！好心人，世界将因为你的善举更加洁净";
                InkSurroundingExtraMark(X, Y);
                break;
            default:
                reply = stain
                    ? "你挖到了一坨污渍，下面什么也没有。还好探测器还能用，屏幕上显示【" + grid.GetContent(true) + "】"
                    : "你什么也没有挖到，探测器显示的结果为【" + grid.GetContent(true) + "】";
        }
        return {true, reply};
    }

    void InkSurroundingExtraMark(const int x, const int y)
    {
        auto HasSurroundingHiddenInks = [&](int r, int c) -> bool {
            for (int i = max(0, r - 1); i <= min(size - 1, r + 1); ++i) {
                for (int j = max(0, c - 1); j <= min(size - 1, c + 1); ++j) {
                    if (i == r && j == c) continue;
                    if (grid_map[i][j].type == GridType::INK && !grid_map[i][j].show && grid_map[i][j].dig_count == 0) {
                        return true;
                    }
                }
            }
            return false;
        };
        for (int i = max(0, x - 1); i <= min(size - 1, x + 1); i++) {
            for (int j = max(0, y - 1); j <= min(size - 1, y + 1); j++) {
                if (grid_map[i][j].IsEmpty() && !HasSurroundingHiddenInks(i, j) && grid_map[i][j].dig_count == 0) {
                    grid_map[i][j].extra_mark = true;
                }
            }
        }
    }

    void ClearRoundStatus()
    {
        for (int x = 0; x < size; x++) {
            for (int y = 0; y < size; y++) {
                grid_map[x][y].dig_count = 0;
                grid_map[x][y].extra_mark = false;
            }
        }
        for (auto& player: players) {
            player.has_dug = false;
            player.change_hp = 0;
            player.change_score = 0;
        }
    }

    void UpdatePlayerStatus()
    {
        for (int pid = 0; pid < playerNum; ++pid) {
            Player& player = players[pid];
            if (!player.has_dug) continue;

            auto [X, Y] = player.current;
            Grid& grid = grid_map[X][Y];
            grid.show = true;
            
            int score = 0;
            switch (grid.type) {
                case GridType::TREASURE: score = 60 / grid.dig_count; break;
                case GridType::INK:      score =  5 / grid.dig_count; break;
                default:;
            }
            player.score += score;
            player.change_score = score;
        }
    }

    int AliveCount() const
    {
        return std::count_if(players.begin(), players.end(), [](const Player& p) {
            return p.hp > 0;
        });
    }

  private:
    // 特殊道具数量随机分配
    struct Item {
        GridType type;
        int specified;
        int actual = 0;
    };
    static void AssignItemCounts(vector<Item>& items, int total_count)
    {
        int specified_sum = 0, unspecified_cnt = 0;
        for (auto& it : items) {
            if (it.specified >= 0) specified_sum += it.specified;
            else unspecified_cnt++;
        }
        int remaining = max(0, total_count - specified_sum);
        vector<int> parts(unspecified_cnt, 0);
        if (unspecified_cnt > 0) {
            vector<int> cuts;
            cuts.reserve(unspecified_cnt + 2);
            cuts.push_back(0);
            for (int i = 0; i < unspecified_cnt - 1; i++) {
                cuts.push_back(rand() % (remaining + 1));
            }
            cuts.push_back(remaining);
            sort(cuts.begin(), cuts.end());
            for (int i = 0; i < unspecified_cnt; ++i) {
                parts[i] = cuts[i + 1] - cuts[i];
            }
        }
        int idx = 0;
        for (auto& it : items) {
            if (it.specified >= 0) it.actual = it.specified;
            else it.actual = parts[idx++];
        }
    }

    // 随机生成地图
    void GenerateRandomMap(const vector<int>& weights, const ItemNumOption& options)
    {
        for (int i = 0; i < k_item_count; i++) {
            int placed = 0;
            while (placed < options[i]) {
                int x = rand() % size;
                int y = rand() % size;
                if (grid_map[x][y].IsEmpty()) {
                    GridType type = static_cast<GridType>(k_color_count + i);
                    if (type == GridType::HUMIDIFIER) grid_map[x][y].humidifier_add = rand() % 3 + 1;
                    grid_map[x][y].type = type;
                    placed++;
                }
            }
        }

        for (int x = 0; x < size; x++) {
            for (int y = 0; y < size; y++) {
                Grid& grid = grid_map[x][y];
                if (!grid.IsEmpty()) continue;

                std::random_device rd;
                std::mt19937 gen(rd());
                std::discrete_distribution<> dist(weights.begin(), weights.end());
                GridType type = static_cast<GridType>(dist(gen));

                grid.type = type;
                int num = CountHumidifierAdd(x, y);

                switch (type) {
                    case GridType::R: num += CountSurroundingBombs(x, y); break;
                    case GridType::B: num += CountRowColBombs(x, y); break;
                    case GridType::Y: num += CountSurroundingTreasures(x, y); break;
                    case GridType::O: num += CountSurroundingBombs(x, y) + CountSurroundingTreasures(x, y); break;
                    case GridType::G: num += CountRowColSurroundingTreasures(x, y); break;
                    case GridType::P: num += CountRowColSurroundingBombs(x, y); break;
                    default: num = -1;
                }
                grid.num = num;
            }
        }
    }

    // 道具辅助计算
    int CountHumidifierAdd(const int r, const int c) const
    {
        int count = 0;
        for (int i = max(0, r - 1); i <= min(size - 1, r + 1); i++) {
            for (int j = max(0, c - 1); j <= min(size - 1, c + 1); j++) {
                if (i == r && j == c) continue;
                if (grid_map[i][j].type == GridType::HUMIDIFIER) count += grid_map[i][j].humidifier_add;
            }
        }
        return count;
    }

    int CountSurroundingHiddenInks(const int r, const int c) const
    {
        int count = 0;
        for (int i = max(0, r - 1); i <= min(size - 1, r + 1); i++) {
            for (int j = max(0, c - 1); j <= min(size - 1, c + 1); j++) {
                if (i == r && j == c) continue;
                if (grid_map[i][j].type == GridType::INK && !grid_map[i][j].show) count++;
            }
        }
        return count;
    }

    // 颜色辅助计算
    int CountSurroundingBombs(const int r, const int c) const
    {
        int count = 0;
        for (int i = max(0, r - 1); i <= min(size - 1, r + 1); i++) {
            for (int j = max(0, c - 1); j <= min(size - 1, c + 1); j++) {
                if (i == r && j == c) continue;
                if (grid_map[i][j].type == GridType::BOMB) count++;
            }
        }
        return count;
    }

    int CountRowColBombs(const int r, const int c) const
    {
        int count = 0;
        for (int j = 0; j < size; j++)
            if (grid_map[r][j].type == GridType::BOMB) count++;
        for (int i = 0; i < size; i++)
            if (grid_map[i][c].type == GridType::BOMB) count++;
        if (grid_map[r][c].type == GridType::BOMB) count--;
        return count;
    }

    int CountSurroundingTreasures(const int r, const int c) const
    {
        int count = 0;
        for (int i = max(0, r - 1); i <= min(size - 1, r + 1); i++) {
            for (int j = max(0, c - 1); j <= min(size - 1, c + 1); j++) {
                if (i == r && j == c) continue;
                if (grid_map[i][j].type == GridType::TREASURE) count++;
            }
        }
        return count;
    }

    int CountRowColSurroundingBombs(const int r, const int c) const
    {
        set<string> s;
        for (int j = 0; j < size; j++)
            if (grid_map[r][j].type == GridType::BOMB)
                s.insert(to_string(r) + "," + to_string(j));
        for (int i = 0; i < size; i++)
            if (grid_map[i][c].type == GridType::BOMB)
                s.insert(to_string(i) + "," + to_string(c));
        for (int i = max(0, r - 1); i <= min(size - 1, r + 1); i++) {
            for (int j = max(0, c - 1); j <= min(size - 1, c + 1); j++) {
                if (i == r && j == c) continue;
                if (grid_map[i][j].type == GridType::BOMB)
                    s.insert(to_string(i) + "," + to_string(j));
            }
        }
        return s.size();
    }

    int CountRowColSurroundingTreasures(const int r, const int c) const
    {
        set<string> s;
        for (int j = 0; j < size; j++)
            if (grid_map[r][j].type == GridType::TREASURE)
                s.insert(to_string(r) + "," + to_string(j));
        for (int i = 0; i < size; i++)
            if (grid_map[i][c].type == GridType::TREASURE)
                s.insert(to_string(i) + "," + to_string(c));
        for (int i = max(0, r - 1); i <= min(size - 1, r + 1); i++) {
            for (int j = max(0, c - 1); j <= min(size - 1, c + 1); j++) {
                if (i == r && j == c) continue;
                if (grid_map[i][j].type == GridType::TREASURE)
                    s.insert(to_string(i) + "," + to_string(j));
            }
        }
        return s.size();
    }

    // 地图style
    const string style = R"(
<style>
    @font-face{
        font-family: 'QisiJoanna';
        src: url("file:///)" + resource_path_ + R"(QisiJoanna.ttf");
    }
    .grid {
        font-family: 'QisiJoanna';
        font-size: 20px;
        width: 40px;
        height: 40px;
        box-sizing: border-box;
    }
    .hidden {
        background: #000000;
        box-shadow: inset 0 0 0 1px #C2C2C2;
    }
    .show {
        box-shadow: inset 0 0 0 1px #000000;
    }
    .current {
        box-shadow: inset 0 0 0 2px #FF0000;
    }
    .stain {
        background: #E9E9E9;
    }
    .extra {
        box-shadow: inset 0 0 0 2px #FF6B00;
    }
    .pos {
        font-size: 18px;
        font-weight: bold;
        width: 40px;
        height: 40px;
        box-sizing: border-box;
    }
    .item-box {
        position: relative;
        display: inline-block;
    }
    .item-badge {
        position: absolute;
        top: 0;
        right: 0;
        font-size: 12px;
    }
    .color-R { color: #C91D32; background: #FADADE; }
    .color-B { color: #0070C0; background: #D9E1F4; }
    .color-Y { color: #BD8900; background: #FFF3CA; }
    .color-O { color: #C55C10; background: #F9CBAA; }
    .color-G { color: #588E31; background: #E3F2D9; }
    .color-P { color: #7030A0; background: #F6CCFF; }
</style>
)";

};
