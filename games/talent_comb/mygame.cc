// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).
//
// This file was generated with the assistance of Claude Code (claude.ai/code).

#include <array>
#include <map>
#include <functional>
#include <memory>
#include <vector>
#include <random>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <set>
#include <queue>

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"
#include "utility/random.h"

#include "talent_comb.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;
const GameProperties k_properties {
    .name_ = "天赋云巢",
    .developer_ = "铁蛋",
    .description_ = "云顶之巢 + 天赋系统，在战斗中获得天赋强化，成为最后的胜者",
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 8; }
uint32_t Multiple(const CustomOptions& options) { return 2; }
const MutableGenericOptions k_default_generic_options;

static std::string TalentRuleText(const int talent_id)
{
    if (talent_id < 0 || talent_id >= static_cast<int>(Talent::COUNT)) {
        return "未知天赋";
    }
    const auto talent = static_cast<Talent>(talent_id);
    const auto& info = GetTalentInfo(talent);
    return "【" + std::string(info.name) + "】\n等级：" + std::string(info.grade) + "\n" + info.description;
}

const std::vector<RuleCommand> k_rule_commands = {
    RuleCommand("查看指定天赋效果",
            [](const int talent_id) -> const char* const
            {
                static std::string rule_text;
                rule_text = TalentRuleText(talent_id);
                return rule_text.c_str();
            },
            AlterChecker<int>(MakeTalentOptionMap())),
};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() < 2) {
        reply() << "天赋云巢至少需要 2 人参加游戏";
        return false;
    }
    return true;
}

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options)
            {
                generic_options.bench_computers_to_player_num_ = 4;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
    InitOptionsCommand("选择特殊事件开始游戏",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options,
                const int32_t& event)
            {
                GET_OPTION_VALUE(game_options, 事件) = event;
                return NewGameMode::MULTIPLE_USERS;
            },
            AlterChecker<int32_t>({{"无", 1}, {"大的要来了", 2}, {"两极分化", 3}, {"大的没了", 4},
                                   {"天降恩泽", 5}, {"调色盘", 6}, {"有1吗", 7}, {"小透不算挂", 8}, {"？？？", 9}})),
};

// ==================== Card Pool Generation ====================

static const std::array<std::vector<int32_t>, k_direct_max> k_points{
    std::vector<int32_t>{3, 4, 8},
    std::vector<int32_t>{1, 5, 9},
    std::vector<int32_t>{2, 6, 7}
};

static std::vector<AreaCard> GenerateBaseCards(SpecialEvent event)
{
    std::vector<int32_t> p0 = {3, 4, 8};
    std::vector<int32_t> p1 = {1, 5, 9};
    std::vector<int32_t> p2 = {2, 6, 7};

    // Remove digits based on special events
    if (event == SpecialEvent::大的要来了) {
        p1.erase(std::remove(p1.begin(), p1.end(), 1), p1.end());
    } else if (event == SpecialEvent::两极分化) {
        p1.erase(std::remove(p1.begin(), p1.end(), 5), p1.end());
    } else if (event == SpecialEvent::大的没了) {
        p1.erase(std::remove(p1.begin(), p1.end(), 9), p1.end());
    }

    std::vector<AreaCard> cards;
    for (int32_t a : p0) {
        for (int32_t b : p1) {
            for (int32_t c : p2) {
                cards.emplace_back(a, b, c);
                cards.emplace_back(a, b, c);
            }
        }
    }
    return cards;
}

// ==================== Forward Declarations ====================

class RoundStage;
class SelectStage;
class ExtraCardStage;
class TalentStage;

// ==================== Round Phase ====================

enum class RoundPhase {
    放置,
    战前额外,
    天赋选择,
    对战,
    战后额外,
};

// ==================== MainStage ====================

class MainStage : public MainGameStage<RoundStage, SelectStage, ExtraCardStage, TalentStage>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility),
            MakeStageCommand(*this, "查看蜂巢初始状态", &MainStage::Info_, VoidChecker("赛况")),
            MakeStageCommand(*this, "查看指定天赋效果", &MainStage::TalentRule_, AlterChecker<int>(MakeTalentOptionMap())))
        , round_(0)
        , alive_(Global().PlayerNum())
        , player_out_(Global().PlayerNum(), 0)
        , player_leave_(Global().PlayerNum(), false)
        , phase_(RoundPhase::放置)
        , fought_round_(0)
        , last_mirror_(-1)
    {
        // Seed
        seed_str_ = GAME_OPTION(种子);
        if (seed_str_.empty()) {
            std::random_device rd;
            std::uniform_int_distribution<unsigned long long> dis;
            seed_str_ = std::to_string(dis(rd));
        }
        g_ = MakeRng(seed_str_);

        // Special event
        int event_opt = GAME_OPTION(事件);
        if (event_opt == 0) {
            // Random: 5/20 NONE, 2/20 each of 7 events, 1/20 MYSTERY
            std::vector<SpecialEvent> pool = {
                SpecialEvent::无, SpecialEvent::无, SpecialEvent::无,
                SpecialEvent::无, SpecialEvent::无,
                SpecialEvent::大的要来了, SpecialEvent::大的要来了,
                SpecialEvent::两极分化, SpecialEvent::两极分化,
                SpecialEvent::大的没了, SpecialEvent::大的没了,
                SpecialEvent::天降恩泽, SpecialEvent::天降恩泽,
                SpecialEvent::调色盘, SpecialEvent::调色盘,
                SpecialEvent::有1吗, SpecialEvent::有1吗,
                SpecialEvent::小透不算挂, SpecialEvent::小透不算挂,
                SpecialEvent::愚人节,
            };
            special_event_ = pool[RandInt(g_, 0, static_cast<uint32_t>(pool.size() - 1))];
        } else {
            special_event_ = static_cast<SpecialEvent>(event_opt - 1);
        }

        // Create players
        const int32_t init_hp = static_cast<int32_t>(GAME_OPTION(血量));
        for (uint64_t i = 0; i < Global().PlayerNum(); ++i) {
            players_.emplace_back(Global().ResourceDir(), init_hp);
            players_.back().InitTalentPools();
            players_.back().RemoveEventIncompatibleTalents(special_event_);
        }

        // Generate card pools
        // Pool1 (cards_): for normal rounds (round 2+), 54 base cards + 2 wild
        cards_ = GenerateBaseCards(special_event_);
        cards_.emplace_back(); // wild
        cards_.emplace_back(); // wild
        if (HasColorful(special_event_)) {
            cards_.emplace_back(); // +3 more wild for colorful
            cards_.emplace_back();
            cards_.emplace_back();
        }
        SeededShuffle(cards_.begin(), cards_.end(), g_);

        // Pool2 (cards2_): for round 1 initial + selection rounds, 54 base cards, no wild
        cards2_ = GenerateBaseCards(special_event_);
        if (HasColorful(special_event_)) {
            cards2_.emplace_back(); // +3 wild for colorful
            cards2_.emplace_back();
            cards2_.emplace_back();
        }
        SeededShuffle(cards2_.begin(), cards2_.end(), g_);

        it_ = cards_.begin();
        it2_ = cards2_.begin();

        // Damage rate (same formula as opencomb)
        dRate_ = std::pow(M_E, Global().PlayerNum() / 6.0) / M_E;
        iRate_ = dRate_;
    }

    virtual void FirstStageFsm(SubStageFsmSetter setter) override;
    virtual void NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;
    virtual void NextStageFsm(SelectStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;
    virtual void NextStageFsm(ExtraCardStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;
    virtual void NextStageFsm(TalentStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;

    int64_t PlayerScore(const PlayerID pid) const override
    {
        return players_[pid].TotalScore();
    }

    int64_t PlayerBattleScore(const PlayerID pid) const
    {
        return players_[pid].TotalScore() + players_[pid].TempBattleScore();
    }

    // 盘面天赋原分（不含终局血量奖惩），供单元测试作确定性断言使用。
    int64_t PlayerRawTotalScore(const PlayerID pid) const
    {
        return players_[pid].TotalScore();
    }

    int32_t PlayerHP(const PlayerID pid) const
    {
        return players_[pid].hp_;
    }

    // ===== UI / Display (implemented in talent_impl.h) =====
    std::string GetName(const std::string& x);
    std::pair<std::string, std::string> PlayerScoreDetail(const PlayerID pid) const;
    void SetPlayerBoard(html::Table& table, const int pos, const PlayerID pid, const bool isEliminated);
    std::string CombHtml(const std::string& str);

    // Check if game should end
    bool CheckGameOver() const
    {
        return alive_ <= 1;
    }

    // ===== Battle System (implemented in talent_impl.h) =====
    bool DoBattle_();
    void ApplyDamage_(PlayerID pid, int32_t damage);
    void ProcessBattle_(PlayerID pid1, PlayerID pid2, bool mirror, std::string& result);
    int32_t ApplyAttackTalents_(PlayerID attacker, PlayerID defender, int32_t damage);
    int32_t ApplyDefenseTalents_(PlayerID defender, int32_t damage);
    void ApplyDefeatTalents_(PlayerID loser, bool mirror, int32_t& damage, std::string& result);
    void ApplyVictoryTalents_(PlayerID winner, bool mirror, std::string& result);
    void DoEliminationAfterBattle_();
    void DoPoison_();

    // Clear 绝地反击 extra for players who carried it into this real battle.
    // Newly triggered extra must survive until the next real battle ends; skipped
    // battle phases such as selection rounds do not consume it.
    void ClearCounterattackAfterRound_()
    {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (player_out_[pid] != 0) continue;
            auto& p = players_[pid];
            if (p.Counterattack().triggered && p.Counterattack().trigger_round < round_) {
                p.Counterattack().triggered = false;
                p.Counterattack().extra_score = 0;
                p.Counterattack().trigger_round = 0;
            }
        }
    }

    void CollectPreBattleExtras_();
    void CollectPostBattleExtras_();

    // ===== Talent Effects (implemented in talent_impl.h) =====
    // idx: the position (1-19) the card was placed at; 0 means "no galaxy-flow trigger applicable".
    std::pair<std::string, int32_t> OnCardPlaced_(PlayerID pid, uint32_t idx, const ScoreResult& result,
                                                  std::optional<Talent> source_talent = std::nullopt,
                                                  const AreaCard* previous_card = nullptr,
                                                  TalentCardPlacementSource placement_source = TalentCardPlacementSource::Selection);
    std::pair<AreaCard, std::string> TransformCardForPlacement_(PlayerID pid, const AreaCard& card, bool is_normal_round = false);
    std::string HandleDiscard_(PlayerID pid, const AreaCard& card, std::optional<Talent> source_talent = std::nullopt);
    bool HandleThreeYearStore_(PlayerID pid, const AreaCard& card);

    // ===== Card Pool Management (implemented in talent_impl.h) =====
    AreaCard DrawFromPool2_();
    std::vector<AreaCard> DrawBalancedCards_(uint32_t count);
    AreaCard DrawFromPool1_();

    // Check if any player needs talent choice
    bool AnyPendingTalentChoice_() const
    {
        for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
            if (player_out_[pid] != 0) continue;
            // Player already has a filled talent pool (ready to choose)
            if (!players_[pid].talent_pool_.empty()) return true;
            // Check if player qualifies for new talent (use selection count, not talent count)
            uint32_t line_count = players_[pid].comb_->LineCount();
            if (players_[pid].talent_selection_count_ < line_count / 3) {
                return true;
            }
        }
        return false;
    }

    // Generate talent pools for qualifying players
    void PrepareNewTalentChoices_()
    {
        for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
            if (player_out_[pid] != 0) continue;
            auto& player = players_[pid];
            if (!player.talent_pool_.empty()) continue;
            uint32_t line_count = player.comb_->LineCount();
            if (player.talent_selection_count_ < line_count / 3) {
                player.GenerateTalentPool(g_);
            }
        }
    }

    // Check if any player has extra cards to place or choices to make
    bool AnyPendingExtraCards_() const
    {
        for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
            if (player_out_[pid] != 0) continue;
            if (!players_[pid].extra_card_queue_.empty()) return true;
        }
        return false;
    }

    // Advance the round phase state machine
    void AdvancePhase_(SubStageFsmSetter& setter);

    // Start a new round (card placement stage)
    void StartNewRound_(SubStageFsmSetter& setter);

    static bool IsSelectionRound(int32_t round)
    {
        // Rounds 8, 15, 22, 29... (first selection at round 8, then every 7)
        return round >= 8 && (round - 8) % 7 == 0;
    }

    int32_t round_;
    int32_t alive_;
    std::vector<Player> players_;
    std::vector<int32_t> player_out_;
    std::vector<bool> player_leave_;
    SpecialEvent special_event_;
    RoundPhase phase_;

    std::vector<AreaCard> cards_;
    decltype(cards_)::iterator it_;
    std::vector<AreaCard> cards2_;
    decltype(cards2_)::iterator it2_;

    std::mt19937 g_;
    std::string seed_str_;

    // Battle state
    std::vector<std::unordered_map<int32_t, int32_t>> fought_list_;
    std::unordered_map<int32_t, int32_t> current_fight_map_;  // attacker→defender for current round
    double dRate_, iRate_;
    int32_t last_mirror_;
    int32_t fought_round_;

    // For foresee event
    std::optional<AreaCard> next_card_;

    // 紧急救援: flag for emergency-triggered SelectStage
    bool is_emergency_select_ = false;

    // Game over
    void DoGameOver_();

  private:
    CompReqErrCode Info_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << Markdown(CombHtml("## 第 " + std::to_string(round_) + " 回合"));
        return StageErrCode::OK;
    }

    CompReqErrCode TalentRule_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int talent_id)
    {
        reply() << TalentRuleText(talent_id);
        return StageErrCode::OK;
    }
};

// Include talent effects, UI rendering, and battle settlement implementations
#include "talent_impl.h"

// ==================== RoundStage ====================

class RoundStage : public SubGameStage<>
{
  public:
    // Normal round: all players get the same card
    RoundStage(MainStage& main_stage, const uint64_t round, const AreaCard& card)
            : StageFsm(main_stage, "第" + std::to_string(round) + "回合",
                MakeStageCommand(*this, "放置砖块到指定位置（0 为弃牌）", &RoundStage::Set_, ArithChecker<uint32_t>(0, 19, "位置")))
            , round_(round)
            , is_initial_(round == 1)
            , comb_html_(main_stage.CombHtml("## 第 " + std::to_string(round) + " 回合"))
    {
        if (is_initial_) {
            // Round 1: each player gets a balanced unique card
            if (HasWindfall(Main().special_event_)) {
                for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
                    if (Main().player_out_[pid] == 0) {
                        initial_cards_[pid] = AreaCard(); // wild card for windfall
                    }
                }
            } else {
                // Count alive players
                uint32_t alive_count = 0;
                std::vector<PlayerID> alive_pids;
                for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
                    if (Main().player_out_[pid] == 0) {
                        alive_count++;
                        alive_pids.push_back(pid);
                    }
                }
                auto balanced = Main().DrawBalancedCards_(alive_count);
                for (size_t i = 0; i < alive_pids.size(); ++i) {
                    initial_cards_[alive_pids[i]] = balanced[i];
                }
            }
        } else {
            shared_card_ = card;
        }
    }

    virtual void OnStageBegin() override
    {
        if (is_initial_) {
            Global().Boardcast() << "初始回合，每个人随机获得一张砖块，请公屏或私信裁判设置数字：";
            // Already mark eliminated players as ready
            for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
                if (Main().player_out_[pid] != 0) {
                    Global().SetReady(pid);
                }
            }
        } else {
            // Handle 三年之期 storage
            for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
                if (Main().player_out_[pid] != 0) {
                    Global().SetReady(pid);
                    continue;
                }
                if (Main().HandleThreeYearStore_(pid, shared_card_)) {
                    Global().Boardcast() << At(pid) << " 触发天赋「三年之期」，本轮砖块已存储（" << Main().players_[pid].ThreeYear().cards.size() << "/3）";
                    Global().SetReady(pid);
                }
            }

            std::string card_info = "本回合砖块为 " + shared_card_.CardName() + "，请公屏或私信裁判设置数字：";
            if (HasForesee(Main().special_event_) && Main().next_card_.has_value()) {
                card_info += "\n（下一轮砖块为：" + Main().next_card_->CardName() + "）";
            }
            Global().Boardcast() << card_info;
        }
        SendInfo(Global().BoardcastMsgSender());
        Global().StartTimer(GAME_OPTION(局时));
    }

  private:
    AreaCard GetCardForPlayer_(PlayerID pid) const
    {
        if (is_initial_) {
            auto it = initial_cards_.find(pid);
            if (it != initial_cards_.end()) return it->second;
            return AreaCard(); // shouldn't happen
        }
        return shared_card_;
    }

    void HandleUnreadyPlayers_()
    {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (Global().IsReady(pid) || Main().player_out_[pid] != 0) continue;

            auto& player = Main().players_[pid];
            const AreaCard card = GetCardForPlayer_(pid);
            auto sender = Global().Boardcast();

            if (player.comb_->HasEmptyPosition()) {
                auto [actual_card, transform_notify] = Main().TransformCardForPlacement_(pid, card, true);
                const auto [idx, result] = player.comb_->SeqFill(actual_card);
                auto [place_notify, actual_delta] = Main().OnCardPlaced_(
                    pid, idx, result, std::nullopt, nullptr,
                    is_initial_ ? TalentCardPlacementSource::InitialDeal : TalentCardPlacementSource::RegularRound);
                sender << At(pid) << " 超时，自动填入位置 " << idx;
                if (actual_delta > 0) {
                    sender << "，收获 " << actual_delta << " 点积分";
                }
            } else {
                // Board full, auto discard
                Main().HandleDiscard_(pid, card);
                sender << At(pid) << " 超时，盘面已满自动弃牌";
            }
        }
        Global().HookUnreadyPlayers();
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Main().player_leave_[pid] = true;
        return StageErrCode::CONTINUE;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        HandleUnreadyPlayers_();
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        HandleUnreadyPlayers_();
        return StageErrCode::CHECKOUT;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) return StageErrCode::OK;
        auto& player = Main().players_[pid];
        const AreaCard card = GetCardForPlayer_(pid);
        if (player.comb_->HasEmptyPosition()) {
            auto [actual_card, transform_notify] = Main().TransformCardForPlacement_(pid, card, true);
            const auto [idx, result] = player.comb_->SeqFill(actual_card);
            Main().OnCardPlaced_(pid, idx, result, std::nullopt, nullptr,
                                 is_initial_ ? TalentCardPlacementSource::InitialDeal : TalentCardPlacementSource::RegularRound);
        } else {
            Main().HandleDiscard_(pid, card);
        }
        return StageErrCode::READY;
    }

    AtomReqErrCode Set_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const uint32_t idx)
    {
        if (Global().IsReady(pid)) {
            reply() << "您已经设置过，无法重复设置";
            return StageErrCode::FAILED;
        }
        if (Main().player_out_[pid] != 0) {
            reply() << "您已被淘汰";
            return StageErrCode::FAILED;
        }

        auto& player = Main().players_[pid];
        const AreaCard card = GetCardForPlayer_(pid);

        if (idx == 0) {
            // Discard
            std::string notify = Main().HandleDiscard_(pid, card);
            reply() << "弃牌成功" << notify;
            return StageErrCode::READY;
        }

        if (auto block_msg = player.ProtectedPositionMessage(idx)) {
            reply() << "[错误] " << *block_msg;
            return StageErrCode::FAILED;
        }

        // Transform card based on talents (三相之力, 以退为进, 完美块)
        const auto previous_card = player.comb_->GetCard(idx);
        auto [actual_card, transform_notify] = Main().TransformCardForPlacement_(pid, card, true);
        const auto result = player.comb_->Fill(idx, actual_card);
        auto [notify, actual_delta] = Main().OnCardPlaced_(
            pid, idx, result, std::nullopt, previous_card ? &*previous_card : nullptr,
            is_initial_ ? TalentCardPlacementSource::InitialDeal : TalentCardPlacementSource::RegularRound);

        auto sender = reply();
        sender << "设置位置 " << idx << " 成功";
        if (actual_delta > 0) {
            sender << "，收获 " << actual_delta << " 点积分";
        } else if (actual_delta < 0) {
            sender << "，损失 " << (-actual_delta) << " 点积分";
        }
        sender << transform_notify << notify;
        return StageErrCode::READY;
    }

    void SendInfo(MsgSenderBase& sender)
    {
        sender() << Markdown{comb_html_};
        if (is_initial_) {
            // Show initial cards image with player assignments
            sender() << Markdown(InitialCardHtml_(), 300);
        } else if (HasForesee(Main().special_event_) && Main().next_card_.has_value()) {
            // 小透不算挂: show current and next card side by side with arrow
            sender() << Markdown(ForeseeCardHtml_(), 64);
        } else {
            const std::string style = "<style>body{margin:0;}</style>" + GetStyle(Global().ResourceDir());
            sender() << Markdown(style + shared_card_.ToHtml(Global().ResourceDir()), 64);
        }
    }

    std::string ForeseeCardHtml_()
    {
        const std::string img_path = Global().ResourceDir();
        const std::string style = "<style>body{margin:0;}</style>" + GetStyle(img_path);
        const std::string arrow_svg =
            "<svg width=\"32\" height=\"64\" viewBox=\"0 0 32 64\" style=\"vertical-align:middle;\">"
            "<polygon points=\"2,24 18,24 18,16 30,32 18,48 18,40 2,40\" fill=\"#666\"/>"
            "</svg>";
        html::Table card_table(1, 3);
        card_table.SetTableStyle("align=\"center\" cellpadding=\"0\" cellspacing=\"0\" ");
        card_table.Get(0, 0).SetContent(shared_card_.ToHtml(img_path));
        card_table.Get(0, 1).SetContent(arrow_svg);
        card_table.Get(0, 2).SetContent(Main().next_card_->ToHtml(img_path));
        return style + card_table.ToString();
    }

    std::string InitialCardHtml_()
    {
        const std::string img_path = Global().ResourceDir();
        const std::string style = "<style>body{margin:0;}</style>" + GetStyle(img_path);
        html::Table card_table(2, initial_cards_.size());
        card_table.SetTableStyle("align=\"center\" cellpadding=\"3\" cellspacing=\"0\" ");
        size_t col = 0;
        for (const auto& [pid, card] : initial_cards_) {
            card_table.Get(0, col).SetContent(Global().PlayerAvatar(pid, 30));
            card_table.Get(1, col).SetContent(card.ToHtml(img_path));
            col++;
        }
        return style + card_table.ToString();
    }

    const uint64_t round_;
    const bool is_initial_;
    AreaCard shared_card_;
    std::map<PlayerID, AreaCard> initial_cards_;
    const std::string comb_html_;
};

// ==================== SelectStage ====================

class SelectStage : public SubGameStage<>
{
  public:
    SelectStage(MainStage& main_stage, const uint64_t round)
            : StageFsm(main_stage, "公共配牌阶段",
                MakeStageCommand(*this, "选择并放置砖块（卡牌ID 位置，位置 0 为弃牌）", &SelectStage::Select_, ArithChecker<uint32_t>(1, 20, "选卡"), ArithChecker<uint32_t>(0, 19, "位置")))
            , round_(round)
            , comb_html_(main_stage.CombHtml("## 第 " + std::to_string(round) + " 回合[公共配牌阶段]"))
    {
        // Prepare selection order: alive players sorted by HP (low first), then score (low first)
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (Main().player_out_[pid] == 0) {
                current_players_.push_back(pid);
            }
        }
        SeededShuffle(current_players_.begin(), current_players_.end(), Main().g_);
        std::sort(current_players_.begin(), current_players_.end(), [this](const PlayerID& p1, const PlayerID& p2) {
            auto& player1 = Main().players_[p1];
            auto& player2 = Main().players_[p2];
            // 紧急救援 (unused): highest priority
            bool er1 = player1.HasTalent(Talent::紧急救援) && !player1.EmergencyRescue().used;
            bool er2 = player2.HasTalent(Talent::紧急救援) && !player2.EmergencyRescue().used;
            if (er1 != er2) return er1;
            // 占得先机 priority next
            if (player1.Seize().selection_priority != player2.Seize().selection_priority) {
                return player1.Seize().selection_priority < player2.Seize().selection_priority;
            }
            if (player1.hp_ != player2.hp_) {
                return player1.hp_ < player2.hp_;
            }
            return Main().PlayerScore(p1) < Main().PlayerScore(p2);
        });

        // Draw n+1 cards from pool2
        for (size_t i = 0; i < current_players_.size() + 1; ++i) {
            tmp_cards_.push_back(Main().DrawFromPool2_());
        }
    }

    virtual void OnStageBegin() override
    {
        // Set all players ready except the first one
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (pid != current_players_[0]) {
                Global().SetReady(pid);
            }
        }
        SendInfo(Global().BoardcastMsgSender());
        Global().Boardcast() << "请 " << At(current_players_[0]) << " 选择";
        Global().StartTimer(GAME_OPTION(局时));
    }

  private:
    void HandleUnreadyPlayer_()
    {
        if (current_players_.empty()) return;
        PlayerID pid = current_players_[0];
        if (Global().IsReady(pid) && !Main().player_leave_[pid]) return;

        auto& player = Main().players_[pid];
        const AreaCard card = tmp_cards_[0];
        Selected_(1);

        auto sender = Global().Boardcast();
        if (player.comb_->HasEmptyPosition()) {
            auto [actual_card, transform_notify] = Main().TransformCardForPlacement_(pid, card);
            const auto [idx, result] = player.comb_->SeqFill(actual_card);
            auto [place_notify, actual_delta] = Main().OnCardPlaced_(pid, idx, result);
            sender << At(pid) << " 超时，自动选择 1 号砖块填入位置 " << idx;
        } else {
            Main().HandleDiscard_(pid, card);
            sender << At(pid) << " 超时，自动选择 1 号砖块，盘面已满自动弃牌";
        }
        Global().SetReady(pid);
        Global().HookUnreadyPlayers();
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Main().player_leave_[pid] = true;
        return StageErrCode::CONTINUE;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        HandleUnreadyPlayer_();
        return StageOver_();
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        HandleUnreadyPlayer_();
        return StageOver_();
    }

    CheckoutErrCode StageOver_()
    {
        if (current_players_.empty()) {
            // Handle 尾货处理 for the last player
            return StageErrCode::CHECKOUT;
        }
        comb_html_ = Main().CombHtml("## 第 " + std::to_string(round_) + " 回合[公共配牌阶段]");
        SendInfo(Global().BoardcastMsgSender());
        Global().Boardcast() << "请 " << At(current_players_[0]) << " 选择";
        Global().ClearReady(current_players_[0]);
        Global().StartTimer(GAME_OPTION(局时));
        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) return StageErrCode::OK;
        auto& player = Main().players_[pid];
        const AreaCard card = tmp_cards_[0];
        Selected_(1);
        if (player.comb_->HasEmptyPosition()) {
            auto [actual_card, transform_notify] = Main().TransformCardForPlacement_(pid, card);
            const auto [idx, result] = player.comb_->SeqFill(actual_card);
            Main().OnCardPlaced_(pid, idx, result);
        } else {
            Main().HandleDiscard_(pid, card);
        }
        return StageErrCode::READY;
    }

    AtomReqErrCode Select_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const uint32_t card_id, const uint32_t pos)
    {
        if (Global().IsReady(pid)) {
            reply() << "[错误] 当前并非您的选卡回合";
            return StageErrCode::FAILED;
        }
        if (card_id < 1 || card_id > tmp_cards_.size()) {
            reply() << "[错误] 无效的选牌ID，请输入 1-" << tmp_cards_.size() << " 之间的数字";
            return StageErrCode::FAILED;
        }

        auto& player = Main().players_[pid];
        const AreaCard card = tmp_cards_[card_id - 1];

        if (pos != 0) {
            if (auto block_msg = player.ProtectedPositionMessage(pos)) {
                reply() << "[错误] " << *block_msg;
                return StageErrCode::FAILED;
            }
        }

        // Check if this is the last player and they have 尾货处理
        bool is_last_player = (current_players_.size() == 1);
        if (is_last_player && player.HasTalent(Talent::尾货处理) && tmp_cards_.size() >= 2) {
            // Get the remaining card (not the one selected) as bonus
            for (size_t i = 0; i < tmp_cards_.size(); ++i) {
                if (i != card_id - 1) {
                    player.extra_card_queue_.push_back({{tmp_cards_[i]}, "尾货处理"});
                }
            }
        }

        Selected_(card_id);

        if (pos == 0) {
            std::string notify = Main().HandleDiscard_(pid, card);
            reply() << "选择 " << card_id << " 号砖块，弃牌成功" << notify;
            return StageErrCode::READY;
        }

        const auto previous_card = player.comb_->GetCard(pos);
        auto [actual_card, transform_notify] = Main().TransformCardForPlacement_(pid, card);
        const auto result = player.comb_->Fill(pos, actual_card);
        auto [notify, actual_delta] = Main().OnCardPlaced_(pid, pos, result, std::nullopt, previous_card ? &*previous_card : nullptr);
        auto sender = reply();
        sender << "选择 " << card_id << " 号砖块，设置位置 " << pos << " 成功";
        if (actual_delta > 0) {
            sender << "，收获 " << actual_delta << " 点积分";
        } else if (actual_delta < 0) {
            sender << "，损失 " << (-actual_delta) << " 点积分";
        }
        sender << transform_notify << notify;
        return StageErrCode::READY;
    }

    void Selected_(const uint32_t id)
    {
        tmp_cards_.erase(tmp_cards_.begin() + id - 1);
        current_players_.erase(current_players_.begin());
    }

    std::string SelectCardHtml_()
    {
        html::Table avatar_table(1, current_players_.size() + 1);
        avatar_table.SetTableStyle("cellpadding=\"0\" cellspacing=\"5\"");
        avatar_table.Get(0, 0).SetContent("　<b>选卡顺序：</b>");
        for (size_t i = 0; i < current_players_.size(); ++i) {
            std::string avatar = Global().PlayerAvatar(current_players_[i], 40);
            if (Main().is_emergency_select_
                       && Main().players_[current_players_[i]].HasTalent(Talent::紧急救援)
                       && !Main().players_[current_players_[i]].EmergencyRescue().used) {
                // 紧急救援 triggered: rainbow border (gold + thick border as fallback)
                avatar = "<div style=\"position:relative;display:inline-block;width:40px;height:40px;\">"
                         + avatar +
                         "<div style=\"position:absolute;top:0;left:0;width:36px;height:36px;"
                         "border:2px solid #FFD700;border-radius:50%;pointer-events:none;"
                         "box-shadow:0 0 4px #FF6347,0 0 8px #FFD700;\"></div></div>";
            } else if (Main().players_[current_players_[i]].HasTalent(Talent::占得先机)) {
                avatar = "<div style=\"position:relative;display:inline-block;width:40px;height:40px;\">"
                         + avatar +
                         "<div style=\"position:absolute;top:0;left:0;width:36px;height:36px;"
                         "border:2px solid red;border-radius:50%;pointer-events:none;\"></div></div>";
            } else if (i == current_players_.size() - 1
                       && Main().players_[current_players_[i]].HasTalent(Talent::尾货处理)) {
                // Last player with 尾货处理: bright cyan border
                avatar = "<div style=\"position:relative;display:inline-block;width:40px;height:40px;\">"
                         + avatar +
                         "<div style=\"position:absolute;top:0;left:0;width:36px;height:36px;"
                         "border:2px solid #00E5FF;border-radius:50%;pointer-events:none;\"></div></div>";
            }
            avatar_table.Get(0, i + 1).SetContent(avatar);
        }
        html::Table card_table(1, tmp_cards_.size() * 2);
        card_table.SetTableStyle("align=\"center\" cellpadding=\"0\" cellspacing=\"0\" ");
        const std::string img_path = Global().ResourceDir();
        for (size_t i = 0; i < tmp_cards_.size(); ++i) {
            card_table.Get(0, i * 2).SetContent(std::to_string(i + 1) + ".");
            card_table.Get(0, i * 2 + 1).SetContent(tmp_cards_[i].ToHtml(img_path));
        }
        const std::string style = "<style>body{margin:0;}</style>" + GetStyle(Global().ResourceDir());
        return style + avatar_table.ToString() + card_table.ToString();
    }

    void SendInfo(MsgSenderBase& sender)
    {
        sender() << Markdown{comb_html_};
        sender() << Markdown(SelectCardHtml_(), 300);
    }

    const uint64_t round_;
    std::vector<PlayerID> current_players_;
    std::vector<AreaCard> tmp_cards_;
    std::string comb_html_;
};

// ==================== ExtraCardStage ====================
// Universal extra card stage: handles both single-card (direct placement) and multi-card (pick one) entries.

class ExtraCardStage : public SubGameStage<>
{
  public:
    ExtraCardStage(MainStage& main_stage)
            : StageFsm(main_stage, "额外砖块选择阶段",
                MakeStageCommand(*this, "放置额外砖块到指定位置（0 为弃牌）", &ExtraCardStage::Set_, ArithChecker<uint32_t>(0, 19, "位置")),
                MakeStageCommand(*this, "选择砖块并放置（砖块序号 位置，位置 0 为弃牌）", &ExtraCardStage::Choose_,
                    ArithChecker<uint32_t>(1, 10, "砖块序号"), ArithChecker<uint32_t>(0, 19, "位置")))
    {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (Main().player_out_[pid] == 0 && !Main().players_[pid].extra_card_queue_.empty()) {
                pending_players_.push_back(pid);
            }
        }
    }

    virtual void OnStageBegin() override
    {
        if (pending_players_.empty()) return;
        // Show board status
        Global().Boardcast() << Markdown{Main().CombHtml("## 额外砖块选择阶段")};
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            Global().SetReady(pid);
        }
        StartNextPlayer_();
    }

  private:
    // Get the current entry for the front pending player
    Player::ExtraCardEntry& CurrentEntry_(Player& player)
    {
        return player.extra_card_queue_.front();
    }

    bool IsMultiCard_(const Player::ExtraCardEntry& entry) const
    {
        return entry.cards.size() > 1;
    }

    void StartNextPlayer_()
    {
        if (pending_players_.empty()) return;
        PlayerID pid = pending_players_.front();
        auto& player = Main().players_[pid];

        if (player.extra_card_queue_.empty()) {
            pending_players_.erase(pending_players_.begin());
            if (pending_players_.empty()) return;
            StartNextPlayer_();
            return;
        }

        Global().ClearReady(pid);
        const auto& entry = CurrentEntry_(player);

        if (IsMultiCard_(entry)) {
            // Multi-card: show selection UI with card images
            // place_all=true (三年之期): show total cards to place; place_all=false (劫掠): pick 1
            int32_t choose_count = entry.place_all ? static_cast<int32_t>(entry.cards.size()) : 1;
            auto sender = Global().Boardcast();
            sender << At(pid) << " 触发「" << entry.source << "」（可选择 " << choose_count
                   << " 张），请从以下砖块中选择放置：";
            for (size_t i = 0; i < entry.cards.size(); ++i) {
                sender << " " << (i + 1) << "." << entry.cards[i].CardName();
            }
            SendCardPreview_(entry.cards);
        } else {
            // Single card: show card image, direct placement
            auto sender = Global().Boardcast();
            sender << At(pid) << " 通过「" << entry.source << "」获得额外砖块 " << entry.cards[0].CardName();
            sender << "，请放置到指定位置，剩余 " << player.extra_card_queue_.size() << " 张";
            SendCardPreview_(entry.cards);
        }
        Global().StartTimer(GAME_OPTION(局时));
    }

    // Show board + next card info for same player continuing with more entries
    void ShowNextCardForPlayer_(PlayerID pid)
    {
        auto& player = Main().players_[pid];
        if (player.extra_card_queue_.empty()) return;
        // Re-broadcast board
        Global().Boardcast() << Markdown{Main().CombHtml("## 额外砖块选择阶段")};
        const auto& entry = CurrentEntry_(player);
        if (IsMultiCard_(entry)) {
            auto sender = Global().Boardcast();
            sender << At(pid) << " 请继续选择（" << entry.source << "，剩余 " << entry.cards.size() << " 张）：";
            for (size_t i = 0; i < entry.cards.size(); ++i) {
                sender << " " << (i + 1) << "." << entry.cards[i].CardName();
            }
            SendCardPreview_(entry.cards);
        } else {
            auto sender = Global().Boardcast();
            sender << At(pid) << " 通过「" << entry.source << "」获得额外砖块 " << entry.cards[0].CardName();
            sender << "，请放置到指定位置，剩余 " << player.extra_card_queue_.size() << " 张";
            SendCardPreview_(entry.cards);
        }
    }

    void ShowRemainingCards_(PlayerID pid)
    {
        auto& player = Main().players_[pid];
        if (player.extra_card_queue_.empty()) return;
        const auto& entry = CurrentEntry_(player);
        if (entry.cards.empty()) return;

        // Re-broadcast board status after placement
        Global().Boardcast() << Markdown{Main().CombHtml("## 额外砖块选择阶段")};

        auto sender = Global().Boardcast();
        sender << At(pid) << " 请继续选择（剩余 " << entry.cards.size() << " 张）：";
        for (size_t i = 0; i < entry.cards.size(); ++i) {
            sender << " " << (i + 1) << "." << entry.cards[i].CardName();
        }
        SendCardPreview_(entry.cards);
    }

    void SendCardPreview_(const std::vector<AreaCard>& cards)
    {
        const std::string img_path = Global().ResourceDir();
        const std::string style = "<style>body{margin:0;}</style>" + GetStyle(img_path);
        if (cards.size() == 1) {
            // Single card: send directly as small image, no full Markdown wrapper
            Global().Boardcast() << Markdown(style + cards[0].ToHtml(img_path), 64);
        } else {
            html::Table card_table(1, cards.size() * 2);
            card_table.SetTableStyle("align=\"center\" cellpadding=\"0\" cellspacing=\"0\" ");
            for (size_t i = 0; i < cards.size(); ++i) {
                card_table.Get(0, i * 2).SetContent(std::to_string(i + 1) + ".");
                card_table.Get(0, i * 2 + 1).SetContent(cards[i].ToHtml(img_path));
            }
            Global().Boardcast() << Markdown(style + card_table.ToString(), 300);
        }
    }

    // Place a card for a player (handles transform, fill/discard, forge check)
    // If allow_overwrite is true (e.g. 临时用品), SeqFill even when board is full (overwrites pos 1).
    void PlaceCard_(PlayerID pid, const AreaCard& card, std::string* out_notify = nullptr, bool allow_overwrite = false)
    {
        auto& player = Main().players_[pid];
        if (player.comb_->HasEmptyPosition() || allow_overwrite) {
            auto [actual_card, transform_notify] = Main().TransformCardForPlacement_(pid, card);
            const auto [idx, result] = player.comb_->SeqFill(actual_card);
            auto [notify, actual_delta] = Main().OnCardPlaced_(
                pid, idx, result, std::nullopt, nullptr, TalentCardPlacementSource::ExtraCard);
            if (out_notify) *out_notify = transform_notify + notify;
        } else {
            Main().HandleDiscard_(pid, card);
            if (out_notify) *out_notify = "";
        }
        CheckForgeAfterPlace_(pid);
    }

    void HandleUnreadyPlayer_()
    {
        if (pending_players_.empty()) return;
        PlayerID pid = pending_players_.front();
        if (Global().IsReady(pid)) return;

        auto& player = Main().players_[pid];

        // Auto-place ALL remaining entries for this player on timeout
        while (!player.extra_card_queue_.empty()) {
            auto& entry = CurrentEntry_(player);
            const AreaCard card = entry.cards[0];
            const std::string source = entry.source;
            const auto source_talent = entry.source_talent;
            player.extra_card_queue_.erase(player.extra_card_queue_.begin());

            auto sender = Global().Boardcast();
            bool can_overwrite = (source == "临时用品");
            if (player.comb_->HasEmptyPosition()) {
                auto [actual_card, transform_notify] = Main().TransformCardForPlacement_(pid, card);
                const auto [idx, result] = player.comb_->SeqFill(actual_card);
                auto [place_notify, actual_delta] = Main().OnCardPlaced_(
                    pid, idx, result, source_talent, nullptr, TalentCardPlacementSource::ExtraCard);
                sender << At(pid) << " 超时，额外砖块自动填入位置 " << idx;
                CheckTempWildAfterPlace_(pid, source, idx);
            } else if (can_overwrite) {
                // 临时用品: overwrite position 1 when board is full
                if (auto block_msg = player.ProtectedPositionMessage(1)) {
                    Main().HandleDiscard_(pid, card, source_talent);
                    sender << At(pid) << " 超时，" << *block_msg << "，额外砖块自动弃牌";
                    CheckForgeAfterPlace_(pid);
                    continue;
                }
                const auto previous_card = player.comb_->GetCard(1);
                auto [actual_card, transform_notify] = Main().TransformCardForPlacement_(pid, card);
                const auto result = player.comb_->Fill(1, actual_card);
                auto [place_notify, actual_delta] = Main().OnCardPlaced_(
                    pid, 1, result, source_talent, previous_card ? &*previous_card : nullptr, TalentCardPlacementSource::ExtraCard);
                sender << At(pid) << " 超时，额外砖块自动覆盖位置 1";
                CheckTempWildAfterPlace_(pid, source, 1);
            } else {
                Main().HandleDiscard_(pid, card, source_talent);
                sender << At(pid) << " 超时，盘面已满自动弃牌";
            }
            CheckForgeAfterPlace_(pid);
        }
        Global().SetReady(pid);
        Global().HookUnreadyPlayers();
    }

    void CheckForgeAfterPlace_(PlayerID pid)
    {
        auto& player = Main().players_[pid];
        static constexpr Talent k_extra_card_action_end_order[] = {
            Talent::锻造,
        };
        for (const auto talent : k_extra_card_action_end_order) {
            if (!player.HasTalent(talent)) continue;
            const auto notify = player.talent_states_.at(talent)->OnExtraCardActionEnd(player);
            if (!notify.empty()) {
                Global().Boardcast() << At(pid) << " " << notify;
            }
        }
    }

    // Record temp_wild position after placement from "临时用品" source
    void CheckTempWildAfterPlace_(PlayerID pid, const std::string& source, uint32_t pos)
    {
        if (source == "临时用品" && pos > 0) {
            auto& player = Main().players_[pid];
            player.TempWild().position = pos;
            player.TempWild().rounds_left = 3;
        }
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Main().player_leave_[pid] = true;
        return StageErrCode::CONTINUE;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        HandleUnreadyPlayer_();
        return StageOver_();
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        HandleUnreadyPlayer_();
        return StageOver_();
    }

    CheckoutErrCode StageOver_()
    {
        if (pending_players_.empty()) return StageErrCode::CHECKOUT;

        PlayerID pid = pending_players_.front();
        auto& player = Main().players_[pid];

        if (player.extra_card_queue_.empty()) {
            Global().SetReady(pid);
            pending_players_.erase(pending_players_.begin());
            if (pending_players_.empty()) return StageErrCode::CHECKOUT;
            // Re-broadcast board when switching to next player
            Global().Boardcast() << Markdown{Main().CombHtml("## 额外砖块选择阶段")};
        }

        StartNextPlayer_();
        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) return StageErrCode::OK;
        auto& player = Main().players_[pid];
        if (player.extra_card_queue_.empty()) return StageErrCode::READY;

        const AreaCard card = CurrentEntry_(player).cards[0];
        const std::string source = CurrentEntry_(player).source;
        const auto source_talent = CurrentEntry_(player).source_talent;
        bool can_overwrite = (source == "临时用品");
        player.extra_card_queue_.erase(player.extra_card_queue_.begin());
        if (player.comb_->HasEmptyPosition()) {
            auto [actual_card, transform_notify] = Main().TransformCardForPlacement_(pid, card);
            const auto [idx, result] = player.comb_->SeqFill(actual_card);
            auto [place_notify, actual_delta] = Main().OnCardPlaced_(
                pid, idx, result, source_talent, nullptr, TalentCardPlacementSource::ExtraCard);
            CheckTempWildAfterPlace_(pid, source, idx);
        } else if (can_overwrite) {
            if (player.ProtectedPositionMessage(1)) {
                Main().HandleDiscard_(pid, card, source_talent);
                CheckForgeAfterPlace_(pid);
                return StageErrCode::READY;
            }
            const auto previous_card = player.comb_->GetCard(1);
            auto [actual_card, transform_notify] = Main().TransformCardForPlacement_(pid, card);
            const auto result = player.comb_->Fill(1, actual_card);
            auto [place_notify, actual_delta] = Main().OnCardPlaced_(
                pid, 1, result, source_talent, previous_card ? &*previous_card : nullptr, TalentCardPlacementSource::ExtraCard);
            CheckTempWildAfterPlace_(pid, source, 1);
        } else {
            Main().HandleDiscard_(pid, card, source_talent);
        }
        CheckForgeAfterPlace_(pid);
        return StageErrCode::READY;
    }

    // Command: place single-card entry directly (位置, 0=discard)
    AtomReqErrCode Set_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const uint32_t idx)
    {
        auto& player = Main().players_[pid];
        if (player.extra_card_queue_.empty()) {
            reply() << "[错误] 您没有待放置的额外砖块";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 当前不是您的放置回合";
            return StageErrCode::FAILED;
        }

        auto& entry = CurrentEntry_(player);
        if (IsMultiCard_(entry)) {
            reply() << "[错误] 当前有多张砖块可选，请使用「砖块序号 位置」命令";
            return StageErrCode::FAILED;
        }

        const AreaCard card = entry.cards[0];
        const std::string source = entry.source;
        const auto source_talent = entry.source_talent;

        if (idx == 0) {
            player.extra_card_queue_.erase(player.extra_card_queue_.begin());
            std::string notify = Main().HandleDiscard_(pid, card, source_talent);
            CheckForgeAfterPlace_(pid);
            reply() << "额外砖块弃牌成功" << notify;
            if (player.extra_card_queue_.empty()) return StageErrCode::READY;
            ShowNextCardForPlayer_(pid);
            Global().StartTimer(GAME_OPTION(局时));
            return StageErrCode::OK;
        }

        if (auto block_msg = player.ProtectedPositionMessage(idx)) {
            reply() << "[错误] " << *block_msg;
            return StageErrCode::FAILED;
        }

        player.extra_card_queue_.erase(player.extra_card_queue_.begin());
        const auto previous_card = player.comb_->GetCard(idx);
        auto [actual_card, transform_notify] = Main().TransformCardForPlacement_(pid, card);
        const auto result = player.comb_->Fill(idx, actual_card);
        auto [notify, actual_delta] = Main().OnCardPlaced_(
            pid, idx, result, source_talent, previous_card ? &*previous_card : nullptr, TalentCardPlacementSource::ExtraCard);
        CheckForgeAfterPlace_(pid);
        CheckTempWildAfterPlace_(pid, source, idx);

        // Build message and send immediately so ShowNextCardForPlayer_ broadcasts AFTER this reply
        std::string msg = "额外砖块设置位置 " + std::to_string(idx) + " 成功";
        if (actual_delta > 0) {
            msg += "，收获 " + std::to_string(actual_delta) + " 点积分";
        } else if (actual_delta < 0) {
            msg += "，损失 " + std::to_string(-actual_delta) + " 点积分";
        }
        msg += transform_notify + notify;
        reply() << msg;

        if (player.extra_card_queue_.empty()) return StageErrCode::READY;
        ShowNextCardForPlayer_(pid);
        Global().StartTimer(GAME_OPTION(局时));
        return StageErrCode::OK;
    }

    // Command: choose from multi-card entry then place (砖块序号, 位置, 0=discard)
    AtomReqErrCode Choose_(const PlayerID pid, const bool is_public, MsgSenderBase& reply,
                           const uint32_t card_id, const uint32_t pos)
    {
        auto& player = Main().players_[pid];
        if (player.extra_card_queue_.empty()) {
            reply() << "[错误] 您没有待选择的额外砖块";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 当前不是您的选择回合";
            return StageErrCode::FAILED;
        }

        auto& entry = CurrentEntry_(player);
        if (card_id < 1 || card_id > entry.cards.size()) {
            reply() << "[错误] 请输入 1-" << entry.cards.size() << " 之间的砖块序号";
            return StageErrCode::FAILED;
        }

        const AreaCard card = entry.cards[card_id - 1];
        const std::string source = entry.source;
        const bool place_all = entry.place_all;
        const auto source_talent = entry.source_talent;

        if (pos != 0) {
            if (auto block_msg = player.ProtectedPositionMessage(pos)) {
                reply() << "[错误] " << *block_msg;
                return StageErrCode::FAILED;
            }
        }

        if (place_all) {
            // Remove chosen card from entry, keep entry if more cards remain
            entry.cards.erase(entry.cards.begin() + card_id - 1);
            if (entry.cards.empty()) {
                player.extra_card_queue_.erase(player.extra_card_queue_.begin());
            }
        } else {
            // Pick one, discard rest
            player.extra_card_queue_.erase(player.extra_card_queue_.begin());
        }

        if (pos == 0) {
            std::string notify = Main().HandleDiscard_(pid, card, source_talent);
            CheckForgeAfterPlace_(pid);
            reply() << source << "选择 " << card_id << " 号砖块，弃牌成功" << notify;
            if (place_all && !player.extra_card_queue_.empty() && !player.extra_card_queue_.front().cards.empty()) {
                ShowRemainingCards_(pid);
                Global().StartTimer(GAME_OPTION(局时));
                return StageErrCode::OK;
            }
            return StageErrCode::READY;
        }

        const auto previous_card = player.comb_->GetCard(pos);
        auto [actual_card, transform_notify] = Main().TransformCardForPlacement_(pid, card);
        const auto result = player.comb_->Fill(pos, actual_card);
        auto [notify, actual_delta] = Main().OnCardPlaced_(
            pid, pos, result, source_talent, previous_card ? &*previous_card : nullptr, TalentCardPlacementSource::ExtraCard);
        CheckForgeAfterPlace_(pid);
        CheckTempWildAfterPlace_(pid, source, pos);

        // Build message and send immediately so ShowRemainingCards_ broadcasts AFTER this reply
        std::string msg = source + "选择 " + std::to_string(card_id) + " 号砖块，设置位置 "
                          + std::to_string(pos) + " 成功";
        if (actual_delta > 0) {
            msg += "，收获 " + std::to_string(actual_delta) + " 点积分";
        } else if (actual_delta < 0) {
            msg += "，损失 " + std::to_string(-actual_delta) + " 点积分";
        }
        msg += transform_notify + notify;
        reply() << msg;

        // For place_all entries with remaining cards, continue choosing
        if (place_all && !player.extra_card_queue_.empty() && !player.extra_card_queue_.front().cards.empty()) {
            ShowRemainingCards_(pid);
            Global().StartTimer(GAME_OPTION(局时));
            return StageErrCode::OK;
        }
        return StageErrCode::READY;
    }

    std::vector<PlayerID> pending_players_;
};

// ==================== TalentStage ====================

class TalentStage : public SubGameStage<>
{
  public:
    TalentStage(MainStage& main_stage)
            : StageFsm(main_stage, "天赋选择阶段",
                MakeStageCommand(*this, "选择天赋", &TalentStage::Choose_, ArithChecker<uint32_t>(1, 5, "天赋序号")))
    {
    }

    virtual void OnStageBegin() override
    {
        // Boardcast player boards before choosing talents
        Global().Boardcast() << Markdown{Main().CombHtml("## 天赋选择阶段")};

        // Pre-process: handle 我全都要 for qualifying players
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (Main().player_out_[pid] != 0) continue;
            auto& player = Main().players_[pid];
            if (player.talent_pool_.empty() || !player.HasTalent(Talent::我全都要)) continue;

            // Remove WANT_ALL from talents
            auto wit = std::find(player.talents_.begin(), player.talents_.end(), Talent::我全都要);
            if (wit != player.talents_.end()) player.talents_.erase(wit);

            // Acquire all talents in pool (counts as one selection)
            std::vector<Talent> acquired(player.talent_pool_.begin(), player.talent_pool_.end());
            for (auto t : acquired) {
                player.talents_.push_back(t);
                player.available_a_.erase(t);
                player.available_b_.erase(t);
            }
            player.talent_pool_.clear();
            ++player.talent_selection_count_;

            std::string msg = " 触发天赋「我全都要」！获得全部天赋：";
            for (size_t i = 0; i < acquired.size(); ++i) {
                msg += "\n「" + std::string(TalentName(acquired[i])) + "」——" + std::string(TalentDescription(acquired[i]));
                // Apply talent's immediate effects
                ApplyImmediateTalentEffects_(pid, acquired[i]);
            }
            Global().Boardcast() << At(pid) << msg;

            // Check cascading talent qualification (WANT_ALL was removed so won't re-trigger)
            uint32_t line_count = player.comb_->LineCount();
            if (player.talent_selection_count_ < line_count / 3) {
                player.GenerateTalentPool(Main().g_);
                // Will go through normal selection flow below
            }
        }

        // Set ready for players who don't need to choose
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (Main().player_out_[pid] != 0 || Main().players_[pid].talent_pool_.empty()) {
                Global().SetReady(pid);
            }
        }

        // Show talent options to each choosing player
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (Main().player_out_[pid] == 0 && !Main().players_[pid].talent_pool_.empty()) {
                auto& player = Main().players_[pid];
                auto sender = Global().Boardcast();
                sender << At(pid) << " 达成了选择天赋的条件，请选择：\n";
                // 多维抉择：候选额外扩展 1A+1B（共 5 选 1），在此给出提示
                if (player.HasTalent(Talent::多维抉择) && player.talent_pool_.size() > 3) {
                    sender << "触发天赋「多维抉择」，额外提供 2 个选项\n";
                }
                for (size_t i = 0; i < player.talent_pool_.size(); ++i) {
                    sender << (i + 1) << ". " << TalentName(player.talent_pool_[i])
                           << "（" << (IsGradeA(player.talent_pool_[i]) ? "A级" : "B级") << "）——"
                           << TalentDescription(player.talent_pool_[i]) << "\n";
                }
            }
        }
        Global().StartTimer(GAME_OPTION(局时));
    }

  private:
    void HandleUnreadyPlayers_()
    {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (Global().IsReady(pid) || Main().player_out_[pid] != 0) continue;
            auto& player = Main().players_[pid];
            if (player.talent_pool_.empty()) continue;

            // Auto-choose first talent
            Talent chosen = player.talent_pool_[0];
            player.AcquireTalent(0);
            std::string extra = ApplyImmediateTalentEffects_(pid, chosen);
            Global().Boardcast() << At(pid) << " 超时，自动选择天赋「" << TalentName(chosen) << "」" << extra;
        }
        Global().HookUnreadyPlayers();
    }

    // Returns additional message to append after "成功选择天赋「X」" (empty if no extra info)
    std::string ApplyImmediateTalentEffects_(PlayerID pid, Talent talent)
    {
        auto& player = Main().players_[pid];
        std::string extra_msg;
        switch (talent) {
            case Talent::摇奖机: {
                // Randomly draw an A-tier talent (excluding PANDORA_BOX which cannot be obtained via random talents)
                std::vector<Talent> avail;
                for (auto t : player.available_a_) {
                    if (t != Talent::潘多拉魔盒) avail.push_back(t);
                }
                if (!avail.empty()) {
                    SeededShuffle(avail.begin(), avail.end(), Main().g_);
                    Talent result = avail[0];
                    // Replace SLOT_MACHINE with the drawn talent in talents_
                    auto it = std::find(player.talents_.begin(), player.talents_.end(), Talent::摇奖机);
                    if (it != player.talents_.end()) *it = result;
                    player.available_a_.erase(result);
                    player.available_b_.erase(result);
                    // Apply the drawn talent's immediate effects
                    ApplyImmediateTalentEffects_(pid, result);
                    extra_msg = "，摇到了A级天赋「" + std::string(TalentName(result)) + "」——" + std::string(TalentDescription(result));
                } else {
                    extra_msg = "，但已没有可用的A级天赋！";
                }
                break;
            }
            case Talent::潘多拉魔盒: {
                // Randomly obtain 2 B-tier talents (exclude random-type talents like SLOT_MACHINE)
                std::vector<Talent> avail;
                for (auto t : player.available_b_) {
                    if (t != Talent::摇奖机) avail.push_back(t);
                }
                if (avail.size() >= 2) {
                    SeededShuffle(avail.begin(), avail.end(), Main().g_);
                    Talent t1 = avail[0], t2 = avail[1];
                    // Replace PANDORA_BOX with first talent
                    auto it = std::find(player.talents_.begin(), player.talents_.end(), Talent::潘多拉魔盒);
                    if (it != player.talents_.end()) *it = t1;
                    player.talents_.push_back(t2);
                    player.available_a_.erase(t1); player.available_b_.erase(t1);
                    player.available_a_.erase(t2); player.available_b_.erase(t2);
                    std::string extra1 = ApplyImmediateTalentEffects_(pid, t1);
                    std::string extra2 = ApplyImmediateTalentEffects_(pid, t2);
                    extra_msg = "，开启潘多拉魔盒！获得B级天赋「" + std::string(TalentName(t1)) + "」" + extra1
                              + " 和「" + std::string(TalentName(t2)) + "」" + extra2;
                } else if (avail.size() == 1) {
                    Talent t1 = avail[0];
                    auto it = std::find(player.talents_.begin(), player.talents_.end(), Talent::潘多拉魔盒);
                    if (it != player.talents_.end()) *it = t1;
                    player.available_a_.erase(t1); player.available_b_.erase(t1);
                    std::string extra1 = ApplyImmediateTalentEffects_(pid, t1);
                    extra_msg = "，开启潘多拉魔盒！但B级天赋已不足，仅获得B级天赋「" + std::string(TalentName(t1)) + "」" + extra1;
                } else {
                    extra_msg = "，但已没有可用的B级天赋！";
                }
                break;
            }
            default:
                extra_msg = player.talent_states_.at(talent)->OnAcquire(player, TalentAcquireContext{HasValuableOne(Main().special_event_)});
                break;
        }

        // Re-check score (talents like 还是有用的 may change scoring)
        player.UpdateScore(ScoreResult{player.comb_->BaseScore(), player.comb_->LineCount(), 0}, HasValuableOne(Main().special_event_));
        return extra_msg;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Main().player_leave_[pid] = true;
        return StageErrCode::CONTINUE;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        HandleUnreadyPlayers_();
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        HandleUnreadyPlayers_();
        return StageErrCode::CHECKOUT;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) return StageErrCode::OK;
        auto& player = Main().players_[pid];
        if (player.talent_pool_.empty()) return StageErrCode::READY;
        Talent chosen = player.talent_pool_[0];
        player.AcquireTalent(0);
        std::string extra = ApplyImmediateTalentEffects_(pid, chosen);
        return StageErrCode::READY;
    }

    AtomReqErrCode Choose_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const uint32_t choice)
    {
        auto& player = Main().players_[pid];
        if (player.talent_pool_.empty()) {
            reply() << "[错误] 当前没有可选择的天赋";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 您已经选择过天赋";
            return StageErrCode::FAILED;
        }
        if (choice < 1 || choice > player.talent_pool_.size()) {
            reply() << "[错误] 请输入 1-" << player.talent_pool_.size() << " 之间的数字";
            return StageErrCode::FAILED;
        }

        Talent chosen = player.talent_pool_[choice - 1];
        player.AcquireTalent(choice - 1);
        if (player.first_talent_round_ == 0) {
            player.first_talent_round_ = Main().round_;
        }
        std::string extra = ApplyImmediateTalentEffects_(pid, chosen);
        reply() << "成功选择天赋「" << TalentName(chosen) << "」" << extra;

        // Check if player qualifies for another talent immediately
        uint32_t line_count = player.comb_->LineCount();
        if (player.talent_selection_count_ < line_count / 3) {
            player.GenerateTalentPool(Main().g_);
            auto sender = reply();
            sender << "再次达成天赋选择条件，请继续选择：\n";
            // 多维抉择：5 选 1 提示
            if (player.HasTalent(Talent::多维抉择) && player.talent_pool_.size() > 3) {
                sender << "触发天赋「多维抉择」，额外提供 2 个选项\n";
            }
            for (size_t i = 0; i < player.talent_pool_.size(); ++i) {
                sender << (i + 1) << ". " << TalentName(player.talent_pool_[i])
                       << "（" << (IsGradeA(player.talent_pool_[i]) ? "A级" : "B级") << "）——"
                       << TalentDescription(player.talent_pool_[i]) << "\n";
            }
            return StageErrCode::OK; // Not ready yet, need to choose again
        }

        return StageErrCode::READY;
    }

};

// ==================== Stage Transitions ====================

void MainStage::AdvancePhase_(SubStageFsmSetter& setter)
{
    switch (phase_) {
        case RoundPhase::放置:
            phase_ = RoundPhase::战前额外;
            CollectPreBattleExtras_();
            if (AnyPendingExtraCards_()) {
                setter.Emplace<ExtraCardStage>(*this);
                return;
            }
            // Fall through to talent check
            [[fallthrough]];

        case RoundPhase::战前额外:
            phase_ = RoundPhase::天赋选择;
            PrepareNewTalentChoices_();
            if (AnyPendingTalentChoice_()) {
                PrepareNewTalentChoices_();
                setter.Emplace<TalentStage>(*this);
                return;
            }
            [[fallthrough]];

        case RoundPhase::天赋选择: {
            phase_ = RoundPhase::对战;

            // 紧急救援: if any alive player has unused emergency rescue, skip entire battle and trigger SelectStage
            bool has_emergency = false;
            if (round_ > 2 && !IsSelectionRound(round_)) {
                for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
                    if (player_out_[pid] != 0) continue;
                    if (players_[pid].HasTalent(Talent::紧急救援) && !players_[pid].EmergencyRescue().used) {
                        has_emergency = true;
                        Global().Boardcast() << At(pid) << " 触发天赋「紧急救援」，跳过本轮对战，启动紧急选牌阶段！";
                    }
                }
            }

            if (has_emergency) {
                // Skip battle entirely, enter emergency SelectStage
                is_emergency_select_ = true;
                setter.Emplace<SelectStage>(*this, round_);
                return;
            }

            // Normal: Do battle (automatic)
            const bool battle_happened = DoBattle_();
            DoEliminationAfterBattle_();
            DoPoison_();

            if (CheckGameOver()) {
                // 游戏结束：保留本轮触发的「绝地反击」额外分，使其计入终局结算。
                DoGameOver_();
                return;
            }

            // 游戏继续：仅在真实发生对战后，清除已经参与过这次对战的「绝地反击」额外分。
            if (battle_happened) {
                ClearCounterattackAfterRound_();
            }

            // Collect post-battle extras (三年之期, 劫掠, etc.)
            CollectPostBattleExtras_();
            if (AnyPendingExtraCards_()) {
                phase_ = RoundPhase::战后额外;
                setter.Emplace<ExtraCardStage>(*this);
                return;
            }
            [[fallthrough]];
        }

        case RoundPhase::对战:
        case RoundPhase::战后额外:
            // After all phases done, start next round
            phase_ = RoundPhase::放置;
            StartNewRound_(setter);
            return;
    }
}

void MainStage::StartNewRound_(SubStageFsmSetter& setter)
{
    round_++;

    // Per-round talent effects (applied at round start)
    for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
        if (player_out_[pid] != 0) continue;
        auto& player = players_[pid];
        const TalentRoundContext context{IsSelectionRound(round_), HasValuableOne(special_event_)};
        static constexpr Talent k_round_start_order[] = {
            Talent::零号位,
            Talent::冥想,
            Talent::利滚利,
            Talent::临时用品,
        };
        for (const auto talent : k_round_start_order) {
            if (!player.HasTalent(talent)) continue;
            const std::string message = player.talent_states_.at(talent)->OnRoundStart(player, context);
            if (!message.empty()) {
                Global().Boardcast() << At(pid) << message;
            }
        }
    }

    if (CheckGameOver()) {
        DoGameOver_();
        return;
    }

    // Check if card pool 1 is exhausted — end game
    if (it_ == cards_.end() && !IsSelectionRound(round_) && round_ > 1) {
        Global().Boardcast() << "卡池耗尽，游戏结束！";
        DoGameOver_();
        return;
    }

    if (IsSelectionRound(round_)) {
        setter.Emplace<SelectStage>(*this, round_);
    } else if (round_ == 1) {
        // Initial round: windfall gives wild cards
        if (HasWindfall(special_event_)) {
            // For windfall, every player gets a wild card in round 1
            AreaCard wild_card; // default constructor = wild
            setter.Emplace<RoundStage>(*this, round_, wild_card);
        } else {
            AreaCard dummy; // initial round uses per-player cards, this is unused
            setter.Emplace<RoundStage>(*this, round_, dummy);
        }
    } else {
        AreaCard card = DrawFromPool1_();
        // Prepare next card for foresee
        if (HasForesee(special_event_)) {
            if (it_ != cards_.end()) {
                next_card_ = *it_;
            } else {
                next_card_ = std::nullopt;
            }
        }
        setter.Emplace<RoundStage>(*this, round_, card);
    }
}

void MainStage::FirstStageFsm(SubStageFsmSetter setter)
{
    // Announce special event
    Global().Boardcast() << "本局特殊事件：" << SpecialEventName(special_event_);

#ifdef TEST_BOT
    // 测试模式：开局直接赋予所有玩家指定天赋
    const auto& test_talents = GAME_OPTION(天赋);
    if (!test_talents.empty()) {
        for (auto& player : players_) {
            for (int t : test_talents) {
                Talent talent = static_cast<Talent>(t);
                if (!player.HasTalent(talent)) {
                    player.talents_.push_back(talent);
                    player.available_a_.erase(talent);
                    player.available_b_.erase(talent);
                }
            }
            for (int t : test_talents) {
                Talent talent = static_cast<Talent>(t);
                if (!player.HasTalent(talent)) continue;
                static constexpr Talent k_initial_effect_talents[] = {
                    Talent::占得先机,
                    Talent::三年之期,
                    Talent::两级反转,
                    Talent::九转玄机,
                    Talent::临时用品,
                    Talent::包扎,
                    Talent::星河流转,
                    Talent::冥想,
                    Talent::天使轮,
                    Talent::贪婪宝藏,
                    Talent::零的力量,
                    Talent::虚空之心,
                };
                if (std::find(std::begin(k_initial_effect_talents), std::end(k_initial_effect_talents), talent) ==
                    std::end(k_initial_effect_talents)) {
                    continue;
                }
                player.talent_states_.at(talent)->OnAcquire(
                    player, TalentAcquireContext{HasValuableOne(special_event_), true});
            }
            // 更新分数（来点实在的+4 等通过 RecalcPermanentExtra_ 生效）
            player.UpdateScore(ScoreResult{player.comb_->BaseScore(), player.comb_->LineCount(), 0}, HasValuableOne(special_event_));
        }
        Global().Boardcast() << "【测试模式】已为所有玩家添加 " << test_talents.size() << " 个天赋";
    }
#endif

    phase_ = RoundPhase::放置;
    StartNewRound_(setter);
}

void MainStage::NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    // After placement, apply placement-stage-end talent effects.
    for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
        if (player_out_[pid] != 0) continue;
        auto& player = players_[pid];
        const TalentPlacementContext context{HasValuableOne(special_event_)};
        static constexpr Talent k_placement_end_order[] = {
            Talent::以退为进,
            Talent::完美块,
        };
        for (const auto talent : k_placement_end_order) {
            if (player.HasTalent(talent)) {
                player.talent_states_.at(talent)->OnPlacementStageEnd(player, context);
            }
        }
    }

    AdvancePhase_(setter);
}

void MainStage::NextStageFsm(SelectStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    // Same post-placement processing as RoundStage
    for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
        if (player_out_[pid] != 0) continue;
        auto& player = players_[pid];
        const TalentPlacementContext context{HasValuableOne(special_event_)};
        static constexpr Talent k_placement_end_order[] = {
            Talent::以退为进,
            Talent::完美块,
        };
        for (const auto talent : k_placement_end_order) {
            if (player.HasTalent(talent)) {
                player.talent_states_.at(talent)->OnPlacementStageEnd(player, context);
            }
        }
    }

    // 紧急救援: after emergency SelectStage, mark used and skip to post-battle extras (no battle)
    if (is_emergency_select_) {
        is_emergency_select_ = false;
        for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
            if (player_out_[pid] != 0) continue;
            if (players_[pid].HasTalent(Talent::紧急救援) && !players_[pid].EmergencyRescue().used) {
                players_[pid].EmergencyRescue().used = true;
            }
        }
        // Skip battle/elimination/poison, go directly to post-battle extras and next round
        CollectPostBattleExtras_();
        if (AnyPendingExtraCards_()) {
            phase_ = RoundPhase::战后额外;
            setter.Emplace<ExtraCardStage>(*this);
            return;
        }
        phase_ = RoundPhase::放置;
        StartNewRound_(setter);
        return;
    }

    AdvancePhase_(setter);
}

void MainStage::NextStageFsm(ExtraCardStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    // Apply per-placement talent effects after extra cards
    for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
        if (player_out_[pid] != 0) continue;
        auto& player = players_[pid];
        const TalentPlacementContext context{HasValuableOne(special_event_)};
        static constexpr Talent k_placement_end_order[] = {
            Talent::以退为进,
            Talent::完美块,
        };
        for (const auto talent : k_placement_end_order) {
            if (player.HasTalent(talent)) {
                player.talent_states_.at(talent)->OnPlacementStageEnd(player, context);
            }
        }
    }

    AdvancePhase_(setter);
}

void MainStage::NextStageFsm(TalentStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    // Handle extra cards from talent effects (e.g., TEMP_WILD manual placement)
    if (AnyPendingExtraCards_()) {
        setter.Emplace<ExtraCardStage>(*this);
        return;
    }
    AdvancePhase_(setter);
}

// ==================== Game Over ====================

void MainStage::DoGameOver_()
{
    // Find the winner
    for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
        if (player_out_[pid] == 0) {
            Global().Boardcast() << "游戏结束，恭喜胜者：" << At(pid) << "！";
        }
    }

    Global().Boardcast() << Markdown(CombHtml("## 终局"));

    if (GAME_OPTION(种子).empty()) {
        Global().Boardcast() << "本局特殊事件：" << SpecialEventShortName(special_event_) << "\n随机数种子：" + seed_str_;
        // Achievements
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (players_[pid].comb_->LineCount() >= 15) {
                Global().Achieve(pid, Achievement::天赋蜂王);
            }
            if (players_[pid].talents_.size() >= 6) {
                Global().Achieve(pid, Achievement::天赋满溢);
            }
            const int32_t total_score = players_[pid].TotalScore();
            if (total_score >= 300) {
                Global().Achieve(pid, Achievement::天赋初启);
            }
            if (total_score >= 340) {
                Global().Achieve(pid, Achievement::天赋进阶);
            }
            if (total_score >= 380) {
                Global().Achieve(pid, Achievement::天赋精通);
            }
            if (player_out_[pid] == 0 && players_[pid].never_lost_) {
                Global().Achieve(pid, Achievement::常胜将军);
            }
            if (player_out_[pid] == 0 && players_[pid].hp_ == 1) {
                Global().Achieve(pid, Achievement::命悬一线);
            }
            if (players_[pid].first_talent_round_ > 0 && players_[pid].first_talent_round_ < 8) {
                Global().Achieve(pid, Achievement::先发制人);
            }
            if (players_[pid].HasFullWildLine5()) {
                Global().Achieve(pid, Achievement::癞子长虹);
            }
            if (players_[pid].AllCellsSameNumber()) {
                Global().Achieve(pid, Achievement::天选之数);
            }
        }
    } else {
        Global().Boardcast() << "本局特殊事件：" << SpecialEventShortName(special_event_) << "\n自定义种子：" + seed_str_;
    }
}

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot
