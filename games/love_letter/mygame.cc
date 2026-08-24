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
    .description_ = "经典桌游情书：打出卡牌淘汰对手，存活或手牌最大者获胜。",
    .shuffled_player_id_ = false,
};

static uint8_t s_player_num = 0;

uint64_t MaxPlayerNum(const CustomOptions& options) { return 8; }
uint32_t Multiple(const CustomOptions& options) {
    const auto v = GET_OPTION_VALUE(options, 牌堆);
    if (v == 1) return 1;
    if (v == 2) return 2;
    return (s_player_num >= 5) ? 2 : 1;
}
const MutableGenericOptions k_default_generic_options;

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options,
                  const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    uint8_t player_num = generic_options_readonly.PlayerNum();
    s_player_num = player_num;
    if (player_num < 2) {
        reply() << "该游戏至少 2 人参加，当前玩家数为 " << player_num;
        return false;
    }
    return true;
}

constexpr PlayerID k_invalid_player_id = 99; // 用于表示无效玩家ID

const std::vector<RuleCommand> k_rule_commands = {
    RuleCommand("卡牌效果",
        [] () -> const char* {
            return "=== 标准版（2-4人，16张） ===\n"
                   "1. 卫兵(×5)：猜一名其他玩家的手牌（不能猜卫兵），猜对则其淘汰\n"
                   "2. 神父(×2)：查看一名其他玩家的手牌\n"
                   "3. 男爵(×2)：与一名其他玩家比手牌大小，小者淘汰，平局无事\n"
                   "4. 侍女(×2)：打出后直到下回合开始，其他玩家的卡牌不能指定你为目标\n"
                   "5. 王子(×2)：指定任意玩家弃掉手牌并摸一张新牌（若弃掉公主则淘汰）\n"
                   "6. 国王(×1)：与一名其他玩家交换手牌\n"
                   "7. 闺蜜(×1)：无效果，但若手中有国王或王子则必须打出\n"
                   "8. 公主(×1)：打出即淘汰\n"
                   "\n=== 豪华版（5-8人，追加16张） ===\n"
                   "0. 弄臣(×1)：指定一名玩家，若其获胜你获得1分\n"
                   "0. 刺客(×1)：被卫兵指定时反杀对方，打出后补抽1张\n"
                   "1. 卫兵(×3)：总计8张\n"
                   "2. 红衣(×2)：令两名玩家交换手牌，你可查看其中一人\n"
                   "3. 女爵(×2)：查看1-2名其他玩家的手牌\n"
                   "4. 谄媚(×2)：指定目标，下一张牌必须以此为目标\n"
                   "5. 伯爵(×2)：回合结束在弃牌堆中，手牌点数+1\n"
                   "6. 宪兵(×1)：被淘汰时在弃牌堆中，获得1分\n"
                   "7. 太后(×1)：比大小，点数更高者淘汰\n"
                   "9. 主教(×1)：猜测手牌点数，猜对获得1分";
        },
        VoidChecker("卡牌")),
};

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("设置游戏配置",
        [](CustomOptions& game_options, MutableGenericOptions& generic_options,
           const uint32_t& time_limit, const uint32_t& hearts, const uint32_t& deck_type)
        {
            GET_OPTION_VALUE(game_options, 时限) = time_limit;
            GET_OPTION_VALUE(game_options, 爱心) = hearts;
            GET_OPTION_VALUE(game_options, 牌堆) = deck_type;
            return NewGameMode::MULTIPLE_USERS;
        },
        OptionalDefaultChecker<ArithChecker<uint32_t>>((uint32_t)90, 10, 3600, "时限（秒）"),
        OptionalDefaultChecker<ArithChecker<uint32_t>>((uint32_t)0, 0, 10, "爱心"),
        OptionalDefaultChecker<AlterChecker<uint32_t>>((uint32_t)0, std::map<std::string, uint32_t>{{"默认", 0}, {"标准", 1}, {"豪华", 2}})),
};

// ========== Card definitions ==========
// 卡牌用名字字符串标识，CardPoint() 返回点数用于比大小
using CardId = const char*;

constexpr CardId GUARD    = "guard";
constexpr CardId PRIEST   = "priest";
constexpr CardId BARON    = "baron";
constexpr CardId HANDMAID = "handmaid";
constexpr CardId PRINCE   = "prince";
constexpr CardId KING     = "king";
constexpr CardId COUNTESS = "countess";
constexpr CardId PRINCESS = "princess";

constexpr CardId ASSASSIN  = "assassin";
constexpr CardId JESTER    = "jester";
constexpr CardId CARDINAL  = "cardinal";
constexpr CardId BARONESS  = "baroness";
constexpr CardId SYCOPHANT = "sycophant";
constexpr CardId COUNT     = "count";
constexpr CardId CONSTABLE = "constable";
constexpr CardId DOWAGER   = "dowager";
constexpr CardId BISHOP    = "bishop";

uint8_t CardPoint(const CardId c)
{
    if (c == GUARD)    return 1;
    if (c == PRIEST)   return 2;
    if (c == BARON)    return 3;
    if (c == HANDMAID) return 4;
    if (c == PRINCE)   return 5;
    if (c == KING)     return 6;
    if (c == COUNTESS) return 7;
    if (c == PRINCESS) return 8;

    if (c == ASSASSIN)  return 0;
    if (c == JESTER)    return 0;
    if (c == CARDINAL)  return 2;
    if (c == BARONESS)  return 3;
    if (c == SYCOPHANT) return 4;
    if (c == COUNT)     return 5;
    if (c == CONSTABLE) return 6;
    if (c == DOWAGER)   return 7;
    if (c == BISHOP)    return 9;
    return 0;
}

const char* CardName(const CardId c)
{
    if (c == GUARD)    return "卫兵(1)";
    if (c == PRIEST)   return "神父(2)";
    if (c == BARON)    return "男爵(3)";
    if (c == HANDMAID) return "侍女(4)";
    if (c == PRINCE)   return "王子(5)";
    if (c == KING)     return "国王(6)";
    if (c == COUNTESS) return "闺蜜(7)";
    if (c == PRINCESS) return "公主(8)";

    if (c == ASSASSIN)  return "刺客(0)";
    if (c == JESTER)    return "弄臣(0)";
    if (c == CARDINAL)  return "红衣(2)";
    if (c == BARONESS)  return "女爵(3)";
    if (c == SYCOPHANT) return "谄媚(4)";
    if (c == COUNT)     return "伯爵(5)";
    if (c == CONSTABLE) return "宪兵(6)";
    if (c == DOWAGER)   return "太后(7)";
    if (c == BISHOP)    return "主教(9)";
    return "未知";
}

std::vector<CardId> MakeDeck(const uint32_t player_num, const bool premium)
{
    std::vector<CardId> deck = {
        GUARD, GUARD, GUARD, GUARD, GUARD,
        PRIEST, PRIEST, BARON, BARON,
        HANDMAID, HANDMAID, PRINCE, PRINCE,
        KING, COUNTESS, PRINCESS
    };
    if (premium) {
        std::vector<CardId> premium = {
            ASSASSIN, JESTER,
            GUARD, GUARD, GUARD,
            CARDINAL, CARDINAL,
            BARONESS, BARONESS,
            SYCOPHANT, SYCOPHANT,
            COUNT, COUNT,
            CONSTABLE, DOWAGER, BISHOP
        };
        deck.insert(deck.end(), premium.begin(), premium.end());
    }
    return deck;
}

// ========== Player Info ==========
struct PlayerInfo {
    bool alive = true;
    bool protect = false;        // 侍女保护
    bool left = false;           // 永久退赛
    uint8_t scores = 0;          // 累计胜局数
    std::vector<CardId> hand;    // 手牌（1~2张）
    std::vector<CardId> discards; // 已打出的牌
    PlayerID jester_bet_ = k_invalid_player_id;   // 弄臣选择目标（99=无）
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

    bool IsPremium_() const {
        const auto v = GAME_OPTION(牌堆);
        if (v == 1) return false;
        if (v == 2) return true;
        return Global().PlayerNum() >= 5;
    }

    std::vector<PlayerInfo> player_infos_;
    std::vector<CardId> deck_;       // 牌堆
    CardId burn_card_ = nullptr;     // 面朝下移除的牌（王子摸空时使用）
    uint8_t TargetScore_() const
    {
        const auto opt = GAME_OPTION(爱心);
        if (opt > 0) return opt;
        switch (Global().PlayerNum()) {
        case 2: return 7;
        case 3: return 5;
        default: return 4;
        }
    }
    std::vector<CardId> removed_;    // 本局移除的牌（面朝上公示）
    uint8_t round_ = 0;
    PlayerID current_pid_ = 0;
    PlayerID first_player_ = 0;
    PlayerID sycophant_target_ = k_invalid_player_id; // 谄媚强制目标（99=无）
    PlayerID bishop_target_ = k_invalid_player_id;     // 主教猜中目标（99=无）

    std::string Hearts_(const uint8_t n) const
    {
        std::string h;
        for (uint8_t i = 0; i < n; ++i) h += "❤️";
        return h;
    }

    void BroadcastStatus_()
    {
        std::ostringstream ss;
        ss << "## 情书 第" << std::to_string(round_ + 1) << "局\n\n";
        ss << "目标：" << Hearts_(TargetScore_()) << "\n\n";
        ss << "| 玩家 | 得分 | 状态 | 弃牌堆 | 弃牌和 |\n";
        ss << "|------|------|------|--------|--------|\n";
        for (PlayerID i = 0; i < Global().PlayerNum(); ++i) {
            const auto& p = player_infos_[i];
            ss << "| " << Global().PlayerAvatar(i, 40) << std::to_string(i) << "." << Global().PlayerName(i)
                << " | " << Hearts_(p.scores) << " | ";
            bool taunted = (sycophant_target_ == i);
            ss << (p.alive ? (p.protect ? "🛡️保护" : (taunted ? "🎯嘲讽" : "🟢存活")) : "💀淘汰");
            ss << " | ";
            uint32_t sum = 0;
            for (auto c : p.discards) {
                if (c == JESTER || c == COUNT || c == CONSTABLE)
                    ss << HTML_COLOR_FONT_HEADER(red) << CardName(c) << HTML_FONT_TAIL << " ";
                else
                    ss << CardName(c) << " ";
                sum += CardPoint(c);
            }
            ss << " | " << sum << " |\n";
        }
        ss << "\n剩余牌堆：" << deck_.size() << " 张";
        if (!removed_.empty()) {
            ss << "\n\n已移除：";
            for (auto c : removed_) { ss << CardName(c) << " "; }
        }
        ss << "  \n当前行动：" << Global().PlayerName(current_pid_);

        CardEffectHelp_(ss);
        Global().Boardcast() << Markdown(ss.str(), 1000);
    }

    void CardEffectHelp_(std::ostringstream& ss) const
    {
        std::unordered_map<CardId, uint8_t> remain;
        for (auto c : deck_) ++remain[c];
        for (PlayerID i = 0; i < Global().PlayerNum(); ++i)
            for (auto c : player_infos_[i].hand) ++remain[c];
        if (burn_card_) ++remain[burn_card_];

        auto R = [&](CardId c) -> uint8_t { auto it = remain.find(c); return it != remain.end() ? it->second : 0; };

        const bool premium = IsPremium_();
        ss << "\n\n---\n**卡牌效果：**  \n";
        if (premium) {
            ss << "0\\. 弄臣(余" << std::to_string(R(JESTER)) << ")：指定一名玩家，若其获胜你得1分 —— 例：`弄臣 1`  \n";
            ss << "0\\. 刺客(余" << std::to_string(R(ASSASSIN)) << ")：被卫兵指定时反杀并重抽，主动打出无效果 —— 例：`刺客`  \n";
        }
        ss << "1\\. 卫兵(余" << std::to_string(R(GUARD)) << ")：猜一名其他玩家手牌数值（不能猜1），猜对则其淘汰 —— 例：`卫兵 1 2`  \n";
        if (premium) {
            ss << "2\\. 神父(余" << std::to_string(R(PRIEST)) << ")：查看一名其他玩家的手牌 —— 例：`神父 1`  \n";
            ss << "2\\. 红衣(余" << std::to_string(R(CARDINAL)) << ")：令两名玩家交换手牌，可查看一人（默认查看第一个目标交换后的手牌，第一个目标不能选自己） —— 例：`红衣 1 2`  \n";
            ss << "3\\. 男爵(余" << std::to_string(R(BARON)) << ")：与一名其他玩家比大小，小者淘汰 —— 例：`男爵 0`  \n";
            ss << "3\\. 女爵(余" << std::to_string(R(BARONESS)) << ")：查看1-2名玩家手牌 —— 例：`女爵 1` 或 `女爵 1 2`  \n";
            ss << "4\\. 侍女(余" << std::to_string(R(HANDMAID)) << ")：打出后其他玩家直到下回合不能指定你为目标 —— 例：`侍女`  \n";
            ss << "4\\. 谄媚(余" << std::to_string(R(SYCOPHANT)) << ")：指定目标，下一张牌必须以该玩家为唯一目标 —— 例：`谄媚 1`  \n";
            ss << "5\\. 王子(余" << std::to_string(R(PRINCE)) << ")：弃牌重抽（弃公主淘汰） —— 例：`王子 2`  \n";
            ss << "5\\. 伯爵(余" << std::to_string(R(COUNT)) << ")：弃牌堆中有此牌，本局结束时手牌点数+1 —— 例：`伯爵`  \n";
            ss << "6\\. 国王(余" << std::to_string(R(KING)) << ")：与一名其他玩家交换手牌 —— 例：`国王 1`  \n";
            ss << "6\\. 宪兵(余" << std::to_string(R(CONSTABLE)) << ")：被淘汰时在弃牌堆中，获得1分 —— 例：`宪兵`  \n";
            ss << "7\\. 闺蜜(余" << std::to_string(R(COUNTESS)) << ")：手中有国王/王子时须打出 —— 例：`闺蜜`  \n";
            ss << "7\\. 太后(余" << std::to_string(R(DOWAGER)) << ")：比大小，点数较高者淘汰 —— 例：`太后 1`  \n";
            ss << "8\\. 公主(余" << std::to_string(R(PRINCESS)) << ")：打出即淘汰 —— 例：`公主`  \n";
            ss << "9\\. 主教(余" << std::to_string(R(BISHOP)) << ")：猜手牌点数，猜对得1分 —— 例：`主教 1 3`";
        } else {
            ss << "2\\. 神父(余" << std::to_string(R(PRIEST)) << ")：查看一名其他玩家的手牌 —— 例：`神父 1`  \n";
            ss << "3\\. 男爵(余" << std::to_string(R(BARON)) << ")：与一名其他玩家比大小，小者淘汰 —— 例：`男爵 0`  \n";
            ss << "4\\. 侍女(余" << std::to_string(R(HANDMAID)) << ")：打出后其他玩家直到下回合不能指定你为目标 —— 例：`侍女`  \n";
            ss << "5\\. 王子(余" << std::to_string(R(PRINCE)) << ")：指定任意玩家弃牌重抽（弃掉公主则淘汰） —— 例：`王子 2`  \n";
            ss << "6\\. 国王(余" << std::to_string(R(KING)) << ")：与一名其他玩家交换手牌 —— 例：`国王 1`  \n";
            ss << "7\\. 闺蜜(余" << std::to_string(R(COUNTESS)) << ")：无效果（手中有国王/王子则必须打出） —— 例：`闺蜜`  \n";
            ss << "8\\. 公主(余" << std::to_string(R(PRINCESS)) << ")：打出即淘汰 —— 例：`公主`";
        }
    }

  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        BroadcastStatus_();
        return StageErrCode::OK;
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
                                    AlterChecker<CardId>({{"卫兵", GUARD}, {"神父", PRIEST}, {"男爵", BARON}, {"侍女", HANDMAID}, {"王子", PRINCE}, {"国王", KING}, {"闺蜜", COUNTESS}, {"公主", PRINCESS}, {"弄臣", JESTER}, {"刺客", ASSASSIN}, {"红衣", CARDINAL}, {"女爵", BARONESS}, {"谄媚", SYCOPHANT}, {"伯爵", COUNT}, {"宪兵", CONSTABLE}, {"太后", DOWAGER}, {"主教", BISHOP}}),
                                    OptionalChecker<ArithChecker<uint32_t>>(0, 9, "目标玩家ID"),
                                    OptionalChecker<ArithChecker<uint32_t>>(0, 9, "猜测点数/目标2")),
                   MakeStageCommand(*this, "换牌",
                                    CommandFlag::UNREADY_ONLY,
                                    &RoundStage::BishopChoice_,
                                    AlterChecker<bool>({{"y", true}, {"n", false}})))
    {}

    virtual void OnStageBegin() override
    {
        Main().deck_ = MakeDeck(Global().PlayerNum(), Main().IsPremium_());
        Main().removed_.clear();
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(Main().deck_.begin(), Main().deck_.end(), g);

        if (!Main().deck_.empty()) {
            Main().burn_card_ = Main().deck_.back();
            Main().deck_.pop_back();
        }

        if (Global().PlayerNum() == 2) {
            for (int i = 0; i < 3 && !Main().deck_.empty(); ++i) {
                Main().removed_.push_back(Main().deck_.back());
                Main().deck_.pop_back();
            }
            std::string msg = "2人局移除并公示的3张牌：";
            for (auto c : Main().removed_) { msg += CardName(c); msg += " "; }
            Global().Boardcast() << msg;
        }

        Main().bishop_target_ = k_invalid_player_id;
        for (PlayerID i = 0; i < Global().PlayerNum(); ++i) {
            auto& p = Main().player_infos_[i];
            p.protect = false;
            p.hand.clear();
            p.discards.clear();
            p.jester_bet_ = k_invalid_player_id;
            if (p.left) continue;
            p.alive = true;
            if (!Main().deck_.empty()) {
                p.hand.push_back(Main().deck_.back());
                Main().deck_.pop_back();
                Global().Tell(i) << "你的初始手牌是 " << CardName(p.hand.back());
            }
            Global().SetReady(i);
        }
        Main().current_pid_ = Main().first_player_;

        DrawAndNotify_(Main().current_pid_);
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        if (Main().bishop_target_ != k_invalid_player_id) {
            const PlayerID bt = Main().bishop_target_;
            Main().bishop_target_ = k_invalid_player_id;
            Global().Boardcast() << At(bt) << " 超时未选择，默认保留手牌";
            Global().SetReady(bt);
            return AfterPlay_();
        }
        Global().Boardcast() << At(Main().current_pid_) << " 超时，自动打出第一张手牌";
        ForcePlay_(Main().current_pid_);
        if (Main().bishop_target_ != k_invalid_player_id) return StageErrCode::CONTINUE; // 等主教目标选择
        return AfterPlay_();
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        auto& p = Main().player_infos_[pid];
        if (p.alive) {
            p.left = true;
            Global().Boardcast() << At(pid) << " 退出游戏，被淘汰";
            Eliminate_(pid);
            p.alive = false;
            if (Main().bishop_target_ == pid) Main().bishop_target_ = k_invalid_player_id;
            if (Main().sycophant_target_ == pid) Main().sycophant_target_ = k_invalid_player_id;
            Global().Eliminate(pid);
        }
        // 只剩1人或当前玩家离开：推进回合/结束本局
        int alive_cnt = 0;
        for (PlayerID i = 0; i < Global().PlayerNum(); ++i)
            if (Main().player_infos_[i].alive) ++alive_cnt;
        if (alive_cnt <= 1 || pid == Main().current_pid_) return AfterPlay_();
        return StageErrCode::CONTINUE;
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
    AtomReqErrCode Play_(const PlayerID pid, const bool is_public, MsgSenderBase& reply,
                         const CardId card,
                         const std::optional<uint32_t>& target_opt,
                         const std::optional<uint32_t>& extra_opt)
    {
        if (pid != Main().current_pid_) {
            Global().Tell(pid) << "[错误] 还没轮到你的回合";
            return StageErrCode::FAILED;
        }
        const PlayerID target = target_opt.has_value()
            ? PlayerID{static_cast<uint32_t>(*target_opt)} : PlayerID{static_cast<uint32_t>(k_invalid_player_id)};
        const uint32_t extra = extra_opt.has_value() ? *extra_opt : static_cast<uint32_t>(k_invalid_player_id);

        auto& p = Main().player_infos_[pid];

        // 手牌 + 规则校验
        auto it = std::find(p.hand.begin(), p.hand.end(), card);
        if (it == p.hand.end()) {
            std::string hs;
            for (auto c : p.hand) hs += CardName(c) + std::string(" ");
            Global().Tell(pid) << "[错误] 你手中没有这张牌，当前手牌：" << hs;
            return StageErrCode::FAILED;
        }
        if (MustPlayCountess_(p.hand, card)) {
            Global().Tell(pid) << "[错误] 你同时持有闺蜜和国王/王子，必须打出闺蜜(7)";
            return StageErrCode::FAILED;
        }

        // 弃牌
        p.hand.erase(it);
        p.discards.push_back(card);

        const auto resolve_rc = ResolveCard_(pid, card, target, extra);
        if (resolve_rc != StageErrCode::OK && resolve_rc != StageErrCode::CHECKOUT) {
            // 校验失败，恢复手牌
            p.discards.pop_back();
            p.hand.push_back(card);
            return resolve_rc;
        }
        if (card != SYCOPHANT) Main().sycophant_target_ = k_invalid_player_id; // 打出成功，消耗谄媚效果
        if (Main().bishop_target_ != k_invalid_player_id) return StageErrCode::OK; // 等待目标选择换牌
        return AfterPlay_();
    }

    // -------- 共享校验 --------
    AtomReqErrCode ValidateTarget_(PlayerID pid, CardId card, PlayerID target)
    {
        const bool self_ok = CardSelfOK(card);
        const PlayerID sycophant = Main().sycophant_target_;
        bool must_null = false;
        if (sycophant != k_invalid_player_id) {
            if (card == CARDINAL) {
                must_null = true; // 无合法目标，允许空发
            } else if (Main().player_infos_[sycophant].protect) {
                must_null = true; // 无合法目标，允许空发
            } else if (!self_ok && sycophant == pid) {
                must_null = true; // 无合法目标，允许空发
            } else if (target != sycophant) {
                Global().Tell(pid) << "[错误] 谄媚效果：你必须以 " << At(sycophant) << "为唯一目标";
                return StageErrCode::FAILED;
            }
        } else {
            must_null = true;
            for (PlayerID i = 0; i < Global().PlayerNum(); ++i) {
                if (i == pid && !self_ok) continue;
                if (Main().player_infos_[i].alive && !Main().player_infos_[i].protect) {
                    must_null = false;
                    break;
                }
            }
        }
        if (must_null) {
            if (target == k_invalid_player_id) {
                return StageErrCode::OK;
            }
            Global().Tell(pid) << "[错误] 你没有合法目标，不能指定目标";
            return StageErrCode::FAILED;
        }
        if (target >= Global().PlayerNum() || (!self_ok && target == pid) ||
            !Main().player_infos_[target].alive || Main().player_infos_[target].protect) {
            Global().Tell(pid) << "[错误] 目标无效";
            return StageErrCode::FAILED;
        }
        return StageErrCode::OK;
    }

    // -------- 卡牌效果分发 --------
    using CardFunc = AtomReqErrCode (RoundStage::*)(PlayerID, CardId, PlayerID, uint32_t);

    AtomReqErrCode ResolveCard_(const PlayerID pid, const CardId card,
                                const PlayerID target, const uint32_t extra)
    {
        static const std::unordered_map<CardId, CardFunc> k_map = {
            {GUARD,     &RoundStage::DoGuard_},
            {PRIEST,    &RoundStage::DoPriest_},
            {BARON,     &RoundStage::DoBaron_},
            {HANDMAID,  &RoundStage::DoHandmaid_},
            {PRINCE,    &RoundStage::DoPrince_},
            {KING,      &RoundStage::DoKing_},
            {COUNTESS,  &RoundStage::DoCountess_},
            {PRINCESS,  &RoundStage::DoPrincess_},

            {JESTER,    &RoundStage::DoJester_},
            {ASSASSIN,  &RoundStage::DoAssassin_},
            {CARDINAL,  &RoundStage::DoCardinal_},
            {BARONESS,  &RoundStage::DoBaroness_},
            {SYCOPHANT, &RoundStage::DoSycophant_},
            {COUNT,     &RoundStage::DoCount_},
            {CONSTABLE, &RoundStage::DoConstable_},
            {DOWAGER,   &RoundStage::DoDowager_},
            {BISHOP,    &RoundStage::DoBishop_},
        };
        auto it = k_map.find(card);
        if (it == k_map.end()) return StageErrCode::FAILED;
        return (this->*it->second)(pid, card, target, extra);
    }

    // ======== 标准版卡牌效果 ========
    AtomReqErrCode DoGuard_(PlayerID pid, CardId card, PlayerID target, uint32_t extra)
    {
        if (auto rc = ValidateTarget_(pid, card, target); rc != StageErrCode::OK) return rc;
        if (target >= Global().PlayerNum()) {
            Global().Boardcast() << At(pid) << " 打出 " << CardName(card) << "（无有效目标，空发）";
            return StageErrCode::OK;
        }
        auto& ps = Main().player_infos_;
        if (extra == 1 || extra > 9) {
            Global().Tell(pid) << "[错误] 猜测点数无效（不能猜1，范围0-9）";
            return StageErrCode::FAILED;
        }

        // 刺客反制：目标手牌中有刺客 → 攻击者淘汰，目标弃刺客+补牌
        auto it_assassin = std::find(ps[target].hand.begin(), ps[target].hand.end(), ASSASSIN);
        if (it_assassin != ps[target].hand.end()) {
            Global().Boardcast() << At(pid) << " 打出卫兵，但 " << At(target) << " 手中有刺客，反杀！";
            Eliminate_(pid);
            Global().Boardcast() << At(target) << "弃掉刺客并重新抽牌";
            // 目标弃掉刺客并补牌
            ps[target].hand.erase(it_assassin);
            ps[target].discards.push_back(ASSASSIN);
            DrawCard_(target);
            return StageErrCode::OK;
        }

        Global().Boardcast() << At(pid) << " 打出卫兵，猜测 "
            << At(target) << " 的手牌点数 " << std::to_string(extra);
        if (!ps[target].hand.empty() && CardPoint(ps[target].hand[0]) == extra) {
            Eliminate_(target);
            Global().Boardcast() << "猜测正确！" << At(target) << " 被淘汰";
        } else {
            Global().Boardcast() << "猜测错误！";
        }
        return StageErrCode::OK;
    }

    AtomReqErrCode DoPriest_(PlayerID pid, CardId card, PlayerID target, uint32_t)
    {
        if (auto rc = ValidateTarget_(pid, card, target); rc != StageErrCode::OK) return rc;
        if (target >= Global().PlayerNum()) {
            Global().Boardcast() << At(pid) << " 打出 " << CardName(card) << "（无有效目标，空发）";
            return StageErrCode::OK;
        }
        auto& ps = Main().player_infos_;
        std::string hs;
        for (auto c : ps[target].hand) { hs += CardName(c); hs += " "; }
        Global().Boardcast() << At(pid) << " 打出神父，查看了 " << At(target) << " 的手牌";
        Global().Tell(pid) << At(target) << " 的手牌：" << hs;
        return StageErrCode::OK;
    }

    AtomReqErrCode DoBaron_(PlayerID pid, CardId card, PlayerID target, uint32_t)
    {
        if (auto rc = ValidateTarget_(pid, card, target); rc != StageErrCode::OK) return rc;
        if (target >= Global().PlayerNum()) {
            Global().Boardcast() << At(pid) << " 打出 " << CardName(card) << "（无有效目标，空发）";
            return StageErrCode::OK;
        }
        auto& ps = Main().player_infos_;
        const uint8_t a = ps[pid].hand.empty() ? 0 : CardPoint(ps[pid].hand[0]);
        const uint8_t b = ps[target].hand.empty() ? 0 : CardPoint(ps[target].hand[0]);
        Global().Boardcast() << At(pid) << " 打出男爵，与 " << At(target) << " 决斗！";
        Global().Tell(pid) << At(target) << " 的手牌是 " << CardName(ps[target].hand[0]);
        Global().Tell(target) << At(pid) << " 的手牌是 " << CardName(ps[pid].hand[0]);
        if (a > b) {
            Global().Boardcast() << At(target) << "的手牌为 " << CardName(ps[target].hand[0]) << "，点数较小，被淘汰";
            Eliminate_(target);
        } else if (a < b) {
            Global().Boardcast() << At(pid) << " 的手牌为 " << CardName(ps[pid].hand[0]) << "，点数较小，被淘汰";
            Eliminate_(pid);
        } else {
            Global().Boardcast() << "点数相同，平局！";
        }
        return StageErrCode::OK;
    }

    AtomReqErrCode DoHandmaid_(PlayerID pid, CardId, PlayerID, uint32_t)
    {
        Main().player_infos_[pid].protect = true;
        Global().Boardcast() << At(pid) << " 打出侍女，直到下回合开始不能被其他玩家卡牌指定";
        return StageErrCode::OK;
    }

    AtomReqErrCode DoPrince_(PlayerID pid, CardId card, PlayerID target, uint32_t)
    {
        if (auto rc = ValidateTarget_(pid, card, target); rc != StageErrCode::OK) return rc;
        auto& ps = Main().player_infos_;
        if (!ps[target].hand.empty()) {
            const CardId discarded = ps[target].hand.back();
            ps[target].hand.pop_back();
            ps[target].discards.push_back(discarded);
            Global().Boardcast() << At(pid) << " 对 " << At(target) << " 使用王子，弃掉 " << CardName(discarded);
            if (discarded == PRINCESS) {
                Global().Boardcast() << At(target) << " 弃掉了公主，被淘汰！";
                Eliminate_(target);
                return StageErrCode::OK;
            }
        }
        DrawCard_(target);
        return StageErrCode::OK;
    }

    AtomReqErrCode DoKing_(PlayerID pid, CardId card, PlayerID target, uint32_t)
    {
        if (auto rc = ValidateTarget_(pid, card, target); rc != StageErrCode::OK) return rc;
        if (target >= Global().PlayerNum()) {
            Global().Boardcast() << At(pid) << " 打出 " << CardName(card) << "（无有效目标，空发）";
            return StageErrCode::OK;
        }
        auto& ps = Main().player_infos_;
        std::swap(ps[pid].hand, ps[target].hand);
        Global().Boardcast() << At(pid) << " 打出了国王，与 " << At(target) << " 交换了手牌";
        if (!ps[pid].hand.empty())
            Global().Tell(pid) << "你从 " << At(target) << " 那里得到了新手牌：" << CardName(ps[pid].hand[0]);
        if (!ps[target].hand.empty())
            Global().Tell(target) << "你从 " << At(pid) << " 那里得到了新手牌：" << CardName(ps[target].hand[0]);
        return StageErrCode::OK;
    }

    AtomReqErrCode DoCountess_(PlayerID pid, CardId, PlayerID, uint32_t)
    {
        Global().Boardcast() << At(pid) << " 打出了闺蜜（无效果）";
        return StageErrCode::OK;
    }

    AtomReqErrCode DoPrincess_(PlayerID pid, CardId, PlayerID, uint32_t)
    {
        Global().Boardcast() << At(pid) << " 打出了公主，被淘汰！";
        Eliminate_(pid);
        return StageErrCode::OK;
    }

    // ======== 豪华版卡牌效果 ========
    AtomReqErrCode DoJester_(PlayerID pid, CardId card, PlayerID target, uint32_t)
    {
        if (auto rc = ValidateTarget_(pid, card, target); rc != StageErrCode::OK) return rc;
        Main().player_infos_[pid].jester_bet_ = target;
        Global().Boardcast() << At(pid) << " 打出弄臣，选择 " << At(target) << " 获胜！";
        return StageErrCode::OK;
    }

    AtomReqErrCode DoAssassin_(PlayerID pid, CardId, PlayerID, uint32_t)
    {
        Global().Boardcast() << At(pid) << " 打出刺客（无效果）";
        return StageErrCode::OK;
    }

    AtomReqErrCode DoCardinal_(PlayerID pid, CardId card, PlayerID target, uint32_t extra)
    {
        if (auto rc = ValidateTarget_(pid, card, target); rc != StageErrCode::OK) return rc;
        if (target == k_invalid_player_id) {
            Global().Boardcast() << At(pid) << " 打出 " << CardName(card) << "（需要两个目标，空发）";
            return StageErrCode::OK;
        }
        const PlayerID t2 = PlayerID{extra};
        if (t2 == target || t2 >= Global().PlayerNum() ||
            !Main().player_infos_[t2].alive || Main().player_infos_[t2].protect) {
            Global().Tell(pid) << "[错误] 第二目标无效";
            return StageErrCode::FAILED;
        }
        auto& ps = Main().player_infos_;
        std::swap(ps[target].hand, ps[t2].hand);
        Global().Boardcast() << At(pid) << " 打出红衣，令 " << At(target)
            << " 和 " << At(t2) << " 交换了手牌";
        if (!ps[target].hand.empty())
            Global().Tell(target) << "你从 " << At(pid) << " 那里得到了新手牌：" << CardName(ps[target].hand[0]);
        if (!ps[t2].hand.empty())
            Global().Tell(t2) << "你从 " << At(pid) << " 那里得到了新手牌：" << CardName(ps[t2].hand[0]);
        // 红衣玩家可查看其中一人
        if (!ps[target].hand.empty())
            Global().Tell(pid) << At(target) << " 的手牌：" << CardName(ps[target].hand[0]);
        return StageErrCode::OK;
    }

    AtomReqErrCode DoBaroness_(PlayerID pid, CardId card, PlayerID target, uint32_t extra)
    {
        if (target == extra) {
            Global().Tell(pid) << "[错误] 目标1和目标2不能相同";
            return StageErrCode::FAILED;
        }
        if (auto rc = ValidateTarget_(pid, card, target); rc != StageErrCode::OK) return rc;
        auto& ps = Main().player_infos_;
        const PlayerID t2 = PlayerID{extra};
        if (t2 != k_invalid_player_id && (Main().sycophant_target_ != k_invalid_player_id ||
            t2 >= Global().PlayerNum() || !Main().player_infos_[t2].alive || Main().player_infos_[t2].protect)) {
            Global().Tell(pid) << "[错误] 第二目标无效";
            return StageErrCode::FAILED;
        }
        if (t2 != k_invalid_player_id)
            Global().Boardcast() << At(pid) << " 打出女爵，查看了 " << At(target) << " 和 " << At(t2) << " 的手牌";
        else
            Global().Boardcast() << At(pid) << " 打出女爵，查看了 " << At(target) << " 的手牌";
        auto show = [&](PlayerID t) {
            std::string hs;
            for (auto c : ps[t].hand) hs += CardName(c) + std::string(" ");
            Global().Tell(pid) << At(t) << " 的手牌：" << hs;
        };
        show(target);
        if (t2 != k_invalid_player_id) show(t2);
        return StageErrCode::OK;
    }

    AtomReqErrCode DoSycophant_(PlayerID pid, CardId card, PlayerID target, uint32_t)
    {
        if (auto rc = ValidateTarget_(pid, card, target); rc != StageErrCode::OK) return rc;
        Main().sycophant_target_ = target;
        Global().Boardcast() << At(pid) << " 打出谄媚，下一张牌的第一个目标（如果有）必须是 " << At(target);
        return StageErrCode::OK;
    }

    AtomReqErrCode DoCount_(PlayerID pid, CardId, PlayerID, uint32_t)
    {
        Global().Boardcast() << At(pid) << " 打出伯爵（若在弃牌堆中，本局结束手牌点数+1）";
        return StageErrCode::OK;
    }

    AtomReqErrCode DoConstable_(PlayerID pid, CardId, PlayerID, uint32_t)
    {
        Global().Boardcast() << At(pid) << " 打出宪兵（被淘汰时若在弃牌堆中，获得1❤）";
        return StageErrCode::OK;
    }

    AtomReqErrCode DoDowager_(PlayerID pid, CardId card, PlayerID target, uint32_t)
    {
        if (auto rc = ValidateTarget_(pid, card, target); rc != StageErrCode::OK) return rc;
        if (target >= Global().PlayerNum()) {
            Global().Boardcast() << At(pid) << " 打出 " << CardName(card) << "（无有效目标，空发）";
            return StageErrCode::OK;
        }
        auto& ps = Main().player_infos_;
        const uint8_t a = ps[pid].hand.empty() ? 0 : CardPoint(ps[pid].hand[0]);
        const uint8_t b = ps[target].hand.empty() ? 0 : CardPoint(ps[target].hand[0]);
        Global().Boardcast() << At(pid) << " 打出太后，与 " << At(target) << " 比拼（高者淘汰）！";
        Global().Tell(pid) << At(target) << " 的手牌是 " << CardName(ps[target].hand[0]);
        Global().Tell(target) << At(pid) << " 的手牌是 " << CardName(ps[pid].hand[0]);
        if (a > b) {
            Global().Boardcast() << At(pid) << " 的手牌为 " << CardName(ps[pid].hand[0]) << "，点数较大，被淘汰";
            Eliminate_(pid);
        } else if (a < b) {
            Global().Boardcast() << At(target) << " 的手牌为 " << CardName(ps[target].hand[0]) << "，点数较大，被淘汰";
            Eliminate_(target);
        } else {
            Global().Boardcast() << "点数相同，平局！";
        }
        return StageErrCode::OK;
    }

    AtomReqErrCode DoBishop_(PlayerID pid, CardId card, PlayerID target, uint32_t extra)
    {
        if (auto rc = ValidateTarget_(pid, card, target); rc != StageErrCode::OK) return rc;
        if (target >= Global().PlayerNum()) {
            Global().Boardcast() << At(pid) << " 打出 " << CardName(card) << "（无有效目标，空发）";
            return StageErrCode::OK;
        }
        if (extra > 9) {
            Global().Tell(pid) << "[错误] 猜测点数无效（范围0-9）";
            return StageErrCode::FAILED;
        }
        auto& ps = Main().player_infos_;
        Global().Boardcast() << At(pid) << " 打出主教，猜测 " << At(target)
            << " 的手牌点数是 " << std::to_string(extra);
        if (!ps[target].hand.empty() && CardPoint(ps[target].hand[0]) == extra) {
            ps[pid].scores++;
            Global().Boardcast() << "猜测正确！" << At(pid) << " 获得1分！\n"
                << At(target) << "输入 `y` 弃牌重抽，`n` 保留";
            Main().bishop_target_ = target;
            Global().SetReady(pid);
            Global().ClearReady(target);
            Global().StartTimer(GAME_OPTION(时限));
        } else {
            Global().Boardcast() << "猜测错误！";
        }
        return StageErrCode::OK;
    }

    AtomReqErrCode BishopChoice_(PlayerID pid, const bool is_public, MsgSenderBase& reply, bool choice)
    {
        if (pid != Main().bishop_target_) {
            Global().Tell(pid) << "[错误] 当前没有需要你做选择";
            return StageErrCode::FAILED;
        }
        auto& p = Main().player_infos_[pid];
        if (choice) {
            Global().Boardcast() << At(pid) << " 选择弃掉 " << CardName(p.hand[0]) << " 重抽一张";
            const CardId discarded = p.hand[0];
            p.discards.push_back(discarded);
            p.hand.clear();
            if (discarded == PRINCESS) {
                Global().Boardcast() << At(pid) << " 弃掉了公主，被淘汰！";
                Eliminate_(pid);
            } else {
                DrawCard_(pid);
            }
        } else {
            Global().Boardcast() << At(pid) << " 选择保留手牌 " << CardName(p.hand[0]);
        }
        Main().bishop_target_ = k_invalid_player_id;
        Global().SetReady(pid);
        const auto rc = AfterPlay_();
        if (rc == StageErrCode::CHECKOUT) return StageErrCode::CHECKOUT;
        return StageErrCode::OK;
    }

    // -------- 辅助函数 --------
    void DrawCard_(const PlayerID target)
    {
        auto& deck = Main().deck_;
        if (!deck.empty()) {
            Main().player_infos_[target].hand.push_back(deck.back());
            deck.pop_back();
            Global().Tell(target) << "你抽到了 " << CardName(Main().player_infos_[target].hand.back());
        } else {
            Main().player_infos_[target].hand.push_back(Main().burn_card_);
            Global().Tell(target) << "牌堆已空，你拿取了暗扣的牌：" << CardName(Main().burn_card_);
            Main().burn_card_ = nullptr;
        }
    }

    bool CardSelfOK(const CardId card)
    {
        return card == PRINCE || card == JESTER || card == BARONESS || card == SYCOPHANT;
    }

    static bool MustPlayCountess_(const std::vector<CardId>& hand, CardId just_played)
    {
        if (just_played == COUNTESS) return false;
        bool has_c = false, has_kp = false;
        for (auto c : hand) {
            if (c == COUNTESS) has_c = true;
            if (c == KING || c == PRINCE) has_kp = true;
        }
        return has_c && has_kp;
    }

    void Eliminate_(const PlayerID pid)
    {
        auto& p = Main().player_infos_[pid];
        if (!p.alive) return;
        for (auto c : p.hand) p.discards.push_back(c);
        p.hand.clear();
        p.alive = false;

        // 宪兵：被淘汰时在弃牌堆中，获得1分
        if ((!p.left) && std::find(p.discards.begin(), p.discards.end(), CONSTABLE) != p.discards.end()) {
            p.scores++;
            Global().Boardcast() << At(pid) << " 弃牌堆中有宪兵，获得1分！";
        }
    }

    void DrawAndNotify_(const PlayerID pid)
    {
        auto& p = Main().player_infos_[pid];
        auto& deck = Main().deck_;

        if (!deck.empty()) {
            p.hand.push_back(deck.back());
            deck.pop_back();
        }

        p.protect = false;

        std::string hs;
        for (auto c : p.hand) hs += CardName(c) + std::string(" ");
        Global().Tell(pid) << "轮到你了！你的手牌：" << hs;
        Global().Boardcast() << "轮到 " << At(pid) << " 行动";
        Global().ClearReady(pid);
        Main().BroadcastStatus_();
        Global().StartTimer(GAME_OPTION(时限));
    }

    void ForcePlay_(const PlayerID pid)
    {
        auto& p = Main().player_infos_[pid];
        if (p.hand.empty()) return;

        // 按闺蜜规则选主牌，另一张为备选
        const CardId card = MustPlayCountess_(p.hand, p.hand[0]) ? COUNTESS : p.hand[0];

        // 无目标牌直接打出
        if (card == HANDMAID || card == COUNTESS || card == PRINCESS ||
            card == ASSASSIN || card == COUNT || card == CONSTABLE) {
            auto it = std::find(p.hand.begin(), p.hand.end(), card);
            p.hand.erase(it);
            p.discards.push_back(card);
            ResolveCard_(pid, card, k_invalid_player_id, k_invalid_player_id);
            Main().sycophant_target_ = k_invalid_player_id;
            return;
        }

        PlayerID target = k_invalid_player_id;
        if (Main().sycophant_target_ != k_invalid_player_id) {
            target = Main().sycophant_target_;
        } else if (CardSelfOK(card)) {
            target = pid;
        } else {
            for (PlayerID t = 0; t < Global().PlayerNum(); ++t) {
                if (t == pid) continue;
                if (!Main().player_infos_[t].alive || Main().player_infos_[t].protect) continue;
                target = t;
                break;
            }
        }

        auto it = std::find(p.hand.begin(), p.hand.end(), card);
        p.hand.erase(it);
        p.discards.push_back(card);

        uint32_t extra = k_invalid_player_id;
        if (card == GUARD || card == BISHOP) {
            extra = 0;
        } else if (card == CARDINAL) {
            extra = pid;
        }
        ResolveCard_(pid, card, target, extra);
        if (card != SYCOPHANT) Main().sycophant_target_ = k_invalid_player_id;
        return;
    }

    CheckoutErrCode AfterPlay_()
    {
        auto& ps = Main().player_infos_;
        auto& deck = Main().deck_;
        const PlayerID pn = Global().PlayerNum();

        // 检查是否有人达到目标分数（主教/宪兵可能在局内得分）
        const uint8_t target_score = Main().TargetScore_();
        for (PlayerID i = 0; i < pn; ++i) {
            if (ps[i].scores >= target_score)
                return StageErrCode::CHECKOUT; // 由 NextStageFsm 统一宣布游戏结束
        }

        std::vector<PlayerID> alive;
        for (PlayerID i = 0; i < pn; ++i)
            if (ps[i].alive) alive.push_back(i);

        if (alive.size() <= 1 || deck.empty()) {
            PlayerID winner = 0;

            if (alive.size() == 1) {
                winner = alive[0];
                Global().Boardcast() << At(winner) << " 是唯一幸存者，赢得本局！";
            } else {
                // 比手牌点数（伯爵效果：弃牌堆有伯爵则+1）
                uint8_t high = 0;
                uint8_t effective_val[8] = {};
                for (auto pid : alive) {
                    uint8_t v = ps[pid].hand.empty() ? 0 : CardPoint(ps[pid].hand[0]);
                    if (std::find(ps[pid].discards.begin(), ps[pid].discards.end(), COUNT) != ps[pid].discards.end()) {
                        Global().Boardcast() << At(pid) << " 弃牌堆中有伯爵，手牌点数+1";
                        ++v;
                    }
                    effective_val[pid] = v;
                    if (v > high) high = v;
                    Global().Boardcast() << At(pid) << " 的手牌：" << CardName(ps[pid].hand[0]) << "，有效点数：" << std::to_string(v);
                }
                std::vector<PlayerID> tied;
                for (auto pid : alive)
                    if (effective_val[pid] == high) tied.push_back(pid);

                if (tied.size() == 1) {
                    winner = tied[0];
                    Global().Boardcast() << "牌堆耗尽！" << At(winner)
                        << " 手牌点数最大（" << std::to_string(high) << "），赢得本局！";
                } else {
                    uint32_t best_sum = 0;
                    for (auto pid : tied) {
                        uint32_t sum = 0;
                        for (auto c : ps[pid].discards) sum += CardPoint(c);
                        if (sum > best_sum) { best_sum = sum; winner = pid; }
                    }
                    Global().Boardcast() << "牌堆耗尽且手牌平局！弃牌堆总和最大的是 "
                        << At(winner) << "，赢得本局！";
                }
            }

            for (auto pid : alive)
                if (!ps[pid].hand.empty())
                    Global().Boardcast() << At(pid) << " 的手牌：" << CardName(ps[pid].hand[0]);

            ps[winner].scores++;

            // 获胜者达到目标分数则直接结束，不触发弄臣
            if (ps[winner].scores >= target_score) {
                Main().first_player_ = winner;
                Main().BroadcastStatus_();
                return StageErrCode::CHECKOUT;
            }

            // 弄臣效果
            for (PlayerID i = 0; i < pn; ++i) {
                if (Main().player_infos_[i].jester_bet_ != k_invalid_player_id &&
                    Main().player_infos_[i].jester_bet_ == winner) {
                    Main().player_infos_[i].scores++;
                    Global().Boardcast() << At(i) << " 的弄臣选择正确，获得1❤！";
                }
            }
            Main().first_player_ = winner;
            Main().BroadcastStatus_();
            return StageErrCode::CHECKOUT;
        }

        Global().SetReady(Main().current_pid_);
        Main().current_pid_ = (Main().current_pid_ + 1) % pn;
        while (!ps[Main().current_pid_].alive)
            Main().current_pid_ = (Main().current_pid_ + 1) % pn;

        DrawAndNotify_(Main().current_pid_);
        return StageErrCode::CONTINUE;
    }
};

// ========== MainStage 阶段切换 ==========
void MainStage::FirstStageFsm(SubStageFsmSetter setter)
{
    uint8_t player_num = Global().PlayerNum();
    if (IsPremium_()) {
        Global().Boardcast() << "本局玩家数为 " << player_num << "，使用情书豪华版牌堆（32张），获胜分数为 " << std::to_string(TargetScore_()) << "❤";
    } else {
        Global().Boardcast() << "本局玩家数为 " << player_num << "，使用情书标准版牌堆（16张），获胜分数为 " << std::to_string(TargetScore_()) << "❤";
    }
    setter.Emplace<RoundStage>(*this);
}

void MainStage::NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    const uint8_t target = TargetScore_();
    for (PlayerID i = 0; i < Global().PlayerNum(); ++i) {
        if (player_infos_[i].scores >= target) {
            Global().Boardcast() << "游戏结束！" << At(i)
                << " 率先获得 " << Hearts_(target) << "，赢得比赛！";
            BroadcastStatus_();
            return;
        }
    }
    ++round_;
    setter.Emplace<RoundStage>(*this);
}

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot
