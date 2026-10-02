
#pragma once

#include <string>
#include <vector>


// 玩家提交的一次行动
struct Action
{
    Act act = Act::NONE;
    int target = -1;        // 目标玩家（ArgType::PLAYER）
    int region = -1;        // 目标区域（ArgType::REGION）
    int direct = -1;        // 目标方向（ArgType::DIRECT）
    bool by_default = false;    // 由超时或退出代为选择

    void Clear()
    {
        act = Act::NONE;
        target = -1;
        region = -1;
        direct = -1;
        by_default = false;
    }
    bool Empty() const { return act == Act::NONE; }
};


class Score
{
  public:
    // 每存活一个回合的得分
    static constexpr int SURVIVE_SCORE = 2;
    // 每击杀一名修士的得分
    static constexpr int KILL_SCORE = 10;
    // 中途退出的惩罚
    static constexpr int QUIT_SCORE = -50;
    // 按最终名次给出的排名分，第七名及以后不再有名次分
    static constexpr int rank_score[6] = {60, 30, 20, 15, 10, 5};

    static int RankScoreOf(const int rank)
    {
        const int idx = rank - 1;
        const int last = static_cast<int>(sizeof(rank_score) / sizeof(rank_score[0])) - 1;
        if (idx > last) {
            return 0;
        }
        return rank_score[idx < 0 ? 0 : idx];
    }

    // 存活回合数
    int survive_rounds = 0;
    // 击杀数
    int kills = 0;
    // 最终名次（1 为最优，0 表示尚未确定）
    int rank = 0;
    // 退出惩罚
    int quit_score = 0;

    int SurviveScore() const { return survive_rounds * SURVIVE_SCORE; }
    int KillScore() const { return kills * KILL_SCORE; }
    int RankScore() const { return rank == 0 ? 0 : RankScoreOf(rank); }
    int FinalScore() const { return SurviveScore() + KillScore() + RankScore() + quit_score; }
};


class Player
{
  public:
    Player(const lgtbot::PlayerID pid, const std::string& name, const std::string& avatar, const Num init_hp)
        : pid(pid), name(name), avatar(avatar), hp(init_hp), hp_start(init_hp) {}

    /* ===== 基本信息 ===== */
    lgtbot::PlayerID pid;
    std::string name;
    std::string avatar;

    /* ===== 修行状态 ===== */
    // 修行方向，全程保密
    Path path = Path::FA;
    // 当前境界索引
    int realm = 0;
    // 当前境界的修为进度，达到最高境界后继续累积但不再晋升
    Num cultivation = 0;

    /* ===== 生存状态 ===== */
    // 玩家血量
    Num hp;
    // 所在区域，-1 表示尚未降临
    int region = -1;
    // 0 存活；1 已死亡；2 已退出
    int out = 0;
    // 死亡或退出的回合，用于最终名次排序
    int out_round = 0;

    /* ===== 技能状态 ===== */
    // 金丹与元婴是否已失去（自行消耗或被窃机夺走）
    bool jindan_lost = false;
    bool yuanying_lost = false;
    // 浑然一体与以命祭杀互斥，任一使用后两者均不可再用
    bool used_hunran = false;
    bool used_yiming = false;
    // 震爆使用后无法再次使用
    bool used_zhenbao = false;
    // 腐朽全局仅可发起一次
    bool used_fuxiu = false;
    // 进入合道的回合，不死不灭于其下一回合起生效
    int hedao_round = -1;
    // 虚行寿果的延迟扣血回合，-1 表示无待结算
    int xuxing_round = -1;
    // 吞天噬地后下一回合必须离开当前区域
    bool must_leave = false;

    /* ===== 仙器 ===== */
    Artifact artifact = Artifact::NONE;
    // 万念璃花剩余使用次数
    int lihua_left = LIHUA_LIMIT;
    // 无垠寿果是否已服用
    bool shouguo_used = false;
    // 水中镜是否已破碎
    bool jingjian_broken = false;

    /* ===== 本回合数据 ===== */
    // 本回合提交的行动
    Action action;
    // 万念璃花追加的行动
    Action extra_action;
    // 本回合结算开始时的血量，用于展示血量变化与仙之威的资格判定
    Num hp_start;
    // 本回合最终受到的伤害总和，不含自身行动的代价
    Num dmg_taken = 0;
    // 上述伤害中源自腐朽的部分，不死不灭免不去这一份
    Num fuxiu_taken = 0;
    // 本回合自身行动造成的扣血
    Num self_cost = 0;
    // 本回合对他人造成的最终伤害总量
    Num dmg_dealt = 0;
    // 本回合是否被他人的攻击实际命中
    bool attacked = false;
    // 本回合对自己造成过伤害的修士
    std::vector<lgtbot::PlayerID> damagers;
    // 本回合累计获得的修为与晋升的境界数
    Num cult_gain = 0;
    int promoted = 0;
    // 本回合的结算结果，用于播报上色
    RoundResult result = RoundResult::NONE;
    // 本回合私信给本人的结算详情
    std::string round_detail;

    /* ===== 计分 ===== */
    Score score;

    /* ===== 查询 ===== */
    bool Alive() const { return out == 0; }
    bool AtMaxRealm() const { return realm >= MaxRealm(path); }
    std::string PathName() const { return std::string(path_cn[PathIndex(path)]); }
    std::string RealmStr() const { return RealmName(path, realm); }

    // 仙器是否仍有效：用尽或已破碎者不再示人。墨杀仙剑可反复施展，永不失效
    bool ArtifactActive() const
    {
        switch (artifact) {
            case Artifact::LIHUA:   return lihua_left > 0;
            case Artifact::SHOUGUO: return !shouguo_used;
            case Artifact::JINGJIAN: return !jingjian_broken;
            case Artifact::MOSHA:   return true;
            default:                return false;
        }
    }

    // 是否持有金丹
    bool HasJindan() const
    {
        const int r = JindanRealm(path);
        return r >= 0 && realm >= r && !jindan_lost;
    }
    // 是否持有元婴
    bool HasYuanying() const
    {
        const int r = YuanyingRealm(path);
        return r >= 0 && realm >= r && !yuanying_lost;
    }
    // 不死不灭是否已生效（进入合道的下一回合起）
    bool BusiActive(const int round) const
    {
        const int r = BusiRealm(path);
        return r >= 0 && hedao_round > 0 && round > hedao_round;
    }
    // 是否可以使用指定行动（仅判断修行方向与境界，不含临时条件）
    bool CanUse(const Act act) const
    {
        const int required = RequiredRealm(path, act);
        return required >= 0 && realm >= required;
    }

    // 增加修为并结算晋升，返回晋升的境界数
    int GainCultivation(const Num value, const Num step)
    {
        cultivation += value;
        int promoted = 0;
        while (!AtMaxRealm() && cultivation >= step) {
            cultivation -= step;
            realm++;
            promoted++;
        }
        return promoted;
    }

    // 清空本回合的临时数据
    void ClearRoundData()
    {
        action.Clear();
        extra_action.Clear();
        dmg_taken = 0;
        fuxiu_taken = 0;
        self_cost = 0;
        dmg_dealt = 0;
        cult_gain = 0;
        promoted = 0;
        attacked = false;
        damagers.clear();
        result = RoundResult::NONE;
        round_detail.clear();
    }
};
