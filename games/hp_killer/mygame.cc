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

#include "game_framework/game_stage.h"
#include "utility/html.h"
#include "occupation.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = GameStage<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = GameStage<void, SubStages...>;

const std::string k_game_name = "HP杀"; // the game name which should be unique among all the games
const uint64_t k_max_player = 9; // 0 indicates no max-player limits
const uint64_t k_multiple = 0; // the default score multiple for the game, 0 for a testing game, 1 for a formal game, 2 or 3 for a long formal game
const std::string k_developer = "森高";
const std::string k_description = "通过对其他玩家造成伤害，杀掉隐藏在玩家中的杀手的游戏";

std::string GameOption::StatusInfo() const
{
    return "共 " + std::to_string(GET_VALUE(回合数)) + " 回合，每回合超时时间 " + std::to_string(GET_VALUE(时限)) + " 秒";
}

static const std::vector<Occupation>& GetOccupationList(const GameOption& option)
{
    return option.PlayerNum() == 5 ? GET_OPTION_VALUE(option, 五人身份) :
           option.PlayerNum() == 6 ? GET_OPTION_VALUE(option, 六人身份) :
           option.PlayerNum() == 7 ? GET_OPTION_VALUE(option, 七人身份) :
           option.PlayerNum() == 8 ? GET_OPTION_VALUE(option, 八人身份) :
           option.PlayerNum() == 9 ? GET_OPTION_VALUE(option, 九人身份) : (assert(false), GET_OPTION_VALUE(option, 五人身份));
}

static std::vector<Occupation>& GetOccupationList(GameOption& option)
{
    return const_cast<std::vector<Occupation>&>(GetOccupationList(const_cast<const GameOption&>(option)));
}

bool GameOption::ToValid(MsgSenderBase& reply)
{
    if (PlayerNum() < 5) {
        reply() << "该游戏至少 5 人参加，当前玩家数为 " << PlayerNum();
        return false;
    }
    const auto player_num_matched = [player_num = PlayerNum()](const std::vector<Occupation>& occupation_list)
        {
            return std::ranges::count_if(occupation_list, [](const Occupation occupation) { return occupation != Occupation::人偶; }) == player_num;
        };
    if (!GET_VALUE(身份列表).empty() && !player_num_matched(GET_VALUE(身份列表))) {
        reply() << "玩家人数和身份列表长度不匹配";
        return false;
    }
    auto& occupation_list = GetOccupationList(*this);
    if (occupation_list.empty()) {
        // do nothing
    } else if (!player_num_matched(occupation_list)) {
        reply() << "[警告] 身份列表配置项身份个数与参加人数不符，将按照默认配置进行游戏";
        occupation_list.clear();
    } else if (std::ranges::count(occupation_list, Occupation::杀手) != 1) {
        reply() << "[警告] 身份列表中杀手个数不为 1，将按照默认配置进行游戏";
        occupation_list.clear();
    } else {
        for (const auto occupation : std::initializer_list<Occupation>
                {Occupation::替身, Occupation::内奸, Occupation::守卫, Occupation::双子（邪）, Occupation::双子（正）}) {
            if (std::ranges::count(occupation_list, occupation) > 1) {
                reply() << "[警告] 身份列表中" << occupation << "个数大于 1，将按照默认配置进行游戏";
                occupation_list.clear();
                break;
            }
        }
    }
    return true;
}

uint64_t GameOption::BestPlayerNum() const { return 8; }

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

struct HurtAction
{
    std::string ToString() const { return MultiTargetActionToString("攻击", tokens_, hp_); }

    std::vector<Token> tokens_;
    int32_t hp_;
};

struct CureAction
{
    std::string ToString() const { return std::string("治愈 ") + token_.ToChar() + " " + std::to_string(hp_); }

    Token token_;
    int32_t hp_;
};

struct BlockHurtAction
{
    std::string ToString() const { return std::string("挡刀") + (token_.has_value() ? std::string(" ") + token_->ToChar() : "杀手"); }

    std::optional<Token> token_;
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

using ActionVariant = std::variant<HurtAction, CureAction, BlockHurtAction, DetectAction, PassAction, ExocrismAction, ShieldAntiAction>;

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
        , is_allowed_heavy_hurt_cure_(false)
        , is_winner_(true)
        , remain_cure_(option.cure_count_)
        , cur_action_(PassAction{})
    {
    }

    RoleBase(const RoleBase&) = delete;
    RoleBase(RoleBase&&) = delete;

  public:
    virtual bool Act(const HurtAction& action, MsgSenderBase& reply);

    virtual bool Act(const CureAction& action, MsgSenderBase& reply);

    virtual bool Act(const BlockHurtAction& action, MsgSenderBase& reply)
    {
        reply() << "侦查失败：您无法执行该类型行动";
        return false;
    }

    virtual bool Act(const DetectAction& action, MsgSenderBase& reply)
    {
        reply() << "侦查失败：您无法执行该类型行动";
        return false;
    }

    virtual bool Act(const ExocrismAction& action, MsgSenderBase& reply)
    {
        reply() << "通灵失败：您无法执行该类型行动";
        return false;
    }

    virtual bool Act(const PassAction& action, MsgSenderBase& reply)
    {
        reply() << "您本回合决定不行动";
        cur_action_ = action;
        return true;
    }

    virtual bool Act(const ShieldAntiAction& action, MsgSenderBase& reply)
    {
        reply() << "盾反失败：您无法执行该类型行动";
        return false;
    }

    virtual std::string PrivateInfo() const
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
    void SetAllowHeavyHurtCure(const bool allow) { is_allowed_heavy_hurt_cure_ = allow; }
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
    bool is_allowed_heavy_hurt_cure_;
    bool is_winner_;
    int32_t remain_cure_;
    ActionVariant cur_action_;
    std::vector<RoleStatus> history_status_;
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
    void Foreach(const Fn& fn) { std::ranges::for_each(roles_, [&fn](const auto& role_p) { fn(*role_p); }); }
    template <typename Fn>
    void Foreach(const Fn& fn) const { std::ranges::for_each(roles_, [&fn](const auto& role_p) { fn(*role_p); }); }

    uint32_t Size() const { return roles_.size(); }

  private:
    RoleVec roles_;
};

bool RoleBase::Act(const HurtAction& action, MsgSenderBase& reply)
{
    if (action.tokens_.size() != 1) {
        reply() << "攻击失败：您需要且只能攻击 1 名角色";
        return false;
    }
    const Token& token = action.tokens_[0];
    const int32_t hp = action.hp_;
    auto& target = role_manager_.GetRole(token);
    if (!target.is_alive_) {
        reply() << "攻击失败：该角色已经死亡";
        return false;
    }
    if (is_allowed_heavy_hurt_cure_ && hp != k_normal_hurt_hp && hp != k_heavy_hurt_hp) {
        reply() << "攻击失败：您只能造成 " << k_normal_hurt_hp << " 或 " << k_heavy_hurt_hp << " 点伤害";
        return false;
    }
    if (!is_allowed_heavy_hurt_cure_ && hp != k_normal_hurt_hp) {
        reply() << "攻击失败：您只能造成 " << k_normal_hurt_hp << " 点伤害";
        return false;
    }
    reply() << "您本回合对角色 " << token.ToChar() << " 造成了 " << hp << " 点伤害";
    cur_action_ = action;
    return true;
}

bool RoleBase::Act(const CureAction& action, MsgSenderBase& reply)
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
    if (is_allowed_heavy_hurt_cure_ && action.hp_ != k_normal_cure_hp && action.hp_ != k_heavy_cure_hp) {
        reply() << "治愈失败：您只能治愈 " << k_normal_cure_hp << " 或 " << k_heavy_cure_hp << " 点血量";
        return false;
    }
    if (!is_allowed_heavy_hurt_cure_ && action.hp_ != k_normal_cure_hp) {
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
    role_manager.Foreach([&](const auto& role)
        {
            if (role.GetTeam() == team) {
                s += ' ';
                s += role.GetToken().ToChar();
            }
        });
    return s;
}

static std::string TokenInfoForRole(const RoleManager& role_manager, const Occupation occupation)
{
    if (const auto* role = role_manager.GetRole(occupation)) {
        return std::string(occupation.ToString()) + "的代号是 " + role->GetToken().ToChar();
    }
    return std::string("场上没有") + occupation.ToString();
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

class KillerRole : public RoleBase
{
  public:
    KillerRole(const uint64_t pid, const Token token, const RoleOption& option, RoleManager& role_manager)
        : RoleBase(pid, token, Occupation::杀手, Team::杀手, option, role_manager)
    {
        is_allowed_heavy_hurt_cure_ = true;
    }

    virtual std::string PrivateInfo() const
    {
        if (!private_info_.empty()) {
            return private_info_;
        }
        return RoleBase::PrivateInfo() + "，" + TokenInfoForTeam(role_manager_, Team::平民);
    }

  private:
    std::string private_info_;
};

class BodyDoubleRole : public RoleBase
{
  public:
    BodyDoubleRole(const uint64_t pid, const Token token, const RoleOption& option, RoleManager& role_manager)
        : RoleBase(pid, token, Occupation::替身, Team::杀手, option, role_manager)
    {
    }

    virtual std::string PrivateInfo() const
    {
        return RoleBase::PrivateInfo() + "，" + TokenInfoForRole(role_manager_, Occupation::杀手);
    }

    virtual bool Act(const BlockHurtAction& action, MsgSenderBase& reply) override
    {
        reply() << "请做好觉悟，本回合对该角色造成的全部伤害将转移到您身上";
        cur_action_ = action;
        return true;
    }
};

class GhostRole : public RoleBase
{
  public:
    GhostRole(const uint64_t pid, const Token token, const RoleOption& option, RoleManager& role_manager)
        : RoleBase(pid, token, Occupation::恶灵, Team::杀手, option, role_manager)
    {
    }

    virtual std::string PrivateInfo() const
    {
        return RoleBase::PrivateInfo() + "，" + TokenInfoForRole(role_manager_, Occupation::灵媒);
    }
};

class AssassinRole : public RoleBase
{
  public:
    AssassinRole(const uint64_t pid, const Token token, RoleOption option, RoleManager& role_manager)
        : RoleBase(pid, token, Occupation::刺客, Team::杀手, option, role_manager)
    {
    }

    virtual std::string PrivateInfo() const
    {
        return RoleBase::PrivateInfo() + "，" + TokenInfoForRole(role_manager_, Occupation::杀手);
    }

    virtual bool Act(const HurtAction& action, MsgSenderBase& reply) override
    {
        if (action.tokens_.empty()) {
            reply() << "攻击失败：需要至少攻击 1 名角色";
            return false;
        }
        for (const auto& token : action.tokens_) {
            if (!role_manager_.GetRole(token).IsAlive()) {
                reply() << "攻击失败：角色 " << token.ToChar() << " 已经死亡";
                return false;
            }
        }
        int32_t max_target_num = 0;
        switch (action.hp_) {
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
        if (action.tokens_.size() > max_target_num) {
            reply() << "攻击失败：伤害值为 " << action.hp_ << " 时最多指定 " << max_target_num << " 名角色";
            return false;
        }

        cur_action_ = action;

        auto sender = reply();
        sender << "您本回合分别对角色";
        for (const auto& token : action.tokens_) {
            sender << " " << token.ToChar();
        }
        sender << " 造成了 " << action.hp_ << " 点伤害";

        return true;
    }
};

class CivilianRole : public RoleBase
{
  public:
    CivilianRole(const uint64_t pid, const Token token, const RoleOption& option, RoleManager& role_manager)
        : RoleBase(pid, token, Occupation::平民, Team::平民, option, role_manager)
    {
    }
};

class GoddessRole : public RoleBase
{
  public:
    GoddessRole(const uint64_t pid, const Token token, RoleOption option, RoleManager& role_manager)
        : RoleBase(pid, token, Occupation::圣女, Team::平民, (option.cure_count_ = -1, option), role_manager)
    {
    }

    virtual bool Act(const HurtAction& action, MsgSenderBase& reply) override
    {
        if (!history_status_.empty() && std::get_if<HurtAction>(&history_status_.back().action_)) {
            reply() << "攻击失败：您无法连续两回合进行攻击";
            return false;
        }
        return RoleBase::Act(action, reply);
    }
};

class DetectiveRole : public RoleBase
{
  public:
    DetectiveRole(const uint64_t pid, const Token token, const RoleOption& option, RoleManager& role_manager)
        : RoleBase(pid, token, Occupation::侦探, Team::平民, option, role_manager)
    {
    }

  public:
    virtual bool Act(const DetectAction& action, MsgSenderBase& reply) override
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
    SorcererRole(const uint64_t pid, const Token token, const RoleOption& option, RoleManager& role_manager)
        : RoleBase(pid, token, Occupation::灵媒, Team::平民, option, role_manager)
        , exocrismed_(false)
    {
    }

    virtual bool Act(const ExocrismAction& action, MsgSenderBase& reply) override
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
    GuardRole(const uint64_t pid, const Token token, const RoleOption& option, RoleManager& role_manager)
        : RoleBase(pid, token, Occupation::守卫, Team::平民, option, role_manager)
    {
    }

    virtual bool Act(const ShieldAntiAction& action, MsgSenderBase& reply) override
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
    TwinRole(const uint64_t pid, const Token token, const RoleOption& option, RoleManager& role_manager)
        : RoleBase(pid, token, Occupation::Condition(k_is_killer_team, Occupation::双子（邪）, Occupation::双子（正）),
                Team::Condition(k_is_killer_team, Team::杀手, Team::平民), option, role_manager)
    {
    }

    virtual std::string PrivateInfo() const
    {
        return RoleBase::PrivateInfo() + "，" +
            TokenInfoForRoles(role_manager_, std::array<Occupation, 2>{Occupation::双子（正）, Occupation::双子（邪）}) +
            "，您当前属于" + GetTeam().ToString() + "阵营";
    }

    virtual bool Act(const HurtAction& action, MsgSenderBase& reply) override
    {
        for (const auto& token : action.tokens_) {
            const auto occupation = role_manager_.GetRole(token).GetOccupation();
            if (occupation == Occupation::双子（正） || occupation == Occupation::双子（邪）) {
                reply() << "攻击失败：您无法对自己和另一名双子进行攻击";
                return false;
            }
        }
        return RoleBase::Act(action, reply);
    }
};

class TraitorRole : public RoleBase
{
  public:
    TraitorRole(const uint64_t pid, const Token token, const RoleOption& option, RoleManager& role_manager)
        : RoleBase(pid, token, Occupation::内奸, Team::特殊, option, role_manager)
    {
    }

    virtual std::string PrivateInfo() const
    {
        return RoleBase::PrivateInfo() + "，" +
            TokenInfoForRoles(role_manager_, std::array<Occupation, 2>{Occupation::杀手, Occupation::平民});
    }
};

class PuppetRole : public RoleBase
{
  public:
    PuppetRole(const Token token, const RoleOption& option, RoleManager& role_manager)
        : RoleBase(std::nullopt, token, Occupation::人偶, Team::特殊, option, role_manager)
    {
    }
};

enum class RoundResult { KILLER_WIN, CIVILIAN_WIN, DRAW, CONTINUE };

// ========== GAME STAGES ==========

class MainStage : public MainGameStage<>
{
  public:
    using RoleMaker = std::unique_ptr<RoleBase>(*)(uint64_t, Token, const RoleOption&, RoleManager&);

    MainStage(const GameOption& option, MatchBase& match)
        : GameStage(option, match,
                MakeStageCommand("查看当前游戏进展情况", &MainStage::Status_, VoidChecker("赛况")),
                MakeStageCommand("攻击某名角色", &MainStage::Hurt_, VoidChecker("攻击"),
                    RepeatableChecker<BasicChecker<Token>>("角色代号", "A"), ArithChecker<int32_t>(0, 25, "血量")),
                MakeStageCommand("治愈某名角色", &MainStage::Cure_, VoidChecker("治愈"),
                    BasicChecker<Token>("角色代号", "A"),
                    BoolChecker(std::to_string(k_heavy_cure_hp), std::to_string(k_normal_cure_hp))),
                MakeStageCommand("检查某名角色上一回合行动", &MainStage::Detect_, VoidChecker("侦查"),
                    BasicChecker<Token>("角色代号", "A")),
                MakeStageCommand("替某名角色承担本回合伤害", &MainStage::BlockHurt_, VoidChecker("挡刀"),
                    OptionalChecker<BasicChecker<Token>>("角色代号（若为空，则为杀手代号）", "A")),
                MakeStageCommand("获取某名死者的职业", &MainStage::Exocrism_, VoidChecker("通灵"),
                    BasicChecker<Token>("角色代号", "A")),
                MakeStageCommand("盾反某几名角色", &MainStage::ShieldAnti_, VoidChecker("盾反"),
                    RepeatableChecker<BatchChecker<BasicChecker<Token>, ArithChecker<int32_t>>>(
                            BasicChecker<Token>("角色代号", "A"),
                            ArithChecker<int32_t>(-1000, 1000, "预测下一回合血量"))),
                MakeStageCommand("跳过本回合行动", &MainStage::Pass_, VoidChecker("pass")))
        , role_manager_(GET_OPTION_VALUE(option, 身份列表).empty()
                ? GetRoleVec_(option, DefaultRoleOption_(option), role_manager_)
                : LoadRoleVec_(GET_OPTION_VALUE(option, 身份列表), DefaultRoleOption_(option), role_manager_))
        , k_image_width_((k_avatar_width_ + k_cellspacing_ + k_cellpadding_) * role_manager_.Size() + 150)
        , role_info_(RoleInfo_())
        , round_(1)
        , last_round_civilian_lost_(false)
        , last_round_killer_lost_(false)
        , last_round_traitor_lost_(false)
    {
    }

    virtual void OnStageBegin() override {
        Boardcast() << "游戏开始，将私信各位玩家角色代号及职业\n\n第 1 回合开始，请私信裁判行动";
        role_manager_.Foreach([&](const auto& role)
            {
                if (role.PlayerId().has_value()) {
                    Tell(*role.PlayerId()) << role.PrivateInfo();
                }
            });
        table_html_ = Html_(false);
        StartTimer(GET_OPTION_VALUE(option(), 时限));
        RolesOnRoundBegin_();
        Boardcast() << Markdown("## 第 1 回合\n\n" + table_html_, k_image_width_);
    }

    virtual CheckoutErrCode OnTimeout() override
    {
        HookUnreadyPlayers();
        return CheckoutErrCode::Condition(OnRoundFinish_(), StageErrCode::CHECKOUT, StageErrCode::CONTINUE);
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (IsReady(pid)) {
            return StageErrCode::OK;
        }
        if (rand() % 2) {
            Hurt_(pid, false, reply, {Token{static_cast<uint32_t>(rand() % option().PlayerNum())}}, 15); // randomly hurt one role
        } else {
            Cure_(pid, false, reply, Token{static_cast<uint32_t>(rand() % option().PlayerNum())}, false); // randomly hurt one role
        }
        return StageErrCode::READY;
    }

    virtual void OnAllPlayerReady() override
    {
        OnRoundFinish_();
    }

    virtual int64_t PlayerScore(const PlayerID pid) const override { return role_manager_.GetRole(pid).IsWinner(); }

  private:
    static RoleOption DefaultRoleOption_(const GameOption& option)
    {
        return RoleOption {
            .hp_ = GET_OPTION_VALUE(option, 血量),
            .cure_count_ = GET_OPTION_VALUE(option, 治愈次数),
        };
    }

    void SettlementAction_(MsgSenderBase::MsgSenderGuard& sender)
    {
        // Multiple body doubles is forbidden.
        RoleBase* const hurt_blocker = role_manager_.GetRole(Occupation::替身);
        const BlockHurtAction* const block_hurt_action =
            hurt_blocker ? std::get_if<BlockHurtAction>(&hurt_blocker->CurAction()) : nullptr;
        const auto is_blocked_hurt = [&](const RoleBase& role)
            {
                return block_hurt_action &&
                    ((!block_hurt_action->token_.has_value() && role.GetOccupation() == Occupation::杀手) ||
                     (block_hurt_action->token_.has_value() && role.GetToken() == *block_hurt_action->token_));
            };
        const auto is_avoid_hurt = [&](const RoleBase& hurter_role, const RoleBase& hurted_role)
            {
                return hurter_role.GetOccupation() == Occupation::圣女 && hurted_role.GetTeam() == Team::平民;
            };

        role_manager_.Foreach([&](auto& role)
            {
                if (const auto action = std::get_if<HurtAction>(&role.CurAction())) {
                    for (const auto& token : action->tokens_) {
                        auto& hurted_role = role_manager_.GetRole(token);
                        if (is_avoid_hurt(role, hurted_role)) {
                            // do nothing
                        } else if (is_blocked_hurt(hurted_role)) {
                            hurt_blocker->AddHp(-action->hp_);
                        } else {
                            hurted_role.AddHp(-action->hp_);
                            if (hurted_role.GetOccupation() == Occupation::灵媒 && role.GetOccupation() == Occupation::恶灵) {
                                role.AddHp(-action->hp_);
                            }
                        }
                    }
                } else if (const auto action = std::get_if<CureAction>(&role.CurAction())) {
                    role_manager_.GetRole(action->token_).AddHp(action->hp_);
                } else if (const auto action = std::get_if<DetectAction>(&role.CurAction())) {
                    auto& detected_role = role_manager_.GetRole(action->token_);
                    assert(role.PlayerId().has_value());
                    auto sender = Tell(*role.PlayerId());
                    sender << "上一回合角色 " << action->token_.ToChar() << " 的行动是「";
                    if (const auto detect_action = std::get_if<HurtAction>(&detected_role.CurAction())) {
                        sender << "攻击 " << detect_action->tokens_[0].ToChar();
                    } else if (const auto detect_action = std::get_if<CureAction>(&detected_role.CurAction())) {
                        sender << "治愈 " << detect_action->token_.ToChar();
                    } else {
                        sender << "其它";
                    }
                    sender << "」";
                    if (!detected_role.IsAlive() && (std::get_if<HurtAction>(&detected_role.CurAction()) ||
                                std::get_if<CureAction>(&detected_role.CurAction()))) {
                        DisableAct_(detected_role, true);
                        sender << "，而且你完成了除灵，他已经失去行动能力了！";
                    }
                } else if (const auto action = std::get_if<ExocrismAction>(&role.CurAction())) {
                    auto& exocrism_role = role_manager_.GetRole(action->token_);
                    assert(role.PlayerId().has_value());
                    auto sender = Tell(*role.PlayerId());
                    sender << "死亡角色 " << exocrism_role.GetToken().ToChar() << " 的职业是「" << exocrism_role.GetOccupation() << "」";
                    if (exocrism_role.GetOccupation() == Occupation::恶灵) {
                        DisableAct_(exocrism_role, true);
                        sender << "，他从下回合开始将失去行动能力！";
                    }
                }
            });

        RoleBase* const guard_role = role_manager_.GetRole(Occupation::守卫);
        if (const ShieldAntiAction* const shield_anti_action =
                guard_role ? std::get_if<ShieldAntiAction>(&guard_role->CurAction()) : nullptr) {
            std::vector<int32_t> hp_additions(role_manager_.Size(), 0);
            role_manager_.Foreach([&](auto& role)
                {
                    if (const int32_t* const hp_p = GetHpFromMultiTargetAction(role.GetToken(), shield_anti_action->token_hps_);
                            hp_p != nullptr && !is_blocked_hurt(role) && role.GetHp() == *hp_p) {
                        Tell(*guard_role->PlayerId()) << "为角色 " << role.GetToken().ToChar() << " 盾反成功";
                        role_manager_.Foreach([&](auto& hurter_role)
                            {
                                const HurtAction* const hurt_action = std::get_if<HurtAction>(&hurter_role.CurAction());
                                if (hurt_action && Has(hurt_action->tokens_, role.GetToken()) &&
                                        !is_avoid_hurt(hurter_role, role)) {
                                    hp_additions[role.GetToken().id_] += hurt_action->hp_;
                                    hp_additions[hurter_role.GetToken().id_] -= hurt_action->hp_;
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
                role.OnRoundBegin();
            });
    }

    void RolesOnRoundEnd_(MsgSenderBase::MsgSenderGuard& sender)
    {
        role_manager_.Foreach([&](auto& role)
            {
                if (role.OnRoundEnd()) {
                    sender << "\n角色 " << role.GetToken().ToChar() << " 死亡，";
                    if (role.PlayerId().has_value()) {
                        sender << "他的「中之人」是" << At(*role.PlayerId());
                    } else {
                        sender << "他是 NPC，没有「中之人」";
                    }
                    if (role.GetOccupation() != Occupation::恶灵) {
                        DisableAct_(role);
                    }
                    RoleBase* other_role = nullptr;
                    if (role.GetOccupation() == Occupation::杀手 && (other_role = role_manager_.GetRole(Occupation::内奸))) {
                        other_role->SetAllowHeavyHurtCure(true);
                        Tell(*other_role->PlayerId()) << "杀手已经死亡，您获得了造成 " << k_heavy_hurt_hp
                                << " 点伤害和治愈 " << k_heavy_cure_hp << " 点 HP 的权利";
                    } else if (((role.GetOccupation() == Occupation::双子（正） && (other_role = role_manager_.GetRole(Occupation::双子（邪）))) ||
                             (role.GetOccupation() == Occupation::双子（邪） && (other_role = role_manager_.GetRole(Occupation::双子（正）))))
                            && other_role->GetHp() > 0) { // cannot use IsAlive() because the is_alive_ field may have not been updated
                        other_role->SetNextRoundTeam(role.GetTeam());
                        Tell(*other_role->PlayerId()) << "另一位双子死亡，您下一回合的阵营变更为：" << role.GetTeam() << "阵营";
                    }
                }
            });
        sender << "\n\n";
    }

    bool CheckTeamsLost_(MsgSenderBase::MsgSenderGuard& sender)
    {
        bool killer_dead = true;
        bool traitor_dead = true;
        bool all_civilian_team_dead_next_round = true;
        uint32_t civilian_dead_count = 0;
        uint32_t civilian_team_dead_count = 0;
        role_manager_.Foreach([&](const auto& role)
            {
                if (role.IsAlive()) {
                    if (role.GetOccupation() == Occupation::内奸) {
                        traitor_dead = false;
                    }
                    if (role.GetOccupation() == Occupation::杀手) {
                        killer_dead = false;
                    }
                    if (role.GetNextRoundTeam() == Team::平民) {
                        // civilian twin can convert to killer team next round, so we check the next-round team instead of current team
                        all_civilian_team_dead_next_round = false;
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
                             all_civilian_team_dead_next_round;
        bool killer_lost = killer_dead;
        bool traitor_lost = traitor_dead;

        switch (!civilian_lost + !killer_lost + !traitor_lost) {
        case 0: // multiple teams lost at the same time
            sender << "游戏结束，多个阵营的失败条件同时满足，此时根据优先级，判定";
            if (traitor_lost && !last_round_traitor_lost_) { // traitor lost at this round
                traitor_lost = false;
                sender << "内奸";
            } else if (killer_lost && !last_round_killer_lost_) { // killer lost at this round
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
            if ((++round_) > GET_OPTION_VALUE(option(), 回合数)) {
                --round_;
                sender << "游戏达到最大回合限制，游戏平局";
            } else {
                role_manager_.Foreach([&](auto& role)
                    {
                        if ((!last_round_civilian_lost_ && civilian_lost && role.GetTeam() == Team::平民) ||
                                (!last_round_killer_lost_ && killer_lost && role.GetTeam() == Team::杀手)) {
                            if (role.PlayerId().has_value()) {
                                Tell(*role.PlayerId()) << "很遗憾，您所在的阵营失败了";
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
                } else if (role.GetOccupation() == Occupation::内奸) {
                    role.SetWinner(!traitor_lost);
                } else if (role.GetOccupation() != Occupation::人偶) {
                    assert(false);
                }
            });

        return true;
    }

    bool Settlement_()
    {
        auto sender = Boardcast();
        sender << "第 " << round_ << " 回合结束，下面公布各角色血量";
        SettlementAction_(sender);
        RolesOnRoundEnd_(sender);
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
            Tell(*role.PlayerId()) << "您失去了行动能力";
            Eliminate(*role.PlayerId());
        }
    }

    template <typename RoleType>
    static std::unique_ptr<RoleBase> MakeRole_(const uint64_t pid, const Token token, const RoleOption& option, RoleManager& role_manager)
    {
        return std::make_unique<RoleType>(pid, token, option, role_manager);
    }

    static RoleManager::RoleVec LoadRoleVec_(const std::vector<Occupation>& occupation_list, const RoleOption& option, RoleManager& role_manager)
    {
        RoleManager::RoleVec v;
        for (uint32_t i = 0, pid = 0; i < occupation_list.size(); ++i) {
            if (occupation_list[i] == Occupation::人偶) {
                v.emplace_back(std::make_unique<PuppetRole>(Token(i), option, role_manager));
            } else {
                v.emplace_back(k_role_makers_[occupation_list[i].ToUInt()](pid++, Token(i), option, role_manager));
            }
        }
        return v;
    }

    static RoleManager::RoleVec GetRoleVec_(const GameOption& option, const RoleOption& role_option, RoleManager& role_manager)
    {
        const auto make_roles = [&]<typename T>(const std::initializer_list<T>& occupation_lists)
            {
                assert(occupation_lists.size() > 0);
                std::random_device rd;
                std::mt19937 g(rd());
                const auto& occupation_list = std::data(occupation_lists)[std::uniform_int_distribution<int>(0, occupation_lists.size() - 1)(rd)];
                std::vector<PlayerID> pids;
                for (uint32_t i = 0; i < option.PlayerNum(); ++i) {
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
                        v.emplace_back(k_role_makers_[static_cast<uint32_t>(occupation)](pids[pid_i++], tokens[i], role_option, role_manager));
                    }
                }
                std::ranges::sort(v, [](const auto& _1, const auto& _2) { return _1->GetToken() < _2->GetToken(); });
                return v;
            };
        if (const auto& occupation_list = GetOccupationList(option); !occupation_list.empty()) {
            return make_roles({occupation_list});
        }
        switch (option.PlayerNum()) {
        case 5: return make_roles(std::initializer_list<std::initializer_list<Occupation>>{
                    {Occupation::杀手, Occupation::双子（邪）, Occupation::双子（正）, Occupation::平民, Occupation::平民}
                });
        case 6: return make_roles(std::initializer_list<std::initializer_list<Occupation>>{
                    {Occupation::杀手, Occupation::刺客, Occupation::双子（邪）, Occupation::双子（正）, Occupation::侦探, Occupation::圣女}
                });
        case 7: return make_roles(std::initializer_list<std::initializer_list<Occupation>>{
                    {Occupation::杀手, Occupation::替身, Occupation::侦探, Occupation::圣女, Occupation::平民, Occupation::平民, Occupation::内奸}
                });
        case 8: return make_roles(std::initializer_list<std::initializer_list<Occupation>>{
                    {Occupation::杀手, Occupation::替身, Occupation::刺客, Occupation::侦探, Occupation::圣女, Occupation::守卫, Occupation::平民, Occupation::平民, Occupation::人偶},
                    {Occupation::杀手, Occupation::替身, Occupation::恶灵, Occupation::侦探, Occupation::圣女, Occupation::灵媒, Occupation::平民, Occupation::平民},
                    {Occupation::杀手, Occupation::替身, Occupation::双子（邪）, Occupation::侦探, Occupation::圣女, Occupation::双子（正）, Occupation::平民, Occupation::平民}
                });
        case 9: return make_roles(std::initializer_list<std::initializer_list<Occupation>>{
                    {Occupation::杀手, Occupation::替身, Occupation::刺客, Occupation::侦探, Occupation::圣女, Occupation::守卫, Occupation::平民, Occupation::平民, Occupation::内奸},
                    {Occupation::杀手, Occupation::替身, Occupation::恶灵, Occupation::侦探, Occupation::圣女, Occupation::灵媒, Occupation::平民, Occupation::平民, Occupation::内奸},
                    {Occupation::杀手, Occupation::替身, Occupation::双子（邪）, Occupation::侦探, Occupation::圣女, Occupation::双子（正）, Occupation::平民, Occupation::平民, Occupation::内奸}
                });
        default:
            assert(false);
            return {};
        }
    }

    std::string Image_(const char* const name, const int32_t width) const
    {
        return std::string("<img src=\"file://") + option().ResourceDir() + "/" + name + ".png\" style=\"width:" +
            std::to_string(width) + "px; vertical-align: middle;\">";
    }

    std::string RoleInfo_() const
    {
        std::string s = HTML_SIZE_FONT_HEADER(4) "<b>本场游戏包含职业：";
        std::vector<Occupation> occupations;
        role_manager_.Foreach([&](const auto& role)
            {
                occupations.emplace_back(role.GetOccupation());
            });
        std::ranges::sort(occupations);
        for (const auto& occupation : occupations) {
            if (occupation == Occupation::杀手 || occupation == Occupation::替身 || occupation == Occupation::恶灵 ||
                    occupation == Occupation::刺客 || occupation == Occupation::双子（邪）) {
                s += HTML_COLOR_FONT_HEADER(red);
            } else if (occupation == Occupation::内奸 || occupation == Occupation::人偶) {
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
                                       role.PlayerId().has_value()      ? PlayerAvatar(*role.PlayerId(), k_avatar_width_) :
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
                            Image_("blank", k_icon_size_) + std::to_string(GET_OPTION_VALUE(option(), 血量)) + "</font></p>");
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
                const auto last_hp = r == 0 ? GET_OPTION_VALUE(option(), 血量) : role.GetHistoryStatus(r - 1)->hp_;
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

        return role_info_ + "\n\n" + table.ToString();
    }

    AtomReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (!is_public) {
            const auto& role = role_manager_.GetRole(pid);
            reply() << role.PrivateInfo() << "，剩余 "
                << (std::get_if<CureAction>(&role.CurAction()) ? role.RemainCure() - 1 : role.RemainCure()) << " 次治愈机会";
        }
        reply() << Markdown("## 第 " + std::to_string(round_) + " 回合\n\n" + table_html_, k_image_width_);
        return StageErrCode::OK;
    }

    bool OnRoundFinish_()
    {
        if (!Settlement_()) {
            table_html_ = Html_(false);
            Boardcast() << Markdown("## 第 " + std::to_string(round_) + " 回合\n\n" + table_html_, k_image_width_);
            ClearReady();
            StartTimer(GET_OPTION_VALUE(option(), 时限));
            RolesOnRoundBegin_();
            return false;
        }
        Boardcast() << Markdown("## 终局\n\n" + Html_(true), k_image_width_);
        return true;
    }

    AtomReqErrCode GenericAct_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const ActionVariant& action)
    {
        if (is_public) {
            reply() << "行动失败：请您私信裁判行动";
            return StageErrCode::FAILED;
        }
        if (IsReady(pid)) {
            reply() << "行动失败：您已完成行动，或您不需要行动";
            return StageErrCode::FAILED;
        }
        auto& role = role_manager_.GetRole(pid);
        if (!role.CanAct()) {
            reply() << "行动失败：您已经失去了行动能力";
            return StageErrCode::FAILED; // should not appened for user player
        }
        if (!std::visit([&role, &reply](auto& action) { return role.Act(action, reply); }, action)) {
            return StageErrCode::FAILED;
        }
        return StageErrCode::READY;
    }

    AtomReqErrCode Hurt_(const PlayerID pid, const bool is_public, MsgSenderBase& reply,
            const std::vector<Token>& tokens, const int32_t hp)
    {
        for (const auto& token : tokens) {
            if (!role_manager_.IsValid(token)) {
                reply() << "攻击失败：场上没有角色 " << token.ToChar();
                return StageErrCode::FAILED;
            }
        }
        return GenericAct_(pid, is_public, reply, HurtAction{.tokens_ = tokens, .hp_ = hp});
    }

    AtomReqErrCode Cure_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const Token token, const bool is_heavy)
    {
        if (!role_manager_.IsValid(token)) {
            reply() << "治愈失败：场上没有该角色";
            return StageErrCode::FAILED;
        }
        return GenericAct_(pid, is_public, reply, CureAction{.token_ = token, .hp_ = is_heavy ? k_heavy_cure_hp : k_normal_cure_hp});
    }

    AtomReqErrCode Detect_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const Token token)
    {
        if (!role_manager_.IsValid(token)) {
            reply() << "侦查失败：场上没有该角色";
            return StageErrCode::FAILED;
        }
        return GenericAct_(pid, is_public, reply, DetectAction{.token_ = token});
    }

    AtomReqErrCode BlockHurt_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::optional<Token>& token)
    {
        if (token.has_value() && !role_manager_.IsValid(*token)) {
            reply() << "挡刀失败：场上没有该角色";
            return StageErrCode::FAILED;
        }
        return GenericAct_(pid, is_public, reply, BlockHurtAction{token});
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
        for (const auto& [token, hp] : token_hps) {
            if (!role_manager_.IsValid(token)) {
                reply() << "盾反失败：场上没有角色 " << token.ToChar();
                return StageErrCode::FAILED;
            }
        }
        return GenericAct_(pid, is_public, reply, ShieldAntiAction{.token_hps_ = token_hps});
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

MainStage::RoleMaker MainStage::k_role_makers_[Occupation::Count()] = {
    // killer team
    [static_cast<uint32_t>(Occupation(Occupation::杀手))] = &MainStage::MakeRole_<KillerRole>,
    [static_cast<uint32_t>(Occupation(Occupation::替身))] = &MainStage::MakeRole_<BodyDoubleRole>,
    [static_cast<uint32_t>(Occupation(Occupation::恶灵))] = &MainStage::MakeRole_<GhostRole>,
    [static_cast<uint32_t>(Occupation(Occupation::刺客))] = &MainStage::MakeRole_<AssassinRole>,
    [static_cast<uint32_t>(Occupation(Occupation::双子（邪）))] = &MainStage::MakeRole_<TwinRole<true>>,
    // civilian team
    [static_cast<uint32_t>(Occupation(Occupation::平民))] = &MainStage::MakeRole_<CivilianRole>,
    [static_cast<uint32_t>(Occupation(Occupation::圣女))] = &MainStage::MakeRole_<GoddessRole>,
    [static_cast<uint32_t>(Occupation(Occupation::侦探))] = &MainStage::MakeRole_<DetectiveRole>,
    [static_cast<uint32_t>(Occupation(Occupation::灵媒))] = &MainStage::MakeRole_<SorcererRole>,
    [static_cast<uint32_t>(Occupation(Occupation::守卫))] = &MainStage::MakeRole_<GuardRole>,
    [static_cast<uint32_t>(Occupation(Occupation::双子（正）))] = &MainStage::MakeRole_<TwinRole<false>>,
    // special team
    [static_cast<uint32_t>(Occupation(Occupation::内奸))] = &MainStage::MakeRole_<TraitorRole>,
};

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

#include "game_framework/make_main_stage.h"
