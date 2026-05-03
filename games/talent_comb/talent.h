// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).
//
// Talent enum + SpecialEvent enum + lightweight helper declarations.

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

// 枚举顺序：A 级在前，B 级在后。
// 新增/调整天赋时：先新增枚举值，再在 talent_classes.h 增加对应类和工厂分支。
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
    零号位,         // 原名：0号位
    坦诚相见,
    星河流转,
    冥想,
    光波干涉,
    九转玄机,
    乾坤大挪移,
    关键选择,

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
    贪婪宝藏,
    零的力量,       // 原名：0的力量
    虚空之心,
    表演型人格,
    二环里,
    恭喜栗子,
    勃勃生机,
    一方通行,

    COUNT
};

struct TalentInfo
{
    const char* grade;
    Talent      id;
    const char* name;
    const char* description;
};

const TalentInfo& GetTalentInfo(Talent t);
std::string TalentName(Talent t);
std::string TalentDescription(Talent t);
std::string_view TalentGrade(Talent t);
bool IsGrade(Talent t, std::string_view grade);
bool IsGradeA(Talent t);
bool IsGradeB(Talent t);
const std::vector<Talent>& TalentsOfGrade(std::string_view grade);
const std::vector<Talent>& GradeATalents();
const std::vector<Talent>& GradeBTalents();
std::map<std::string, int> MakeTalentOptionMap();
std::map<std::string, int> MakeGradeBTalentOptionMap();

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

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot
