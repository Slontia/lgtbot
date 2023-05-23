// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <array>
#include <functional>
#include <memory>
#include <set>
#include <ranges>

#include "game_util/poker.h"
#include "game_framework/game_stage.h"
#include "utility/html.h"
#include "game_util/bet_pool.h"

using namespace lgtbot::game_util::bet_pool;
namespace poker = lgtbot::game_util::poker;

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = GameStage<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = GameStage<void, SubStages...>;

const std::string k_game_name = "幸运波卡";
const uint64_t k_max_player = 4; /* 0 means no max-player limits */
const uint64_t k_multiple = 1;
const std::string k_developer = "森高";
const std::string k_description = "能够看到各个玩家部分手牌，两阶段下注的比拼大小的游戏";

std::string GameOption::StatusInfo() const
{
    std::string str;
    str += "首轮下注时，平均每副手牌可分得 " + std::to_string(GET_VALUE(首轮筹码)) + " 枚筹码，弃牌可获得 " +
        std::to_string(GET_VALUE(首轮弃牌得分)) + " 分\n";
    str += "次轮下注时，平均每副手牌可分得 " + std::to_string(GET_VALUE(次轮筹码)) + " 枚筹码，弃牌可获得 " +
        std::to_string(GET_VALUE(次轮弃牌得分)) + " 分\n";
    str += "每轮下注时限 " + std::to_string(GET_VALUE(下注时间)) + " 秒\n";
    str += GET_VALUE(模式) == 0 ? "隐藏手牌最大的一张，公开 2-3 张公共牌\n" :
           GET_VALUE(模式) == 1 ? "公开全部手牌和 0-1 张公共牌\n" :
                                  "隐藏手牌最大的一张，公开 0-1 张公共牌，公布被隐藏了哪些牌\n";
    if (GET_VALUE(种子).empty()) {
        str += "未指定种子";
    } else {
        str += "种子：" + GET_VALUE(种子);
    }
    return str;
}

bool GameOption::ToValid(MsgSenderBase& reply)
{
    if (PlayerNum() < 2) {
        reply() << "该游戏至少 2 人参加";
        return false;
    }
    if (GET_VALUE(次轮筹码) > GET_VALUE(首轮筹码)) {
        reply() << "「次轮筹码」不得高于「首轮筹码」";
        return false;
    }
    if (GET_VALUE(首轮弃牌得分) > GET_VALUE(首轮筹码)) {
        reply() << "「首轮弃牌得分」不得高于「首轮筹码」";
        return false;
    }
    if (GET_VALUE(次轮弃牌得分) > GET_VALUE(次轮筹码)) {
        reply() << "「次轮弃牌得分」不得高于「次轮筹码」";
        return false;
    }
    return true;
}

uint64_t GameOption::BestPlayerNum() const { return 3; }

// ========== GAME STAGES ==========

static constexpr const uint32_t k_hand_poker_num = 3;

static const char* const k_unknown_poker = "�?";

static std::string HandID2Str(const uint32_t id) { return std::string(1, 'A' + id); }

static uint32_t PlayerHandNum(const uint32_t player_num)
{
    return player_num == 2 ? 4 :
           player_num == 3 ? 3 :
           player_num == 4 ? 2 : (abort(), 0);
}

static constexpr const uint32_t k_mode_show_some_public_pokers = 0;
static constexpr const uint32_t k_mode_show_max_poker_each_hand = 1;
static constexpr const uint32_t k_mode_show_hiden_pokers_group = 2;

template <poker::CardType k_type>
struct PlayerHand
{
    static constexpr const uint32_t DISCARD_ALL = 3;
    static constexpr const uint32_t DISCARD_ALL_IMMUTBLE = 4;
    static constexpr const uint32_t DISCARD_NOT_CHOOSE = 5;

    PlayerHand(const uint32_t id, const PlayerID& pid, std::array<poker::Card<k_type>, k_hand_poker_num> hand)
        : id_(id)
        , pid_(pid)
        , hand_([&hand] { std::sort(hand.begin(), hand.end()); return hand; }())
        , immutable_coins_(0)
        , mutable_coins_(0)
        , immutable_score_(0)
        , mutable_score_(0)
        , discard_idx_(DISCARD_NOT_CHOOSE)
    {
    }

    const uint32_t id_;
    const PlayerID pid_;
    const std::array<poker::Card<k_type>, k_hand_poker_num> hand_;

    int32_t immutable_coins_;
    int32_t mutable_coins_;
    int32_t immutable_score_;
    int32_t mutable_score_;
    uint32_t discard_idx_;
};

template <poker::CardType k_type>
poker::OptionalDeck<k_type> GetBestDeck(const PlayerHand<k_type>& hand, const std::vector<poker::Card<k_type>>& public_pokers)
{
    if (hand.discard_idx_ >= k_hand_poker_num) {
        return std::nullopt; // the hand is discarded, regard as the minimal hand
    }
    poker::Hand<k_type> poker_hand;
    for (const auto& poker : hand.hand_) {
        poker_hand.Add(poker);
    }
    for (const auto& poker : public_pokers) {
        poker_hand.Add(poker);
    }
    poker_hand.Remove(hand.hand_[hand.discard_idx_]);
    return poker_hand.BestDeck();
}

template <poker::CardType k_type>
struct PlayerRoundInfo
{
    PlayerRoundInfo(const PlayerID& pid, const std::string& player_name, const int64_t score, std::vector<PlayerHand<k_type>>& hands)
        : pid_(pid), player_name_(player_name), score_(score), score_change_(0), remain_coins_(0), hands_(hands)
    {
    }

    void SetRemainCoins(const int64_t remain_coins) { remain_coins_ = remain_coins; }

    std::string CanPrepare(const bool is_first) const
    {
        if (remain_coins_ > 0) {
            return "您还存在未分配完的筹码 " + std::to_string(remain_coins_) + " 枚，无法准备";
        }
        if (!is_first && std::any_of(hands_.begin(), hands_.end(), [pid = pid_](const PlayerHand<k_type>& hand)
                    {
                        return hand.pid_ == pid && // is my hand
                               hand.discard_idx_ == PlayerHand<k_type>::DISCARD_NOT_CHOOSE && // has not been chose
                               hand.immutable_coins_ + hand.mutable_coins_ > 0; // not bet in first round
                    })) {
            return "您还有未完成选牌的牌组，无法准备";
        }
        return {};
    }

    std::string Bet(const uint32_t hand_id, const int64_t coins, const uint32_t discard_idx)
    {
        return Reset_(hand_id, coins, 0, discard_idx);
    }

    std::string Fold(const uint32_t hand_id, const int64_t coins, const int64_t scores, const bool is_mutable)
    {
        return Reset_(hand_id, coins, scores, is_mutable ? PlayerHand<k_type>::DISCARD_ALL : PlayerHand<k_type>::DISCARD_ALL_IMMUTBLE);
    }

    void RandomAct(const bool is_first)
    {
        for (uint32_t hand_id = 0; remain_coins_ > 0; hand_id = (hand_id + 1) % hands_.size()) {
            auto& hand = hands_[hand_id];
            if (hand.pid_ != pid_) {
                continue;
            }
            if (hand.discard_idx_ != PlayerHand<k_type>::DISCARD_ALL && hand.discard_idx_ != PlayerHand<k_type>::DISCARD_ALL_IMMUTBLE) {
                const auto coins =
                    rand() % ((is_first ? remain_coins_ : std::min(remain_coins_, static_cast<int64_t>(hand.immutable_coins_))) + 1);
                remain_coins_ -= coins;
                hand.mutable_coins_ += coins;
            }
            if (!is_first && hand.discard_idx_ == PlayerHand<k_type>::DISCARD_NOT_CHOOSE) {
                hand.discard_idx_ = rand() % k_hand_poker_num; // TODO: choose the best deck
            }
        }
    }

    // return score chagne
    void ClearRemainCoins(const auto& teller)
    {
        if (remain_coins_ <= 0) {
            score_change_ += remain_coins_;
            return;
        }
        for (auto& hand : hands_) {
            if (hand.pid_ == pid_ && hand.discard_idx_ != PlayerHand<k_type>::DISCARD_ALL && hand.discard_idx_ != PlayerHand<k_type>::DISCARD_ALL_IMMUTBLE) {
                teller(pid_) << "[警告] 您存在未用完的筹码 " << remain_coins_ << " 枚，默认全部下注到牌组 " << HandID2Str(hand.id_) << " 上";
                hand.mutable_coins_ += remain_coins_;
                remain_coins_ = 0;
                break;
            }
        }
    }

