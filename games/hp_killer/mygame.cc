// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <array>
#include <functional>
#include <memory>
#include <set>
#include <variant>
#include <random>
#include <algorithm>

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"
#include "occupation.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;
const GameProperties k_properties { 
    .name_ = "HP杀", // the game name which should be unique among all the games
    .developer_ = "森高",
    .description_ = "通过对其他玩家造成伤害，杀掉隐藏在玩家中的杀手的游戏",
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 9; } // 0 indicates no max-player limits
uint32_t Multiple(const CustomOptions& options) { return 3; } // the default score multiple for the game, 0 for a testing game, 1 for a formal game, 2 or 3 for a long formal game
const MutableGenericOptions k_default_generic_options{
    .is_formal_{false},
};

const char* const k_role_rules[Occupation::Count()] = {
    // killer team
    [static_cast<uint32_t>(Occupation(Occupation::杀手))] = R"EOF(【杀手 | 杀手阵营】
- 开局时知道所有「平民阵营」角色的代号，但不知道代号与职位的对应关系
- 可以选择「攻击 <代号> 25」和「治愈 <代号> 15」)EOF",

    [static_cast<uint32_t>(Occupation(Occupation::替身))] = R"EOF(【替身 | 杀手阵营】
- 开局时知道所有「平民阵营」角色的代号（五人场除外）
- 特殊技能「挡刀 <代号>」：令当前回合**攻击**指定角色造成的减 HP 效果转移到**自己**身上，次数不限)EOF",

    [static_cast<uint32_t>(Occupation(Occupation::恶灵))] = R"EOF(【恶灵 | 杀手阵营】
- 开局时知道所有「平民阵营」角色的代号（五人场除外）
- 死亡后仍可继续行动（「中之人」仍会被公布），直到触发以下任意一种情况时，从下一回合起失去行动能力：
    - 被【侦探】侦查到**治愈**或**攻击**操作
    - 被【灵媒】通灵)EOF",

    [static_cast<uint32_t>(Occupation(Occupation::刺客))] = R"EOF(【刺客 | 杀手阵营】
- 开局时知道所有「平民阵营」角色的代号（五人场除外）
- 特殊技能「攻击 <代号> (<代号>)... <伤害>」：扣除多名角色的 HP，代号不允许重复
    - 伤害可以是 0、5、10 或 15 中的一个：
        - 如果伤害是 0 或 15，则只能指定 1 个代号
        - 如果伤害是 5，则可以指定 1~5 个代号
        - 如果伤害是 10，则可以指定 1 或 2 个代号
    - 【侦探】侦查的结果，是攻击指令中**最靠前**的角色，例如：当刺客执行「攻击 B A C 5」时，侦探侦查到的结果是「攻击 B」)EOF",

    [static_cast<uint32_t>(Occupation(Occupation::双子（邪）))] = R"EOF(【双子 | 平民/杀手阵营
- 场上有两位【双子】，分别在「杀手阵营」和「平民阵营」，每位【双子】都知道双方的代号和阵营
- 【双子】不能成为【双子】攻击的目标，包括自己
- 如果【双子】中的一方死亡，另一方存活，则从下一回合起，存活方将加入死亡方的阵营（如果【双子】的死亡导致游戏结束，则存活方阵营**不发生**改变）)EOF",

    [static_cast<uint32_t>(Occupation(Occupation::魔女))] = R"EOF(【魔女 | 杀手阵营】
- 开局时知道所有「平民阵营」角色的代号（五人场除外）
- 不允许使用「攻击 <代号> 15」指令
- 特殊技能「诅咒 <代号> 5」和「诅咒 <代号> 10」：令指定角色进入诅咒状态
    - 处于诅咒状态的角色**执行非 pass 操作的回合**会流失 5 或 10 点 HP，直到被物理攻击（对于单个诅咒状态，进入诅咒状态的回合会流失体力，但是解除诅咒的回合不会，如果物理攻击被挡刀或者盾反则诅咒效果不会被解除）
    - 多次诅咒效果可以叠加
    - 当被侦探侦查时会显示「攻击 <代号>」
    - 无法被守卫「盾反」)EOF",

    // civilian team
    [static_cast<uint32_t>(Occupation(Occupation::平民))] = R"EOF(【平民 | 平民阵营】
- 无特殊能力)EOF",

    [static_cast<uint32_t>(Occupation(Occupation::圣女))] = R"EOF(【圣女 | 平民阵营】
- 不允许连续两回合使用「攻击 <代号>」指令，不受治愈次数的限制
- 攻击「平民阵营」角色不扣除 HP)EOF",

    [static_cast<uint32_t>(Occupation(Occupation::侦探))] = R"EOF(【侦探 | 平民阵营】
- 特殊技能「侦查 <代号>」：
    - 首回合不允许使用，且不允许连续两回合使用，此外次数不限
    - 当前回合结束时，将被私信告知指定角色的行动**种类**和行动**目标**（HP 具体数值保密），结果只可能有三种：「攻击 <代号>」、「治愈 <代号>」或「其它」（除攻击和治愈外的其它行动，包括 pass 等）
    - 可对死亡的角色使用（参见【恶灵】）
    - 侦查到的结果**取决于角色的行动指令**（*例如，有角色 A 打算攻击【杀手】B，结果本回合【替身】C 使用了挡刀技能，尽管实际扣除 HP 的是 C，但【侦探】侦查到的结果却是「A 攻击 B」*）)EOF",

    [static_cast<uint32_t>(Occupation(Occupation::灵媒))] = R"EOF(【灵媒 | 平民阵营】
- 特殊技能「通灵 <代号>」：
    - 一局内仅限一次，被指定的角色**需已死亡**
    - 当前回合结束时，会被私信告知该角色的职位，若为【恶灵】，则他下回合起**失去行动能力**)EOF",

    [static_cast<uint32_t>(Occupation(Occupation::守卫))] = R"EOF(【守卫 | 平民阵营】
- 特殊技能「盾反 <代号> <血量> (<代号> <血量>)...」：
    - 预测指定的 1 或 2 名角色下一回合的血量（如「盾反 A 70 B 50」），若预测其中某名角色成功，且该角色不是【替身】挡刀的目标，则当前回合**攻击**该角色造成的减 HP 效果会被转移到**伤害来源**身上（视为对这名角色**盾反成功**）
    - 次数不限，但不允许相邻两回合指定同一名角色
    - 当前回合结束时，会被私信告知对那些角色盾反成功)EOF",

    [static_cast<uint32_t>(Occupation(Occupation::双子（正）))] = R"EOF(【双子 | 平民/杀手阵营】
- 场上有两位【双子】，分别在「杀手阵营」和「平民阵营」，每位【双子】都知道双方的代号和阵营
- 【双子】不能成为【双子】攻击的目标，包括自己
- 如果【双子】中的一方死亡，另一方存活，则从下一回合起，存活方将加入死亡方的阵营（如果【双子】的死亡导致游戏结束，则存活方阵营**不发生**改变）)EOF",

    [static_cast<uint32_t>(Occupation(Occupation::骑士))] = R"EOF(【骑士 | 平民阵营】
- 游戏开始时【骑士】的中之人会被公布
- 【骑士】不知道自己的代号
- 当【骑士】攻击某名自己外的角色时，若也恰好受到该角色的攻击，则该角色的攻击伤害为 0，且【骑士】可以知晓该情况)EOF",

    // special team
    [static_cast<uint32_t>(Occupation(Occupation::初版内奸))] = R"EOF(【初版内奸 | 第三方阵营】
- 当 **【初版内奸】死亡**时，【初版内奸】失败
- 开局时知道【杀手】和所有【平民】的代号，但不知道代号与职位的对应关系
- 【杀手】死亡后的下一回合，【初版内奸】可执行「攻击 <代号> 25」和「治愈 <代号> 15」的行动指令)EOF",

    [static_cast<uint32_t>(Occupation(Occupation::内奸))] = R"EOF(【内奸 | 第三方阵营】
- 当 **【内奸】死亡**时，【内奸】失败
- 开局时知道【杀手】的中之人和所有【平民】的中之人
- 【杀手】死亡后的下一回合，【内奸】可执行「攻击 <代号> 25」和「治愈 <代号> 15」的行动指令)EOF",

    [static_cast<uint32_t>(Occupation(Occupation::特工))] = R"EOF(【特工 | 第三方阵营】
- 当「平民阵营」和「杀手阵营」同时满足失败条件时，【特工】取得单独胜利，否则【特工】失败（达到最大回合限制时，【特工】也会失败）
- 无法使用「攻击 <代号>」
- 当某角色死亡时，你获知他当前所在阵营
- 特殊技能 <蓄力 <代号> <血量> (<代号> <血量>)...>：
    - 自由分配 20 点隐藏伤害给至多 4 名角色
    - 不同回合分配的隐藏伤害可以累积
    - 为每名角色分配的隐藏伤害只能为 5、10 或 15 中的一个
    - 隐藏伤害对其余角色不可见
    - 【侦探】侦查时会显示「其他」
- 特殊技能 <释放 代号 (代号)...>：
    - 释放并清空对部分角色此前累积的隐藏伤害（此轮不可再蓄积）
    - 使用该技能的前提条件是对这些角色累积了隐藏伤害
    - 该技能效果等同于 <攻击 代号 血量 (<代号> <血量>)...>，即 <释放> 造成的伤害可以被挡刀或盾反，【侦探】侦查的结果，是指令中**最靠前**的角色，例如：当特工执行「释放 B A C」时，侦探侦查到的结果是「攻击 B」)EOF",

    [static_cast<uint32_t>(Occupation(Occupation::人偶))] = R"EOF(【人偶 | NPC】
- 不会做出任何行动)EOF",

};

const std::vector<RuleCommand> k_rule_commands = {
    RuleCommand("查看某名角色技能",
            [](const Occupation occupation) { return k_role_rules [static_cast<uint32_t>(occupation)]; },
            EnumChecker<Occupation>()),
};

static std::vector<Occupation>& GetOccupationList(CustomOptions& option, const uint32_t player_num)
{
    return player_num == 5 ? GET_OPTION_VALUE(option, 五人身份) :
           player_num == 6 ? GET_OPTION_VALUE(option, 六人身份) :
           player_num == 7 ? GET_OPTION_VALUE(option, 七人身份) :
           player_num == 8 ? GET_OPTION_VALUE(option, 八人身份) :
           player_num == 9 ? GET_OPTION_VALUE(option, 九人身份) : (assert(false), GET_OPTION_VALUE(option, 五人身份));
}

static const std::vector<Occupation>& GetOccupationList(const CustomOptions& option, const uint32_t player_num)
{
    return GetOccupationList(const_cast<CustomOptions&>(option), player_num);
}

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() < 5) {
        reply() << "该游戏至少 5 人参加，当前玩家数为 " << generic_options_readonly.PlayerNum();
        return false;
    }
    const auto player_num_matched = [player_num = generic_options_readonly.PlayerNum()](const std::vector<Occupation>& occupation_list)
        {
            return std::ranges::count_if(occupation_list, [](const Occupation occupation) { return occupation != Occupation::人偶; }) == player_num;
        };
#ifdef TEST_BOT
    if (!GET_OPTION_VALUE(game_options, 身份列表).empty() && !player_num_matched(GET_OPTION_VALUE(game_options, 身份列表))) {
        reply() << "玩家人数和身份列表长度不匹配";
        return false;
    }
#endif
    auto& occupation_list = GetOccupationList(game_options, generic_options_readonly.PlayerNum());
    if (occupation_list.empty()) {
        return true;
    }
    if (!player_num_matched(occupation_list)) {
        reply() << "[错误] 身份列表配置项身份个数与参加人数不符，请修正配置";
        return false;
    }
    if (std::ranges::count(occupation_list, Occupation::杀手) != 1) {
        reply() << "[错误] 身份列表中杀手个数必须为 1，请修正配置";
        return false;
    }
    if ((std::ranges::count(occupation_list, Occupation::内奸) > 0) +
            (std::ranges::count(occupation_list, Occupation::特工) > 0) +
            (std::ranges::count(occupation_list, Occupation::初版内奸) > 0) > 1) {
        reply() << "[错误] 初版内奸、内奸和特工不允许出现在同一局游戏中，请修正配置";
        return false;
    }
    if ((std::ranges::count(occupation_list, Occupation::守卫) > 0) +
            (std::ranges::count(occupation_list, Occupation::魔女) > 0) > 1) {
        reply() << "[错误] 守卫和魔女不允许出现在同一局游戏中，请修正配置";
        return false;
    }
    for (const auto occupation : std::initializer_list<Occupation>
            {Occupation::替身, Occupation::初版内奸, Occupation::内奸, Occupation::守卫, Occupation::双子（邪）, Occupation::双子（正）}) {
        if (std::ranges::count(occupation_list, occupation) > 1) {
            reply() << "[错误] 身份列表中" << occupation << "个数不允许大于 1，请修正配置";
            return false;
        }
    }
    return true;
}

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options)
            {
                generic_options.bench_computers_to_player_num_ = 8;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
};

// ========== PLAYER INFO ==========

struct Token
{
    char ToChar() const { return 'A' + id_; }

    friend std::istream& operator>>(std::istream& is, Token& token)
    {
        std::string s;
        if (!(is >> s)) {
            // already failed, do nothing
        } else if (s.size() != 1) {
            is.setstate(std::ios_base::failbit);
        } else if (std::islower(s[0])) {
            token.id_ = s[0] - 'a';
        } else if (std::isupper(s[0])) {
            token.id_ = s[0] - 'A';
        } else {
            is.setstate(std::ios_base::failbit);
        }
        return is;
    }

    friend std::ostream& operator<<(std::ostream& os, const Token& token)
    {
        return os << token.ToChar();
    }

    auto operator<=>(const Token&) const = default;

    uint32_t id_;
};

static const int32_t* GetHpFromMultiTargetAction(const Token token,
        const std::vector<std::tuple<Token, int32_t>>& token_hps)
{
    const auto it = std::ranges::find_if(token_hps,
            [token](const auto& token_hp) { return std::get<Token>(token_hp) == token; });
    return it == std::end(token_hps) ? nullptr : &std::get<int32_t>(*it);
}

template <typename Container, typename T>
bool Has(const Container& container, const T& value)
{
    return std::ranges::find(container, value) != std::end(container);
}

static std::string MultiTargetActionToString(const char* const name, const std::vector<Token>& tokens)
{
    std::string ret;
    for (const auto& token : tokens) {
        if (!ret.empty()) {
            ret += "<br>";
        }
        ret += name;
        ret += " ";
        ret += token.ToChar();
    }
    return ret;
}

static std::string MultiTargetActionToString(const char* const name, const std::vector<Token>& tokens, const int32_t hp)
{
    std::string ret;
    for (const auto& token : tokens) {
        if (!ret.empty()) {
            ret += "<br>";
        }
        ret += name;
        ret += " ";
        ret += token.ToChar();
        ret += " ";
        ret += std::to_string(hp);
    }
    return ret;
}

static std::string MultiTargetActionToString(const char* const name,
        const std::vector<std::tuple<Token, int32_t>>& token_hps)
{
    std::string ret;
    for (const auto& [token, hp] : token_hps) {
        if (!ret.empty()) {
            ret += "<br>";
        }
        ret += name;
        ret += " ";
        ret += token.ToChar();
        ret += " ";
        ret += std::to_string(hp);
    }
    return ret;
}

struct AttackAction
{
    std::string ToString() const { return MultiTargetActionToString("攻击", token_hps_); }

    std::vector<std::tuple<Token, int32_t>> token_hps_;
};

struct CurseAction
{
    std::string ToString() const { return std::string("诅咒 ") + token_.ToChar() + " " + std::to_string(hp_); }

    Token token_;
    int32_t hp_;
};

struct CureAction
{
    std::string ToString() const { return std::string("治愈 ") + token_.ToChar() + " " + std::to_string(hp_); }

    Token token_;
    int32_t hp_;
};

struct BlockAttackAction
{
    std::string ToString() const { return std::string("挡刀") + token_.ToChar(); }

    Token token_;
};

struct DetectAction
{
    std::string ToString() const { return std::string("侦查 ") + token_.ToChar(); }

    Token token_;
};

struct PassAction
{
    std::string ToString() const { return "pass"; }
};

struct ExocrismAction
{
    std::string ToString() const { return std::string("通灵 ") + token_.ToChar(); }

    Token token_;
};

struct ShieldAntiAction
{
    std::string ToString() const { return MultiTargetActionToString("盾反", token_hps_); }

    std::vector<std::tuple<Token, int32_t>> token_hps_;
};

struct AssignHiddenDamangeAction
{
    std::string ToString() const { return MultiTargetActionToString("蓄力", token_hps_); }

    std::vector<std::tuple<Token, int32_t>> token_hps_;
};

struct FlushHiddenDamangeAction
{
    std::string ToString() const { return ""; } // FlushHiddenDamangeAction will be replaced as AttackAction

    std::vector<Token> tokens_;
};

struct GoodNightAction
{
    std::string ToString() const { return "晚安"; }
};

using ActionVariant = std::variant<AttackAction, CurseAction, CureAction, BlockAttackAction, DetectAction,
      PassAction, ExocrismAction, ShieldAntiAction, AssignHiddenDamangeAction, FlushHiddenDamangeAction, GoodNightAction>;

class RoleManager;

static constexpr const int32_t k_heavy_hurt_hp = 25;
static constexpr const int32_t k_heavy_cure_hp = 15;
static constexpr const int32_t k_normal_hurt_hp = 15;
static constexpr const int32_t k_normal_cure_hp = 10;
static constexpr const int32_t k_civilian_dead_threshold = 2;
static constexpr const int32_t k_civilian_team_dead_threshold = 3;

struct RoleStatus
{
    int32_t hp_;
    ActionVariant action_;
};

struct RoleOption
{
    int32_t hp_;
    int32_t cure_count_; // -1 means no limit
};

enum Effect { CURSE, MAX_EFFECT };

static std::string TokenInfoForRole(const RoleManager& role_manager, const Occupation occupation);

class RoleBase
{
  protected:
    RoleBase(const std::optional<PlayerID> pid, const Token token, const Occupation occupation, const Team team, const RoleOption& option, RoleManager& role_manager)
        : pid_(pid)
        , token_(token)
        , occupation_(occupation)
        , role_manager_(role_manager)
        , next_round_team_(team)
        , team_(team)
        , hp_(option.hp_)
        , can_act_(true)
        , disable_act_when_refresh_(false)
        , is_alive_(true)
        , is_allowed_heavy_hurt_(false)
        , is_allowed_heavy_cure_(false)
        , is_winner_(false)
        , remain_cure_(option.cure_count_)
        , cur_action_(PassAction{})
        , effects_{0}
    {
    }

    RoleBase(const RoleBase&) = delete;
    RoleBase(RoleBase&&) = delete;

  public:
    virtual ~RoleBase() {}

    virtual bool Act(const AttackAction& action, MsgSenderBase& reply, StageUtility& utility);

    virtual bool Act(const CurseAction& action, MsgSenderBase& reply, StageUtility& utility)
    {
        reply() << "攻击失败：您无法使用魔法攻击";
        return false;
    }

    virtual bool Act(const CureAction& action, MsgSenderBase& reply, StageUtility& utility);

    virtual bool Act(const BlockAttackAction& action, MsgSenderBase& reply, StageUtility& utility)
    {
        reply() << "侦查失败：您无法执行该类型行动";
        return false;
    }

    virtual bool Act(const DetectAction& action, MsgSenderBase& reply, StageUtility& utility)
    {
        reply() << "侦查失败：您无法执行该类型行动";
        return false;
    }

    virtual bool Act(const ExocrismAction& action, MsgSenderBase& reply, StageUtility& utility)
    {
        reply() << "通灵失败：您无法执行该类型行动";
        return false;
    }

    virtual bool Act(const PassAction& action, MsgSenderBase& reply, StageUtility& utility)
    {
        reply() << "您本回合决定不行动";
        cur_action_ = action;
        return true;
    }

    virtual bool Act(const ShieldAntiAction& action, MsgSenderBase& reply, StageUtility& utility)
    {
        reply() << "盾反失败：您无法执行该类型行动";
        return false;
    }

    virtual bool Act(const AssignHiddenDamangeAction& action, MsgSenderBase& reply, StageUtility& utility)
    {
        reply() << "蓄力失败：您无法执行该类型行动";
        return false;
    }

    virtual bool Act(const FlushHiddenDamangeAction& action, MsgSenderBase& reply, StageUtility& utility)
    {
        reply() << "释放失败：您无法执行该类型行动";
        return false;
    }

    virtual bool Act(const GoodNightAction& action, MsgSenderBase& reply, StageUtility& utility)
    {
        if (team_ != Team::平民) {
            reply() << "晚安失败：您无法执行该类型行动";
            return false;
        }
        reply() << "晚安玛卡巴卡，您已经无法再行动了";
        DisableActWhenRoundEnd();
        assert(pid_.has_value());
        DisableAct();
        cur_action_ = action;
        return true;
    }

    virtual std::string PrivateInfo(const MainStage& main_stage) const
    {
        return std::string("您的代号是 ") + GetToken().ToChar() + "，职业是「" + GetOccupation().ToString() + "」";
    }

    virtual void OnRoundBegin()
    {
        team_ = next_round_team_;
    }

    // return true if dead in this round
    virtual bool OnRoundEnd()
    {
        if (!can_act_ && !is_alive_) {
            // both action and HP unchange os we need not push to |history_status_|
            return false;
        }
        if (disable_act_when_refresh_) {
            DisableAct();
            disable_act_when_refresh_ = false;
        }
        if (std::get_if<CureAction>(&cur_action_) && remain_cure_ > 0) {
            --remain_cure_;
        }
        history_status_.emplace_back(hp_, cur_action_);
        cur_action_ = PassAction{}; // change to default action
        if (hp_ <= 0 && is_alive_) {
            is_alive_ = false;
            return true;
        }
        return false;
    }

    void AddHp(const int32_t addition_hp) { hp_ += addition_hp; }
    void SetHp(const int32_t hp) { hp_ = hp; }
    void SetAllowHeavyHurt(const bool allow) { is_allowed_heavy_hurt_ = allow; }
    void SetAllowHeavyCure(const bool allow) { is_allowed_heavy_cure_ = allow; }
    void SetWinner(const bool is_winner) { is_winner_ = is_winner; }
    void DisableAct() { can_act_ = false; }
    void DisableActWhenRoundEnd() { disable_act_when_refresh_ = true; }

    std::optional<PlayerID> PlayerId() const { return pid_; }
    Token GetToken() const { return token_; }
    Occupation GetOccupation() const { return occupation_; }
    Team GetTeam() const { return team_; }
    void SetNextRoundTeam(const Team team) { next_round_team_ = team; }
    Team GetNextRoundTeam() const { return next_round_team_; }
    int32_t GetHp() const { return hp_; }
    bool CanAct() const { return can_act_; }
    bool IsAlive() const { return is_alive_; }
    bool IsWinner() const { return is_winner_; }
    int32_t RemainCure() const { return remain_cure_; }
    const ActionVariant& CurAction() const { return cur_action_; }
    const RoleStatus* GetHistoryStatus(const uint32_t idx) const
    {
        return idx < history_status_.size() ? &history_status_[idx] : nullptr;
    }
    uint32_t& EffectCount(const Effect effect) { return effects_[effect]; }

  protected:
    const std::optional<PlayerID> pid_;
    const Token token_;
    const Occupation occupation_;
    RoleManager& role_manager_;
    Team next_round_team_;
    Team team_;
    int32_t hp_;
    bool can_act_;
    bool disable_act_when_refresh_;
    bool is_alive_;
    bool is_allowed_heavy_hurt_;
    bool is_allowed_heavy_cure_;
    bool is_winner_;
    int32_t remain_cure_;
    ActionVariant cur_action_;
    std::vector<RoleStatus> history_status_;
    std::array<uint32_t, MAX_EFFECT> effects_;
};

class PuppetRole : public RoleBase
{
  public:
    PuppetRole(const Token token, const RoleOption& option, RoleManager& role_manager)
        : RoleBase(std::nullopt, token, Occupation::人偶, Team::特殊, option, role_manager)
    {
    }
};

class RoleManager
{
  public:
    using RoleVec = std::vector<std::unique_ptr<RoleBase>>;

    RoleManager(RoleVec roles) : roles_(std::move(roles))
    {
    }

    RoleBase& GetRole(const Token token) { return *roles_[token.id_]; }
    const RoleBase& GetRole(const Token token) const { return *roles_[token.id_]; }

    RoleBase& GetRole(const PlayerID pid)
    {
        return const_cast<RoleBase&>(const_cast<const RoleManager*>(this)->GetRole(pid));
    }

    const RoleBase& GetRole(const PlayerID pid) const
    {
        for (auto& role : roles_) {
            if (role->PlayerId() == pid) {
                return *role;
            }
        }
        assert(false);
        return *static_cast<RoleBase*>(nullptr);
    }

    RoleBase* GetRole(const Occupation occupation)
    {
        return const_cast<RoleBase*>(const_cast<const RoleManager*>(this)->GetRole(occupation));
    }

    const RoleBase* GetRole(const Occupation occupation) const
    {
        for (auto& role : roles_) {
            if (role->GetOccupation() == occupation) {
                return role.get();
            }
        }
        return nullptr;
    }


    bool IsValid(const Token token) { return token.id_ < roles_.size(); }

    template <typename Fn>
    void Foreach(const Occupation occupation, const Fn& fn) const
    {
        std::ranges::for_each(roles_, [&fn, occupation](const auto& role_p)
                {
                    if (role_p->GetOccupation() == occupation) {
                        fn(*role_p);
                    }
                });
    }

    template <typename Fn>
    void Foreach(const Team team, const Fn& fn) const
    {
        std::ranges::for_each(roles_, [&fn, team](const auto& role_p)
                {
                    if (role_p->GetTeam() == team) {
                        fn(*role_p);
                    }
                });
    }

    template <typename Fn>
    void Foreach(const Fn& fn) const { std::ranges::for_each(roles_, [&fn](const auto& role_p) { fn(*role_p); }); }

    uint32_t Size() const { return roles_.size(); }

  private:
    RoleVec roles_;
};

bool RoleBase::Act(const AttackAction& action, MsgSenderBase& reply, StageUtility& utility)
{
    if (action.token_hps_.size() != 1) {
        reply() << "攻击失败：您需要且只能攻击 1 名角色";
        return false;
    }
    const Token& token = std::get<Token>(action.token_hps_[0]);
    const int32_t hp = std::get<int32_t>(action.token_hps_[0]);
    auto& target = role_manager_.GetRole(token);
    if (!target.is_alive_) {
        reply() << "攻击失败：该角色已经死亡";
        return false;
    }
    if (is_allowed_heavy_hurt_ && hp != k_normal_hurt_hp && hp != k_heavy_hurt_hp) {
        reply() << "攻击失败：您只能造成 " << k_normal_hurt_hp << " 或 " << k_heavy_hurt_hp << " 点伤害";
        return false;
    }
    if (!is_allowed_heavy_hurt_ && hp != k_normal_hurt_hp) {
        reply() << "攻击失败：您只能造成 " << k_normal_hurt_hp << " 点伤害";
        return false;
    }
    reply() << "您本回合对角色 " << token.ToChar() << " 造成了 " << hp << " 点伤害";
    cur_action_ = action;
    return true;
}

bool RoleBase::Act(const CureAction& action, MsgSenderBase& reply, StageUtility& utility)
{
    auto& target = role_manager_.GetRole(action.token_);
    if (!target.is_alive_) {
        reply() << "治愈失败：该角色已经死亡";
        return false;
    }
    if (remain_cure_ == 0) {
        reply() << "治愈失败：您已经没有治愈的机会了";
        return false;
    }
    if (is_allowed_heavy_cure_ && action.hp_ != k_normal_cure_hp && action.hp_ != k_heavy_cure_hp) {
        reply() << "治愈失败：您只能治愈 " << k_normal_cure_hp << " 或 " << k_heavy_cure_hp << " 点血量";
        return false;
    }
    if (!is_allowed_heavy_cure_ && action.hp_ != k_normal_cure_hp) {
        reply() << "治愈失败：您只能治愈 " << k_normal_cure_hp << " 点血量";
        return false;
    }
    auto sender = reply();
    sender << "您本回合对角色 " << action.token_.ToChar() << " 治愈了 " << action.hp_ << " 点血量，您";
    if (remain_cure_ > 0) {
        sender << "还可治愈 " << (remain_cure_ - 1) << " 次";
    } else {
        sender << "没有治愈次数的限制";
    }
    cur_action_ = action;
    return true;
}

static std::string TokenInfoForTeam(const RoleManager& role_manager, const Team team)
{
    std::string s = std::string(team.ToString()) + "阵营的代号包括";
    role_manager.Foreach(team, [&](const auto& role)
        {
            s += ' ';
            s += role.GetToken().ToChar();
        });
    return s;
}

template <typename Fn>
static std::string InfoForRole(const RoleManager& role_manager, const Occupation occupation, const Fn& get_role_info, const char* const info_type) {
    std::string s;
    role_manager.Foreach(occupation, [&](const auto& role)
        {
            s += ' ';
            s += get_role_info(role);
        });
    if (s.empty()) {
        return std::string("场上没有") + occupation.ToString();
    }
    return std::string(occupation.ToString()) + "的" + info_type + "是" + s;
}

static std::string TokenInfoForRole(const RoleManager& role_manager, const Occupation occupation)
{
    return InfoForRole(role_manager, occupation, [](const RoleBase& role) { return role.GetToken().ToChar(); }, "代号");
}

template <size_t N> requires (N >= 2)
static std::string TokenInfoForRoles(const RoleManager& role_manager, const std::array<Occupation, N>& occupations)
{
    std::string s;
    const bool has_role = std::ranges::any_of(occupations,
            [&](const Occupation occupation) { return role_manager.GetRole(occupation); });
    if (!has_role) {
        s += "场上没有";
    }
    for (size_t i = 0; i < N; ++i) {
        if (i > 0) {
            s += "、";
        }
        s += occupations[i].ToString();
    }
    if (!has_role) {
        return s;
    }
    s += "的代号在";
    role_manager.Foreach([&](const auto& role)
        {
            if (std::ranges::find(occupations, role.GetOccupation()) != std::end(occupations)) {
                s += ' ';
                s += role.GetToken().ToChar();
            }
        });
    s += " 之间";
    return s;
}

enum class RoundResult { KILLER_WIN, CIVILIAN_WIN, DRAW, CONTINUE };

// ========== GAME STAGES ==========

class MainStage : public MainGameStage<>
{
  public:
    using RoleMaker = std::unique_ptr<RoleBase>(*)(uint64_t, Token, const RoleOption&, const uint64_t, RoleManager&);

    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility),
                MakeStageCommand(*this, "查看某名角色技能", &MainStage::RoleRule_, EnumChecker<Occupation>()),
                MakeStageCommand(*this, "查看当前游戏进展情况", &MainStage::Status_, VoidChecker("赛况")),
                MakeStageCommand(*this, "攻击某名角色", &MainStage::Hurt_, VoidChecker("攻击"),
                    RepeatableChecker<BasicChecker<Token>>("角色代号", "A"), ArithChecker<int32_t>(0, 25, "血量")),
                MakeStageCommand(*this, "[魔女] 诅咒某名角色", &MainStage::Curse_, VoidChecker("诅咒"),
                    BasicChecker<Token>("角色代号", "A"), ArithChecker<int32_t>(5, 10, "血量")),
                MakeStageCommand(*this, "治愈某名角色", &MainStage::Cure_, VoidChecker("治愈"),
                    BasicChecker<Token>("角色代号", "A"),
                    BoolChecker(std::to_string(k_heavy_cure_hp), std::to_string(k_normal_cure_hp))),
                MakeStageCommand(*this, "[侦探] 检查某名角色上一回合行动", &MainStage::Detect_, VoidChecker("侦查"),
                    BasicChecker<Token>("角色代号", "A")),
                MakeStageCommand(*this, "[替身] 替某名角色承担本回合伤害", &MainStage::BlockHurt_, VoidChecker("挡刀"),
                    BasicChecker<Token>("角色代号（若为空，则为杀手代号）", "A")),
                MakeStageCommand(*this, "[灵媒] 获取某名死者的职业", &MainStage::Exocrism_, VoidChecker("通灵"),
                    BasicChecker<Token>("角色代号", "A")),
                MakeStageCommand(*this, "[守卫] 盾反某几名角色", &MainStage::ShieldAnti_, VoidChecker("盾反"),
                    RepeatableChecker<BatchChecker<BasicChecker<Token>, ArithChecker<int32_t>>>(
                            BasicChecker<Token>("角色代号", "A"),
                            ArithChecker<int32_t>(-1000, 1000, "预测下一回合血量"))),
                MakeStageCommand(*this, "[特工] 为一名或多名角色分配隐藏伤害", &MainStage::AssignHiddenDamange_, VoidChecker("蓄力"),
                    RepeatableChecker<BatchChecker<BasicChecker<Token>, ArithChecker<int32_t>>>(
                        BasicChecker<Token>("角色代号", "A"), ArithChecker<int32_t>(5, 15, "血量"))),
                MakeStageCommand(*this, "[特工] 释放隐藏伤害", &MainStage::FlushHiddenDamange_, VoidChecker("释放"),
                    RepeatableChecker<BasicChecker<Token>>("角色代号", "A")),
                MakeStageCommand(*this, "认为已经达成平民胜利条件，不再警惕，放弃行动", &MainStage::GoodNight_, VoidChecker("晚安")),
                MakeStageCommand(*this, "跳过本回合行动", &MainStage::Pass_, VoidChecker("pass")))
#ifdef TEST_BOT
        , role_manager_(GAME_OPTION(身份列表).empty()
                ? GetRoleVec_(Global().Options(), DefaultRoleOption_(Global().Options()), Global().PlayerNum(), role_manager_)
                : LoadRoleVec_(GAME_OPTION(身份列表), DefaultRoleOption_(Global().Options()), role_manager_))
#else
        , role_manager_(GetRoleVec_(Global().Options(), DefaultRoleOption_(Global().Options()), Global().PlayerNum(), role_manager_))
#endif
        , k_image_width_((k_avatar_width_ + k_cellspacing_ + k_cellpadding_) * role_manager_.Size() + 150)
        , role_info_(RoleInfo_())
        , round_(1)
        , last_round_civilian_lost_(false)
        , last_round_killer_lost_(false)
        , last_round_traitor_lost_(false)
    {
    }

    virtual void OnStageBegin() override {
        {
            auto sender = Global().Boardcast();
            sender << "游戏开始，将私信各位玩家角色代号及职业\n\n";
            if (GAME_OPTION(晚安模式)) {
                sender << "需要注意，该局开启了「晚安模式」，「平民阵营」只有执行「晚安」指令才有可能取得胜利\n\n";
            }
            sender << "第 1 回合开始，请私信裁判行动";
        }
        role_manager_.Foreach([&](const auto& role)
            {
                if (role.PlayerId().has_value()) {
                    Global().Tell(*role.PlayerId()) << role.PrivateInfo(*this) << "\n\n" << k_role_rules[static_cast<uint32_t>(role.GetOccupation())];
                }
            });
        table_html_ = Html_(false);
        Global().StartTimer(GAME_OPTION(时限));
        RolesOnRoundBegin_();
        Global().Boardcast() << Markdown("## 第 1 回合\n\n" + table_html_, k_image_width_);
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        Global().HookUnreadyPlayers();
        return CheckoutErrCode::Condition(OnRoundFinish_(), StageErrCode::CHECKOUT, StageErrCode::CONTINUE);
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        if (rand() % 2) {
            Hurt_(pid, false, reply, {Token{static_cast<uint32_t>(rand() % Global().PlayerNum())}}, 15); // randomly hurt one role
        } else {
            Cure_(pid, false, reply, Token{static_cast<uint32_t>(rand() % Global().PlayerNum())}, false); // randomly hurt one role
        }
        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        return CheckoutErrCode::Condition(OnRoundFinish_(), StageErrCode::CHECKOUT, StageErrCode::CONTINUE);
    }

    virtual int64_t PlayerScore(const PlayerID pid) const override { return role_manager_.GetRole(pid).IsWinner(); }

    std::string PlayerInfoForRole(const RoleManager& role_manager, const Occupation occupation) const
    {
        return InfoForRole(role_manager, occupation, [this](const RoleBase& role)
                {
                    const auto& player_id = role.PlayerId();
                    return player_id.has_value() ? Global().PlayerName(*player_id) : "[NPC]";
                }, "中之人");
    }

  private:
    static RoleOption DefaultRoleOption_(const CustomOptions& option)
    {
        return RoleOption {
            .hp_ = GET_OPTION_VALUE(option, 血量),
            .cure_count_ = GET_OPTION_VALUE(option, 治愈次数),
        };
    }

    static std::string RoleAction(const RoleBase& role)
        {
            std::string s = "「";
            if (const auto action = std::get_if<AttackAction>(&role.CurAction())) {
                s += "攻击 " + std::string(1, std::get<Token>(action->token_hps_[0]).ToChar());
            } else if (const auto action = std::get_if<CurseAction>(&role.CurAction())) {
                s += "攻击 " + std::string(1, action->token_.ToChar());
            } else if (const auto action = std::get_if<CureAction>(&role.CurAction())) {
                s += "治愈 " + std::string(1, action->token_.ToChar());
            } else {
                s += "其它";
            }
            return s + "」";
        };

    void SettlementAction_()
    {
        // Multiple body doubles is forbidden.
        RoleBase* const hurt_blocker = role_manager_.GetRole(Occupation::替身);
        const BlockAttackAction* const block_hurt_action =
            hurt_blocker ? std::get_if<BlockAttackAction>(&hurt_blocker->CurAction()) : nullptr;
        const auto is_blocked_hurt = [&](const RoleBase& role)
            {
                return block_hurt_action && role.GetToken() == block_hurt_action->token_;
            };
        const auto is_avoid_hurt = [&](const RoleBase& hurter_role, const RoleBase& hurted_role)
            {
                if (hurter_role.GetOccupation() == Occupation::圣女 && hurted_role.GetTeam() == Team::平民) {
                    return true;
                }
                if (const auto action = std::get_if<AttackAction>(&hurted_role.CurAction());
                        hurted_role.GetOccupation() == Occupation::骑士 && hurter_role.GetToken() != hurted_role.GetToken() && action) {
                    for (const auto token_hp : action->token_hps_) {
                        if (std::get<Token>(token_hp) == hurter_role.GetToken()) {
                            Global().Tell(*hurted_role.PlayerId()) << "您反制 " << hurter_role.GetToken().ToChar() << " 的攻击成功并免受伤害";
                            return true;
                        }
                    }
                }
                return false;
            };

        std::vector<bool> be_attackeds(role_manager_.Size(), false);
        role_manager_.Foreach([&](auto& role)
            {
                if (const auto action = std::get_if<AttackAction>(&role.CurAction())) {
                    for (const auto& [token, hp]: action->token_hps_) {
                        auto& hurted_role = role_manager_.GetRole(token);
                        if (role.GetOccupation() == Occupation::恶灵 && hp == k_heavy_hurt_hp) {
                            role.AddHp(-25);
                        }
                        if (is_avoid_hurt(role, hurted_role)) {
                            be_attackeds[token.id_] = true;
                        } else if (is_blocked_hurt(hurted_role)) {
                            hurt_blocker->AddHp(-hp);
                        } else {
                            be_attackeds[token.id_] = true;
                            hurted_role.AddHp(-hp);
                        }
                    }
                } else if (const auto action = std::get_if<CureAction>(&role.CurAction())) {
                    role_manager_.GetRole(action->token_).AddHp(action->hp_);
                } else if (const auto action = std::get_if<DetectAction>(&role.CurAction())) {
                    auto& detected_role = role_manager_.GetRole(action->token_);
                    assert(role.PlayerId().has_value());
                    auto sender = Global().Tell(*role.PlayerId());
                    sender << "上一回合角色 " << action->token_.ToChar() << " 的行动是";
                    sender << RoleAction(detected_role);
                    if (!detected_role.IsAlive() && (std::get_if<AttackAction>(&detected_role.CurAction()) ||
                                std::get_if<CureAction>(&detected_role.CurAction()))) {
                        DisableAct_(detected_role, true);
                        sender << "，而且你完成了除灵，他已经失去行动能力了！";
                    }
                } else if (const auto action = std::get_if<ExocrismAction>(&role.CurAction())) {
                    auto& exocrism_role = role_manager_.GetRole(action->token_);
                    assert(role.PlayerId().has_value());
                    auto sender = Global().Tell(*role.PlayerId());
                    sender << "死亡角色 " << exocrism_role.GetToken().ToChar() << " 的职业是「" << exocrism_role.GetOccupation() << "」";
                    if (exocrism_role.GetOccupation() == Occupation::恶灵) {
                        DisableAct_(exocrism_role, true);
                        sender << "，他从下回合开始将失去行动能力！";
                    }
                }
            });

        SettlementCurse_(be_attackeds);
        SettlementShieldAnti_(is_blocked_hurt, is_avoid_hurt, be_attackeds);
    }

    void SettlementCurse_(std::vector<bool>& be_attackeds)
    {
        // settlement cure erase posion effect
        for (uint32_t id = 0; id < be_attackeds.size(); ++id) {
            if (be_attackeds[id]) {
                role_manager_.GetRole(Token{id}).EffectCount(CURSE) = 0; // attact action will clear curse effect
            }
        }

        // settlement curse effect
        role_manager_.Foreach([&](auto& role)
            {
                if (const auto action = std::get_if<CurseAction>(&role.CurAction());
                        role.GetOccupation() == Occupation::魔女 && action) {
                    role_manager_.GetRole(action->token_).EffectCount(CURSE) += action->hp_;
                }
            });
        role_manager_.Foreach([&](auto& role)
            {
                if (!std::holds_alternative<PassAction>(role.CurAction())) {
                    role.AddHp(-role.EffectCount(CURSE));
                }
            });
    }

    void SettlementShieldAnti_(const auto& is_blocked_hurt, const auto& is_avoid_hurt, std::vector<bool>& be_attackeds)
    {
        RoleBase* const guard_role = role_manager_.GetRole(Occupation::守卫); // there should be at most 1 guard
        if (const ShieldAntiAction* const shield_anti_action =
                guard_role ? std::get_if<ShieldAntiAction>(&guard_role->CurAction()) : nullptr) {
            std::vector<int32_t> hp_additions(role_manager_.Size(), 0);
            role_manager_.Foreach([&](auto& role)
                {
                    if (const int32_t* const hp_p = GetHpFromMultiTargetAction(role.GetToken(), shield_anti_action->token_hps_);
                            hp_p != nullptr && !is_blocked_hurt(role) && role.GetHp() == *hp_p) {
                        Global().Tell(*guard_role->PlayerId()) << "为角色 " << role.GetToken().ToChar() << " 盾反成功";
                        be_attackeds[role.GetToken().id_] = false;
                        role_manager_.Foreach([&](auto& hurter_role)
                            {
                                const AttackAction* const hurt_action = std::get_if<AttackAction>(&hurter_role.CurAction());
                                if (!hurt_action || is_avoid_hurt(hurter_role, role)) {
                                    return;
                                }
                                const auto it = std::ranges::find_if(hurt_action->token_hps_,
                                        [&](const auto& token_hp) { return std::get<Token>(token_hp) == role.GetToken(); });
                                if (it != hurt_action->token_hps_.end()) {
                                    // `hurter_role` has attacked `role`, we should reflect the damage.
                                    const int32_t hp = std::get<int32_t>(*it);
                                    hp_additions[role.GetToken().id_] += hp;
                                    hp_additions[hurter_role.GetToken().id_] -= hp;
                                }
                            });
                    }
                });
            role_manager_.Foreach([&](auto& role)
                {
                    role.AddHp(hp_additions[role.GetToken().id_]);
                });
        }
    }

    void RolesOnRoundBegin_()
    {
        role_manager_.Foreach([&](auto& role)
            {
                if (!role.CanAct() && role.PlayerId().has_value()) {
                    Global().Eliminate(*role.PlayerId());
                }
                role.OnRoundBegin();
            });
    }

    void RolesOnRoundEnd_(MsgSenderBase::MsgSenderGuard& sender)
    {
        bool has_dead = false;
        role_manager_.Foreach([&](auto& role)
            {
                if (!role.OnRoundEnd()) {
                    return; // not dead
                }
                if (std::exchange(has_dead, true) == false) {
                    sender << "\n";
                }
                sender << "\n- 角色 " << role.GetToken().ToChar() << " 死亡，";
                if (role.PlayerId().has_value()) {
                    sender << "他的「中之人」是" << At(*role.PlayerId());
                } else {
                    sender << "他是 NPC，没有「中之人」";
                }
                role_manager_.Foreach(Occupation::特工, [&](auto& agent_role)
                        {
                            if (agent_role.IsAlive() && agent_role.GetToken() != role.GetToken()) {
                                Global().Tell(*agent_role.PlayerId()) << "死亡角色 " << role.GetToken().ToChar() << " 的阵营是「"
                                                             << role.GetTeam().ToString() << "阵营」";
                            }
                        });
                if (role.GetOccupation() != Occupation::恶灵) {
                    DisableAct_(role);
                }
                RoleBase* other_role = nullptr;
                if (role.GetOccupation() == Occupation::杀手 &&
                        ((other_role = role_manager_.GetRole(Occupation::内奸)) || (other_role = role_manager_.GetRole(Occupation::初版内奸)))) {
                    other_role->SetAllowHeavyHurt(true);
                    other_role->SetAllowHeavyCure(true);
                    Global().Tell(*other_role->PlayerId()) << "杀手已经死亡，您获得了造成 " << k_heavy_hurt_hp
                            << " 点伤害和治愈 " << k_heavy_cure_hp << " 点 HP 的权利";
                } else if (((role.GetOccupation() == Occupation::双子（正） && (other_role = role_manager_.GetRole(Occupation::双子（邪）))) ||
                            (role.GetOccupation() == Occupation::双子（邪） && (other_role = role_manager_.GetRole(Occupation::双子（正）))))
                        && other_role->GetHp() > 0) { // cannot use IsAlive() because the is_alive_ field may have not been updated
                    other_role->SetNextRoundTeam(role.GetTeam());
                    Global().Tell(*other_role->PlayerId()) << "另一位双子死亡，您下一回合的阵营变更为：" << role.GetTeam() << "阵营";
                }
            });
    }

    bool CheckTeamsLost_(MsgSenderBase::MsgSenderGuard& sender)
    {
        bool killer_dead = true;
        bool traitor_dead = true;
        uint32_t civilian_dead_count = 0;
        uint32_t civilian_team_dead_count = 0;
        uint32_t civilian_team_alive_count = 0;
        uint32_t civilian_team_alive_cannot_act_count = 0;
        role_manager_.Foreach([&](const auto& role)
            {
                if (role.IsAlive()) {
                    if (role.GetOccupation() == Occupation::内奸 || role.GetOccupation() == Occupation::初版内奸) {
                        traitor_dead = false;
                    }
                    if (role.GetOccupation() == Occupation::杀手) {
                        killer_dead = false;
                    }
                    if (role.GetNextRoundTeam() == Team::平民) {
                        // civilian twin can convert to killer team next round, so we check the next-round team instead of current team
                        ++civilian_team_alive_count;
                        civilian_team_alive_cannot_act_count += !role.CanAct();
                    }
                    return;
                } else if (role.GetTeam() == Team::平民) {
                    ++civilian_team_dead_count;
                    if (role.GetOccupation() == Occupation::平民) {
                        ++civilian_dead_count;
                    }
                }
            });
        bool civilian_lost = civilian_dead_count >= k_civilian_dead_threshold ||
                             civilian_team_dead_count >= k_civilian_team_dead_threshold ||
                             civilian_team_alive_count == 0;
        bool killer_lost = killer_dead &&
                           (!GAME_OPTION(晚安模式) || civilian_team_alive_cannot_act_count > (civilian_team_alive_count / 2));
        bool traitor_lost = traitor_dead;

        if (const auto role = role_manager_.GetRole(Occupation::特工); civilian_lost && killer_lost && role != nullptr) {
            sender << "游戏结束，杀手阵营和平民阵营同时失败，特工取得胜利";
            role_manager_.Foreach(Occupation::特工, [&](auto& role) { role.SetWinner(true); });
            return true;
        }

        switch (!civilian_lost + !killer_lost + !traitor_lost) {
        case 0: // multiple teams lost at the same time
            sender << "游戏结束，多个阵营的失败条件同时满足，此时根据优先级，判定";
            if (!last_round_traitor_lost_ &&
                    (role_manager_.GetRole(Occupation::内奸) || role_manager_.GetRole(Occupation::初版内奸))) { // traitor lost at this round
                traitor_lost = false;
                sender << "内奸";
            } else if (!last_round_killer_lost_) { // killer lost at this round
                killer_lost = false;
                sender << "杀手阵营";
            } else {
                assert(false);
            }
            sender << "胜利";
            break;

        case 1: // only one team wins
            sender << "游戏结束，";
            if (!traitor_lost) {
                sender << "内奸";
            } else if (!killer_lost) {
                sender << "杀手阵营";
            } else {
                sender << "平民阵营";
            }
            sender << "胜利";
            break;

        default:
            if ((++round_) > GAME_OPTION(回合数)) {
                --round_;
                sender << "游戏达到最大回合限制，游戏平局";
            } else {
                role_manager_.Foreach([&](auto& role)
                    {
                        if ((!last_round_civilian_lost_ && civilian_lost && role.GetTeam() == Team::平民) ||
                                (!last_round_killer_lost_ && killer_lost && role.GetTeam() == Team::杀手)) {
                            if (role.PlayerId().has_value()) {
                                Global().Tell(*role.PlayerId()) << "很遗憾，您所在的阵营失败了";
                            }
                            if (role.CanAct()) {
                                DisableAct_(role);
                            }
                        }
                    });
                last_round_civilian_lost_ = civilian_lost;
                last_round_killer_lost_ = killer_lost;
                last_round_traitor_lost_ = traitor_lost;
                sender << "游戏继续，第 " << round_ << " 回合开始，请私信裁判行动";
                return false;
            }
        }

        role_manager_.Foreach([&](auto& role)
            {
                if (role.GetTeam() == Team::平民) {
                    role.SetWinner(!civilian_lost);
                } else if (role.GetTeam() == Team::杀手) {
                    role.SetWinner(!killer_lost);
                } else if (role.GetOccupation() == Occupation::内奸 || role.GetOccupation() == Occupation::初版内奸) {
                    role.SetWinner(!traitor_lost);
                } else if (role.GetOccupation() != Occupation::人偶 && role.GetOccupation() != Occupation::特工) {
                    assert(false);
                }
            });

        return true;
    }

    bool Settlement_()
    {
        auto sender = Global().Boardcast();
        sender << "第 " << round_ << " 回合结束，下面公布各角色血量：";
        SettlementAction_();
        RolesOnRoundEnd_(sender);
        sender << "\n\n";
        return CheckTeamsLost_(sender);
    }

    void DisableAct_(RoleBase& role, const bool delay_to_refresh = false)
    {
        if (delay_to_refresh) {
            // Some logic may depend on the value of |role.can_act_| so we cannot modify it immediately.
            // For example, the ghost will be disabled acting when being exorcismed. If we modify the
            // |role.can_act_| immediately, the action will not be emplaced into |role.history_status_|.
            role.DisableActWhenRoundEnd();
        } else {
            role.DisableAct();
        }
        if (role.PlayerId()) {
            Global().Tell(*role.PlayerId()) << "您失去了行动能力";
            Global().Eliminate(*role.PlayerId());
        }
    }

    template <typename RoleType>
    static std::unique_ptr<RoleBase> MakeRole_(const uint64_t pid, const Token token, const RoleOption& option, const uint64_t role_num, RoleManager& role_manager)
    {
        return std::make_unique<RoleType>(pid, token, option, role_num, role_manager);
    }

    static RoleManager::RoleVec LoadRoleVec_(const std::vector<Occupation>& occupation_list, const RoleOption& option, RoleManager& role_manager)
    {
        RoleManager::RoleVec v;
        for (uint32_t i = 0, pid = 0; i < occupation_list.size(); ++i) {
            if (occupation_list[i] == Occupation::人偶) {
                v.emplace_back(std::make_unique<PuppetRole>(Token(i), option, role_manager));
            } else {
                v.emplace_back(k_role_makers_[occupation_list[i].ToUInt()](pid++, Token(i), option, occupation_list.size(), role_manager));
            }
        }
        return v;
    }

    static RoleManager::RoleVec GetRoleVec_(const CustomOptions& option, const RoleOption& role_option, const uint32_t player_num, RoleManager& role_manager)
    {
        const auto make_roles = [&]<typename T>(const std::initializer_list<T>& occupation_lists)
            {
                assert(occupation_lists.size() > 0);
                std::random_device rd;
                std::mt19937 g(rd());
                const auto& occupation_list = std::data(occupation_lists)[std::uniform_int_distribution<int>(0, occupation_lists.size() - 1)(rd)];
                std::vector<PlayerID> pids;
                for (uint32_t i = 0; i < player_num; ++i) {
                    pids.emplace_back(i);
                }
                std::vector<Token> tokens;
                for (uint32_t i = 0; i < occupation_list.size(); ++i) {
                    tokens.emplace_back(i);
                }
                std::ranges::shuffle(pids, g);
                std::ranges::shuffle(tokens, g);
                RoleManager::RoleVec v;
                for (size_t i = 0, pid_i = 0; i < occupation_list.size(); ++i) {
                    const auto occupation = std::data(occupation_list)[i];
                    if (occupation == Occupation::人偶) {
                        v.emplace_back(std::make_unique<PuppetRole>(tokens[i], role_option, role_manager));
                    } else {
                        v.emplace_back(k_role_makers_[static_cast<uint32_t>(occupation)](pids[pid_i++], tokens[i], role_option, occupation_list.size(), role_manager));
                    }
                }
                std::ranges::sort(v, [](const auto& _1, const auto& _2) { return _1->GetToken() < _2->GetToken(); });
                return v;
            };
        if (const auto& occupation_list = GetOccupationList(option, player_num); !occupation_list.empty()) {
            return make_roles({occupation_list});
        }
        switch (player_num) {
        case 5: return make_roles(std::initializer_list<std::initializer_list<Occupation>>{
                    {Occupation::杀手, Occupation::恶灵, Occupation::守卫, Occupation::平民, Occupation::初版内奸},
                });
        case 6: return make_roles(std::initializer_list<std::initializer_list<Occupation>>{
                    {Occupation::杀手, Occupation::刺客, Occupation::双子（邪）, Occupation::双子（正）, Occupation::侦探, Occupation::圣女},
                });
        case 7: return make_roles(std::initializer_list<std::initializer_list<Occupation>>{
                    {Occupation::杀手, Occupation::替身, Occupation::侦探, Occupation::圣女, Occupation::平民, Occupation::平民, Occupation::内奸},
                    {Occupation::杀手, Occupation::替身, Occupation::侦探, Occupation::圣女, Occupation::平民, Occupation::平民, Occupation::特工},
                });
        case 8: return make_roles(std::initializer_list<std::initializer_list<Occupation>>{
                    {Occupation::杀手, Occupation::替身, Occupation::刺客, Occupation::侦探, Occupation::圣女, Occupation::守卫, Occupation::平民, Occupation::平民, Occupation::人偶},
                    {Occupation::杀手, Occupation::替身, Occupation::恶灵, Occupation::侦探, Occupation::圣女, Occupation::灵媒, Occupation::平民, Occupation::平民, Occupation::人偶},
                    {Occupation::杀手, Occupation::替身, Occupation::魔女, Occupation::侦探, Occupation::圣女, Occupation::骑士, Occupation::平民, Occupation::平民, Occupation::人偶},
                });
        case 9: return make_roles(std::initializer_list<std::initializer_list<Occupation>>{
                    {Occupation::杀手, Occupation::替身, Occupation::刺客, Occupation::侦探, Occupation::圣女, Occupation::守卫, Occupation::平民, Occupation::平民, Occupation::内奸},
                    {Occupation::杀手, Occupation::替身, Occupation::恶灵, Occupation::侦探, Occupation::圣女, Occupation::灵媒, Occupation::平民, Occupation::平民, Occupation::内奸},
                    {Occupation::杀手, Occupation::替身, Occupation::魔女, Occupation::侦探, Occupation::圣女, Occupation::骑士, Occupation::平民, Occupation::平民, Occupation::内奸},
                    {Occupation::杀手, Occupation::替身, Occupation::刺客, Occupation::侦探, Occupation::圣女, Occupation::守卫, Occupation::平民, Occupation::平民, Occupation::特工},
                    {Occupation::杀手, Occupation::替身, Occupation::恶灵, Occupation::侦探, Occupation::圣女, Occupation::灵媒, Occupation::平民, Occupation::平民, Occupation::特工},
                    {Occupation::杀手, Occupation::替身, Occupation::魔女, Occupation::侦探, Occupation::圣女, Occupation::骑士, Occupation::平民, Occupation::平民, Occupation::特工},
                });
        default:
            assert(false);
            return {};
        }
    }

    std::string Image_(const char* const name, const int32_t width) const
    {
        return std::string("<img src=\"file:///") + Global().ResourceDir() + "/" + name + ".png\" style=\"width:" +
            std::to_string(width) + "px; vertical-align: middle;\">";
    }

    std::string RoleInfo_() const
    {
        std::string s = HTML_SIZE_FONT_HEADER(4) "<b>本场游戏包含职业：";
        std::vector<Occupation> occupations;
        role_manager_.Foreach([&](const auto& role) { occupations.emplace_back(role.GetOccupation()); });
        std::ranges::sort(occupations);
        for (const auto& occupation : occupations) {
            if (occupation == Occupation::杀手 || occupation == Occupation::替身 || occupation == Occupation::恶灵 ||
                    occupation == Occupation::刺客 || occupation == Occupation::双子（邪） || occupation == Occupation::魔女) {
                s += HTML_COLOR_FONT_HEADER(red);
            } else if (occupation == Occupation::内奸 || occupation == Occupation::初版内奸 || occupation == Occupation::特工 ||
                    occupation == Occupation::人偶) {
                s += HTML_COLOR_FONT_HEADER(blue);
            } else {
                s += HTML_COLOR_FONT_HEADER(black);
            }
            s += occupation.ToString();
            if (occupation == Occupation::人偶) {
                s += "（NPC）";
            }
            s += HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE;
        }
        s += "</b>" HTML_FONT_TAIL;
        return s;
    }

    std::string Html_(const bool with_action) const
    {
        const char* const k_dark_blue = "#7092BE";
        const char* const k_middle_grey = "#E0E0E0";
        const char* const k_light_grey = "#F5F5F5";

        html::Table table(0, role_manager_.Size() + 1);
        table.SetTableStyle(" align=\"center\" cellspacing=\"" + std::to_string(k_cellspacing_) + "\" cellpadding=\"" +
                std::to_string(k_cellpadding_) + "\"");

        const auto new_line = [&](const std::string_view title, const char* const color, const auto fn)
            {
                table.AppendRow();
                table.GetLastRow(0).SetContent("**" + std::string(title) + "**");
                table.GetLastRow(0).SetColor(color);
                for (uint32_t token_id = 0; token_id < role_manager_.Size(); ++token_id) {
                    const auto& role = role_manager_.GetRole(Token{token_id});
                    auto& box = table.Get(table.Row() - 1, token_id + 1);
                    box.SetColor(role.IsAlive() ? k_dark_blue : k_middle_grey);
                    fn(box, role);
                }
            };
        new_line("玩家", k_dark_blue, [&](html::Box& box, const RoleBase& role)
                {
                    const auto image = (role.IsAlive() && !with_action) ? Image_("unknown_player", k_avatar_width_)       :
                                       role.PlayerId().has_value()      ? Global().PlayerAvatar(*role.PlayerId(), k_avatar_width_) :
                                       "<p style=\"width:" + std::to_string(k_avatar_width_) + "px;\"></p>";
                    box.SetContent(image);
                });
        new_line("角色代号", k_dark_blue, [&](html::Box& box, const RoleBase& role)
                {
                    box.SetContent(std::string("<font size=\"6\"> **") + role.GetToken().ToChar() + "** ");
                });
        new_line("职业", k_dark_blue, [&](html::Box& box, const RoleBase& role)
                {
                    box.SetContent(std::string("<font size=\"5\"> **") +
                            (with_action ? role.GetOccupation().ToString() : "??") +
                            "** " HTML_FONT_TAIL);
                });
        new_line("初始状态", k_light_grey, [&](html::Box& box, const RoleBase& role)
                {
                    box.SetContent("<p align=\"left\"><font size=\"4\">" +
                            Image_("blank", k_icon_size_) + std::to_string(GAME_OPTION(血量)) + "</font></p>");
                    box.SetColor(k_light_grey);
                });
        for (uint32_t r = 0; r < (with_action ? round_ : round_ - 1); ++r) {
            table.AppendRow();
            table.GetLastRow(0).SetContent("**第 " + std::to_string(r + 1) + " 回合**");
            table.GetLastRow(0).SetColor(r % 2 ? k_light_grey : k_middle_grey);
            if (with_action) {
                for (uint32_t token_id = 0; token_id < role_manager_.Size(); ++token_id) {
                    const auto status = role_manager_.GetRole(Token{token_id}).GetHistoryStatus(r);
                    table.Get(table.Row() - 1, token_id + 1).SetColor(r % 2 ? k_light_grey : k_middle_grey);
                    if (!status) {
                        continue;
                    }
                    table.Get(table.Row() - 1, token_id + 1).SetContent("<b>" +
                            std::visit([](const auto& action) { return action.ToString(); }, status->action_) + "</b>");
                }
                table.AppendRow();
                table.MergeDown(table.Row() - 2, 0, 2);
            }
            for (uint32_t token_id = 0; token_id < role_manager_.Size(); ++token_id) {
                const auto& role = role_manager_.GetRole(Token{token_id});
                const auto status = role.GetHistoryStatus(r);
                table.Get(table.Row() - 1, token_id + 1).SetColor(r % 2 ? k_light_grey : k_middle_grey);
                if (!status) {
                    continue;
                }
                const auto last_hp = r == 0 ? GAME_OPTION(血量) : role.GetHistoryStatus(r - 1)->hp_;
                if (!with_action && last_hp <= 0) { // hind the hp from dead role to protect Ghost's identity
                    continue;
                }
                const auto image = Image_(
                    last_hp > 0 && status->hp_ <= 0 ? "dead" :
                    last_hp < status->hp_ ? "up"   :
                    last_hp > status->hp_ ? "down" : "blank", k_icon_size_);
                table.Get(table.Row() - 1, token_id + 1).SetContent("<p align=\"left\"><font size=\"4\">" +
                        image + std::to_string(status->hp_) + "</font></p>");
            }
        }

        std::string s = role_info_ + "\n\n" + table.ToString() + "\n\n";
        for (uint32_t token_id = 0; token_id < role_manager_.Size(); ++token_id) {
            if (const auto& role = role_manager_.GetRole(Token{token_id}); role.GetOccupation() == Occupation::骑士) {
                s += "\n- 骑士的中之人是：";
                s += Global().PlayerAvatar(*role.PlayerId(), 30);
                s += HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE;
                s += Global().PlayerName(*role.PlayerId());
            }
        }

        return s;
    }

    AtomReqErrCode RoleRule_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const Occupation& occupation)
    {
        reply() << k_role_rules[static_cast<uint32_t>(occupation)];
        return StageErrCode::OK;
    }

    AtomReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (!is_public) {
            const auto& role = role_manager_.GetRole(pid);
            reply() << role.PrivateInfo(*this) << "，剩余 "
                << (std::get_if<CureAction>(&role.CurAction()) ? role.RemainCure() - 1 : role.RemainCure()) << " 次治愈机会";
        }
        reply() << Markdown("## 第 " + std::to_string(round_) + " 回合\n\n" + table_html_, k_image_width_);
        return StageErrCode::OK;
    }

    bool OnRoundFinish_()
    {
        if (!Settlement_()) {
            table_html_ = Html_(false);
            Global().Boardcast() << Markdown("## 第 " + std::to_string(round_) + " 回合\n\n" + table_html_, k_image_width_);
            Global().ClearReady();
            Global().StartTimer(GAME_OPTION(时限));
            RolesOnRoundBegin_();
            return false;
        }
        Global().Boardcast() << Markdown("## 终局\n\n" + Html_(true), k_image_width_);
        return true;
    }

    AtomReqErrCode GenericAct_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const ActionVariant& action)
    {
        if (is_public) {
            reply() << "行动失败：请您私信裁判行动";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "行动失败：您已完成行动，或您不需要行动";
            return StageErrCode::FAILED;
        }
        auto& role = role_manager_.GetRole(pid);
        if (!role.CanAct()) {
            reply() << "行动失败：您已经失去了行动能力";
            return StageErrCode::FAILED; // should not appened for user player
        }
        if (!std::visit(
                    [&role, &reply, &utility = Global()](auto& action) { return role.Act(action, reply, utility); },
                    action)) {
            return StageErrCode::FAILED;
        }
        return StageErrCode::READY;
    }

    template <typename Tokens>
    bool CheckMultipleTokens_(const Tokens& tokens, const char* const action_name, MsgSenderBase& reply)
    {
        if (tokens.empty()) {
            reply() << action_name << "失败：需要至少指定 1 名角色";
            return false;
        }
        for (uint32_t i = 0; i < tokens.size(); ++i) {
            if (!CheckToken(tokens[i], action_name, reply)) {
                return false;
            }
            for (uint32_t j = i + 1; j < tokens.size(); ++j) {
                if (tokens[i] == tokens[j]) {
                    reply() << action_name << "失败：角色 " << tokens[i].ToChar() << " 被多次指定";
                    return false;
                }
            }
        }
        return true;
    }

    bool CheckToken(const Token token, const char* const action_name, MsgSenderBase& reply)
    {
        if (!role_manager_.IsValid(token)) {
            reply() << action_name << "失败：场上没有角色 " << token.ToChar();
            return false;
        }
        if (!role_manager_.GetRole(token).IsAlive()) {
            reply() << action_name << "失败：角色 " << token.ToChar() << " 已经死亡";
            return false;
        }
        return true;
    }

    AtomReqErrCode Hurt_(const PlayerID pid, const bool is_public, MsgSenderBase& reply,
            const std::vector<Token>& tokens, const int32_t hp)
    {
        if (!CheckMultipleTokens_(tokens, "攻击", reply)) {
            return StageErrCode::FAILED;
        }
        AttackAction action;
        for (const auto& token : tokens) {
            action.token_hps_.emplace_back(token, hp);
        }
        return GenericAct_(pid, is_public, reply, std::move(action));
    }

    AtomReqErrCode Curse_(const PlayerID pid, const bool is_public, MsgSenderBase& reply,
            const Token token, const int32_t hp)
    {
        if (!CheckToken(token, "治愈", reply)) {
            return StageErrCode::FAILED;
        }
        return GenericAct_(pid, is_public, reply, CurseAction{.token_ = token, .hp_ = hp});
    }

    AtomReqErrCode Cure_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const Token token, const bool is_heavy)
    {
        if (!CheckToken(token, "治愈", reply)) {
            return StageErrCode::FAILED;
        }
        return GenericAct_(pid, is_public, reply, CureAction{.token_ = token, .hp_ = is_heavy ? k_heavy_cure_hp : k_normal_cure_hp});
    }

    AtomReqErrCode Detect_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const Token token)
    {
        // detecting dead roles is valid
        if (!role_manager_.IsValid(token)) {
            reply() << "侦查失败：场上没有该角色";
            return StageErrCode::FAILED;
        }
        return GenericAct_(pid, is_public, reply, DetectAction{.token_ = token});
    }

    AtomReqErrCode BlockHurt_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const Token& token)
    {
        if (!role_manager_.IsValid(token)) {
            reply() << "挡刀失败：场上没有该角色";
            return StageErrCode::FAILED;
        }
        return GenericAct_(pid, is_public, reply, BlockAttackAction{token});
    }

    AtomReqErrCode Exocrism_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const Token token)
    {
        if (!role_manager_.IsValid(token)) {
            reply() << "通灵失败：场上没有该角色";
            return StageErrCode::FAILED;
        }
        if (role_manager_.GetRole(token).IsAlive()) {
            reply() << "通灵失败：只能通灵死亡角色";
            return StageErrCode::FAILED;
        }
        return GenericAct_(pid, is_public, reply, ExocrismAction{.token_ = token});
    }

    AtomReqErrCode ShieldAnti_(const PlayerID pid, const bool is_public, MsgSenderBase& reply,
            const std::vector<std::tuple<Token, int32_t>>& token_hps)
    {
        if (!CheckMultipleTokens_(token_hps | std::views::transform([](const auto& tuple) { return std::get<Token>(tuple); }),
                    "盾反", reply)) {
            return StageErrCode::FAILED;
        }
        return GenericAct_(pid, is_public, reply, ShieldAntiAction{.token_hps_ = token_hps});
    }

    AtomReqErrCode AssignHiddenDamange_(const PlayerID pid, const bool is_public, MsgSenderBase& reply,
            const std::vector<std::tuple<Token, int32_t>>& token_hps)
    {
        if (!CheckMultipleTokens_(token_hps | std::views::transform([](const auto& tuple) { return std::get<Token>(tuple); }),
                    "蓄力", reply)) {
            return StageErrCode::FAILED;
        }
        return GenericAct_(pid, is_public, reply, AssignHiddenDamangeAction{.token_hps_ = token_hps});
    }

    AtomReqErrCode FlushHiddenDamange_(const PlayerID pid, const bool is_public, MsgSenderBase& reply,
            const std::vector<Token>& tokens)
    {
        if (!CheckMultipleTokens_(tokens, "释放", reply)) {
            return StageErrCode::FAILED;
        }
        return GenericAct_(pid, is_public, reply, FlushHiddenDamangeAction{.tokens_ = tokens});
    }

    AtomReqErrCode GoodNight_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (!GAME_OPTION(晚安模式)) {
            reply() << "晚安失败：当前游戏模式下无需晚安";
            return StageErrCode::FAILED;
        }
        return GenericAct_(pid, is_public, reply, GoodNightAction{});
    }

    AtomReqErrCode Pass_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        return GenericAct_(pid, is_public, reply, PassAction{});
    }

    static RoleMaker k_role_makers_[Occupation::Count()];
    static constexpr const uint32_t k_avatar_width_ = 80;
    static constexpr const uint32_t k_cellspacing_ = 3;
    static constexpr const uint32_t k_cellpadding_ = 1;
    static constexpr const uint32_t k_icon_size_ = 40;
    RoleManager role_manager_;
    const uint32_t k_image_width_;
    const std::string role_info_;
    int round_;
    std::string table_html_;

    bool last_round_civilian_lost_;
    bool last_round_killer_lost_;
    bool last_round_traitor_lost_;
};


class KillerRole : public RoleBase
{
  public:
    KillerRole(const uint64_t pid, const Token token, const RoleOption& option, const uint64_t role_num, RoleManager& role_manager)
        : RoleBase(pid, token, Occupation::杀手, Team::杀手, option, role_manager)
    {
        is_allowed_heavy_hurt_ = true;
        is_allowed_heavy_cure_ = true;
    }

    virtual std::string PrivateInfo(const MainStage& main_stage) const
    {
        if (!private_info_.empty()) {
            return private_info_;
        }
        return RoleBase::PrivateInfo(main_stage) + "，" + TokenInfoForTeam(role_manager_, Team::平民);
    }

  private:
    std::string private_info_;
};

class BodyDoubleRole : public RoleBase
{
  public:
    BodyDoubleRole(const uint64_t pid, const Token token, const RoleOption& option, const uint64_t role_num, RoleManager& role_manager)
        : RoleBase(pid, token, Occupation::替身, Team::杀手, option, role_manager)
    {
    }

    virtual std::string PrivateInfo(const MainStage& main_stage) const
    {
        if (main_stage.Global().PlayerNum() > 5) {
            return RoleBase::PrivateInfo(main_stage) + "，" + TokenInfoForTeam(role_manager_, Team::平民);
        }
        return RoleBase::PrivateInfo(main_stage);
    }

    virtual bool Act(const BlockAttackAction& action, MsgSenderBase& reply, StageUtility& utility) override
    {
        reply() << "请做好觉悟，本回合对该角色造成的全部伤害将转移到您身上";
        cur_action_ = action;
        return true;
    }
};

class GhostRole : public RoleBase
{
  public:
    GhostRole(const uint64_t pid, const Token token, const RoleOption& option, const uint64_t role_num, RoleManager& role_manager)
        : RoleBase(pid, token, Occupation::恶灵, Team::杀手, option, role_manager)
    {
    }

    virtual std::string PrivateInfo(const MainStage& main_stage) const
    {
        if (main_stage.Global().PlayerNum() > 5) {
            return RoleBase::PrivateInfo(main_stage) + "，" + TokenInfoForTeam(role_manager_, Team::平民);
        }
        return RoleBase::PrivateInfo(main_stage);
    }
};

class AssassinRole : public RoleBase
{
  public:
    AssassinRole(const uint64_t pid, const Token token, RoleOption option, const uint64_t role_num, RoleManager& role_manager)
        : RoleBase(pid, token, Occupation::刺客, Team::杀手, option, role_manager)
    {
    }

    virtual std::string PrivateInfo(const MainStage& main_stage) const
    {
        if (main_stage.Global().PlayerNum() > 5) {
            return RoleBase::PrivateInfo(main_stage) + "，" + TokenInfoForTeam(role_manager_, Team::平民);
        }
        return RoleBase::PrivateInfo(main_stage);
    }

    virtual bool Act(const AttackAction& action, MsgSenderBase& reply, StageUtility& utility) override
    {
        assert(!action.token_hps_.empty());
        assert(std::all_of(action.token_hps_.begin(), action.token_hps_.end(),
                    [&](const auto& token_hp) { return std::get<int32_t>(token_hp) == std::get<int32_t>(action.token_hps_[0]); }));
        int32_t max_target_num = 0;
        const int32_t hp = std::get<int32_t>(action.token_hps_[0]);
        switch (hp) {
        case 0:
            max_target_num = 1;
            break;
        case 5:
            max_target_num = 5;
            break;
        case 10:
            max_target_num = 2;
            break;
        case 15:
            max_target_num = 1;
            break;
        default:
            reply() << "攻击失败：您只能造成 0 / 5 / 10 / 15 点伤害";
            return false;
        }
        if (action.token_hps_.size() > max_target_num) {
            reply() << "攻击失败：伤害值为 " << hp << " 时最多指定 " << max_target_num << " 名角色";
            return false;
        }

        cur_action_ = action;

        auto sender = reply();
        sender << "您本回合分别对角色";
        for (const auto& token_hp : action.token_hps_) {
            sender << " " << std::get<Token>(token_hp).ToChar();
        }
        sender << " 造成了 " << hp << " 点伤害";

        return true;
    }
};

class WitchRole : public RoleBase
{
  public:
    WitchRole(const uint64_t pid, const Token token, const RoleOption& option, const uint64_t role_num, RoleManager& role_manager)
        : RoleBase(pid, token, Occupation::魔女, Team::杀手, option, role_manager)
    {
    }

    virtual std::string PrivateInfo(const MainStage& main_stage) const
    {
        if (main_stage.Global().PlayerNum() > 5) {
            return RoleBase::PrivateInfo(main_stage) + "，" + TokenInfoForTeam(role_manager_, Team::平民);
        }
        return RoleBase::PrivateInfo(main_stage);
    }

    virtual bool Act(const AttackAction& action, MsgSenderBase& reply, StageUtility& utility) override
    {
        reply() << "攻击失败：您无法使用物理攻击";
        return false;
    }

    virtual bool Act(const CurseAction& action, MsgSenderBase& reply, StageUtility& utility) override
    {
        auto& target = role_manager_.GetRole(action.token_);
        cur_action_ = action;
        if (!target.IsAlive()) {
            reply() << "诅咒失败：该角色已经死亡";
            return false;
        }
        if (action.hp_ != 5 && action.hp_ != 10) {
            reply() << "诅咒失败：诅咒的血量只能为 5 或 10";
            return false;
        }
        reply() << "您本回合诅咒角色 " << action.token_.ToChar() << " 成功";
        return true;
    }
};

class CivilianRole : public RoleBase
{
  public:
    CivilianRole(const uint64_t pid, const Token token, const RoleOption& option, const uint64_t role_num, RoleManager& role_manager)
        : RoleBase(pid, token, Occupation::平民, Team::平民, option, role_manager)
    {
    }
};

class GoddessRole : public RoleBase
{
  public:
    GoddessRole(const uint64_t pid, const Token token, RoleOption option, const uint64_t role_num, RoleManager& role_manager)
        : RoleBase(pid, token, Occupation::圣女, Team::平民, (option.cure_count_ = -1, option), role_manager)
    {
    }

    virtual bool Act(const AttackAction& action, MsgSenderBase& reply, StageUtility& utility) override
    {
        if (!history_status_.empty() && std::get_if<AttackAction>(&history_status_.back().action_)) {
            reply() << "攻击失败：您无法连续两回合进行攻击";
            return false;
        }
        return RoleBase::Act(action, reply, utility);
    }
};

class DetectiveRole : public RoleBase
{
  public:
    DetectiveRole(const uint64_t pid, const Token token, const RoleOption& option, const uint64_t role_num, RoleManager& role_manager)
        : RoleBase(pid, token, Occupation::侦探, Team::平民, option, role_manager)
    {
    }

  public:
    virtual bool Act(const DetectAction& action, MsgSenderBase& reply, StageUtility& utility) override
    {
        if (history_status_.empty()) {
            reply() << "侦查失败：首回合无法侦查";
            return false;
        }
        if (!history_status_.empty() && std::get_if<DetectAction>(&history_status_.back().action_)) {
            reply() << "侦查失败：您无法连续两回合进行侦查";
            return false;
        }
        reply() << "您选择侦查角色 " << action.token_.ToChar() << "，本回合结束后将私信您他的行动";
        cur_action_ = action;
        return true;
    }
};

class SorcererRole : public RoleBase
{
  public:
    SorcererRole(const uint64_t pid, const Token token, const RoleOption& option, const uint64_t role_num, RoleManager& role_manager)
        : RoleBase(pid, token, Occupation::灵媒, Team::平民, option, role_manager)
        , exocrismed_(false)
    {
    }

    virtual bool Act(const ExocrismAction& action, MsgSenderBase& reply, StageUtility& utility) override
    {
        if (exocrismed_) {
            reply() << "通灵失败：您本局游戏已经通灵过一次了";
            return false;
        }
        reply() << "您选择通灵角色 " << action.token_.ToChar() << "，本回合结束后将私信您他的职业";
        cur_action_ = action;
        exocrismed_ = true;
        return true;
    }

  private:
    bool exocrismed_;
};

class GuardRole : public RoleBase
{
  public:
    GuardRole(const uint64_t pid, const Token token, const RoleOption& option, const uint64_t role_num, RoleManager& role_manager)
        : RoleBase(pid, token, Occupation::守卫, Team::平民, option, role_manager)
    {
    }

    virtual bool Act(const ShieldAntiAction& action, MsgSenderBase& reply, StageUtility& utility) override
    {
        if (action.token_hps_.size() > 2 || action.token_hps_.empty()) {
            reply() << "盾反失败：您需要指定 1~2 名角色的血量";
            return false;
        }
        if (action.token_hps_.size() == 2 &&
                std::get<Token>(action.token_hps_[0]) == std::get<Token>(action.token_hps_[1])) {
            reply() << "盾反失败：您需要为不同角色盾反";
            return false;
        }
        for (const auto& [token, hp] : action.token_hps_) {
            if (!role_manager_.GetRole(token).IsAlive()) {
                reply() << "盾反失败：角色 " << token.ToChar() << " 已经死亡";
                return false;
            }
            if (GetHpFromMultiTargetAction(token, last_hp_tokens_) != nullptr) {
                reply() << "盾反失败：您上一回合盾反过角色 " << token.ToChar() << " 了，您无法连续两回合盾反同一角色";
                return false;
            }
        }
        cur_action_ = action;
        last_hp_tokens_ = action.token_hps_;
        auto sender = reply();
        sender << "您选择盾反角色";
        for (const auto& [token, _] : action.token_hps_) {
            sender << " " << token.ToChar();
        }
        sender << "，如果盾反成功，您将收到反馈";
        return true;
    }

    virtual bool OnRoundEnd() override
    {
        if (!std::get_if<ShieldAntiAction>(&cur_action_)) {
            last_hp_tokens_.clear();
        }
        return RoleBase::OnRoundEnd();
    }

  private:
    std::vector<std::tuple<Token, int32_t>> last_hp_tokens_;
};

template <bool k_is_killer_team>
class TwinRole : public RoleBase
{
  public:
    TwinRole(const uint64_t pid, const Token token, const RoleOption& option, const uint64_t role_num, RoleManager& role_manager)
        : RoleBase(pid, token, Occupation::Condition(k_is_killer_team, Occupation::双子（邪）, Occupation::双子（正）),
                Team::Condition(k_is_killer_team, Team::杀手, Team::平民), option, role_manager)
    {
    }

    virtual std::string PrivateInfo(const MainStage& main_stage) const
    {
        return RoleBase::PrivateInfo(main_stage) + "，" +
            TokenInfoForRoles(role_manager_, std::array<Occupation, 2>{Occupation::双子（正）, Occupation::双子（邪）}) +
            "，您当前属于" + GetTeam().ToString() + "阵营";
    }

    virtual bool Act(const AttackAction& action, MsgSenderBase& reply, StageUtility& utility) override
    {
        for (const auto& token_hp : action.token_hps_) {
            const auto occupation = role_manager_.GetRole(std::get<Token>(token_hp)).GetOccupation();
            if (occupation == Occupation::双子（正） || occupation == Occupation::双子（邪）) {
                reply() << "攻击失败：您无法对自己和另一名双子进行攻击";
                return false;
            }
        }
        return RoleBase::Act(action, reply, utility);
    }
};

class KnightRole : public RoleBase
{
  public:
    KnightRole(const uint64_t pid, const Token token, const RoleOption& option, const uint64_t role_num, RoleManager& role_manager)
        : RoleBase(pid, token, Occupation::骑士, Team::平民, option, role_manager)
    {
    }

    virtual std::string PrivateInfo(const MainStage& main_stage) const
    {
        return std::string("您的职业是「") + GetOccupation().ToString() + "」，不知道自己的代号";
    }
};

class FirstVersionTraitorRole : public RoleBase
{
  public:
    FirstVersionTraitorRole(const uint64_t pid, const Token token, const RoleOption& option, const uint64_t role_num, RoleManager& role_manager)
        : RoleBase(pid, token, Occupation::初版内奸, Team::特殊, option, role_manager)
    {
    }

    virtual std::string PrivateInfo(const MainStage& main_stage) const
    {
        return RoleBase::PrivateInfo(main_stage) + "，" +
            TokenInfoForRoles(role_manager_, std::array<Occupation, 2>{Occupation::杀手, Occupation::平民});
    }
};

class TraitorRole : public RoleBase
{
  public:
    TraitorRole(const uint64_t pid, const Token token, const RoleOption& option, const uint64_t role_num, RoleManager& role_manager)
        : RoleBase(pid, token, Occupation::内奸, Team::特殊, option, role_manager)
    {
    }

    virtual std::string PrivateInfo(const MainStage& main_stage) const
    {
        return RoleBase::PrivateInfo(main_stage) + "，" + main_stage.PlayerInfoForRole(role_manager_, Occupation::杀手) + "，" +
            main_stage.PlayerInfoForRole(role_manager_, Occupation::平民);
    }

};

class AgentRole : public RoleBase
{
  public:
    AgentRole(const uint64_t pid, const Token token, const RoleOption& option, const uint64_t role_num, RoleManager& role_manager)
        : RoleBase(pid, token, Occupation::特工, Team::特殊, option, role_manager)
        , hidden_damages_(role_num, 0)
    {
    }

    virtual bool Act(const AttackAction& action, MsgSenderBase& reply, StageUtility& utility) override
    {
        reply() << "攻击失败：您只能通过释放隐藏伤害的方式攻击角色";
        return false;
    }

    virtual bool Act(const AssignHiddenDamangeAction& action, MsgSenderBase& reply, StageUtility& utility)
    {
        int32_t sum_hp = 0;
        for (const auto& [token, hp] : action.token_hps_) {
            if (hp != 5 && hp != 10 && hp != 15) {
                reply() << "蓄力失败：隐藏伤害只能为 5 / 10 / 15 中的一个";
                return false;
            }
            sum_hp += hp;
        }
        if (sum_hp > 20) {
            reply() << "蓄力失败：您最多可造成共 20 点隐藏伤害";
            return false;
        }
        for (const auto& [token, hp] : action.token_hps_) {
            hidden_damages_[token.id_] += hp;
        }
        auto sender = reply();
        sender << "蓄力成功，您当前累积造成的隐藏伤害为：";
        for (uint32_t i = 0; i < hidden_damages_.size(); ++i) {
            if (hidden_damages_[i] > 0) {
                sender << '\n' << static_cast<char>('A' + i) << ' ' << hidden_damages_[i];
            }
        }
        cur_action_ = action;
        return true;
    }

    virtual bool Act(const FlushHiddenDamangeAction& action, MsgSenderBase& reply, StageUtility& utility)
    {
        for (const auto& token : action.tokens_) {
            if (hidden_damages_[token.id_] == 0) {
                reply() << "释放失败：角色 " << token.ToChar() << " 并未累积隐藏伤害";
                return false;
            }
        }
        AttackAction attact_action;
        for (const auto& token : action.tokens_) {
            attact_action.token_hps_.emplace_back(token, hidden_damages_[token.id_]);
            hidden_damages_[token.id_] = 0;
        }
        cur_action_ = attact_action;
        auto sender = reply();
        bool found_hidden_damage = false;
        for (uint32_t i = 0; i < hidden_damages_.size(); ++i) {
            if (hidden_damages_[i] == 0) {
                continue;
            }
            if (!found_hidden_damage) {
                sender << "释放成功，您当前剩余累积造成的隐藏伤害为：";
                found_hidden_damage = true;
            }
            sender << '\n' << static_cast<char>('A' + i) << ' ' << hidden_damages_[i];
        }
        if (!found_hidden_damage) {
            sender << "释放成功，您已无剩余累积隐藏伤害";
        }
        return true;
    }

    std::vector<int32_t> hidden_damages_;
};

MainStage::RoleMaker MainStage::k_role_makers_[Occupation::Count()] = {
    // killer team
    [static_cast<uint32_t>(Occupation(Occupation::杀手))] = &MainStage::MakeRole_<KillerRole>,
    [static_cast<uint32_t>(Occupation(Occupation::替身))] = &MainStage::MakeRole_<BodyDoubleRole>,
    [static_cast<uint32_t>(Occupation(Occupation::恶灵))] = &MainStage::MakeRole_<GhostRole>,
    [static_cast<uint32_t>(Occupation(Occupation::刺客))] = &MainStage::MakeRole_<AssassinRole>,
    [static_cast<uint32_t>(Occupation(Occupation::双子（邪）))] = &MainStage::MakeRole_<TwinRole<true>>,
    [static_cast<uint32_t>(Occupation(Occupation::魔女))] = &MainStage::MakeRole_<WitchRole>,
    // civilian team
    [static_cast<uint32_t>(Occupation(Occupation::平民))] = &MainStage::MakeRole_<CivilianRole>,
    [static_cast<uint32_t>(Occupation(Occupation::圣女))] = &MainStage::MakeRole_<GoddessRole>,
    [static_cast<uint32_t>(Occupation(Occupation::侦探))] = &MainStage::MakeRole_<DetectiveRole>,
    [static_cast<uint32_t>(Occupation(Occupation::灵媒))] = &MainStage::MakeRole_<SorcererRole>,
    [static_cast<uint32_t>(Occupation(Occupation::守卫))] = &MainStage::MakeRole_<GuardRole>,
    [static_cast<uint32_t>(Occupation(Occupation::双子（正）))] = &MainStage::MakeRole_<TwinRole<false>>,
    [static_cast<uint32_t>(Occupation(Occupation::骑士))] = &MainStage::MakeRole_<KnightRole>,
    // special team
    [static_cast<uint32_t>(Occupation(Occupation::初版内奸))] = &MainStage::MakeRole_<FirstVersionTraitorRole>,
    [static_cast<uint32_t>(Occupation(Occupation::内奸))] = &MainStage::MakeRole_<TraitorRole>,
    [static_cast<uint32_t>(Occupation(Occupation::特工))] = &MainStage::MakeRole_<AgentRole>,
};

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

