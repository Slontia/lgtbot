// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"

#include "game_util/poker.h"
#include "game_util/bet_pool.h"

using namespace lgtbot::game_util::bet_pool;
namespace poker = lgtbot::game_util::poker;

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;
const GameProperties k_properties { 
    .name_ = "德州波卡", // the game name which should be unique among all the games
    .developer_ = "森高",
    .description_ = "同时下注或加注的德州波卡游戏",
};
constexpr uint64_t k_max_player = 15;
uint64_t MaxPlayerNum(const CustomOptions& options) { return k_max_player; }
uint32_t Multiple(const CustomOptions& options)
{
    return GET_OPTION_VALUE(options, 种子).empty() ? GET_OPTION_VALUE(options, 局数) / 4 : 0;
}
const MutableGenericOptions k_default_generic_options;
const std::vector<RuleCommand> k_rule_commands = {};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (GET_OPTION_VALUE(game_options, 幸存) >= generic_options_readonly.PlayerNum()) {
        reply() << "幸存玩家数 " << GET_OPTION_VALUE(game_options, 幸存) << " 必须小于参赛玩家数 " << generic_options_readonly.PlayerNum();
        return false;
    }
    if (GET_OPTION_VALUE(game_options, 底注变化).empty()) {
        reply() << "底注增长局数不允许为空";
        return false;
    }
    if (generic_options_readonly.PlayerNum() < 2) {
        reply() << "该游戏至少 2 人参加，当前玩家数为 " << generic_options_readonly.PlayerNum();
        return false;
    }
    return true;
}

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options)
            {
                generic_options.bench_computers_to_player_num_ = 10;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
};

// ========== GAME STAGES ==========

static constexpr const uint32_t k_public_card_num = 5;
static constexpr const uint32_t k_private_card_num = 5;

static constexpr const uint32_t k_markdown_width = 700;

struct PlayerChipInfo
{
    explicit PlayerChipInfo(const int32_t chips)
        : remain_chips_(chips)
        , bet_chips_(0)
        , last_bet_chips_(0)
        , is_fold_(false)
        , action_(NO_ACTION)
        , history_remain_chips_{remain_chips_}
    {}

    int32_t remain_chips_;
    int32_t bet_chips_;
    int32_t last_bet_chips_;
    bool is_fold_;
    enum { CALL, RAISE, BET, CHECK, ALLIN, FOLD, BLIND, NO_ACTION } action_;
    std::vector<int32_t> history_remain_chips_;
};

const char* Action2Str(const int action)
{
    static constexpr const char* k_strs[] = {
        [PlayerChipInfo::CALL] = "(Call)",
        [PlayerChipInfo::RAISE] = "(Raise)",
        [PlayerChipInfo::BET] = "(Bet)",
        [PlayerChipInfo::CHECK] = "(Check)",
        [PlayerChipInfo::ALLIN] = "(All In)",
        [PlayerChipInfo::FOLD] = "(Fold)",
        [PlayerChipInfo::BLIND] = "(Blind)",
        [PlayerChipInfo::NO_ACTION] = "",
    };
    return k_strs[action];
}

void Reset(PlayerChipInfo& info)
{
    if (info.bet_chips_ > 0) {
        info.history_remain_chips_.emplace_back(info.remain_chips_);
    }
    info.bet_chips_ = 0;
    info.last_bet_chips_ = 0;
    info.is_fold_ = info.remain_chips_ == 0;
    info.action_ = PlayerChipInfo::NO_ACTION;
}

void Consume(PlayerChipInfo& info, const int32_t chips)
{
    info.last_bet_chips_ = info.bet_chips_;
    info.bet_chips_ += chips;
    info.remain_chips_ -= chips;
}

void NoAction(PlayerChipInfo& info)
{
    info.last_bet_chips_ = info.bet_chips_;
    if (info.action_ != PlayerChipInfo::FOLD && info.action_ != PlayerChipInfo::ALLIN) {
        info.action_ = PlayerChipInfo::NO_ACTION;
    }
}

bool AchieveMaxBet(const PlayerChipInfo& info, const int32_t max_bet_chips)
{
    return info.remain_chips_ == 0 || info.bet_chips_ == max_bet_chips || info.is_fold_;
}

template <poker::CardType k_type>
class RoundStage;

class MainStage : public MainGameStage<RoundStage<poker::CardType::POKER>, RoundStage<poker::CardType::BOKAA>>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility))
        , round_(1)
        , player_chip_infos_(Global().PlayerNum(), PlayerChipInfo(GAME_OPTION(筹码)))
    {
    }

    virtual void FirstStageFsm(SubStageFsmSetter setter) override;

    virtual void NextStageFsm(RoundStage<poker::CardType::POKER>& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override
    {
        return NextSubStage_(sub_stage, reason, setter);
    }

    virtual void NextStageFsm(RoundStage<poker::CardType::BOKAA>& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override
    {
        return NextSubStage_(sub_stage, reason, setter);
    }

    virtual int64_t PlayerScore(const PlayerID pid) const override
    {
        return player_chip_infos_[pid].remain_chips_;
    }

    PlayerChipInfo& GetPlayerChipInfo(const PlayerID pid) { return player_chip_infos_[pid]; }
    const PlayerChipInfo& GetPlayerChipInfo(const PlayerID pid) const { return player_chip_infos_[pid]; }
    const std::vector<PlayerChipInfo>& GetPlayerChipInfos() const { return player_chip_infos_; }

  private:
    template <poker::CardType k_type>
    void NextSubStage_(RoundStage<k_type>& sub_stage, const CheckoutReason reason, SubStageFsmSetter& setter)
    {
        if (GAME_OPTION(幸存) >= std::ranges::count_if(player_chip_infos_,
                [](const PlayerChipInfo& chip_info) { return chip_info.remain_chips_ > 0; })) {
            Global().Boardcast() << "达到了幸存玩家人数设置阈值，游戏结束";
            return;
        }
        if (round_ == GAME_OPTION(局数)) {
            Global().Boardcast() << "达到了最大回合限制，游戏结束";
            return;
        }
        setter.Emplace<RoundStage<k_type>>(*this, ++round_);
    }

    int round_;
    std::vector<PlayerChipInfo> player_chip_infos_;
};

static void ConsumeAndReply(const int32_t chips, PlayerChipInfo& chip_info, MsgSenderBase& reply)
{
    Consume(chip_info, chips);
    if (chip_info.remain_chips_ == 0) {
        reply() << "您投入了全部的 " << chips << " 枚筹码（累计 " << chip_info.bet_chips_ << " 枚），成败在此一举";
        chip_info.action_ = PlayerChipInfo::ALLIN;
    } else {
        reply() << "您本次操作投入了 " << chips << " 枚筹码（累计 " << chip_info.bet_chips_ << " 枚），剩余 "
            << chip_info.remain_chips_ << " 枚";
    }
}

class RaiseStage : public SubGameStage<>
{
  public:
    RaiseStage(MainStage& main_stage, const char* const state, const int32_t bet_chips, const int32_t raise_chips)
        : StageFsm(main_stage, std::string(state) + " 加注阶段",
                MakeStageCommand(*this, "投入所有的筹码", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY,
                    &RaiseStage::AllIn_, VoidChecker("allin", "a")),
                MakeStageCommand(*this, "跟注，并加注一定的筹码", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY,  &RaiseStage::Raise_,
                    VoidChecker("raise", "r"), ArithChecker<int32_t>(1, INT32_MAX, "筹码数")),
                MakeStageCommand(*this, "跟注（筹码不足时会 all in）", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY,
                    &RaiseStage::Call_, VoidChecker("call", "c")),
                MakeStageCommand(*this, "弃牌", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &RaiseStage::Fold_,
                    VoidChecker("fold", "f")))
        , bet_chips_(bet_chips)
        , raise_chips_(raise_chips)
        , max_raise_chips_(0)
    {
    }

    int32_t MaxRaiseChips() const { return max_raise_chips_; }

  private:
    void FoldForUnreadyPlayers_()
    {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (!Global().IsReady(pid)) {
                Fold_(pid, false, EmptyMsgSender::Get());
                Global().Tell(pid) << "您超时未行动，自动按照 fold 处理";
            }
        }
        Global().HookUnreadyPlayers();
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        FoldForUnreadyPlayers_();
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnStageOver()
    {
        FoldForUnreadyPlayers_();
        return StageErrCode::CHECKOUT;
    }

    virtual void OnStageBegin() override
    {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            auto& chip_info = Main().GetPlayerChipInfo(pid);
            if (AchieveMaxBet(chip_info, bet_chips_)) {
                // if has no remain chips or raises highest chips, need not take action
                NoAction(chip_info);
                Global().SetReady(pid);
            }
        }
        Global().StartTimer(GAME_OPTION(时限));
        Global().Boardcast() << "请未达到最高下注额的玩家，私信裁判行动，您的行动可以是 <call> <raise 加注部分筹码数> <fold> <allin> 中的一种";
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        if (rand() % Global().PlayerNum() == 0 && StageErrCode::READY == Raise_(pid, false, reply, rand() % 50 + raise_chips_)) {
            // raise successfully
            return StageErrCode::READY;
        }
        if (rand() % Global().PlayerNum() <= 1) {
            Fold_(pid, false, reply);
        } else {
            Call_(pid, false, reply);
        }
        return StageErrCode::READY;
    }

    AtomReqErrCode AllIn_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        auto& chip_info = Main().GetPlayerChipInfo(pid);
        if (const int32_t total_chips = chip_info.remain_chips_ + chip_info.bet_chips_; total_chips > bet_chips_) {
            max_raise_chips_ = std::max(max_raise_chips_, total_chips - bet_chips_);
        }
        ConsumeAndReply(chip_info.remain_chips_, chip_info, reply);
        return StageErrCode::READY;
    }

    AtomReqErrCode Raise_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int32_t chips)
    {
        auto& chip_info = Main().GetPlayerChipInfo(pid);
        assert(bet_chips_ > chip_info.bet_chips_); // if equal, the status of the player should be READY and can not execute any commands
        const int32_t offset_chips = bet_chips_ + chips - chip_info.bet_chips_;
        if (offset_chips > chip_info.remain_chips_) {
            reply() << "加注失败：筹码不足（当前剩余 " << chip_info.remain_chips_ << " 枚，需要 " << offset_chips << " 枚）";
            return StageErrCode::FAILED;
        }
        if (offset_chips < chip_info.remain_chips_ && chips < raise_chips_) {
            reply() << "加注失败：加注额必须大于等于 " << raise_chips_ << " 枚，或者您可以加注 "
                << (chip_info.remain_chips_ + chip_info.bet_chips_ - bet_chips_) << " 枚以投入全部筹码";
            return StageErrCode::FAILED;
        }
        max_raise_chips_ = std::max(max_raise_chips_, chips);
        chip_info.action_ = PlayerChipInfo::RAISE;
        ConsumeAndReply(offset_chips, chip_info, reply);
        return StageErrCode::READY;
    }

    AtomReqErrCode Call_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        auto& chip_info = Main().GetPlayerChipInfo(pid);
        const int32_t chips = std::min(chip_info.remain_chips_, bet_chips_ - chip_info.bet_chips_);
        chip_info.action_ = PlayerChipInfo::CALL;
        ConsumeAndReply(chips, chip_info, reply);
        return StageErrCode::READY;
    }

    AtomReqErrCode Fold_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        auto& chip_info = Main().GetPlayerChipInfo(pid);
        chip_info.last_bet_chips_ = chip_info.bet_chips_;
        chip_info.action_ = PlayerChipInfo::FOLD;
        chip_info.is_fold_ = true;
        reply() << "弃牌成功";
        return StageErrCode::READY;
    }

    const int32_t bet_chips_;
    const int32_t raise_chips_;
    int32_t max_raise_chips_;
};

class BetStage : public SubGameStage<>
{
  public:
    BetStage(MainStage& main_stage, const char* const state, const uint32_t base_chips)
        : StageFsm(main_stage, std::string(state) + " 下注阶段",
                MakeStageCommand(*this, "投入所有的筹码", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY,
                    &BetStage::AllIn_, VoidChecker("allin", "a")),
                MakeStageCommand(*this, "下注一定的筹码", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &BetStage::Bet_,
                    VoidChecker("bet", "b"), ArithChecker<int32_t>(1, INT32_MAX, "筹码数")),
                MakeStageCommand(*this, "过牌", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &BetStage::Check_,
                    VoidChecker("check", "c")))
        , max_raise_chips_(0)
        , base_chips_(base_chips)
    {
    }

    int32_t MaxRaiseChips() const { return max_raise_chips_; }

  private:
    void CheckForUnreadyPlayers_()
    {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (!Global().IsReady(pid)) {
                Check_(pid, false, EmptyMsgSender::Get());
                Global().Tell(pid) << "您超时未行动，自动按照 check 处理";
            }
        }
        Global().HookUnreadyPlayers();
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        CheckForUnreadyPlayers_();
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnStageOver()
    {
        CheckForUnreadyPlayers_();
        return StageErrCode::CHECKOUT;
    }

    virtual void OnStageBegin() override
    {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            auto& chip_info = Main().GetPlayerChipInfo(pid);
            if (chip_info.remain_chips_ == 0 || chip_info.is_fold_) {
                // if has no remain chips or raises highest chips, need not take action
                NoAction(chip_info);
                Global().SetReady(pid);
            }
        }
        Global().StartTimer(GAME_OPTION(时限));
        Global().Boardcast() << "请私信裁判行动，您的行动可以是 <check> <bet 筹码数> <allin> 中的一种";
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        if (rand() % 2 || StageErrCode::READY != Bet_(pid, false, reply, base_chips_)) {
            Check_(pid, false, reply);
        }
        return StageErrCode::READY;
    }

    AtomReqErrCode AllIn_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        auto& chip_info = Main().GetPlayerChipInfo(pid);
        max_raise_chips_ = std::max(max_raise_chips_, chip_info.remain_chips_);
        ConsumeAndReply(chip_info.remain_chips_, chip_info, reply);
        return StageErrCode::READY;
    }

    AtomReqErrCode Bet_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int32_t chips)
    {
        auto& chip_info = Main().GetPlayerChipInfo(pid);
        if (chips > chip_info.remain_chips_) {
            reply() << "下注失败：筹码不足（当前剩余 " << chip_info.remain_chips_ << " 枚）";
            return StageErrCode::FAILED;
        }
        if (chips < chip_info.remain_chips_ && chips % base_chips_) {
            reply() << "下注失败：下注额必须是底注 " << base_chips_ << " 的整数倍，或者您可以下注 "
                << chip_info.remain_chips_ << " 枚以投入全部筹码";
            return StageErrCode::FAILED;
        }
        max_raise_chips_ = std::max(max_raise_chips_, chips);
        chip_info.action_ = PlayerChipInfo::BET;
        ConsumeAndReply(chips, chip_info, reply);
        return StageErrCode::READY;
    }

    AtomReqErrCode Check_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        auto& chip_info = Main().GetPlayerChipInfo(pid);
        chip_info.last_bet_chips_ = chip_info.bet_chips_;
        chip_info.action_ = PlayerChipInfo::CHECK;
        reply() << "您选择不下注";
        return StageErrCode::READY;
    }

    int32_t max_raise_chips_;
    const uint32_t base_chips_;
};

template <poker::CardType k_type>
class RoundStage : public SubGameStage<RaiseStage, BetStage>
{
  public:
    // round is started from 1
    RoundStage(MainStage& main_stage, const uint64_t round)
        : StageFsm(main_stage,
                "第 " + std::to_string(round) + " / " + std::to_string(GET_OPTION_VALUE(main_stage.Global().Options(), 局数)) + " 局",
                MakeStageCommand(*this, "查看当前游戏进展情况", &RoundStage::Status_, VoidChecker("赛况")))
        , base_chips_([&]()
                {
                    const auto index = (round - 1) / GAME_OPTION(底注变化局数);
                    const auto& base_bet_list = GAME_OPTION(底注变化);
                    return index >= base_bet_list.size() ? base_bet_list.back() : base_bet_list[index];
                }())
        , bet_chips_(base_chips_)
        , raise_chips_(base_chips_)
        , open_public_cards_num_(0)
    {
        Global().Boardcast() << Name() << "开始，将私信各位玩家手牌信息";
        const auto& seed_in_option = GAME_OPTION(种子);
        const auto shuffled_pokers = poker::ShuffledPokers<k_type>(
                 seed_in_option.empty() ? "" : (seed_in_option + std::to_string(round)));
        auto poker_it = shuffled_pokers.begin();
        // fill `public_cards_`
        for (auto& card : public_cards_) {
            card = *(poker_it++);
        }
        // fill `player_hand_infos_`
        player_hand_infos_.reserve(Global().PlayerNum());
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            std::array<poker::Card<k_type>, 2> private_cards{*(poker_it++), *(poker_it++)};
            poker::Hand<k_type> hand;
            for (const auto& card : private_cards) {
                hand.Add(card);
            }
            for (const auto& card : public_cards_) {
                hand.Add(card);
            }
            player_hand_infos_.emplace_back(private_cards, hand.BestDeck());
#if TEST_BOT
            std::cout << "Player " << pid << " best deck: " << hand.BestDeck()->ToString() << std::endl;
#endif
            if (!main_stage.GetPlayerChipInfo(pid).is_fold_) {
                Global().Tell(pid) << "您的手牌是 " << player_hand_infos_[pid].ToString();
            }
        }
        std::copy(poker_it, shuffled_pokers.end(), std::back_inserter(unused_cards_));
    }

    virtual void FirstStageFsm(SubStageFsmSetter setter) override
    {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            auto& info = Main().GetPlayerChipInfo(pid);
            Consume(info, std::min(base_chips_, info.remain_chips_)); // bet base chips
            info.action_ = info.bet_chips_ == 0    ? PlayerChipInfo::NO_ACTION :
                           info.remain_chips_ == 0 ? PlayerChipInfo::ALLIN     : PlayerChipInfo::BLIND;
        }
        html_ = Html_(nullptr, false, false);
        Global().Boardcast() << Markdown(html_, k_markdown_width);
        Global().SaveMarkdown(Html_(nullptr, true, false), k_markdown_width);
        if (TryOpenCard_()) {
            return;
        }
        setter.Emplace<BetStage>(this->Main(), StateName_(), base_chips_);
    }

    virtual void NextStageFsm(RaiseStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override
    {
        return NextSubStage_(sub_stage.MaxRaiseChips(), setter);
    }

    virtual void NextStageFsm(BetStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override
    {
        return NextSubStage_(sub_stage.MaxRaiseChips(), setter);
    }

  private:
    struct PlayerWinInfo
    {
        uint64_t remain_chips_before_open_ = 0;
        bool is_winner_ = false;
    };

    struct PlayerHandInfo
    {
        std::string ToString() const { return hand_[0].ToString() + " " + hand_[1].ToString(); }

        std::string ToHtml() const { return hand_[0].ToHtml() + " " + hand_[1].ToHtml(); }

        std::array<poker::Card<k_type>, 2> hand_;
        poker::OptionalDeck<k_type> deck_;
        double win_possibility_ = 0;
    };

    const char* StateName_() const { return k_state_names_[open_public_cards_num_]; }

    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (html_.empty()) {
            reply() << "本局还未开始下注";
        } else {
            reply() << Markdown(html_, k_markdown_width);
        }
        if (!is_public) {
            reply() << "您的手牌是 " << player_hand_infos_[pid].ToString();
        }
        return StageErrCode::OK;
    }

    std::string PlayerHeader_(const PlayerID pid) const
    {
        return "<font size=\"3\">" + this->Global().PlayerAvatar(pid, 30) + HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE " **" + this->Global().PlayerName(pid) + "** </font>\n\n";
    }

    // If `win_info` is not null, show winner players' deck and all public cards.
    // If `win_possibility` is not null, show players' hand and win possibilities.
    std::pair<std::string, const char*> PlayerHtml_(const PlayerID pid, const PlayerWinInfo* const win_info, const bool show_hand, const bool with_bg_color) const
    {
        const auto& chip_info = this->Main().GetPlayerChipInfo(pid);
        // mark the name of players obtain the highest chips red
        std::string s = PlayerHeader_(pid);
        s += HTML_FONT_TAIL "\n\n<center>\n\n";
        if (chip_info.bet_chips_ == 0) {
            // the player is elimindated iff he did not bet base chips
            s += HTML_COLOR_FONT_HEADER(grey) "已淘汰" HTML_FONT_TAIL;
            return {s, "#d1d1d1"};
        }
        if (win_info && win_info->remain_chips_before_open_ < chip_info.remain_chips_) {
            s += "可支配筹码：**" + std::to_string(win_info->remain_chips_before_open_) + " → " HTML_COLOR_FONT_HEADER(green) +
                std::to_string(chip_info.remain_chips_) + "**" HTML_FONT_TAIL "\n\n";
        } else {
            s += "可支配筹码：" HTML_COLOR_FONT_HEADER(green) "**" + std::to_string(chip_info.remain_chips_) + "**" HTML_FONT_TAIL "\n\n";
        }
        s += "当前下注：**";
        if (with_bg_color && !win_info && chip_info.last_bet_chips_ != chip_info.bet_chips_) {
            s += std::to_string(chip_info.last_bet_chips_) + " → ";
        }
        const char* bg_color = nullptr;
        if (chip_info.is_fold_) {
            s += HTML_COLOR_FONT_HEADER(grey);
            bg_color = "#EFEFEF";
        } else if ((with_bg_color && chip_info.bet_chips_ == bet_chips_) ||
                (!win_info && chip_info.action_ == PlayerChipInfo::ALLIN)) {
            s += HTML_COLOR_FONT_HEADER(blue);
            bg_color = "#EBEDFB";
        } else if (with_bg_color) {
            s += HTML_COLOR_FONT_HEADER(red);
            bg_color = "#FBEBEB";
        }
        s += std::to_string(chip_info.bet_chips_);
        s += "** ";
        if (!win_info) {
            s += Action2Str(chip_info.action_);
        }
        s += HTML_FONT_TAIL;
        s += "\n\n";
        const auto& hand_info = player_hand_infos_[pid];
        s += "<font size=\"5\">";
#define DECK_INFO_HEADER(color) HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE  "<font size=\"4\"><span style=\"background-color:" #color "; text-align:center;\">" HTML_COLOR_FONT_HEADER(white) HTML_ESCAPE_SPACE
#define DECK_INFO_TAIL HTML_ESCAPE_SPACE HTML_FONT_TAIL "</span></font>" ;
        if (win_info && win_info->is_winner_) {
            s += hand_info.ToHtml() + DECK_INFO_HEADER(#f3b13d) + hand_info.deck_->TypeName() + DECK_INFO_TAIL "\n\n";
        } else if (show_hand) {
            if (chip_info.is_fold_) {
                s += HTML_COLOR_FONT_HEADER(grey) + hand_info.ToString();
            } else if (win_info) { // `win_info` not null means opening cards
                s += hand_info.ToHtml();
                s += DECK_INFO_HEADER(#63c187);
                s += hand_info.deck_->TypeName();
                s += DECK_INFO_TAIL;
            } else if (hand_info.win_possibility_ == 0) {
                s += hand_info.ToHtml();
            } else {
                std::stringstream ss;
                ss << std::setiosflags(std::ios::fixed) << std::setprecision(1) << (hand_info.win_possibility_ * 100);
                s += hand_info.ToHtml();
                s += hand_info.win_possibility_ > 0.5 ? DECK_INFO_HEADER(#f3b13d) : DECK_INFO_HEADER(#63c187);
                s += ss.str();
                s += "%" DECK_INFO_TAIL;
            }
            s += "\n\n";
        } else if (win_info) { // `win_info` not null means opening cards
            s += HTML_COLOR_FONT_HEADER(grey) "不公布手牌" HTML_FONT_TAIL "\n\n";
        }
#undef DECK_INFO_TAIL
#undef DECK_INFO_HEADER
        s += "</font>\n\n";

        s += "</center>\n\n";
        return {s, bg_color};
    }

    void RefreshWinPossibility_()
    {
        std::vector<poker::Hand<k_type>> hands;
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (Main().GetPlayerChipInfo(pid).is_fold_) {
                continue;
            }
            hands.emplace_back();
            for (const auto& card : player_hand_infos_[pid].hand_) {
                hands.back().Add(card);
            }
            for (uint32_t i = 0; i < open_public_cards_num_; ++i) {
                hands.back().Add(public_cards_[i]);
            }
        }
        std::vector<poker::Card<k_type>> possible_hid_cards = unused_cards_;
        for (uint32_t i = open_public_cards_num_; i < k_public_card_num; ++i) {
            possible_hid_cards.emplace_back(public_cards_[i]);
        }
        const auto possibilities = poker::WinPossibility(hands, std::span(possible_hid_cards), k_public_card_num - open_public_cards_num_, true /*ignore_suit*/);
        auto it = possibilities.begin();
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            player_hand_infos_[pid].win_possibility_ = Main().GetPlayerChipInfo(pid).is_fold_ ? 0 : *(it++);
        }
    }

    std::string Html_(const std::array<PlayerWinInfo, k_max_player>* const player_win_infos, const bool show_hand, const bool with_bg_color)
    {
        static const std::string k_unknown_poker = HTML_COLOR_FONT_HEADER(grey) "�?" HTML_FONT_TAIL;

        std::string s = "## " + this->Name() + "：" + StateName_() + " 阶段\n\n";

        // show header -- public cards
        s += "<center>";
        s += "<font size=\"4\">\n\n**公共牌：";
        const auto show_card_num = player_win_infos ? k_public_card_num : open_public_cards_num_;
        for (uint32_t i = 0; i < show_card_num; ++i) {
            s += public_cards_[i].ToHtml() + HTML_ESCAPE_SPACE;
        }
        for (uint32_t i = show_card_num; i < k_public_card_num; ++i) {
            s += k_unknown_poker + HTML_ESCAPE_SPACE;
        }
        s += "**\n\n</font>";

        // show header -- cur total bet chips
        const auto& chip_infos = Main().GetPlayerChipInfos();
        s += "<font size=\"5\">\n\n当前底池：" + std::to_string(std::accumulate(chip_infos.begin(), chip_infos.end(), 0ull,
                [](int32_t total, const auto& info) { return total + info.bet_chips_; }));
        s += "\n\n</font>";

        // show header -- cur base/bet/raise chips
        s += "**底注：" + std::to_string(base_chips_);
        s += HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE "最高下注：" HTML_COLOR_FONT_HEADER(blue);
        s += std::to_string(bet_chips_) + HTML_FONT_TAIL;
        s += HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE "加注下限：";
        s += std::to_string(raise_chips_);
        s += "**</center>\n\n";

        // show player infos
        static constexpr const uint32_t k_table_column = 2;
        html::Table player_table(0, k_table_column);
        player_table.SetTableStyle(" align=\"center\" cellpadding=\"20\" cellspacing=\"0\" width=\"650\" style=\"table-layout:fixed\"");
        uint32_t i = 0;
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (Main().GetPlayerChipInfo(pid).bet_chips_ == 0) {
                continue;
            }
            if (i % k_table_column == 0) {
                player_table.AppendRow();
            }
            auto [content, bg_color] = PlayerHtml_(pid, player_win_infos ? &player_win_infos->at(pid) : nullptr, show_hand, with_bg_color);
            player_table.GetLastRow(i % k_table_column).SetContent(std::move(content));
            if (bg_color) {
                player_table.GetLastRow(i % k_table_column).SetColor(bg_color);
            }
            i++;
        }
        s += player_table.ToString();

        s += "\n\n已淘汰玩家：";
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if ((Main().GetPlayerChipInfo(pid).bet_chips_ == 0)) {
                s += this->Global().PlayerAvatar(pid, 30);
                s += HTML_ESCAPE_SPACE;
            }
        }

        return s;
    }

    static std::string BetResultHtml_(const std::vector<CallBetPoolResult>& bet_ret)
    {
        std::string s;
        const auto print_ids = [&s](const std::set<uint64_t>& ids)
            {
                for (const auto id : ids) {
                    s += std::to_string(id) + " ";
                }
            };
        for (const auto& ret : bet_ret) {
            const std::string avg_coins = std::to_string(ret.total_coins_ / ret.participant_ids_.size());
            if (ret.participant_ids_.size() == ret.winner_ids_.size()) {
                s += "- 玩家 " HTML_COLOR_FONT_HEADER(blue);
                print_ids(ret.participant_ids_);
                s += HTML_FONT_TAIL " 取回多余的下注各 " HTML_COLOR_FONT_HEADER(blue) + avg_coins + HTML_FONT_TAIL " 枚";
            } else {
                s += "- 玩家 " HTML_COLOR_FONT_HEADER(blue);
                print_ids(ret.participant_ids_);
                s += HTML_FONT_TAIL " 各下注 " HTML_COLOR_FONT_HEADER(blue) + avg_coins + HTML_FONT_TAIL " 枚形成的边池，由手牌最大的玩家 **" + HTML_COLOR_FONT_HEADER(green);
                print_ids(ret.winner_ids_);
                s += HTML_FONT_TAIL "** 平分筹码，平均每名玩家可分得 **" HTML_COLOR_FONT_HEADER(green);
                s += std::to_string(ret.total_coins_ / ret.winner_ids_.size()) + HTML_FONT_TAIL "** 枚\n";
            }
        }
        return s;
    }

    bool TryOpenCard_()
    {
        const uint32_t fold_player_num = std::ranges::count_if(Main().GetPlayerChipInfos(),
                [&](const PlayerChipInfo& chip_info) { return chip_info.is_fold_; });
        const uint32_t allin_player_num = std::ranges::count_if(Main().GetPlayerChipInfos(),
                [&](const PlayerChipInfo& chip_info) { return !chip_info.is_fold_ && chip_info.remain_chips_ == 0; });
        // player raise highest cannot fold, so it is impossible that all players fold
        assert(fold_player_num != Global().PlayerNum());

        if (allin_player_num + fold_player_num < Global().PlayerNum() - 1 && open_public_cards_num_ < k_public_card_num) {
            return false;
        }

        // record remain chips for each player
        int32_t lost_chips = 0;
        std::array<PlayerWinInfo, k_max_player> player_win_infos;
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            player_win_infos[pid].remain_chips_before_open_ = Main().GetPlayerChipInfo(pid).remain_chips_;
        }

        auto sender = Global().Boardcast();
        std::string bet_result_html;
        if (fold_player_num == Global().PlayerNum() - 1) { // do not open card
            sender << "仅剩一人未弃牌，本局结束\n";
            for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
                auto& info = Main().GetPlayerChipInfo(pid);
                if (!info.is_fold_) {
                    const auto& player_chip_infos = Main().GetPlayerChipInfos();
                    info.remain_chips_ += std::accumulate(player_chip_infos.begin(), player_chip_infos.end(), 0,
                            [](const int32_t total, const PlayerChipInfo& chip_info) { return total + chip_info.bet_chips_; });
                }
            }
        } else {
            sender << "筹码数达成共识，现在开牌：\n";
            // update remain chips for each other
            const auto bet_rets = CallBetPool(std::views::iota(0ull, static_cast<uint64_t>(Global().PlayerNum())) |
                        std::views::transform([&](const uint64_t pid)
                        {
                            return CallBetPoolInfo{
                                .id_ = pid,
                                .coins_ = Main().GetPlayerChipInfo(pid).bet_chips_,
                                .obj_ = Main().GetPlayerChipInfo(pid).is_fold_ ? std::nullopt
                                    : player_hand_infos_[pid].deck_,
                            };
                        }),
                        [](const auto& _1, const auto& _2) { return _1.CompareIgnoreSuit(_2) < 0; } // ignore suit
                    );
            for (const auto& ret : bet_rets) {
                for (const PlayerID pid : ret.winner_ids_) {
                    auto& info = Main().GetPlayerChipInfo(pid);
                    info.remain_chips_ += ret.total_coins_ / ret.winner_ids_.size();
                    lost_chips += ret.total_coins_ % ret.winner_ids_.size();
                    if (ret.participant_ids_.size() > ret.winner_ids_.size() && fold_player_num < Global().PlayerNum() - 1) {
                        // the player has won chips from other players, and not all other players are fold, so we need open card
                        player_win_infos[pid].is_winner_ = true;
                    }
                }
            }
            bet_result_html = "\n\n" + BetResultHtml_(bet_rets);
        }

        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            auto& info = Main().GetPlayerChipInfo(pid);
            if (info.remain_chips_ == 0 && info.bet_chips_ > 0) { // the remain chips become 0 in this round
                sender << "\n- " << At(pid) << "筹码归零，被淘汰";
                Global().Eliminate(pid);
            }
            const auto& deck = player_hand_infos_[pid].deck_;
            if (player_win_infos[pid].is_winner_) {
                assert(deck.has_value());
                sender << "\n- " << At(pid) << "凭借「" << *deck << "」收获 "
                    << (info.remain_chips_ - player_win_infos[pid].remain_chips_before_open_) << " 枚筹码";
            }
        }
        if (lost_chips > 0) {
            sender << "\n\n有 " << lost_chips << " 枚筹码因无法均分而被丢弃";
        }

        // save image
        sender << Markdown(Html_(&player_win_infos, false, false) + bet_result_html, k_markdown_width); // `Html_` must be called before `Reset`
        Global().SaveMarkdown(Html_(&player_win_infos, true, false) + bet_result_html, k_markdown_width);

        // reset chip info for each player
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            Reset(Main().GetPlayerChipInfo(pid));
        }

        return true;
    }

    void NextSubStage_(const int32_t max_raise_chips, SubStageFsmSetter& setter)
    {
        bet_chips_ += max_raise_chips;
        raise_chips_ = std::max(raise_chips_, max_raise_chips);
        if (open_public_cards_num_ > 0) {
            RefreshWinPossibility_();
        }
        html_ = Html_(nullptr, false, true);
        Global().Boardcast() << Markdown(html_, k_markdown_width);
        Global().SaveMarkdown(Html_(nullptr, true, true), k_markdown_width);
        if (max_raise_chips > 0 && !std::ranges::all_of(Main().GetPlayerChipInfos(),
                    [&](const PlayerChipInfo& chip_info) { return AchieveMaxBet(chip_info, bet_chips_); })) {
            // players have not reach a consensus, continue raising
            setter.Emplace<RaiseStage>(this->Main(), StateName_(), bet_chips_, raise_chips_);
            return;
        }

        if (TryOpenCard_()) {
            return;
        }

        if (open_public_cards_num_ == 0) {
            open_public_cards_num_ = 3;
        } else {
            ++open_public_cards_num_;
        }
        auto sender = Global().Boardcast();
        sender << "新一轮下注开始，当前公开的公共牌：\n";
        for (uint32_t i = 0; i < open_public_cards_num_; ++i) {
            sender << public_cards_[i].ToString() << " ";
        }

        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            NoAction(Main().GetPlayerChipInfo(pid));
        }
        RefreshWinPossibility_();
        html_ = Html_(nullptr, false, false);
        sender << Markdown(html_, k_markdown_width);
        Global().SaveMarkdown(Html_(nullptr, true, false), k_markdown_width);

        setter.Emplace<BetStage>(this->Main(), StateName_(), base_chips_);
    }

    static constexpr const char* k_state_names_[6] = {
        [0] = "preflop",
        [1] = "",
        [2] = "",
        [3] = "flop",
        [4] = "turn",
        [5] = "river",
    };
    const int32_t base_chips_;
    int32_t bet_chips_;
    int32_t raise_chips_;
    uint8_t open_public_cards_num_;
    std::array<poker::Card<k_type>, k_public_card_num> public_cards_;
    std::vector<PlayerHandInfo> player_hand_infos_;
    std::string html_;
    std::vector<poker::Card<k_type>> unused_cards_;
};

void MainStage::FirstStageFsm(SubStageFsmSetter setter)
{
    if (GAME_OPTION(卡牌) == poker::CardType::BOKAA) {
        setter.Emplace<RoundStage<poker::CardType::BOKAA>>(*this, round_);
    } else if (GAME_OPTION(卡牌) == poker::CardType::POKER) {
        setter.Emplace<RoundStage<poker::CardType::POKER>>(*this, round_);
    } else {
        assert(false);
    }
}

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot

