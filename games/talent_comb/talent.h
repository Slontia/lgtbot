// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).
//
// Talent enum + TalentInfo table + grade helpers; SpecialEvent enum + helpers.
//
// Design notes:
//   * `Talent` 枚举顺序必须与 `k_talent_info` 中 id 的顺序严格一致
//     （A 级天赋在前，B 级天赋在后；新增天赋时务必保持同一顺序）。
//   * 天赋等级用字符串存储（便于后续扩展 "S" / "C" 等），
//     通过 IsGrade / IsGradeA / IsGradeB / TalentGrade 访问。
//   * options.h 中的下拉选项（"事件" / "天赋"）由本文件提供的
//     MakeSpecialEventOptionMap / MakeTalentOptionMap 在运行时生成，
//     后续新增枚举值时无需手动同步 options.h。

#pragma once

#include <cstring>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

// ==================== Special Event ====================

enum class SpecialEvent
{
    无 = 0,           // 无
    大的要来了,        // 卡池中没有1
    两极分化,          // 卡池中没有5
    大的没了,          // 卡池中没有9
    天降恩泽,          // 第一轮每人发一个癞子
    调色盘,            // 卡池中添加大量癞子
    有1吗,             // 每行1额外加12分
    小透不算挂,        // 提前公布下一轮的卡
    愚人节,            // ？？？：天降恩泽+调色盘+有1吗+小透不算挂
    COUNT
};

static std::string SpecialEventShortName(SpecialEvent event)
{
    switch (event) {
        case SpecialEvent::无: return "无";
        case SpecialEvent::大的要来了: return "大的要来了";
        case SpecialEvent::两极分化: return "两极分化";
        case SpecialEvent::大的没了: return "大的没了";
        case SpecialEvent::天降恩泽: return "天降恩泽";
        case SpecialEvent::调色盘: return "调色盘";
        case SpecialEvent::有1吗: return "有1吗";
        case SpecialEvent::小透不算挂: return "小透不算挂";
        case SpecialEvent::愚人节: return "？？？";
        default: return "未知";
    }
}

static std::string SpecialEventName(SpecialEvent event)
{
    switch (event) {
        case SpecialEvent::无: return "无";
        case SpecialEvent::大的要来了: return "大的要来了——卡池中没有1";
        case SpecialEvent::两极分化: return "两极分化——卡池中没有5";
        case SpecialEvent::大的没了: return "大的没了——卡池中没有9";
        case SpecialEvent::天降恩泽: return "天降恩泽——第一轮每人发一个癞子";
        case SpecialEvent::调色盘: return "调色盘——卡池中添加大量癞子";
        case SpecialEvent::有1吗: return "有1吗——每行1额外加12分";
        case SpecialEvent::小透不算挂: return "小透不算挂——提前公布下一轮的卡";
        case SpecialEvent::愚人节: return "？？？\n\n2023年愚人节纪念事件：\n天降恩泽、调色盘、有1吗、小透不算挂";
        default: return "未知";
    }
}

static bool HasWindfall(SpecialEvent event) { return event == SpecialEvent::天降恩泽 || event == SpecialEvent::愚人节; }
static bool HasColorful(SpecialEvent event) { return event == SpecialEvent::调色盘 || event == SpecialEvent::愚人节; }
static bool HasValuableOne(SpecialEvent event) { return event == SpecialEvent::有1吗 || event == SpecialEvent::愚人节; }
static bool HasForesee(SpecialEvent event) { return event == SpecialEvent::小透不算挂 || event == SpecialEvent::愚人节; }

// ==================== Talent System ====================

// 枚举顺序 = k_talent_info 顺序（A 级在前，B 级在后）。
// 新增/调整天赋时：先改 k_talent_info，再把枚举值按同样顺序调整即可。
// 约束：枚举名不能以数字开头，故 0号位 → 零号位。
enum class Talent
{
    // ========== A 级天赋 ==========
    完美块,
    绝地反击,
    占得先机,
    零风险投资,
    钢铁之躯,
    以退为进,
    致命魔术,
    三相之力,
    紧急救援,
    我全都要,
    潘多拉魔盒,
    利滚利,
    戴森球,
    零号位,        // 原名 0号位
    坦诚相见,
    星河流转,
    冥想,
    光波干涉,

    // ========== B 级天赋 ==========
    嗜血,
    还是有用的,
    快攻,
    特立独行,
    成双成对,
    垃圾回收,
    来点实在的,
    攻击形态,
    防御形态,
    局部强化,
    有舍有得,
    三年之期,
    锻造,
    尾货处理,
    图灵测试,
    事不过三,
    摇奖机,
    两级反转,
    败者之刃,
    临时用品,
    包扎,
    百味草,
    天使轮,
    劫掠,
    多维抉择,
    张三来袭,

    COUNT
};

// Unified talent descriptor: grade, id, name, description — indexed by Talent enum value.
// Entries MUST be declared in the same order as the Talent enum.
struct TalentInfo
{
    const char* grade;          // "A" / "B" / ... 后续可扩展更多等级
    Talent      id;
    const char* name;
    const char* description;
};

static const TalentInfo k_talent_info[] = {
    // ========== A 级天赋 ==========
    { "A", Talent::完美块,      "完美块",       "你的312视为万能牌" },
    { "A", Talent::绝地反击,    "绝地反击",     "受到致命伤害时，血量降为1，下回合作战时分数短暂提升15%" },
    { "A", Talent::占得先机,    "占得先机",     "选牌阶段优先选牌" },
    { "A", Talent::零风险投资,  "零风险投资",   "你的分数不会降低" },
    { "A", Talent::钢铁之躯,    "钢铁之躯",     "你受到的伤害降低30%" },
    { "A", Talent::以退为进,    "以退为进",     "你的7均视为6，4均视为3" },
    { "A", Talent::致命魔术,    "致命魔术",     "造成伤害时有15%概率造成额外100%伤害" },
    { "A", Talent::三相之力,    "三相之力",     "你的下三张非选牌阶段的牌依次获得左/中/右单线癞子" },
    { "A", Talent::紧急救援,    "紧急救援",     "跳过下一轮对战，立刻启动选牌阶段，你最先选择" },
    { "A", Talent::我全都要,    "我全都要",     "下次选择天赋时获得全部" },
    { "A", Talent::潘多拉魔盒,  "潘多拉魔盒",   "随机获得两个B级天赋" },
    { "A", Talent::利滚利,      "利滚利",       "你每有100分，每回合加1分" },
    { "A", Talent::戴森球,      "戴森球",       "若6条长度3的连线均完成，则分数+6%" },
    { "A", Talent::零号位,      "0号位",        "弃牌时，获得弃牌三个方向数字之和的临时分（持续1回合，可叠加）" },
    { "A", Talent::坦诚相见,    "坦诚相见",     "对战时仅比较双方盘面的基础连线分，无视双方全部额外分数加成" },
    { "A", Talent::星河流转,    "星河流转",     "位于2、4、7、13、16、18号位置的砖块获得长度3连线方向的单线癞子" },
    { "A", Talent::冥想,        "冥想",         "每回合获得5点生命值，直到单次连线获得25分及以上的分数" },
    { "A", Talent::光波干涉,    "光波干涉",     "如果2条连线上的数字相同且连线长度也相同，这些连线获得15%额外分数" },

    // ========== B 级天赋 ==========
    { "B", Talent::嗜血,        "嗜血",         "战斗获胜时，生命值+2" },
    { "B", Talent::还是有用的,  "还是有用的",     "你的每一行1分数额外加3，每一行2分数额外加6" },
    { "B", Talent::快攻,        "快攻",         "战斗获胜时，对对手造成伤害+6" },
    { "B", Talent::特立独行,    "特立独行",     "分数为奇数时，战斗时分数短暂提升6" },
    { "B", Talent::成双成对,    "成双成对",     "分数为偶数时，战斗时分数短暂提升6" },
    { "B", Talent::垃圾回收,    "垃圾回收",     "每次弃牌时获得2分" },
    { "B", Talent::来点实在的,  "来点实在的",   "立刻获得4分" },
    { "B", Talent::攻击形态,    "攻击形态",     "造成的伤害增加15%，受到的伤害增加5%" },
    { "B", Talent::防御形态,    "防御形态",     "受到的伤害减少15%，造成的伤害减少5%" },
    { "B", Talent::局部强化,    "局部强化",     "10号位每个方向的数字完成一条匹配连线+3分（每方向仅一次，癞子可匹配任意数字，上限9分）" },
    { "B", Talent::有舍有得,    "有舍有得",     "每战败4次，随机获得一枚额外的砖块" },
    { "B", Talent::三年之期,    "三年之期",     "把接下来三个回合获得的砖块存起来，第四个回合时一次性摆放，并恢复存储期间受到的所有伤害" },
    { "B", Talent::锻造,        "锻造",         "弃牌时，按顺序获得砖块三个方向数字的碎片，集齐三个方向的碎片合成一枚砖块放置" },
    { "B", Talent::尾货处理,    "尾货处理",     "选牌时，如果你最后一个选，则可以获得剩下两个砖块" },
    { "B", Talent::图灵测试,    "图灵测试",     "对战镜像战败也不会受到伤害" },
    { "B", Talent::事不过三,    "事不过三",     "你第4次战败时，免疫此次伤害" },
    { "B", Talent::摇奖机,      "摇奖机",       "随机获得一个A级天赋" },
    { "B", Talent::两级反转,    "两级反转",     "你的1视为9，你的9视为1" },
    { "B", Talent::败者之刃,    "败者之刃",     "你战败后获得4分临时分（可累积），在战胜一次后清除" },
    { "B", Talent::临时用品,    "临时用品",     "获得一个仅三回合可用的癞子" },
    { "B", Talent::包扎,        "包扎",         "立即获得20点生命" },
    { "B", Talent::百味草,      "百味草",       "获得1层中毒，每因中毒损失1点生命，获得1分" },
    { "B", Talent::天使轮,      "天使轮",       "获得15点临时分，直到下次完成一次连线为止" },
    { "B", Talent::劫掠,        "劫掠",         "击败玩家后，从他的盘面上随机挑选5个砖块，你选择其中一枚放置" },
    { "B", Talent::多维抉择,    "多维抉择",     "天赋选择时，候选中额外提供1个A级与1个B级天赋" },
    { "B", Talent::张三来袭,    "张三来袭",     "你每放置一张3，回复3血" },
};

static_assert(sizeof(k_talent_info) / sizeof(k_talent_info[0]) == static_cast<size_t>(Talent::COUNT),
              "k_talent_info entries must match Talent::COUNT");

inline const TalentInfo& GetTalentInfo(Talent t)
{
    return k_talent_info[static_cast<size_t>(t)];
}

inline std::string TalentName(Talent t)
{
    const auto idx = static_cast<size_t>(t);
    return (idx < static_cast<size_t>(Talent::COUNT)) ? std::string(k_talent_info[idx].name) : std::string("未知");
}

inline std::string TalentDescription(Talent t)
{
    const auto idx = static_cast<size_t>(t);
    return (idx < static_cast<size_t>(Talent::COUNT)) ? std::string(k_talent_info[idx].description) : std::string("");
}

// 获取天赋等级字符串（"A" / "B" / ...）。越界时返回 ""。
inline std::string_view TalentGrade(Talent t)
{
    const auto idx = static_cast<size_t>(t);
    return (idx < static_cast<size_t>(Talent::COUNT)) ? std::string_view(k_talent_info[idx].grade) : std::string_view("");
}

// 判断天赋是否为指定等级。
inline bool IsGrade(Talent t, std::string_view grade)
{
    return TalentGrade(t) == grade;
}

inline bool IsGradeA(Talent t) { return IsGrade(t, "A"); }
inline bool IsGradeB(Talent t) { return IsGrade(t, "B"); }

// 按等级筛选天赋列表（惰性构建、缓存）。
inline const std::vector<Talent>& TalentsOfGrade(std::string_view grade)
{
    // 使用 static map 缓存每个等级的列表。
    static std::map<std::string, std::vector<Talent>> cache;
    auto it = cache.find(std::string(grade));
    if (it != cache.end()) return it->second;
    std::vector<Talent> r;
    for (const auto& info : k_talent_info) {
        if (std::string_view(info.grade) == grade) r.push_back(info.id);
    }
    return cache.emplace(std::string(grade), std::move(r)).first->second;
}

inline const std::vector<Talent>& GradeATalents() { return TalentsOfGrade("A"); }
inline const std::vector<Talent>& GradeBTalents() { return TalentsOfGrade("B"); }

// Backward-compatible aliases (used in several call sites).
static const std::vector<Talent>& k_grade_a_talents = GradeATalents();
static const std::vector<Talent>& k_grade_b_talents = GradeBTalents();

// ==================== Option Map Builders ====================
// 由 options.h 调用，根据枚举自动生成下拉选项的 {名称 -> 整数值} 映射。
// 新增枚举值时只需维护枚举本身，options.h 无需修改。

// 事件选项：保留"随机"=0 的特殊值，其余 = enum 值 + 1。
// 解析端（mygame.cc）用 `static_cast<SpecialEvent>(event_opt - 1)` 反查。
inline std::map<std::string, int> MakeSpecialEventOptionMap()
{
    std::map<std::string, int> m;
    m.emplace("随机", 0);
    for (int i = 0; i < static_cast<int>(SpecialEvent::COUNT); ++i) {
        m.emplace(SpecialEventShortName(static_cast<SpecialEvent>(i)), i + 1);
    }
    return m;
}

// 天赋选项：名称 -> enum 值（与 static_cast<Talent>(int) 直接对应）。
inline std::map<std::string, int> MakeTalentOptionMap()
{
    std::map<std::string, int> m;
    for (const auto& info : k_talent_info) {
        m.emplace(info.name, static_cast<int>(info.id));
    }
    return m;
}

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot
