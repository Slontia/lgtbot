
class Score
{
  public:
    Score(const int size)
    {
        explore_map.resize(size);
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                explore_map[i].push_back(0);
            }
        }
    }
    // 抓人分
    int catch_score = 0;
    // 逃生分
    static constexpr const int exit_order[8] = {150, 200, 250, 250, 250, 250, 250, 250};
    int exit_score = 0;
    // 探索分   
    vector<vector<int>> explore_map;
    // 退出惩罚
    int quit_score = 0;

    pair<int, int> ExploreCount() const
    {
        int count1 = 0, count2 = 0;
        for (const auto &row : explore_map) {
            count1 += count(row.begin(), row.end(), 1);
            count2 += count(row.begin(), row.end(), 2);
        }
        return make_pair(count1, count2);
    }

    static string ScoreInfo() { return score_rule; }

    int FinalScore() const { return catch_score + exit_score + ExploreScore() + quit_score; }

  private:
    int ExploreScore() const
    {
        auto [c1, c2] = ExploreCount();
        return c2 * 2 + c1 * 1;
    }

    static constexpr const char* score_rule = R"(
　【抓人分】<br>
抓人+100，被抓-100<br>
　【逃生分】<br>
第1/2/3/4个逃生+150/200/250/250<br>
　【探索分】<br>
每探索一个自己未探索的格子+1<br>
每探索一个所有玩家未探索的格子额外+1<br>
【退出惩罚】强制退出-300分<br>)";
};


class PlayerAchievement
{
  public:
    // 【悄无声息】
    bool exit_without_sound = false;
    // 【环游世界】
    bool explore_all_map = false;
    // 【游荡幽灵】
    bool explore_all_map_silently() const { return explore_all_map && !trigger_sound; }
    // 【我赶时间】
    bool exit_first_round = false;
    // 【饥渴难耐】
    bool catch_first_round = false;
    // 【乒铃乓啷】
    bool visit_five_grid_type() const { return uniqueTotalCount() >= 5; }
    // 【嗜杀成性】
    bool catch_everyone_4p = false;
    // 【守株待兔】
    bool catch_without_moving = false;
    // 【牛头魅魔】
    bool boss_chase_four_steps = false;

    PlayerAchievement(const vector<pair<int, int>>& pos): blocks(std::move(pos))
    {
        InitializeBlockChecker();
    }

    // [悄无声息]辅助：触发声音
    bool trigger_sound = false;

    // [嗜杀成性]辅助
    void recordCatch(const int playerNum) {
        if (++catch_count == playerNum - 1 && playerNum >= 4) {
            catch_everyone_4p = true;   // 成就【嗜杀成性】
        }
    }

    // [乒铃乓啷]辅助
    void visitGrid(GridType g) {
        // 单向传送门视为普通传送门
        if (g == GridType::ONEWAYPORTAL) {
            grids.insert(GridType::PORTAL);
            return;
        }
        if (g != GridType::EMPTY) grids.insert(g);
    }
    void visitAttach(AttachType a) {
        if (a != AttachType::EMPTY) attaches.insert(a);
    }
    int uniqueGridCount() const { return grids.size(); }
    int uniqueAttachCount() const { return attaches.size(); }
    int uniqueTotalCount() const { return grids.size() + attaches.size(); }

    // [环游世界]辅助：初始化区块检查器
    void InitializeBlockChecker()
    {
        sort(blocks.begin(), blocks.end());
        total = blocks.size();
        visited.assign(total, false);
        count = 0;
    }

    // [环游世界]辅助：每步移动时检查走过的区块
    void MakeStep(int x, int y)
    {
        if (explore_all_map) return;

        for (int dx = 0; dx < 3; ++dx) {
            for (int dy = 0; dy < 3; ++dy) {
                pair<int,int> cand = {x - dx, y - dy};
                auto it = lower_bound(blocks.begin(), blocks.end(), cand);
                if (it != blocks.end() && *it == cand) {
                    int idx = distance(blocks.begin(), it);
                    if (!visited[idx] && ++count == total)
                        explore_all_map = true;     // 成就【环游世界】
                    visited[idx] = true;
                    return;
                }
            }
        }
    }
  private:
    // [乒铃乓啷]辅助
    template <typename E>
    struct EnumHash {
        std::size_t operator()(E e) const noexcept {
            return std::hash<typename std::underlying_type<E>::type>()(
                static_cast<typename std::underlying_type<E>::type>(e));
        }
    };
    std::unordered_set<GridType, EnumHash<GridType>> grids;
    std::unordered_set<AttachType, EnumHash<AttachType>> attaches;
    // [环游世界]辅助
    vector<pair<int,int>> blocks;
    vector<char> visited;
    int total = 0;
    int count = 0;
    // [嗜杀成性]辅助
    int catch_count = 0;
};


