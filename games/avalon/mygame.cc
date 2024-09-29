// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#ifdef ENUM_BEGIN
#ifdef ENUM_MEMBER
#ifdef ENUM_END

ENUM_BEGIN(AvatarIcon)
ENUM_MEMBER(AvatarIcon, captain)
ENUM_MEMBER(AvatarIcon, member)
ENUM_MEMBER(AvatarIcon, sword)
ENUM_MEMBER(AvatarIcon, witch)
ENUM_MEMBER(AvatarIcon, target)
ENUM_END(AvatarIcon)

#endif
#endif
#endif

#ifndef AVALON_GAME_MAIN_CC
#define AVALON_GAME_MAIN_CC

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"

#include "occupation.h"

#include <random>

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

#define ENUM_FILE "mygame.cc"
#include "utility/extend_enum.h"

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;

// 0 indicates no max-player limits
uint64_t MaxPlayerNum(const CustomOptions& options) { return 10; }

uint32_t Multiple(const CustomOptions& options) { return 0; }

const GameProperties k_properties {
    .name_ = "阿瓦隆",
    .developer_ = "森高",
    .description_ = "找出信赖的队友，组队完成三次任务的游戏",
    .shuffled_player_id_ = true,
};

// The default generic options.
const MutableGenericOptions k_default_generic_options;

// The commands for showing more rules information. Users can get the information by "#规则 <game name> <rule command>...".
const std::vector<RuleCommand> k_rule_commands = {};

// The commands for initialize the options when starting a game by "#新游戏 <game name> <init options command>..."
const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options)
            {
                // Set the target player numbers when an user start the game with the "单机" argument.
                // It is ok to make `k_init_options_commands` empty.
                generic_options.bench_computers_to_player_num_ = 10;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
    InitOptionsCommand("不启用任何扩展包",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options)
            {
                GET_OPTION_VALUE(game_options, 兰斯洛特模式) = LancelotMode::disable;
                GET_OPTION_VALUE(game_options, 王者之剑) = false;
                return NewGameMode::MULTIPLE_USERS;
            },
            VoidChecker("经典")),
};

// The function is invoked before a game starts. You can make final adaption for the options.
// The return value of false denotes failure to start a game.
bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() < 5) {
        reply() << "该游戏至少 5 人参加，当前玩家数为 " << generic_options_readonly.PlayerNum();
        return false;
    }
    return true;
}

// ========== GAME STAGES ==========

constexpr int32_t k_mission_avater_size = 40;
constexpr int32_t k_member_avater_size = 64;
constexpr int32_t k_team_up_avater_size = 32;

class TeamUpStage;
class VoteStage;
class ActStage;
class DetectStage;
class AssassinStage;

struct Player
{
    Occupation occupation_;
    Team team_;
    bool can_assassin_{false};
    bool has_been_witch_{false};
};

static constexpr int32_t k_mission_num = 5;

struct Mission
{
    int32_t member_num_{0};
    bool is_protected_{false};
    bool to_convert_lancelot_{false};
};

static std::vector<Player> InitializePlayers(const uint32_t player_num, const bool with_extend)
{
    static const Player 梅林{    .occupation_ = Occupation::梅林,     .team_ = Team::好};
    static const Player 派西维尔{.occupation_ = Occupation::派西维尔, .team_ = Team::好};
    static const Player 忠臣{    .occupation_ = Occupation::亚瑟的忠臣,     .team_ = Team::好};
    static const Player 好兰斯{  .occupation_ = Occupation::兰斯洛特, .team_ = Team::好};

    static const Player 莫德雷德{.occupation_ = Occupation::莫德雷德, .team_ = Team::坏};
    static const Player 刺客莫德{.occupation_ = Occupation::莫德雷德, .team_ = Team::坏, .can_assassin_ = true};
    static const Player 莫甘娜{  .occupation_ = Occupation::莫甘娜,   .team_ = Team::坏};
    static const Player 奥伯伦{  .occupation_ = Occupation::奥伯伦,   .team_ = Team::坏};
    static const Player 刺客{    .occupation_ = Occupation::刺客,     .team_ = Team::坏, .can_assassin_ = true};
    static const Player 爪牙{    .occupation_ = Occupation::莫德雷德的爪牙, .team_ = Team::坏};
    static const Player 坏兰斯{  .occupation_ = Occupation::兰斯洛特, .team_ = Team::坏};

    if (with_extend) {
        switch (player_num) {
            case 5:  return { 莫甘娜,   刺客,   梅林,   派西维尔, 忠臣, };
            case 6:  return { 莫甘娜,   刺客,   梅林,   派西维尔, 忠臣,     忠臣, };
            case 7:  return { 莫甘娜,   刺客,   坏兰斯, 梅林,     派西维尔, 忠臣,     好兰斯, };
            case 8:  return { 莫甘娜,   刺客,   坏兰斯, 梅林,     派西维尔, 忠臣,     忠臣,   好兰斯, };
            case 9:  return { 莫甘娜,   刺客,   坏兰斯, 梅林,     派西维尔, 忠臣,     忠臣,   忠臣,   好兰斯, };
            case 10: return { 刺客莫德, 莫甘娜, 奥伯伦, 坏兰斯,   梅林,     派西维尔, 忠臣,   忠臣,   忠臣,   好兰斯, };
        }
    } else {
        switch (player_num) {
            case 5:  return { 莫甘娜,   刺客,   梅林,   派西维尔, 忠臣, };
            case 6:  return { 莫甘娜,   刺客,   梅林,   派西维尔, 忠臣, 忠臣, };
            case 7:  return { 莫甘娜,   奥伯伦, 刺客,   梅林,     派西维尔, 忠臣,     忠臣, };
            case 8:  return { 莫甘娜,   刺客,   爪牙,   梅林,     派西维尔, 忠臣,     忠臣, 忠臣, };
            case 9:  return { 莫德雷德, 莫甘娜, 刺客,   梅林,     派西维尔, 忠臣,     忠臣, 忠臣, 忠臣, };
            case 10: return { 莫德雷德, 莫甘娜, 奥伯伦, 刺客,     梅林,     派西维尔, 忠臣, 忠臣, 忠臣, 忠臣, };
        }
    }

    assert(false);
    return {};
}

static std::array<Mission, k_mission_num> InitializeMissions(const uint32_t player_num, const LancelotMode lancelot_mode, const bool shuffle = true)
{
    std::array<bool, k_mission_num> to_convert_lancelots;
    to_convert_lancelots.fill(false);
    std::random_device rd;
    std::mt19937 gen {rd()};
    if (lancelot_mode == LancelotMode::explicit_five_rounds) {
        std::array<bool, 7> values = {true, true, false, false, false, false, false};
        if (shuffle) {
            std::ranges::shuffle(values, gen);
        }
        std::copy_n(values.begin(), 5, to_convert_lancelots.begin());
    } else if (lancelot_mode == LancelotMode::implicit_three_rounds) {
        std::array<bool, 5> values = {true, true, false, false, false};
        if (shuffle) {
            std::ranges::shuffle(values, gen);
        }
        std::copy_n(values.begin(), 3, to_convert_lancelots.begin() + 2);
    }
    int i = 0;
    const auto make_normal = [&](const int32_t member_num)
        {
            return Mission{.member_num_ = member_num, .is_protected_ = false, .to_convert_lancelot_ = to_convert_lancelots[i++]};
        };
    const auto make_protected = [&](const int32_t member_num)
        {
            return Mission{.member_num_ = member_num, .is_protected_ = true, .to_convert_lancelot_ = to_convert_lancelots[i++]};
        };
    switch (player_num) {
        case 5:  return { make_normal(2), make_normal(3), make_normal(2), make_normal(3),    make_normal(3) };
        case 6:  return { make_normal(2), make_normal(3), make_normal(4), make_normal(3),    make_normal(4) };
        case 7:  return { make_normal(2), make_normal(3), make_normal(3), make_protected(4), make_normal(4) };
        case 8:
        case 9:
        case 10: return { make_normal(3), make_normal(4), make_normal(4), make_protected(5), make_normal(5) };
    }
    assert(false);
    return {};
}

static std::string RepeatString(const int32_t count, const std::string& s)
{
    std::string result;
    for (int32_t i = 0; i < count; ++i) {
        result += s;
    }
    return result;
}

static bool NeedLancelotCard(const LancelotMode mode)
{
    return mode == LancelotMode::explicit_five_rounds || mode == LancelotMode::implicit_three_rounds;
}

class MainStage : public MainGameStage<TeamUpStage, VoteStage, ActStage, DetectStage, AssassinStage>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility),
                MakeStageCommand(*this, "查看当前游戏进展情况", &MainStage::Status_, VoidChecker("赛况")),
                MakeStageCommand(*this, "尝试刺杀梅林", &MainStage::Assassin_, VoidChecker("刺杀"), ArithChecker<uint32_t>(0, utility.PlayerNum() - 1, "玩家 ID")))
        , players_(InitializePlayers(utility.PlayerNum(), GAME_OPTION(兰斯洛特模式) != LancelotMode::disable))
        , missions_(InitializeMissions(utility.PlayerNum(), GAME_OPTION(兰斯洛特模式)
#ifdef TEST_BOT
                    , !GAME_OPTION(测试模式)
#endif
                    ))
        , witch_pid_(Global().PlayerNum() >= GAME_OPTION(湖中仙女人数) ? std::optional<PlayerID>{Global().PlayerNum() - 1} : std::nullopt)
        , occupations_html_([&players = players_]()
                {
                    std::string s = "<b>本场游戏包含身份：";
                    for (const auto& player : players) {
                        if (player.team_ == Team::坏) {
                            s += HTML_COLOR_FONT_HEADER(red);
                        }
                        s += player.occupation_.ToString();
                        if (player.team_ == Team::坏) {
                            s += HTML_FONT_TAIL;
                        }
                        s += HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE;
                    }
                    s += "</b>";
                    return s;
                }())
        , mission_table_{1 + k_mission_num, static_cast<uint32_t>(5 + NeedLancelotCard(GAME_OPTION(兰斯洛特模式)))}
    {
        std::random_device rd;
        std::mt19937 gen {rd()};
#ifdef TEST_BOT
        if (!GAME_OPTION(测试模式)) {
#else
        if (true) {
#endif
            std::ranges::shuffle(players_, gen);
        }

        const bool with_lancelot_card = NeedLancelotCard(GAME_OPTION(兰斯洛特模式));
        mission_table_.SetTableStyle(" align=\"center\" border=\"1px solid #ccc\" cellpadding=\"5\" cellspacing=\"1\" ");
        mission_table_.Get(0, 0).SetContent("**序号**");
        if (with_lancelot_card) {
            mission_table_.Get(0, 1).SetContent("**兰斯洛特转换**");
        }
        mission_table_.Get(0, 1 + with_lancelot_card).SetContent("<div style=\"width:" + std::to_string(k_mission_avater_size * 5) + "px;\"> <b>完成情况</b> </div>");
        mission_table_.Get(0, 2 + with_lancelot_card).SetContent("<div style=\"width:" + std::to_string(k_mission_avater_size) + "px;\"> <b>队长</b> </div>");
        mission_table_.Get(0, 3 + with_lancelot_card).SetContent("<div style=\"width:" + std::to_string(k_mission_avater_size * 5) + "px;\"> <b>参与者</b> </div>");
        mission_table_.Get(0, 4 + with_lancelot_card).SetContent("**结果**");
        for (int i = 0; i < k_mission_num; ++i) {
            mission_table_.Get(1 + i, 0).SetContent(std::to_string(i + 1));
            if (GAME_OPTION(兰斯洛特模式) == LancelotMode::explicit_five_rounds ||
                    (GAME_OPTION(兰斯洛特模式) == LancelotMode::implicit_three_rounds && i <= 1)) {
                mission_table_.Get(1 + i, 1).SetContent(std::string("![](file:///") + Global().ResourceDir() +
                        (missions_[i].to_convert_lancelot_ ? "/convert.png)" : "/not_convert.png)"));
            }
            mission_table_.Get(1 + i, 1 + with_lancelot_card).SetContent(
                    RepeatString(missions_[i].member_num_, std::string("![](file:///") + Global().ResourceDir() + "/null.png)"));
            if (missions_[i].is_protected_) {
                mission_table_.SetRowStyle(1 + i, "align=\"center\" bgcolor=\"#eee7dd\"");
            }
        }

        team_up_table_.SetTableStyle(" align=\"center\" border=\"1px solid #ccc\" cellpadding=\"5\" cellspacing=\"1\" ");
        team_up_table_.Get(0, 0).SetContent("**任务序号**");
        team_up_table_.Get(0, 1).SetContent("<div style=\"width:" + std::to_string(k_team_up_avater_size) + "px;\"> <b>队长</b> </div>");
        team_up_table_.Get(0, 2).SetContent("<div style=\"width:" + std::to_string(k_team_up_avater_size * 5) + "px;\"> <b>提议参与者</b> </div>");
        team_up_table_.Get(0, 3).SetContent("<div style=\"width:" + std::to_string(k_team_up_avater_size * 5) + "px;\"> <b>同意者</b> </div>");
        team_up_table_.Get(0, 4).SetContent("<div style=\"width:" + std::to_string(k_team_up_avater_size * 5) + "px;\"> <b>反对者</b> </div>");
        team_up_table_.Get(0, 5).SetContent("**结果**");
    }

    virtual void FirstStageFsm(SubStageFsmSetter setter) override;

    virtual void NextStageFsm(TeamUpStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;
    virtual void NextStageFsm(VoteStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;
    virtual void NextStageFsm(ActStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;
    virtual void NextStageFsm(DetectStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;
    virtual void NextStageFsm(AssassinStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override {}

    virtual int64_t PlayerScore(const PlayerID pid) const override { return players_[pid].team_ == winner_team_; }

    std::vector<Player>& GetPlayers() { return players_; }

    // requires: the role must exist.
    PlayerID GetOccupationPlayerID(const Occupation occupation) const
    {
        const auto it = std::ranges::find_if(players_,
                [occupation](const auto& player) { return player.occupation_ == occupation; });
        assert(it != players_.end());
        return std::distance(players_.begin(), it);
    }

    void ShowHtml(const std::string& title) const
    {
        std::string html = "## ";
        html += title;
        html += "\n\n";
        html += occupations_html_;
        html += "\n\n<div align=\"center\">\n\n**任务执行结果**\n\n";
        html += mission_table_.ToString();
        html += "\n\n**玩家列表**\n\n";
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            const bool obtained_sword = GAME_OPTION(王者之剑) && !member_pids_.empty() && pid == member_pids_.front();
            AppendPlayerAvatar_(pid,
                    AvatarIcon::BitSet{}.set(AvatarIcon{AvatarIcon::captain}.ToUInt(), captain_pid_ == pid)
                                        .set(AvatarIcon{AvatarIcon::witch}.ToUInt(), witch_pid_ == pid)
                                        .set(AvatarIcon{AvatarIcon::member}.ToUInt(), !obtained_sword && std::ranges::count(member_pids_, pid))
                                        .set(AvatarIcon{AvatarIcon::sword}.ToUInt(), obtained_sword),
                    k_member_avater_size, html);
            html += HTML_ESCAPE_SPACE;
        }
        html += "\n\n**组队结果**\n\n";
        html += team_up_table_.ToString();
        html += "\n\n</div>";
        Global().Boardcast() << Markdown(std::move(html), 800);
    }

  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << "这里输出当前游戏情况";
        // Returning `OK` means the game stage
        return StageErrCode::OK;
    }

    CompReqErrCode Assassin_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const PlayerID assassin_pid)
    {
        if (!players_[pid].can_assassin_) {
            reply() << "刺杀失败：你不具有刺杀能力";
            return StageErrCode::FAILED;
        }
        auto sender = Global().Boardcast();
        sender << ::Name(pid) << "选择刺杀" << ::Name(assassin_pid) << "，";
        if (GetPlayers()[assassin_pid ].occupation_ == Occupation::梅林) {
            sender << "成功，坏人阵营胜利";
            winner_team_ = Team::坏;
        } else {
            sender << "失败，好人阵营胜利";
            winner_team_ = Team::好;
        }
        return StageErrCode::CHECKOUT;
    }

    void UpdateTeamUpTable_(const std::vector<bool>& players_agree);
    void UpdateMissionTable_(const int32_t failure_num, const std::optional<PlayerID> target_pid);

    bool IsMissionSucc_(const int32_t failure_num) const { return failure_num <= missions_[mission_idx_].is_protected_; }

    std::string PlayerAvatar_(const PlayerID pid, const int32_t size) const
    {
        std::string s;
        AppendPlayerAvatar_(pid, size, s);
        return s;
    }

    void AppendPlayerAvatar_(const PlayerID pid, const int32_t size, std::string& s) const
    {
        AppendPlayerAvatar_(pid, AvatarIcon::BitSet{}, size, s);
    }

    void AppendPlayerAvatar_(const PlayerID pid, const std::bitset<AvatarIcon::Count()> icons, const int32_t size, std::string& s) const
    {
        // parent div header
        s += "<div style=\"position:relative; display:inline-block; width:";
        s += std::to_string(size);
        s += "px; height:";
        s += std::to_string(size);
        s += "px; line-height:";
        s += std::to_string(size);
        s += "px; \">";

        // player avatar div
        s += "<div style=\"position:absolute; top:0; left:0;\">\n\n";
        s += Global().PlayerAvatar(pid, size);
        s += "\n\n</div>";

        // icon dives
        const auto append_icon = [&](const auto& icon_name)
            {
                s += "<div style=\"position:absolute; top:0; left:0;\">\n\n<img src=\"file:///";
                s += Global().ResourceDir();
                s += icon_name;
                s += ".png\" style=\"width:";
                s += std::to_string(size);
                s += "px; height:";
                s += std::to_string(size);
                s += "px; \">\n\n</div>";
            };
        std::ranges::for_each(
                AvatarIcon::Members() | std::views::filter([&](const AvatarIcon icon) { return icons[icon.ToUInt()]; })
                                      | std::views::transform(&AvatarIcon::ToString),
                append_icon);
        append_icon(std::to_string(pid.Get()));

        // parent div tailer
        s += "</div>";
    }

    std::string GetTeamUpTitle_() const
    {
        return "任务 " + std::to_string(mission_idx_ + 1) + " - 第 " + std::to_string(team_up_failed_count_ + 1) + " 轮组队";
    }

    std::string GetActTitle_() const
    {
        return "任务 " + std::to_string(mission_idx_ + 1) + " - 行动";
    }

    void ToNextMission_()
    {
        ++mission_idx_;
        captain_pid_ = (captain_pid_ + 1) % Global().PlayerNum();
        if (GAME_OPTION(兰斯洛特模式) == LancelotMode::implicit_three_rounds && mission_idx_ >= 2) {
            mission_table_.Get(1 + mission_idx_, 1).SetContent(std::string("![](file:///") + Global().ResourceDir() +
                    (missions_[mission_idx_].to_convert_lancelot_ ? "/convert.png)" : "/not_convert.png)"));
        }
        if (missions_[mission_idx_].to_convert_lancelot_) {
            Global().Boardcast() << "请注意，两位兰斯洛特的阵营发生了转换！";
            std::ranges::for_each(
                    std::views::iota(0U, Global().PlayerNum()) |
                        std::views::filter([&](const PlayerID pid) { return players_[pid].occupation_ == Occupation::兰斯洛特; }),
                    [&](const PlayerID pid)
                    {
                        auto& team = players_[pid].team_;
                        const Team new_team = Team::Condition(team == Team::好, Team::坏, Team::好);
                        Global().Tell(pid) << "您的阵营从「" << team << "」变成了" << "「" << new_team << "」";
                        team = new_team;
                    });
        }
    }

    std::vector<Player> players_;
    std::array<Mission, k_mission_num> missions_;
    int32_t mission_succ_count_{0};
    int32_t team_up_failed_count_{0};
    int32_t mission_idx_{0};
    PlayerID captain_pid_{0};
    std::optional<PlayerID> witch_pid_{std::nullopt};
    std::vector<PlayerID> member_pids_;
    std::string occupations_html_;
    html::Table mission_table_;
    html::Table team_up_table_{1, 6};
    Team winner_team_{Team::好};
};

class DetectStage : public SubGameStage<>
{
  public:
    DetectStage(MainStage& main_stage, const PlayerID witch_pid)
        : StageFsm(main_stage, "湖中仙女阶段",
                MakeStageCommand(*this, "湖中仙女玩家决定要验证的玩家 ID", &DetectStage::Detect_, ArithChecker<uint32_t>(0, Global().PlayerNum() - 1, "玩家 ID")))
        , witch_pid_(witch_pid)
    {
        Main().GetPlayers()[witch_pid].has_been_witch_ = true;
    }

    virtual void OnStageBegin() override
    {
        Global().Boardcast() << "请湖中仙女" << At(witch_pid_) << "在 " << GAME_OPTION(投票时限)
            << " 秒私信或公开给出要验证身份的玩家的 ID，如果未能在规定时间给出，则默认验证可验证的 ID 最小的玩家";
        std::ranges::for_each(
                std::views::iota(0U, Global().PlayerNum()) | std::views::filter([&](const PlayerID pid) { return pid != witch_pid_; }),
                [&](const PlayerID pid) { Global().SetReady(pid); });
        Global().StartTimer(GAME_OPTION(投票时限));
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        PlayerID random_pid;
        while (random_pid = rand() % Global().PlayerNum(), Main().GetPlayers()[random_pid].has_been_witch_)
            ;
        detected_pid_ = random_pid;
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnStageTimeout()
    {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (!Main().GetPlayers()[pid].has_been_witch_) {
                detected_pid_ = pid;
                break;
            }
        }
        return StageErrCode::CHECKOUT;
    }

    PlayerID GetDetectedPid() const { return detected_pid_; }

  private:
    AtomReqErrCode Detect_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const uint32_t pid_to_detect)
    {
        if (pid != witch_pid_) {
            reply() << "组队失败：对不起，本轮的湖中仙女是" << ::Name(witch_pid_) << "，只有湖中仙女才可以验证玩家";
            return StageErrCode::FAILED;
        }
        if (Main().GetPlayers()[pid_to_detect].has_been_witch_) {
            reply() << "行动失败：该玩家已经成为过湖中仙女，无法被验证";
            return StageErrCode::FAILED;
        }
        detected_pid_ = pid_to_detect;
        return AtomReqErrCode::CHECKOUT;
    }

    const PlayerID witch_pid_;
    PlayerID detected_pid_;
};

class ActStage : public SubGameStage<>
{
  public:
    ActStage(MainStage& main_stage, const std::vector<PlayerID>& member_pids)
        : StageFsm(main_stage, "行动阶段",
                MakeStageCommand(*this, "玩家决定行动成功或者失败", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY,
                    &ActStage::Act_, BoolChecker("成功", "失败")),
                MakeStageCommand(*this, "持有王者之剑的玩家决定行动成功或者失败，并指定一名玩家反转其行动",
                    CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &ActStage::ActWithSword_,
                    BoolChecker("成功", "失败"), ArithChecker<uint32_t>(0, Global().PlayerNum() - 1, "玩家 ID")))
        , players_succ_(Global().PlayerNum(), true)
        , member_pids_(member_pids)
    {
    }

    virtual void OnStageBegin() override
    {
        {
            auto sender = Global().Boardcast();
            sender << "请参与此次行动的玩家私信「成功」或「失败」以决定此次行动的结果，超时未行动视为「成功」\n";
            std::ranges::for_each(member_pids_, [&sender](const auto pid) { sender << At(pid); });
        }
        std::ranges::for_each(std::views::iota(0U, Global().PlayerNum()), [&](const PlayerID pid) { Global().SetReady(pid); });
        std::ranges::for_each(member_pids_, [&](const PlayerID pid) { Global().ClearReady(pid); });
        Global().StartTimer(GAME_OPTION(投票时限));
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        players_succ_[pid] = Main().GetPlayers()[pid].team_ == Team::好 || rand() % 2 == 0;
        if (GAME_OPTION(王者之剑) && pid == member_pids_.front() && rand() % 2 == 0) {
            reverse_pid_ = member_pids_[rand() % member_pids_.size()];
            if (reverse_pid_ == pid) {
                reverse_pid_ = std::nullopt;
            }
        }
        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        TryReverseAction_();
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnStageOver()
    {
        TryReverseAction_();
        return StageErrCode::CHECKOUT;
    }

    uint32_t GetFailureNumber() const { return std::ranges::count(players_succ_, false); }

    std::optional<PlayerID> GetReversePlayerID() const { return reverse_pid_; }

  private:
    void TryReverseAction_()
    {
        if (!reverse_pid_.has_value()) {
            return;
        }
        const PlayerID sword_pid = member_pids_.front();
        Global().Boardcast() << ::Name(sword_pid) << "反转了" << ::Name(*reverse_pid_) << "的行动";
        Global().Tell(sword_pid) << ::Name(*reverse_pid_) << "原本的行动是："
                                 << (players_succ_[*reverse_pid_] ? "成功" : "失败");
        players_succ_[*reverse_pid_] = !players_succ_[*reverse_pid_];
    }

    AtomReqErrCode ActInternal_(const PlayerID pid, MsgSenderBase& reply, const bool to_succ)
    {
        if (Main().GetPlayers()[pid].team_ == Team::好 && !to_succ) {
            reply() << "行动失败：好人必须让任务成功";
            return AtomReqErrCode::FAILED;
        }
        players_succ_[pid] = to_succ;
        return AtomReqErrCode::READY;
    }

    AtomReqErrCode Act_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const bool to_succ)
    {
        const auto ret = ActInternal_(pid, reply, to_succ);
        if (ret == AtomReqErrCode::FAILED) {
            return ret;
        }
        reply() << "行动成功，您选择了让此次任务" << (to_succ ? "成功" : "失败");
        return ret;
    }

    AtomReqErrCode ActWithSword_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const bool to_succ, const PlayerID pid_to_reverse)
    {
        if (!GAME_OPTION(王者之剑)) {
            reply() << "行动失败：当前游戏未启用王者之剑";
            return AtomReqErrCode::FAILED;
        }
        if (pid != member_pids_.front()) {
            reply() << "行动失败：您未持有王者之剑";
            return AtomReqErrCode::FAILED;
        }
        if (std::ranges::find(member_pids_, pid_to_reverse) == member_pids_.end()) {
            reply() << "行动失败：您指定的玩家未参与此次行动";
            return AtomReqErrCode::FAILED;
        }
        if (pid_to_reverse == pid) {
            reply() << "行动失败：反转自己的行动没有意义";
            return AtomReqErrCode::FAILED;
        }
        const auto ret = Act_(pid, is_public, reply, to_succ);
        if (ret == AtomReqErrCode::FAILED) {
            return ret;
        }
        reply() << "行动成功，您选择了让此次任务" << (to_succ ? "成功" : "失败") << "，并反转了"
                << ::Name(pid_to_reverse) << "的行动，其原本的行动在阶段结束后将被私信给您";
        return ret;
    }

    const std::vector<PlayerID>& member_pids_;
    std::vector<bool> players_succ_;
    std::optional<PlayerID> reverse_pid_;
};

class VoteStage : public SubGameStage<>
{
  public:
    VoteStage(MainStage& main_stage)
        : StageFsm(main_stage, "投票阶段",
                MakeStageCommand(*this, "玩家决定同意或反对队长的组队提议", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY,
                        &VoteStage::Vote_, BoolChecker("同意", "反对")))
        , players_agree_(Global().PlayerNum(), true)
    {
    }

    virtual void OnStageBegin() override
    {
        Global().Boardcast() << "请包括队长在内的所有玩家私信「同意」或「反对」以决定是否按照队长的提议组队行动，"
                                "超时未行动视为「同意」\n只有「同意」的玩家**超过**玩家总数的一半时，组队才算成功";
        Global().StartTimer(GAME_OPTION(投票时限));
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        players_agree_[pid] = rand() % 2;
        return StageErrCode::READY;
    }

    const std::vector<bool>& GetPlayersAgree() const { return players_agree_; }

  private:
    AtomReqErrCode Vote_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const bool to_agree)
    {
        players_agree_[pid] = to_agree;
        reply() << "行动成功，您" << (to_agree ? "同意" : "反对") << "了队长的组队提议";
        return AtomReqErrCode::READY;
    }

    std::vector<bool> players_agree_;
};

class TeamUpStage : public SubGameStage<>
{
  public:
    TeamUpStage(MainStage& main_stage, const PlayerID captain_pid, const uint32_t member_num, std::vector<PlayerID>& member_pids)
        : StageFsm(main_stage, "组队阶段",
                MakeStageCommand(*this, "队长提出参加此次行动的玩家的 ID 列表（如果启用了王者之剑，则列表第一位玩家持有王者之剑）",
                    &TeamUpStage::TeamUp_, RepeatableChecker<ArithChecker<uint32_t>>(0U, main_stage.Global().PlayerNum() - 1)))
        , captain_pid_{captain_pid}
        , member_num_{member_num}
        , member_pids_(member_pids)
    {
        member_pids_.clear();
        member_pids_.reserve(member_num);
        assert(member_num <= Global().PlayerNum());
    }

    virtual void OnStageBegin() override
    {
        if (GAME_OPTION(王者之剑)) {
            Global().Boardcast() << "请队长" << At(captain_pid_) << "在 " << GAME_OPTION(组队时限)
                << " 秒私信或公开给出参加本次行动的玩家的 ID，其中第一个 ID 为持有王者之剑的玩家，例如「1 4 6 10」"
                   "（1 号玩家持有王者之剑）\n如果未能在规定时间给出，则默认为从当前队长开始顺时针的 "
                << member_pids_.size() << " 名玩家，队长的顺时针的下一位玩家持有王者之剑";
        } else {
            Global().Boardcast() << "请队长" << At(captain_pid_) << "在 " << GAME_OPTION(组队时限)
                << " 秒私信或公开给出参加本次行动的玩家的 ID，例如「1 4 6 10」\n如果未能在规定时间给出，则默认为从当前队长开始顺时针的 "
                << member_pids_.size() << " 名玩家";
        }
        std::ranges::for_each(
                std::views::iota(0U, Global().PlayerNum()) | std::views::filter([&](const PlayerID pid) { return pid != captain_pid_; }),
                [&](const PlayerID pid) { Global().SetReady(pid); });
        Global().StartTimer(GAME_OPTION(组队时限));
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        std::vector<PlayerID> pids;
        pids.reserve(Global().PlayerNum());
        std::ranges::copy(std::views::iota(0U, Global().PlayerNum()), std::back_inserter(pids));
        std::random_device rd;
        std::mt19937 gen {rd()};
        std::ranges::shuffle(pids, gen);
        std::ranges::copy(pids | std::views::take(member_num_), std::back_inserter(member_pids_));
        if (member_pids_[0] == pid) {
            std::swap(member_pids_[0], member_pids_[1]);
        }
        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        std::ranges::copy(std::views::iota(captain_pid_.Get(), Global().PlayerNum()) | std::views::take(member_num_), std::back_inserter(member_pids_));
        std::ranges::copy(std::views::iota(0U, member_num_ - member_pids_.size()), std::back_inserter(member_pids_));
        std::swap(member_pids_[0], member_pids_[1]);
        return StageErrCode::CHECKOUT;
    }

  private:
    static bool HasRepeatedElements_(const std::vector<uint32_t>& span)
    {
        for (auto it = span.begin(); it != span.end(); ++it) {
            if (std::ranges::any_of(std::next(it), span.end(), [cur_v = *it](const uint32_t v) { return cur_v == v; })) {
                return true;
            }
        }
        return false;
    }

    AtomReqErrCode TeamUp_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::vector<uint32_t>& attempt_member_pids)
    {
        if (pid != captain_pid_) {
            reply() << "组队失败：对不起，本轮的队长是 " << ::Name(captain_pid_) << "，只有队长才可以发起组队";
            return StageErrCode::FAILED;
        }
        if (attempt_member_pids.size() != member_num_) {
            reply() << "组队失败：您给出了 " << attempt_member_pids.size() << " 名玩家，但实际需要 " << member_num_ << " 名玩家";
            return StageErrCode::FAILED;
        }
        if (HasRepeatedElements_(attempt_member_pids)) {
            reply() << "组队失败：您给出的玩家 ID 中包含了重复的元素";
            return StageErrCode::FAILED;
        }
        if (GAME_OPTION(王者之剑) && attempt_member_pids[0] == pid) {
            reply() << "组队失败：队长本人无法持有王者之剑，不能放在列表的第一位";
            return StageErrCode::FAILED;
        }
        std::ranges::copy(attempt_member_pids | std::views::transform([](const uint32_t pid) { return PlayerID{pid}; }),
                std::back_inserter(member_pids_));
        return StageErrCode::CHECKOUT;
    }

    const PlayerID captain_pid_;
    const uint32_t member_num_;
    std::vector<PlayerID>& member_pids_;
};

class AssassinStage : public SubGameStage<>
{
  public:
    AssassinStage(MainStage& main_stage) : StageFsm(main_stage, "刺杀阶段")
    {
    }

    virtual void OnStageBegin() override
    {
        Global().StartTimer(GAME_OPTION(投票时限));
    }
};

void MainStage::FirstStageFsm(SubStageFsmSetter setter)
{
    const auto append_bad_team_players = [&](auto& sender, const Occupation except_occupation)
        {
            sender << "- 除【" << except_occupation << "】以外的「坏人阵营」玩家包括 ";
            for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
                if (players_[pid].team_ == Team::坏 && players_[pid].occupation_ != except_occupation) {
                    sender << ::Name(pid) << " ";
                }
            }
        };
    Global().Boardcast() << "游戏开始，将私信各位玩家身份";
    for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
        auto sender = Global().Tell(pid);
        sender << "您的身份是【" << players_[pid].occupation_ << "】，属于「" << players_[pid].team_ << "人阵营」";
        switch (players_[pid].occupation_) {
            case Occupation::莫德雷德:
                sender << "\n\n- 【梅林】看不到你\n";
                append_bad_team_players(sender, Occupation::奥伯伦);
                break;
            case Occupation::莫甘娜:
                sender << "\n\n- 【派西维尔】能看到但区分不出你和【梅林】\n";
                append_bad_team_players(sender, Occupation::奥伯伦);
                break;
            case Occupation::奥伯伦:
                sender << "\n\n- 你看不到「坏人阵营」的玩家，「坏人阵营」的玩家也看不到你";
                break;
            case Occupation::刺客:
            case Occupation::莫德雷德的爪牙:
                sender << "\n\n";
                append_bad_team_players(sender, Occupation::奥伯伦);
                break;
            case Occupation::梅林:
                sender << "\n\n";
                append_bad_team_players(sender, Occupation::莫德雷德);
                break;
            case Occupation::派西维尔:
                sender << "\n\n - 【梅林】和【莫甘娜】在";
                {
                    bool found_one = false;
                    for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
                        if (players_[pid].occupation_ != Occupation::梅林 && players_[pid].occupation_ != Occupation::莫甘娜) {
                            continue;
                        }
                        sender << ::Name(pid);
                        if (std::exchange(found_one, true) == false) {
                            sender << "和";
                        }
                    }
                }
                sender << "之间";
                break;
            case Occupation::兰斯洛特:
                sender << "\n\n- 场上有两名分别位于不同阵营的【兰斯洛特】——";
                switch (GAME_OPTION(兰斯洛特模式)) {
                    case LancelotMode::implicit_three_rounds:
                        sender << "从第三次任务开始，每次任务开始前，会翻出一张兰斯洛特转换卡，如果翻出了「转换」标识，则你和另一名【兰斯洛特】的阵营对换";
                        break;
                    case LancelotMode::explicit_five_rounds:
                        sender << "游戏开始时，会为每次任务各翻出一张兰斯洛特转换卡，如果翻出了「转换」标识，则在该次任务开始前，你和另一名【兰斯洛特】的阵营对换";
                        break;
                    case LancelotMode::recognition:
                        sender << "另一名【兰斯洛特】是";
                        for (PlayerID other_pid = 0; other_pid < Global().PlayerNum(); ++other_pid) {
                            if (players_[pid].occupation_ == Occupation::兰斯洛特 && pid != other_pid) {
                                sender << ::Name(other_pid);
                                break;
                            }
                        }
                }
                break;
        }
        if (players_[pid].can_assassin_) {
            sender << "\n- 你可以随时刺杀【梅林】，若刺杀成功，「坏人阵营」胜利，否则「好人阵营」胜利";
        }
    }
    setter.Emplace<TeamUpStage>(*this, captain_pid_, missions_[mission_idx_].member_num_, member_pids_);
    ShowHtml(GetTeamUpTitle_());
}

static constexpr uint32_t k_max_team_up_failed_count = 4;

void MainStage::NextStageFsm(TeamUpStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    {
        auto sender = Global().Boardcast();
        if (reason == CheckoutReason::BY_TIMEOUT) {
            sender << "队长组队超时，本轮参与行动的玩家为：\n";
        } else {
            sender << "队长提出组队成功，本轮参与行动的玩家为：\n";
        }
        std::ranges::for_each(member_pids_, [&sender](const auto pid) { sender << At(pid); });
    }
    if (team_up_failed_count_ >= k_max_team_up_failed_count) {
        Global().Boardcast() << "由于本轮已经组队失败了四次，本次组队将不再投票，直接行动";
        team_up_failed_count_ = 0;
        setter.Emplace<ActStage>(*this, member_pids_);
        ShowHtml(GetActTitle_());
    } else {
        setter.Emplace<VoteStage>(*this);
        ShowHtml(GetTeamUpTitle_());
    }
}

static bool IsTeamUpSucc(const std::vector<bool>& players_agree, const int32_t player_num)
{
    return std::ranges::count(players_agree, true) > player_num / 2;
}

void MainStage::UpdateTeamUpTable_(const std::vector<bool>& players_agree)
{
    team_up_table_.AppendRow();
    team_up_table_.GetLastRow(0).SetContent(std::to_string(1 + mission_idx_));
    team_up_table_.GetLastRow(1).SetContent(PlayerAvatar_(captain_pid_, k_team_up_avater_size));
    team_up_table_.GetLastRow(2).SetContent([&]()
            {
                std::string s;
                std::ranges::for_each(member_pids_,
                        [&](const PlayerID pid) { AppendPlayerAvatar_(pid, k_team_up_avater_size, s); });
                return s;
            }());
    std::string agree_avatars;
    std::string disagree_avatars;
    int32_t agree_num = 0;
    int32_t disagree_num = 0;
    const auto append_avatar = [&](const PlayerID pid, std::string& avatars, int32_t& num)
        {
            if (num++ == 5) {
                avatars += "<br>";
            }
            AppendPlayerAvatar_(pid, k_team_up_avater_size, avatars);
        };
    for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
        if (players_agree[pid]) {
            append_avatar(pid, agree_avatars, agree_num);
        } else {
            append_avatar(pid, disagree_avatars, disagree_num);
        }
    }
    team_up_table_.GetLastRow(3).SetContent(std::move(agree_avatars));
    team_up_table_.GetLastRow(4).SetContent(std::move(disagree_avatars));
    team_up_table_.GetLastRow(5).SetContent(
            IsTeamUpSucc(players_agree, Global().PlayerNum()) ? HTML_COLOR_FONT_HEADER(green) "成功" HTML_FONT_TAIL
                                                              : HTML_COLOR_FONT_HEADER(red) "失败" HTML_FONT_TAIL);
}

void MainStage::NextStageFsm(VoteStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    UpdateTeamUpTable_(sub_stage.GetPlayersAgree());
    if (IsTeamUpSucc(sub_stage.GetPlayersAgree(), Global().PlayerNum())) {
        Global().Boardcast() << "多数派同意，组队成功！";
        team_up_failed_count_ = 0;
        setter.Emplace<ActStage>(*this, member_pids_);
        ShowHtml(GetActTitle_());
    } else {
        Global().Boardcast() << "组队失败，本轮已经失败了 " << ++team_up_failed_count_ << " 次（失败 " << k_max_team_up_failed_count
                    << " 次时的下一次组队将不再投票，直接行动）\n接下来将变更队长，进行下一次组队";
        captain_pid_ = (captain_pid_ + 1) % Global().PlayerNum();
        setter.Emplace<TeamUpStage>(*this, captain_pid_, missions_[mission_idx_].member_num_, member_pids_);
        ShowHtml(GetTeamUpTitle_());
    }
}

static std::string GetMissionResultString(const std::string& resource_dir, const int32_t member_num, const int32_t failure_num)
{
    std::string s;
    for (int32_t i = 0; i < member_num - failure_num; ++i) {
        s += "![](file:///" + resource_dir + "/succ.png)";
    }
    for (int32_t i = 0; i < failure_num; ++i) {
        s += "![](file:///" + resource_dir + "/fail.png)";
    }
    return s;
}

void MainStage::UpdateMissionTable_(const int32_t failure_num, const std::optional<PlayerID> reverse_pid)
{
    const bool with_lancelot_card = NeedLancelotCard(GAME_OPTION(兰斯洛特模式));
    mission_table_.Get(1 + mission_idx_, 1 + with_lancelot_card).SetContent(
            GetMissionResultString(Global().ResourceDir(), missions_[mission_idx_].member_num_, failure_num));
    mission_table_.Get(1 + mission_idx_, 2 + with_lancelot_card).SetContent(PlayerAvatar_(captain_pid_, k_mission_avater_size));
    mission_table_.Get(1 + mission_idx_, 3 + with_lancelot_card).SetContent([&]()
            {
                std::string s;
                std::ranges::for_each(member_pids_, [&](const PlayerID pid)
                        {
                            AppendPlayerAvatar_(pid,
                                    AvatarIcon::BitSet{}.set(AvatarIcon{AvatarIcon::sword}.ToUInt(), GAME_OPTION(王者之剑) && pid == member_pids_.front())
                                                        .set(AvatarIcon{AvatarIcon::target}.ToUInt(), reverse_pid == pid),
                                    k_mission_avater_size, s);
                        });
                return s;
            }());
    mission_table_.Get(1 + mission_idx_, 4 + with_lancelot_card).SetContent(
            IsMissionSucc_(failure_num) ? HTML_COLOR_FONT_HEADER(green) "成功" HTML_FONT_TAIL : HTML_COLOR_FONT_HEADER(red) "失败" HTML_FONT_TAIL);
}

void MainStage::NextStageFsm(ActStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    UpdateMissionTable_(sub_stage.GetFailureNumber(), sub_stage.GetReversePlayerID());
    {
        auto sender = Global().Boardcast();
        if (IsMissionSucc_(sub_stage.GetFailureNumber())) {
            sender << "行动告捷！当前好人已经行动成功 " << ++mission_succ_count_ << " 次";
            if (mission_succ_count_ > k_mission_num / 2) {
                const PlayerID assassin_pid = std::distance(players_.begin(),
                        std::ranges::find_if(players_, [](const auto& player) { return player.can_assassin_; }));
                sender << "，达到了任务成功次数的要求，但是" << At{assassin_pid} << "，你还有翻盘的机会，使用「刺杀 梅林ID」（例如「刺杀 0」），一击干掉他！";
                setter.Emplace<AssassinStage>(*this);
                ShowHtml("终局 - 绝地反击");
                return;
            }
        } else {
            const auto mission_failure_num = mission_idx_ + 1 - mission_succ_count_;
            sender << "噢不，行动失败了...当前好人已经行动失败 " << mission_failure_num << " 次";
            if (mission_failure_num > k_mission_num / 2) {
                sender << "，无力回天了，坏人宣告胜利";
                winner_team_ = Team::坏;
                ShowHtml("终局");
                return;
            }
        }
    }
    if (witch_pid_.has_value() && mission_idx_ > 0) {
        setter.Emplace<DetectStage>(*this, *witch_pid_);
        ShowHtml(GetActTitle_());
    } else {
        ToNextMission_();
        setter.Emplace<TeamUpStage>(*this, captain_pid_, missions_[mission_idx_].member_num_, member_pids_);
        ShowHtml(GetTeamUpTitle_());
    }
}

void MainStage::NextStageFsm(DetectStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    const auto detected_pid = sub_stage.GetDetectedPid();
    Global().Tell(*witch_pid_) << ::Name(sub_stage.GetDetectedPid()) << "当前的阵营是「"
                               << players_[detected_pid].team_.ToString() << "」";
    {
        auto sender = Global().Boardcast();
        sender  << (reason == CheckoutReason::BY_TIMEOUT ? "湖中仙女选择超时，默认选择验证" : "湖中仙女选择验证")
                << At(detected_pid) << "的阵营，其结果已私信告知";
        if (mission_idx_ != k_mission_num - 2) {
            sender << "，接下来由" << At(detected_pid) << "担任湖中仙女";
            witch_pid_ = detected_pid;
        } else {
            witch_pid_ = std::nullopt;
        }
    }
    ToNextMission_();
    setter.Emplace<TeamUpStage>(*this, captain_pid_, missions_[mission_idx_].member_num_, member_pids_);
    ShowHtml(GetTeamUpTitle_());
}

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot

#endif
