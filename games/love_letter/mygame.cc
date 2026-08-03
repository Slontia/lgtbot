// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"

#include <algorithm>
#include <random>
#include <string>

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;

const GameProperties k_properties {
    .name_ = "情书",
    .developer_ = "50er",
    .description_ = "经典桌游情书：16张角色牌，打出卡牌效果淘汰对手，存活到最后或手牌最大者获胜。\n"
                    "1卫兵×5 / 2神父×2 / 3男爵×2 / 4侍女×2 / 5王子×2 / 6国王×1 / 7伯爵夫人×1 / 8公主×1",
    .shuffled_player_id_ = false,
};

uint64_t MaxPlayerNum(const CustomOptions& options) { return 4; }
uint32_t Multiple(const CustomOptions& options) { return 1; }
const MutableGenericOptions k_default_generic_options;

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options,
                  const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() < 2) {
        reply() << "该游戏至少 2 人参加，当前玩家数为 " << generic_options_readonly.PlayerNum();
        return false;
    }
    return true;
}

const std::vector<RuleCommand> k_rule_commands = {
    RuleCommand("卡牌效果",
        [] () -> const char* {
            return "1. 卫兵(×5)：猜一名其他玩家的手牌（不能猜卫兵），猜对则其淘汰\n"
                   "2. 神父(×2)：查看一名其他玩家的手牌\n"
                   "3. 男爵(×2)：与一名其他玩家比手牌大小，小者淘汰，平局无事\n"
                   "4. 侍女(×2)：打出后直到下回合开始，免疫其他玩家的卡牌效果\n"
                   "5. 王子(×2)：指定任意玩家弃掉手牌并摸一张新牌（若弃掉公主则淘汰）\n"
                   "6. 国王(×1)：与一名其他玩家交换手牌\n"
                   "7. 伯爵夫人(×1)：无效果，但若手中有国王或王子则必须打出\n"
                   "8. 公主(×1)：打出即淘汰";
        },
        VoidChecker("卡牌")),
};

const std::vector<InitOptionsCommand> k_init_options_commands = {};

// ========== Card definitions ==========
enum Card : uint8_t {
    GUARD    = 1, // 卫兵
    PRIEST   = 2, // 神父
    BARON    = 3, // 男爵
    HANDMAID = 4, // 侍女
    PRINCE   = 5, // 王子
    KING     = 6, // 国王
    COUNTESS = 7, // 伯爵夫人
    PRINCESS = 8, // 公主
};

const char* CardName(const uint8_t c)
{
    switch (c) {
    case GUARD:    return "卫兵(1)";
    case PRIEST:   return "神父(2)";
    case BARON:    return "男爵(3)";
    case HANDMAID: return "侍女(4)";
    case PRINCE:   return "王子(5)";
    case KING:     return "国王(6)";
    case COUNTESS: return "伯爵夫人(7)";
    case PRINCESS: return "公主(8)";
    default:       return "未知";
    }
}

std::vector<uint8_t> MakeDeck()
{
    return {GUARD, GUARD, GUARD, GUARD, GUARD,
            PRIEST, PRIEST, BARON, BARON,
            HANDMAID, HANDMAID, PRINCE, PRINCE,
            KING, COUNTESS, PRINCESS};
}

// ========== Player Info ==========
struct PlayerInfo {
    bool alive = true;
    bool protect = false;   // 侍女保护
    uint8_t scores = 0;        // 累计胜局数
    std::vector<uint8_t> hand; // 手牌（通常1~2张）
    std::vector<uint8_t> discards; // 已打出的牌
};

// ========== Forward declarations ==========
class RoundStage;

