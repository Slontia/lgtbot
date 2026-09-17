#pragma once

#include <algorithm>
#include <string>
#include <vector>


/* ========== 取招偏好 ========== */
inline constexpr int BOT_WEIGHT_BASE = 2;       // 每一式的基础权重
inline constexpr int BOT_WEIGHT_PER_REALM = 3;  // 招式每高一个境界追加的权重
inline constexpr int BOT_WEIGHT_RELIC = 14;     // 仙器难得，逢机便用
inline constexpr int BOT_LOW_HP_PERCENT = 35;   // 血量低于初始值的该百分比即视为重伤
inline constexpr int BOT_DEFEND_BONUS = 4;      // 重伤时守御类招式的权重倍数
inline constexpr int BOT_LOW_REALM = 2;         // 境界低于此值即视为根基尚浅
inline constexpr int BOT_FEEBLE_CULT_BONUS = 8; // 根基浅且出手无力时，修行的权重倍数
inline constexpr int BOT_ALONE_CULT_BONUS = 6;  // 独处一域又未至顶点时，修行的权重倍数
inline constexpr int BOT_REACH_BONUS = 6;       // 需伤及他域时，跨域招式的权重倍数
inline constexpr int BOT_CLIMB_BONUS = 4;       // 半仙晋升手段（修行、吞天噬地）的权重倍数
inline constexpr int BOT_FUXIU_BONUS = 4;       // 半仙施展腐朽的权重倍数
inline constexpr int BOT_HUNT_BONUS = 5;        // 围猎半仙或残血者时，攻伐招式的权重倍数
inline constexpr int BOT_LAST_RESORT_BONUS = 6; // 命悬一线时，浑然一体与以命祭杀的权重倍数


/* ========== 招式估值 ========== */
inline constexpr Num BOT_FEEBLE_DMG = N(3);     // 最强一击不足此数即视为出手无力
inline constexpr Num BOT_WEAK_HP = N(5);        // 血量不高于此数即为残血，值得围猎
inline constexpr Num BOT_DESPERATE_HP = N(5);   // 自身血量低于此数即是命悬一线

// 该招式能否伤及他域之人
inline constexpr bool ActReaches(const Act act)
{
    switch (act) {
        case Act::WANWU: case Act::XUSHEN: case Act::XIANWEI: case Act::MOSHA: return true;
        default:                                                               return false;
    }
}
inline constexpr Num ActDamage(const Act act)
{
    switch (act) {
        case Act::HUOQIU:                   return DMG_HUOQIU;
        case Act::XINGHUO:                  return DMG_XINGHUO;
        case Act::MENGJI:                   return DMG_MENGJI;
        case Act::BAOJI:                    return DMG_BAOJI;
        case Act::MIEHUN:                   return DMG_MIEHUN;
        case Act::SIHOU:                    return DMG_SIHOU;
        case Act::ZHENBAO:                  return DMG_ZHENBAO;
        case Act::YITONG:                   return DMG_YITONG;
        case Act::XUSHEN:                   return DMG_XUSHEN;
        case Act::YIMING:                   return DMG_YIMING;
        case Act::XUEZHEN:                  return DMG_XUEZHEN;
        case Act::YINGUO:                   return DMG_YINGUO;
        case Act::MOSHA:                    return DMG_MOSHA;
        case Act::WANWU: case Act::XIANWEI:  return N(99);   // 径直取人性命
        default:                            return 0;
    }
}


/* ========== 局势判读与取招 ========== */

// 出手前先看清局势：所有判断只取公开可见之情报
struct BotView
{
    bool feeble = false;        // 根基尚浅，且手上最重的一击也打不出 3 点伤害
    bool alone = false;         // 本区域只有自己
    bool at_peak = false;       // 已至顶点，修行再无所得
    bool self_banxian = false;  // 自己便是半仙
    bool desperate = false;     // 自身命悬一线，该动用保命或同归的手段了
    int banxian_here = -1;      // 同区域中已现身的半仙
    int banxian_afar = -1;      // 他域中已现身的半仙
    int weak_here = -1;         // 同区域中的残血之人
};

// 境界名为「半仙」者，其身份已写在公屏的赛况之上，无需窥探即可知晓
inline bool BanxianRevealed(const Player& p)
{
    return p.Alive() && p.path == Path::BAN && p.realm >= MaxRealm(Path::BAN);
}

inline BotView SurveyBoard(const Board& board, const Player& player, const std::vector<Act>& candidates)
{
    BotView view;
    view.at_peak = player.AtMaxRealm();
    view.self_banxian = (player.path == Path::BAN);
    view.alone = board.RegionEmptyExcept(player.region, player.pid);
    view.desperate = (player.hp < BOT_DESPERATE_HP);
    if (player.realm < BOT_LOW_REALM) {
        Num best = 0;
        for (const Act act : candidates) {
            best = std::max(best, ActDamage(act));
        }
        view.feeble = (best < BOT_FEEBLE_DMG);
    }
    Num weakest = BOT_WEAK_HP;
    for (const Player& other : board.players) {
        if (!other.Alive() || other.pid == player.pid) {
            continue;
        }
        const bool same_region = (other.region == player.region);
        if (BanxianRevealed(other)) {
            (same_region ? view.banxian_here : view.banxian_afar) = other.pid.Get();
        }
        // 残血者只在同域时才好下手，取其中最弱的一个
        if (same_region && other.hp <= weakest) {
            weakest = other.hp;
            view.weak_here = other.pid.Get();
        }
    }
    return view;
}

// 电脑玩家的取招权重：境界越高的招式越受青睐，仙器逢机便用；重伤之际偏向守御；此外再依局势加权
inline int ActWeight(const Board& board, const Player& player, const Act act, const BotView& view)
{
    if (ActKindOf(act) == ActKind::RELIC) {
        return BOT_WEIGHT_RELIC;
    }
    const int required = RequiredRealm(player.path, act);
    int weight = BOT_WEIGHT_BASE + (required > 0 ? required : 0) * BOT_WEIGHT_PER_REALM;
    const bool climb = (act == Act::XIULIAN || act == Act::TUNTIAN);
    // 根基尚浅又打不疼人，与其胡乱出手，不如闭关积攒修为
    if (view.feeble && act == Act::XIULIAN) {
        weight *= BOT_FEEBLE_CULT_BONUS;
    }
    // 独处一域：未至顶点便安心修行，已至顶点则设法伤及他域
    if (view.alone) {
        if (!view.at_peak && climb) {
            weight *= BOT_ALONE_CULT_BONUS;
        } else if (view.at_peak && ActReaches(act)) {
            weight *= BOT_REACH_BONUS;
        }
    }
    // 半仙进阶更快、上限更高，故更急于登高；腐朽亦是其独门手段
    if (view.self_banxian) {
        if (!view.at_peak && climb) {
            weight *= BOT_CLIMB_BONUS;
        }
        if (act == Act::FUXIU) {
            weight *= BOT_FUXIU_BONUS;
        }
    }
    // 命悬一线：元婴一途只此一次，与其坐等身死，不如脱体而出或拉人同归
    if (view.desperate && (act == Act::HUNRAN || act == Act::YIMING)) {
        weight *= BOT_LAST_RESORT_BONUS;
    }
    // 半仙已然现身，或身边有人只剩残血，皆是该出手的时候
    if (ActKindOf(act) == ActKind::ATTACK) {
        if (view.banxian_here >= 0 || view.weak_here >= 0) {
            weight *= BOT_HUNT_BONUS;
        }
        if (view.banxian_afar >= 0 && ActReaches(act)) {
            weight *= BOT_HUNT_BONUS;
        }
    }
    if (player.hp * 100 <= board.cfg.init_hp * BOT_LOW_HP_PERCENT) {
        if (ActKindOf(act) == ActKind::DEFEND) {
            weight *= BOT_DEFEND_BONUS;
        } else if (ActKindOf(act) == ActKind::ATTACK) {
            weight = (weight + 1) / 2;
        }
    }
    return weight;
}

// 择猎：已现身的半仙最当诛，其次是同域中气息最弱者；皆不在射程之内则返回 -1
inline int PickPrey(const std::vector<int>& targets, const BotView& view)
{
    const auto has = [&targets](const int id) {
        return id >= 0 && std::find(targets.begin(), targets.end(), id) != targets.end();
    };
    if (has(view.banxian_here)) {
        return view.banxian_here;
    }
    if (has(view.banxian_afar)) {
        return view.banxian_afar;
    }
    if (has(view.weak_here)) {
        return view.weak_here;
    }
    return -1;
}

// 按权重抽取一式
inline Act PickWeightedAct(Board& board, const std::vector<Act>& acts, const std::vector<int>& weights)
{
    int total = 0;
    for (const int w : weights) {
        total += w;
    }
    if (total <= 0) {
        return board.RandomPick(acts);
    }
    int roll = board.RandomInt(total);
    for (size_t i = 0; i < acts.size(); i++) {
        roll -= weights[i];
        if (roll < 0) {
            return acts[i];
        }
    }
    return acts.back();
}

inline void RandomAct(Board& board, const int pid, Action& action)
{
    const Player& player = board.P(pid);
    const std::vector<Act> candidates = AvailableActs(board, player);
    const BotView view = SurveyBoard(board, player, candidates);
    std::vector<int> weights;
    weights.reserve(candidates.size());
    for (const Act act : candidates) {
        weights.push_back(ActWeight(board, player, act, view));
    }
    for (int attempt = 0; attempt < 30 && !candidates.empty(); attempt++) {
        Action candidate;
        candidate.act = PickWeightedAct(board, candidates, weights);
        switch (ActArgType(candidate.act)) {
            case ArgType::PLAYER_OPT: {
                std::vector<int> targets;
                for (int i = 0; i < board.PlayerNum(); i++) {
                    if (i != pid && board.P(i).Alive()) {
                        targets.push_back(i);
                    }
                }
                // 有猎物便指名取其性命；否则或横扫全场，或随手一斩，各半
                const int prey = PickPrey(targets, view);
                candidate.target = prey >= 0                            ? prey
                                 : (targets.empty() || board.RandomInt(2) == 0) ? -1
                                                                        : board.RandomPick(targets);
                break;
            }
            case ArgType::PLAYER: {
                std::vector<int> targets;
                for (int i = 0; i < board.PlayerNum(); i++) {
                    if (i == pid || !board.P(i).Alive()) {
                        continue;
                    }
                    if (candidate.act == Act::XIANWEI) {
                        if (board.P(i).hp <= XIANWEI_HP_LIMIT) {
                            targets.push_back(i);
                        }
                    } else if (board.P(i).region == player.region) {
                        targets.push_back(i);
                    }
                }
                if (targets.empty()) {
                    continue;
                }
                const int prey = PickPrey(targets, view);
                candidate.target = prey >= 0 ? prey : board.RandomPick(targets);
                break;
            }
            case ArgType::REGION: {
                std::vector<int> regions;
                for (int r = 0; r < REGION_NUM; r++) {
                    if (r != CENTER_REGION && (!board.Destroyed(r) || !board.RegionEmpty(r))) {
                        regions.push_back(r);
                    }
                }
                if (regions.empty()) {
                    continue;
                }
                // 半仙若在他域，湮灭之地便直指其所在
                const int lair = view.banxian_afar < 0 ? -1 : board.P(view.banxian_afar).region;
                candidate.region = (lair >= 0 && std::find(regions.begin(), regions.end(), lair) != regions.end())
                        ? lair : board.RandomPick(regions);
                break;
            }
            case ArgType::DIRECT: {
                // 半仙若在他域，剑气便朝那个方向递出
                candidate.direct = -1;
                if (view.banxian_afar >= 0) {
                    const int lair = board.P(view.banxian_afar).region;
                    std::vector<int> aimed;
                    for (int d = 0; d < DIRECT_NUM; d++) {
                        const std::vector<int> covered = board.XushenRegions(player.region, d);
                        if (std::find(covered.begin(), covered.end(), lair) != covered.end()) {
                            aimed.push_back(d);
                        }
                    }
                    if (!aimed.empty()) {
                        candidate.direct = board.RandomPick(aimed);
                    }
                }
                if (candidate.direct < 0) {
                    candidate.direct = board.RandomInt(DIRECT_NUM);
                }
                break;
            }
            case ArgType::NONE:
                break;
        }
        std::string err;
        if (ValidateAction(board, pid, candidate, err)) {
            action = candidate;
            return;
        }
    }
    action.Clear();
    action.act = Act::XIULIAN;
    action.by_default = true;
}
