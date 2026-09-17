
#pragma once

#include <algorithm>
#include <cctype>
#include <map>
#include <string>
#include <vector>


/* ========== 指令词表 ========== */

const std::map<std::string, Path> path_map = {
    {"法修", Path::FA}, {"体修", Path::TI}, {"邪修", Path::XIE},
};

const std::map<std::string, Act> simple_act_map = {
    {"聚气修行", Act::XIULIAN}, {"捶打修行", Act::XIULIAN}, {"修行", Act::XIULIAN},
    {"漫天星火", Act::XINGHUO}, {"金光护身", Act::JINGUANG}, {"吞天噬地", Act::TUNTIAN},
    {"不动如山", Act::BUDONG}, {"浑然一体", Act::HUNRAN}, {"以命祭杀", Act::YIMING},
    {"伪神目", Act::WEISHEN}, {"夺天造化功", Act::DUOTIAN}, {"虚行寿果", Act::XUXING},
    {"淬体", Act::CUITI}, {"铁骨铜皮", Act::TIEGU}, {"嘶吼", Act::SIHOU}, {"震爆", Act::ZHENBAO},
    {"护灵", Act::HULING}, {"窃机", Act::QIEJI}, {"因果反噬", Act::YINGUO},
    {"祭灭血阵", Act::XUEZHEN}, {"分魂", Act::FENHUN}, {"腐朽", Act::FUXIU},
    {"仙之势", Act::XIANSHI}, {"万念璃花", Act::LIHUA}, {"无垠寿果", Act::SHOUGUO},
    {"墨杀", Act::MOSHA},
};

const std::map<std::string, Act> target_act_map = {
    {"火球术", Act::HUOQIU}, {"猛击", Act::MENGJI}, {"暴击", Act::BAOJI},
    {"灭魂", Act::MIEHUN}, {"仙之威", Act::XIANWEI}, {"以痛止戈", Act::YITONG},
    {"墨杀", Act::MOSHA},
};

const std::map<std::string, Act> region_act_map = {
    {"万物皆虚", Act::WANWU},
};

const std::map<std::string, Act> direct_act_map = {
    {"虚神指", Act::XUSHEN},
};

// 招式代号词表：每个招式的代号（仙器以 Q 开头），大小写皆可，另收录几个顺手的别名
inline std::map<std::string, Act> MakeCodeActMap()
{
    std::map<std::string, Act> code_map;
    const auto add = [&code_map](const std::string& code, const Act act) {
        std::string upper = code;
        std::string lower = code;
        for (char& c : upper) {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        for (char& c : lower) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        code_map[upper] = act;
        code_map[lower] = act;
    };
    for (int a = 1; a < ACT_NUM; a++) {
        const Act act = static_cast<Act>(a);
        if (!ActInfoOf(act).code.empty()) {
            add(std::string(ActInfoOf(act).code), act);
        }
    }
    add("JQ", Act::XIULIAN);    // 聚气修行
    add("CD", Act::XIULIAN);    // 捶打修行
    return code_map;
}
const std::map<std::string, Act> code_act_map = MakeCodeActMap();

// 区域参数：阿拉伯数字与中文数字皆可，值为 0 起始的区域下标
const std::map<std::string, int> region_map = {
    {"1", 0}, {"一", 0},
    {"2", 1}, {"二", 1},
    {"3", 2}, {"三", 2},
    {"4", 3}, {"四", 3},
    {"5", 4}, {"五", 4},
    {"6", 5}, {"六", 5},
    {"7", 6}, {"七", 6},
    {"8", 7}, {"八", 7},
    {"9", 8}, {"九", 8},
};

const std::map<std::string, int> direct_map = {
    {"上", 0}, {"下", 1}, {"左", 2}, {"右", 3},
    {"左上", 4}, {"右上", 5}, {"左下", 6}, {"右下", 7},
    {"上左", 4}, {"上右", 5}, {"下左", 6}, {"下右", 7},
};


/* ========== 招式可用性 ========== */

// 判断某个招式在当前局势下能否被该玩家施展
inline bool ActUsable(const Board& board, const Player& player, const Act act)
{
    if (act == Act::LIHUA) {
        return player.artifact == Artifact::LIHUA && player.lihua_left > 0;
    }
    if (act == Act::SHOUGUO) {
        return player.artifact == Artifact::SHOUGUO && !player.shouguo_used;
    }
    if (act == Act::MOSHA) {
        return player.artifact == Artifact::MOSHA;
    }
    if (!player.CanUse(act)) {
        return false;
    }
    switch (act) {
        case Act::TUNTIAN:
            return player.region != CENTER_REGION && !board.Destroyed(player.region) &&
                   board.RegionEmptyExcept(player.region, player.pid);
        case Act::HUNRAN: case Act::YIMING:
            return player.HasYuanying() && !player.used_hunran && !player.used_yiming;
        case Act::ZHENBAO:
            return player.HasJindan() && !player.used_zhenbao;
        case Act::DUOTIAN:
            return board.artifact_state == ArtifactState::DESCENDED && player.region == CENTER_REGION;
        case Act::XUXING:
            return player.xuxing_round < 0;
        case Act::FUXIU:
            return !board.fuxiu_active;
        case Act::HUOQIU: case Act::MENGJI: case Act::BAOJI: case Act::MIEHUN: case Act::YITONG:
            return !board.RegionEmptyExcept(player.region, player.pid);
        case Act::XIANWEI:
            for (const Player& p : board.players) {
                if (p.Alive() && p.pid != player.pid && p.hp <= XIANWEI_HP_LIMIT) {
                    return true;
                }
            }
            return false;
        case Act::WANWU:
            for (int r = 0; r < REGION_NUM; r++) {
                if (r != CENTER_REGION && (!board.Destroyed(r) || !board.RegionEmpty(r))) {
                    return true;
                }
            }
            return false;
        default:
            return true;
    }
}

inline std::vector<Act> AvailableActs(const Board& board, const Player& player)
{
    std::vector<Act> list;
    for (int a = 1; a < ACT_NUM; a++) {
        const Act act = static_cast<Act>(a);
        if (ActUsable(board, player, act)) {
            list.push_back(act);
        }
    }
    return list;
}

// 将一次行动转为可读文本
inline std::string ActDesc(const Player& player, const Action& action)
{
    if (action.Empty()) {
        return "未行动";
    }
    std::string text = ActName(player.path, action.act);
    switch (ActArgType(action.act)) {
        case ArgType::PLAYER: text += "→" + PlayerNo(action.target); break;
        case ArgType::PLAYER_OPT: text += action.target < 0 ? "→全场" : "→" + PlayerNo(action.target); break;
        case ArgType::REGION: text += "→" + std::string(region_cn[action.region]); break;
        case ArgType::DIRECT: text += "→" + std::string(direct_cn[action.direct]); break;
        case ArgType::NONE: break;
    }
    return text;
}

// 生成某条修行路线的完整能力表，供规则指令使用
inline std::string MakePathDetail(const Path path)
{
    const int idx = PathIndex(path);
    std::string text = "【" + std::string(path_cn[idx]) + "】" + std::string(path_desc[idx]) + "\n境界：";
    for (int r = 0; r < RealmNum(path); r++) {
        text += (r > 0 ? " → " : "") + RealmName(path, r);
    }
    if (HasShouhun(path)) {
        text += "\n\n[" + RealmName(path, ShouhunRealm(path)) + "·被动] " + std::string(PASSIVE_SHOUHUN) + "：" +
                std::string(PASSIVE_SHOUHUN_BRIEF);
    }
    if (BusiRealm(path) >= 0) {
        text += "\n\n[" + RealmName(path, BusiRealm(path)) + "·被动] " + std::string(PASSIVE_BUSI) +
                "：进入该境界的下一回合起自动生效。" + std::string(PASSIVE_BUSI_BRIEF);
    }
    if (HuichunRealm(path) >= 0) {
        text += "\n\n[" + RealmName(path, HuichunRealm(path)) + "·被动] " + std::string(PASSIVE_HUICHUN) + "：" +
                std::string(PASSIVE_HUICHUN_BRIEF);
    }
    for (int r = 0; r < RealmNum(path); r++) {
        std::string acts;
        for (int a = 1; a < ACT_NUM; a++) {
            const Act act = static_cast<Act>(a);
            if (ActKindOf(act) == ActKind::RELIC) {
                continue;
            }
            if (RequiredRealm(path, act) == r) {
                acts += "\n　-" + ActCode(act) + "　" + ActName(path, act) + "：" + ActBrief(path, act);
            }
        }
        text += "\n\n[" + RealmName(path, r) + "]" + (acts.empty() ? "\n　（无额外能力，仅为过渡阶段）" : acts);
    }
    return text;
}


/* ========== 行动解析与校验 ========== */

// 将代号快捷指令附带的目标文本解析进行动，目标可省略时给出对应提示
inline bool FillArg(const Board& board, const int pid, Action& action, const std::string& arg, std::string& err)
{
    const std::string name = ActName(board.P(pid).path, action.act);
    switch (ActArgType(action.act)) {
        case ArgType::NONE:
            if (!arg.empty()) {
                err = "「" + name + "」无需指定目标";
                return false;
            }
            return true;
        case ArgType::PLAYER_OPT:
            if (arg.empty()) {
                action.target = -1;     // 不指定目标，剑气遍及全场
                return true;
            }
            [[fallthrough]];
        case ArgType::PLAYER: {
            if (arg.empty()) {
                err = "「" + name + "」须指明目标：在代号之后附上玩家编号";
                return false;
            }
            int value = 0;
            for (const char c : arg) {
                if (c < '0' || c > '9') {
                    err = "玩家编号只能是数字";
                    return false;
                }
                value = value * 10 + (c - '0');
                if (value > MAX_PLAYER) {
                    break;
                }
            }
            if (value < 1 || value > board.PlayerNum()) {
                err = "场中并无此人";
                return false;
            }
            action.target = value - 1;
            return true;
        }
        case ArgType::REGION: {
            if (arg.empty()) {
                err = "「" + name + "」须指明区域：在代号之后附上 1-9 或一至九";
                return false;
            }
            const auto it = region_map.find(arg);
            if (it == region_map.end()) {
                err = "此界只有九域，请指定 1-9 或一至九";
                return false;
            }
            action.region = it->second;
            return true;
        }
        case ArgType::DIRECT: {
            if (arg.empty()) {
                // 天地只余一域时，指向何方都是同一批人，方向便无需再指
                if (!board.OnlyCenterLeft()) {
                    err = "「" + name + "」须指向一个方向：上/下/左/右/左上/右上/左下/右下";
                    return false;
                }
                action.direct = 0;
                return true;
            }
            const auto it = direct_map.find(arg);
            if (it == direct_map.end()) {
                err = "天地之间无此方向。可用方向：上/下/左/右/左上/右上/左下/右下";
                return false;
            }
            action.direct = it->second;
            return true;
        }
    }
    return true;
}

inline bool ValidateAction(const Board& board, const int pid, const Action& action, std::string& err)
{
    const Player& player = board.P(pid);
    if (!player.Alive()) {
        err = "你已身死道消，再无出手之机";
        return false;
    }
    const bool relic_act = (ActKindOf(action.act) == ActKind::RELIC);
    if (!relic_act && RequiredRealm(player.path, action.act) < 0) {
        err = "【" + player.PathName() + "】一脉之中，并无「" + std::string(ActInfoOf(action.act).name) + "」这等法门";
        return false;
    }
    if (!relic_act && !player.CanUse(action.act)) {
        err = "境界未至：「" + ActName(player.path, action.act) + "」须修至【" +
              RealmName(player.path, RequiredRealm(player.path, action.act)) +
              "】方可施展，你如今不过【" + player.RealmStr() + "】";
        return false;
    }
    switch (action.act) {
        case Act::LIHUA:
            if (player.artifact != Artifact::LIHUA) {
                err = "仙器【万念璃花】不在你手";
                return false;
            }
            if (player.lihua_left <= 0) {
                err = "万念璃花已尽三度绽放，碎作齑粉";
                return false;
            }
            break;
        case Act::MOSHA:
            if (player.artifact != Artifact::MOSHA) {
                err = "仙器【墨杀仙剑】不在你手";
                return false;
            }
            break;
        case Act::SHOUGUO:
            if (player.artifact != Artifact::SHOUGUO) {
                err = "仙器【无垠寿果】不在你手";
                return false;
            }
            if (player.shouguo_used) {
                err = "寿果已然入腹，天地间再无第二枚";
                return false;
            }
            break;
        case Act::TUNTIAN:
            if (player.region == CENTER_REGION) {
                err = "中央区域乃此界之根，吞之不动";
                return false;
            }
            if (board.Destroyed(player.region)) {
                err = "此地早已不存于世，无物可吞";
                return false;
            }
            if (!board.RegionEmptyExcept(player.region, player.pid)) {
                err = "此地尚有他人踏足，吞天噬地无从施展";
                return false;
            }
            break;
        case Act::HUNRAN: case Act::YIMING:
            if (!player.HasYuanying()) {
                err = "你的元婴早已不存，此招无从谈起";
                return false;
            }
            if (player.used_hunran || player.used_yiming) {
                err = "浑然一体与以命祭杀，一生只择其一，且再无回头之路";
                return false;
            }
            break;
        case Act::ZHENBAO:
            if (!player.HasJindan()) {
                err = "金丹早已破碎，无物可爆";
                return false;
            }
            if (player.used_zhenbao) {
                err = "金丹已然自爆，此招不复再来";
                return false;
            }
            break;
        case Act::DUOTIAN:
            if (board.artifact_state != ArtifactState::DESCENDED) {
                err = "天穹未裂，何来仙器可夺";
                return false;
            }
            if (player.region != CENTER_REGION) {
                err = "唯有立于中央区域，方能伸手夺天";
                return false;
            }
            break;
        case Act::XUXING:
            if (player.xuxing_round >= 0) {
                err = "前番寿果的反噬尚未了结，不可再食";
                return false;
            }
            break;
        case Act::FUXIU:
            if (board.fuxiu_active) {
                err = "腐朽一世只可发动一次，你已用过";
                return false;
            }
            break;
        default:
            break;
    }
    switch (ActArgType(action.act)) {
        case ArgType::PLAYER_OPT: {
            if (action.target < 0) {
                return true;    // 不指名，剑气遍及全场
            }
            if (action.target >= board.PlayerNum()) {
                err = "场中并无此人";
                return false;
            }
            if (action.target == pid) {
                err = "剑不斩己，此招不可指向自身";
                return false;
            }
            if (!board.P(action.target).Alive()) {
                err = PlayerNo(action.target) + " 早已身死道消，不必再费气力";
                return false;
            }
            break;
        }
        case ArgType::PLAYER: {
            if (action.target < 0 || action.target >= board.PlayerNum()) {
                err = "场中并无此人";
                return false;
            }
            if (action.target == pid) {
                err = "刀不斩己，此招不可指向自身";
                return false;
            }
            const Player& target = board.P(action.target);
            if (!target.Alive()) {
                err = PlayerNo(action.target) + " 早已身死道消，不必再费气力";
                return false;
            }
            if (action.act != Act::XIANWEI && target.region != player.region) {
                err = "「" + ActName(player.path, action.act) + "」只及同域之人，" +
                      PlayerNo(action.target) + " 并不在区域「" + std::string(region_cn[player.region]) + "」之中";
                return false;
            }
            if (action.act == Act::XIANWEI && target.hp > XIANWEI_HP_LIMIT) {
                err = "仙之威只取残命：目标须是 3 血及以下，" + PlayerNo(action.target) + " 尚有 " +
                      NumStr(target.hp) + " 血";
                return false;
            }
            break;
        }
        case ArgType::REGION:
            if (action.region < 0 || action.region >= REGION_NUM) {
                err = "此界只有九域，并无此地";
                return false;
            }
            if (action.region == CENTER_REGION) {
                err = "中央区域乃此界之根，湮之不灭";
                return false;
            }
            if (board.Destroyed(action.region) && board.RegionEmpty(action.region)) {
                err = "区域「" + std::string(region_cn[action.region]) + "」早已归于虚无，再无可湮灭之物";
                return false;
            }
            break;
        case ArgType::DIRECT:
            if (action.direct < 0 || action.direct >= DIRECT_NUM) {
                err = "天地之间并无此方向";
                return false;
            }
            break;
        case ArgType::NONE:
            break;
    }
    return true;
}