// ========== MainStage ==========
class MainStage : public MainGameStage<RoundStage>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility),
                   MakeStageCommand(*this, "查看当前游戏进展情况", &MainStage::Status_, VoidChecker("赛况")))
        , round_(0)
        , player_infos_(Global().PlayerNum())
    {}

    virtual void FirstStageFsm(SubStageFsmSetter setter) override;
    virtual void NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_infos_[pid].scores; }

    std::vector<PlayerInfo> player_infos_;
    std::vector<uint8_t> deck_;       // 牌堆（背面朝上）
    uint8_t burn_card_ = 0;            // 面朝下移除的牌（王子摸空时使用）
    uint8_t TargetScore_() const
    {
        switch (Global().PlayerNum()) {
        case 2: return 7;
        case 3: return 5;
        default: return 4;
        }
    }
    std::vector<uint8_t> removed_;    // 本局移除的牌（面朝上公示）
    uint8_t round_ = 0;               // 当前局数（0-based）
    uint8_t turn_ = 1;                // 当前局内第几轮
    uint8_t alive_cnt_ = 0;           // 本局存活人数（用于判断一轮是否完成）
    PlayerID current_pid_ = 0;        // 当前回合玩家
    PlayerID first_player_ = 0;       // 当前局先手玩家

    void BroadcastStatus_()
    {
        std::string s;
        s += "## 情书 第" + std::to_string(round_ + 1) + "局 第" + std::to_string(turn_) + "轮\n\n";
        s += "目标分数：" + std::to_string(TargetScore_()) + "\n\n";
        s += "| 玩家 | 得分 | 状态 | 弃牌堆 |\n";
        s += "|------|------|------|--------|\n";
        for (PlayerID i = 0; i < Global().PlayerNum(); ++i) {
            const auto& p = player_infos_[i];
            s += "| " + Global().PlayerName(i) + " | " + std::to_string(p.scores) + " | ";
            s += p.alive ? (p.protect ? "🛡️侍女" : "存活") : "淘汰";
            s += " | ";
            for (auto c : p.discards) { s += CardName(c); s += " "; }
            s += " |\n";
        }
        s += "\n剩余牌堆：" + std::to_string(deck_.size()) + " 张";
        if (!removed_.empty()) {
            s += "\n\n已移除公示：";
            for (auto c : removed_) { s += CardName(c); s += " "; }
        }
        s += "\n当前行动：" + Global().PlayerName(current_pid_);
        Global().Boardcast() << Markdown(s);
    }

  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        BroadcastStatus_();
        return StageErrCode::OK;
    }

    void SetupRound_()
    {
        deck_ = MakeDeck();
        removed_.clear();
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(deck_.begin(), deck_.end(), g);

        // 面朝下移除1张牌，本局不使用（不公示，王子摸空时发放）
        if (!deck_.empty()) {
            burn_card_ = deck_.back();
            deck_.pop_back();
        }

        // 2人局：再从牌堆顶面朝上移除3张牌并公示
        if (Global().PlayerNum() == 2) {
            for (int i = 0; i < 3 && !deck_.empty(); ++i) {
                removed_.push_back(deck_.back());
                deck_.pop_back();
            }
            std::string msg = "2人局移除并公示的3张牌：";
            for (auto c : removed_) { msg += CardName(c); msg += " "; }
            Global().Boardcast() << msg;
        }

        for (PlayerID i = 0; i < Global().PlayerNum(); ++i) {
            player_infos_[i].alive = true;
            player_infos_[i].protect = false;
            player_infos_[i].hand.clear();
            player_infos_[i].discards.clear();
            if (!deck_.empty()) {
                player_infos_[i].hand.push_back(deck_.back());
                deck_.pop_back();
            }
        }
        current_pid_ = first_player_;
        turn_ = 1;
        alive_cnt_ = Global().PlayerNum();
    }
};

// ========== RoundStage ==========
class RoundStage : public SubGameStage<>
{
  public:
    RoundStage(MainStage& main_stage)
        : StageFsm(main_stage,
                   "第 " + std::to_string(main_stage.round_ + 1) + " 局",
                   MakeStageCommand(*this, "打出一张手牌",
                                    CommandFlag::UNREADY_ONLY,
                                    &RoundStage::Play_,
                                    ArithChecker<uint32_t>(1, 8, "卡牌"),
                                    OptionalChecker<ArithChecker<uint32_t>>(0, 15, "目标玩家ID"),
                                    OptionalChecker<ArithChecker<uint32_t>>(2, 8, "猜测卡牌(仅卫兵需要)")))
    {}

    // -------- 阶段回调 --------
    virtual void OnStageBegin() override
    {
        DrawAndNotify_(Main().current_pid_);
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        Global().Boardcast() << Global().PlayerName(Main().current_pid_) << " 超时，自动打出第一张手牌";
        ForcePlay_(Main().current_pid_);
        return AfterPlay_();
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        auto& p = Main().player_infos_[pid];
        if (p.alive) {
            p.alive = false;
            Global().Boardcast() << Global().PlayerName(pid) << " 退出游戏，被淘汰";
            Eliminate_(pid);
            Global().Eliminate(pid);
        }
        return AfterPlay_();
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        return StageErrCode::OK;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        return AfterPlay_();
    }

  private:
    // -------- 玩家命令 --------
    AtomReqErrCode Play_(const PlayerID pid, const bool is_public, MsgSenderBase& reply,
                         const uint32_t card_val,
                         const std::optional<uint32_t>& target_opt,
                         const std::optional<uint32_t>& guard_guess_opt)
    {
        if (pid != Main().current_pid_) {
            reply() << "[错误] 还没轮到你的回合";
            return StageErrCode::FAILED;
        }
        const auto target = target_opt.has_value()
                                ? PlayerID{static_cast<uint32_t>(*target_opt)}
                                : PlayerID{99}; // 99 = 无目标
        const auto guard_guess = guard_guess_opt.has_value()
                                     ? static_cast<uint8_t>(*guard_guess_opt)
                                     : uint8_t{0};
        return PlayInternal_(pid, reply, static_cast<uint8_t>(card_val), target, guard_guess);
    }

    AtomReqErrCode PlayInternal_(const PlayerID pid, MsgSenderBase& reply,
                                 const uint8_t card, const PlayerID target, const uint8_t guard_guess)
    {
        auto& p = Main().player_infos_[pid];

        // 校验手牌
        auto it = std::find(p.hand.begin(), p.hand.end(), card);
        if (it == p.hand.end()) {
            reply() << "[错误] 你手里没有这张牌，你的手牌：";
            for (auto c : p.hand) reply() << CardName(c) << " ";
            return StageErrCode::FAILED;
        }

        // 伯爵夫人强制规则
        if (MustPlayCountess_(p.hand) && card != COUNTESS) {
            reply() << "[错误] 你同时持有伯爵夫人和国王/王子，必须打出伯爵夫人(7)";
            return StageErrCode::FAILED;
        }

        // 校验目标
        const bool needs_target = (card == GUARD || card == PRIEST || card == BARON ||
                                   card == PRINCE || card == KING);
        if (needs_target && card != PRINCE) {
            // 检查是否有可选目标（排除自己、淘汰、受侍女保护的玩家）
            bool has_valid_target = false;
            for (PlayerID i = 0; i < Global().PlayerNum(); ++i) {
                if (i == pid) continue;
                if (Main().player_infos_[i].alive && !Main().player_infos_[i].protect)
                    has_valid_target = true;
            }
            if (!has_valid_target) {
                // 无有效目标，允许空发（只弃牌，不执行效果）
                p.hand.erase(it);
                p.discards.push_back(card);
                reply() << "你打出了 " << CardName(card) << "（无有效目标，空发）";
                Global().Boardcast() << Global().PlayerName(pid) << " 打出了 "
                    << CardName(card) << "，但无有效目标";
                return AfterPlay_();
            }
            if (target >= Global().PlayerNum() || !Main().player_infos_[target].alive ||
                target == pid || Main().player_infos_[target].protect) {
                reply() << "[错误] 目标无效（已淘汰、是自己或受侍女保护）";
                return StageErrCode::FAILED;
            }
        } else if (card == PRINCE) {
            if (target >= Global().PlayerNum() || !Main().player_infos_[target].alive ||
                (target != pid && Main().player_infos_[target].protect)) {
                reply() << "[错误] 目标无效（已淘汰或受侍女保护）";
                return StageErrCode::FAILED;
            }
        }

        // 卫兵必须填写猜测
        if (card == GUARD && guard_guess == 0) {
            reply() << "[错误] 卫兵需要猜测目标的手牌（2-8）";
            return StageErrCode::FAILED;
        }

        // 从手牌移除，加入弃牌堆
        p.hand.erase(it);
        p.discards.push_back(card);

        // 执行卡牌效果
        ResolveCard_(pid, card, target, guard_guess);

        reply() << "你打出了 " << CardName(card);

        // 直接处理后置流程（回合切换/局结束）
        return AfterPlay_();
    }

    // -------- 卡牌效果分发 --------
    using CardFunc = void (RoundStage::*)(PlayerID, PlayerID, uint8_t);

    void ResolveCard_(const PlayerID pid, const uint8_t card,
                      const PlayerID target, const uint8_t guard_guess)
    {
        static constexpr CardFunc kCardFuncs[] = {
            nullptr,                // 0 - unused
            &RoundStage::DoGuard_,  // 1
            &RoundStage::DoPriest_, // 2
            &RoundStage::DoBaron_,  // 3
            &RoundStage::DoHandmaid_, // 4
            &RoundStage::DoPrince_, // 5
            &RoundStage::DoKing_,   // 6
            &RoundStage::DoCountess_, // 7
            &RoundStage::DoPrincess_, // 8
        };
        if (card > 0 && card < 9) {
            (this->*kCardFuncs[card])(pid, target, guard_guess);
        }
    }

    // -------- 各卡牌效果 --------
    void DoGuard_(PlayerID pid, PlayerID target, uint8_t guard_guess)
    {
        auto& ps = Main().player_infos_;
        if (target >= Global().PlayerNum() || !ps[target].alive || target == pid) return;
        Global().Boardcast() << Global().PlayerName(pid) << " 猜测 "
            << Global().PlayerName(target) << " 的手牌是 " << CardName(guard_guess);
        if (!ps[target].hand.empty() && ps[target].hand[0] == guard_guess) {
            Eliminate_(target);
            Global().Boardcast() << "猜测正确！" << Global().PlayerName(target) << " 被淘汰";
        } else {
            Global().Boardcast() << "猜测错误！";
        }
    }

    void DoPriest_(PlayerID pid, PlayerID target, uint8_t)
    {
        auto& ps = Main().player_infos_;
        if (target >= Global().PlayerNum() || !ps[target].alive || target == pid) return;
        std::string hs;
        for (auto c : ps[target].hand) { hs += CardName(c); hs += " "; }
        Global().Tell(pid) << Global().PlayerName(target) << " 的手牌：" << hs;
        Global().Boardcast() << Global().PlayerName(pid) << " 查看了 "
            << Global().PlayerName(target) << " 的手牌";
    }

    void DoBaron_(PlayerID pid, PlayerID target, uint8_t)
    {
        auto& ps = Main().player_infos_;
        if (target >= Global().PlayerNum() || !ps[target].alive || target == pid) return;
        const uint8_t a = ps[pid].hand.empty() ? 0 : ps[pid].hand[0];
        const uint8_t b = ps[target].hand.empty() ? 0 : ps[target].hand[0];
        Global().Boardcast() << Global().PlayerName(pid) << "(" << CardName(a) << ") 与 "
            << Global().PlayerName(target) << "(" << CardName(b) << ") 决斗！";
        if (a > b) {
            Eliminate_(target);
            Global().Boardcast() << Global().PlayerName(target) << " 点数较小，被淘汰";
        } else if (a < b) {
            Eliminate_(pid);
            Global().Boardcast() << Global().PlayerName(pid) << " 点数较小，被淘汰";
        } else {
            Global().Boardcast() << "点数相同，平局！";
        }
    }

    void DoHandmaid_(PlayerID pid, PlayerID, uint8_t)
    {
        Main().player_infos_[pid].protect = true;
        Global().Boardcast() << Global().PlayerName(pid) << " 打出侍女，直到下回合开始不受其他玩家卡牌效果影响";
    }

    void DoPrince_(PlayerID pid, PlayerID target, uint8_t)
    {
        auto& ps = Main().player_infos_;
        auto& deck = Main().deck_;
        if (target >= Global().PlayerNum() || !ps[target].alive) return;
        if (!ps[target].hand.empty()) {
            const uint8_t discarded = ps[target].hand.back();
            ps[target].hand.pop_back();
            ps[target].discards.push_back(discarded);
            Global().Boardcast() << Global().PlayerName(target) << " 弃掉了 " << CardName(discarded);
            if (discarded == PRINCESS) {
                Eliminate_(target);
                Global().Boardcast() << Global().PlayerName(target) << " 弃掉了公主，被淘汰！";
                return;
            }
        }
        if (!deck.empty()) {
            ps[target].hand.push_back(deck.back());
            deck.pop_back();
            Global().Tell(target) << "你抽到了 " << CardName(ps[target].hand.back());
        } else {
            // 牌堆空了，拿取面朝下移除的那张牌
            ps[target].hand.push_back(Main().burn_card_);
            Global().Boardcast() << "牌堆已空，" << Global().PlayerName(target)
                << " 拿取了面朝下移除的牌";
            Global().Tell(target) << "你拿取了面朝下移除的牌：" << CardName(Main().burn_card_);
        }
    }

    void DoKing_(PlayerID pid, PlayerID target, uint8_t)
    {
        auto& ps = Main().player_infos_;
        if (target >= Global().PlayerNum() || !ps[target].alive || target == pid) return;
        std::swap(ps[pid].hand, ps[target].hand);
        Global().Boardcast() << Global().PlayerName(pid) << " 与 "
            << Global().PlayerName(target) << " 交换了手牌";
        if (!ps[pid].hand.empty())
            Global().Tell(pid) << "你的新手牌：" << CardName(ps[pid].hand[0]);
        if (!ps[target].hand.empty())
            Global().Tell(target) << "你的新手牌：" << CardName(ps[target].hand[0]);
    }

    void DoCountess_(PlayerID pid, PlayerID, uint8_t)
    {
        Global().Boardcast() << Global().PlayerName(pid) << " 打出了伯爵夫人（无效果）";
    }

    void DoPrincess_(PlayerID pid, PlayerID, uint8_t)
    {
        Eliminate_(pid);
        Global().Boardcast() << Global().PlayerName(pid) << " 打出了公主，被淘汰！";
    }

    // -------- 辅助函数 --------
    static bool MustPlayCountess_(const std::vector<uint8_t>& hand)
    {
        bool has_c = false, has_kp = false;
        for (auto c : hand) {
            if (c == COUNTESS) has_c = true;
            if (c == KING || c == PRINCE) has_kp = true;
        }
        return has_c && has_kp;
    }

    void Eliminate_(const PlayerID pid)
    {
        Main().player_infos_[pid].alive = false;
    }

    void DrawAndNotify_(const PlayerID pid)
    {
        auto& p = Main().player_infos_[pid];
        auto& deck = Main().deck_;

        // 摸一张牌
        if (!deck.empty()) {
            p.hand.push_back(deck.back());
            deck.pop_back();
        }

        // 侍女保护到期
        p.protect = false;

        // 告知手牌
        std::string hs;
        for (auto c : p.hand) hs += CardName(c) + std::string(" ");
        Global().Tell(pid) << "轮到你了！你的手牌：" << hs;

        // 列出可选目标（排除受侍女保护的玩家，但王子可选中自己）
        std::string ts;
        for (PlayerID i = 0; i < Global().PlayerNum(); ++i) {
            if (i != pid && Main().player_infos_[i].alive && !Main().player_infos_[i].protect) {
                ts += std::to_string(i) + "(" + Global().PlayerName(i) + ") ";
            }
        }
        Global().Tell(pid) << "可选目标：" + (ts.empty() ? std::string("无（可空发或指定自己）") : ts);
        Global().Tell(pid) << "格式：<卡牌值1-8> [目标ID] [猜测卡牌(仅卫兵)]（公屏输入即可）";

        Global().Boardcast() << "轮到 " << Global().PlayerName(pid) << " 行动（第"
            << std::to_string(Main().round_ + 1) << "局 第"
            << std::to_string(Main().turn_) << "轮）";

        // 公屏播报赛况
        Main().BroadcastStatus_();

        Global().StartTimer(GAME_OPTION(时限));
    }

    void ForcePlay_(const PlayerID pid)
    {
        auto& p = Main().player_infos_[pid];
        if (p.hand.empty()) return;

        const uint8_t card = MustPlayCountess_(p.hand) ? COUNTESS : p.hand[0];

        PlayerID target = pid;
        for (PlayerID i = 0; i < Global().PlayerNum(); ++i) {
            if (i != pid && Main().player_infos_[i].alive) { target = i; break; }
        }

        // 从手牌移除
        auto it = std::find(p.hand.begin(), p.hand.end(), card);
        if (it != p.hand.end()) {
            p.hand.erase(it);
            p.discards.push_back(card);
        }

        ResolveCard_(pid, card, target, PRIEST);
    }

    CheckoutErrCode AfterPlay_()
    {
        auto& ps = Main().player_infos_;
        auto& deck = Main().deck_;
        const PlayerID pn = Global().PlayerNum();

        // 统计存活玩家
        std::vector<PlayerID> alive;
        for (PlayerID i = 0; i < pn; ++i) {
            if (ps[i].alive) alive.push_back(i);
        }

        // 判断本局是否结束
        if (alive.size() <= 1 || deck.empty()) {
            PlayerID winner = 0;

            if (alive.size() == 1) {
                winner = alive[0];
                Global().Boardcast() << Global().PlayerName(winner) << " 是唯一幸存者，赢得本局！";
            } else if (alive.size() > 1) {
                // 比手牌大小
                uint8_t high = 0;
                for (auto pid : alive) {
                    const uint8_t v = ps[pid].hand.empty() ? 0 : ps[pid].hand[0];
                    if (v > high) high = v;
                }
                std::vector<PlayerID> tied;
                for (auto pid : alive) {
                    if ((ps[pid].hand.empty() ? 0 : ps[pid].hand[0]) == high)
                        tied.push_back(pid);
                }

                if (tied.size() == 1) {
                    winner = tied[0];
                    Global().Boardcast() << "牌堆耗尽！" << Global().PlayerName(winner)
                        << " 手牌最大（" << CardName(high) << "），赢得本局！";
                } else {
                    // 平局：比较弃牌堆总和
                    uint32_t best_sum = 0;
                    for (auto pid : tied) {
                        uint32_t sum = 0;
                        for (auto c : ps[pid].discards) sum += c;
                        if (sum > best_sum) { best_sum = sum; winner = pid; }
                    }
                    Global().Boardcast() << "牌堆耗尽且手牌平局！弃牌堆总和最大的是 "
                        << Global().PlayerName(winner) << "，赢得本局！";
                }
            }

            // 公布所有存活玩家的手牌
            for (auto pid : alive) {
                if (!ps[pid].hand.empty()) {
                    Global().Boardcast() << Global().PlayerName(pid) << " 的手牌："
                        << CardName(ps[pid].hand[0]);
                }
            }

            ps[winner].scores++;
            Main().first_player_ = winner;
            Global().ClearReady();
            return StageErrCode::CHECKOUT;
        }

        // 本局继续：移到下一个存活玩家
        Main().current_pid_ = (Main().current_pid_ + 1) % pn;
        while (!ps[Main().current_pid_].alive)
            Main().current_pid_ = (Main().current_pid_ + 1) % pn;

        // 一轮计数：所有存活玩家都行动过一次后轮数+1
        if (--Main().alive_cnt_ == 0) {
            ++Main().turn_;
            Main().alive_cnt_ = static_cast<uint8_t>(alive.size());
        }

        DrawAndNotify_(Main().current_pid_);
        Global().ClearReady();
        return StageErrCode::CONTINUE;
    }
};

// ========== MainStage 阶段切换 ==========
void MainStage::FirstStageFsm(SubStageFsmSetter setter)
{
    SetupRound_();
    Global().Boardcast() << "=== 情书 第 " << std::to_string(round_ + 1) << " 局开始 ===";
    setter.Emplace<RoundStage>(*this);
}

void MainStage::NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    // 检查是否有人达到目标分数
    const uint8_t target = TargetScore_();
    for (PlayerID i = 0; i < Global().PlayerNum(); ++i) {
        if (player_infos_[i].scores >= target) {
            Global().Boardcast() << "游戏结束！" << Global().PlayerName(i)
                << " 率先获得 " << std::to_string(target) << " 分，赢得比赛！";
            std::string ss = "最终得分：";
            for (PlayerID j = 0; j < Global().PlayerNum(); ++j)
                ss += Global().PlayerName(j) + " " + std::to_string(player_infos_[j].scores) + "分  ";
            Global().Boardcast() << ss;
            return; // 游戏结束（不设置新阶段）
        }
    }

    ++round_;
    SetupRound_();
    Global().Boardcast() << "=== 情书 第 " << std::to_string(round_ + 1) << " 局开始 ===";
    setter.Emplace<RoundStage>(*this);
}

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot

