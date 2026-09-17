
#pragma once

#include <array>
#include <string>
#include <string_view>


/* ========== 定点数值 ========== */
// 血量与修为均可能出现 0.5 / 1.5 / 2.5 等小数，统一以 1/100 为单位存储，避免浮点比较误差
using Num = int32_t;
inline constexpr Num NUM_UNIT = 100;

// 整数转定点
inline constexpr Num N(const int integer) { return integer * NUM_UNIT; }
// 浮点转定点，四舍五入到 1/100，供带小数的配置项取用
inline constexpr Num NumFrom(const double value)
{
    return static_cast<Num>(value * NUM_UNIT + (value < 0 ? -0.5 : 0.5));
}
// 定点数减半
inline constexpr Num NumHalf(const Num v) { return v / 2; }
// 定点数乘 1.5
inline constexpr Num NumMul1_5(const Num v) { return v * 3 / 2; }

// 定点数转显示字符串，自动省略无意义的小数位
inline std::string NumStr(const Num v)
{
    const bool neg = v < 0;
    const Num a = neg ? -v : v;
    std::string s = std::to_string(a / NUM_UNIT);
    const Num frac = a % NUM_UNIT;
    if (frac != 0) {
        s += '.';
        s += static_cast<char>('0' + frac / 10);
        if (frac % 10 != 0) {
            s += static_cast<char>('0' + frac % 10);
        }
    }
    return neg ? "-" + s : s;
}


/* ========== 基础常量 ========== */
inline constexpr int MIN_PLAYER = 6;            // 最少玩家数
inline constexpr int MAX_PLAYER = 32;           // 最多玩家数
inline constexpr int LIHUA_LIMIT = 3;           // 万念璃花可使用次数
inline constexpr int FINAL_DUEL_ROUNDS = 3;     // 只余两人后的决战回合上限
inline constexpr Num HUNRAN_RECOVER_HP = N(6);  // 浑然一体逃脱后恢复到的血量


/* ========== 可配置规则数值 ========== */
inline constexpr int DEFAULT_INIT_HP = 20;              // 初始血量
inline constexpr double DEFAULT_CULTIVATE_STEP = 1;     // 晋升一个境界所需修为，可带小数
inline constexpr int DEFAULT_BANXIAN_THRESHOLD = 9;     // 达到该人数时随机产生一名半仙
inline constexpr int DESTROY_NEVER = 99;                // 「摧毁回合」取该值即天道永不摧毁
inline constexpr int DEFAULT_DESTROY_START_ROUND = 4;   // 天道摧毁开始生效的回合
inline constexpr int DEFAULT_ARTIFACT_CHANCE = 10;      // 仙器陨落判定概率（百分比）
inline constexpr int DEFAULT_MAX_ROUND = 30;            // 回合上限，届满以残存血量论高下
inline constexpr int ROUND_WARN_AHEAD = 5;              // 距回合上限几回合起在赛况中提醒

struct Config
{
    Num init_hp = N(DEFAULT_INIT_HP);
    Num cultivate_step = NumFrom(DEFAULT_CULTIVATE_STEP);
    int banxian_threshold = DEFAULT_BANXIAN_THRESHOLD;
    int destroy_start_round = DEFAULT_DESTROY_START_ROUND;
    int artifact_chance = DEFAULT_ARTIFACT_CHANCE;
    int max_round = DEFAULT_MAX_ROUND;
};


// 展示用的玩家编号，从 1 开始
inline std::string PlayerNo(const int pid) { return std::to_string(pid + 1) + "号"; }


/* ========== 地图 ========== */
inline constexpr int REGION_NUM = 9;
inline constexpr int CENTER_REGION = 4;         // 「五」为中央区域
inline constexpr std::string_view region_cn[REGION_NUM] = {"一", "二", "三", "四", "五", "六", "七", "八", "九"};

// 八方向：上 下 左 右 左上 右上 左下 右下
inline constexpr int DIRECT_NUM = 8;
inline constexpr std::string_view direct_cn[DIRECT_NUM] = {"上", "下", "左", "右", "左上", "右上", "左下", "右下"};
inline constexpr int k_DR_Direct[DIRECT_NUM] = {-1, 1, 0, 0, -1, -1, 1, 1};
inline constexpr int k_DC_Direct[DIRECT_NUM] = {0, 0, -1, 1, -1, 1, -1, 1};

inline constexpr int RegionRow(const int region) { return region / 3; }
inline constexpr int RegionCol(const int region) { return region % 3; }
inline constexpr int RegionAt(const int row, const int col) { return row * 3 + col; }
inline constexpr bool RegionInBound(const int row, const int col) { return row >= 0 && row < 3 && col >= 0 && col < 3; }

// 两区域是否相邻（八向相邻，同一区域不算相邻）
inline constexpr bool RegionAdjacent(const int a, const int b)
{
    if (a == b) {
        return false;
    }
    const int dr = RegionRow(a) - RegionRow(b);
    const int dc = RegionCol(a) - RegionCol(b);
    return dr >= -1 && dr <= 1 && dc >= -1 && dc <= 1;
}


/* ========== 修行方向与境界 ========== */
enum class Path {
    FA,     // 法修
    TI,     // 体修
    XIE,    // 邪修
    BAN,    // 半仙
};
inline constexpr int PATH_NUM = 4;

inline constexpr std::string_view path_cn[PATH_NUM] = {"法修", "体修", "邪修", "半仙"};
inline constexpr std::string_view path_icon[PATH_NUM] = {"🔮", "🗿", "👁", "☯"};
inline constexpr std::string_view path_desc[PATH_NUM] = {
    "最为根本纯正的修士",
    "专注体魄修炼的存在，最终目的是将身体和神魂融为一体",
    "资质较差，凭借收纳他人死去之魂或金丹元婴成就自我",
    "本是天上仙人，重伤跌入凡尘，修行本是恢复先前实力，故而进阶速度更快，上限更高",
};

inline constexpr int MAX_REALM_NUM = 6;
inline constexpr int realm_num[PATH_NUM] = {6, 6, 6, 5};
inline constexpr std::string_view realm_cn[PATH_NUM][MAX_REALM_NUM] = {
    {"炼气", "筑基", "金丹", "元婴", "化神", "合道"},
    {"炼气", "筑基", "金丹", "创道", "融魂", "合道"},
    {"炼气", "筑基", "金丹", "元婴", "化神", "合道"},
    {"炼气", "筑基", "元婴", "合道", "半仙", ""},
};

inline constexpr int PathIndex(const Path path) { return static_cast<int>(path); }
inline constexpr int RealmNum(const Path path) { return realm_num[PathIndex(path)]; }
inline constexpr int MaxRealm(const Path path) { return RealmNum(path) - 1; }
inline std::string RealmName(const Path path, const int realm) { return std::string(realm_cn[PathIndex(path)][realm]); }


/* ========== 行动 ========== */
enum class Act {
    NONE,           // 未行动

    XIULIAN,        // 聚气修行 / 捶打修行
    HUOQIU,         // 火球术
    XINGHUO,        // 漫天星火
    JINGUANG,       // 金光护身
    TUNTIAN,        // 吞天噬地
    BUDONG,         // 不动如山
    HUNRAN,         // 浑然一体
    YIMING,         // 以命祭杀
    WEISHEN,        // 伪神目
    DUOTIAN,        // 夺天造化功
    WANWU,          // 万物皆虚
    XUSHEN,         // 虚神指
    XUXING,         // 虚行寿果

    MENGJI,         // 猛击
    CUITI,          // 淬体
    TIEGU,          // 铁骨铜皮
    SIHOU,          // 嘶吼
    ZHENBAO,        // 震爆
    BAOJI,          // 暴击
    YITONG,         // 以痛止戈

    MIEHUN,         // 灭魂
    HULING,         // 护灵
    QIEJI,          // 窃机
    YINGUO,         // 因果反噬
    XUEZHEN,        // 祭灭血阵
    FENHUN,         // 分魂

    FUXIU,          // 腐朽
    XIANWEI,        // 仙之威
    XIANSHI,        // 仙之势

    LIHUA,          // [仙器] 万念璃花
    SHOUGUO,        // [仙器] 无垠寿果
    MOSHA,          // [仙器] 墨杀

    COUNT,
};
inline constexpr int ACT_NUM = static_cast<int>(Act::COUNT);

// 行动附带的参数类型
enum class ArgType {
    NONE,       // 无参数
    PLAYER,     // 同区域内的一名玩家
    PLAYER_OPT, // 同区域内的一名玩家，亦可不指定
    REGION,     // 一个区域
    DIRECT,     // 一个方向
};

// 招式类别，用于状态图中的分色与标签
enum class ActKind {
    CULTIVATE,  // 修行
    ATTACK,     // 攻伐
    DEFEND,     // 守御
    SCHEME,     // 谋略
    SECRET,     // 秘术
    RELIC,      // 仙器
};

struct ActInfo {
    std::string_view name;      // 展示名（修行类按修行方向另行取名）
    ArgType arg;                // 参数类型
    std::string_view code;      // 快捷代号，仙器一律以 Q 开头
    ActKind kind;               // 招式类别
    std::string_view brief;     // 行动简介
};

inline constexpr ActInfo act_info[ACT_NUM] = {
    /* NONE     */ {"未行动",       ArgType::NONE,   "",     ActKind::CULTIVATE, ""},
    /* XIULIAN  */ {"修行",         ArgType::NONE,   "XL",   ActKind::CULTIVATE,
                    "积攒修为以晋升境界，本回合受到的伤害达到阈值则修行失败"},
    /* HUOQIU   */ {"火球术",       ArgType::PLAYER, "HQ",   ActKind::ATTACK,
                    "指定同区域内一人，对其造成 2 点伤害"},
    /* XINGHUO  */ {"漫天星火",     ArgType::NONE,   "MT",   ActKind::ATTACK,
                    "对同区域内除自己外所有人造成 1 点伤害"},
    /* JINGUANG */ {"金光护身",     ArgType::NONE,   "JG",   ActKind::DEFEND,
                    "抵挡 4 点伤害；但金光遇虚神指即被虚化，无法抵挡；本回合对其余各式伤害仍然照挡"},
    /* TUNTIAN  */ {"吞天噬地",     ArgType::NONE,   "TT",   ActKind::SCHEME,
                    "仅当位于非中央区域再无第二人时可用：吞噬此地使其自遗界消失，获得 2 点修为；下回合必须离开"},
    /* BUDONG   */ {"不动如山",     ArgType::NONE,   "BD",   ActKind::DEFEND,
                    "抵挡 2 点伤害，溢出伤害再减半"},
    /* HUNRAN   */ {"浑然一体",     ArgType::NONE,   "HR",   ActKind::SECRET,
                    "本回合若死亡，则元婴脱体而出，回复至 6 血并现身中央区域。"
                    "元婴一途仅此一次：一经施展，浑然一体与以命祭杀此后都无法再用"},
    /* YIMING   */ {"以命祭杀",     ArgType::NONE,   "YM",   ActKind::SECRET,
                    "扣去自身 3 血，对本回合伤害过你的人各造成 5 点伤害。"
                    "元婴一途仅此一次：一经施展，以命祭杀与浑然一体此后都无法再用"},
    /* WEISHEN  */ {"伪神目",       ArgType::NONE,   "WS",   ActKind::SCHEME,
                    "私下得知全场所有人的修行方向"},
    /* DUOTIAN  */ {"夺天造化功",   ArgType::NONE,   "DT",   ActKind::SCHEME,
                    "仙器降世时，加入中央区域的仙器争夺"},
    /* WANWU    */ {"万物皆虚",     ArgType::REGION, "WW",   ActKind::ATTACK,
                    "指定一个非中央区域，湮灭该区域与其中的一切。"
                    "此乃无来源的湮灭：其中之人凭空消失，不能获得击杀分，亦无从追溯是谁出的手"},
    /* XUSHEN   */ {"虚神指",       ArgType::DIRECT, "XS",   ActKind::ATTACK,
                    "对该方向直线上的全部区域，连同自身所在区域，除自己外所有人受到 3 点群体伤害；"
                    "若整个范围内只有一人，则改为 4 点伤害"},
    /* XUXING   */ {"虚行寿果",     ArgType::NONE,   "XX",   ActKind::SECRET,
                    "本回合增加 5 血，下下回合结束时自动扣去 4 血"},
    /* MENGJI   */ {"猛击",         ArgType::PLAYER, "MJ",   ActKind::ATTACK,
                    "指定同区域内一人，对其造成 3 点伤害；你自己受到的伤害放大至 1.5 倍，同时获得 0.5 修为。"},
    /* CUITI    */ {"淬体",         ArgType::NONE,   "CT",   ActKind::CULTIVATE,
                    "扣去自身 1 血，强制获得 1 点修为，不会被打断；若另受到 2 点或以上伤害，再获得 0.5 修为"},
    /* TIEGU    */ {"铁骨铜皮",     ArgType::NONE,   "TG",   ActKind::DEFEND,
                    "抵挡 4 点伤害，同时扣去自身 1 血"},
    /* SIHOU    */ {"嘶吼",         ArgType::NONE,   "SH",   ActKind::ATTACK,
                    "对同区域内除自己外所有人造成 1.5 点伤害；每有一人被你震伤（真实扣血），增加 0.5 点修为，单次至多 3 点"},
    /* ZHENBAO  */ {"震爆",         ArgType::NONE,   "ZB",   ActKind::ATTACK,
                    "自爆金丹，对同区域所有人（含自己）造成 3 点伤害。"
                    "金丹只此一枚：一经引爆，震爆此后都无法再用"},
    /* BAOJI    */ {"暴击",         ArgType::PLAYER, "BJ",   ActKind::ATTACK,
                    "指定同区域内一人，对其造成 4 点伤害；你自己受到的伤害放大至 1.5 倍。"},
    /* YITONG   */ {"以痛止戈",     ArgType::PLAYER, "YT",   ActKind::SECRET,
                    "本回合不死不灭的免疫门槛由 5 点收紧为 3 点，随后对同区域内一人造成 5 点伤害"},
    /* MIEHUN   */ {"灭魂",         ArgType::PLAYER, "MH",   ActKind::ATTACK,
                    "指定同区域内一人，对其造成 1.5 点伤害"},
    /* HULING   */ {"护灵",         ArgType::NONE,   "HL",   ActKind::DEFEND,
                    "抵挡 2 点伤害"},
    /* QIEJI    */ {"窃机",         ArgType::NONE,   "QJ",   ActKind::SCHEME,
                    "窃取同区域内使用金丹元婴招数者的金丹与元婴"},
    /* YINGUO   */ {"因果反噬",     ArgType::NONE,   "YG",   ActKind::SCHEME,
                    "本回合本区域内，造成伤害总量最多者受到 3 点伤害并被公开播报姓名；无人出手时则区域内所有人一并承受"},
    /* XUEZHEN  */ {"祭灭血阵",     ArgType::NONE,   "JM",   ActKind::SCHEME,
                    "失去 3 血，在当前区域布置血阵并全场通报；血阵于下回合自动触发，"
                    "无论布置者是否存活，此地除各自施术者外所有人受到 5 点无来源伤害。"},
    /* FENHUN   */ {"分魂",         ArgType::NONE,   "FH",   ActKind::DEFEND,
                    "强制扣去自身 6 血，本回合无伤"},
    /* FUXIU    */ {"腐朽",         ArgType::NONE,   "FX",   ActKind::SCHEME,
                    "自本回合起，除半仙外所有人每回合固定扣去 2 血，无法防御也无法抵挡；"
                    "唯有半仙身死，腐朽方止。一局只可发动一次"},
    /* XIANWEI  */ {"仙之威",       ArgType::PLAYER, "XZW",  ActKind::ATTACK,
                    "指定一名血量为 3 及以下的修士，无视一切防御与机制将其瞬杀；"
                    "本回合服食寿果之类的回血一概不作数，亦无从逃脱"},
    /* XIANSHI  */ {"仙之势",       ArgType::NONE,   "XZS",  ActKind::DEFEND,
                    "扣除 3 血使本回合无伤，若未被攻击则额外扣去 2 血"},
    /* LIHUA    */ {"万念璃花",     ArgType::NONE,   "QH",   ActKind::RELIC,
                    "得知同区域所有人的行动，并在全场最后追加一次行动"},
    /* SHOUGUO  */ {"无垠寿果",     ArgType::NONE,   "QG",   ActKind::RELIC,
                    "服用寿果，增加 15 血"},
    /* MOSHA    */ {"墨杀",         ArgType::PLAYER_OPT, "QM", ActKind::RELIC,
                    "指定一人，对其斩下 5 点无来源伤害；不指定目标则对全场除你以外所有人各斩 3 点。"
                    "无来源之伤不为以命祭杀所反制，亦不引因果反噬之报"},
};

// act_info 依 Act 的次序逐条排列，一旦错位便会张冠李戴，故在此设卡
static_assert(act_info[static_cast<int>(Act::XIULIAN)].code == "XL");
static_assert(act_info[static_cast<int>(Act::LIHUA)].code == "QH");
static_assert(act_info[static_cast<int>(Act::SHOUGUO)].code == "QG");
static_assert(act_info[static_cast<int>(Act::MOSHA)].code == "QM");

inline constexpr const ActInfo& ActInfoOf(const Act act) { return act_info[static_cast<int>(act)]; }
inline constexpr ArgType ActArgType(const Act act) { return ActInfoOf(act).arg; }
inline constexpr ActKind ActKindOf(const Act act) { return ActInfoOf(act).kind; }
inline std::string ActCode(const Act act) { return std::string(ActInfoOf(act).code); }

inline constexpr std::string_view act_kind_cn[6] = {"修行", "攻伐", "守御", "谋略", "秘术", "仙器"};
inline constexpr std::string_view act_kind_css[6] = {"k-cult", "k-atk", "k-def", "k-sch", "k-sec", "k-relic"};
inline std::string ActKindName(const Act act) { return std::string(act_kind_cn[static_cast<int>(ActKindOf(act))]); }
inline std::string ActKindCss(const Act act) { return std::string(act_kind_css[static_cast<int>(ActKindOf(act))]); }

// 行动参数的中文提示
inline std::string ArgTypeHint(const ArgType arg)
{
    switch (arg) {
        case ArgType::PLAYER:   return "玩家编号";
        case ArgType::PLAYER_OPT: return "玩家编号<br>（可不填）";
        case ArgType::REGION:   return "区域 1-9";
        case ArgType::DIRECT:   return "方向";
        case ArgType::NONE:     return "—";
    }
    return "—";
}

// 修行类行动按修行方向取不同名称
inline std::string ActName(const Path path, const Act act)
{
    if (act == Act::XIULIAN) {
        return path == Path::TI ? "捶打修行" : "聚气修行";
    }
    return std::string(ActInfoOf(act).name);
}

// 各修行方向解锁该行动所需的境界索引，返回 -1 表示该方向没有此行动
inline constexpr int RequiredRealm(const Path path, const Act act)
{
    switch (path) {
        case Path::FA:
            switch (act) {
                case Act::XIULIAN: case Act::HUOQIU: case Act::XINGHUO: case Act::JINGUANG: return 0;
                case Act::TUNTIAN:                                                          return 1;
                case Act::BUDONG:                                                           return 2;
                case Act::HUNRAN: case Act::YIMING:                                         return 3;
                case Act::WEISHEN: case Act::DUOTIAN:                                       return 4;
                case Act::WANWU: case Act::XUSHEN: case Act::XUXING:                        return 5;
                default:                                                                    return -1;
            }
        case Path::TI:
            switch (act) {
                case Act::XIULIAN: case Act::MENGJI: case Act::CUITI:                       return 0;
                case Act::TIEGU: case Act::SIHOU:                                           return 1;
                case Act::ZHENBAO:                                                          return 2;
                case Act::BAOJI: case Act::YITONG:                                          return 5;
                default:                                                                    return -1;
            }
        case Path::XIE:
            switch (act) {
                case Act::XIULIAN: case Act::MIEHUN: case Act::HULING:                      return 0;
                case Act::QIEJI:                                                            return 1;
                case Act::YINGUO:                                                           return 2;
                case Act::XUEZHEN:                                                          return 3;
                case Act::DUOTIAN: case Act::FENHUN:                                        return 4;
                case Act::WANWU: case Act::XUSHEN: case Act::XUXING:                        return 5;
                default:                                                                    return -1;
            }
        case Path::BAN:
            switch (act) {
                case Act::XIULIAN: case Act::HUOQIU: case Act::XINGHUO: case Act::JINGUANG: return 0;
                case Act::TUNTIAN:                                                          return 1;
                case Act::HUNRAN: case Act::YIMING:                                         return 2;
                case Act::DUOTIAN: case Act::XUSHEN: case Act::XUXING:                      return 3;
                case Act::FUXIU: case Act::XIANWEI: case Act::XIANSHI:                      return 4;
                default:                                                                    return -1;
            }
    }
    return -1;
}

// 被动能力：不占用行动，随境界自动生效
inline constexpr bool HasShouhun(const Path path) { return path == Path::XIE; }          // 收魂炼灵
inline constexpr int ShouhunRealm(const Path path) { return HasShouhun(path) ? 0 : -1; }
inline constexpr int BusiRealm(const Path path) { return path == Path::TI ? 5 : -1; }    // 不死不灭
inline constexpr int HuichunRealm(const Path path) { return path == Path::TI ? 5 : -1; } // 回春

inline constexpr std::string_view PASSIVE_SHOUHUN = "收魂炼灵";
inline constexpr std::string_view PASSIVE_SHOUHUN_BRIEF =
    "本回合结束时，全场每有 1 人陨落且你仍存活，便获得 2 点修为";
inline constexpr std::string_view PASSIVE_BUSI = "不死不灭";
inline constexpr std::string_view PASSIVE_BUSI_BRIEF =
    "当血量降至 5 点及以下，若你本回合受到的伤害总和（防御结算后，不含自伤）未达到 5 点，则免疫本次致命伤；"
    "唯腐朽之伤出自天地朽坏，不朽之躯无法抵挡";
inline constexpr std::string_view PASSIVE_HUICHUN = "回春";
inline constexpr std::string_view PASSIVE_HUICHUN_BRIEF =
    "踏入该境界的当回合，血肉重焕生机，当即回复 8 血。此乃一次之缘，此后不再应验";

// 结成金丹/元婴所处的境界索引，返回 -1 表示该修行方向不存在该境界
inline constexpr int JindanRealm(const Path path) { return path == Path::BAN ? -1 : 2; }
inline constexpr int YuanyingRealm(const Path path)
{
    switch (path) {
        case Path::FA: case Path::XIE:  return 3;
        case Path::BAN:                 return 2;
        case Path::TI:                  return -1;
    }
    return -1;
}


/* ========== 伤害与修为数值 ========== */
inline constexpr Num DMG_HUOQIU = N(2);
inline constexpr Num DMG_XINGHUO = N(1);
inline constexpr Num DMG_MENGJI = N(3);
inline constexpr Num DMG_BAOJI = N(4);
inline constexpr Num DMG_MIEHUN = NUM_UNIT * 3 / 2;     // 1.5
inline constexpr Num DMG_SIHOU = NUM_UNIT * 3 / 2;      // 1.5
inline constexpr Num DMG_YITONG = N(5);
inline constexpr Num DMG_ZHENBAO = N(3);
inline constexpr Num DMG_XUSHEN = N(3);
inline constexpr Num DMG_XUSHEN_SINGLE = N(4);          // 攻击范围内仅一人时的伤害
inline constexpr Num DMG_YIMING = N(5);
inline constexpr Num DMG_YINGUO = N(3);
inline constexpr Num DMG_XUEZHEN = N(5);
inline constexpr Num DMG_FUXIU = N(2);
inline constexpr Num DMG_MOSHA = N(5);                  // 墨杀指名一人时的伤害
inline constexpr Num DMG_MOSHA_ALL = N(3);              // 墨杀不指名时遍及全场的伤害

// 电脑估量自身战力用：该招式对单个目标的基础伤害，不伤人的招式记 0
inline constexpr Num BLOCK_JINGUANG = N(4);
inline constexpr Num BLOCK_BUDONG = N(2);
inline constexpr Num BLOCK_HULING = N(2);
inline constexpr Num BLOCK_TIEGU = N(4);

inline constexpr Num COST_TIEGU = N(1);
inline constexpr Num COST_CUITI = N(1);
inline constexpr Num COST_YIMING = N(3);
inline constexpr Num COST_XUEZHEN = N(3);
inline constexpr Num COST_FENHUN = N(6);
inline constexpr Num COST_XIANSHI = N(3);
inline constexpr Num COST_XIANSHI_EXTRA = N(2);         // 本回合未被攻击时的额外扣血
inline constexpr Num HEAL_JINGJIAN = N(3);              // 水中镜挡下死劫后额外回复的血量

inline constexpr Num HEAL_XUXING = N(5);
inline constexpr Num COST_XUXING_LATER = N(4);          // 虚行寿果的延迟扣血
inline constexpr int XUXING_DELAY = 2;                  // 延迟的回合数
inline constexpr Num HEAL_SHOUGUO = N(15);

inline constexpr Num XIANWEI_HP_LIMIT = N(3);           // 仙之威可指定的最高血量
inline constexpr Num BUSI_HP_LIMIT = N(5);              // 不死不灭生效的血量上限
inline constexpr Num BUSI_DMG_LIMIT = N(5);             // 不死不灭生效的伤害上限（未达到该值）
inline constexpr Num BUSI_DMG_LIMIT_YITONG = N(3);      // 施展以痛止戈后，本回合收紧至该值

inline constexpr Num CULT_XIULIAN = N(1);               // 法修/体修/半仙 修行所得修为
inline constexpr Num CULT_XIULIAN_XIE = NUM_UNIT / 2;   // 邪修 聚气修行所得修为（0.5）
inline constexpr Num CULT_CENTER_BONUS = NUM_UNIT / 2;  // 中央区域修行成功的额外修为（0.5）
inline constexpr Num CULT_TUNTIAN = N(2);
inline constexpr Num CULT_MENGJI = NUM_UNIT / 2;        // 猛击且本回合受伤时的额外修为
inline constexpr Num CULT_SIHOU = NUM_UNIT / 2;         // 嘶吼每震伤一人所得修为（0.5）
inline constexpr Num CULT_SIHOU_LIMIT = N(3);           // 一次嘶吼至多积攒的修为
inline constexpr Num HEAL_HUICHUN = N(8);               // 回春：踏入合道当回合回复的血量
inline constexpr Num CULT_CUITI = N(1);
inline constexpr Num CULT_CUITI_BONUS = NUM_UNIT / 2;   // 淬体时额外受到 2 点及以上伤害的额外修为
inline constexpr Num CULT_QIEJI_YUANYING = N(2);        // 窃机每窃取一枚元婴
inline constexpr Num CULT_QIEJI_JINDAN = N(1);          // 窃机每窃取一枚金丹
inline constexpr Num CULT_SHOUHUN = N(2);               // 收魂炼灵每有一人死亡

inline constexpr Num XIULIAN_FAIL_DMG = N(3);           // 法修/体修/半仙 修行失败的伤害阈值
inline constexpr Num XIULIAN_FAIL_DMG_XIE = N(2);       // 邪修 聚气失败的伤害阈值
inline constexpr Num CUITI_BONUS_DMG = N(2);            // 淬体额外获得修为的伤害阈值

// 行动简介：修行类的收益与打断阈值随修行方向而异，需按方向另行组装
inline std::string ActBrief(const Path path, const Act act)
{
    if (act != Act::XIULIAN) {
        return std::string(ActInfoOf(act).brief);
    }
    const bool is_xie = (path == Path::XIE);
    std::string text = "增加 " + NumStr(is_xie ? CULT_XIULIAN_XIE : CULT_XIULIAN) + " 点修为；" +
                       "受到 " + NumStr(is_xie ? XIULIAN_FAIL_DMG_XIE : XIULIAN_FAIL_DMG) +
                       " 点或以上伤害时被打断，分毫修为也无";
    if (path != Path::TI) {
        text += "；于中央区域修行成功，额外获得 " + NumStr(CULT_CENTER_BONUS) + " 修行值";
    }
    return text;
}


/* ========== 仙器 ========== */
enum class Artifact {
    NONE,
    LIHUA,      // 万念璃花
    SHOUGUO,    // 无垠寿果
    JINGJIAN,   // 水中镜
    MOSHA,      // 墨杀仙剑
    COUNT,
};
inline constexpr int ARTIFACT_NUM = static_cast<int>(Artifact::COUNT) - 1;

inline constexpr std::string_view artifact_cn[static_cast<int>(Artifact::COUNT)] = {"无", "万念璃花", "无垠寿果", "水中镜", "墨杀仙剑"};
inline constexpr std::string_view artifact_icon[static_cast<int>(Artifact::COUNT)] = {"", "🌸", "🍑", "🪞", "🗡"};
inline constexpr std::string_view artifact_desc[static_cast<int>(Artifact::COUNT)] = {
    "",
    "得知同区域内所有人本回合的行动，并在全场最后追加一次行动。每回合限一次，使用三次后自动破碎",
    "服用寿果，增加 15 血。只可使用一次",
    "当本回合受到的伤害结算后足以致命，则直接免去此次伤害，并额外回复 3 血。使用一次后自动破碎",
    "指定一人，对其斩下 5 点无来源伤害；不指定则对全场除你以外所有人各斩 3 点。无来源之伤不为以命祭杀所反制，亦不引因果反噬之报",
};

// 本局仙器事件的推进流程
enum class ArtifactState {
    NONE,       // 尚未有人掌握夺天造化功
    QUAKE,      // 天地大震，每回合判定仙器是否陨落
    DESCENDED,  // 判定成功，下回合起仙器可被争夺
    SETTLED,    // 本局仙器事件已结束
};


/* ========== 结算相关 ========== */
// 伤害来源类型，用于判定以命祭杀的反制目标与防御的适用范围
enum class DmgSource {
    ATTACK,     // 由某位修士造成，可作为以命祭杀的反制目标
    ARRAY,      // 血阵造成的无来源伤害
    RELIC,      // 仙器造成的无来源伤害，不计入伤害归因，故无从反制亦无从报应
    FUXIU,      // 腐朽造成的固定伤害，无法防御且不计入防御结算
    SELF,       // 自身行动的代价，不受自身防御与免伤影响
};

// 本回合每名玩家的结算结果，用于播报与上色
enum class RoundResult {
    NONE,       // 无事发生
    DAMAGED,    // 受到伤害
    DEFENDED,   // 触发防御或免伤
    PROMOTED,   // 境界晋升
    ESCAPED,    // 浑然一体逃脱
    DEAD,       // 本回合死亡
};
