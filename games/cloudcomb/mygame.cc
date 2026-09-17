// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).
//
// This file was generated with the assistance of Claude Code (claude.ai/code).

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"
#include "utility/random.h"

#include "cloudcomb.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;
const GameProperties k_properties {
    .name_ = "云顶之巢",
    .developer_ = "铁蛋",
    .description_ = "摆放砖块构建连线，并在对战中击败其他玩家，成为最后的存活者",
    // 打乱玩家编号：选牌轮的「顺位」顺序，打乱后同一批玩家每局拿到的编号不同，避免固定的先后优势。
    .shuffled_player_id_ = true,
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 8; }
uint32_t Multiple(const CustomOptions& options) { return GET_OPTION_VALUE(options, 种子).empty() ? 2 : 0; }
const MutableGenericOptions k_default_generic_options;

const std::vector<RuleCommand> k_rule_commands = {};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() < 2) {
        reply() << "云顶之巢至少需要 2 人参加游戏";
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

static std::mt19937 MakeSubRng(std::mt19937& seed_generator)
{
    std::array<uint32_t, 8> seed_data;
    for (auto& value : seed_data) {
        value = seed_generator();
    }
    std::seed_seq seq(seed_data.begin(), seed_data.end());
    return std::mt19937(seq);
}

// ==================== Forward Declarations ====================

class RoundStage;
class SelectStage;

// ==================== MainStage ====================

class MainStage : public MainGameStage<RoundStage, SelectStage>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility))
        , round_(0)
        , alive_(Global().PlayerNum())
        , player_out_(Global().PlayerNum(), 0)
        , player_leave_(Global().PlayerNum(), false)
        , player_final_score_(Global().PlayerNum(), 0)
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
        std::mt19937 seed_generator = MakeRng(seed_str_);
        event_rng_ = MakeSubRng(seed_generator);
        pool1_rng_ = MakeSubRng(seed_generator);
        pool2_rng_ = MakeSubRng(seed_generator);
        battle_rng_ = MakeSubRng(seed_generator);
        selection_order_rng_ = MakeSubRng(seed_generator);

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
            special_event_ = pool[RandInt(event_rng_, 0, static_cast<uint32_t>(pool.size() - 1))];
        } else {
            special_event_ = static_cast<SpecialEvent>(event_opt - 1);
        }

        // Create players
        const int32_t init_hp = static_cast<int32_t>(GAME_OPTION(血量));
        for (uint64_t i = 0; i < Global().PlayerNum(); ++i) {
            players_.emplace_back(Global().ResourceDir(), init_hp);
        }

        // Generate card pools
        // Pool1 (cards_): for normal rounds (round 2+), 54 base cards + 2 wild
        cards_ = GenerateBaseCards(special_event_);
        cards_.emplace_back(); // wild
        cards_.emplace_back(); // wild
        if (HasColorful(special_event_)) {
            cards_.emplace_back(); // +3 more wild for 调色盘
            cards_.emplace_back();
            cards_.emplace_back();
        }
        SeededShuffle(cards_.begin(), cards_.end(), pool1_rng_);

        // Pool2 (cards2_): for round 1 initial + selection rounds, 54 base cards, no wild
        cards2_ = GenerateBaseCards(special_event_);
        if (HasColorful(special_event_)) {
            cards2_.emplace_back(); // 调色盘：卡池2 也加入 3 张癞子
            cards2_.emplace_back();
            cards2_.emplace_back();
        }
        SeededShuffle(cards2_.begin(), cards2_.end(), pool2_rng_);

        it_ = cards_.begin();
        it2_ = cards2_.begin();

        // Damage rate
        dRate_ = std::pow(M_E, Global().PlayerNum() / 6.0) / M_E;
        iRate_ = dRate_;
    }

    virtual void FirstStageFsm(SubStageFsmSetter setter) override;
    virtual void NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;
    virtual void NextStageFsm(SelectStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;

    // 框架仅在游戏结束时取一次作为入库 game_score。由「计分」选项决定结算方式：
    //   排名（默认）：返回按淘汰名次结算的 player_final_score_（DoGameOver_ 已写入）
    //   分数：直接返回玩家的最终总分
    int64_t PlayerScore(const PlayerID pid) const override
    {
        if (GAME_OPTION(计分) == ScoreMode::分数) {
            return players_[pid].TotalScore();
        }
        return player_final_score_[pid];
    }

    // 对战比拼用的分数（盘面连线分 + 「有1吗」加成）。
    int64_t PlayerBattleScore(const PlayerID pid) const { return players_[pid].TotalScore(); }

    // 盘面原分，不受「计分」选项影响，供选牌排序与单元测试使用。
    int64_t PlayerRawTotalScore(const PlayerID pid) const { return players_[pid].TotalScore(); }

    int32_t PlayerHP(const PlayerID pid) const { return players_[pid].hp_; }

    // ===== UI / Display =====
    std::string GetName(const std::string& x);
    void SetPlayerBoard(html::Table& table, const int pos, const PlayerID pid, const bool isEliminated);
    std::string CombHtml(const std::string& str);

    // Check if game should end
    bool CheckGameOver() const { return alive_ <= 1; }

    // ===== Battle System =====
    // sender 由调用方持有，DoBattle_ 与 DoEliminationAfterBattle_ 共用，使对战结果与玩家淘汰播报合并为一条消息。
    void DoBattle_(MsgSenderBase::MsgSenderGuard& sender);
    void ProcessBattle_(PlayerID pid1, PlayerID pid2, bool mirror, std::string& result);
    void DoEliminationAfterBattle_(MsgSenderBase::MsgSenderGuard& sender);

    // ===== Scoring =====
    // 放置砖块后刷新分数。返回 {播报文本, 盘面得分变化}。
    std::pair<std::string, int32_t> OnCardPlaced_(PlayerID pid, const ScoreResult& result);

    // ===== Card Pool Management =====
    AreaCard DrawFromPool2_();
    std::vector<AreaCard> DrawBalancedCards_(uint32_t count);
    AreaCard DrawFromPool1_();

    // 放置阶段结束后：结算对战与淘汰，然后进入下一回合。
    void AfterPlacement_(SubStageFsmSetter& setter);

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
    // 0=存活，非 0 表示该玩家被淘汰的回合数；名次结算据此比较先后。
    std::vector<int32_t> player_out_;
    std::vector<bool> player_leave_;
    // DoGameOver_ 中由 ComputeRankScores_ 填入；提交给框架作 game_score 的最终名次分。
    std::vector<int64_t> player_final_score_;
    SpecialEvent special_event_;

    std::vector<AreaCard> cards_;
    decltype(cards_)::iterator it_;
    std::vector<AreaCard> cards2_;
    decltype(cards2_)::iterator it2_;

    std::mt19937 event_rng_;
    std::mt19937 pool1_rng_;
    std::mt19937 pool2_rng_;
    std::mt19937 battle_rng_;
    std::mt19937 selection_order_rng_;
    std::string seed_str_;

    // Battle state
    std::vector<std::unordered_map<int32_t, int32_t>> fought_list_;
    double dRate_, iRate_;
    int32_t fought_round_;
    int32_t last_mirror_;

    // For foresee event
    std::optional<AreaCard> next_card_;

    // Game over
    void DoGameOver_();
    void ComputeRankScores_();
};

// ==================== UI / Display ====================

inline std::string MainStage::GetName(const std::string& x)
{
    std::string ret;
    int n = x.length();
    if (n == 0) return ret;
    int l = 0;
    int r = n - 1;
    if (x[0] == '<') l++;
    if (x[r] == '>') {
        while (r >= 0 && x[r] != '(') r--;
        r--;
    }
    for (int i = l; i <= r; i++) {
        ret += x[i];
    }
    return ret;
}

// 昵称下只保留一行「积分：XXX　　血量：XXX」（与开放蜂巢云顶模式一致）。
// 「有1吗」的额外分已计入积分总值，不再单独展示明细；连线数量也不展示。
inline void MainStage::SetPlayerBoard(html::Table& table, const int pos, const PlayerID pid, const bool isEliminated)
{
    std::string board = "### " + Global().PlayerAvatar(pid, 40) + "&nbsp;&nbsp; " + Global().PlayerName(pid) +
                        "\n\n### " HTML_COLOR_FONT_HEADER(green) "积分：" + std::to_string(players_[pid].TotalScore()) +
                        HTML_FONT_TAIL HTML_COLOR_FONT_HEADER(#8B0000) "　　血量：" + std::to_string(players_[pid].hp_) +
                        HTML_FONT_TAIL "\n\n" + players_[pid].comb_->ToHtml();
    if (isEliminated) {
        table.Get(pos / 2, pos % 2).SetColor("#C0C0C0").SetContent(board);
    } else {
        table.Get(pos / 2, pos % 2).SetContent(board);
    }
}

inline std::string MainStage::CombHtml(const std::string& str)
{
    html::Table table(players_.size() / 2 + 1, 2);
    table.SetTableStyle("align=\"center\" cellpadding=\"20\" cellspacing=\"0\"");
    int pos = 0;
    // Active players
    for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
        if (player_out_[pid] == 0) {
            SetPlayerBoard(table, pos++, pid, false);
        }
    }
    // Eliminated players
    for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
        if (player_out_[pid] > 0) {
            SetPlayerBoard(table, pos++, pid, true);
        }
    }
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << dRate_;
    return str + "（伤害倍率：" + ss.str() + "）" + GetStyle(Global().ResourceDir()) + table.ToString();
}

// ==================== Scoring ====================

// Called after a card is placed to refresh the player's score.
// Returns {notification string, base score delta}.
inline std::pair<std::string, int32_t> MainStage::OnCardPlaced_(PlayerID pid, const ScoreResult& result)
{
    auto& player = players_[pid];
    std::string notify;

    const int32_t old_valuable = player.valuable_one_bonus_;
    player.UpdateScore(result, HasValuableOne(special_event_));

    // "有1吗" special event: notify if bonus changed
    if (HasValuableOne(special_event_)) {
        const int32_t valuable_delta = player.valuable_one_bonus_ - old_valuable;
        if (valuable_delta > 0) {
            notify += "\n触发特殊事件「有1吗」，额外获得 " + std::to_string(valuable_delta) + " 点积分";
        } else if (valuable_delta < 0) {
            notify += "\n特殊事件「有1吗」积分变化，减少 " + std::to_string(-valuable_delta) + " 点额外积分";
        }
    }

    return {notify, result.score_delta};
}

// ==================== Battle System ====================

inline void MainStage::DoBattle_(MsgSenderBase::MsgSenderGuard& sender)
{
    if (alive_ <= 1) return;

    // Skip battle for rounds 1, 2, and selection rounds
    if (round_ <= 2 || IsSelectionRound(round_)) {
        sender << "本轮不进行玩家对战";
        return;
    }

    std::string result;
    std::vector<PlayerID> alive_players;
    for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
        if (player_out_[pid] == 0) {
            alive_players.push_back(pid);
        }
    }

    int32_t min_round = (alive_ + 1) / 2 - 1;
    if (alive_ == 2) min_round = 0;
    while (fought_list_.size() > static_cast<size_t>(min_round)) {
        fought_list_.erase(fought_list_.begin());
    }

    // Generate fight pairs (avoid recent repeats)
    std::unordered_map<int32_t, int32_t> fight_map;
    std::vector<PlayerID> list = alive_players;
    bool retry;
    int32_t max_attempts = 100;
    do {
        retry = false;
        fight_map.clear();
        SeededShuffle(list.begin(), list.end(), battle_rng_);
        for (size_t i = 0; i + 1 < list.size(); i += 2) {
            for (const auto& hist : fought_list_) {
                auto it1 = hist.find(list[i]);
                if (it1 != hist.end() && it1->second == list[i + 1]) {
                    retry = true;
                    break;
                }
                auto it2 = hist.find(list[i + 1]);
                if (it2 != hist.end() && it2->second == list[i]) {
                    retry = true;
                    break;
                }
            }
            if (retry) break;
            fight_map[list[i]] = list[i + 1];
        }
    } while (retry && --max_attempts > 0);

    fought_list_.push_back(fight_map);
    while (fought_list_.size() > static_cast<size_t>(min_round)) {
        fought_list_.erase(fought_list_.begin());
    }

    // Process each battle pair
    for (const auto& entry : fight_map) {
        ProcessBattle_(PlayerID(entry.first), PlayerID(entry.second), false, result);
    }

    // Mirror match for odd player
    if (list.size() % 2 == 1) {
        PlayerID solo = list.back();
        PlayerID mirror;
        int32_t attempts = 0;
        do {
            mirror = list[RandInt(battle_rng_, 0, static_cast<uint32_t>(list.size() - 2))];
        } while (mirror == last_mirror_ && ++attempts < 20);
        last_mirror_ = mirror;
        ProcessBattle_(solo, mirror, true, result);
    }

    // Update damage rate
    fought_round_++;
    if (dRate_ < 1.0) {
        dRate_ = std::min(std::exp(fought_round_ * 0.22 / Global().PlayerNum()) * iRate_, 1.0);
    }
    if (dRate_ > 1.0) {
        dRate_ = std::max(std::exp(fought_round_ * -0.44 / Global().PlayerNum()) * iRate_, 1.0);
    }

    sender << result;
}

// 比拼双方分数，分低者按分差 × 伤害倍率扣血。
// mirror 为真时 pid2 只是被借用的"镜像"对手：pid2 不会受到伤害，也不会记录败绩。
inline void MainStage::ProcessBattle_(PlayerID pid1, PlayerID pid2, bool mirror, std::string& result)
{
    const int64_t score1 = PlayerBattleScore(pid1);
    const int64_t score2 = PlayerBattleScore(pid2);
    const std::string name1 = GetName(Global().PlayerName(pid1));
    const std::string name2 = GetName(Global().PlayerName(pid2));

    const int32_t score_cmp = (score1 > score2) ? 1 : (score1 < score2 ? -1 : 0);
    int32_t damage = static_cast<int32_t>((score1 - score2) * dRate_);

    std::string p1_info, p2_info;
    if (mirror) p2_info = "(镜像)";

    if (score_cmp > 0) {
        // pid1 wins, pid2 loses（镜像不受伤害、不记败绩）
        if (!mirror) {
            players_[pid2].never_lost_ = false;
            players_[pid2].hp_ -= damage;
            p2_info = "(" + std::to_string(-damage) + ")";
        }
    } else if (score_cmp < 0) {
        // pid2 wins, pid1 loses（pid1 是真实玩家，镜像败北同样扣血）
        damage = std::abs(damage);
        players_[pid1].never_lost_ = false;
        players_[pid1].hp_ -= damage;
        p1_info = "(" + std::to_string(-damage) + ")";
    } else if (!mirror) {
        p2_info = "(0)";
    }

    result += name1 + p1_info + " vs " + name2 + p2_info + "\n";
}

// Process deaths and eliminations after battle. 共用 sender，与对战结果合并播报。
inline void MainStage::DoEliminationAfterBattle_(MsgSenderBase::MsgSenderGuard& sender)
{
    for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
        if (player_out_[pid] != 0 || players_[pid].hp_ > 0) continue;

        alive_--;
        player_out_[pid] = round_;
        sender << At(pid) << " 已被淘汰！\n";
        Global().Eliminate(pid);
    }
}

// ==================== Card Pool Management ====================

// Draw next card from pool2
inline AreaCard MainStage::DrawFromPool2_()
{
    if (it2_ == cards2_.end()) {
        // Reshuffle pool2
        SeededShuffle(cards2_.begin(), cards2_.end(), pool2_rng_);
        it2_ = cards2_.begin();
    }
    return *(it2_++);
}

// Draw N balanced initial cards from pool2 (pseudo-random: minimize PointSum spread)
// Under special events that reduce card pool, target max sum difference <= 4
inline std::vector<AreaCard> MainStage::DrawBalancedCards_(uint32_t count)
{
    // Draw a candidate pool (draw more than needed, pick the most balanced set)
    const uint32_t candidate_count = std::min(static_cast<uint32_t>(std::distance(it2_, cards2_.end())),
                                               count * 3);
    if (candidate_count < count) {
        // Not enough candidates, reshuffle
        SeededShuffle(cards2_.begin(), cards2_.end(), pool2_rng_);
        it2_ = cards2_.begin();
    }

    // Draw candidates
    std::vector<AreaCard> candidates;
    uint32_t draw_count = std::min(std::max(count * 3, count + 6), static_cast<uint32_t>(std::distance(it2_, cards2_.end())));
    for (uint32_t i = 0; i < draw_count; ++i) {
        candidates.push_back(*(it2_++));
    }

    // Sort candidates by PointSum
    std::sort(candidates.begin(), candidates.end(), [](const AreaCard& a, const AreaCard& b) {
        return a.PointSum() < b.PointSum();
    });

    // Find the best window of `count` consecutive cards with minimal spread
    int32_t best_spread = INT32_MAX;
    size_t best_start = 0;
    for (size_t i = 0; i + count <= candidates.size(); ++i) {
        int32_t spread = candidates[i + count - 1].PointSum() - candidates[i].PointSum();
        if (spread < best_spread) {
            best_spread = spread;
            best_start = i;
        }
    }

    // Take the best window
    std::vector<AreaCard> result(candidates.begin() + best_start, candidates.begin() + best_start + count);

    // Put unused candidates back into pool (before the iterator)
    // Since we already advanced it2_, we rebuild the remaining pool
    std::vector<AreaCard> unused;
    for (size_t i = 0; i < candidates.size(); ++i) {
        if (i < best_start || i >= best_start + count) {
            unused.push_back(candidates[i]);
        }
    }
    // Insert unused cards back at current position
    auto pos = it2_ - cards2_.begin();
    cards2_.insert(it2_, unused.begin(), unused.end());
    it2_ = cards2_.begin() + pos;

    // Shuffle the result so assignment order is random
    SeededShuffle(result.begin(), result.end(), pool2_rng_);
    return result;
}

// Draw next card from pool1 (should never be called when exhausted)
inline AreaCard MainStage::DrawFromPool1_()
{
    assert(it_ != cards_.end());
    return *(it_++);
}

// ==================== RoundStage ====================

class RoundStage : public SubGameStage<>
{
  public:
    // Normal round: all players get the same card
    RoundStage(MainStage& main_stage, const uint64_t round, const AreaCard& card)
            : StageFsm(main_stage, "第" + std::to_string(round) + "回合",
                MakeStageCommand(*this, "放置砖块到指定位置（0 为弃牌）", &RoundStage::Set_, ArithChecker<uint32_t>(0, 19, "位置")),
                MakeStageCommand(*this, "查看游戏进展情况与本轮砖块", &RoundStage::Info_, VoidChecker("赛况")))
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
                // 顺位模式：把抽到的砖块按"数字和 → 中间数字"升序发给编号从小到大的玩家。
                // 第三、四关键字（左、右数字）用来补全次序：数字和与中间数字相同的砖块确实存在（如 3b7 / 4b6 / 8b2），只比前两项无法给出确定的顺序。
                if (GAME_OPTION(选牌顺序) == SelectOrder::顺位) {
                    std::sort(balanced.begin(), balanced.end(), [](const AreaCard& lhs, const AreaCard& rhs) {
                        if (lhs.PointSum() != rhs.PointSum()) return lhs.PointSum() < rhs.PointSum();
                        if (lhs.PointAt(1) != rhs.PointAt(1)) return lhs.PointAt(1) < rhs.PointAt(1);
                        if (lhs.PointAt(0) != rhs.PointAt(0)) return lhs.PointAt(0) < rhs.PointAt(0);
                        return lhs.PointAt(2) < rhs.PointAt(2);
                    });
                }
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
            for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
                if (Main().player_out_[pid] != 0) {
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
                const auto [idx, result] = player.comb_->SeqFill(card);
                auto [place_notify, actual_delta] = Main().OnCardPlaced_(pid, result);
                sender << At(pid) << " 超时，自动填入位置 " << idx;
                if (actual_delta > 0) {
                    sender << "，收获 " << actual_delta << " 点积分";
                }
            } else {
                // Board full, auto discard
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
            const auto [idx, result] = player.comb_->SeqFill(card);
            Main().OnCardPlaced_(pid, result);
        }
        return StageErrCode::READY;
    }

    AtomReqErrCode Set_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const uint32_t idx)
    {
        if (Global().IsReady(pid)) {
            reply() << "[错误] 您已经设置过，无法重复设置";
            return StageErrCode::FAILED;
        }
        if (Main().player_out_[pid] != 0) {
            reply() << "[错误] 您已被淘汰";
            return StageErrCode::FAILED;
        }

        auto& player = Main().players_[pid];
        const AreaCard card = GetCardForPlayer_(pid);

        if (idx == 0) {
            reply() << "弃牌成功";
            return StageErrCode::READY;
        }

        const auto result = player.comb_->Fill(idx, card);
        auto [notify, actual_delta] = Main().OnCardPlaced_(pid, result);

        auto sender = reply();
        sender << "设置位置 " << idx << " 成功";
        if (actual_delta > 0) {
            sender << "，收获 " << actual_delta << " 点积分";
        } else if (actual_delta < 0) {
            sender << "，损失 " << (-actual_delta) << " 点积分";
        }
        sender << notify;
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

    // 「赛况」回复：当前最新棋盘 + 本轮砖块图。盘面用 CombHtml 实时生成（不复用 comb_html_ 缓存）。
    AtomReqErrCode Info_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << Markdown{Main().CombHtml("## 第 " + std::to_string(round_) + " 回合")};
        if (is_initial_) {
            reply() << Markdown(InitialCardHtml_(), 300);
        } else if (HasForesee(Main().special_event_) && Main().next_card_.has_value()) {
            reply() << Markdown(ForeseeCardHtml_(), 64);
        } else {
            const std::string style = "<style>body{margin:0;}</style>" + GetStyle(Global().ResourceDir());
            reply() << Markdown(style + shared_card_.ToHtml(Global().ResourceDir()), 64);
        }
        return StageErrCode::OK;
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
                MakeStageCommand(*this, "选择并放置砖块（卡牌ID 位置，位置 0 为弃牌）", &SelectStage::Select_, ArithChecker<uint32_t>(1, 20, "选卡"), ArithChecker<uint32_t>(0, 19, "位置")),
                MakeStageCommand(*this, "查看游戏进展情况与可选砖块", &SelectStage::Info_, VoidChecker("赛况")))
            , round_(round)
            , comb_html_(main_stage.CombHtml("## 第 " + std::to_string(round) + " 回合[公共配牌阶段]"))
    {
        // Prepare selection order: alive players sorted by HP (low first), then score (low first)
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (Main().player_out_[pid] == 0) {
                current_players_.push_back(pid);
            }
        }
        // 「选牌顺序」决定血量与分数都相同时的先后：
        //   随机（默认）：先按随机种子打乱，平局顺序由种子决定
        //   顺位：不打乱，平局按开局分配的玩家编号升序
        // 排序必须用 stable_sort：std::sort 对等价元素的相对顺序没有任何保证，
        // 会让上面这一步确定的平局顺序失效（两种模式都失真）。
        if (GAME_OPTION(选牌顺序) == SelectOrder::随机) {
            SeededShuffle(current_players_.begin(), current_players_.end(), Main().selection_order_rng_);
        }
        std::stable_sort(current_players_.begin(), current_players_.end(), [this](const PlayerID& p1, const PlayerID& p2) {
            auto& player1 = Main().players_[p1];
            auto& player2 = Main().players_[p2];
            if (player1.hp_ != player2.hp_) return player1.hp_ < player2.hp_;
            // 用 PlayerRawTotalScore 不能用 PlayerScore：后者受「计分」选项影响，排名模式下游戏中固定为 0
            return Main().PlayerRawTotalScore(p1) < Main().PlayerRawTotalScore(p2);
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
            const auto [idx, result] = player.comb_->SeqFill(card);
            Main().OnCardPlaced_(pid, result);
            sender << At(pid) << " 超时，自动选择 1 号砖块填入位置 " << idx;
        } else {
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
            const auto [idx, result] = player.comb_->SeqFill(card);
            Main().OnCardPlaced_(pid, result);
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

        Selected_(card_id);

        if (pos == 0) {
            reply() << "选择 " << card_id << " 号砖块，弃牌成功";
            return StageErrCode::READY;
        }

        const auto result = player.comb_->Fill(pos, card);
        auto [notify, actual_delta] = Main().OnCardPlaced_(pid, result);
        auto sender = reply();
        sender << "选择 " << card_id << " 号砖块，设置位置 " << pos << " 成功";
        if (actual_delta > 0) {
            sender << "，收获 " << actual_delta << " 点积分";
        } else if (actual_delta < 0) {
            sender << "，损失 " << (-actual_delta) << " 点积分";
        }
        sender << notify;
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
            avatar_table.Get(0, i + 1).SetContent(Global().PlayerAvatar(current_players_[i], 40));
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

    // 「赛况」回复：当前最新棋盘 + 剩余待选卡总图（含选卡顺序头像）。
    AtomReqErrCode Info_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << Markdown{Main().CombHtml("## 第 " + std::to_string(round_) + " 回合[公共配牌阶段]")};
        reply() << Markdown(SelectCardHtml_(), 300);
        return StageErrCode::OK;
    }

    const uint64_t round_;
    std::vector<PlayerID> current_players_;
    std::vector<AreaCard> tmp_cards_;
    std::string comb_html_;
};

// ==================== Stage Transitions ====================

void MainStage::AfterPlacement_(SubStageFsmSetter& setter)
{
    // 对战结果与玩家淘汰共用同一个 sender，合并为一条播报。
    {
        auto sender = Global().Boardcast();
        DoBattle_(sender);
        DoEliminationAfterBattle_(sender);
    }

    if (CheckGameOver()) {
        DoGameOver_();
        return;
    }

    StartNewRound_(setter);
}

void MainStage::StartNewRound_(SubStageFsmSetter& setter)
{
    round_++;

    // Check if card pool 1 is exhausted — end game
    if (it_ == cards_.end() && !IsSelectionRound(round_) && round_ > 1) {
        Global().Boardcast() << "卡池耗尽，游戏结束！";
        DoGameOver_();
        return;
    }

    if (IsSelectionRound(round_)) {
        setter.Emplace<SelectStage>(*this, round_);
    } else if (round_ == 1) {
        // Initial round uses per-player cards prepared inside RoundStage; this card is unused
        AreaCard dummy;
        setter.Emplace<RoundStage>(*this, round_, dummy);
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

    StartNewRound_(setter);
}

void MainStage::NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    AfterPlacement_(setter);
}

void MainStage::NextStageFsm(SelectStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    AfterPlacement_(setter);
}

// ==================== Game Over ====================

// 按淘汰名次结算每位玩家的最终 game_score。
// rank_value = 淘汰回合数；存活玩家用 INT64_MAX 视为最佳（并列）。
// 对玩家 X：worse = 比 X 淘汰早的玩家数，better = 比 X 淘汰晚或仍存活的玩家数。
// player_final_score_[X] = (worse - better) * 100。同回合淘汰自然并列同分；
// 卡池耗尽时所有存活者并列第一。
void MainStage::ComputeRankScores_()
{
    auto rank_value = [&](PlayerID pid) -> int64_t {
        if (player_out_[pid] == 0) return std::numeric_limits<int64_t>::max();
        return static_cast<int64_t>(player_out_[pid]);
    };
    const uint32_t N = Global().PlayerNum();
    for (PlayerID pid = 0; pid.Get() < N; ++pid) {
        const int64_t mine = rank_value(pid);
        int32_t worse = 0, better = 0;
        for (PlayerID other = 0; other.Get() < N; ++other) {
            if (other == pid) continue;
            const int64_t v = rank_value(other);
            if (v < mine)      ++worse;   // other 淘汰更早 → 我比 ta 名次好
            else if (v > mine) ++better;  // other 淘汰更晚 / 存活 → 我比 ta 名次差
            // 相等：并列名次，不计入两边
        }
        player_final_score_[pid] = (worse - better) * 100;
    }
}

void MainStage::DoGameOver_()
{
    ComputeRankScores_();

    // 单一存活者（正常淘汰流程）→ 公告获胜玩家。
    if (alive_ == 1) {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (player_out_[pid] == 0) {
                Global().Boardcast() << "游戏结束，恭喜胜者：" << At(pid) << "！";
                break;
            }
        }
    }

    Global().Boardcast() << Markdown(CombHtml("## 终局"));

    if (GAME_OPTION(种子).empty()) {
        Global().Boardcast() << "本局特殊事件：" << SpecialEventShortName(special_event_) << "\n随机数种子：" + seed_str_;
        // Achievements
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (players_[pid].comb_->LineCount() >= 15) {
                Global().Achieve(pid, Achievement::万线归巢);
            }
            const int32_t total_score = players_[pid].TotalScore();
            if (total_score >= 280) {
                Global().Achieve(pid, Achievement::初入云端);
            }
            if (total_score >= 320) {
                Global().Achieve(pid, Achievement::扶摇直上);
            }
            if (total_score >= 350) {
                Global().Achieve(pid, Achievement::登顶云霄);
            }
            if (player_out_[pid] == 0 && players_[pid].never_lost_) {
                Global().Achieve(pid, Achievement::常胜将军);
            }
            if (player_out_[pid] == 0 && players_[pid].hp_ == 1) {
                Global().Achieve(pid, Achievement::命悬一线);
            }
            if (players_[pid].HasFullWildLine()) {
                Global().Achieve(pid, Achievement::云中彩桥);
            }
            if (players_[pid].AllCellsSameNumber()) {
                Global().Achieve(pid, Achievement::天选之数);
            }
        }
    } else {
        Global().Boardcast() << "本局特殊事件：" << SpecialEventShortName(special_event_) << "\n本局使用了自定义种子";
    }
}

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot
