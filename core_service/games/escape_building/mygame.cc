// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <random>
#include <span>

#include "hints.h"

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;

const GameProperties k_properties {
    .name_ = "逃脱大楼",
    .developer_ = "铁蛋",
    .description_ = "堕入无边地狱解救人质和杀死罪犯的传奇故事",
    .shuffled_player_id_ = true,
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 2; }
uint32_t Multiple(const CustomOptions& options) { return 1; }
const MutableGenericOptions k_default_generic_options{
    .is_formal_ = false,
};
const std::vector<RuleCommand> k_rule_commands = {};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() != 2) {
        reply() << "该游戏为双人游戏，必须为 2 人参加，当前玩家数为 " << generic_options_readonly.PlayerNum();
        return false;
    }
    return true;
}

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options)
            {
                generic_options.bench_computers_to_player_num_ = 2;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
    InitOptionsCommand("配置游戏初始楼层高度",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options, const uint32_t floor) {
                GET_OPTION_VALUE(game_options, 楼层) = floor;
                return NewGameMode::MULTIPLE_USERS;
            },
            ArithChecker<uint32_t>(10, 50, "层数")),
};

// Role
enum class Role {
    POLICE,
    KILLER
};

// ========== GAME STAGES ==========

class RoundStage;
class ShootStage;

class MainStage : public MainGameStage<RoundStage, ShootStage>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility), MakeStageCommand(*this, "查看当前游戏进展情况", &MainStage::Status_, VoidChecker("赛况")))
        , round_(0)
        , player_scores_(Global().PlayerNum(), 0)
        , player_hp_(Global().PlayerNum(), 2)
    {}

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }

    std::vector<int64_t> player_scores_;

    // 回合数
    int round_;
    // 玩家信息
    std::vector<Role> player_role_;
    std::vector<int64_t> player_hp_;

    // 玩家行动
    int64_t police_select;
    bool shoot;
    int64_t hostage_floor = -1;
    int64_t knife_floor = -1;

    // 游戏状态
    int64_t police_floor;           // 警察楼层
    int64_t bullets;                // 剩余子弹
    int64_t killed_hostages = 0;    // 误杀人质数
    int64_t rescued_hostages = 0;   // 解救人质数
    bool smoke_used = false;        // 烟雾弹已被使用
    bool smoke_trigger = false;     // 下回合触发烟雾弹
    bool police_insane = false;     // 精神错乱

    struct FloorInfo {
        bool hostage_used = false;  // 是否已经放置过人质
        std::string police_records; // 警察本层记录
        std::string killer_records; // 杀手本层记录

        void NewKillerRecord(const std::string record) {
            if (!killer_records.empty()) killer_records += "<br>";
            killer_records += record;
        }
        void NewPoliceRecord(const std::string record) {
            if (!police_records.empty()) police_records += "<br>";
            police_records += record;
        }
        void UpdatePoliceRecord(const std::string record) {
            police_records += record;
        }
    };
    std::vector<FloorInfo> floors;

    PlayerID RoleID(const Role role) const { if (role == Role::KILLER) return 0; else return 1; }

    // 赛况表格
    std::string GetTable(const bool show) const
    {
        html::Table playerTable(2, 4);
        playerTable.SetTableStyle("align=\"center\" cellpadding=\"2\"");
        for (int pid = 0; pid < 2; pid++) {
            playerTable.Get(pid, 0).SetStyle("style=\"width:40px;\"").SetContent(Global().PlayerAvatar(pid, 40));
            playerTable.Get(pid, 1).SetStyle("style=\"width:250px; text-align:left;\"").SetContent(Global().PlayerName(pid));
            playerTable.Get(pid, 2).SetStyle("style=\"width:50px;\"")
                .SetContent(player_role_[pid] == Role::POLICE ? "<span style='color:blue;'><b>[警察]</b></span>" : "<span style='color:red;'><b>[杀手]</b></span>");
            playerTable.Get(pid, 3).SetStyle("style=\"width:80px;\"")
                .SetContent("血量：<span style='color:orange;font-weight:bold;'>" + std::to_string(player_hp_[pid]) + "</span>");
        }

        const std::string no_bullet = bullets == 0 ? " background-color: #c9c9c9;" : "";
        const std::string get_hostages = rescued_hostages >= GAME_OPTION(解救人质) ? " background-color: #d9f2e6;" : "";
        const std::string insane = killed_hostages >= 2 ? " background-color: #f8d6d6;" : "";
        html::Table extra(1, 3);
        extra.SetTableStyle("style=\"margin: 12px auto;\"");
        extra.Get(0, 0).SetStyle("style=\"width:140px;" + no_bullet + "\"")
            .SetContent("剩余子弹：<span style='color:#3498db;'><b>" + std::to_string(bullets) + "</b></span>");
        extra.Get(0, 1).SetStyle("style=\"width:140px;" + get_hostages + "\"")
            .SetContent("解救人质：<span style='color:#2ecc71;'><b>" + std::to_string(rescued_hostages) + "</b></span> / " + std::to_string(GAME_OPTION(解救人质)));
        extra.Get(0, 2).SetStyle("style=\"width:140px;" + insane + "\"")
            .SetContent("误杀人质：<span style='color:#e74c3c;'><b>" + std::to_string(killed_hostages) + "</b></span> / 3");

        html::Table table(GAME_OPTION(楼层) + 1, show ? 3 : 2);
        table.Get(0, 0).SetStyle("style=\"width:50px;\"").SetContent("楼层");
        table.Get(0, 1).SetStyle("style=\"width:180px;\"").SetContent("<span style='color:blue;'><b>[警察]</b></span>");
        if (show) table.Get(0, 2).SetStyle("style=\"width:180px;\"").SetContent("<span style='color:red;'><b>[杀手]</b></span>");
        for (int i = 0; i < GAME_OPTION(楼层); i++) {
            int floor = GAME_OPTION(楼层) - i;
            table.Get(i + 1, 0).SetContent(std::to_string(floor));
            table.Get(i + 1, 1).SetContent(floors[floor].police_records);
            if (show) table.Get(i + 1, 2).SetContent(floors[floor].killer_records);
        }
        return "### 第 " + std::to_string(round_) + " 回合" + playerTable.ToString() + extra.ToString() + table.ToString();
    }

  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (!is_public && player_role_[pid] == Role::KILLER) {
            reply() << Markdown(GetTable(true));
        } else {
            reply() << Markdown(GetTable(false));
        }
        return StageErrCode::OK;
    }

    void FirstStageFsm(SubStageFsmSetter setter) override
    {
        player_role_.push_back(Role::KILLER);
        player_role_.push_back(Role::POLICE);
        Global().Boardcast() << "本局游戏双方玩家身份：\n"
                             << "杀手：" << At(RoleID(Role::KILLER)) << "\n"
                             << "警察：" << At(RoleID(Role::POLICE));

        floors.resize(GAME_OPTION(楼层) + 1);
        floors[GAME_OPTION(楼层)].NewPoliceRecord("[R0] 警察开始行动");

        player_hp_[0] = GAME_OPTION(杀手血量);
        player_hp_[1] = GAME_OPTION(警察血量);
        police_floor = GAME_OPTION(楼层);
        bullets = GAME_OPTION(子弹);
        setter.Emplace<RoundStage>(*this, ++round_);
    }

    void NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override
    {
        // 超时或强退中途结束
        if (player_scores_[0] == -1 || player_scores_[1] == -1) {
            // 终局赛况
            Global().Boardcast() << Markdown(GetTable(true));
            return;
        }
        // 更新游戏状态
        police_floor = police_select;
        floors[police_floor].NewPoliceRecord("[R" + std::to_string(round_) + "] ");
        if (hostage_floor > 0) {
            floors[hostage_floor].hostage_used = true;
            floors[hostage_floor].NewKillerRecord("[R" + std::to_string(round_) + "] <span style='color:blue;'>弹射人质</span>");
        }
        if (knife_floor > 0) {
            floors[knife_floor].NewKillerRecord("[R" + std::to_string(round_) + "] <span style='color:red;'>拿刀出击</span>");
        }
        // 黑影
        bool has_shadow = false;
        bool encounter_hostage = hostage_floor == police_floor;
        bool encounter_killer = knife_floor == police_floor;
        if (police_insane) {
            has_shadow = true;
        } else if (encounter_hostage || encounter_killer) {
            has_shadow = true;
        }
        // 烟雾弹
        bool has_smoke = false;
        if (smoke_trigger) {
            has_smoke = true;
            smoke_used = true;
            smoke_trigger = false;
            floors[police_floor].NewKillerRecord("[R" + std::to_string(round_) + "] <span style='color:gray;'>释放烟雾弹</span>");
        }
        // 根据条件开始新阶段
        PlayerID police = RoleID(Role::POLICE);
        std::string floor_hint = " 乘坐电梯来到了 " + std::to_string(police_floor) + " 楼，";
        if ((has_shadow || has_smoke) && bullets == 0) {
            // 子弹耗尽直接结算
            if (encounter_killer) {
                // 被杀手刺伤
                player_hp_[police]--;
                floors[police_floor].UpdatePoliceRecord("<span style='color:blue;'>接</span>被<span style='color:red;'>杀手</span>刺伤");
                Global().Boardcast() << At(police) << floor_hint << GetRandomHint(shadow_hints) << "\n" << GetRandomHint(no_bullet_killer_stab_hints);
            } else if (encounter_hostage) {
                // 解救人质
                rescued_hostages++;
                floors[police_floor].UpdatePoliceRecord("<span style='color:blue;'>接</span>到一名<span style='color:blue;'>人质</span>");
                Global().Boardcast() << At(police) << floor_hint << GetRandomHint(shadow_hints) << "\n" << GetRandomHint(rescue_hostage_hint);
            } else if (has_smoke) {
                // 烟雾弹但无事发生
                floors[police_floor].UpdatePoliceRecord("什么都没有<span style='color:blue;'>接</span>到");
                Global().Boardcast() << At(police) << floor_hint << GetRandomHint(smoke_hints) << "\n" << GetRandomHint(no_bullet_smoke_hints);
            } else {
                // 无事发生
                floors[police_floor].UpdatePoliceRecord("什么都没有<span style='color:blue;'>接</span>到");
                Global().Boardcast() << At(police) << floor_hint << GetRandomHint(shadow_hints) << "\n" << GetRandomHint(get_empty_hint);
            }
            HandleRoundOver(setter);    // 回合结束
        }
        else if (has_smoke) {
            Global().Boardcast() << At(police) << floor_hint << GetRandomHint(smoke_hints) << "——【烟雾弹】被释放，本回合信息未知！";
            setter.Emplace<ShootStage>(*this, round_);
        }
        else if (has_shadow) {
            Global().Boardcast() << At(police) << floor_hint << GetRandomHint(shadow_hints);
            setter.Emplace<ShootStage>(*this, round_);
        }
        else {
            floors[police_floor].UpdatePoliceRecord("无事发生");
            Global().Boardcast() << At(police) << floor_hint << GetRandomHint(empty_hints);
            HandleRoundOver(setter);    // 回合结束
        }
    }

    void NextStageFsm(ShootStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override
    {
        HandleRoundOver(setter);    // 回合结束
    }

    void HandleRoundOver(SubStageFsmSetter& setter)
    {
        if (!CheckGameEnd()) {
            setter.Emplace<RoundStage>(*this, ++round_);
            return;
        }
        // 终局赛况
        Global().Boardcast() << Markdown(GetTable(true));
    }

    bool CheckGameEnd()
    {
        PlayerID police = RoleID(Role::POLICE);
        PlayerID killer = RoleID(Role::KILLER);
        // 误杀 3 名人质
        if (killed_hostages >= 3) {
            player_scores_[killer] = 1;
            Global().Boardcast() << At(police) << " 误杀了 3 名人质，游戏结束！";
            return true;
        }
        // 生命值归零
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (player_hp_[pid] <= 0) {
                player_scores_[!pid] = 1;
                Global().Boardcast() << At(pid) << " 生命值归零，游戏结束！";
                return true;
            }
        }
        // 警察抵达一楼
        if (police_floor == 1) {
            if (rescued_hostages >= GAME_OPTION(解救人质)) {
                player_scores_[police] = 1;
                Global().Boardcast() << "已抵达一楼，游戏结束！" << At(police) << " 成功解救了 " << rescued_hostages << " 名人质，获得胜利！";
            } else {
                player_scores_[killer] = 1;
                Global().Boardcast() << "已抵达一楼，游戏结束！未能解救 " << GAME_OPTION(解救人质) << " 名人质，杀手 " << At(killer) << " 获得胜利！";
            }
            return true;
        }
        return false;
    }

};

class RoundStage : public SubGameStage<>
{
  public:
    RoundStage(MainStage& main_stage, const uint64_t round)
        : StageFsm(main_stage, "第 " + std::to_string(round) + " 回合",
                MakeStageCommand(*this, "[警察] 选择电梯楼层下楼", &RoundStage::PoliceSelect_,
                    ArithChecker<int64_t>(1, 50, "楼层")),
                MakeStageCommand(*this, "[杀手] 选择楼层弹射人质", &RoundStage::KillerHostageSelect_,
                    AlterChecker<int>({{"人质", 0}, {"1", 0}}), ArithChecker<int64_t>(1, 50, "楼层")),
                MakeStageCommand(*this, "[杀手] 选择楼层拿刀主动出击（可选）", &RoundStage::KillerKnifeSelect_,
                    AlterChecker<int>({{"拿刀", 0}, {"2", 0}}), ArithChecker<int64_t>(1, 50, "楼层")),
                MakeStageCommand(*this, "[杀手] 启用烟雾弹（可选，每局限一次）", &RoundStage::KillerSmokeTrigger_,
                    AlterChecker<int>({{"烟雾弹", 0}, {"y", 0}}), OptionalDefaultChecker<BoolChecker>(true, "确定", "取消")),
                MakeStageCommand(*this, "[杀手] 确认行动，确认后无法修改", &RoundStage::KillerConfirm_,
                    AlterChecker<int>({{"确认", 0}, {"0", 0}})))
    {
        higher_floor = Main().police_floor - 1;
        lower_floor = (Main().police_floor - GAME_OPTION(电梯) > 0) ? (Main().police_floor - GAME_OPTION(电梯)) : 1;
    }

    int higher_floor;
    int lower_floor;

    int t_hostage_floor = -1;
    int t_knife_floor = -1;
    bool t_smoke_trigger = false;

    virtual void OnStageBegin() override
    {
        Global().Boardcast() << Markdown(Main().GetTable(false));
        Global().Boardcast() << "本回合能够到达 " << higher_floor << "-" << lower_floor << " 楼，请双方私信裁判行动，时限 " << GAME_OPTION(时限) << " 秒，超时未行动判负"
                             << (higher_floor == 1 ? "（已抵达 2 楼，本回合不强制放人质）" : "");
        Global().StartTimer(GAME_OPTION(时限));
        std::string hostage_available = GetKillerHostageFloors();
        // 杀手无法安装弹射装置，强制平局
        if (Main().police_floor > 2 && hostage_available.empty()) {
            Global().SetReady(0); Global().SetReady(1);
            Main().player_scores_[0] = Main().player_scores_[1] = -1;
            Global().Boardcast() << "[提示] 杀手已无可安装弹射装置的楼层，且当前未抵达 2 楼，满足平局特殊条件，游戏结束！";
            return;
        }
        Global().Tell(Main().RoleID(Role::KILLER)) << "当前位于 " << Main().police_floor << " 楼，本回合可放置弹射装置的楼层为 " << (hostage_available.empty() ? "无" : hostage_available)
                                                   << (higher_floor == 1 ? "（已抵达 2 楼，本回合不强制放人质）" : "");
    }

  private:
    std::string GetKillerHostageFloors() const
    {
        std::string result;
        for (int f = higher_floor; f >= lower_floor; f--) {
            if (!Main().floors[f].hostage_used) {
                if (!result.empty()) result += "、";
                result += std::to_string(f);
            }
        }
        return result;
    }
    
    bool CheckCommon(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const Role role)
    {
        if (Main().player_role_[pid] != role) {
            if (Main().player_role_[pid] == Role::KILLER) {
                reply() << "[错误] 您的角色是「杀手」，无法使用此行动。杀手可用指令如下：\n"
                        << "- 人质 X、拿刀 X、烟雾弹、确认\n"
                        << "详细指令说明请使用「帮助」指令";
            } else {
                reply() << "[错误] 您的角色是「警察」，无法使用此行动。\n"
                        << "- 请发送单独的数字代表本回合要去的楼层\n"
                        << "详细指令说明请使用「帮助」指令";
            }
            return false;
        }
        if (is_public) {
            reply() << "[错误] 请私信裁判进行行动";
            return false;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 本回合行动已确认，已无退路可言";
            return false;
        }
        return true;
    }

    std::string GetCurrentPlan() const
    {
        return std::string("本回合行动计划：\n") +
               "弹射装置：" + (t_hostage_floor > 0 ? std::to_string(t_hostage_floor) + " 楼" : (Main().police_floor > 2 ? "【尚未部署】" : "未部署")) + "\n" +
               "拿刀楼层：" + (t_knife_floor > 0 ? std::to_string(t_knife_floor) + " 楼" : "不出动") + "\n" +
               "烟雾弹：" + (t_smoke_trigger ? "将在本回合释放" : (Main().smoke_used ? "已使用" : "未启用")) +
               (t_hostage_floor > 0 || Main().police_floor == 2 ? "\n\n若您已完成部署，请使用「确认」指令确认行动" : "");
    }
    
    AtomReqErrCode PoliceSelect_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int64_t f)
    {
        if (!CheckCommon(pid, is_public, reply, Role::POLICE)) {
            return StageErrCode::FAILED;
        }
        if (f > GAME_OPTION(楼层)) {
            reply() << "[错误] 喂，整座楼都没有这么高，你要准备上天吗？";
            return StageErrCode::FAILED;
        }
        if (f >= Main().police_floor || f < Main().police_floor - GAME_OPTION(电梯)) {
            reply() << "[错误] 下楼失败：最多下 " << GAME_OPTION(电梯) << " 层楼，且不能上楼。本回合可选楼层范围为 " << higher_floor << "-" << lower_floor;
            return StageErrCode::FAILED;
        }

        Main().police_select = f;
        reply() << "你按下了楼层 " << f << "，指示灯亮起，电梯伴随着低鸣缓缓下行……";
        return StageErrCode::READY;
    }

    AtomReqErrCode KillerHostageSelect_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int _, const int64_t f)
    {
        if (!CheckCommon(pid, is_public, reply, Role::KILLER)) {
            return StageErrCode::FAILED;
        }
        if (f > GAME_OPTION(楼层)) {
            reply() << "[错误] 喂，整座楼都没有这么高，你要准备上天吗？";
            return StageErrCode::FAILED;
        }
        if (f >= Main().police_floor || f < Main().police_floor - GAME_OPTION(电梯)) {
            reply() << "[错误] 安装失败：弹射装置只能安装在警察能够到达的楼层。本回合可选楼层范围为 " << higher_floor << "-" << lower_floor;
            return StageErrCode::FAILED;
        }
        if (Main().floors[f].hostage_used) {
            reply() << "[错误] 安装失败：" << f << " 楼的弹射装置已经报废，无法再次安装";
            return StageErrCode::FAILED;
        }

        t_hostage_floor = f;
        bool warning = false;
        if (t_knife_floor == f) {
            t_knife_floor = -1;
            warning = true;
        }
        reply() << (warning ? "[警告] 弹射和拿刀楼层存在冲突，已更新弹射楼层。\n\n" : "")
                << "您准备在 " << f << " 楼安装弹射装置。电梯停靠后，装置将被悄然部署。\n\n"
                << GetCurrentPlan();
        return StageErrCode::OK;
    }

    AtomReqErrCode KillerKnifeSelect_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int _, const int64_t f)
    {
        if (!CheckCommon(pid, is_public, reply, Role::KILLER)) {
            return StageErrCode::FAILED;
        }
        if (f > GAME_OPTION(楼层)) {
            reply() << "[错误] 喂，整座楼都没有这么高，你要准备上天吗？";
            return StageErrCode::FAILED;
        }
        if (f >= Main().police_floor || f < Main().police_floor - GAME_OPTION(电梯)) {
            reply() << "[错误] 行动失败：拿刀出击只能在警察能够到达的楼层。本回合可选楼层范围为 " << higher_floor << "-" << lower_floor;
            return StageErrCode::FAILED;
        }

        t_knife_floor = f;
        bool warning = false;
        if (t_hostage_floor == f) {
            t_hostage_floor = -1;
            warning = true;
        }
        reply() << (warning ? "[警告] 拿刀和弹射楼层存在冲突，已更新拿刀楼层。\n\n" : "")
                << "你已锁定 " << f << " 楼。拿刀行动已列入本回合计划，路线正在确认中。\n\n"
                << GetCurrentPlan();
        return StageErrCode::OK;
    }

    AtomReqErrCode KillerSmokeTrigger_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int _, const bool trigger)
    {
        if (!CheckCommon(pid, is_public, reply, Role::KILLER)) {
            return StageErrCode::FAILED;
        }
        if (Main().smoke_used) {
            reply() << "[错误] 设定失败：烟雾弹已使用完毕，本回合无法纳入行动计划";
            return StageErrCode::FAILED;
        }

        t_smoke_trigger = trigger;
        reply() << (trigger ? "烟雾弹已准备就绪，等待指令\n\n" : "烟雾弹指令已撤回，计划恢复静默\n\n")
                << GetCurrentPlan();
        return StageErrCode::OK;
    }

    AtomReqErrCode KillerConfirm_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int _)
    {
        if (!CheckCommon(pid, is_public, reply, Role::KILLER)) {
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 本回合行动已确认，已无退路可言";
            return StageErrCode::FAILED;
        }
        if (t_hostage_floor < 0 && Main().police_floor > 2) {
            reply() << "[错误] 确认失败：弹射装置尚未部署！在警察到达 2 楼前，每回合必须安装弹射装置";
            return StageErrCode::FAILED;
        }

        Main().hostage_floor = t_hostage_floor;
        Main().knife_floor = t_knife_floor;
        Main().smoke_trigger = t_smoke_trigger;
        reply() << "本回合行动已确认：\n"
                << "弹射装置：" << (t_hostage_floor > 0 ? std::to_string(t_hostage_floor) + " 楼" : "未部署") << "\n"
                << "拿刀楼层：" << (t_knife_floor > 0 ? std::to_string(t_knife_floor) + " 楼" : "不出动") << "\n"
                << "烟雾弹：" << (t_smoke_trigger ? "将在本回合释放" : (Main().smoke_used ? "已被使用" : "未启用"));
        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (!Global().IsReady(pid)) {
                if (Main().player_role_[pid] == Role::KILLER) {
                    if (t_hostage_floor > 0 || Main().police_floor <= 2) {
                        Main().hostage_floor = t_hostage_floor;
                        Main().knife_floor = t_knife_floor;
                        Main().smoke_trigger = t_smoke_trigger;
                        Global().Tell(pid) << "您超时未行动，已自动确认行动";
                    } else {
                        Main().player_scores_[pid] = -1;
                        Global().Boardcast() << "杀手 " << At(PlayerID(pid)) << " 超时判负";
                    }
                } else {
                    Main().player_scores_[pid] = -1;
                    Global().Boardcast() << "警察 " << At(PlayerID(pid)) << " 超时判负";
                }
            }
        }
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Main().player_scores_[pid] = -1;
        Global().Boardcast() << At(PlayerID(pid)) << " 强退认输。";
        return StageErrCode::CHECKOUT;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        if (Main().player_role_[pid] == Role::POLICE) {
            int32_t offset = rand() % GAME_OPTION(电梯);
            int select = higher_floor - offset;
            Main().police_select = std::max(1, select);
        } else {
            if (Main().police_floor == 2) {
                if (rand() % 2 == 0 && !Main().floors[1].hostage_used) {
                    Main().hostage_floor = 1;
                } else {
                    Main().knife_floor = 1;
                }
            } else {
                do {
                    int32_t offset = rand() % GAME_OPTION(电梯);
                    t_hostage_floor = higher_floor - offset;
                    Main().hostage_floor = std::max(1, t_hostage_floor);
                } while (Main().floors[Main().hostage_floor].hostage_used);
                if (rand() % 2 == 0) {
                    do {
                        int32_t offset = rand() % GAME_OPTION(电梯);
                        t_knife_floor = higher_floor - offset;
                        if (t_knife_floor < 1) break;
                        Main().knife_floor = t_knife_floor;
                    } while (Main().knife_floor == Main().hostage_floor);
                }
                if (rand() % 5 == 0 && !Main().smoke_used) {
                    Main().smoke_trigger = true;
                }
            }
        }
        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        return StageErrCode::CHECKOUT;
    }
};

class ShootStage : public SubGameStage<>
{
  public:
    ShootStage(MainStage& main_stage, const uint64_t round)
        : StageFsm(main_stage, "第 " + std::to_string(round) + " 回合[开枪阶段]",
                MakeStageCommand(*this, "[警察] 危机四伏，选择是否开枪！", &ShootStage::Shoot_,
                    AlterChecker<bool>({{"开枪", true}, {"是", true}, {"开", true}, {"不开枪", false}, {"否", false}, {"接", false}})))
    {}

    PlayerID police = Main().RoleID(Role::POLICE);
    PlayerID killer = Main().RoleID(Role::KILLER);

    virtual void OnStageBegin() override
    {
        Global().Boardcast() << "请 " << At(police) << " 选择是否开枪！时限 60 秒，超时默认不开枪";
        Global().SetReady(killer);
        Global().StartTimer(60);
    }

  private:
    AtomReqErrCode Shoot_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const bool shoot)
    {
        if (Main().player_role_[pid] == Role::KILLER) {
            reply() << "[错误] 您的角色是「杀手」，无法使用此行动。";
            return StageErrCode::FAILED;
        }
        Main().shoot = shoot;
        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        Main().shoot = false;
        Global().Boardcast() << At(PlayerID(police)) << " 超时未选择，默认不开枪";
        HandleShootStageOver();
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Main().player_scores_[pid] = -1;
        Global().Boardcast() << At(PlayerID(pid)) << " 强退认输。";
        return StageErrCode::CHECKOUT;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        Main().shoot = rand() % 2;
        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        HandleShootStageOver();
        return StageErrCode::CHECKOUT;
    }

    void HandleShootStageOver()
    {
        if (Main().shoot) {
            Main().bullets--;
            if (Main().police_floor == Main().knife_floor) {
                // 开枪打中杀手
                Main().player_hp_[killer]--;
                Main().floors[Main().police_floor].UpdatePoliceRecord("<span style='color:red;'>开枪</span>击中<span style='color:red;'>杀手</span>");
                Global().Boardcast() << GetRandomHint(shoot_killer_hints);
            } else if (Main().police_floor == Main().hostage_floor) {
                // 开枪误杀人质
                Main().killed_hostages++;
                Main().floors[Main().police_floor].UpdatePoliceRecord("<span style='color:red;'>开枪</span>误杀<span style='color:blue;'>人质</span>");
                if (Main().killed_hostages == 2) {
                    Main().police_insane = true;
                    Global().Boardcast() << GetRandomHint(shoot_more_hostage_hints) <<"\n（已误杀 2 名人质，精神崩塌加深，接下来的每层都将看到黑影。请注意：再次误杀人质将直接判负）";
                } else if (Main().killed_hostages == 3) {
                    Global().Boardcast() << GetRandomHint(police_lose_hints);
                } else {
                    Global().Boardcast() << GetRandomHint(shoot_hostage_hints);
                }
            } else {
                // 开枪无事发生
                Main().floors[Main().police_floor].UpdatePoliceRecord("<span style='color:red;'>开枪</span>没有命中");
                Global().Boardcast() << GetRandomHint(shoot_empty_hints);
            }
        } else {
            if (Main().police_floor == Main().knife_floor) {
                // 被杀手刺伤
                Main().player_hp_[police]--;
                Main().floors[Main().police_floor].UpdatePoliceRecord("<span style='color:blue;'>接</span>被<span style='color:red;'>杀手</span>刺伤");
                Global().Boardcast() << GetRandomHint(killer_stab_hints);
            } else if (Main().police_floor == Main().hostage_floor) {
                // 解救人质
                Main().rescued_hostages++;
                Main().floors[Main().police_floor].UpdatePoliceRecord("<span style='color:blue;'>接</span>到一名<span style='color:blue;'>人质</span>");
                Global().Boardcast() << GetRandomHint(rescue_hostage_hint);
            } else {
                // 没开枪无事发生
                Main().floors[Main().police_floor].UpdatePoliceRecord("什么都没有<span style='color:blue;'>接</span>到");
                Global().Boardcast() << GetRandomHint(get_empty_hint);
            }
        }
    }
};

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot

