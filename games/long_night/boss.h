
// 回合阶段前置声明：RoundStage 定义于游戏命名空间内
namespace lgtbot { namespace game { namespace GAME_MODULE_NAME { class RoundStage; } } }
using RoundStage = lgtbot::game::GAME_MODULE_NAME::RoundStage;

// BOSS 基类：一局游戏中可同时存在多个 BOSS，由 Board::bosses 持有
class Boss
{
  public:
    Boss(const BossType type, int& size, vector<Player>& players)
        : type(type), size(size), players(players) {}
    virtual ~Boss() = default;

    // BOSS信息
    const BossType type;
    int x = 0, y = 0;
    int steps = -1;
    lgtbot::PlayerID target = 0;
    // 同类型BOSS存在多个时的编号后缀
    string index_label;

    // 辅助成员变量
    int& size;   // 地图大小
    vector<Player>& players;  // Board玩家引用

    // BOSS 基础信息
    virtual string GetBossBaseName() const = 0;
    virtual string GetBossIcon() const = 0;
    virtual string GetBossDescription() const = 0;      // 开场描述

    // 显示名称：同类型有多个时带编号
    string GetBossName() const { return GetBossBaseName() + index_label; }
    string GetBossStartInfo() const { return GetBossName() + " " + GetBossDescription(); }
    // 播报前缀：[BOSS-名称] / 【名称】
    string GetBossTag() const { return "[BOSS-" + GetBossName() + "]"; }
    string GetBossSay() const { return "【" + GetBossName() + "】"; }

    // 回合结算行动：由各 BOSS 自行实现（函数体见 mygame.cc）
    virtual void HandleRoundAction(RoundStage& stage, string& boss_record, lgtbot::ChildMsgSenderBase::MsgSenderGuard& sender) = 0;

    // BOSS行为信息
    struct BossMoveRecord {
        string content;
        Sound sound;
        vector<string> propagation; // 声音向其他所有玩家的传播方向（私信完整赛况）

        BossMoveRecord(string content, Sound sound) : content(content), sound(sound) {}
    };
    vector<BossMoveRecord> boss_all_record;

    void NewRecord(const string& content, const Sound sound = Sound::NONE) { boss_all_record.push_back({content, sound}); }
    void UpdateContentRecord(const string& content) { if (!boss_all_record.empty()) boss_all_record.back().content = content; }
    void UpdateSoundRecord(const Sound sound) { if (!boss_all_record.empty()) boss_all_record.back().sound = sound; }
    void AddSoundPropagation(const string& direct_str) { if (!boss_all_record.empty()) boss_all_record.back().propagation.push_back(direct_str); }
    // 添加BOSS开局记录
    void InitBossStartRecord()
    {
        NewRecord("【开局】初始锁定玩家为 [" + to_string(target) + "号]", StartRecordSound());
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
    void BossInitialize()
    {
        this->steps = InitialSteps();
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
            if (SkipSameCellTarget() && d == 0) continue;   // [邦邦]位置相同需强制更换目标
            if (d < curDist) {
                this->target = player.pid;
                OnTargetSelected();     // [米诺陶斯]需要重置步数
                curDist = d;
            }
        }
        return curTarget != this->target;
    }

    // BOSS移动和更换目标（抓到人返回true）
    bool BossMove()
    {
        OnMoveBegin();      // [米诺陶斯]需要增加步数
        int targetDist = ManhattanDistance(this->x, this->y, players[this->target].x, players[this->target].y, size);
        // 步数足够直接走到目标位置
        if (this->steps >= targetDist) {
            this->x = players[this->target].x;
            this->y = players[this->target].y;
            OnReachTarget();    // [米诺陶斯]需要重置步数
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

    bool Is(const BossType type) const { return this->type == type; }

  protected:
    // ========== 行为钩子：默认无特殊行为，由派生类按需覆盖 ==========
    virtual int InitialSteps() const = 0;                       // 初始步数
    virtual Sound StartRecordSound() const { return Sound::NONE; }  // 开局记录携带的声响
    virtual bool SkipSameCellTarget() const { return false; }   // 同格玩家是否跳过（不作为目标）
    virtual void OnTargetSelected() {}                          // 选中新目标时
    virtual void OnMoveBegin() {}                               // 每次移动开始时
    virtual void OnReachTarget() {}                             // 抵达目标位置时
};


// 【🐮米诺陶斯】步数逐回合递增，抵达目标即捕捉出局；移动结束发出巨响
class MinotaurBoss : public Boss
{
  public:
    MinotaurBoss(int& size, vector<Player>& players) : Boss(BossType::MINOTAUR, size, players) {}

    string GetBossBaseName() const override { return "米诺陶斯"; }
    string GetBossIcon() const override { return "🐮"; }
    string GetBossDescription() const override
    {
        return "现身于地图中，会在回合结束时追击最近的玩家。BOSS发出震耳欲聋的巨响！请所有玩家留意BOSS开局所在的方位！";
    }

    void HandleRoundAction(RoundStage& stage, string& boss_record, lgtbot::ChildMsgSenderBase::MsgSenderGuard& sender) override;

  protected:
    int InitialSteps() const override { return 0; }
    Sound StartRecordSound() const override { return Sound::BOSS; }
    void OnTargetSelected() override { steps = 0; }     // 更换目标重置步数
    void OnMoveBegin() override { steps++; }            // 每回合步数递增
    void OnReachTarget() override { steps = 0; }        // 抓到玩家重置步数
};


// 【💣邦邦】固定速度追击，抵达目标不捕捉但会换目标；每回合在落点放置炸弹
class BangBangBoss : public Boss
{
  public:
    BangBangBoss(int& size, vector<Player>& players) : Boss(BossType::BANGBANG, size, players) {}

    string GetBossBaseName() const override { return "邦邦"; }
    string GetBossIcon() const override { return "💣"; }
    string GetBossDescription() const override
    {
        return "带着[炸弹]现身于地图中，会在回合结束时追击最近玩家，并在结束位置放置[炸弹]。玩家经过并离开会炸飞并出局！";
    }

    void HandleRoundAction(RoundStage& stage, string& boss_record, lgtbot::ChildMsgSenderBase::MsgSenderGuard& sender) override;

  protected:
    int InitialSteps() const override { return rand() % 3 + 3; }    // 固定速度 3-5 随机
    bool SkipSameCellTarget() const override { return true; }       // 位置相同需强制更换目标
};


// BOSS 工厂：按类型创建对应的 BOSS 对象
inline std::unique_ptr<Boss> CreateBoss(const BossType type, int& size, vector<Player>& players)
{
    switch (type) {
        case BossType::MINOTAUR:    return std::make_unique<MinotaurBoss>(size, players);
        case BossType::BANGBANG:    return std::make_unique<BangBangBoss>(size, players);
        default:                    return nullptr;
    }
}
