
class Boss
{
  public:
    Boss(int& size, vector<Player>& players) : size(size), players(players) {}
    
    // BOSS信息
    BossType type = BossType::NONE;
    int x, y;
    int steps = -1;
    PlayerID target = 0;

    // 辅助成员变量
    int& size;   // 地图大小
    vector<Player>& players;  // Board玩家引用

    // BOSS行为信息
    struct BossMove {
        string content;
        Sound sound;
        vector<string> propagation; // 声音向其他所有玩家的传播方向（私信完整赛况）

        BossMove(string content, Sound sound) : content(content), sound(sound) {}
    };
    vector<BossMove> boss_all_record;

    void NewRecord(const string& content, const Sound sound = Sound::NONE) { boss_all_record.push_back({content, sound}); }
    void UpdateContentRecord(const string& content) { if (!boss_all_record.empty()) boss_all_record.back().content = content; }
    void UpdateSoundRecord(const Sound sound) { if (!boss_all_record.empty()) boss_all_record.back().sound = sound; }
    void AddSoundPropagation(const string& direct_str) { if (!boss_all_record.empty()) boss_all_record.back().propagation.push_back(direct_str); }
    // 添加BOSS开局记录
    void InitBossStartRecord()
    {
        if (Is(BossType::MINOTAUR)) NewRecord("【开局】初始锁定玩家为 [" + to_string(target) + "号]", Sound::BOSS);
        if (Is(BossType::BANGBANG)) NewRecord("【开局】初始锁定玩家为 [" + to_string(target) + "号]");
    }

    string GetBossName() const
    {
        if (Is(BossType::MINOTAUR)) return "米诺陶斯";
        if (Is(BossType::BANGBANG)) return "邦邦";
        return "[错误BOSS]";
    }

    string GetBossIcon() const
    {
        if (Is(BossType::MINOTAUR)) return "🐮";
        if (Is(BossType::BANGBANG)) return "💣";
        return "⚠️";
    }

    string GetBossStartInfo() const
    {
        switch (type) {
            case BossType::MINOTAUR:    return "米诺陶斯 现身于地图中，会在回合结束时追击最近的玩家。BOSS发出震耳欲聋的巨响！请所有玩家留意BOSS开局所在的方位！";
            case BossType::BANGBANG:    return "邦邦 带着[炸弹]现身于地图中，会在回合结束时追击最近玩家，并在结束位置放置[炸弹]。玩家经过并离开会炸飞并出局！";
            default:                    return "[错误BOSS]";
        }
    }

    string GetBossRecord(const int query_pid, const bool is_public, const bool is_html = true) const
    {
        string result;

        for (const auto& mv : boss_all_record) {
            string sound_d;
            if (mv.sound != Sound::NONE && !is_public) {
                if (query_pid >= 0 && query_pid < mv.propagation.size()) {
                    sound_d = "[" + mv.propagation[query_pid] + "]";
                } else if (query_pid >= 0) {
                    sound_d = "{越界异常[pid=" + to_string(query_pid) + "]}";
                }
            }

            result += is_html ? "<br>" : "\n";

            if (mv.sound == Sound::BOSS) {  // [巨响]
                result += mv.content + "（巨响" + sound_d + "）";
            } else {
                result += mv.content;
            }
        }

        return result;
    }

    // BOSS初始化
    void BossInitialize(const BossType type)
    {
        this->type = type;
        if (Is(BossType::MINOTAUR)) this->steps = 0;
        if (Is(BossType::BANGBANG)) this->steps = rand() % 3 + 3;
        BossSpawn();
        BossChangeTarget(true);
    }

    // BOSS生成
    void BossSpawn()
    {
        // 不生成在玩家周围8格
        for (int attempt = 0; attempt < 500; attempt++) {
            int x = rand() % size;
            int y = rand() % size;
            bool nearPlayer = false;
            for (int i = 0; i < players.size() && !nearPlayer; i++) {
                for (int dx = -1; dx <= 1 && !nearPlayer; dx++) {
                    for (int dy = -1; dy <= 1 && !nearPlayer; dy++) {
                        int nx = (players[i].x + dx + size) % size;
                        int ny = (players[i].y + dy + size) % size;
                        if (nx == x && ny == y)
                            nearPlayer = true;
                    }
                }
            }
            this->x = x;
            this->y = y;
            if (!nearPlayer) break;
        }
    }

    // BOSS更换目标（更换返回true）
    bool BossChangeTarget(const bool reset)
    {
        int curTarget = this->target;
        int curDist = ManhattanDistance(this->x, this->y, players[this->target].x, players[this->target].y, size);
        if (reset || players[this->target].out > 0) curDist = INT_MAX;
        for (auto& player : players) {
            if (player.out > 0) continue;
            int d = ManhattanDistance(this->x, this->y, player.x, player.y, size);
            if (Is(BossType::BANGBANG) && d == 0) continue;     // [邦邦]位置相同需强制更换目标
            if (d < curDist) {
                this->target = player.pid;
                if (Is(BossType::MINOTAUR)) this->steps = 0;    // [米诺陶斯]需要重置步数
                curDist = d;
            }
        }
        return curTarget != this->target;
    }

    // BOSS移动和更换目标（抓到人返回true）
    bool BossMove()
    {
        int targetDist = ManhattanDistance(this->x, this->y, players[this->target].x, players[this->target].y, size);
        if (Is(BossType::MINOTAUR)) this->steps++;  // [米诺陶斯]需要增加步数
        // 步数足够直接走到目标位置
        if (this->steps >= targetDist) {
            this->x = players[this->target].x;
            this->y = players[this->target].y;
            if (Is(BossType::MINOTAUR)) this->steps = 0;    // [米诺陶斯]需要重置步数
            return true;
        }
        // 执行移动
        int stepsRemaining = this->steps;
        while (stepsRemaining > 0) {
            targetDist = ManhattanDistance(this->x, this->y, players[this->target].x, players[this->target].y, size);
            int tx = players[this->target].x, ty = players[this->target].y;
            int dx = tx - this->x, dy = ty - this->y;
            if (abs(dx) > size / 2) { dx = (dx > 0) ? dx - size : dx + size; }
            if (abs(dy) > size / 2) { dy = (dy > 0) ? dy - size : dy + size; }

            // 如果|dx|≠|dy|则沿较大差值轴走一步
            if (abs(dx) != abs(dy)) {
                if (abs(dx) > abs(dy)) {
                    this->x = (this->x + ((dx > 0) ? 1 : -1) + size) % size;
                } else {
                    this->y = (this->y + ((dy > 0) ? 1 : -1) + size) % size;
                }
            } else {
                // 如果已经正方形，随机选择在 x 或 y 方向上移动一步
                if (rand() % 2 == 0)
                    this->x = (this->x + ((dx > 0) ? 1 : -1) + size) % size;
                else
                    this->y = (this->y + ((dy > 0) ? 1 : -1) + size) % size;
            }
            stepsRemaining--;
        }
        return false;
    }

    bool IsBossNearby(const Player& player) const
    {
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                int nx = (this->x + dx + size) % size;
                int ny = (this->y + dy + size) % size;
                if (player.x == nx && player.y == ny) {
                    return true;
                }
            }
        }
        return false;
    }

    bool Enable() const { return this->type != BossType::NONE; }
    bool Is(BossType type) const { return this->type == type; }
};