struct Move {
    int direct;
    Sound sound;
    vector<string> propagation; // 声音向其他所有玩家的传播方向（私信完整赛况）
    // string extra_pub_content;   // 移动时的额外公屏信息（暂不使用）
    pair<string, bool> extra_pri_content;   // 移动时的额外私信信息（私信完整赛况）
    pair<string, bool> content; // true（带移动方向）/ false（不带移动方向）
    string style;

    Move(int direct, Sound sound, pair<string, bool> content)
        : direct(direct), sound(sound), content(content) {}
    Move(Sound sound, pair<string, bool> content, string style)
        : direct(-1), sound(sound), content(content), style(style) {}
    Move(pair<string, bool> extra_pri_content, string style)
        : direct(-1), sound(Sound::NONE), extra_pri_content(extra_pri_content), style(style) {}

    void UpdateExtraPriContent(const string& content, const bool has_dir, const string& style)
    {
        this->extra_pri_content.first = content;
        this->extra_pri_content.second = has_dir;
        this->style = style;
    }
};

class RoundMove
{
  public:
    vector<Move> round_move;

    void push_back(const Move& mv) { round_move.push_back(mv); }
    bool empty() const { return round_move.empty(); }
    Move& back() { return round_move.back(); }
    void clear() { round_move.clear(); }
    size_t size() const { return round_move.size(); }
    Move& operator[](size_t idx) { return round_move[idx]; }

    string GetMoveRecord(const int query_pid, const bool is_public, const bool is_html = false) const
    {
        string result;

        if (is_html) result += MoveRecordStyle();

        const size_t N = round_move.size();
        size_t i = 0;

        while (i < N) {
            const Move& mv = round_move[i];

            bool mergeable = (mv.direct >= 0 && mv.sound == Sound::NONE && mv.content.first.empty());
            if (mergeable) {
                size_t j = i, count = 0;
                while (j < N) {
                    const Move& next = round_move[j];
                    // 忽略判断：非自己查看或公屏预览，合并方向时需跳过[无方向]的私信隐藏信息
                    if (!next.extra_pri_content.first.empty() && !next.extra_pri_content.second && (query_pid != -1 || is_public)) {
                        j++; continue;
                    }
                    // 计数判断：方向相同、无声响、非[无方向]content信息、无私信隐藏信息或私信隐藏信息[含有方向]
                    if (next.direct == mv.direct && next.sound == Sound::NONE && next.content.first.empty() &&
                        (next.extra_pri_content.first.empty() || next.extra_pri_content.second)) {
                        count++; j++;
                    } else break;   // 计数失败则达到可合并尽头
                }
                if (count >= 3) {
                    if (is_html) {
                        result += "<span class=\"move merge\">" + dirSymbol(mv.direct) + "*" + to_string(count) + "</span>";
                    } else {
                        result += dirSymbol(mv.direct) + "*" + to_string(count);
                    }
                    i = j;
                    continue;
                }
            }
            result += formatSingle(mv, query_pid, is_public, is_html);
            i++;
        }
        return result;
    }

  private:
    static string formatSingle(const Move& mv, const int query_pid, const bool is_public, const bool is_html)
    {
        string d = dirSymbol(mv.direct);

        // 查询pid非-1（非自己），获取私信完整赛况声响方向
        string sound_d;
        if (mv.sound != Sound::NONE && query_pid != -1 && !is_public) {
            if (query_pid < mv.propagation.size()) {
                sound_d = "(" + mv.propagation[query_pid] + ")";
            } else {
                sound_d = "{越界异常[pid=" + to_string(query_pid) + "]}";
            }
        }

        if (mv.sound == Sound::SHASHA) { // [沙沙]
            return is_html ? "<span class=\"move sound-grass\">" + d + SHASHA_STR + sound_d + "</span>" : "[" + d + SHASHA_STR + sound_d + "]";
        }
        else if (mv.sound == Sound::PAPA) { // [啪啪]
            return is_html ? "<span class=\"move sound-water\">" + d + PAPA_STR + sound_d + "</span>" : "[" + d + PAPA_STR + sound_d + "]";
        }
        else if (!mv.content.first.empty()) {
            if (is_html) {
                return mv.content.second
                    ? "<span class=\"move hit\">(" + d + mv.content.first + ")</span>"  // 撞墙
                    : "<span class=\"move " + mv.style + "\">" + mv.content.first + "</span>"; // 回合结束
            }
            return mv.content.second ? "(" + d + mv.content.first + ")" : mv.content.first;
        }
        else { // 普通移动
            if (!mv.extra_pri_content.first.empty()) {
                if (query_pid == -1 && !is_public) {
                    // 私信额外内容（仅查询自己-1）
                    if (is_html) {
                        return mv.extra_pri_content.second
                            ? "<span class=\"move " + mv.style + "\">" + d + mv.extra_pri_content.first + "</span>"  // 带方向信息
                            : "<span class=\"move " + mv.style + "\">" + mv.extra_pri_content.first + "</span>";   // 不带方向信息
                    }
                    return mv.content.second ? "[" + d + mv.extra_pri_content.first + "]" : "[" + mv.extra_pri_content.first + "]";
                } else {
                    string direct_str = is_html ? "<span class=\"move normal\">" + d + "</span>" : d;
                    return mv.content.second ? direct_str : "";
                }
            } else {
                return is_html ? "<span class=\"move normal\">" + d + "</span>" : d;
            }
        }
    }

    static string dirSymbol(int d)
    {
        switch (d) {
            case 0: return "↑";
            case 1: return "↓";
            case 2: return "←";
            case 3: return "→";
            default: return "";
        }
    }

    static string MoveRecordStyle();
};


class Player
{
  public:
    Player(const PlayerID pid, const string &name, const string &avatar, const int size, const vector<pair<int, int>>& pos)
        : pid(pid), name(name), avatar(avatar), score(size), achievement(pos) {}

    // 玩家信息
    const PlayerID pid;     // 玩家ID
    const string name;      // 玩家名字
    const string avatar;    // 玩家头像
    // 出局（1被抓 2出口）
    int out = 0;
    // 当前坐标
    int x, y;
    // 抓捕目标
    PlayerID target;
    // 移动相关
    RoundMove move_record;          // 当前回合完整记录
    vector<RoundMove> all_record;   // 历史回合完整记录
    int subspace = -1;          // 亚空间剩余步数
    string private_record;      // 当前回合私信记录
    int hide_remaining = 0;     // 隐匿剩余次数
    bool in_heat_zone = false;  // 在热浪区域内
    bool heated = false;        // 两次烫伤强制停止
    // 挂机状态（等待时间缩减）
    bool hook_status = false;
    // 加时卡
    int extra_time_card = EXTRATIMECRAD_COUNT;
    // 炸弹
    int bomb = 0;               // 剩余数量
    bool bomb_trigger = false;  // 炸弹触发状态

    // 玩家分数
    Score score;
    // 玩家成就
    PlayerAchievement achievement;    

    void NewStepRecord(const Direct direct, const string& end = "") { move_record.push_back({static_cast<int>(direct), Sound::NONE, {end, true}}); }
    void NewContentRecord(const string& content, const string& style = "end") { move_record.push_back({Sound::NONE, {content, false}, style}); }
    void ClearMoveRecord() { move_record.clear(); }

    void UpdateSoundRecord(const Sound sound) { if (!move_record.empty()) move_record.back().sound = sound; }
    void AddSoundPropagation(const string& direct_str) { if (!move_record.empty()) move_record.back().propagation.push_back(direct_str); }
    void UpdateEndRecord(const string& content) { if (!move_record.empty()) move_record.back().content = {content, true}; }

    void NewExtraPriContent(const string& content, const string& style) { if (!move_record.empty()) move_record.push_back({{content, false}, style}); }
    void UpdateExtraPriContent(const string& content, const string& style) { if (!move_record.empty()) move_record.back().UpdateExtraPriContent(content, true, style); }

    string GetAllMoveRecord(const int query_pid, const int is_public, const bool is_html = true) const
    {
        string record;
        for (size_t round = 0; round < all_record.size(); round++) {
            if (round > 0) record += is_html ? "<br>" : "\n";
            if (is_html) {
                record += "【第 " + to_string(round + 1) + " 回合】<br>";
            } else {
                record += "【第 " + to_string(round + 1) + " 回合】\n";
            }
            record += all_record[round].GetMoveRecord(query_pid == pid ? -1 : query_pid, is_public, is_html);  // -1代表是自己查询
        }
        return record;
    }

    bool InSubspace() const { return subspace > 0; }
};

string RoundMove::MoveRecordStyle()
{
    return
R"(<style>
.move {
    display: inline-block;
    white-space: nowrap;
    color: #000;
    padding: 0 5px;
    border-radius: 4px;
    margin-right: 2px;
    margin-bottom: 1px;
    font-family: inherit;
}
.normal {
    border:1px solid #aaa;
    background: #f2f2f2;
}
.merge {
    border:1px solid #888;
    background: #ededed;
    letter-spacing: -0.5px;
}
.sound-grass {
    border:1px solid #6fbf6f;
    background: repeating-linear-gradient(
        45deg,
        #c8f3c8,
        #c8f3c8 4px,
        #b6e8b6 4px,
        #b6e8b6 8px
    );
}
.sound-water {
    border: 1px solid #d0e8ff;
    background:
        repeating-linear-gradient(
            -45deg,
            rgba(255,255,255,0.15),
            rgba(255,255,255,0.15) 6px,
            transparent 6px,
            transparent 12px
        ),
        linear-gradient(
            135deg,
            #e0f0ff,
            #c8e1ff,
            #b3d8ff,
            #c0d8f8,
            #d0c8f8
        );
    background-size: 80px 80px, 100% 100%;
    backdrop-filter: blur(2px);
    box-shadow: 0 1px 2px rgba(179, 216, 255, 0.5);
}
.heat-wave {
    border: 1px solid #ff9a9a;
    background:
        radial-gradient(
            circle at 50% 50%,
            rgba(255,255,255,0.35) 0%,
            rgba(255,180,180,0.25) 40%,
            rgba(255,120,120,0.15) 70%,
            transparent 100%
        ),
        repeating-linear-gradient(
            45deg,
            #ffd6d6,
            #ffd6d6 6px,
            #ffc2c2 6px,
            #ffc2c2 12px
        );
    box-shadow: 0 0 4px rgba(255,120,120,0.6);
    filter: saturate(1.05);
}
.heat-core {
    border: 1px solid #d96b6b;
    background:
        radial-gradient(
            circle at 50% 50%,
            #ffb3b3 0%,
            #f07a7a 60%,
            #d85c5c 100%
        );
    box-shadow: 0 0 6px rgba(220,100,100,0.5);
}
.hit {
    border: 1px dashed #c44;
    background: #f7d6d6;
}
.end {
    border: 1px solid #c44;
    background: #f7d6d6;
}
.escape {
    border: 1px solid #e0b400;
    background: repeating-linear-gradient(
        45deg,
        #fff3b0,
        #fff3b0 4px,
        #ffe27a 4px,
        #ffe27a 8px
    );
}
.catch {
    border: 1px solid #4aa3a3;
    background:
        repeating-linear-gradient(
            -45deg,
            rgba(180, 235, 235, 0.85),
            rgba(180, 235, 235, 0.85) 6px,
            rgba(150, 215, 215, 0.85) 6px,
            rgba(150, 215, 215, 0.85) 12px
        );
    filter: saturate(0.95) contrast(1.05);
}
.teleport {
    border: 1px solid #8a7bd1;
    background:
        radial-gradient(
            circle at 30% 30%,
            rgba(255,255,255,0.35),
            rgba(255,255,255,0.05) 40%,
            transparent 60%
        ),
        linear-gradient(
            135deg,
            #e6ddff 0%,
            #cbbdff 45%,
            #b2a3f2 100%
        );
    background-size: 120px 120px, 100% 100%;
    filter: saturate(1.05);
}
.hide {
    border: 1px dashed #7f8f86;
    background:
        linear-gradient(
            135deg,
            rgba(255,255,255,0.18),
            rgba(255,255,255,0.05)
        ),
        repeating-linear-gradient(
            45deg,
            #dfe8e2,
            #dfe8e2 6px,
            #d4ddd7 6px,
            #d4ddd7 12px
        );
    filter: saturate(0.85) contrast(0.95);
}
.bomb {
    color: #fff;
    border: 1px solid #b22222;
    background: 
        radial-gradient(circle at 50% 50%, #ff6666 0%, #990000 70%),
        repeating-linear-gradient(
            45deg,
            rgba(255, 0, 0, 0.15),
            rgba(255, 0, 0, 0.15) 4px,
            rgba(139, 0, 0, 0.15) 4px,
            rgba(139, 0, 0, 0.15) 8px
        );
    box-shadow: 0 1px 2px #ff4444;
}
</style>)";
}
