
struct GetBoardOptions {
    bool with_player = true;
    bool with_content = false;
    bool with_ground = true;
};

class Board
{
  public:
    Board(string image_path, const Texture image_texture, const BlockMode mode, const vector<string>& custom_blocks)
        : image_path_(std::move(image_path)), image_texture_(image_texture), block_mode(mode), g(std::random_device{}()), unitMaps(mode, g, custom_blocks) {}

    // 随机引擎
    std::mt19937 g;

    // 图片资源文件夹
    const string image_path_;
    const Texture image_texture_;

	// 玩家
    uint32_t playerNum;
    vector<Player> players;
    // 地图内的玩家
    vector<vector<vector<PlayerID>>> player_map;
    // 地图大小
    int size = 9;
    // 地图
    vector<vector<Grid>> grid_map;
    BlockMode block_mode;
    int exit_num;  // 逃生舱数量
    string init_html_;
    // 区块模板
    UnitMaps unitMaps;
    // BOSS
    Boss boss{size, players};
    // 完整赛况额外信息
    string all_extra_record;
    // 特殊区块位置（暂不使用）
    // pair<int, int> special_pos = {12, 12};

    // 初始化地图
    void Initialize()
    {
        grid_map.resize(size);
        player_map.resize(size);
        for (int i = 0; i < size; i++) {
            grid_map[i].resize(size);
            player_map[i].resize(size);
        }

        InitializeBlocks();              // 初始化区块
        FixAdjacentWalls(grid_map);     // 相邻墙面修复
        FixInvalidPortals(grid_map);    // 封闭传送门替换为水洼
        RandomizePlayers();             // 随机生成玩家

        // 成就辅助：记录玩家初始位置
        for (PlayerID pid = 0; pid < playerNum; ++pid) {
            players[pid].achievement.MakeStep(players[pid].x, players[pid].y);
        }
    }

    // 保存初始盘面
    void SaveGameStartMap()
    {
        init_html_ = GetBoard(grid_map);
    }

    // 获取地图html
    string GetBoard(const vector<vector<Grid>>& grid_map, const GetBoardOptions& options = {}) const
    {
        size_t size = grid_map.size();
        html::Table map(size * 2 + 1, size * 2 + 1);
        map.SetTableStyle("align=\"center\" cellpadding=\"0\" cellspacing=\"0\" style=\"border: 2px dashed black;\"");
        // 方格信息（包括玩家）
        for (int x = 1; x < size * 2; x = x + 2) {
            for (int y = 1; y < size * 2; y = y + 2) {
                int gridX = (x+1)/2-1, gridY = (y+1)/2-1;
                const Grid& grid = grid_map[gridX][gridY];
                if (options.with_ground) {
                    map.Get(x, y).SetStyle("class=\"grid\" " + GetGridStyle(grid.Type(), grid.Attach(), true));
                } else {
                    map.Get(x, y).SetStyle("class=\"grid\" " + GetGridStyle(GridType::EMPTY, AttachType::EMPTY, true));
                }
                if (options.with_content) {
                    string content = grid.GetContent().first;
                    if (!content.empty()) {
                        map.Get(x, y).SetContent(GetColoredMark(content, grid.Type(), grid.Attach()));
                    }
                } else if (options.with_player) {
                    string player_marks;
                    if (boss.Enable() && boss.x == gridX && boss.y == gridY) {
                        player_marks += boss.GetBossIcon();
                    }
                    for (auto pid: player_map[gridX][gridY]) {
                        player_marks += num[pid];
                        if (player_marks.length() % 2 == 0) {
                            player_marks += "<br>";
                        }
                    }
                    // 根据[地形/附着]改变玩家标记样式
                    if (!player_marks.empty()) {
                        map.Get(x, y).SetContent(GetColoredMark(player_marks, grid.Type(), grid.Attach()));
                    }
                }
                // 调试：测试地形文字颜色
                // map.Get(x, y).SetContent(GetColoredMark(string(num[0]), grid.Type(), grid.Attach()));
            }
        }
        // 纵向围墙
        for (int x = 1; x < size * 2; x = x + 2) {
            for (int y = 0; y < size * 2 - 1; y = y + 2) {
                int gridX = (x+1)/2-1, gridY = y/2;
                Wall wall = grid_map[gridX][gridY].GetWall<Direct::LEFT>();
                map.Get(x, y).SetStyle("class=\"wall-col\" style=\"" + GetWallStyle(wall, "col") + "\"");
                if (options.with_content) {
                    string content = Grid::GetWallContent(grid_map, gridX, gridY, Direct::LEFT);
                    if (!content.empty()) map.Get(x, y).SetContent(content);
                }
            }
            Wall wall = grid_map[(x+1)/2-1][size-1].GetWall<Direct::RIGHT>();
            map.Get(x, size*2).SetStyle("class=\"wall-col\" style=\"" + GetWallStyle(wall, "col") + "\"");
            if (options.with_content) {
                string content = Grid::GetWallContent(grid_map, (x+1)/2-1, size-1, Direct::RIGHT);
                if (!content.empty()) map.Get(x, size*2).SetContent(content);
            }
        }
        // 横向围墙
        for (int y = 1; y < size * 2; y = y + 2) {
            for (int x = 0; x < size * 2 - 1; x = x + 2) {
                int gridX = x/2, gridY = (y+1)/2-1;
                Wall wall = grid_map[gridX][gridY].GetWall<Direct::UP>();
                map.Get(x, y).SetStyle("class=\"wall-row\" style=\"" + GetWallStyle(wall, "row") + "\"");
                if (options.with_content) {
                    string content = Grid::GetWallContent(grid_map, gridX, gridY, Direct::UP);
                    if (!content.empty()) map.Get(x, y).SetContent(content);
                }
            }
            Wall wall = grid_map[size-1][(y+1)/2-1].GetWall<Direct::DOWN>();
            map.Get(size*2, y).SetStyle("class=\"wall-row\" style=\"" + GetWallStyle(wall, "row") + "\"");
            if (options.with_content) {
                string content = Grid::GetWallContent(grid_map, size-1, (y+1)/2-1, Direct::DOWN);
                if (!content.empty()) map.Get(size*2, y).SetContent(content);
            }
        }
        // 角落方块
        for (int x = 0; x < size * 2 + 1; x = x + 2) {
            for (int y = 0; y < size * 2 + 1; y = y + 2) {
                map.Get(x, y).SetStyle("class=\"corner\" style=\"background-image: url('" + ToFileUrl(image_path_ + GetImageTypeFolder() + "walls/corner.png") + "');\"");
            }
        }
        return GetBoardStyle() + map.ToString();
    }

    // 获取终局对比盘面
    string GetFinalBoard() const
    {
        html::Table finalTable(2, 2);
        finalTable.SetTableStyle("align=\"center\" cellpadding=\"0\" cellspacing=\"0\"");
        finalTable.Get(0, 0).SetContent(HTML_SIZE_FONT_HEADER(5) "<b>初始地图</b>" HTML_FONT_TAIL);
        finalTable.Get(0, 1).SetContent(HTML_SIZE_FONT_HEADER(5) "<b>终局地图</b>" HTML_FONT_TAIL);
        finalTable.Get(1, 0).SetStyle("style=\"padding: 10px 20px 10px 10px\"").SetContent(init_html_);
        finalTable.Get(1, 1).SetStyle("style=\"padding: 10px 10px 10px 20px\"").SetContent(GetBoard(grid_map));
        return GetPlayerTable(-1) + GetBoardStyle() + finalTable.ToString();
    }

    // 全部区块信息展示
    string GetAllBlocksInfo(const SpecialEvent event, const bool bomb_mode, const optional<BlockMode> test_block_mode = std::nullopt) const
    {
        const BlockMode test_mode = test_block_mode.value_or(BlockMode::CLASSIC);

        string title = "";
        int col_num = (test_mode == BlockMode::CRAZY) ? 8 : 4;
        const auto& maps = [&]() -> const std::vector<UnitMaps::Map>& {
            if (test_mode == BlockMode::CLASSIC) return unitMaps.maps;
            if (test_mode == BlockMode::CRAZY)   return unitMaps.all_maps;
            return unitMaps.pool_maps;
        }();
        const auto& exits = [&]() -> const std::vector<UnitMaps::Map>& {
            if (test_mode == BlockMode::CLASSIC) return unitMaps.exits;
            if (test_mode == BlockMode::CRAZY)   return unitMaps.all_exits;
            return unitMaps.pool_exits;
        }();
        const auto& all_special_maps = unitMaps.all_special_maps;
        
        // 调试：根据模式生成完整区块预览
        switch (test_mode) {
            case BlockMode::TWIST:  title = HTML_SIZE_FONT_HEADER(6) "<b>《漫漫长夜》「幻变」模式轮换区块</b>" HTML_FONT_TAIL; break;
            case BlockMode::CRAZY:  title = HTML_SIZE_FONT_HEADER(6) "<b>《漫漫长夜》「狂野」+「疯狂」模式全部区块</b>" HTML_FONT_TAIL; break;
            case BlockMode::BUTTON: title = HTML_SIZE_FONT_HEADER(6) "<b>《漫漫长夜》「按钮」模式主题区块</b>" HTML_FONT_TAIL; break;
            case BlockMode::TRAP:   title = HTML_SIZE_FONT_HEADER(6) "<b>《漫漫长夜》「陷阱」模式主题区块</b>" HTML_FONT_TAIL; break;
            default:;
        }

        bool has_special = (test_mode == BlockMode::CRAZY);
        for (const auto& map : maps) {
            if (map.is_special) {
                has_special = true;
            }
        }
        SpecialEvent rain_event = event == SpecialEvent::RAINSTORY ? SpecialEvent::RAINSTORY : SpecialEvent::NONE;

        int line_num = (ceil(maps.size() / (double) col_num) + ceil(exits.size() / (double) col_num)) * 2 +  + has_special * (ceil(all_special_maps.size() / (double) col_num) * 2 + 1);
        html::Table blocks(line_num, col_num);
        blocks.SetTableStyle("align=\"center\" cellpadding=\"0\" cellspacing=\"0\" style=\"font-size: 25px; line-height: 1.1;\"");
        int row = 0;
        for (int i = 0; i < maps.size(); i++) {
            blocks.Get(row, i % col_num).SetStyle("style=\"padding: 25px 25px 5px 25px\"").SetContent(GetSingleBlock(maps[i].id, false, rain_event));
            // [疯狂模式] 特殊地图不显示id
            if (maps[i].is_special && block_mode == BlockMode::CRAZY) {
                blocks.Get(row + 1, i % col_num).SetContent(HTML_COLOR_FONT_HEADER(red) HTML_SIZE_FONT_HEADER(6) "<b>？？？</b>" HTML_FONT_TAIL HTML_FONT_TAIL);
            } else {
                blocks.Get(row + 1, i % col_num).SetContent(GetMapTitleContent(maps[i].title, maps[i].id, false));
            }
            if ((i + 1) % col_num == 0 || i == maps.size() - 1) row += 2;
        }
        for (int i = 0; i < exits.size(); i++) {
            blocks.Get(row, i % col_num).SetStyle("style=\"padding: 20px 25px 5px 25px\"").SetContent(GetSingleBlock(exits[i].id, true, rain_event));
            blocks.Get(row + 1, i % col_num).SetContent(GetMapTitleContent(exits[i].title, exits[i].id, true));
            if ((i + 1) % col_num == 0 || i == exits.size() - 1) row += 2;
        }
        if (has_special && block_mode == BlockMode::CRAZY) {
            blocks.MergeRight(row, 0, col_num);
            blocks.Get(row++, 0).SetStyle("style=\"padding: 20px 25px 5px 25px\"")
                .SetContent(HTML_SIZE_FONT_HEADER(6) "<b>特殊区块列表<br>" HTML_FONT_TAIL HTML_SIZE_FONT_HEADER(5) "（多传送门按照图中字母代号传送）" HTML_FONT_TAIL "</b>");
            for (int i = 0; i < all_special_maps.size(); i++) {
                blocks.Get(row, i % col_num).SetStyle("style=\"padding: 20px 25px 5px 25px\"")
                    .SetContent(GetBoard(unitMaps.FindBlockById(all_special_maps[i].id, false, SpecialEvent::NONE), GetBoardOptions{.with_content = true}));
                blocks.Get(row + 1, i % col_num).SetContent(GetMapTitleContent(all_special_maps[i].title, all_special_maps[i].id, false));
                if ((i + 1) % col_num == 0 || i == all_special_maps.size() - 1) row += 2;
            }
        }

        const vector<pair<Wall, string>> all_walls_info = {
            { Wall::DOOR, "【门】初始关闭状态的门，关闭时视为墙壁。当关联的按钮被按下时会切换开关状态<br><b>如果门发生过变化，在回合结束会进行提示。</b>关闭的门在私信墙壁信息会显示为**普通墙壁**" },
            { Wall::DOOROPEN, "【门 (开)】初始打开状态的门，玩家可移动穿过打开的门。在私信墙壁信息会显示为**无墙壁**" },
        };
        const vector<pair<AttachType, string>> all_attachs_info = {
            { AttachType::BUTTON, "【按钮】玩家进入时会触发区块内按钮相关事件。（出生不算）<br>进入按钮格**没有任何额外信息提示**，且仅在进入时才会触发按钮" },
            { AttachType::BOMB, "【炸弹】玩家**经过并尝试离开**会引爆炸弹，使玩家**立即出局并-100分**<br>在炸弹上**结束行动**可拆除炸弹，放置后直接停止不会拆除。<font color=red>**炸弹不能放置在其他附着类型上**</font>" },
            { AttachType::BOX, "【箱子】玩家相邻箱子且向箱子移动时，箱子可被推动。（不会出生在箱子内）<br>**箱子不可移动到本区块外，其后方有玩家、墙壁或其他附着时，均不可被推动**。<br>若箱子不可被推动，则视为**撞墙**，且**箱子本身不会显示为墙壁**。" },
            { AttachType::HEATBOX, "【小太阳】有 5x5 **热浪范围**的箱子，基础推动规则和**箱子**一致。（不会出生在小太阳内）<br>玩家进入小太阳周围 5x5 范围时，将**私信**收到热浪提示。<br><font color=red>小太阳和其他热源区块**不会同时出现在地图中**</font>（自定义模式区块不足时除外）" },
            { AttachType::JAMMERBOX, "【屏蔽器】有 3x3 **寂静范围**的箱子，基础推动规则和**箱子**一致。（不会出生在屏蔽器内）<br>玩家位于屏蔽器周围 3x3 范围内时，<font color=teal>**将无法获知所有声响的方向来源**</font>。" },
        };
        const vector<pair<GridType, string>> all_grids_info = {
            { GridType::GRASS, "【树丛】玩家进入时会发出让其他人听见的<font color=#00af50>**沙沙声**</font>。（出生不算）" },
            { GridType::WATER, "【水洼】玩家进入时会发出让其他人听见的<font color=#01b0f1>**啪啪声**</font>。（出生不算）" },
            { GridType::PORTAL, "【传送门】玩家进入时会发出其他人听见的<font color=#01b0f1>**啪啪声**</font>。（出生不算）<br>进入后，再任意 <font color=purple>**2次**</font> 移动后就会传送至同区块另一个传送门。<br>进入后，玩家视作进入亚空间，上述 <font color=purple>**2次**</font> 移动都在亚空间内。" },
            { GridType::ONEWAYPORTAL, "【传送门出口】玩家进入时会发出其他人听见的<font color=#01b0f1>**啪啪声**</font>。（出生不算）<br>传送门的单向出口，进入时不会触发传送（必须从入口进入才会传送至此处）<br>**玩家在进入同一区块的传送门入口时，传送门会转换方向，入口和出口交换位置**" },
            { GridType::TRAP, "【陷阱】陷阱隐藏在树丛中：被奇数次进入时，会发出让其他人听见的<font color=#00af50>**沙沙声**</font>（出生不算）<br>被偶数次进入时，不发出声响，并**强制玩家停止**（出生不算）" },
            { GridType::HEAT, "【热源】进入热源周围 8 格时，将**私信**收到热浪提示。（只有移动时才能感受到热浪）<br>当进入热源时，将**私信**收到高温烫伤提示（不会出生在热源内）<br><font color=red>在整局游戏中，**当第 2 次或更多次进入热源时，会被强制停止行动**</font>" },
            { GridType::EXIT, "【逃生舱】逃生者使用后，**会消失**。" + (test_mode == BlockMode::CLASSIC ? ("本局逃生舱数量为 **" + to_string(exit_num) + "** 个。") : "") },
        };

        auto GenerateWallStyle = [&](Wall wall, const string& direction) {
            return "<div class='wall-row' style=\""
                + GetWallStyle(wall, direction)
                + "\"></div>";
        };
        vector<UnitMaps::Map> all_maps_in_game;
        all_maps_in_game.insert(all_maps_in_game.end(), maps.begin(), maps.end());
        all_maps_in_game.insert(all_maps_in_game.end(), exits.begin(), exits.end());
        if (has_special && block_mode == BlockMode::CRAZY) all_maps_in_game.insert(all_maps_in_game.end(), all_special_maps.begin(), all_special_maps.end());

        int legend_max_size = all_walls_info.size() + all_attachs_info.size() + all_grids_info.size() + 1;
        html::Table legend(legend_max_size, 2);
        legend.SetTableStyle("cellpadding=\"5\" cellspacing=\"0\" style=\"font-size: 22px;\"");
        for (int i = 0; i < legend_max_size; i++) {
            legend.Get(i, 1).SetStyle("style=\"text-align: left;\"");
        }
        const string wall_html =
            "<div style=\"width:" + to_string(GRID_SIZE) + "px;\">" +
                GenerateWallStyle(Wall::NORMAL, "row") +
                GetGridStyle(GridType::EMPTY, AttachType::EMPTY, false) +
                GenerateWallStyle(Wall::NORMAL, "row") +
            "</div>";
        legend.Get(0, 0).SetContent(wall_html);
        legend.Get(0, 1).SetContent("【墙壁】如图例，上下为墙壁，玩家不可从上下通过，可以从左右通过");
        row = 1;
        for (const auto& wall_info: all_walls_info) {
            if (UnitMaps::MapContainWallType(all_maps_in_game, wall_info.first)) {
                legend.Get(row, 0).SetContent(GenerateWallStyle(wall_info.first, "row"));
                legend.Get(row, 1).SetContent(wall_info.second);
                row++;
            }
        }
        for (const auto& attach_info: all_attachs_info) {
            if (UnitMaps::MapContainAttachType(all_maps_in_game, attach_info.first) || (attach_info.first == AttachType::BOMB && bomb_mode)) {
                legend.Get(row, 0).SetContent(GetGridStyle(GridType::EMPTY, attach_info.first, false));
                legend.Get(row, 1).SetContent(attach_info.second);
                row++;
            }
        }
        for (const auto& grid_info: all_grids_info) {
            if (UnitMaps::MapContainGridType(all_maps_in_game, grid_info.first)) {
                if (grid_info.first == GridType::GRASS && event == SpecialEvent::RAINSTORY) continue;
                legend.Get(row, 0).SetContent(GetGridStyle(grid_info.first, AttachType::EMPTY, false));
                legend.Get(row, 1).SetContent(grid_info.second);
                row++;
            }
        }
        legend.ResizeRow(row);
        return title + blocks.ToString() + legend.ToString();
    }

    string GetMapTitleContent(const string& title, const string& id, const bool is_exit) const
    {
        return "<b>" HTML_COLOR_FONT_HEADER(#990000) HTML_SIZE_FONT_HEADER(4) + title + HTML_FONT_TAIL HTML_FONT_TAIL
            "<br>" HTML_COLOR_FONT_HEADER(red) + (is_exit ? "EXIT " : "") + id + HTML_FONT_TAIL "</b>";
    }

    string GetSingleBlock(const string& id, const bool is_exit, const SpecialEvent event) const
    {
        // [疯狂模式] 特殊地图不显示预览
        if (id[0] == 'S' && block_mode == BlockMode::CRAZY) {
            string size = to_string(GRID_SIZE * 3 + WALL_SIZE * 4) + "px";
            return "<div style=\"width:" + size + "; height:" + size + "; background:#e0e0e0;"
                "border:2px dashed black; font-size:30px; color:#333;"
                "display:flex; align-items:center; justify-content:center;\">"
                "<b>特殊区块</b></div>";
        }
        return GetBoard(unitMaps.FindBlockById(id, is_exit, event), GetBoardOptions{.with_content = true});
    }

    // 获取玩家信息
    string GetPlayerTable(const int round) const
    {
        html::Table playerTable(playerNum + boss.Enable(), 6);
        playerTable.SetTableStyle("align=\"center\" cellpadding=\"2\"");
        for (int pid = 0; pid < playerNum; pid++) {
            playerTable.Get(pid, 0).SetStyle("style=\"width:60px; text-align:right;\"").SetContent(to_string(pid) + "号：");
            playerTable.Get(pid, 1).SetStyle("style=\"width:40px;\"").SetContent(players[pid].avatar);
            playerTable.Get(pid, 2).SetStyle("style=\"width:250px; text-align:left;\"").SetContent(players[pid].name);
            if (players[pid].out == 2) {
                playerTable.MergeRight(pid, 3, 3);
                playerTable.Get(pid, 3).SetStyle("style=\"width:120px;\"").SetColor("#FFEBA3").SetContent("【逃生舱撤离】");
            } else if (players[pid].out == 1) {
                playerTable.MergeRight(pid, 3, 3);
                playerTable.Get(pid, 3).SetStyle("style=\"width:120px;\"").SetColor("#E5E5E5").SetContent("【已出局】");
            } else if (players[pid].target == 100) {
                playerTable.MergeRight(pid, 3, 3);
                playerTable.Get(pid, 3).SetStyle("style=\"width:130px;\"").SetContent("【单机模式】<br>寻找唯一逃生舱！");
            } else if (players[pid].out == 0) {
                playerTable.Get(pid, 3).SetStyle("style=\"width:40px;\"").SetContent("捕捉<br>目标");
                playerTable.Get(pid, 4).SetStyle("style=\"width:40px;\"").SetContent("[" + to_string(players[pid].target) + "号]");
                playerTable.Get(pid, 5).SetStyle("style=\"width:40px;\"").SetContent(players[players[pid].target].avatar);
            } else {
                playerTable.MergeRight(pid, 3, 3);
                playerTable.Get(pid, 3).SetStyle("style=\"width:120px;\"").SetColor("#FFA07A").SetContent("[玩家状态错误]");
            }
        }
        if (boss.Enable()) {
            playerTable.Get(playerNum, 0).SetStyle("style=\"width:60px;\"").SetContent("<font size=\"5\">" + boss.GetBossIcon() + "</font>");
            playerTable.MergeRight(playerNum, 1, 2);
            playerTable.Get(playerNum, 1).SetStyle("style=\"width:290px;\"").SetColor("lavender").SetContent("[BOSS] " + boss.GetBossName());
            playerTable.Get(playerNum, 3).SetStyle("style=\"width:40px;\"").SetContent("追击<br>目标");
            playerTable.Get(playerNum, 4).SetStyle("style=\"width:40px;\"").SetContent("[" + to_string(boss.target) + "号]");
            playerTable.Get(playerNum, 5).SetStyle("style=\"width:40px;\"").SetContent(players[boss.target].avatar);
        }
        return (round > 0 ? "### 第 " + to_string(round) + " 回合" : "") + playerTable.ToString();
    }

    string GetPlayerString() const
    {
        string players_string;
        for (int pid = 0; pid < playerNum; pid++) {
            if (pid > 0) players_string += "\n";
            players_string += "[" + to_string(pid) + "号]" + players[pid].name;
            if (players[pid].out == 2) players_string += "【逃生舱撤离】";
            else if (players[pid].out == 1) players_string += "【已出局】";
            else if (players[pid].target == 100) players_string += "【单机模式】";
            else if (players[pid].out == 0) players_string += "\n目标→ [" + to_string(players[pid].target) + "号]" + players[players[pid].target].name;
            else players_string += "[玩家状态错误]";
        }
        if (boss.Enable()) {
            players_string += "\n[BOSS] " + boss.GetBossName() + "\n";
            players_string += "目标→ [" + to_string(boss.target) + "号]" + players[boss.target].name;
        }
        return players_string;
    }

    // 完整赛况
    string GetAllRecordHtml(const int query_pid, const int is_public) const
    {
        const char* style = R"(
<style>
    .record {}
    .player {
        padding: 8px;
        margin-bottom: 8px;
        border-left: 4px solid #4CAF50;
        background: #F9F9F9;
    }
    .player.out {
        opacity: 0.6;
        border-left-color: #E5E5E5;
        background: #F9F9F9;
    }
    .player.exit {
        opacity: 0.6;
        border-left-color: #FFE06F;
        background: #FFF3E0;
    }
    .player-title {
        font-size: 16px;
    }
    .player-record {}
    .boss {
        margin-top: 12px;
        padding: 8px;
        background: #FFF3E0;
        border-left: 4px solid #FF9800;
    }
    .extra {
        margin-top: 12px;
        padding: 8px;
        background: #E3F2FD;
        border-left: 4px solid #2196F3;
    }
</style>
)";
        string record_html;
        record_html += "<div class='record'>";
        // 玩家记录
        for (int pid = 0; pid < playerNum; pid++) {
            record_html += "<div class='player";
            if (players[pid].out == 1) record_html += " out";
            if (players[pid].out == 2) record_html += " exit";
            record_html += "'>";

            record_html += "<div class='player-title'>";
            record_html += "<b>[" + to_string(pid) + "号]" + esc_html(players[pid].name) + "</b>";
            record_html += "</div>";

            record_html += "<div class='player-record'>";
            record_html += players[pid].GetAllMoveRecord(query_pid, is_public);
            record_html += "</div>";

            record_html += "</div>";
        }
        // BOSS 记录
        if (boss.Enable()) {
            record_html += "<div class='boss'>";
            record_html += "<div class='boss-title'><b>[BOSS] " + boss.GetBossName() + " " + boss.GetBossIcon() + "</b></div>";
            record_html += "<div class='boss-record'>";
            record_html += boss.GetBossRecord(query_pid, is_public);
            record_html += "</div>";
            record_html += "</div>";
        }
        // 额外信息
        if (!all_extra_record.empty()) {
            record_html += "<div class='extra'>";
            record_html += "<div class='extra-title'><b>[额外信息]</b></div>";
            record_html += "<div class='extra-record'>";
            record_html += all_extra_record;
            record_html += "</div>";
            record_html += "</div>";
        }
        record_html += "</div>";
        return style + record_html;
    }

    string GetAllRecordString(const int query_pid, const int is_public) const
    {
        string record_string;
        // 玩家记录
        for (int pid = 0; pid < playerNum; pid++) {
            record_string += "[" + to_string(pid) + "号]" + players[pid].name;
            if (players[pid].out == 1) record_string += "【已出局】";
            if (players[pid].out == 2) record_string += "【逃生舱撤离】";
            record_string += "\n" + players[pid].GetAllMoveRecord(query_pid, is_public, false) + "\n";
        }
        // BOSS 记录
        if (boss.Enable()) {
            record_string += "[BOSS] " + boss.GetBossName() + " " + boss.GetBossIcon();
            record_string += boss.GetBossRecord(query_pid, is_public, false) + "\n";
        }
        // 额外信息
        if (!all_extra_record.empty()) {
            record_string += "[额外信息]";
            record_string += replace_br_with_line(all_extra_record) + "\n";
        }
        return record_string;
    }

    string GetAllScore() const
    {
        html::Table scoreTable(playerNum + 1, 7);
        scoreTable.SetTableStyle("cellpadding=\"2\" cellspacing=\"0\" border=\"1\"");
        scoreTable.Get(0, 0).SetStyle("style=\"width:50px;\"").SetContent("序号");
        scoreTable.Get(0, 1).SetStyle("style=\"width:250px;\"").SetContent("玩家");
        scoreTable.Get(0, 2).SetStyle("style=\"width:70px;\"").SetContent("抓人分");
        scoreTable.Get(0, 3).SetStyle("style=\"width:70px;\"").SetContent("逃生分");
        scoreTable.Get(0, 4).SetStyle("style=\"width:70px;\"").SetContent("探索分");
        scoreTable.Get(0, 5).SetStyle("style=\"width:70px;\"").SetContent("额外<br>探索分");
        scoreTable.Get(0, 6).SetStyle("style=\"width:100px;\"").SetContent("【最终总分】");
        for (int pid = 0; pid < playerNum; pid++) {
            Score s = players[pid].score;
            scoreTable.Get(pid + 1, 0).SetContent(to_string(pid) + "号");
            scoreTable.Get(pid + 1, 1).SetContent(players[pid].name);
            if (s.catch_score > 0) scoreTable.Get(pid + 1, 2).SetColor("#BAFFA8");
            if (s.catch_score < 0) scoreTable.Get(pid + 1, 2).SetColor("#FFA07A");
            scoreTable.Get(pid + 1, 2).SetContent(to_string(s.catch_score));
            if (s.exit_score > 0) scoreTable.Get(pid + 1, 3).SetColor("#FFDB60");
            scoreTable.Get(pid + 1, 3).SetContent(to_string(s.exit_score));
            auto [c1, c2] = s.ExploreCount();
            scoreTable.Get(pid + 1, 4).SetContent(to_string(c1 + c2));
            scoreTable.Get(pid + 1, 5).SetContent(to_string(c2));
            scoreTable.Get(pid + 1, 6).SetContent((s.quit_score < 0 ? HTML_COLOR_FONT_HEADER("red") : HTML_COLOR_FONT_HEADER("black")) + to_string(s.FinalScore()) + HTML_FONT_TAIL);
        }
        return Score::ScoreInfo() + scoreTable.ToString();
    }

    // 玩家移动
    bool MakeMove(const PlayerID pid, const Direct direction, const bool hide)
    {
        int d = static_cast<int>(direction);
        int cx = players[pid].x;
        int cy = players[pid].y;
        int nx = (cx + k_DX_Direct[d] + size) % size;
        int ny = (cy + k_DY_Direct[d] + size) % size;

        bool hit_wall = false;
        if (players[pid].subspace < 0) {
            switch (direction) {
                case Direct::UP:    hit_wall = !grid_map[cx][cy].CanPass<Direct::UP>(); break;
                case Direct::DOWN:  hit_wall = !grid_map[cx][cy].CanPass<Direct::DOWN>(); break;
                case Direct::LEFT:  hit_wall = !grid_map[cx][cy].CanPass<Direct::LEFT>(); break;
                case Direct::RIGHT: hit_wall = !grid_map[cx][cy].CanPass<Direct::RIGHT>(); break;
                default: return false;
            }
            // 非撞墙，尝试移动箱子
            if (!hit_wall && grid_map[nx][ny].HasBox()) {
                bool box_success = BoxMove(nx, ny, k_DX_Direct[d], k_DY_Direct[d]);
                if (box_success) players[pid].achievement.visitAttach(AttachType::BOX); // 成就[乒铃乓啷]辅助
                hit_wall = !box_success;
            }
            // 撞墙
            if (hit_wall) {
                if (!hide) players[pid].NewStepRecord(direction, "撞");
                return false;
            }
        }
        // 轨迹记录
        if (!hide) players[pid].NewStepRecord(direction);

        // 非撞墙，炸弹触发，实际不移动
        if (players[pid].bomb_trigger) {
            return true;
        }

        // 亚空间，实际不移动
        if (players[pid].InSubspace()) {
            players[pid].subspace--;
            return true;
        }

        RemovePlayerFromMap(pid);

        players[pid].x = nx;
        players[pid].y = ny;
        player_map[nx][ny].push_back(pid);

        // 成就记录
        players[pid].achievement.MakeStep(nx, ny);

        // 探索分
        auto& explore = players[pid].score.explore_map[nx][ny];
        if (explore == 0) {
            bool first = true;
            for (auto& player: players) {
                if (player.score.explore_map[nx][ny] > 0) {
                    first = false;
                }
            }
            explore = first ? 2 : 1;
        }
    
        return true;
    }

    // 箱子移动
    bool BoxMove(const int b_cx, const int b_cy, const int dx, const int dy)
    {
        int b_nx = (b_cx + dx + size) % size;
        int b_ny = (b_cy + dy + size) % size;

        // 起点必须有箱子
        if (!grid_map[b_cx][b_cy].HasBox()) return false;
        // 仅存活玩家阻挡箱子，出局玩家可被覆盖
        for (auto pid : player_map[b_nx][b_ny]) {
            if (players[pid].out == 0) {
                return false;
            }
        }
        // 后方有任何附着物，则不可推动
        if (grid_map[b_nx][b_ny].Attach() != AttachType::EMPTY) return false;

        bool wall = false;
        if (dx == -1 && dy == 0)        wall = !grid_map[b_cx][b_cy].CanPass<Direct::UP>();
        else if (dx == 1 && dy == 0)    wall = !grid_map[b_cx][b_cy].CanPass<Direct::DOWN>();
        else if (dx == 0 && dy == -1)   wall = !grid_map[b_cx][b_cy].CanPass<Direct::LEFT>();
        else if (dx == 0 && dy == 1)    wall = !grid_map[b_cx][b_cy].CanPass<Direct::RIGHT>();
        if (wall) return false;

        bool sameBlock = false;
        for (auto pos : unitMaps.pos) {
            int bx = pos.first, by = pos.second;
            if (b_cx >= bx && b_cx < bx + 3 && b_cy >= by && b_cy < by + 3 &&
                b_nx >= bx && b_nx < bx + 3 && b_ny >= by && b_ny < by + 3) {
                sameBlock = true;
                break;
            }
        }
        if (!sameBlock) return false;

        // 支持全部箱子类型
        AttachType box_type = grid_map[b_cx][b_cy].Attach();
        grid_map[b_cx][b_cy].SetAttach(AttachType::EMPTY);
        grid_map[b_nx][b_ny].SetAttach(box_type);
        return true;
    }

    // 玩家从地图中移除（辅助功能）
    void RemovePlayerFromMap(const PlayerID pid)
    {
        int cx = players[pid].x;
        int cy = players[pid].y;
        auto &oldCell = player_map[cx][cy];
        auto it = std::find(oldCell.begin(), oldCell.end(), pid);
        if (it != oldCell.end()) oldCell.erase(it);
    }

    // 解析多步行动：将输入字符串解析为方向数组
    static string parseDirections(const string &input, vector<Direct> &out)
    {
        // 按 key 长度降序，优先匹配中文
        vector<string> keys;
        for (auto& p : direction_map) keys.push_back(p.first);
        sort(keys.begin(), keys.end(),[](const string& a, const string& b) {
            return a.size() > b.size();
        });
        vector<Direct> tmp;
        size_t i = 0;
        while (i < input.size()) {
            bool matched = false;
            for (const auto& k : keys) {
                if (i + k.size() <= input.size() &&
                    input.compare(i, k.size(), k) == 0) {
                    tmp.push_back(direction_map.at(k));
                    i += k.size();
                    matched = true;
                    break;
                }
            }
            if (!matched) {
                size_t len = utf8CharLen(static_cast<unsigned char>(input[i]));
                return input.substr(i, len);    // 返回非法字符
            }
        }
        out = std::move(tmp);
        return "";
    }

    // 地区声响
    Sound GetSound(const Grid& grid, const SpecialEvent event) const
    {
        switch(grid.Type()) {
            case GridType::GRASS:   return Sound::SHASHA;
            case GridType::WATER:   return Sound::PAPA;
            case GridType::ONEWAYPORTAL:
            case GridType::PORTAL:  return Sound::PAPA;
            case GridType::TRAP:    return grid.TrapStatus() ? Sound::NONE : (event == SpecialEvent::RAINSTORY ? Sound::PAPA : Sound::SHASHA);
            default: return Sound::NONE;
        }
    }

    // 获取声响方向（用于发送声响）
    string GetSoundDirection(const int fromX, const int fromY, const Player& to) const
    {
        int toX = to.x;
        int toY = to.y;

        int dx = toX - fromX;
        int dy = toY - fromY;

        if (abs(dx) > size / 2) {
            if (dx > 0)
                dx -= size;
            else
                dx += size;
        }
        if (abs(dy) > size / 2) {
            if (dy > 0)
                dy -= size;
            else
                dy += size;
        }
    
        if (size % 2 == 0 && abs(dx) == size / 2) {
            dx = (rand() % 2 == 0) ? size / 2 : -size / 2;
        }
        if (size % 2 == 0 && abs(dy) == size / 2) {
            dy = (rand() % 2 == 0) ? size / 2 : -size / 2;
        }

        // 两玩家在同一位置
        if (dx == 0 && dy == 0) {
            return "同格";
        }

        if (dx == 0) {
            return (dy < 0) ? "正右" : "正左";
        }
        if (dy == 0) {
            return (dx < 0) ? "正下" : "正上";
        }
        string vertical = (dx < 0) ? "下" : "上";
        string horizontal = (dy < 0) ? "右" : "左";
        return horizontal + vertical;
    }

    // 热浪提示（返回true提示热浪）
    bool HeatWaveNotice(const PlayerID pid)
    {
        int cx = players[pid].x;
        int cy = players[pid].y;
        if (grid_map[cx][cy].Type() == GridType::HEAT) {
            return false;
        }

        bool in_zone = false;

        // [热源-HEAT]: 3*3 范围
        for (int dx = -1; dx <= 1 && !in_zone; dx++) {
            for (int dy = -1; dy <= 1 && !in_zone; dy++) {
                int nx = (cx + dx + size) % size;
                int ny = (cy + dy + size) % size;
                if (grid_map[nx][ny].Type() == GridType::HEAT) {
                    in_zone = true;
                }
            }
        }
        // [小太阳-HEATBOX]: 5*5 范围
        for (int dx = -2; dx <= 2 && !in_zone; dx++) {
            for (int dy = -2; dy <= 2 && !in_zone; dy++) {
                int nx = (cx + dx + size) % size;
                int ny = (cy + dy + size) % size;
                if (grid_map[nx][ny].Attach() == AttachType::HEATBOX) {
                    in_zone = true;
                }
            }
        }

        if (in_zone && !players[pid].in_heat_zone) {
            players[pid].in_heat_zone = true;
            return true;
        }
        if (!in_zone) {
            players[pid].in_heat_zone = false;
        }
        return false;
    }

    // 玩家是否在屏蔽器附近
    bool IsNearJammer(const PlayerID pid)
    {
        int cx = players[pid].x;
        int cy = players[pid].y;

        bool in_zone = false;

        // [屏蔽器-JAMMERBOX]: 3*3 范围
        for (int dx = -1; dx <= 1 && !in_zone; dx++) {
            for (int dy = -1; dy <= 1 && !in_zone; dy++) {
                int nx = (cx + dx + size) % size;
                int ny = (cy + dy + size) % size;
                if (grid_map[nx][ny].Attach() == AttachType::JAMMERBOX) {
                    in_zone = true;
                }
            }
        }

        if (in_zone) return true;
        return false;
    }

    // 获取四周墙壁信息（仅显示墙壁，不展示详细颜色）
    pair<string, string> GetSurroundingWalls(const PlayerID pid) const
    {
        Grid grid = grid_map[players[pid].x][players[pid].y];
        if (players[pid].InSubspace()) {
            grid.SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        } else {
            grid.HideSpecialWalls();
        }
        string info;
        if (grid.CanPass<Direct::UP>()) info += "空"; else info += "墙";
        if (grid.CanPass<Direct::DOWN>()) info += "空"; else info += "墙";
        if (grid.CanPass<Direct::LEFT>()) info += "空"; else info += "墙";
        if (grid.CanPass<Direct::RIGHT>()) info += "空"; else info += "墙";
        return make_pair(info, GetBoard({{grid}}, GetBoardOptions{.with_player = false, .with_ground = false}));
    }

    // [BOSS-邦邦] 周围墙壁信息
    pair<string, string> GetBangBangSurroundingWalls(const int x, const int y) const
    {
        Grid grid = grid_map[x][y];
        grid.SetType(GridType::EMPTY).SetAttach(AttachType::BOMB);
        grid.HideSpecialWalls();
        string info;
        if (grid.CanPass<Direct::UP>()) info += "空"; else info += "墙";
        if (grid.CanPass<Direct::DOWN>()) info += "空"; else info += "墙";
        if (grid.CanPass<Direct::LEFT>()) info += "空"; else info += "墙";
        if (grid.CanPass<Direct::RIGHT>()) info += "空"; else info += "墙";
        return make_pair(info, GetBoard({{grid}}, GetBoardOptions{.with_player = false}));
    }

    // 玩家随机传送
    void TeleportPlayer(const PlayerID pid)
    {
        players[pid].subspace = -1;         // 移除亚空间状态
        players[pid].in_heat_zone = false;  // 移除热浪区域状态
        players[pid].bomb_trigger = false;  // 移除炸弹触发状态
        vector<pair<int, int>> candidates = FindLargestConnectedArea();
    
        // 禁止坐标
        vector<pair<int, int>> forbiddenSources;
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                if (grid_map[i][j].Type() == GridType::EXIT || grid_map[i][j].Type() == GridType::HEAT) {
                    forbiddenSources.push_back({i, j});
                }
            }
        }
        if (boss.Enable()) {
            forbiddenSources.push_back({boss.x, boss.y});
        }
        for (const auto& player: players) {
            if (player.pid == pid || player.out > 0) continue;
            forbiddenSources.push_back({player.x, player.y});
        }
        // 禁止区域（3*3）
        vector<vector<bool>> forbidden(size, vector<bool>(size, false));
        for (const auto& src : forbiddenSources) {
            int sx = src.first, sy = src.second;
            for (int di = -1; di <= 1; di++) {
                for (int dj = -1; dj <= 1; dj++) {
                    int ni = (sx + di + size) % size;
                    int nj = (sy + dj + size) % size;
                    forbidden[ni][nj] = true;
                }
            }
        }
        // 过滤候选区域（同时去除BOSS可能到达的区域）
        vector<pair<int, int>> finalCandidates;
        std::copy_if(
            candidates.begin(), candidates.end(),
            std::back_inserter(finalCandidates),
            [&](auto const& pos) {
                auto [x, y] = pos;
                return !forbidden[x][y] && (!boss.Enable() || ManhattanDistance(x, y, boss.x, boss.y, size) > boss.steps);
            }
        );
    
        // 保险方案：无有效区域，直接在最大联通区域内随机
        if (finalCandidates.empty()) {
            finalCandidates = candidates;
        }
    
        std::shuffle(finalCandidates.begin(), finalCandidates.end(), g);
        pair<int, int> newPos = finalCandidates.front();
    
        RemovePlayerFromMap(pid);
    
        players[pid].x = newPos.first;
        players[pid].y = newPos.second;
        player_map[newPos.first][newPos.second].push_back(pid);
    }

    // 捕捉顺位变更
    void UpdatePlayerTarget(const Target type)
    {
        for (PlayerID pid = 0; pid < playerNum; ++pid) {
            if (players[pid].out > 0) continue;
            PlayerID target = type == Target::NEXT ? (pid + 1) % playerNum : (playerNum + pid - 1) % playerNum;
            while (players[target].out > 0 && target != pid) {
                players[target].target = -1;
                target = type == Target::NEXT ? (target + 1) % playerNum : (playerNum + target - 1) % playerNum;
            }
            players[pid].target = target;
        }
    }

    string MapPreview(const vector<string>& map_str) const
    {
        vector<vector<Grid>> grid_map;
        grid_map.resize(size);
        for (int i = 0; i < size; i++) {
            grid_map[i].resize(size);
        }
        for (int i = 0; i < unitMaps.origin_pos.size(); i++) {
            string map_id;
            bool is_exit = false;
            string str = map_str[i];
            std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::toupper(c); });
            if (!str.empty() && str[0] == 'E') {
                is_exit = true;
                map_id = str.substr(1);
            } else {
                map_id = str;
            }
            if (is_exit) {
                unitMaps.SetExitBlock(unitMaps.origin_pos[i].first, unitMaps.origin_pos[i].second, grid_map, map_id, SpecialEvent::NONE);   // 预览时需取用原始地图
            } else {
                unitMaps.SetMapBlock(unitMaps.origin_pos[i].first, unitMaps.origin_pos[i].second, grid_map, map_id, SpecialEvent::NONE);    // 预览时需取用原始地图
            }
        }
        FixAdjacentWalls(grid_map);
        FixInvalidPortals(grid_map);
        return GetBoard(grid_map, GetBoardOptions{.with_content = true});
    }

    int TypeCount(const GridType type) const
    {
        int count = 0;
        for (int x = 0; x < size; x++) {
            for (int y = 0; y < size; y++) {
                if (grid_map[x][y].Type() == type) {
                    count++;
                }
            }
        }
        return count;
    }

  private:
    // 初始化区块
    void InitializeBlocks()
    {
        vector<UnitMaps::Map> selected = unitMaps.RandomSelectBlocks(block_mode, exit_num);

        for (int i = 0; i < static_cast<int>(selected.size()) && i < static_cast<int>(unitMaps.pos.size()); i++) {
            if (selected[i].is_exit) {
                unitMaps.SetExitBlock(unitMaps.pos[i].first, unitMaps.pos[i].second, grid_map, selected[i].id, SpecialEvent::RANDOM);   // 此处特殊事件仅用于调用原始地图
            } else {
                unitMaps.SetMapBlock(unitMaps.pos[i].first, unitMaps.pos[i].second, grid_map, selected[i].id, SpecialEvent::RANDOM);    // 此处特殊事件仅用于调用原始地图
            }
            // 记录特殊地图坐标（暂不使用）
            // if (selected[i].is_special) {
            //     special_pos = unitMaps.pos[i];
            // }
        }
    }

    // 相邻墙面修复（优先级更高的墙壁会覆盖低优先级）
    void FixAdjacentWalls(vector<vector<Grid>>& grid_map) const
    {
        for (int x = 0; x < size; x++) {
            for (int y = 0; y < size; y++) {
                for (int d = 0; d < 4; d++) {
                    Direct dir = static_cast<Direct>(d);
                    Direct rev = opposite(dir);
                    int nx = (x + k_DX_Direct[d] + size) % size;
                    int ny = (y + k_DY_Direct[d] + size) % size;

                    Wall w1 = grid_map[x][y].GetWall(dir);
                    Wall w2 = grid_map[nx][ny].GetWall(rev);
                    Wall w = static_cast<int>(w1) > static_cast<int>(w2) ? w1 : w2;
                    if (w != w1) grid_map[x][y].SetWall(dir, w);
                    if (w != w2) grid_map[nx][ny].SetWall(rev, w);
                }
            }
        }
    }

    // 封闭传送门自动更新传送门样式
    void FixInvalidPortals(vector<vector<Grid>>& grid_map) const
    {
        for (int x = 0; x < size; x++) {
            for (int y = 0; y < size; y++) {
                if (grid_map[x][y].Type() == GridType::PORTAL && grid_map[x][y].IsFullyEnclosed()) {
                    pair<int, int> pRelPos = grid_map[x][y].PortalPos();
                    grid_map[x][y].SetType(GridType::ONEWAYPORTAL);
                    grid_map[x + pRelPos.first][y + pRelPos.second].SetType(GridType::ONEWAYPORTAL);
                }
            }
        }
    }

    bool IsBlocked(const Grid& g) const
    {
        return g.Type() == GridType::HEAT ||
            g.Attach() == AttachType::BOX ||
            g.Attach() == AttachType::HEATBOX ||
            g.Attach() == AttachType::JAMMERBOX;
    }

    // 判断(x, y)格子沿(dx, dy)方向是否连通（考虑环绕）
    bool IsConnected(int x, int y, int dx, int dy) const
    {
        int nx = (x + dx + size) % size;
        int ny = (y + dy + size) % size;
        // 当前格或目标格无法通行
        if (IsBlocked(grid_map[x][y]) || IsBlocked(grid_map[nx][ny])) {
            return false;
        }
        if (dx == -1 && dy == 0)
            return grid_map[x][y].CanPass<Direct::UP>() && grid_map[nx][ny].CanPass<Direct::DOWN>();
        else if (dx == 1 && dy == 0)
            return grid_map[x][y].CanPass<Direct::DOWN>() && grid_map[nx][ny].CanPass<Direct::UP>();
        else if (dx == 0 && dy == -1)
            return grid_map[x][y].CanPass<Direct::LEFT>() && grid_map[nx][ny].CanPass<Direct::RIGHT>();
        else if (dx == 0 && dy == 1)
            return grid_map[x][y].CanPass<Direct::RIGHT>() && grid_map[nx][ny].CanPass<Direct::LEFT>();
        return false;
    }

    int BFSRouteDistance(int sx, int sy, int ex, int ey) const
    {
        vector<vector<bool>> visited(size, vector<bool>(size, false));
        queue<tuple<int, int, int>> q; // (x, y, distance)
        q.push({sx, sy, 0});
        visited[sx][sy] = true;
        while (!q.empty()) {
            auto [x, y, d] = q.front();
            q.pop();
            if (x == ex && y == ey)
                return d;
            int dx[4] = {-1, 1, 0, 0};
            int dy[4] = {0, 0, -1, 1};
            for (int i = 0; i < 4; i++) {
                int nx = (x + dx[i] + size) % size;
                int ny = (y + dy[i] + size) % size;
                if (!visited[nx][ny] && IsConnected(x, y, dx[i], dy[i])) {
                    visited[nx][ny] = true;
                    q.push({nx, ny, d + 1});
                }
            }
        }
        return INT_MAX;
    }
    
    // * 随机生成所有玩家的位置，所有位置均在最大联通区域内 *
    // 按照顺序依次尝试：
    //   方案1：使用非逃生舱区块，相邻玩家之间的 BFS 路径距离≥5；每个玩家落点到所有逃生舱的 BFS 路径距离≥5
    //   方案2：使用非逃生舱区块，仅要求相邻玩家之间的 BFS 路径距离≥5（不检查逃生舱距离）
    //   方案3：直接使用非逃生舱区块随机分配（不检查距离）
    //   方案4：允许使用逃生舱区块
    //   保险方案：直接从最大连通区域中随机选取位置
    void RandomizePlayers()
    {
        // 1. 获取最大连通区域
        vector<pair<int, int>> largest = FindLargestConnectedArea();
        vector<vector<bool>> inLargest(size, vector<bool>(size, false));
        for (auto coord : largest)
            inLargest[coord.first][coord.second] = true;

        // 1.1 收集所有逃生舱位置
        vector<pair<int, int>> exitPositions;
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                if (grid_map[i][j].Type() == GridType::EXIT)
                    exitPositions.push_back({i, j});
            }
        }

        // Lambda：计算 (x,y) 的连通度（即周围可连通方向数量）
        auto connectivityDegree = [this](int x, int y) -> int {
            int degree = 0;
            int dx[4] = {-1, 1, 0, 0};
            int dy[4] = {0, 0, -1, 1};
            for (int k = 0; k < 4; k++) {
                if (IsConnected(x, y, dx[k], dy[k]))
                    degree++;
            }
            return degree;
        };

        // 2. 构造候选区块：遍历 UnitMaps::pos 中所有区块
        //      - blockPos：区块左上角坐标
        //      - validCells：区块内所有满足在最大联通区域且连通度 ≥ 2 的候选落点
        //      - isExit：若区块内含有逃生舱，则为 true
        struct BlockCandidate {
            pair<int, int> blockPos;
            vector<pair<int, int>> validCells;
            bool isExit;
        };
        vector<BlockCandidate> candidateBlocks;
        for (auto pos : unitMaps.pos) {
            int bx = pos.first, by = pos.second;
            bool containsExit = false;
            vector<pair<int, int>> validCells;
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    if (grid_map[bx + i][by + j].Type() == GridType::EXIT) {
                        containsExit = true;
                        break;
                    }
                    if (!inLargest[bx + i][by + j])
                        continue;
                    if (connectivityDegree(bx + i, by + j) >= 2)
                        validCells.push_back({bx + i, by + j});
                }
                if (containsExit)
                    break;
            }
            candidateBlocks.push_back({{bx, by}, validCells, containsExit});
        }

        // 3. 筛选非逃生舱区块（用于方案1、2、3）
        vector<BlockCandidate> candidateNonExit;
        for (auto &b : candidateBlocks) {
            if (!b.isExit && !b.validCells.empty())
                candidateNonExit.push_back(b);
        }
        // 为使得候选区块分布随机，先随机打乱候选非逃生舱区块顺序
        std::shuffle(candidateNonExit.begin(), candidateNonExit.end(), g);

        // 4. 方案1：使用非逃生舱区块，相邻玩家之间的 BFS 路径距离≥5；每个玩家落点到所有逃生舱的 BFS 路径距离≥5
        vector<pair<int, int>> assigned(playerNum, {-1, -1});
        vector<bool> used(candidateNonExit.size(), false);
        // 回溯函数，checkExit 表示是否需要额外检查落点到逃生舱的距离
        function<bool(int, bool)> bt = [&](int idx, bool checkExit) -> bool {
            if (idx == playerNum) {
                int d = BFSRouteDistance(assigned[playerNum - 1].first, assigned[playerNum - 1].second,
                                        assigned[0].first, assigned[0].second);
                return d >= 5 || playerNum == 1;
            }
            for (int i = 0; i < candidateNonExit.size(); i++) {
                if (used[i])
                    continue;
                // 对当前区块随机遍历其候选落点
                vector<pair<int, int>> cellCandidates = candidateNonExit[i].validCells;
                std::shuffle(cellCandidates.begin(), cellCandidates.end(), g);
                for (auto candidate : cellCandidates) {
                    if (idx > 0) {
                        int d = BFSRouteDistance(assigned[idx - 1].first, assigned[idx - 1].second,
                                                candidate.first, candidate.second);
                        if (d < 5)
                            continue;
                    }
                    if (checkExit) {
                        bool ok = true;
                        for (auto &ex : exitPositions) {
                            int d = BFSRouteDistance(candidate.first, candidate.second, ex.first, ex.second);
                            if (d < 5) { ok = false; break; }
                        }
                        if (!ok)
                            continue;
                    }
                    assigned[idx] = candidate;
                    used[i] = true;
                    if (bt(idx + 1, checkExit))
                        return true;
                    used[i] = false;
                }
            }
            return false;
        };

        if (candidateNonExit.size() >= (unsigned)playerNum && bt(0, true)) {
            // 方案1成功
            for (int i = 0; i < playerNum; i++) {
                int x = assigned[i].first, y = assigned[i].second;
                players[i].x = x;
                players[i].y = y;
                player_map[x][y].push_back(i);
            }
            return;
        }

        // 5. 方案2：使用非逃生舱区块，仅要求相邻玩家之间的 BFS 路径距离≥5（不检查逃生舱距离）
        std::fill(used.begin(), used.end(), false);
        if (candidateNonExit.size() >= (unsigned)playerNum && bt(0, false)) {
            for (int i = 0; i < playerNum; i++) {
                int x = assigned[i].first, y = assigned[i].second;
                players[i].x = x;
                players[i].y = y;
                player_map[x][y].push_back(i);
            }
            return;
        }

        // 6. 方案3：直接使用非逃生舱区块随机分配（不检查距离）
        if (candidateNonExit.size() >= (unsigned)playerNum) {
            vector<int> indices(candidateNonExit.size());
            for (int i = 0; i < candidateNonExit.size(); i++)
                indices[i] = i;
            std::shuffle(indices.begin(), indices.end(), g);
            for (int i = 0; i < playerNum; i++) {
                auto &block = candidateNonExit[indices[i]];
                vector<pair<int, int>> cells = block.validCells;
                std::shuffle(cells.begin(), cells.end(), g);
                pair<int, int> chosen = cells.front();
                players[i].x = chosen.first;
                players[i].y = chosen.second;
                player_map[chosen.first][chosen.second].push_back(i);
            }
            return;
        }

        // 7. 方案4：允许使用逃生舱区块
        vector<BlockCandidate> candidateAll;
        for (auto &b : candidateBlocks) {
            if (!b.validCells.empty())
                candidateAll.push_back(b);
        }
        if (candidateAll.size() >= (unsigned)playerNum) {
            vector<int> indices(candidateAll.size());
            for (int i = 0; i < candidateAll.size(); i++)
                indices[i] = i;
            std::shuffle(indices.begin(), indices.end(), g);
            for (int i = 0; i < playerNum; i++) {
                auto &block = candidateAll[indices[i]];
                vector<pair<int, int>> cells = block.validCells;
                std::shuffle(cells.begin(), cells.end(), g);
                pair<int, int> chosen = cells.front();
                players[i].x = chosen.first;
                players[i].y = chosen.second;
                player_map[chosen.first][chosen.second].push_back(i);
            }
            return;
        }

        // 8. 保险方案：直接从最大连通区域随机选取位置
        std::shuffle(largest.begin(), largest.end(), g);
        for (int i = 0; i < playerNum; i++) {
            players[i].x = largest[i].first;
            players[i].y = largest[i].second;
            player_map[largest[i].first][largest[i].second].push_back(i);
        }
    }

    // 查找最大联通区域
    vector<pair<int, int>> FindLargestConnectedArea()
    {
        vector<vector<bool>> visited(size, vector<bool>(size, false));
        vector<vector<pair<int, int>>> components;

        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                if (visited[i][j]) continue;

                vector<pair<int, int>> comp;
                queue<pair<int, int>> q;
                q.push({i, j});
                visited[i][j] = true;
                while (!q.empty()) {
                    auto [cx, cy] = q.front();
                    q.pop();
                    comp.push_back({cx, cy});
                    int dx[4] = {-1, 1, 0, 0};
                    int dy[4] = {0, 0, -1, 1};
                    for (int k = 0; k < 4; k++) {
                        int nx = (cx + dx[k] + size) % size;
                        int ny = (cy + dy[k] + size) % size;
                        if (!visited[nx][ny] && IsConnected(cx, cy, dx[k], dy[k])) {
                            visited[nx][ny] = true;
                            q.push({nx, ny});
                        }
                    }
                }
                components.push_back(comp);
            }
        }

        vector<pair<int, int>> largest;
        for (auto &comp : components) {
            if (comp.size() > largest.size())
                largest = comp;
        }
        if (largest.empty()) assert(false);
        return largest;
    }

    string GetImageTypeFolder() const
    {
        switch (image_texture_) {
            case Texture::CLASSIC:  return "classic/";
            case Texture::RETRO:    return "retro/";
            default: return "";
        }
    }

    // 获取样式
    string GetGridStyle(const GridType type, const AttachType attach, const bool is_bg) const
    {
        string bg_image = ToFileUrl(image_path_ + GetImageTypeFolder() + GetGridImage(type));
        string attach_image = ToFileUrl(image_path_ + GetImageTypeFolder() + GetAttachImage(attach));

        if (is_bg) {
            return "style=\""
                "background-image: url('" + attach_image + "'), url('" + bg_image + "');"
                "background-repeat: no-repeat, no-repeat;"
                "background-position: center, center;"
                "background-size: contain, cover;"
                "\"";
        } else {
            return "<div style=\"position: relative; width: 50px; height: 50px;\">"
                "<img src='" + bg_image + "' style='width:100%; height:100%;'>"
                "<img src='" + attach_image + "' style='position:absolute; top:0; left:0; width:100%; height:100%;'>"
                "</div>";
        }
    }

    string GetWallStyle(const Wall wall, const string& direction) const
    {
        const string folder = GetImageTypeFolder() + "walls/";
        string wall_image = ToFileUrl(image_path_ + folder + GetWallImage(wall, direction));
        return "background-image: url('" + wall_image + "');";
    }

    static string GetColoredMark(const string& content, const GridType type, const AttachType attach)
    {
        string color;

        switch (type) {
            case GridType::PORTAL:  color = "#FFFF00"; break;
            case GridType::ONEWAYPORTAL:  color = "#FFF8E7"; break;
            // case GridType::HEAT:    color = "#00FFFF"; break;
            default:;
        }
        switch (attach) {
            case AttachType::BOMB:      color = "#FFFF00"; break;
            case AttachType::BOX:       color = "#FFFF00"; break;
            case AttachType::HEATBOX:   color = "#FFFF00"; break;
            case AttachType::JAMMERBOX: color = "#FFFF00"; break;
            default:;
        }

        if (color.empty()) return "<b>" + content + "</b>";
        return "<font color=\"" + color + "\"><b>" + content + "</b></font>";
    }

    static string GetBoardStyle()
    {
        return R"(
<style>
    .grid {
        width: )" + to_string(GRID_SIZE) + R"(px;
        height: )" + to_string(GRID_SIZE) + R"(px;
        font-size: 19px;
        line-height: 1;
        background-size: cover;
        background-repeat: no-repeat;
    }
    .wall-row {
        width: )" + to_string(GRID_SIZE) + R"(px;
        height: )" + to_string(WALL_SIZE) + R"(px;
        font-size: 8px;
        line-height: 8px;
        background-size: cover;
        background-repeat: no-repeat;
    }
    .wall-col {
        width: )" + to_string(WALL_SIZE) + R"(px;
        height: )" + to_string(GRID_SIZE) + R"(px;
        font-size: 8px;
        line-height: 8px;
        background-size: cover;
        background-repeat: no-repeat;
    }
    .corner {
        width: )" + to_string(WALL_SIZE) + R"(px;
        height: )" + to_string(WALL_SIZE) + R"(px;
    }
</style>
)";
    }
};