    std::string Reset_(const uint32_t hand_id, const int64_t coins, const int64_t scores, const uint32_t discard_idx)
    {
        PlayerHand<k_type>& hand = hands_[hand_id];
        if (hand.pid_ != pid_) {
            return "您所选择牌组本非本人牌组";
        }
        if (hand.discard_idx_ == PlayerHand<k_type>::DISCARD_ALL_IMMUTBLE) {
            return "该牌组已被弃置，无法变动";
        }
        const int64_t offset = hand.mutable_coins_ + hand.mutable_score_ - coins - scores;
        if (score_ + score_change_ + remain_coins_ + offset < 0) {
            return "您积分不足，无法加注";
        }
        if (hand.immutable_coins_ < coins && discard_idx < k_hand_poker_num) {
            return "下注筹码数不得超过上一轮下注数量 " + std::to_string(hand.immutable_coins_);
        }
        remain_coins_ += offset;
        hand.mutable_coins_ = coins;
        hand.mutable_score_ = scores;
        hand.discard_idx_ = discard_idx;
        return {};
    }

    std::string ToHtml(const bool show_coins, const bool show_all_pokers, const std::vector<poker::Card<k_type>>& public_pokers) const
    {
        std::string str;
        str += "### " + player_name_ + "（当前积分：" + std::to_string(score_);
        if (score_change_ > 0) {
            str += HTML_COLOR_FONT_HEADER(green);
            str += " + " + std::to_string(score_change_) + HTML_FONT_TAIL;
        } else if (score_change_ < 0) {
            str += HTML_COLOR_FONT_HEADER(red);
            str += " - " + std::to_string(-score_change_) + HTML_FONT_TAIL;
        }
        if (show_coins) {
            if (remain_coins_ > 0) {
                str += "，剩余筹码：" HTML_COLOR_FONT_HEADER(green) + std::to_string(remain_coins_) + HTML_FONT_TAIL;
            } else if (remain_coins_ < 0) {
                str += "，透支分数：" HTML_COLOR_FONT_HEADER(red) + std::to_string(remain_coins_) + HTML_FONT_TAIL;
            } else {
                str += "，剩余筹码：0";
            }
        }
        str += "）\n\n";
        const bool show_deck = !public_pokers.empty();
        const char* const css =
            "<style>\n"
            "table {\n"
                "width:90%;\n"
                //"box-shadow: 0 2px 12px 0 rgba(0, 0, 0, 0.1);/* 阴影 */\n"
                //"border-collapse: collapse;\n"
                "letter-spacing: 1px;\n"
            "}\n"
            "table,th,tr,td {\n"
                //"border-bottom:1px solid #dedede;\n"
                "padding: 5px;\n"
                "text-align: center;\n"
            "}\n"
            "</style>\n\n";
        html::Table table(1, show_deck ? 5 : 4);
        //table.SetTableStyle(" align=\"center\" cellpadding=\"1\" cellspacing=\"1\" ");
        table.Get(0, 0).SetContent("**ID**");
        table.Get(0, 1).SetContent("**手牌**");
        table.Get(0, 2).SetContent("**下注**");
        table.Get(0, 3).SetContent("**得分**");
        if (show_deck) {
            table.Get(0, 4).SetContent("**牌型**");
        }
        for (const auto& hand : hands_) {
            if (hand.pid_ != pid_) {
                continue;
            }
            table.AppendRow();
            table.GetLastRow(0).SetContent(HandID2Str(hand.id_));
            {
                std::string hand_str;
                const bool is_fold =
                    hand.discard_idx_ == PlayerHand<k_type>::DISCARD_ALL_IMMUTBLE ||
                    hand.discard_idx_ == PlayerHand<k_type>::DISCARD_ALL ||
                    (show_deck && hand.immutable_coins_ + hand.mutable_coins_ == 0);
                for (uint32_t i = 0; i < hand.hand_.size(); ++i) {
                    hand_str += !show_all_pokers && i == hand.hand_.size() - 1 ? k_unknown_poker :
                                is_fold || hand.discard_idx_ == i              ? HTML_COLOR_FONT_HEADER(grey) + hand.hand_[i].ToString() + HTML_FONT_TAIL :
                                                                                 hand.hand_[i].ToHtml();
                    hand_str += HTML_ESCAPE_SPACE;
                }
                table.GetLastRow(1).SetContent(std::move(hand_str));
            }
            if (show_coins) {
                table.GetLastRow(2).SetContent(std::to_string(hand.immutable_coins_) + " + " + std::to_string(hand.mutable_coins_));
            } else {
                table.GetLastRow(2).SetContent(std::to_string(hand.immutable_coins_));
            }
            {
                const auto total_score = hand.immutable_score_ + hand.mutable_score_;
                table.GetLastRow(3).SetContent(hand.mutable_score_ > hand.immutable_coins_ + hand.mutable_coins_ ?
                        HTML_COLOR_FONT_HEADER(green) + std::to_string(total_score) + HTML_FONT_TAIL :
                        std::to_string(total_score));
            }
            if (show_deck) {
                const auto deck = GetBestDeck(hand, public_pokers);
                if (hand.immutable_coins_ + hand.mutable_coins_ == 0) {
                    table.GetLastRow(4).SetContent("未下注");
                } else if (deck.has_value()) {
                    std::string s;
                    for (const auto& poker : deck->pokers_) {
                        s += poker.ToHtml() + HTML_ESCAPE_SPACE;
                    }
                    s += HTML_COLOR_FONT_HEADER(blue) "（";
                    s += deck->TypeName();
                    s += "）" HTML_FONT_TAIL;
                    table.GetLastRow(4).SetContent(std::move(s));
                } else {
                    table.GetLastRow(4).SetContent("弃牌");
                }
            }
        }
        return str + css + table.ToString();
    }

    const PlayerID pid_;
    const std::string player_name_;
    const int64_t score_;
    int64_t score_change_;
    int64_t remain_coins_;
    std::vector<PlayerHand<k_type>>& hands_;
    std::string html_;
};

template <poker::CardType k_type> class BetStage;
template <poker::CardType k_type> class RoundStage;

class MainStage : public MainGameStage<RoundStage<poker::CardType::BOKAA>, RoundStage<poker::CardType::POKER>>
{
  public:
    MainStage(const GameOption& option, MatchBase& match)
        : GameStage(option, match)
        , player_scores_(option.PlayerNum(), 0)
        , round_(0)
    {
    }

    virtual VariantSubStage OnStageBegin() override;

    virtual VariantSubStage NextSubStage(RoundStage<poker::CardType::BOKAA>& sub_stage, const CheckoutReason reason) override;

    virtual VariantSubStage NextSubStage(RoundStage<poker::CardType::POKER>& sub_stage, const CheckoutReason reason) override;

    int64_t PlayerScore(const PlayerID pid) const { return player_scores_[pid]; }

    int64_t& PlayerScoreRef(const PlayerID pid) { return player_scores_[pid]; }

  private:
    template <poker::CardType k_type>
    VariantSubStage NextSubStage_(RoundStage<k_type>& sub_stage, const CheckoutReason reason);

    std::vector<int64_t> player_scores_;
    uint32_t round_;
};

std::optional<uint64_t> ToHandID(const std::string& str)
{
    if (str.size() != 1) {
        return std::nullopt;
    } else if (str[0] >= 'A' && str[0] <= 'Z') {
        return str[0] - 'A';
    } else if (str[0] >= 'a' && str[0] <= 'z') {
        return str[0] - 'a';
    } else {
        return std::nullopt;
    }
}

template <poker::CardType k_type>
class BetStage : public SubGameStage<>
{
  public:
    BetStage(MainStage& main_stage, const bool is_first, std::vector<PlayerHand<k_type>>& hands, std::vector<PlayerRoundInfo<k_type>>& info)
        : GameStage(main_stage, is_first ? "首轮下注" : "次轮下注",
                is_first ?
                this->MakeStageCommand("对某个牌组下注一定金额", &BetStage::Bet_,
                    AnyArg("牌组", "A"), ArithChecker<uint32_t>(0, 100000, "金币数")) :
                this->MakeStageCommand("对某个牌组下注一定金额，并决定出一张**不使用**的牌", &BetStage::BetAndChoose_,
                    AnyArg("牌组", "A"), ArithChecker<uint32_t>(0, 100000, "金币数"), AnyArg("扑克", "红3")),
                this->MakeStageCommand("弃掉一个牌组", &BetStage::Fold_, AnyArg("牌组", "A"), VoidChecker("弃牌")),
                this->MakeStageCommand("完成行动", &BetStage::Prepare_, VoidChecker("准备")))
        , is_first_(is_first), hands_(hands), player_round_infos_(info)
    {
    }

    void OnStageBegin()
    {
        StartTimer(GET_OPTION_VALUE(option(), 下注时间));
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply)
    {
        player_round_infos_[pid].RandomAct(is_first_);
        return StageErrCode::READY;
    }

    CheckoutErrCode OnTimeout()
    {
        HookUnreadyPlayers();
        return StageErrCode::CHECKOUT;
    }

    void OnAllPlayerReady() { }
 
  private:
    std::string RemainCoinsInfo_(const PlayerID pid) const
    {
        if (player_round_infos_[pid].remain_coins_ >= 0) {
            return "剩余筹码 " + std::to_string(player_round_infos_[pid].remain_coins_) + " 枚";
        } else {
            return "透支分数 " + std::to_string(-player_round_infos_[pid].remain_coins_);
        }
    }

    AtomReqErrCode Bet_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::string& hand_id_str,
            const uint32_t coins)
    {
        const auto hand_id = ParseHandIdIfValid_(pid, is_public, reply, hand_id_str);
        if (!hand_id.has_value()) {
            return StageErrCode::FAILED;
        }
        if (const auto errmsg = player_round_infos_[pid].Bet(*hand_id, coins, PlayerHand<k_type>::DISCARD_NOT_CHOOSE); !errmsg.empty()) {
            reply() << "[错误] 行动失败：" << errmsg;
            return StageErrCode::FAILED;
        }
        auto sender = reply();
        sender << "下注成功：您当前对牌组 " << HandID2Str(*hand_id) << " 下注 " << coins << " 枚筹码，"
            << RemainCoinsInfo_(pid) << "\n\n若您已完成下注，请使用「准备」提交决定";
        return StageErrCode::OK;
    }

    AtomReqErrCode BetAndChoose_(const PlayerID pid, const bool is_public, MsgSenderBase& reply,
            const std::string& hand_id_str, const uint32_t coins, const std::string& poker_str)
    {
        const auto hand_id = ParseHandIdIfValid_(pid, is_public, reply, hand_id_str);
        if (!hand_id.has_value()) {
            return StageErrCode::FAILED;
        }
        std::stringstream ss;
        const auto poker = poker::Parse<std::string, std::stringstream&, k_type>(poker_str, ss);
        if (!poker.has_value()) {
            reply() << "弃牌失败：非法的扑克名「" << poker_str << "」，" << ss.str();
            return StageErrCode::FAILED;
        }
        std::optional<uint32_t> discard_idx;
        for (uint32_t i = 0; i < k_hand_poker_num; ++i) {
            if (hands_[*hand_id].hand_[i] == poker) {
                discard_idx = i;
                break;
            }
        }
        if (!discard_idx.has_value()) {
            reply() << "[错误] 行动失败：您指定的扑克「" << poker_str << "」未在该组手牌内";
            return StageErrCode::FAILED;
        }
        if (const auto errmsg = player_round_infos_[pid].Bet(*hand_id, coins, *discard_idx); !errmsg.empty()) {
            reply() << "[错误] 行动失败：" << errmsg;
            return StageErrCode::FAILED;
        }

        std::string use_poker_str;
        for (uint32_t i = 0; i < k_hand_poker_num; ++i) {
            if (i != discard_idx) {
                use_poker_str += hands_[*hand_id].hand_[i].ToString() + " ";
            }
        }
        reply() << "下注成功：您当前对牌组 " << HandID2Str(*hand_id) << " 下注 " << coins << " 枚筹码，决胜卡牌为 "
            << use_poker_str << "，" << RemainCoinsInfo_(pid) << "\n\n若您已完成下注，请使用「准备」提交决定";
        return StageErrCode::OK;
    }

    AtomReqErrCode Fold_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::string& hand_id_str)
    {
        const auto hand_id = ParseHandIdIfValid_(pid, is_public, reply, hand_id_str);
        if (!hand_id.has_value()) {
            return StageErrCode::FAILED;
        }
        const auto avg_coins_each_hand = is_first_ ? GET_OPTION_VALUE(option(), 首轮筹码) : GET_OPTION_VALUE(option(), 次轮筹码);
        const auto fold_score = is_first_ ? GET_OPTION_VALUE(option(), 首轮弃牌得分) : GET_OPTION_VALUE(option(), 次轮弃牌得分);
        if (const auto errmsg = player_round_infos_[pid].Fold(*hand_id, avg_coins_each_hand - fold_score, fold_score, true); !errmsg.empty()) {
            reply() << "[错误] 弃牌失败：" << errmsg;
            return StageErrCode::FAILED;
        }
        reply() << "弃牌成功：您弃牌 " << HandID2Str(*hand_id) << "，下注了 " << (avg_coins_each_hand - fold_score)
            << " 枚筹码，同时可直接将 " << fold_score << " 枚筹码转化为积分，" << RemainCoinsInfo_(pid)
            << "\n\n若您已完成下注，请使用「准备」提交决定";
        return StageErrCode::OK;
    }

    std::optional<uint32_t> ParseHandIdIfValid_(const PlayerID pid, const bool is_public, MsgSenderBase& reply,
            const std::string& hand_id_str)
    {
        if (is_public) {
            reply() << "[错误] 请私信裁判行动";
            return std::nullopt;
        }
        if (IsReady(pid)) {
            reply() << "[错误] 您已经完成准备，无法行动";
            return std::nullopt;
        }
        const auto hand_id = ToHandID(hand_id_str);
        if (!hand_id.has_value()) {
            reply() << "[错误] 非法的牌组 ID：" << hand_id_str << "，应该为 A-Z 中的字母";
            return std::nullopt;
        }
        return hand_id;
    }

    AtomReqErrCode Prepare_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (IsReady(pid)) {
            reply() << "[错误] 您已经完成准备，无法重复准备";
            return StageErrCode::FAILED;
        }
        if (const std::string str = player_round_infos_[pid].CanPrepare(is_first_); !str.empty()) {
            reply() << "[错误] " << str;
            return StageErrCode::FAILED;
        }
        reply() << "准备成功";
        return StageErrCode::READY;
    }

    const bool is_first_;
    std::vector<PlayerHand<k_type>>& hands_;
    std::vector<PlayerRoundInfo<k_type>>& player_round_infos_;
};

template <poker::CardType k_type>
class RoundStage : public SubGameStage<BetStage<k_type>>
{
    using VariantSubStage = SubGameStage<BetStage<k_type>>::VariantSubStage;
    using GameStage = SubGameStage<BetStage<k_type>>;

  public:
    RoundStage(MainStage& main_stage, const uint32_t round)
        : GameStage(main_stage, "第 " + std::to_string(round + 1) + " 回合",
                this->MakeStageCommand("通过图片查看各玩家手牌及金币情况", &RoundStage::Status_, VoidChecker("赛况")))
        , is_first_(true), player_htmls_(this->option().PlayerNum())
    {
        const auto shuffled_pokers = poker::ShuffledPokers<k_type>(GET_OPTION_VALUE(this->option(), 种子).empty() ? "" : GET_OPTION_VALUE(this->option(), 种子) + std::to_string(round));
        const auto player_num = this->option().PlayerNum();
        const uint32_t player_hand_num = PlayerHandNum(player_num);
        auto it = shuffled_pokers.cbegin();
        for (PlayerID pid = 0; pid < player_num; ++pid) {
            for (uint32_t i = 0; i < player_hand_num; ++i) {
                hands_.emplace_back(pid * player_hand_num + i, pid, std::array<poker::Card<k_type>, k_hand_poker_num>{*it++, *it++, *it++});
            }
            player_round_infos_.emplace_back(pid, this->PlayerAvatar(pid, 50) + HTML_ESCAPE_SPACE + HTML_ESCAPE_SPACE + this->PlayerName(pid), main_stage.PlayerScore(pid), hands_);
        }
        for (uint64_t i = 0; i < GET_OPTION_VALUE(this->option(), 公共牌数); ++i) {
            public_pokers_.emplace_back(*it++);
        }
    }

    virtual VariantSubStage OnStageBegin() override
    {
        for (auto& info : player_round_infos_) {
            info.SetRemainCoins(GET_OPTION_VALUE(this->option(), 首轮筹码) * PlayerHandNum(this->option().PlayerNum()));
        }
        SavePlayerHtmls_();
        this->Group() << Markdown{MiddleHtml_(true), k_markdown_width_};
        for (PlayerID pid = 0; pid < this->option().PlayerNum(); ++pid) {
            this->Tell(pid) << Markdown{PrivateHtml_(pid, true), k_markdown_width_};
        }
        this->Boardcast() << "请各位玩家私信裁判进行第一轮下注，您可通过「帮助」命令查看命令格式";
        return std::make_unique<BetStage<k_type>>(this->main_stage(), is_first_, hands_, player_round_infos_);
    }

    virtual VariantSubStage NextSubStage(BetStage<k_type>& sub_stage, const CheckoutReason reason) override
    {
        for (auto& player_info : player_round_infos_) {
            player_info.ClearRemainCoins([this](const PlayerID pid) { return this->Tell(pid); });
        }
        for (auto& hand : hands_) {
            hand.immutable_coins_ += hand.mutable_coins_;
            hand.mutable_coins_ = 0;
            hand.immutable_score_ += hand.mutable_score_;
            hand.mutable_score_ = 0;
        }
        if (is_first_) {
            is_first_ = false;
            for (auto& info : player_round_infos_) {
                info.SetRemainCoins(GET_OPTION_VALUE(this->option(), 次轮筹码) * PlayerHandNum(this->option().PlayerNum()));
            }
            SavePlayerHtmls_();
            for (auto& hand : hands_) {
                if (hand.discard_idx_ == PlayerHand<k_type>::DISCARD_ALL) {
                    player_round_infos_[hand.pid_].Fold(
                            hand.id_,
                            GET_OPTION_VALUE(this->option(), 次轮筹码) - GET_OPTION_VALUE(this->option(), 次轮弃牌得分),
                            GET_OPTION_VALUE(this->option(), 次轮弃牌得分),
                            false /* is_mutable */);
                }
            }
            this->Boardcast() << "第一轮下注结束，公布各玩家选择：";
            this->Group() << Markdown{MiddleHtml_(false), k_markdown_width_};
            for (PlayerID pid = 0; pid < this->option().PlayerNum(); ++pid) {
                this->Tell(pid) << Markdown{PrivateHtml_(pid, false), k_markdown_width_};
            }
            this->Boardcast() << "请各位玩家私信裁判进行第二轮下注，并决定**不参与**决胜的卡牌，您可通过「帮助」命令查看命令格式";
            return std::make_unique<BetStage<k_type>>(this->main_stage(), is_first_, hands_, player_round_infos_);
        } else {
            std::vector<CallBetPoolInfo<poker::OptionalDeck<k_type>>> decks;
            for (auto& hand : hands_) {
                if (hand.discard_idx_ == PlayerHand<k_type>::DISCARD_NOT_CHOOSE) {
                    hand.discard_idx_ = 0;
                }
                decks.emplace_back(CallBetPoolInfo{
                        .id_ = hand.id_,
                        .coins_ = hand.immutable_coins_,
                        .obj_ = GetBestDeck(hand, public_pokers_)});
            }
            const auto bet_rets = CallBetPool(decks, [](const auto& _1, const auto& _2) { return _1.CompareIgnoreSuit(_2) < 0; });
            for (const auto& ret : bet_rets) {
                for (const auto& hand_id : ret.winner_ids_) {
                    hands_[hand_id].mutable_score_ += ret.total_coins_ / ret.winner_ids_.size();
                }
            }
            for (const auto& hand : hands_) {
                player_round_infos_[hand.pid_].score_change_ += hand.immutable_score_ + hand.mutable_score_;
            }
            this->Boardcast() << "第二轮下注结束，公布各玩家选择：";
            this->Group() << Markdown{EndHtml_() + BetResultHtml_(bet_rets), k_markdown_width_};
            for (PlayerID pid = 0; pid < this->option().PlayerNum(); ++pid) {
                this->Tell(pid) << Markdown{EndHtml_() + BetResultHtml_(bet_rets), k_markdown_width_};
            }
            this->Boardcast() << "回合结束";
            for (const auto& info : player_round_infos_) {
                this->main_stage().PlayerScoreRef(info.pid_) += info.score_change_;
            }
            return nullptr;
        }
    }

  private:
    void SavePlayerHtmls_()
    {
        for (const PlayerRoundInfo<k_type>& info : player_round_infos_) {
            player_htmls_[info.pid_] = info.ToHtml(false, GET_OPTION_VALUE(this->option(), 模式) == k_mode_show_max_poker_each_hand, {});
        }
    }

    std::string MiddleHtml_(const bool is_first) const
    {
        std::string s = MiddleHeadHtml_(is_first);
        for (PlayerID p = 0; p < this->option().PlayerNum(); ++p) {
            s += player_htmls_[p] + "\n\n";
        }
        return s;
    }

    std::string EndHtml_() const
    {
        std::string s = HeadHtml_(GET_OPTION_VALUE(this->option(), 公共牌数), false) + "\n\n";
        for (const auto& info : player_round_infos_) {
            s += info.ToHtml(false, true, public_pokers_) + "\n\n";
        }
        return s;
    }

    std::string PrivateHtml_(const PlayerID& pid, const bool is_first) const
    {
        std::string s = MiddleHeadHtml_(is_first);
        s += player_round_infos_[pid].ToHtml(true, true, {}) + "\n\n";
        for (PlayerID p = 0; p < this->option().PlayerNum(); ++p) {
            if (p != pid) {
                s += player_htmls_[p] + "\n\n";
            }
        }
        return s;
    }

    std::string MiddleHeadHtml_(const bool is_first) const
    {
        const uint32_t show_public_num =
            GET_OPTION_VALUE(this->option(), 模式) != k_mode_show_some_public_pokers ? (is_first ? 0 : 1) : (is_first ? 2 : 3);
        return HeadHtml_(show_public_num, GET_OPTION_VALUE(this->option(), 模式) == k_mode_show_hiden_pokers_group) + "\n\n";
    }

    std::string HeadHtml_(const uint32_t show_public_num, const bool show_hiden_pokers) const
    {
        std::string s = "<center>\n\n## " + this->name() + "\n\n</center>\n\n<center>\n\n**第" + (is_first_ ? "一" : "二") + "轮下注**</center>\n\n";
        s += "<center><font size=\"4\">\n\n**公共牌：";
        for (uint32_t i = 0; i < show_public_num; ++i) {
            s += public_pokers_[i].ToHtml() + HTML_ESCAPE_SPACE;
        }
        for (uint32_t i = show_public_num; i < public_pokers_.size(); ++i) {
            s += std::string(k_unknown_poker) + HTML_ESCAPE_SPACE;
        }
        s += "**\n\n</font></center>\n\n";
        if (show_hiden_pokers) {
            s += "<center>隐藏牌：";
            std::vector<poker::Card<k_type>> hiden_pokers = public_pokers_;
            for (const auto& hand : hands_) {
                hiden_pokers.emplace_back(hand.hand_[k_hand_poker_num - 1]);
            }
            std::ranges::sort(hiden_pokers);
            for (const auto& poker : hiden_pokers) {
                s += poker.ToHtml() + HTML_ESCAPE_SPACE;
            }
            s += "</center>";
        }
        return s;
    }

    static std::string BetResultHtml_(const std::vector<CallBetPoolResult>& bet_ret)
    {
        std::string s;
        const auto print_ids = [&s](const std::set<uint64_t>& ids)
            {
                for (const auto id : ids) {
                    s += HandID2Str(id) + " ";
                }
            };
        for (const auto& ret : bet_ret) {
            s += "- 由牌组 " HTML_COLOR_FONT_HEADER(blue);
            print_ids(ret.participant_ids_);
            s += HTML_FONT_TAIL " 共同参与下注的边池，包含筹码共 " HTML_COLOR_FONT_HEADER(blue);
            s += std::to_string(ret.total_coins_) + HTML_FONT_TAIL " 枚，手牌最大的牌组 **" + HTML_COLOR_FONT_HEADER(green);
            print_ids(ret.winner_ids_);
            s += HTML_FONT_TAIL "** 平分金币，平均每牌组可分得 **" HTML_COLOR_FONT_HEADER(green);
            s += std::to_string(ret.total_coins_ / ret.winner_ids_.size()) + HTML_FONT_TAIL "** 枚\n";
        }
        return s;
    }

    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (is_public) {
            reply() << Markdown{MiddleHtml_(is_first_), k_markdown_width_};
        } else {
            reply() << Markdown{PrivateHtml_(pid, is_first_), k_markdown_width_};
        }
        return StageErrCode::OK;
    }

    std::vector<PlayerHand<k_type>> hands_;
    std::vector<PlayerRoundInfo<k_type>> player_round_infos_;
    std::vector<poker::Card<k_type>> public_pokers_;
    std::vector<std::string> player_htmls_;
    bool is_first_;
    static constexpr uint32_t k_markdown_width_ = 650;
};


MainStage::VariantSubStage MainStage::OnStageBegin()
{
    if (GET_OPTION_VALUE(option(), 卡牌) == poker::CardType::BOKAA) {
        return std::make_unique<RoundStage<poker::CardType::BOKAA>>(*this, round_);
    } else if (GET_OPTION_VALUE(option(), 卡牌) == poker::CardType::POKER) {
        return std::make_unique<RoundStage<poker::CardType::POKER>>(*this, round_);
    } else {
        assert(false);
        return {};
    }
}

MainStage::VariantSubStage MainStage::NextSubStage(RoundStage<poker::CardType::BOKAA>& sub_stage, const CheckoutReason reason)
{
    return NextSubStage_(sub_stage, reason);
}

MainStage::VariantSubStage MainStage::NextSubStage(RoundStage<poker::CardType::POKER>& sub_stage, const CheckoutReason reason)
{
    return NextSubStage_(sub_stage, reason);
}

template <poker::CardType k_type>
MainStage::VariantSubStage MainStage::NextSubStage_(RoundStage<k_type>& sub_stage, const CheckoutReason reason)
{
    if ((++round_) < GET_OPTION_VALUE(option(), 轮数)) {
        return std::make_unique<RoundStage<k_type>>(*this, round_);
    }
    return {};
}

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

#include "game_framework/make_main_stage.h"
