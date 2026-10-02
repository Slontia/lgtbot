
#pragma once

#include <algorithm>
#include <random>
#include <string>
#include <vector>


// 一座待触发的血祭大阵
struct BloodArray
{
    lgtbot::PlayerID caster;    // 施术者，触发时对其豁免
    int region;         // 所在区域
};


class Board
{
  public:
    Board() : g(std::random_device{}()) {}

    /* ===== 全局状态 ===== */
    // 本局生效的规则数值，来自配置项
    Config cfg;
    // 所有玩家，下标即 lgtbot::PlayerID
    std::vector<Player> players;
    // 各区域是否已被摧毁，中央区域永远为 false
    std::vector<bool> destroyed = std::vector<bool>(REGION_NUM, false);
    // 下回合将自动触发的血阵
    std::vector<BloodArray> arrays_active;
    // 本回合新布置、将于下回合触发的血阵
    std::vector<BloodArray> arrays_pending;

    // 腐朽是否已发动，以及发动者
    bool fuxiu_active = false;
    int fuxiu_caster = -1;

    /* ===== 仙器 ===== */
    ArtifactState artifact_state = ArtifactState::NONE;
    // 已公布的仙器种类
    Artifact falling_artifact = Artifact::NONE;
    // 仙器的持有者，未被持有时为 -1
    int artifact_owner = -1;

    // 随机数发生器
    std::mt19937 g;

    /* ===== 玩家查询 ===== */
    int PlayerNum() const { return static_cast<int>(players.size()); }
    Player& P(const int pid) { return players[pid]; }
    const Player& P(const int pid) const { return players[pid]; }

    int AliveCount() const
    {
        return static_cast<int>(std::count_if(players.begin(), players.end(),
                [](const Player& p) { return p.Alive(); }));
    }

    // 区域内的存活玩家
    std::vector<lgtbot::PlayerID> AliveInRegion(const int region) const
    {
        std::vector<lgtbot::PlayerID> list;
        for (const Player& p : players) {
            if (p.Alive() && p.region == region) {
                list.push_back(p.pid);
            }
        }
        return list;
    }

    // 区域内除指定玩家外是否还有其他存活玩家
    bool RegionEmptyExcept(const int region, const lgtbot::PlayerID pid) const
    {
        for (const Player& p : players) {
            if (p.Alive() && p.region == region && p.pid != pid) {
                return false;
            }
        }
        return true;
    }

    // 区域内是否没有任何存活玩家
    bool RegionEmpty(const int region) const
    {
        for (const Player& p : players) {
            if (p.Alive() && p.region == region) {
                return false;
            }
        }
        return true;
    }

    /* ===== 地图查询 ===== */
    bool Destroyed(const int region) const { return destroyed[region]; }

    // 摧毁一个区域，中央区域不会被摧毁。此地既已不存于世，其上的血阵一并烟消云散
    // 返回随之消散的血阵数量
    int DestroyRegion(const int region)
    {
        if (region == CENTER_REGION) {
            return 0;
        }
        destroyed[region] = true;
        const auto here = [region](const BloodArray& a) { return a.region == region; };
        const int gone = static_cast<int>(std::count_if(arrays_active.begin(), arrays_active.end(), here)) +
                         static_cast<int>(std::count_if(arrays_pending.begin(), arrays_pending.end(), here));
        std::erase_if(arrays_active, here);
        std::erase_if(arrays_pending, here);
        return gone;
    }

    // 除中央区域外的八个区域是否已尽数湮灭
    bool OnlyCenterLeft() const
    {
        for (int r = 0; r < REGION_NUM; r++) {
            if (r != CENTER_REGION && !destroyed[r]) {
                return false;
            }
        }
        return true;
    }

    // 玩家能否移动到目标区域：可停留原地，或前往未被摧毁的相邻区域
    bool CanMoveTo(const Player& player, const int target) const
    {
        if (target < 0 || target >= REGION_NUM) {
            return false;
        }
        if (target == player.region) {
            return !player.must_leave;
        }
        return !destroyed[target] && RegionAdjacent(player.region, target);
    }

    // 玩家当前所有可选的移动目标
    std::vector<int> MovableRegions(const Player& player) const
    {
        std::vector<int> list;
        for (int r = 0; r < REGION_NUM; r++) {
            if (CanMoveTo(player, r)) {
                list.push_back(r);
            }
        }
        return list;
    }

    // 虚神指的波及区域：自身区域 + 指定方向直线上的 0 至 2 个区域，已摧毁区域不阻挡
    std::vector<int> XushenRegions(const int from, const int direct) const
    {
        std::vector<int> list{from};
        int row = RegionRow(from);
        int col = RegionCol(from);
        for (int step = 0; step < 2; step++) {
            row += k_DR_Direct[direct];
            col += k_DC_Direct[direct];
            if (!RegionInBound(row, col)) {
                break;
            }
            list.push_back(RegionAt(row, col));
        }
        return list;
    }

    // 当前所有无人的普通区域，供天道摧毁抽取
    std::vector<int> DestroyableRegions() const
    {
        std::vector<int> list;
        for (int r = 0; r < REGION_NUM; r++) {
            if (r != CENTER_REGION && !destroyed[r] && RegionEmpty(r)) {
                list.push_back(r);
            }
        }
        return list;
    }

    /* ===== 血阵查询 ===== */
    // 指定区域内待触发的血阵数量
    int ArrayCount(const int region) const
    {
        return static_cast<int>(std::count_if(arrays_active.begin(), arrays_active.end(),
                [region](const BloodArray& a) { return a.region == region; }));
    }

    // 存在血阵的所有区域
    std::vector<int> ArrayRegions() const
    {
        std::vector<int> list;
        for (const BloodArray& a : arrays_active) {
            if (std::find(list.begin(), list.end(), a.region) == list.end()) {
                list.push_back(a.region);
            }
        }
        std::sort(list.begin(), list.end());
        return list;
    }

    // 将本回合布置的血阵转为下回合生效
    void ActivatePendingArrays()
    {
        arrays_active = arrays_pending;
        arrays_pending.clear();
    }

    /* ===== 位置快照 ===== */
    // 定位阶段开始时保存位置快照，此后赛况一律读取快照，直到该阶段结束
    void FreezeRegion()
    {
        frozen_region.resize(PlayerNum());
        for (int i = 0; i < PlayerNum(); i++) {
            frozen_region[i] = players[i].region;
        }
        freeze_region = true;
    }
    void UnfreezeRegion() { freeze_region = false; }
    // 赛况中展示的位置：定位阶段进行中时取阶段开始时的快照
    int ShownRegion(const int pid) const { return freeze_region ? frozen_region[pid] : players[pid].region; }

    /* ===== 随机 ===== */
    int RandomInt(const int n)
    {
        std::uniform_int_distribution<int> dist(0, n - 1);
        return dist(g);
    }

    template <typename T>
    T RandomPick(const std::vector<T>& list) { return list[RandomInt(static_cast<int>(list.size()))]; }

    /* ===== 界面绘制 ===== */
    // 遗界全景：九宫格棋盘与修士名录
    std::string BoardHtml(const std::string& phase, const int round, const int duel_deadline) const;
    // 私信状态图：身份牌 + 招式表 + 随身仙器
    std::string MyStatusHtml(const int pid, const int round) const;
    // 终局名录
    std::string ScoreHtml() const;
    // 四条修行路线的完整招式图鉴，与对局状态无关
    static std::string AtlasHtml();
    // 伪神目：私下呈上全场存活修士的道途名录
    std::string WeishenHtml() const;
    // 万念璃花：私下呈上本区域众人此回合的行动
    std::string LihuaHtml(const int holder) const;

    static std::string PanelStyle();
    static int ImageWidth() { return 640; }
    // 图鉴宽度
    static int AtlasWidth() { return 1180; }

  private:
    // 定位阶段冻结的位置快照，避免通过赛况提前窥知他人去向
    bool freeze_region = false;
    std::vector<int> frozen_region;
};


/* ========== 技能层前置声明：定义见 skill.h ========== */
std::vector<Act> AvailableActs(const Board& board, const Player& player);
std::string ActDesc(const Player& player, const Action& action);


/* ========== 界面绘制 ========== */

// 转义昵称中的 HTML 特殊字符，避免破坏表格结构
inline std::string EscHtml(const std::string& text)
{
    std::string out;
    for (const char c : text) {
        switch (c) {
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '&': out += "&amp;"; break;
            case '"': out += "&quot;"; break;
            default: out += c;
        }
    }
    return out;
}

// 昵称栏：头像与昵称分列，昵称过长时以省略号截断且不换行
inline std::string NameCell(const std::string& avatar, const std::string& name)
{
    return "<td class='pname'><span class='av'>" + avatar + "</span><span class='nm'>" +
           EscHtml(name) + "</span></td>";
}

// 区块内一行玩家的排版规格：人少时尽量放大，人多到一格装不下才逐级缩小，超过 9 人改为双列
struct CellLayout
{
    int font;       // 文字字号
    int line;       // 行高
    int avatar;     // 头像边长
    int badge;      // 头像上的序号角标字号
    bool dense;     // 是否双列排布
};

inline CellLayout CellLayoutOf(const int count)
{
    static constexpr CellLayout tiers[] = {
        {17, 27, 22, 11, false},    // 1 - 3 人
        {16, 25, 20, 11, false},    // 4 人
        {15, 23, 19, 10, false},    // 5 人
        {14, 21, 17, 10, false},    // 6 人
        {13, 19, 16,  9, false},    // 7 人
        {13, 17, 15,  9, false},    // 8 人
        {12, 15, 13,  8, false},    // 9 人
    };
    static constexpr int tier_max[] = {3, 4, 5, 6, 7, 8, 9};
    for (int i = 0; i < static_cast<int>(sizeof(tier_max) / sizeof(tier_max[0])); i++) {
        if (count <= tier_max[i]) {
            return tiers[i];
        }
    }
    return {11, 15, 13, 8, true};
}

// 玩家头像与其右下角的黄色序号角标
inline std::string AvatarBadge(const std::string& avatar, const int pid, const CellLayout& layout)
{
    const int badge_box = layout.badge + 3;
    return "<span class='pav' style='width:" + std::to_string(layout.avatar) + "px; height:" +
           std::to_string(layout.avatar) + "px'>" + avatar + "<i class='pno' style='font-size:" +
           std::to_string(layout.badge) + "px; min-width:" + std::to_string(badge_box) + "px; height:" +
           std::to_string(badge_box) + "px; line-height:" + std::to_string(badge_box) + "px'>" +
           std::to_string(pid + 1) + "</i></span>";
}

// 血量与修为的进度条：进度条与数值分列固定宽度，保证多行之间纵向对齐
inline std::string BarCell(const std::string& cls, const int percent, const std::string& value)
{
    const int width = percent < 0 ? 0 : (percent > 100 ? 100 : percent);
    return "<span class='barwrap'><span class='bar " + cls + "'><i style='width:" + std::to_string(width) +
           "%'></i></span></span><span class='numv'>" + value + "</span>";
}

// 仙器对应的主动招式；水中镜为纯被动，返回 Act::NONE
inline Act RelicAct(const Artifact relic)
{
    switch (relic) {
        case Artifact::LIHUA:   return Act::LIHUA;
        case Artifact::SHOUGUO: return Act::SHOUGUO;
        case Artifact::MOSHA:   return Act::MOSHA;
        default:                return Act::NONE;
    }
}

// 身份牌内的一条被动能力：名称、生效状态与说明
inline std::string PassiveRow(const std::string_view name, const std::string& tag, const bool active,
                              const std::string_view brief, const bool first)
{
    return "<div class='prow" + std::string(first ? " top" : "") + "'>"
           "<span class='plab'>被动</span><span class='pn'>" + std::string(name) + "</span>"
           "<span class='ptag" + std::string(active ? "" : " off") + "'>" + tag + "</span>"
           "<div class='pd'>" + std::string(brief) + "</div></div>";
}

inline std::string Board::PanelStyle()
{
    return R"(
<style>
body { margin: 0; background: #efe7d6; }
.xr {
    width: 584px;
    padding: 16px 18px 18px 18px;
    background:
        radial-gradient(circle at 16% 10%, rgba(96,88,72,0.10) 0%, rgba(96,88,72,0) 40%),
        radial-gradient(circle at 84% 82%, rgba(96,88,72,0.09) 0%, rgba(96,88,72,0) 44%),
        linear-gradient(155deg, #fbf6ea 0%, #f0e8d6 100%);
    border: 1px solid #b6a98f;
    box-shadow: inset 0 0 0 4px #f7f2e6, inset 0 0 0 5px #cec2a8;
    color: #2f2b26;
    font-size: 14px;
}
/* 图鉴两路并列，需要更宽的版心 */
.xr.wide { width: 1140px; }
.xr .title {
    text-align: center;
    font-size: 26px;
    font-weight: bold;
    letter-spacing: 10px;
    color: #33302a;
    text-shadow: 0 1px 0 rgba(255,255,255,0.85);
}
.xr .rule {
    height: 1px;
    margin: 7px 0 5px 0;
    background: linear-gradient(90deg, rgba(120,110,92,0) 0%, #8d8270 50%, rgba(120,110,92,0) 100%);
}
.xr .sub { text-align: center; color: #7a7263; letter-spacing: 2px; margin-bottom: 11px; }
.xr .notice {
    margin: 0 0 11px 0;
    padding: 6px 10px;
    font-size: 13px;
    color: #55483a;
    border-left: 3px solid #a8322d;
    background: rgba(168,50,45,0.07);
}
.xr table.map { width: 100%; border-collapse: separate; border-spacing: 7px; table-layout: fixed; }
.xr table.map td {
    width: 185px;
    height: 185px;
    box-sizing: border-box;
    vertical-align: top;
    padding: 6px 8px;
    border: 1px solid #b0a289;
    background: linear-gradient(160deg, #fdfaf1 0%, #f6efdf 100%);
}
.xr table.map td.center {
    border: 2px solid #9c6b2f;
    background: linear-gradient(160deg, #fdf6e2 0%, #f1e5c4 100%);
}
.xr table.map td.gone {
    border-color: #bcb3a2;
    background: repeating-linear-gradient(45deg, #e2dac8 0px, #e2dac8 6px, #d9d0bd 6px, #d9d0bd 12px);
}
.xr .rhead { display: block; margin-bottom: 3px; }
.xr .rname { font-size: 21px; font-weight: bold; letter-spacing: 2px; color: #33302a; }
.xr td.gone .rname { color: #8c8474; }
.xr .tag {
    display: inline-block;
    margin-left: 6px;
    padding: 1px 7px;
    font-size: 14px;
    border-radius: 3px;
    vertical-align: 3px;
}
.xr .tag.center { background: #9c6b2f; color: #fff7e4; }
.xr .tag.array { background: #a8322d; color: #fdf1ec; }
.xr .tag.relic { background: #4a6741; color: #f2f7ee; }
.xr .tag.gone { background: #bcb3a2; color: #f6f2e8; }
.xr .pl { white-space: nowrap; overflow: hidden; color: #33302a; }
.xr .pl.dense { display: inline-block; width: 50%; }
.xr .pav { position: relative; display: inline-block; vertical-align: middle; line-height: 0; margin-right: 10px; }
.xr .pav img {
    width: 100% !important;
    height: 100% !important;
    border-radius: 50% !important;
    vertical-align: top !important;
    border: 1px solid #b8a98c !important;
    box-sizing: border-box !important;
}
.xr .pno {
    position: absolute;
    right: -6px;
    bottom: -4px;
    font-style: normal;
    font-weight: bold;
    text-align: center;
    color: #4a3600;
    background: linear-gradient(160deg, #ffe89a, #f0c02e);
    border: 1px solid #a8801a;
    border-radius: 8px;
}
.xr .pl .rl { color: #3c5a4a; }
.xr .pl .hp { color: #a8322d; font-weight: bold; }
.xr td.gone .pl { color: #6e6659; }
.xr .pl .relic {
    display: inline-block;
    margin-left: 4px;
    padding: 0 3px;
    font-size: 12px;
    border-radius: 4px;
    background: #efe0c4;
    border: 1px solid #c2ac74;
}
.xr .empty { margin-top: 10px; font-size: 18px; line-height: 26px; color: #a8a08e; }
.xr table.roster {
    width: 100%;
    table-layout: fixed;
    border-collapse: collapse;
    margin-top: 12px;
    font-size: 13px;
}
.xr table.roster th, .xr table.roster td {
    box-sizing: border-box;
    white-space: nowrap;
    overflow: hidden;
}
.xr table.roster th {
    padding: 3px 4px;
    border: 1px solid #b0a289;
    background: #e7ddc7;
    color: #4a4033;
}
.xr table.roster td {
    padding: 2px 4px;
    border: 1px solid #c8bda6;
    background: #fcf9f1;
    text-align: center;
    color: #33302a;
}
.xr table.roster td.pname { text-align: left; white-space: nowrap; overflow: hidden; }
.xr table.roster td.pname .av { display: inline-block; width: 26px; vertical-align: middle; }
.xr table.roster td.pname .nm {
    display: inline-block;
    max-width: 196px;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
    vertical-align: middle;
}
.xr table.roster tr.dead td { color: #9a9282; background: #ece5d5; }
.xr table.roster tr.win td { background: #f8ead0; }
.xr .barwrap { display: inline-block; width: 58px; vertical-align: middle; }
.xr .numv { display: inline-block; width: 38px; text-align: right; vertical-align: middle; }
.xr .bar {
    display: block;
    width: 58px;
    height: 9px;
    box-sizing: border-box;
    border: 1px solid #b0a289;
    background: #efe8d6;
}
.xr .bar i { display: block; height: 100%; }
.xr .bar.hp i { background: linear-gradient(90deg, #a8322d 0%, #d4795c 100%); }
.xr .bar.cult i { background: linear-gradient(90deg, #35576b 0%, #6d9fb4 100%); }
.xr .rk {
    display: inline-block;
    width: 22px;
    height: 22px;
    line-height: 22px;
    border-radius: 50%;
    background: #e0d6c0;
    color: #4a4033;
    font-size: 12px;
}
.xr .rk.first { background: #a8322d; color: #fdf3e6; font-weight: bold; }
.xr .foot { margin-top: 9px; font-size: 12px; color: #7a7263; text-align: center; }
.xr .idcard {
    margin-bottom: 12px;
    padding: 10px 12px;
    border: 1px solid #b6a98f;
    background: linear-gradient(160deg, #fdf9ef 0%, #f4ecd9 100%);
}
.xr .idcard .who { font-size: 19px; font-weight: bold; color: #33302a; }
.xr .idcard .path { margin-left: 10px; font-size: 17px; color: #7a4b1e; }
.xr table.attr { width: 100%; border-collapse: collapse; margin-top: 8px; font-size: 14px; }
.xr table.attr td { padding: 3px 2px; border: none; background: transparent; text-align: left; white-space: nowrap; }
.xr table.attr td.lab { width: 62px; color: #7a7263; }
.xr .pas { margin-top: 9px; padding-top: 8px; border-top: 1px dashed #c3b69b; }
.xr .pas .prow { margin-top: 7px; }
.xr .pas .prow.top { margin-top: 0; }
.xr .pas .plab {
    display: inline-block;
    padding: 0 5px;
    margin-right: 6px;
    font-size: 11px;
    border-radius: 2px;
    color: #fdf7ec;
    background: #6f6553;
}
.xr .pas .pn { font-size: 14px; font-weight: bold; color: #8a6a12; }
.xr .pas .ptag {
    display: inline-block;
    margin-left: 6px;
    padding: 0 5px;
    font-size: 11px;
    border-radius: 2px;
    color: #fdf7ec;
    background: #8a6a12;
}
.xr .pas .ptag.off { background: #a09479; }
.xr .pas .pd { margin-top: 2px; font-size: 13px; color: #55483a; line-height: 19px; }
.xr .sect {
    margin: 12px 0 6px 0;
    padding-left: 8px;
    font-size: 15px;
    font-weight: bold;
    color: #4a4033;
    border-left: 4px solid #9c6b2f;
}
.xr table.skill { width: 100%; table-layout: fixed; border-collapse: collapse; font-size: 13px; }
.xr table.skill th, .xr table.skill td { box-sizing: border-box; }
.xr table.skill th {
    padding: 3px 4px;
    border: 1px solid #b0a289;
    background: #e7ddc7;
    color: #4a4033;
    white-space: nowrap;
}
.xr table.skill td {
    padding: 4px 5px;
    border: 1px solid #c8bda6;
    background: #fcf9f1;
    color: #4a4033;
    vertical-align: top;
    line-height: 18px;
}
.xr table.skill td.c { text-align: center; white-space: nowrap; }
.xr .code {
    display: inline-block;
    padding: 0 6px;
    font-family: monospace;
    font-size: 14px;
    font-weight: bold;
    color: #3b2f18;
    background: #f0e3c0;
    border: 1px solid #c2ac74;
    border-radius: 3px;
}
.xr .sname { font-size: 14px; font-weight: bold; }
.xr .kind { display: inline-block; padding: 0 5px; font-size: 11px; border-radius: 2px; color: #fdf7ec; }
.xr .k-cult .sname { color: #2f6a52; }
.xr .k-cult .kind { background: #2f6a52; }
.xr .k-atk .sname { color: #a8322d; }
.xr .k-atk .kind { background: #a8322d; }
.xr .k-def .sname { color: #35576b; }
.xr .k-def .kind { background: #35576b; }
.xr .k-sch .sname { color: #6b4a86; }
.xr .k-sch .kind { background: #6b4a86; }
.xr .k-sec .sname { color: #9c6b2f; }
.xr .k-sec .kind { background: #9c6b2f; }
.xr .k-relic .sname { color: #8a6a12; }
.xr .k-relic .kind { background: #8a6a12; }
.xr .mates { margin-top: 6px; }
.xr .mates .mate {
    display: inline-block;
    margin: 0 8px 6px 0;
    padding: 3px 10px 3px 4px;
    font-size: 15px;
    line-height: 26px;
    white-space: nowrap;
    color: #33302a;
    background: #f6f0e1;
    border: 1px solid #cbbfa6;
    border-radius: 14px;
}
.xr .mates .mate .rl { color: #3c5a4a; }
.xr .mates .mate .hp { color: #a8322d; font-weight: bold; }
.xr .mates .mate .relic {
    display: inline-block;
    margin-left: 5px;
    padding: 0 3px;
    font-size: 13px;
    border-radius: 4px;
    background: #efe0c4;
    border: 1px solid #c2ac74;
}
.xr .mates .alone { font-size: 14px; color: #7a7263; }
.xr .relicbox {
    margin-top: 6px;
    padding: 9px 11px;
    border: 2px solid #9c6b2f;
    background: linear-gradient(160deg, #fdf6e2 0%, #f3e7c6 100%);
}
.xr .relicbox .rn { font-size: 17px; font-weight: bold; color: #7a4b1e; }
.xr .relicbox .rd { margin-top: 4px; font-size: 13px; color: #55483a; line-height: 19px; }
.xr .gone-note { margin-top: 10px; padding: 8px 10px; font-size: 14px; color: #7a7263; background: #ece5d5; }
.xr table.atlas { width: 100%; table-layout: fixed; border-collapse: separate; border-spacing: 10px; }
.xr table.atlas > tbody > tr > td {
    width: 50%;
    padding: 10px 12px;
    border: 1px solid #b6a98f;
    background: linear-gradient(160deg, #fdf9ef 0%, #f4ecd9 100%);
    vertical-align: top;
}
.xr .pathname { font-size: 20px; font-weight: bold; color: #7a4b1e; }
.xr .pathdesc { margin-top: 3px; font-size: 12px; color: #7a7263; line-height: 17px; }
.xr .realmline { margin-top: 5px; font-size: 13px; color: #4a4033; }
.xr .realmline .rw { color: #35576b; font-weight: bold; }
.xr table.al { width: 100%; table-layout: fixed; border-collapse: collapse; margin-top: 7px; font-size: 12px; }
.xr table.al th, .xr table.al td { box-sizing: border-box; }
.xr table.al th {
    padding: 2px 4px;
    border: 1px solid #b0a289;
    background: #e7ddc7;
    color: #4a4033;
    white-space: nowrap;
}
.xr table.al td {
    padding: 3px 5px;
    border: 1px solid #c8bda6;
    background: #fcf9f1;
    color: #4a4033;
    vertical-align: top;
    line-height: 16px;
}
.xr table.al td.c { text-align: center; white-space: nowrap; }
.xr table.al .code { padding: 0 4px; font-size: 12px; }
.xr table.al .sname { font-size: 13px; }
.xr table.al tr.pv td { background: #f4efe0; }
.xr table.al .pvtag {
    display: inline-block;
    padding: 0 5px;
    font-size: 11px;
    border-radius: 2px;
    color: #fdf7ec;
    background: #6f6553;
}
.xr table.al .pvname { font-size: 13px; font-weight: bold; color: #8a6a12; }
</style>
)";
}

inline std::string Board::BoardHtml(const std::string& phase, const int round, const int duel_deadline) const
{
    std::string html = PanelStyle() + "<div class='xr'>";
    html += "<div class='title'>登 仙 路</div>";
    html += "<div class='rule'></div>";
    html += "<div class='sub'>" + phase + "　·　存活 " + std::to_string(AliveCount()) + " / " +
            std::to_string(PlayerNum()) + " 名修士</div>";

    // 全局事件提示
    std::string notice;
    if (duel_deadline > 0) {
        const int left = duel_deadline - round + 1;
        notice += "⚔ 只余两名修士对峙，" +
                  std::string(left > 0 ? "若 " + std::to_string(left) + " 回合仍不分胜负，则以残存生机论高下"
                                       : "决战之期已至") + "<br>";
    }
    if (cfg.max_round - round < ROUND_WARN_AHEAD) {
        const int left = cfg.max_round - round + 1;
        notice += "⏳ 遗界天数将尽，" +
                  std::string(left > 1 ? "余 " + std::to_string(left) + " 回合" : "本回合即为最后一回合") +
                  "，届时以残存生机论高下<br>";
    }
    if (fuxiu_active) {
        notice += "☠ 万物腐败，生机尽失：除半仙外所有修士每回合固定扣去 2 血<br>";
    }
    // 仙器相关提示一律冠以该仙器自身的标记，与状态图中的仙器栏一致
    const std::string relic_icon = std::string(artifact_icon[static_cast<int>(falling_artifact)]);
    switch (artifact_state) {
        case ArtifactState::QUAKE:
            notice += "🌩 天地大震：破碎仙器随时可能自中央区域的天空陨落<br>";
            break;
        case ArtifactState::DESCENDED:
            notice += relic_icon + " 仙器【" + std::string(artifact_cn[static_cast<int>(falling_artifact)]) +
                      "】已降临中央区域，化神及以上者可施展夺天造化功争夺<br>";
            break;
        case ArtifactState::SETTLED:
            if (artifact_owner >= 0 && P(artifact_owner).ArtifactActive()) {
                notice += relic_icon + " " + PlayerNo(artifact_owner) + " 持有仙器【" +
                          std::string(artifact_cn[static_cast<int>(falling_artifact)]) + "】<br>";
            }
            break;
        case ArtifactState::NONE:
            break;
    }
    if (!notice.empty()) {
        html += "<div class='notice'>" + notice.substr(0, notice.size() - 4) + "</div>";
    }

    // 九宫遗界：整体为正方形，九个格子亦均为正方形
    html += "<table class='map'>";
    for (int row = 0; row < 3; row++) {
        html += "<tr>";
        for (int col = 0; col < 3; col++) {
            const int r = RegionAt(row, col);
            const bool gone = Destroyed(r);
            html += std::string("<td class='") + (r == CENTER_REGION ? "center" : (gone ? "gone" : "plain")) + "'>";
            html += "<div class='rhead'><span class='rname'>" + std::string(region_cn[r]) + "</span>";
            if (r == CENTER_REGION) {
                html += "<span class='tag center'>中央</span>";
            }
            if (gone) {
                html += "<span class='tag gone'>已湮灭</span>";
            }
            if (ArrayCount(r) > 0) {
                html += "<span class='tag array'>血阵</span>";
            }
            if (artifact_state == ArtifactState::DESCENDED && r == CENTER_REGION) {
                html += "<span class='tag relic'>仙器</span>";
            }
            html += "</div>";
            std::vector<int> list;
            for (int i = 0; i < PlayerNum(); i++) {
                if (P(i).Alive() && ShownRegion(i) == r) {
                    list.push_back(i);
                }
            }
            const CellLayout layout = CellLayoutOf(static_cast<int>(list.size()));
            const std::string pl_class = layout.dense ? "pl dense" : "pl";
            const std::string pl_style = " style='font-size:" + std::to_string(layout.font) +
                                         "px; line-height:" + std::to_string(layout.line) + "px'";
            for (const int pid : list) {
                const Player& p = P(pid);
                html += "<div class='" + pl_class + "'" + pl_style + ">" + AvatarBadge(p.avatar, pid, layout) +
                        "<span class='rl'>" + p.RealmStr() + "</span> "
                        "<span class='hp'>" + NumStr(p.hp) + "</span>";
                if (p.ArtifactActive()) {
                    html += "<span class='relic'>" +
                            std::string(artifact_icon[static_cast<int>(p.artifact)]) + "</span>";
                }
                html += "</div>";
            }
            if (list.empty()) {
                html += "<div class='empty'>" + std::string(gone ? "此地已不存于世" : "此处空无一人") + "</div>";
            }
            html += "</td>";
        }
        html += "</tr>";
    }
    html += "</table>";

    // 修士名录
    html += "<table class='roster'><tr><th style='width:42px'>编号</th>"
            "<th style='width:236px'>修士</th><th style='width:52px'>境界</th>"
            "<th style='width:108px'>修为</th><th style='width:108px'>血量</th>"
            "<th style='width:38px'>位置</th></tr>";
    for (const Player& p : players) {
        html += std::string("<tr") + (p.Alive() ? ">" : " class='dead'>");
        html += "<td>" + PlayerNo(p.pid.Get()) + "</td>";
        html += NameCell(p.avatar, p.name);
        if (!p.Alive()) {
            html += "<td colspan='4'>" +
                    std::string(p.out == 2 ? "已离去" : "已陨落（第 " + std::to_string(p.out_round) + " 回合）") +
                    "</td></tr>";
            continue;
        }
        html += "<td>" + p.RealmStr() + "</td>";
        html += "<td>" + BarCell("cult", p.AtMaxRealm() ? 100 : static_cast<int>(p.cultivation * 100 / cfg.cultivate_step),
                        p.AtMaxRealm() ? std::string("顶点") : NumStr(p.cultivation)) + "</td>";
        html += "<td>" + BarCell("hp", static_cast<int>(p.hp * 100 / cfg.init_hp), NumStr(p.hp)) + "</td>";
        const int shown = ShownRegion(p.pid.Get());
        html += "<td>" + std::string(shown < 0 ? "—" : std::string(region_cn[shown])) + "</td>";
        html += "</tr>";
    }
    html += "</table></div>";
    return html;
}

inline std::string Board::MyStatusHtml(const int pid, const int round) const
{
    const Player& p = P(pid);
    const CellLayout id_layout{17, 27, 34, 13, false};

    std::string html = PanelStyle() + "<div class='xr'>";
    html += "<div class='title'>登 仙 路</div>";
    html += "<div class='rule'></div>";
    html += "<div class='sub'>修士自陈　·　此帖仅你一人可见</div>";

    // 身份牌
    html += "<div class='idcard'>";
    html += "<div>" + AvatarBadge(p.avatar, pid, id_layout) +
            "<span class='who'>" + EscHtml(p.name) + "</span>" +
            "<span class='path'>" + std::string(path_icon[PathIndex(p.path)]) + " " + p.PathName() + "</span></div>";
    html += "<table class='attr'>";
    html += "<tr><td class='lab'>境界</td><td>" + p.RealmStr() + "</td>"
            "<td class='lab'>所在</td><td>" +
            std::string(p.region < 0 ? "尚未降临" : "区域「" + std::string(region_cn[p.region]) + "」") + "</td></tr>";
    html += "<tr><td class='lab'>修为</td><td>" +
            BarCell("cult", p.AtMaxRealm() ? 100 : static_cast<int>(p.cultivation * 100 / cfg.cultivate_step),
                    p.AtMaxRealm() ? std::string("顶点") : NumStr(p.cultivation)) + "</td>"
            "<td class='lab'>血量</td><td>" +
            BarCell("hp", static_cast<int>(p.hp * 100 / cfg.init_hp), NumStr(p.hp)) + "</td></tr>";
    html += "</table>";

    // 被动不占行动，随境界自行运转，故列于身份牌内而非招式表中
    std::string passive;
    if (HasShouhun(p.path)) {
        passive += PassiveRow(PASSIVE_SHOUHUN, "生效中", true, PASSIVE_SHOUHUN_BRIEF, passive.empty());
    }
    if (BusiRealm(p.path) >= 0) {
        const bool active = p.BusiActive(round);
        const std::string tag = active               ? "生效中"
                              : p.hedao_round > 0    ? "下回合起生效"
                                                     : RealmName(p.path, BusiRealm(p.path)) + "后生效";
        passive += PassiveRow(PASSIVE_BUSI, tag, active, PASSIVE_BUSI_BRIEF, passive.empty());
    }
    if (HuichunRealm(p.path) >= 0) {
        // 回春只在踏入合道的那一回合应验，此后便是过去之事
        const bool pending = p.hedao_round < 0;
        const std::string tag = pending ? RealmName(p.path, HuichunRealm(p.path)) + "时应验" : "已应验";
        passive += PassiveRow(PASSIVE_HUICHUN, tag, !pending, PASSIVE_HUICHUN_BRIEF, passive.empty());
    }
    if (!passive.empty()) {
        html += "<div class='pas'>" + passive + "</div>";
    }
    html += "</div>";

    if (!p.Alive()) {
        html += "<div class='gone-note'>你已身死道消，再无出手之机。且看后来者如何走完这条登仙路。</div></div>";
        return html;
    }

    // 同区域的修士：头像与角标同赛况地图，便于直接照着编号出手
    if (p.region >= 0) {
        html += "<div class='sect'>同区域的修士</div><div class='mates'>";
        const CellLayout mate_layout{15, 26, 26, 11, false};
        int count = 0;
        for (const lgtbot::PlayerID mate : AliveInRegion(p.region)) {
            if (mate == p.pid) {
                continue;
            }
            const Player& other = P(mate.Get());
            count++;
            html += "<span class='mate'>" + AvatarBadge(other.avatar, mate.Get(), mate_layout) +
                    "<span class='rl'>" + other.RealmStr() + "</span> "
                    "<span class='hp'>" + NumStr(other.hp) + "</span>";
            if (other.ArtifactActive()) {
                html += "<span class='relic'>" +
                        std::string(artifact_icon[static_cast<int>(other.artifact)]) + "</span>";
            }
            html += "</span>";
        }
        if (count == 0) {
            html += "<span class='alone'>此地唯你一人，四下并无他者气息</span>";
        }
        html += "</div>";
    }

    // 基础招式表：仙器另行单列
    html += "<div class='sect'>当前可用招式</div>";
    html += "<table class='skill'><tr><th style='width:56px'>代号</th><th style='width:104px'>招式</th>"
            "<th style='width:74px'>目标</th><th>说明</th></tr>";
    bool any = false;
    for (const Act act : AvailableActs(*this, p)) {
        if (ActKindOf(act) == ActKind::RELIC) {
            continue;
        }
        any = true;
        html += "<tr class='" + ActKindCss(act) + "'>";
        html += "<td class='c'><span class='code'>" + ActCode(act) + "</span></td>";
        html += "<td class='c'><span class='sname'>" + ActName(p.path, act) + "</span><br>"
                "<span class='kind'>" + ActKindName(act) + "</span></td>";
        html += "<td class='c'>" + ArgTypeHint(ActArgType(act)) + "</td>";
        html += "<td>" + ActBrief(p.path, act) + "</td>";
        html += "</tr>";
    }
    if (!any) {
        html += "<tr><td colspan='4'>此刻无招可出</td></tr>";
    }
    html += "</table>";

    // 仙器单列：已用尽或破碎者不再列出
    if (p.ArtifactActive()) {
        const int idx = static_cast<int>(p.artifact);
        html += "<div class='sect'>随身仙器</div><div class='relicbox'>";
        const Act relic_act = RelicAct(p.artifact);
        html += "<div><span class='rn'>" + std::string(artifact_icon[idx]) + " " + std::string(artifact_cn[idx]) +
                "</span>";
        if (relic_act != Act::NONE) {
            html += "　<span class='code'>" + ActCode(relic_act) + "</span>";
        }
        if (p.artifact == Artifact::LIHUA) {
            html += "　<span class='kind' style='background:#8a6a12'>尚余 " + std::to_string(p.lihua_left) + " 度</span>";
        } else if (p.artifact == Artifact::SHOUGUO) {
            html += std::string("　<span class='kind' style='background:#8a6a12'>") +
                    (p.shouguo_used ? "已服用" : "尚未服用") + "</span>";
        } else if (p.artifact == Artifact::MOSHA) {
            html += "　<span class='kind' style='background:#8a6a12'>可反复挥剑</span>";
        } else {
            html += "　<span class='kind' style='background:#8a6a12'>被动</span>";
        }
        html += "</div><div class='rd'>" + std::string(artifact_desc[idx]) + "</div></div>";
    }

    html += "<div class='foot'>代号可直接施展，目标写在代号之后，如「HQ 3」「WW 七」「XS 右下」；仙器代号一律以 Q 起首</div>";
    html += "</div>";
    return html;
}

inline std::string Board::ScoreHtml() const
{
    const int n = PlayerNum();
    std::vector<int> order(n);
    for (int i = 0; i < n; i++) {
        order[i] = i;
    }
    std::sort(order.begin(), order.end(), [this](const int a, const int b) {
        return players[a].score.rank < players[b].score.rank;
    });

    std::string html = PanelStyle() + "<div class='xr'>";
    html += "<div class='title'>登 仙 路</div>";
    html += "<div class='rule'></div>";
    html += "<div class='sub'>终局　·　道途尽头的修士名录</div>";
    html += "<table class='roster'><tr><th style='width:36px'>名次</th>"
            "<th style='width:42px'>编号</th><th style='width:236px'>修士</th>"
            "<th style='width:52px'>修行</th><th style='width:52px'>境界</th>"
            "<th style='width:55px'>生存</th><th style='width:55px'>击杀</th>"
            "<th style='width:56px'>总分</th></tr>";
    for (const int idx : order) {
        const Player& p = players[idx];
        const bool win = (p.score.rank == 1);
        html += std::string("<tr") + (win ? " class='win'>" : ">");
        html += "<td><span class='rk" + std::string(win ? " first" : "") + "'>" +
                std::to_string(p.score.rank) + "</span></td>";
        html += "<td>" + PlayerNo(p.pid.Get()) + "</td>";
        html += NameCell(p.avatar, p.name);
        html += "<td>" + std::string(path_icon[PathIndex(p.path)]) + " " + p.PathName() + "</td>";
        html += "<td>" + p.RealmStr() + "</td>";
        html += "<td>" + std::to_string(p.score.SurviveScore()) + "</td>";
        html += "<td>" + std::to_string(p.score.KillScore()) + "</td>";
        html += "<td>" + std::to_string(p.score.FinalScore()) + "</td>";
        html += "</tr>";
    }
    html += "</table>";
    html += "<div class='foot'>存活每回合 +2　击杀每人 +10　名次分 60 / 30 / 20 / 15 / 10 / 5</div>";
    html += "</div>";
    return html;
}

// 四条修行路线的完整招式图鉴：2×2 排布，每格一路，按境界标注解锁时机
inline std::string Board::AtlasHtml()
{
    std::string html = PanelStyle() + "<div class='xr wide'>";
    html += "<div class='title'>登 仙 路</div>";
    html += "<div class='rule'></div>";
    html += "<div class='sub'>四路道途图鉴　·　招式与其解锁境界</div>";
    html += "<table class='atlas'>";
    for (int row = 0; row < 2; row++) {
        html += "<tr>";
        for (int col = 0; col < 2; col++) {
            const Path path = static_cast<Path>(row * 2 + col);
            const int idx = PathIndex(path);
            html += "<td>";
            html += "<div><span class='pathname'>" + std::string(path_icon[idx]) + " " +
                    std::string(path_cn[idx]) + "</span></div>";
            html += "<div class='pathdesc'>" + std::string(path_desc[idx]) + "</div>";
            html += "<div class='realmline'>境界：";
            for (int r = 0; r < RealmNum(path); r++) {
                html += (r > 0 ? " → " : "") + std::string("<span class='rw'>") + RealmName(path, r) + "</span>";
            }
            html += "</div>";

            html += "<table class='al'><tr><th style='width:46px'>境界</th><th style='width:40px'>代号</th>"
                    "<th style='width:88px'>招式</th><th>说明</th></tr>";
            // 被动不占行动，与其解锁境界的招式列在一处
            for (int r = 0; r < RealmNum(path); r++) {
                if (ShouhunRealm(path) == r) {
                    html += "<tr class='pv'><td class='c'>" + RealmName(path, r) + "</td>"
                            "<td class='c'><span class='pvtag'>被动</span></td>"
                            "<td class='c'><span class='pvname'>" + std::string(PASSIVE_SHOUHUN) + "</span></td>"
                            "<td>" + std::string(PASSIVE_SHOUHUN_BRIEF) + "</td></tr>";
                }
                if (BusiRealm(path) == r) {
                    html += "<tr class='pv'><td class='c'>" + RealmName(path, r) + "</td>"
                            "<td class='c'><span class='pvtag'>被动</span></td>"
                            "<td class='c'><span class='pvname'>" + std::string(PASSIVE_BUSI) + "</span></td>"
                            "<td>进入该境界的下一回合起自动生效。" + std::string(PASSIVE_BUSI_BRIEF) + "</td></tr>";
                }
                if (HuichunRealm(path) == r) {
                    html += "<tr class='pv'><td class='c'>" + RealmName(path, r) + "</td>"
                            "<td class='c'><span class='pvtag'>被动</span></td>"
                            "<td class='c'><span class='pvname'>" + std::string(PASSIVE_HUICHUN) + "</span></td>"
                            "<td>" + std::string(PASSIVE_HUICHUN_BRIEF) + "</td></tr>";
                }
                for (int a = 1; a < ACT_NUM; a++) {
                    const Act act = static_cast<Act>(a);
                    if (ActKindOf(act) == ActKind::RELIC || RequiredRealm(path, act) != r) {
                        continue;
                    }
                    html += "<tr class='" + ActKindCss(act) + "'>";
                    html += "<td class='c'>" + RealmName(path, r) + "</td>";
                    html += "<td class='c'><span class='code'>" + ActCode(act) + "</span></td>";
                    html += "<td class='c'><span class='sname'>" + ActName(path, act) + "</span><br>"
                            "<span class='kind'>" + ActKindName(act) + "</span></td>";
                    html += "<td>" + ActBrief(path, act) + "</td>";
                    html += "</tr>";
                }
            }
            html += "</table></td>";
        }
        html += "</tr>";
    }
    html += "</table>";

    // 仙器不随境界解锁，而是降世后由人争夺，故单列于图鉴之末
    html += "<div class='sect'>仙器　·　一局只降其一，化神及以上者于中央区域争夺</div>";
    html += "<table class='al'><tr><th style='width:46px'>类别</th><th style='width:40px'>代号</th>"
            "<th style='width:88px'>仙器</th><th>说明</th></tr>";
    for (int i = 1; i < static_cast<int>(Artifact::COUNT); i++) {
        const Artifact relic = static_cast<Artifact>(i);
        const Act act = RelicAct(relic);
        html += "<tr class='k-relic'>";
        html += "<td class='c'>仙器</td>";
        html += "<td class='c'>" + std::string(act == Act::NONE ? "<span class='pvtag'>被动</span>"
                                                                : "<span class='code'>" + ActCode(act) + "</span>") +
                "</td>";
        html += "<td class='c'><span class='sname'>" + std::string(artifact_icon[i]) + " " +
                std::string(artifact_cn[i]) + "</span></td>";
        html += "<td>" + std::string(artifact_desc[i]) + "</td>";
        html += "</tr>";
    }
    html += "</table>";

    html += "<div class='foot'>招式于对应境界解锁，此后一直可用　·　私信「状态」可查看自身当前可用招式</div>";
    html += "</div>";
    return html;
}

// 伪神目：私下呈上全场存活修士的道途名录，连同其修为与血量
inline std::string Board::WeishenHtml() const
{
    std::string html = PanelStyle() + "<div class='xr'>";
    html += "<div class='title'>登 仙 路</div>";
    html += "<div class='rule'></div>";
    html += "<div class='sub'><span class='" + ActKindCss(Act::WEISHEN) + "'>"
            "<span class='code'>" + ActCode(Act::WEISHEN) + "</span> "
            "<span class='sname'>" + std::string(ActInfoOf(Act::WEISHEN).name) + "</span> "
            "<span class='kind'>" + ActKindName(Act::WEISHEN) + "</span></span>"
            "　·　全场修士的道途尽入眼底</div>";
    html += "<table class='roster'><tr><th style='width:42px'>编号</th>"
            "<th style='width:236px'>修士</th><th style='width:54px'>修行</th>"
            "<th style='width:40px'>境界</th><th style='width:106px'>修为</th>"
            "<th style='width:106px'>血量</th></tr>";
    for (const Player& p : players) {
        html += std::string("<tr") + (p.Alive() ? ">" : " class='dead'>");
        html += "<td>" + PlayerNo(p.pid.Get()) + "</td>";
        html += NameCell(p.avatar, p.name);
        html += "<td>" + std::string(path_icon[PathIndex(p.path)]) + " " + p.PathName() + "</td>";
        html += "<td>" + p.RealmStr() + "</td>";
        if (!p.Alive()) {
            html += "<td colspan='2'>" +
                    std::string(p.out == 2 ? "已离去" : "已陨落（第 " + std::to_string(p.out_round) + " 回合）") +
                    "</td></tr>";
            continue;
        }
        html += "<td>" + BarCell("cult", p.AtMaxRealm() ? 100 : static_cast<int>(p.cultivation * 100 / cfg.cultivate_step),
                        p.AtMaxRealm() ? std::string("顶点") : NumStr(p.cultivation)) + "</td>";
        html += "<td>" + BarCell("hp", static_cast<int>(p.hp * 100 / cfg.init_hp), NumStr(p.hp)) + "</td>";
        html += "</tr>";
    }
    html += "</table>";
    html += "<div class='foot'>此帖仅你一人可见　·　全场道途尽在于此</div>";
    html += "</div>";
    return html;
}

// 万念璃花：私下呈上本区域众人此回合已定的行动
inline std::string Board::LihuaHtml(const int holder) const
{
    const Player& owner = P(holder);
    std::string html = PanelStyle() + "<div class='xr'>";
    html += "<div class='title'>登 仙 路</div>";
    html += "<div class='rule'></div>";
    const int lihua_idx = static_cast<int>(Artifact::LIHUA);
    html += "<div class='sub'><span class='k-relic'>"
            "<span class='code'>" + ActCode(Act::LIHUA) + "</span> "
            "<span class='sname'>" + std::string(artifact_icon[lihua_idx]) + " " +
            std::string(artifact_cn[lihua_idx]) + "</span> "
            "<span class='kind'>仙器</span></span>"
            "　·　区域「" + std::string(region_cn[owner.region]) + "」众生之念尽入眼底</div>";
    html += "<table class='roster'><tr><th style='width:42px'>编号</th>"
            "<th style='width:236px'>修士</th><th style='width:52px'>境界</th>"
            "<th style='width:106px'>血量</th><th style='width:148px'>本回合行动</th></tr>";
    for (const lgtbot::PlayerID mate : AliveInRegion(owner.region)) {
        const Player& p = P(mate.Get());
        const bool self = (mate.Get() == static_cast<uint32_t>(holder));
        html += std::string("<tr") + (self ? " class='win'>" : ">");
        html += "<td>" + PlayerNo(p.pid.Get()) + "</td>";
        html += NameCell(p.avatar, p.name);
        html += "<td>" + p.RealmStr() + "</td>";
        html += "<td>" + BarCell("hp", static_cast<int>(p.hp * 100 / cfg.init_hp), NumStr(p.hp)) + "</td>";
        html += "<td>" + std::string(self ? "你自己：" : "") + ActDesc(p, p.action) + "</td>";
        html += "</tr>";
    }
    html += "</table>";
    html += "<div class='foot'>此帖仅你一人可见　·　你的追加行动将在全场结算完毕后单独生效</div>";
    html += "</div>";
    return html;
}
