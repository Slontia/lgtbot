// 感谢 BC 提供的区块初始化思路
class UnitMaps {
  public:
    const int k_map_num = 12;
    const int k_exit_num = 4;
    const int k_special_num = 2;

    vector<pair<int, int>> pos = {
        {0, 0}, {0, 3}, {0, 6},
        {3, 0}, {3, 3}, {3, 6},
        {6, 0}, {6, 3}, {6, 6},
    };
    vector<pair<int, int>> origin_pos = pos;

    struct Map {
        vector<vector<Grid>> block;
        string id;
        string title;
        GridType grid;
        AttachType attach;
        bool is_exit = false;
        bool is_special = false;

        Map(vector<vector<Grid>> b, string i, string t, GridType g, AttachType a = AttachType::EMPTY, bool s = false)
            : block(std::move(b)), id(std::move(i)), title(std::move(t)), grid(g), attach(a), is_special(s) {}
        Map(vector<vector<Grid>> b, string i, string t, bool e, GridType g, AttachType a = AttachType::EMPTY)
            : block(std::move(b)), id(std::move(i)), title(std::move(t)), is_exit(e), grid(g), attach(a) {}
    };

    vector<Map> all_maps = {
        {Map1(), "1", "水廊", GridType::WATER},
        {Map2(), "2", "表里空间A", GridType::PORTAL},
        {Map3(), "3", "旋转楼道A", GridType::GRASS}, 
        {Map4(), "4", "缠绕走廊A", GridType::GRASS},
        {Map5(), "5", "水花大厅", GridType::WATER},
        {Map6(), "6", "表里空间B", GridType::PORTAL},
        {Map7(), "7", "旋转楼道B", GridType::GRASS},
        {Map8(), "8", "缠绕走廊B", GridType::GRASS},
        {Map9(), "9", "藏书阁A", GridType::EMPTY},
        {Map10(), "10", "藏书阁B", GridType::EMPTY},
        {Map11(), "11", "横廊", GridType::GRASS},
        {Map12(), "12", "竖廊", GridType::GRASS},
        {Map13(), "13", "镜面迷宫A", GridType::PORTAL},
        {Map14(), "14", "镜面迷宫B", GridType::PORTAL},
        {Map15(), "15", "伪装亚空间", GridType::PORTAL},
        {Map16(), "16", "隐藏房间", GridType::PORTAL},
        {Map17(), "17", "植物园A", GridType::WATER},
        {Map18(), "18", "植物园B", GridType::WATER},
        {Map19(), "19", "换鞋区A", GridType::EMPTY},
        {Map20(), "20", "换鞋区B", GridType::EMPTY},
        {Map21(), "21", "转角A", GridType::EMPTY},
        {Map22(), "22", "转角B", GridType::EMPTY},
        {Map23(), "23", "捕鸟陷阱A", GridType::TRAP},
        {Map24(), "24", "捕鸟陷阱B", GridType::TRAP},
        {Map25(), "25", "岩浆井A", GridType::HEAT},
        {Map26(), "26", "岩浆井B", GridType::HEAT},
        {Map27(), "27", "仓库A", GridType::EMPTY, AttachType::BOX},
        {Map28(), "28", "仓库B", GridType::EMPTY, AttachType::BOX},
        {Map29(), "29", "湖边A", GridType::WATER},
        {Map30(), "30", "湖边B", GridType::WATER},
        {Map31(), "31", "捕兽陷阱", GridType::TRAP},
        {Map32(), "32", "空间裂隙", GridType::PORTAL},
        {Map33(), "33", "长廊", GridType::EMPTY, AttachType::BUTTON},
        {Map34(), "34", "忏悔室", GridType::EMPTY, AttachType::BUTTON},
        {Map35(), "35", "旋转门A", GridType::EMPTY, AttachType::BUTTON},
        {Map36(), "36", "旋转门B", GridType::EMPTY, AttachType::BUTTON},
        {Map37(), "37", "会客厅", GridType::GRASS, AttachType::BUTTON},
        {Map38(), "38", "公共厕所", GridType::WATER, AttachType::BUTTON},
        {Map39(), "39", "女卫生间", GridType::TRAP, AttachType::BUTTON},
        {Map40(), "40", "男卫生间", GridType::TRAP, AttachType::BUTTON},
        {Map41(), "41", "红石迷宫A", GridType::GRASS, AttachType::BUTTON},
        {Map42(), "42", "红石迷宫B", GridType::GRASS, AttachType::BUTTON},
        {Map43(), "43", "小太阳", GridType::EMPTY, AttachType::HEATBOX},
        {Map44(), "44", "屏蔽器", GridType::EMPTY, AttachType::JAMMERBOX},
        // {Map31(), "31", "单向门A", GridType::PORTAL},
        // {Map32(), "32", "单向门B", GridType::PORTAL},
    };
    vector<Map> all_exits = {
        {Exit1(), "1", "逃生长廊A", true, GridType::EMPTY},
        {Exit2(), "2", "逃生长廊B", true, GridType::EMPTY}, 
        {Exit3(), "3", "逃生长廊C", true, GridType::EMPTY},
        {Exit4(), "4", "逃生长廊D", true, GridType::EMPTY},
        {Exit5(), "5", "快速逃生通道A", true, GridType::PORTAL},
        {Exit6(), "6", "快速逃生通道B", true, GridType::PORTAL},
        {Exit7(), "7", "亚空间逃生舱A", true, GridType::PORTAL},
        {Exit8(), "8", "亚空间逃生舱B", true, GridType::PORTAL},
        {Exit9(), "9", "逃生地道A", true, GridType::TRAP},
        {Exit10(), "10", "逃生地道B", true, GridType::TRAP},
        {Exit11(), "11", "隐蔽逃生通道A", true, GridType::EMPTY, AttachType::BUTTON},
        {Exit12(), "12", "隐蔽逃生通道B", true, GridType::EMPTY, AttachType::BUTTON},
        {Exit13(), "13", "银行金库", true, GridType::PORTAL, AttachType::JAMMERBOX},
        {Exit14(), "14", "三箱之力", true, GridType::EMPTY, AttachType::BOX},
    };
    vector<Map> all_special_maps = {
        {MapS1(), "S1", "实验场", GridType::HEAT, AttachType::EMPTY, true},
        {MapS2(), "S2", "原子空间", GridType::PORTAL, AttachType::EMPTY, true},
        {MapS3(), "S3", "原子阱", GridType::PORTAL, AttachType::EMPTY, true},
        {MapS4(), "S4", "黑洞", GridType::PORTAL, AttachType::EMPTY, true},
    };

    // 幻变
    const vector<string> twist_mode_ids = {
        "2", "6", "3", "7",
        "9", "10", "11", "12",
        "19", "20", "21", "22",
        "5", "25", "26", "27",
        "29", "30", "43", "44",
        "E1", "E2", "E3", "E4",
        "E13", "E14",
    };
    // 按钮
    const vector<string> button_mode_ids = {
        "2", "6", "3", "7",
        "4", "8", "9", "10",
        "21", "22", "25", "26",
        "27", "37", "33", "34",
        "35", "36", "39", "40",
        "E1", "E2", "E3", "E4",
        "E11", "E12",
    };
    // 陷阱
    const vector<string> trap_mode_ids = {
        "2", "6", "9", "10",
        "19", "20", "21", "22",
        "23", "24", "25", "27",
        "31", "32", "39", "40",
        "16", "S3",
        "E1", "E2", "E3", "E4",
        "E9", "E10",
    };
    vector<Map> pool_maps;
    vector<Map> pool_exits;

    vector<Map> maps;
    vector<Map> exits;
    vector<Map> special_maps;

    UnitMaps() = default;

    UnitMaps(const BlockMode mode, std::mt19937& gen, const vector<string>& custom_blocks): g(std::make_unique<std::mt19937>(gen))
    {
        if (mode == BlockMode::CLASSIC) {
            // 经典-CLASSIC
            maps.insert(maps.end(), all_maps.begin(), all_maps.begin() + k_map_num);
            exits.insert(exits.end(), all_exits.begin(), all_exits.begin() + k_exit_num);
        } else if (mode == BlockMode::TWIST) {
            // 幻变-TWIST
            SampleBlockPoolsFromIds(twist_mode_ids);
        } else if (mode == BlockMode::WILD) {
            // 狂野-WILD
            std::sample(all_maps.begin(), all_maps.end(), std::back_inserter(maps), k_map_num, *g);
            SampleExits(all_exits, k_exit_num / 2);
        } else if (mode == BlockMode::CRAZY) {
            // 疯狂-CRAZY
            std::sample(all_special_maps.begin(), all_special_maps.end(), std::back_inserter(special_maps), k_special_num, *g);
            maps.insert(maps.end(), special_maps.begin(), special_maps.end());
            std::sample(all_maps.begin(), all_maps.end(), std::back_inserter(maps), k_map_num - k_special_num, *g);
            SampleExits(all_exits, k_exit_num / 2);
        } else if (mode == BlockMode::BUTTON) {
            // 按钮-BUTTON
            SampleBlockPoolsFromIds(button_mode_ids);
        } else if (mode == BlockMode::TRAP) {
            // 陷阱-TRAP
            SampleBlockPoolsFromIds(trap_mode_ids);
        } else {
            // 自定义-CUSTOM
            for (const auto& block : custom_blocks) {
                if (auto it = std::find_if(all_special_maps.begin(), all_special_maps.end(), [&](const Map& m) { return m.id == block; });
                    it != all_special_maps.end()) {
                    maps.push_back(*it);
                }
                const bool is_exit = !block.empty() && block[0] == 'E';
                string search_id = is_exit ? block.substr(1) : block;
                const auto& list = is_exit ? all_exits : all_maps;
                if (auto it = std::find_if(list.begin(), list.end(), [&](const Map& m) { return m.id == search_id; });
                    it != list.end()) {
                    (is_exit ? exits : maps).push_back(*it);
                }
            }
        }
    }

    void SampleBlockPoolsFromIds(const vector<string>& mode_ids)
    {
        maps.clear(); exits.clear();
        pool_maps.clear(); pool_exits.clear();

        for (const auto& map_id : mode_ids) {
            const bool is_exit = !map_id.empty() && map_id[0] == 'E';
            const bool is_special = !map_id.empty() && map_id[0] == 'S';
            const std::string id = is_exit ? map_id.substr(1) : map_id;

            const auto& source = is_exit ? all_exits : (is_special ? all_special_maps : all_maps);
            auto it = std::find_if(source.begin(), source.end(), [&id](const Map& m) { return m.id == id; });

            if (it != source.end()) {
                if (is_exit) pool_exits.push_back(*it);
                else pool_maps.push_back(*it);
            }
        }

        std::sample(pool_maps.begin(), pool_maps.end(), std::back_inserter(maps), k_map_num, *g);
        SampleExits(pool_exits, k_exit_num / 2);

        std::sort(maps.begin(), maps.end(), [](const Map& a, const Map& b) { return CompareMapId(a.id, b.id); });
        std::sort(exits.begin(), exits.end(), [](const Map& a, const Map& b) { return CompareMapId(a.id, b.id); });
    }

    void SampleExits(const vector<Map>& exits_pool, const int k_exit_pair)
    {
        if (exits_pool.size() % 2 != 0) return;

        int pair_num = exits_pool.size() / 2;
        vector<int> pairs(pair_num);
        std::iota(pairs.begin(), pairs.end(), 0);
        std::shuffle(pairs.begin(), pairs.end(), *g);
        pairs.resize(k_exit_pair);
        std::sort(pairs.begin(), pairs.end());
        for (int p : pairs) {
            exits.push_back(exits_pool[p * 2]);
            exits.push_back(exits_pool[p * 2 + 1]);
        }
    }

    bool HasHeatConflictPool(const vector<Map>& maps_pool) const
    {
        bool has_heat = false, has_heatbox = false;
        for (const auto& map : maps_pool) {
            if (IsHeatMap(map)) has_heat = true;
            if (IsHeatBoxMap(map)) has_heatbox = true;
        }
        return has_heat && has_heatbox;
    }

    // 自定义模式下，判断“热源/小太阳互斥规则”是否足够抽满普通区块，用【其余区块 + min(热源数, 小太阳数)】做保守检测
    bool CanApplyHeatExclusiveRule(const vector<Map>& maps_pool, const int required_map_num) const
    {
        int heat_count = 0;
        int heatbox_count = 0;
        int other_count = 0;

        for (const auto& map : maps_pool) {
            if (IsHeatMap(map)) ++heat_count;
            else if (IsHeatBoxMap(map)) ++heatbox_count;
            else ++other_count;
        }

        return other_count + std::min(heat_count, heatbox_count) >= required_map_num;
    }

    // 区块抽取随机
    vector<Map> RandomSelectBlocks(const BlockMode block_mode, const int exit_num) const
    {
        vector<Map> maps_pool = maps;
        vector<Map> exits_pool = exits;
        std::shuffle(maps_pool.begin(), maps_pool.end(), *g);
        std::shuffle(exits_pool.begin(), exits_pool.end(), *g);

        const int total_pos = static_cast<int>(pos.size());

        int selected_map_num = 0;
        int selected_exit_num = 0;

        if (block_mode != BlockMode::CUSTOM) {
            // 非自定义模式
            selected_map_num = total_pos - exit_num;
            selected_exit_num = exit_num;
        } else {
            // 自定义模式
            const int exit_count = static_cast<int>(exits_pool.size());

            if (exit_count < exit_num) {
                selected_exit_num = exit_count;
                selected_map_num = total_pos - selected_exit_num;
            } else {
                selected_exit_num = exit_num;
                selected_map_num = total_pos - selected_exit_num;
            }
        }

        // 热源 / 小太阳互斥规则
        bool enable_heat_exclusive_rule = false;
        if (HasHeatConflictPool(maps_pool)) {
            if (block_mode == BlockMode::CUSTOM) {
                enable_heat_exclusive_rule = CanApplyHeatExclusiveRule(maps_pool, selected_map_num);
            } else {
                enable_heat_exclusive_rule = true;
            }
        }

        vector<Map> selected_maps;
        if (enable_heat_exclusive_rule) {
            vector<Map> normal_maps;
            vector<Map> heat_maps;
            vector<Map> heatbox_maps;

            for (const auto& map : maps_pool) {
                if (IsHeatMap(map)) heat_maps.push_back(map);
                else if (IsHeatBoxMap(map)) heatbox_maps.push_back(map);
                else normal_maps.push_back(map);
            }

            // 二选一，只保留一种热系区块
            const bool choose_heat =
                heatbox_maps.empty() ? true :
                heat_maps.empty() ? false :
                (std::uniform_int_distribution<int>(0, 1)(*g) == 0);

            vector<Map> candidate_maps = normal_maps;
            if (choose_heat) {
                candidate_maps.insert(candidate_maps.end(), heat_maps.begin(), heat_maps.end());
            } else {
                candidate_maps.insert(candidate_maps.end(), heatbox_maps.begin(), heatbox_maps.end());
            }

            std::shuffle(candidate_maps.begin(), candidate_maps.end(), *g);

            // 双保险，避免极端情况下越界
            selected_map_num = std::min(selected_map_num, static_cast<int>(candidate_maps.size()));
            for (int i = 0; i < selected_map_num; ++i) {
                selected_maps.push_back(candidate_maps[i]);
            }
        } else {
            // 规则关闭，直接全池随机，防止自定义区块不足时越界
            selected_map_num = std::min(selected_map_num, static_cast<int>(maps_pool.size()));
            for (int i = 0; i < selected_map_num; ++i) {
                selected_maps.push_back(maps_pool[i]);
            }
        }

        vector<Map> selected;
        selected.reserve(selected_maps.size() + selected_exit_num);

        for (auto& map : selected_maps) {
            selected.push_back(map);
        }

        selected_exit_num = std::min(selected_exit_num, static_cast<int>(exits_pool.size()));
        for (int i = 0; i < selected_exit_num; ++i) {
            selected.push_back(exits_pool[i]);
        }

        std::shuffle(selected.begin(), selected.end(), *g);
        return selected;
    }

    vector<vector<Grid>> FindBlockById(const string& id, bool is_exit, SpecialEvent event) const
    {
        const bool has_event = event != SpecialEvent::NONE;

        const auto& special_list = has_event ? special_maps : all_special_maps;
        if (const Map* map = FindMapById(special_list, id))
            return map->block;

        const auto& normal_list = is_exit
            ? (has_event ? exits : all_exits)
            : (has_event ? maps : all_maps);
        if (const Map* map = FindMapById(normal_list, id))
            return map->block;

        return InitializeMapTemplate();
    }

    bool IsBlockExist(const string& id, bool is_exit) const
    {
        if (FindMapById(all_special_maps, id))
            return true;

        const auto& list = is_exit ? all_exits : all_maps;
        return FindMapById(list, id) != nullptr;
    }

    const Map* FindMapById(const vector<Map>& maps, const string& id) const
    {
        auto it = std::find_if(maps.begin(), maps.end(),
            [&id](const Map& map) { return map.id == id; });

        return it != maps.end() ? &(*it) : nullptr;
    }

    static bool MapContainGridType(const vector<Map>& maps, const GridType& type)
    {
        for (const auto& map: maps) {
            for (int k = 0; k < 9; ++k) {
                if (map.block[k / 3][k % 3].Type() == type) {
                    return true;
                }
            }
        }
        return false;
    }

    static bool MapContainAttachType(const vector<Map>& maps, const AttachType& type)
    {
        for (const auto& map: maps) {
            for (int k = 0; k < 9; ++k) {
                if (map.block[k / 3][k % 3].Attach() == type) {
                    return true;
                }
            }
        }
        return false;
    }

    static bool MapContainWallType(const vector<Map>& maps, const Wall& wall)
    {
        for (const auto& map: maps) {
            for (int k = 0; k < 9; ++k) {
                if (map.block[k / 3][k % 3].ContainWallType(wall)) {
                    return true;
                }
            }
        }
        return false;
    }

    static SpecialEvent GetRandomSpecialEvent()
    {
        static constexpr std::array events = {
            SpecialEvent::LAZYGARDENER,
            SpecialEvent::OVERGROWTH,
            SpecialEvent::RAINSTORY,
        };
        return events[rand() % events.size()];
    }

    // 特殊事件详情
    static string ShowSpecialEvent(const SpecialEvent event)
    {
        switch (event) {
            case SpecialEvent::LAZYGARDENER: return "[特殊事件]【怠惰的园丁】树丛将在其区块内随机位置生成（有可能生成在中间）";
            case SpecialEvent::OVERGROWTH:   return "[特殊事件]【营养过剩】树丛和陷阱将额外向随机1个方向再次生成1个树丛（不可隔墙生长）";
            case SpecialEvent::RAINSTORY:    return "[特殊事件]【雨天小故事】地图中所有树丛变成水洼，陷阱会发出啪啪声";
            default:                         return "[特殊事件]【无】";
        }
    }

    // 特殊事件1——怠惰的园丁：草丛将在其区块内随机位置生成
    void SpecialEvent1()
    {
        auto MarkMaps = [](auto& maps) {
            for (auto& map : maps) {
                for (int k = 0; k < 9; ++k) {
                    int i = k / 3, j = k % 3;
                    if (map.block[i][j].Type() == GridType::GRASS)
                        map.block[i][j].SetContent("？");
                }
            }
        };
        auto ProcessMaps = [](auto& maps) {
            for (auto& map: maps) {
                for (int k = 0; k < 9; ++k) {
                    int i = k / 3, j = k % 3;
                    if (map.block[i][j].Type() == GridType::GRASS) {
                        map.block[i][j].SetType(GridType::EMPTY);
                        int m;
                        do {
                            m = rand() % 9;
                        } while (map.block[m / 3][m % 3].Type() != GridType::EMPTY);
                        map.block[m / 3][m % 3].SetType(GridType::GRASS);
                        break;
                    }
                }
            }
        };

        MarkMaps(all_maps);
        MarkMaps(all_exits);
        MarkMaps(all_special_maps);
        ProcessMaps(maps);
        ProcessMaps(exits);
        ProcessMaps(special_maps);
    }

    // 特殊事件2——营养过剩：树丛和陷阱将额外向随机1个方向再次生成1个树丛
    void SpecialEvent2()
    {
        auto MarkMaps = [](auto& maps) {
            for (auto& map : maps) {
                for (int k = 0; k < 9; ++k) {
                    int i = k / 3, j = k % 3;
                    if (map.block[i][j].CanGrow())
                        map.block[i][j].SetContent("？");
                }
            }
        };
        auto ProcessMaps = [this](auto& maps) {
            for (auto& map : maps) {
                vector<pair<int, int>> growablePositions;
                int grassCount = 0;
                for (int i = 0; i < 3; ++i) {
                    for (int j = 0; j < 3; ++j) {
                        if (map.block[i][j].Type() == GridType::GRASS || map.block[i][j].Type() == GridType::TRAP) {
                            grassCount++;
                        }
                        if (map.block[i][j].CanGrow()) {
                            growablePositions.emplace_back(i, j);
                        }
                    }
                }
                int targetGrowth = min(grassCount, static_cast<int>(growablePositions.size()));
                if (targetGrowth == 0) continue;    // 无法生长则跳过

                std::shuffle(growablePositions.begin(), growablePositions.end(), *g);
                for (int i = 0; i < targetGrowth; i++) {
                    int x = growablePositions[i].first;
                    int y = growablePositions[i].second;
                    map.block[x][y].SetType(GridType::GRASS);
                }
            }
        };

        MarkMaps(all_maps);
        MarkMaps(all_exits);
        MarkMaps(all_special_maps);
        ProcessMaps(maps);
        ProcessMaps(exits);
        ProcessMaps(special_maps);
    }

    // 特殊事件3——雨天小故事：地图中所有树丛变成水洼
    void SpecialEvent3()
    {
        auto ProcessMaps = [](auto& maps) {
            for (auto& map: maps) {
                for (int k = 0; k < 9; ++k) {
                    int i = k / 3, j = k % 3;
                    if (map.block[i][j].Type() == GridType::GRASS) {
                        map.block[i][j].SetType(GridType::WATER);
                    }
                }
            }
        };

        ProcessMaps(maps);
        ProcessMaps(exits);
        ProcessMaps(special_maps);
        ProcessMaps(all_special_maps);
    }

    // 大地图区块位置随机
    bool RandomizeBlockPosition(const int size)
    {
        vector<pair<int,int>> candidates;
        for (int i = 0; i < size - 2; i++) {
            for (int j = 0; j < size - 2; j++) {
                candidates.push_back({i, j});
            }
        }
        std::shuffle(candidates.begin(), candidates.end(), *g);
        vector<pair<int,int>> chosen;
        bool found = backtrack(0, chosen, candidates);
        if (found) {
            for (int i = 0; i < 9; i++) {
                pos[i] = chosen[i];
            }
        }
        return found;
    }

    // 边长12：16个区块
    void SetMapPosition12()
    {
        pos.insert(pos.begin() + 3, {0, 9});
        pos.insert(pos.begin() + 6, {3, 9});
        pos.push_back({6, 9});
        pos.push_back({9, 0});
        pos.push_back({9, 3});
        pos.push_back({9, 6});
        pos.push_back({9, 9});
        origin_pos = pos;
    }

    void SetMapBlock(const int x, const int y, vector<vector<Grid>>& grid_map, const string& map_id, const SpecialEvent event) const
    {
        SetBlock(x, y, grid_map, FindBlockById(map_id, false, event));
    }

    void SetExitBlock(const int x, const int y, vector<vector<Grid>>& grid_map, const string& exit_id, const SpecialEvent event) const
    {
        SetBlock(x, y, grid_map, FindBlockById(exit_id, true, event));
    }

  private:
    std::unique_ptr<std::mt19937> g;

    bool IsHeatMap(const Map& map) const { return map.grid == GridType::HEAT; }

    bool IsHeatBoxMap(const Map& map) const { return map.attach == AttachType::HEATBOX; }

    void SetBlock(const int x, const int y, vector<vector<Grid>>& grid_map, const vector<vector<Grid>> block) const
    {
        for (int i = 0; i < block.size(); i++) {
            for (int j = 0; j < block[i].size(); j++) {
                grid_map[x + i][y + j] = block[i][j];
            }
        }
    }

    static bool overlaps(const pair<int, int>& a, const pair<int, int>& b)
    {
        if (a.first + 2 < b.first || b.first + 2 < a.first || a.second + 2 < b.second || b.second + 2 < a.second)
            return false;
        return true;
    }

    // 回溯搜索
    static bool backtrack(int index, vector<pair<int, int>>& chosen, const vector<pair<int, int>>& candidates)
    {
        if (chosen.size() == 9) {
            return true;
        }
        for (int i = index; i < candidates.size(); i++) {
            bool valid = true;
            for (const auto& p : chosen) {
                if (overlaps(p, candidates[i])) {
                    valid = false;
                    break;
                }
            }
            if (!valid)
                continue;
            chosen.push_back(candidates[i]);
            if (backtrack(i + 1, chosen, candidates))
                return true;
            chosen.pop_back();
        }
        return false;
    }

    static vector<vector<Grid>> InitializeMapTemplate()
    {
        vector<vector<Grid>> map;
        map.resize(3);
        for (auto& row : map) {
            row.resize(3);
        }
        return map;
    }

    /* ========== 常规地图 ========== */
    static vector<vector<Grid>> Map1()
    {
        auto map = InitializeMapTemplate();
        map[0][0].SetType(GridType::WATER);
        map[0][2].SetType(GridType::WATER);
        map[2][0].SetType(GridType::WATER);
        map[2][2].SetType(GridType::WATER);

        map[0][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[0][1].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[0][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        map[1][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);
        map[1][1].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::NORMAL, Wall::NORMAL);
        map[1][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);

        map[2][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[2][1].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[2][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        return map;
    }

    static vector<vector<Grid>> Map2()
    {
        auto map = InitializeMapTemplate();
        map[0][2].SetType(GridType::PORTAL).SetPortal(2, -2);
        map[2][0].SetType(GridType::PORTAL).SetPortal(-2, 2);
        map[1][1].SetType(GridType::GRASS);

        map[0][0].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);
        map[0][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL).SetGrowable(true);
        map[0][2].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);

        map[1][0].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[1][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);
        map[1][2].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::NORMAL, Wall::EMPTY);

        map[2][0].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::NORMAL);
        map[2][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY).SetGrowable(true);
        map[2][2].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);

        return map;
    }

    static vector<vector<Grid>> Map3()
    {
        auto map = InitializeMapTemplate();
        map[1][1].SetType(GridType::GRASS);

        map[0][0].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::NORMAL, Wall::EMPTY).SetGrowable(true);
        map[0][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL).SetGrowable(true);
        map[0][2].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY).SetGrowable(true);

        map[1][0].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);
        map[1][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[1][2].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);

        map[2][0].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::NORMAL).SetGrowable(true);
        map[2][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY).SetGrowable(true);
        map[2][2].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL).SetGrowable(true);

        return map;
    }

    static vector<vector<Grid>> Map4()
    {
        auto map = InitializeMapTemplate();
        map[1][1].SetType(GridType::GRASS);

        map[0][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[0][1].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY).SetGrowable(true);
        map[0][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL).SetGrowable(true);

        map[1][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[1][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);
        map[1][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);

        map[2][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY).SetGrowable(true);
        map[2][1].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::NORMAL).SetGrowable(true);
        map[2][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);

        return map;
    }

    static vector<vector<Grid>> Map5()
    {
        auto map = InitializeMapTemplate();
        map[0][0].SetType(GridType::WATER);
        map[0][2].SetType(GridType::WATER);
        map[2][0].SetType(GridType::WATER);
        map[2][2].SetType(GridType::WATER);

        map[0][0].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::NORMAL);
        map[0][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);
        map[0][2].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::NORMAL, Wall::EMPTY);

        map[1][0].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[1][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[1][2].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);

        map[2][0].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[2][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);
        map[2][2].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);

        return map;
    }

    static vector<vector<Grid>> Map6()
    {
        auto map = InitializeMapTemplate();
        map[0][0].SetType(GridType::PORTAL).SetPortal(2, 2);
        map[2][2].SetType(GridType::PORTAL).SetPortal(-2, -2);
        map[1][1].SetType(GridType::GRASS);

        map[0][0].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[0][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY).SetGrowable(true);
        map[0][2].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);

        map[1][0].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::NORMAL);
        map[1][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);
        map[1][2].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);

        map[2][0].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);
        map[2][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL).SetGrowable(true);
        map[2][2].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::NORMAL, Wall::EMPTY);

        return map;
    }

    static vector<vector<Grid>> Map7()
    {
        auto map = InitializeMapTemplate();
        map[1][1].SetType(GridType::GRASS);

        map[0][0].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL).SetGrowable(true);
        map[0][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY).SetGrowable(true);
        map[0][2].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::NORMAL).SetGrowable(true);

        map[1][0].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);
        map[1][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[1][2].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);

        map[2][0].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY).SetGrowable(true);
        map[2][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL).SetGrowable(true);
        map[2][2].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::NORMAL, Wall::EMPTY).SetGrowable(true);

        return map;
    }

    static vector<vector<Grid>> Map8()
    {
        auto map = InitializeMapTemplate();
        map[1][1].SetType(GridType::GRASS);

        map[0][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY).SetGrowable(true);
        map[0][1].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL).SetGrowable(true);
        map[0][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);

        map[1][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[1][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);
        map[1][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);

        map[2][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[2][1].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::NORMAL, Wall::EMPTY).SetGrowable(true);
        map[2][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL).SetGrowable(true);

        return map;
    }

    static vector<vector<Grid>> Map9()
    {
        auto map = InitializeMapTemplate();

        map[0][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[0][1].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::NORMAL, Wall::EMPTY);
        map[0][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);

        map[1][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[1][1].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[1][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        map[2][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);
        map[2][1].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::EMPTY, Wall::NORMAL);
        map[2][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);

        return map;
    }

    static vector<vector<Grid>> Map10()
    {
        auto map = InitializeMapTemplate();

        map[0][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);
        map[0][1].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::EMPTY, Wall::NORMAL);
        map[0][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);

        map[1][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[1][1].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[1][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        map[2][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[2][1].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::NORMAL, Wall::EMPTY);
        map[2][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);

        return map;
    }

    static vector<vector<Grid>> Map11()
    {
        auto map = InitializeMapTemplate();
        map[1][1].SetType(GridType::GRASS);

        map[0][0].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);
        map[0][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);
        map[0][2].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);

        map[1][0].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);
        map[1][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[1][2].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);

        map[2][0].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);
        map[2][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);
        map[2][2].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);

        return map;
    }

    static vector<vector<Grid>> Map12()
    {
        auto map = InitializeMapTemplate();
        map[1][1].SetType(GridType::GRASS);

        map[0][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL).SetGrowable(true);
        map[0][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL).SetGrowable(true);
        map[0][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL).SetGrowable(true);

        map[1][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);
        map[1][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[1][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);

        map[2][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL).SetGrowable(true);
        map[2][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL).SetGrowable(true);
        map[2][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL).SetGrowable(true);

        return map;
    }

    static vector<vector<Grid>> Map13()
    {
        auto map = InitializeMapTemplate();
        map[0][0].SetType(GridType::WATER);
        map[0][2].SetType(GridType::PORTAL).SetPortal(2, -2);
        map[2][0].SetType(GridType::PORTAL).SetPortal(-2, 2);
        map[2][2].SetType(GridType::WATER);

        map[0][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[0][1].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::NORMAL);
        map[0][2].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);

        map[1][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[1][1].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::NORMAL, Wall::NORMAL);
        map[1][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);

        map[2][0].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::NORMAL);
        map[2][1].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);
        map[2][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        return map;
    }

    static vector<vector<Grid>> Map14()
    {
        auto map = InitializeMapTemplate();
        map[0][0].SetType(GridType::PORTAL).SetPortal(2, 2);
        map[0][2].SetType(GridType::WATER);
        map[2][0].SetType(GridType::WATER);
        map[2][2].SetType(GridType::PORTAL).SetPortal(-2, -2);

        map[0][0].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[0][1].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::NORMAL, Wall::EMPTY);
        map[0][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        map[1][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[1][1].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::NORMAL, Wall::NORMAL);
        map[1][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);

        map[2][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[2][1].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[2][2].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::NORMAL, Wall::EMPTY);

        return map;
    }

    static vector<vector<Grid>> Map15()
    {
        auto map = InitializeMapTemplate();
        map[0][0].SetType(GridType::PORTAL).SetPortal(0, 2);
        map[0][2].SetType(GridType::PORTAL).SetPortal(0, -2);
        map[2][0].SetType(GridType::WATER);
        map[2][2].SetType(GridType::WATER);

        map[0][0].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[0][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);
        map[0][2].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);

        map[1][0].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::NORMAL);
        map[1][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);
        map[1][2].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::NORMAL, Wall::EMPTY);

        map[2][0].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[2][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[2][2].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        return map;
    }

    static vector<vector<Grid>> Map16()
    {
        auto map = InitializeMapTemplate();
        map[1][1].SetType(GridType::PORTAL).SetPortal(1, 0);
        map[2][1].SetType(GridType::PORTAL).SetPortal(-1, 0);

        map[0][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[0][1].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[0][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        map[1][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);
        map[1][1].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::NORMAL, Wall::NORMAL);
        map[1][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);

        map[2][0].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::NORMAL);
        map[2][1].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);
        map[2][2].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::NORMAL, Wall::EMPTY);

        return map;
    }

    static vector<vector<Grid>> Map17()
    {
        auto map = InitializeMapTemplate();
        map[0][0].SetType(GridType::WATER);
        map[0][2].SetType(GridType::WATER);
        map[1][1].SetType(GridType::GRASS);
        map[2][0].SetType(GridType::WATER);
        map[2][2].SetType(GridType::WATER);

        map[0][0].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[0][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL).SetGrowable(true);
        map[0][2].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);

        map[1][0].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);
        map[1][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[1][2].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);

        map[2][0].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::NORMAL);
        map[2][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY).SetGrowable(true);
        map[2][2].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);

        return map;
    }

    static vector<vector<Grid>> Map18()
    {
        auto map = InitializeMapTemplate();
        map[0][0].SetType(GridType::WATER);
        map[0][2].SetType(GridType::WATER);
        map[1][1].SetType(GridType::GRASS);
        map[2][0].SetType(GridType::WATER);
        map[2][2].SetType(GridType::WATER);

        map[0][0].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[0][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY).SetGrowable(true);
        map[0][2].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);

        map[1][0].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);
        map[1][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[1][2].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);

        map[2][0].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[2][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL).SetGrowable(true);
        map[2][2].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::NORMAL, Wall::EMPTY);

        return map;
    }

    static vector<vector<Grid>> Map19()
    {
        auto map = InitializeMapTemplate();

        map[0][0].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[0][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[0][2].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);

        map[1][0].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::NORMAL, Wall::EMPTY);
        map[1][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[1][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);

        map[2][0].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[2][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[2][2].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::NORMAL);

        return map;
    }

    static vector<vector<Grid>> Map20()
    {
        auto map = InitializeMapTemplate();

        map[0][0].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);
        map[0][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[0][2].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);

        map[1][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[1][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);
        map[1][2].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::EMPTY, Wall::NORMAL);

        map[2][0].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::NORMAL, Wall::EMPTY);
        map[2][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[2][2].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        return map;
    }

    static vector<vector<Grid>> Map21()
    {
        auto map = InitializeMapTemplate();

        map[0][0].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::NORMAL, Wall::EMPTY);
        map[0][1].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[0][2].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);

        map[1][0].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[1][1].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[1][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);

        map[2][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[2][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[2][2].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::NORMAL, Wall::EMPTY);

        return map;
    }

    static vector<vector<Grid>> Map22()
    {
        auto map = InitializeMapTemplate();

        map[0][0].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[0][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);
        map[0][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        map[1][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[1][1].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::NORMAL, Wall::EMPTY);
        map[1][2].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);

        map[2][0].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::NORMAL, Wall::EMPTY);
        map[2][1].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[2][2].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);

        return map;
    }

    static vector<vector<Grid>> Map23()
    {
        auto map = InitializeMapTemplate();
        map[1][1].SetType(GridType::TRAP);

        map[0][0].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::NORMAL);
        map[0][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY).SetGrowable(true);
        map[0][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);

        map[1][0].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);
        map[1][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[1][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);

        map[2][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);
        map[2][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);
        map[2][2].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::NORMAL).SetGrowable(true);

        return map;
    }

    static vector<vector<Grid>> Map24()
    {
        auto map = InitializeMapTemplate();
        map[1][1].SetType(GridType::TRAP);

        map[0][0].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY).SetGrowable(true);
        map[0][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);
        map[0][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);

        map[1][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);
        map[1][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[1][2].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);

        map[2][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);
        map[2][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL).SetGrowable(true);
        map[2][2].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);

        return map;
    }

    static vector<vector<Grid>> Map25()
    {
        auto map = InitializeMapTemplate();
        map[1][1].SetType(GridType::HEAT);

        return map;
    }

    static vector<vector<Grid>> Map26()
    {
        auto map = InitializeMapTemplate();
        map[0][0].SetType(GridType::HEAT);
        map[2][2].SetType(GridType::HEAT);

        return map;
    }

    static vector<vector<Grid>> Map27()
    {
        auto map = InitializeMapTemplate();
        map[1][1].SetAttach(AttachType::BOX);

        return map;
    }

    static vector<vector<Grid>> Map28()
    {
        auto map = InitializeMapTemplate();
        map[0][0].SetAttach(AttachType::BOX);
        map[2][2].SetAttach(AttachType::BOX);

        return map;
    }

    static vector<vector<Grid>> Map29()
    {
        auto map = InitializeMapTemplate();
        map[0][0].SetType(GridType::WATER);
        map[1][1].SetType(GridType::WATER);
        map[2][2].SetType(GridType::WATER);

        map[0][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[0][1].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::NORMAL);
        map[0][2].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::NORMAL, Wall::EMPTY);

        map[1][0].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[1][1].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[1][2].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        map[2][0].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[2][1].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);
        map[2][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        return map;
    }

    static vector<vector<Grid>> Map30()
    {
        auto map = InitializeMapTemplate();
        map[0][2].SetType(GridType::WATER);
        map[1][1].SetType(GridType::WATER);
        map[2][0].SetType(GridType::WATER);

        map[0][0].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::NORMAL);
        map[0][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);
        map[0][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        map[1][0].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[1][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);
        map[1][2].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::NORMAL, Wall::EMPTY);

        map[2][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[2][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[2][2].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);

        return map;
    }

    static vector<vector<Grid>> Map31()
    {
        auto map = InitializeMapTemplate();
        map[1][1].SetType(GridType::TRAP);

        map[0][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);
        map[0][1].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);
        map[0][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);

        map[1][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);
        map[1][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);
        map[1][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);

        map[2][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);
        map[2][1].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);
        map[2][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);

        return map;
    }

    static vector<vector<Grid>> Map32()
    {
        auto map = InitializeMapTemplate();
        map[1][0].SetType(GridType::PORTAL).SetPortal(0, 2);
        map[1][2].SetType(GridType::PORTAL).SetPortal(0, -2);
        map[1][1].SetType(GridType::TRAP);

        map[0][0].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);
        map[0][1].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);
        map[0][2].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);

        map[1][0].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::EMPTY, Wall::NORMAL);
        map[1][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);
        map[1][2].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::NORMAL, Wall::EMPTY);

        map[2][0].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);
        map[2][1].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);
        map[2][2].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);

        return map;
    }

    static vector<vector<Grid>> Map33()
    {
        auto map = InitializeMapTemplate();
        map[0][2].SetAttach(AttachType::BUTTON).SetButton({{0, -2, Direct::DOWN}}).SetContent("A");
        map[2][0].SetAttach(AttachType::BUTTON).SetButton({{0, 2, Direct::UP}}).SetContent("B");

        map[0][0].SetWall(Wall::EMPTY, Wall::DOOR, Wall::EMPTY, Wall::EMPTY).SetWallContent({"", "A", "", ""});
        map[0][1].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[0][2].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);

        map[1][0].SetWall(Wall::DOOR, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[1][1].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[1][2].SetWall(Wall::NORMAL, Wall::DOOR, Wall::EMPTY, Wall::EMPTY);

        map[2][0].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[2][1].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[2][2].SetWall(Wall::DOOR, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetWallContent({"B", "", "", ""});

        return map;
    }

    static vector<vector<Grid>> Map34()
    {
        auto map = InitializeMapTemplate();
        map[0][1].SetAttach(AttachType::BUTTON).SetButton({{0, 0, Direct::DOWN}}).SetContent("A");
        map[1][0].SetAttach(AttachType::BUTTON).SetButton({{0, 0, Direct::RIGHT}}).SetContent("D");
        map[1][2].SetAttach(AttachType::BUTTON).SetButton({{0, 0, Direct::LEFT}}).SetContent("B");
        map[2][1].SetAttach(AttachType::BUTTON).SetButton({{0, 0, Direct::UP}}).SetContent("C");

        map[0][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[0][1].SetWall(Wall::NORMAL, Wall::DOOR, Wall::EMPTY, Wall::EMPTY);
        map[0][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        map[1][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::DOOR);
        map[1][1].SetWall(Wall::DOOR, Wall::DOOR, Wall::DOOR, Wall::DOOR).SetWallContent({"A", "C", "D", "B"});
        map[1][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::DOOR, Wall::NORMAL);

        map[2][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[2][1].SetWall(Wall::DOOR, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[2][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        return map;
    }

    static vector<vector<Grid>> Map35()
    {
        auto map = InitializeMapTemplate();
        map[0][2].SetAttach(AttachType::BUTTON).SetButton({{0, -1, Direct::DOWN}, {1, -1, Direct::DOWN}}).SetContent("A");
        map[2][0].SetAttach(AttachType::BUTTON).SetButton({{-1, 0, Direct::RIGHT}, {-1, 1, Direct::RIGHT}}).SetContent("B");

        map[0][0].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[0][1].SetWall(Wall::EMPTY, Wall::DOOR, Wall::EMPTY, Wall::EMPTY);
        map[0][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        map[1][0].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::DOOR);
        map[1][1].SetWall(Wall::DOOR, Wall::DOOR, Wall::DOOR, Wall::DOOR).SetWallContent({"A", "A", "B", "B"});
        map[1][2].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::DOOR, Wall::EMPTY);

        map[2][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[2][1].SetWall(Wall::DOOR, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[2][2].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        return map;
    }

    static vector<vector<Grid>> Map36()
    {
        auto map = InitializeMapTemplate();
        map[0][2].SetAttach(AttachType::BUTTON).SetButton({{0, -1, Direct::DOWN}, {1, 0, Direct::LEFT}}).SetContent("A");
        map[2][0].SetAttach(AttachType::BUTTON).SetButton({{-1, 0, Direct::RIGHT}, {0, 1, Direct::UP}}).SetContent("B");

        map[0][0].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[0][1].SetWall(Wall::EMPTY, Wall::DOOR, Wall::EMPTY, Wall::EMPTY);
        map[0][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        map[1][0].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::DOOROPEN);
        map[1][1].SetWall(Wall::DOOR, Wall::DOOR, Wall::DOOROPEN, Wall::DOOROPEN).SetWallContent({"A", "B", "B", "A"});
        map[1][2].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::DOOROPEN, Wall::EMPTY);

        map[2][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[2][1].SetWall(Wall::DOOR, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[2][2].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        return map;
    }

    static vector<vector<Grid>> Map37()
    {
        auto map = InitializeMapTemplate();
        map[1][1].SetType(GridType::GRASS).SetAttach(AttachType::BUTTON).SetButton({
            {-1,  0, Direct::UP},
            { 0, -1, Direct::UP}, { 0, -1, Direct::DOWN},
            { 0,  1, Direct::UP}, { 0,  1, Direct::DOWN},
            { 1,  0, Direct::DOWN},
        });

        map[0][0].SetWall(Wall::EMPTY, Wall::DOOROPEN, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);
        map[0][1].SetWall(Wall::DOOR, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[0][2].SetWall(Wall::EMPTY, Wall::DOOR, Wall::EMPTY, Wall::EMPTY);

        map[1][0].SetWall(Wall::DOOROPEN, Wall::DOOR, Wall::NORMAL, Wall::EMPTY).SetGrowable(true);
        map[1][1].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[1][2].SetWall(Wall::DOOR, Wall::DOOROPEN, Wall::EMPTY, Wall::NORMAL).SetGrowable(true);

        map[2][0].SetWall(Wall::DOOR, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[2][1].SetWall(Wall::NORMAL, Wall::DOOROPEN, Wall::EMPTY, Wall::EMPTY);
        map[2][2].SetWall(Wall::DOOROPEN, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);

        return map;
    }

    static vector<vector<Grid>> Map38()
    {
        auto map = InitializeMapTemplate();
        map[0][0].SetType(GridType::WATER);
        map[0][2].SetType(GridType::WATER);
        map[1][1].SetType(GridType::GRASS).SetAttach(AttachType::BUTTON).SetButton({
            {-1, -1, Direct::DOWN}, {-1, -1, Direct::RIGHT},
            {-1,  1, Direct::DOWN}, {-1,  1, Direct::LEFT},
            { 1, -1, Direct::UP}, { 1, -1, Direct::RIGHT},
            { 1,  1, Direct::UP}, { 1,  1, Direct::LEFT},
        });
        map[2][0].SetType(GridType::WATER);
        map[2][2].SetType(GridType::WATER);

        map[0][0].SetWall(Wall::NORMAL, Wall::DOOR, Wall::EMPTY, Wall::DOOROPEN);
        map[0][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::DOOROPEN, Wall::DOOR).SetGrowable(true);
        map[0][2].SetWall(Wall::NORMAL, Wall::DOOROPEN, Wall::DOOR, Wall::EMPTY);

        map[1][0].SetWall(Wall::DOOR, Wall::DOOROPEN, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);
        map[1][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[1][2].SetWall(Wall::DOOROPEN, Wall::DOOR, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);

        map[2][0].SetWall(Wall::DOOROPEN, Wall::NORMAL, Wall::EMPTY, Wall::DOOR);
        map[2][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::DOOR, Wall::DOOROPEN).SetGrowable(true);
        map[2][2].SetWall(Wall::DOOR, Wall::NORMAL, Wall::DOOROPEN, Wall::EMPTY);

        return map;
    }

    static vector<vector<Grid>> Map39()
    {
        auto map = InitializeMapTemplate();
        map[1][1].SetType(GridType::TRAP).SetAttach(AttachType::BUTTON).SetButton({
            {-1,  1, Direct::DOWN}, {-1,  1, Direct::LEFT},
            { 0,  0, Direct::UP}, { 0,  0, Direct::DOWN}, { 0,  0, Direct::LEFT}, { 0,  0, Direct::RIGHT},
            { 1, -1, Direct::UP}, { 1, -1, Direct::RIGHT},
        });

        map[0][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[0][1].SetWall(Wall::EMPTY, Wall::DOOR, Wall::EMPTY, Wall::DOOROPEN);
        map[0][2].SetWall(Wall::NORMAL, Wall::DOOR, Wall::DOOROPEN, Wall::NORMAL);

        map[1][0].SetWall(Wall::EMPTY, Wall::DOOROPEN, Wall::EMPTY, Wall::DOOR);
        map[1][1].SetWall(Wall::DOOR, Wall::DOOROPEN, Wall::DOOR, Wall::DOOROPEN);
        map[1][2].SetWall(Wall::DOOR, Wall::EMPTY, Wall::DOOROPEN, Wall::EMPTY).SetGrowable(true);

        map[2][0].SetWall(Wall::DOOROPEN, Wall::NORMAL, Wall::NORMAL, Wall::DOOR);
        map[2][1].SetWall(Wall::DOOROPEN, Wall::EMPTY, Wall::DOOR, Wall::EMPTY).SetGrowable(true);
        map[2][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);

        return map;
    }

    static vector<vector<Grid>> Map40()
    {
        auto map = InitializeMapTemplate();
        map[1][1].SetType(GridType::TRAP).SetAttach(AttachType::BUTTON).SetButton({
            {-1,  1, Direct::DOWN}, {-1,  1, Direct::LEFT},
            { 0,  0, Direct::UP}, { 0,  0, Direct::DOWN}, { 0,  0, Direct::LEFT}, { 0,  0, Direct::RIGHT},
            { 1, -1, Direct::UP}, { 1, -1, Direct::RIGHT},
        });

        map[0][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);
        map[0][1].SetWall(Wall::EMPTY, Wall::DOOROPEN, Wall::EMPTY, Wall::DOOR).SetGrowable(true);
        map[0][2].SetWall(Wall::NORMAL, Wall::DOOR, Wall::DOOR, Wall::NORMAL);

        map[1][0].SetWall(Wall::EMPTY, Wall::DOOROPEN, Wall::EMPTY, Wall::DOOR);
        map[1][1].SetWall(Wall::DOOROPEN, Wall::DOOR, Wall::DOOR, Wall::DOOROPEN);
        map[1][2].SetWall(Wall::DOOR, Wall::EMPTY, Wall::DOOROPEN, Wall::EMPTY).SetGrowable(true);

        map[2][0].SetWall(Wall::DOOROPEN, Wall::NORMAL, Wall::NORMAL, Wall::DOOROPEN);
        map[2][1].SetWall(Wall::DOOR, Wall::EMPTY, Wall::DOOROPEN, Wall::EMPTY);
        map[2][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);

        return map;
    }

    static vector<vector<Grid>> Map41()
    {
        auto map = InitializeMapTemplate();
        map[1][1].SetType(GridType::GRASS).SetAttach(AttachType::BUTTON).SetButton({
            {-1, -1, Direct::UP}, {-1, -1, Direct::DOWN}, {-1, -1, Direct::LEFT}, {-1, -1, Direct::RIGHT},
            {-1,  1, Direct::UP}, {-1,  1, Direct::DOWN}, {-1,  1, Direct::LEFT}, {-1,  1, Direct::RIGHT},
            { 1, -1, Direct::UP}, { 1, -1, Direct::DOWN}, { 1, -1, Direct::LEFT}, { 1, -1, Direct::RIGHT},
            { 1,  1, Direct::UP}, { 1,  1, Direct::DOWN}, { 1,  1, Direct::LEFT}, { 1,  1, Direct::RIGHT},
        });

        map[0][0].SetWall(Wall::DOOROPEN, Wall::DOOR, Wall::DOOR, Wall::DOOROPEN).SetGrowable(true);
        map[0][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::DOOROPEN, Wall::DOOR).SetGrowable(true);
        map[0][2].SetWall(Wall::DOOR, Wall::DOOROPEN, Wall::DOOR, Wall::DOOROPEN).SetGrowable(true);

        map[1][0].SetWall(Wall::DOOR, Wall::DOOROPEN, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);
        map[1][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[1][2].SetWall(Wall::DOOROPEN, Wall::DOOR, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);

        map[2][0].SetWall(Wall::DOOROPEN, Wall::DOOR, Wall::DOOROPEN, Wall::DOOR).SetGrowable(true);
        map[2][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::DOOR, Wall::DOOROPEN).SetGrowable(true);
        map[2][2].SetWall(Wall::DOOR, Wall::DOOROPEN, Wall::DOOROPEN, Wall::DOOR).SetGrowable(true);

        return map;
    }

    static vector<vector<Grid>> Map42()
    {
        auto map = InitializeMapTemplate();
        map[1][1].SetType(GridType::GRASS).SetAttach(AttachType::BUTTON).SetButton({
            {-1, -1, Direct::UP}, {-1, -1, Direct::DOWN}, {-1, -1, Direct::LEFT}, {-1, -1, Direct::RIGHT},
            {-1,  1, Direct::UP}, {-1,  1, Direct::DOWN}, {-1,  1, Direct::LEFT}, {-1,  1, Direct::RIGHT},
            { 1, -1, Direct::UP}, { 1, -1, Direct::DOWN}, { 1, -1, Direct::LEFT}, { 1, -1, Direct::RIGHT},
            { 1,  1, Direct::UP}, { 1,  1, Direct::DOWN}, { 1,  1, Direct::LEFT}, { 1,  1, Direct::RIGHT},
        });

        map[0][0].SetWall(Wall::DOOR, Wall::DOOR, Wall::DOOROPEN, Wall::DOOROPEN).SetGrowable(true);
        map[0][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::DOOROPEN, Wall::DOOROPEN).SetGrowable(true);
        map[0][2].SetWall(Wall::DOOR, Wall::DOOR, Wall::DOOROPEN, Wall::DOOROPEN).SetGrowable(true);

        map[1][0].SetWall(Wall::DOOR, Wall::DOOR, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);
        map[1][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[1][2].SetWall(Wall::DOOR, Wall::DOOR, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);

        map[2][0].SetWall(Wall::DOOR, Wall::DOOR, Wall::DOOROPEN, Wall::DOOROPEN).SetGrowable(true);
        map[2][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::DOOROPEN, Wall::DOOROPEN).SetGrowable(true);
        map[2][2].SetWall(Wall::DOOR, Wall::DOOR, Wall::DOOROPEN, Wall::DOOROPEN).SetGrowable(true);

        return map;
    }

    static vector<vector<Grid>> Map43()
    {
        auto map = InitializeMapTemplate();
        map[1][1].SetAttach(AttachType::HEATBOX);

        return map;
    }

    static vector<vector<Grid>> Map44()
    {
        auto map = InitializeMapTemplate();
        map[1][1].SetAttach(AttachType::JAMMERBOX);

        return map;
    }

    // static vector<vector<Grid>> Map45()
    // {
    //     auto map = InitializeMapTemplate();
    //     map[0][0].SetType(GridType::WATER);
    //     map[0][2].SetType(GridType::PORTAL).SetPortal(2, -2);
    //     map[2][0].SetType(GridType::PORTAL).SetPortal(-2, 2);
    //     map[2][2].SetType(GridType::WATER);

    //     map[0][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
    //     map[0][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
    //     map[0][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

    //     map[1][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
    //     map[1][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
    //     map[1][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

    //     map[2][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
    //     map[2][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
    //     map[2][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

    //     return map;
    // }

    // 旧版31、32（暂时废弃）
    // static vector<vector<Grid>> Map31()
    // {
    //     auto map = InitializeMapTemplate();
    //     map[0][2].SetType(GridType::ONEWAYPORTAL).SetPortal(2, -2);
    //     map[2][0].SetType(GridType::PORTAL).SetPortal(-2, 2);
    //     map[1][1].SetType(GridType::GRASS);

    //     map[0][0].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);
    //     map[0][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL).SetGrowable(true);
    //     map[0][2].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);

    //     map[1][0].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
    //     map[1][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);
    //     map[1][2].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::NORMAL, Wall::EMPTY);

    //     map[2][0].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::NORMAL);
    //     map[2][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY).SetGrowable(true);
    //     map[2][2].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);

    //     return map;
    // }

    // static vector<vector<Grid>> Map32()
    // {
    //     auto map = InitializeMapTemplate();
    //     map[0][0].SetType(GridType::ONEWAYPORTAL).SetPortal(2, 2);
    //     map[2][2].SetType(GridType::PORTAL).SetPortal(-2, -2);
    //     map[1][1].SetType(GridType::GRASS);

    //     map[0][0].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
    //     map[0][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY).SetGrowable(true);
    //     map[0][2].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);

    //     map[1][0].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::NORMAL);
    //     map[1][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);
    //     map[1][2].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);

    //     map[2][0].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);
    //     map[2][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL).SetGrowable(true);
    //     map[2][2].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::NORMAL, Wall::EMPTY);

    //     return map;
    // }

    /* ========== 逃生舱 ========== */
    static vector<vector<Grid>> Exit1()
    {
        auto map = InitializeMapTemplate();
        map[1][1].SetType(GridType::EXIT);

        map[0][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[0][1].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[0][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);

        map[1][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);
        map[1][1].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::NORMAL, Wall::NORMAL);
        map[1][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);

        map[2][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[2][1].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[2][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        return map;
    }

    static vector<vector<Grid>> Exit2()
    {
        auto map = InitializeMapTemplate();
        map[1][1].SetType(GridType::EXIT);

        map[0][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[0][1].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);
        map[0][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        map[1][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[1][1].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::NORMAL, Wall::NORMAL);
        map[1][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);

        map[2][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[2][1].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[2][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        return map;
    }

    static vector<vector<Grid>> Exit3()
    {
        auto map = InitializeMapTemplate();
        map[1][1].SetType(GridType::EXIT);

        map[0][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[0][1].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[0][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        map[1][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);
        map[1][1].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);
        map[1][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);

        map[2][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[2][1].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::NORMAL);
        map[2][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);

        return map;
    }

    static vector<vector<Grid>> Exit4()
    {
        auto map = InitializeMapTemplate();
        map[1][1].SetType(GridType::EXIT);

        map[0][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[0][1].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[0][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        map[1][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[1][1].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);
        map[1][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);

        map[2][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[2][1].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::NORMAL, Wall::EMPTY);
        map[2][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        return map;
    }

    static vector<vector<Grid>> Exit5()
    {
        auto map = InitializeMapTemplate();
        map[0][0].SetType(GridType::PORTAL).SetPortal(2, 2);
        map[0][2].SetType(GridType::WATER);
        map[1][1].SetType(GridType::EXIT);
        map[2][0].SetType(GridType::WATER);
        map[2][2].SetType(GridType::PORTAL).SetPortal(-2, -2);

        map[0][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[0][1].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[0][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);

        map[1][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);
        map[1][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);
        map[1][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);

        map[2][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[2][1].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::NORMAL, Wall::EMPTY);
        map[2][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        return map;
    }

    static vector<vector<Grid>> Exit6()
    {
        auto map = InitializeMapTemplate();
        map[0][0].SetType(GridType::WATER);
        map[0][2].SetType(GridType::PORTAL).SetPortal(2, -2);
        map[1][1].SetType(GridType::EXIT);
        map[2][0].SetType(GridType::PORTAL).SetPortal(-2, 2);
        map[2][2].SetType(GridType::WATER);

        map[0][0].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[0][1].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[0][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        map[1][0].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);
        map[1][1].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[1][2].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::NORMAL);

        map[2][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[2][1].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[2][2].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        return map;
    }

    static vector<vector<Grid>> Exit7()
    {
        auto map = InitializeMapTemplate();
        map[0][0].SetType(GridType::WATER);
        map[0][2].SetType(GridType::PORTAL).SetPortal(2, -2);
        map[1][1].SetType(GridType::EXIT);
        map[2][0].SetType(GridType::PORTAL).SetPortal(-2, 2);
        map[2][2].SetType(GridType::WATER);

        map[0][0].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::NORMAL);
        map[0][1].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);
        map[0][2].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);

        map[1][0].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[1][1].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::NORMAL, Wall::NORMAL);
        map[1][2].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);

        map[2][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[2][1].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[2][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        return map;
    }

    static vector<vector<Grid>> Exit8()
    {
        auto map = InitializeMapTemplate();
        map[0][0].SetType(GridType::WATER);
        map[0][2].SetType(GridType::PORTAL).SetPortal(2, -2);
        map[1][1].SetType(GridType::EXIT);
        map[2][0].SetType(GridType::PORTAL).SetPortal(-2, 2);
        map[2][2].SetType(GridType::WATER);

        map[0][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[0][1].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[0][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        map[1][0].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::NORMAL);
        map[1][1].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);
        map[1][2].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::NORMAL, Wall::EMPTY);

        map[2][0].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[2][1].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::NORMAL);
        map[2][2].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);

        return map;
    }

    static vector<vector<Grid>> Exit9()
    {
        auto map = InitializeMapTemplate();
        map[0][0].SetType(GridType::PORTAL).SetPortal(2, 2);
        map[0][1].SetType(GridType::EXIT);
        map[1][1].SetType(GridType::TRAP);
        map[2][2].SetType(GridType::PORTAL).SetPortal(-2, -2);

        map[0][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[0][1].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);
        map[0][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY).SetGrowable(true);

        map[1][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);
        map[1][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[1][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);

        map[2][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL).SetGrowable(true);
        map[2][1].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::NORMAL, Wall::EMPTY).SetGrowable(true);
        map[2][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        return map;
    }

    static vector<vector<Grid>> Exit10()
    {
        auto map = InitializeMapTemplate();
        map[0][0].SetType(GridType::PORTAL).SetPortal(2, 2);
        map[1][1].SetType(GridType::TRAP);
        map[2][1].SetType(GridType::EXIT);
        map[2][2].SetType(GridType::PORTAL).SetPortal(-2, -2);

        map[0][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[0][1].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL).SetGrowable(true);
        map[0][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY).SetGrowable(true);

        map[1][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);
        map[1][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[1][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);

        map[2][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL).SetGrowable(true);
        map[2][1].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::NORMAL, Wall::NORMAL);
        map[2][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);

        return map;
    }

    static vector<vector<Grid>> Exit11()
    {
        auto map = InitializeMapTemplate();
        map[1][1].SetType(GridType::EXIT);
        map[1][2].SetAttach(AttachType::BUTTON).SetButton({{1, -1, Direct::LEFT}});

        map[0][0].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[0][1].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[0][2].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);

        map[1][0].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[1][1].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);
        map[1][2].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);

        map[2][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::DOOR);
        map[2][1].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::DOOR, Wall::NORMAL);
        map[2][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);

        return map;
    }

    static vector<vector<Grid>> Exit12()
    {
        auto map = InitializeMapTemplate();
        map[1][1].SetType(GridType::EXIT);
        map[1][0].SetAttach(AttachType::BUTTON).SetButton({{-1, 1, Direct::RIGHT}});

        map[0][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[0][1].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::NORMAL, Wall::DOOR);
        map[0][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::DOOR, Wall::EMPTY);

        map[1][0].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::NORMAL);
        map[1][1].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::NORMAL, Wall::NORMAL);
        map[1][2].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::NORMAL, Wall::EMPTY);

        map[2][0].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[2][1].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[2][2].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        return map;
    }

    static vector<vector<Grid>> Exit13()
    {
        auto map = InitializeMapTemplate();
        map[0][0].SetType(GridType::PORTAL).SetPortal(2, 2);
        map[0][2].SetAttach(AttachType::BUTTON).SetButton({{1, -1, Direct::DOWN}}).SetContent("A");
        map[1][1].SetType(GridType::EXIT).SetAttach(AttachType::JAMMERBOX);
        map[2][0].SetAttach(AttachType::BUTTON).SetButton({{-1, 1, Direct::UP}}).SetContent("B");
        map[2][2].SetType(GridType::PORTAL).SetPortal(-2, -2);

        map[0][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[0][1].SetWall(Wall::EMPTY, Wall::DOOR, Wall::EMPTY, Wall::NORMAL);
        map[0][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);

        map[1][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);
        map[1][1].SetWall(Wall::DOOR, Wall::DOOR, Wall::NORMAL, Wall::NORMAL).SetWallContent({"B", "A", "", ""});
        map[1][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);

        map[2][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[2][1].SetWall(Wall::DOOR, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);
        map[2][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        return map;
    }

    static vector<vector<Grid>> Exit14()
    {
        auto map = InitializeMapTemplate();
        map[0][1].SetAttach(AttachType::BOX);
        map[1][1].SetType(GridType::EXIT).SetAttach(AttachType::BOX);
        map[2][1].SetAttach(AttachType::BOX);

        map[0][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[0][1].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[0][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        map[1][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);
        map[1][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);
        map[1][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);

        map[2][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[2][1].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[2][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        return map;
    }

    /* ========== 特殊地图 ========== */
    static vector<vector<Grid>> MapS1()
    {
        auto map = InitializeMapTemplate();
        map[0][0].SetType(GridType::WATER);
        map[0][1].SetType(GridType::TRAP);
        map[0][2].SetType(GridType::PORTAL).SetPortal(2, -2);
        map[1][0].SetType(GridType::GRASS);
        map[1][1].SetType(GridType::HEAT);
        map[1][2].SetType(GridType::GRASS);
        map[2][0].SetType(GridType::PORTAL).SetPortal(-2, 2);
        map[2][1].SetType(GridType::TRAP);
        map[2][2].SetType(GridType::WATER);

        map[0][0].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[0][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[0][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);

        map[1][0].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[1][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[1][2].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);

        map[2][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[2][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);
        map[2][2].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        return map;
    }

    static vector<vector<Grid>> MapS2()
    {
        auto map = InitializeMapTemplate();
        map[0][0].SetType(GridType::PORTAL).SetPortal(2, 2).SetContent("A");
        map[0][2].SetType(GridType::PORTAL).SetPortal(2, -2).SetContent("B");
        map[1][1].SetType(GridType::WATER);
        map[2][0].SetType(GridType::PORTAL).SetPortal(-2, 2).SetContent("B");
        map[2][2].SetType(GridType::PORTAL).SetPortal(-2, -2).SetContent("A");

        map[0][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[0][1].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[0][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        map[1][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);
        map[1][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[1][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);

        map[2][0].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[2][1].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[2][2].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        return map;
    }

    static vector<vector<Grid>> MapS3()
    {
        auto map = InitializeMapTemplate();
        map[0][0].SetType(GridType::PORTAL).SetPortal(2, 2).SetContent("A");
        map[0][2].SetType(GridType::PORTAL).SetPortal(2, -2).SetContent("B");
        map[1][1].SetType(GridType::TRAP);
        map[2][0].SetType(GridType::PORTAL).SetPortal(-2, 2).SetContent("B");
        map[2][2].SetType(GridType::PORTAL).SetPortal(-2, -2).SetContent("A");

        map[0][0].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[0][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL).SetGrowable(true);
        map[0][2].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);

        map[1][0].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);
        map[1][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[1][2].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY).SetGrowable(true);

        map[2][0].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::NORMAL);
        map[2][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY).SetGrowable(true);
        map[2][2].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);

        return map;
    }

    static vector<vector<Grid>> MapS4()
    {
        auto map = InitializeMapTemplate();
        map[0][0].SetType(GridType::PORTAL).SetPortal(2, 2).SetContent("A");
        map[0][1].SetType(GridType::PORTAL).SetPortal(2, 0).SetContent("B");
        map[0][2].SetType(GridType::PORTAL).SetPortal(2, -2).SetContent("C");
        map[1][0].SetType(GridType::PORTAL).SetPortal(0, 2).SetContent("D");
        map[1][1].SetType(GridType::PORTAL).SetPortal(0, 0).SetContent("E(E)");
        map[1][2].SetType(GridType::PORTAL).SetPortal(0, -2).SetContent("D");
        map[2][0].SetType(GridType::PORTAL).SetPortal(-2, 2).SetContent("C");
        map[2][1].SetType(GridType::PORTAL).SetPortal(-2, 0).SetContent("B");
        map[2][2].SetType(GridType::PORTAL).SetPortal(-2, -2).SetContent("A");

        map[0][0].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::EMPTY, Wall::NORMAL);
        map[0][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);
        map[0][2].SetWall(Wall::EMPTY, Wall::NORMAL, Wall::NORMAL, Wall::EMPTY);

        map[1][0].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);
        map[1][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY);
        map[1][2].SetWall(Wall::NORMAL, Wall::NORMAL, Wall::EMPTY, Wall::EMPTY);

        map[2][0].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::EMPTY, Wall::NORMAL);
        map[2][1].SetWall(Wall::EMPTY, Wall::EMPTY, Wall::NORMAL, Wall::NORMAL);
        map[2][2].SetWall(Wall::NORMAL, Wall::EMPTY, Wall::NORMAL, Wall::EMPTY);

        return map;
    }
};
