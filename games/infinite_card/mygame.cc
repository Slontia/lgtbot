// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "card.h"
#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"
#include "utility/random.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace std;

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;

const GameProperties k_properties {
    .name_ = "无限牌",
    .developer_ = "铁蛋",
    .description_ = "两人各持三组隐藏的牌，逐局比大小，先让对手金币归零者获胜",
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return k_player_num; }
uint32_t Multiple(const CustomOptions& options) {
    return 1;
}
const MutableGenericOptions k_default_generic_options;
const std::vector<RuleCommand> k_rule_commands = {};

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options)
            {
                generic_options.bench_computers_to_player_num_ = k_player_num;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
};

bool AdaptOptions(ChildMsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() != k_player_num) {
        reply() << "该游戏为 " << k_player_num << " 人游戏，当前玩家数为 " << generic_options_readonly.PlayerNum();
        return false;
    }
    return true;
}

static constexpr size_t k_max_log_num = 8;          // 赛况图中展示的记录条数
static constexpr uint32_t k_board_width = 700;      // 赛况图的宽度
static constexpr uint32_t k_hand_width = 460;       // 私人牌面图的宽度

// 卡牌底色，按数字大小分档，无限牌单独一色
static constexpr const char* k_low_back = "#9AA5AD";        // 数字 1~3
static constexpr const char* k_mid_back = "#5B9BD5";        // 数字 4~7
static constexpr const char* k_high_back = "#E06C63";       // 数字 8~10
static constexpr const char* k_infinite_back = "#7F5093";   // 无限牌
static constexpr int k_low_bound = 3;
static constexpr int k_mid_bound = 7;

// ========== 指令校验器 ==========

// 单张卡牌，如「7」「∞」
class CardChecker : public MsgArgChecker<Card>
{
  public:
    virtual ~CardChecker() {}

    virtual std::string FormatInfo() const override { return "<卡牌>"; }

    virtual std::string EscapedFormatInfo() const override
    {
        return string(HTML_ESCAPE_LT) + "卡牌" + HTML_ESCAPE_GT;
    }

    virtual std::string ColoredFormatInfo() const override
    {
        return HTML_COLOR_FONT_HEADER(green) + EscapedFormatInfo() + HTML_FONT_TAIL;
    }

    virtual std::string ExampleInfo() const override { return "7"; }

    virtual std::optional<Card> Check(MsgReader& reader) const override
    {
        if (!reader.HasNext()) {
            return std::nullopt;
        }
        return ParseCard(reader.NextArg());
    }

    virtual std::string ArgString(const Card& value) const override { return CardName(value); }
};

// ========== GAME STAGES ==========

class RoundStage;

class MainStage : public MainGameStage<RoundStage>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility),
                MakeStageCommand(*this, "查看当前赛况（私信时附带自己的牌）", &MainStage::Status_, VoidChecker("赛况")))
    {
        players_.resize(Global().PlayerNum());
        scores_.assign(Global().PlayerNum(), 0);
        rng_ = MakeRng(GAME_OPTION(种子));
        for (auto& player : players_) {
            player.coin_ = static_cast<int>(GAME_OPTION(金币));
        }
    }

    virtual int64_t PlayerScore(const PlayerID pid) const override { return scores_[pid]; }

    // ---------- 单个小局的记录，用于赛况图展示 ----------

    struct DuelRecord
    {
        bool done_ = false;                     // 该小局是否已结束
        bool revealed_ = false;                 // 双方的牌是否已揭示
        bool by_fold_ = false;                  // 是否因一方放弃而结束
        int winner_ = -1;                       // 胜者，-1 表示平局
        int pot_ = 0;                           // 该小局累积区的金币
        std::array<int, k_player_num> bets_{};  // 双方最终的投入
    };

    // ---------- 状态 ----------

    vector<Player> players_;
    vector<int64_t> scores_;
    mt19937 rng_;
    array<DuelRecord, k_duel_num> records_{};
    vector<string> logs_;               // 本回合的记录
    int round_{0};                      // 当前回合数，从 1 开始
    int duel_{0};                       // 当前小局，从 0 开始
    int base_{0};                       // 本回合的基础投入
    int pot_{0};                        // 本小局累积区的金币
    int turn_{0};                       // 本小局当前行动方
    int duel_first_{0};                 // 本小局的先手
    int round_first_{0};                // 本回合首个小局的先手
    Step step_{Step::SELECT};           // 当前所处的阶段
    bool game_over_{false};
    optional<int> winner_;              // 整场游戏的胜者，平局时为空

    // ---------- 通用查询 ----------

    static PlayerID Pid_(const int pid) { return PlayerID(static_cast<uint32_t>(pid)); }

    // 本游戏固定为两人对局，对手即另一方
    static int Opponent_(const int pid) { return k_player_num - 1 - pid; }

    // 赛况图记录中的玩家标识，图片内无法渲染 @，改用与玩家列表一致的编号
    static string PlayerTag_(const int pid) { return to_string(pid + 1) + "号"; }

    void AddLog_(const string& log) { logs_.emplace_back(log); }

    // 该玩家尚未排出的数字牌之和
    int RemainSum_(const int pid) const
    {
        int sum = 0;
        for (const auto& group : players_[pid].groups_) {
            for (const auto& card : group) {
                if (!card.IsInfinite()) {
                    sum += card.point_;
                }
            }
        }
        for (const auto& card : players_[pid].hand_) {
            if (!card.IsInfinite()) {
                sum += card.point_;
            }
        }
        return sum;
    }

    // 仅当前行动方处于未准备状态，其余玩家标记为已完成以免框架等待其行动
    void SetReadyForTurn_()
    {
        Global().ClearReady();
        for (uint32_t pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (static_cast<int>(pid) != turn_ && !players_[pid].eliminated_) {
                Global().SetReady(PlayerID(pid));
            }
        }
    }

    // ---------- 图片 UI ----------

    // 单张卡牌
    static string CardBox_(const Card& card)
    {
        const char* back = k_infinite_back;
        if (!card.IsInfinite()) {
            back = card.point_ <= k_low_bound ? k_low_back : (card.point_ <= k_mid_bound ? k_mid_back : k_high_back);
        }
        return "<div style=\"display:inline-block; min-width:52px; margin:2px; padding:5px 4px; border-radius:8px; "
               "background:" + string(back) + "; color:#FFFFFF; font-size:20px; font-weight:bold; "
               "text-align:center;\">" + CardName(card) + "</div>";
    }

    // 尚未揭示的牌
    static string HiddenBox_()
    {
        return "<div style=\"display:inline-block; min-width:52px; margin:2px; padding:5px 4px; border-radius:8px; "
               "background:#D9D9D9; color:#FFFFFF; font-size:20px; font-weight:bold; text-align:center;\">?</div>";
    }

    // 一组牌，空组显示占位文字
    static string CardsBox_(const vector<Card>& cards)
    {
        if (cards.empty()) {
            return "<font size=\"3\" color=\"#AAAAAA\">（空）</font>";
        }
        string str;
        for (const auto& card : cards) {
            str += CardBox_(card);
        }
        return str;
    }

    // 回合、小局、基础投入、累积区与当前阶段
    string HeadTable_() const
    {
        static const char* const k_heads[5] = { "回合", "小局", "基础投入", "累积区", "当前阶段" };
        html::Table table(2, 5);
        table.SetTableStyle(" align=\"center\" cellpadding=\"4\" cellspacing=\"2\" ");
        for (uint32_t i = 0; i < 5; ++i) {
            table.Get(0, i).SetColor("#EDEDED").SetStyle("style=\"width:130px;\"")
                .SetContent(string("<font size=\"3\" color=\"#666666\">") + k_heads[i] + "</font>");
        }
        table.Get(1, 0).SetContent("<font size=\"4\"><b>" + to_string(round_) + "</b> / " +
                to_string(GAME_OPTION(回合上限)) + "</font>");
        // 四个小局全部结束后 duel_ 会越过末位，展示时收敛到末位
        table.Get(1, 1).SetContent("<font size=\"4\"><b>" + to_string(std::min(duel_ + 1, k_duel_num)) + "</b> / " +
                to_string(k_duel_num) + "</font>");
        table.Get(1, 2).SetContent("<font size=\"4\"><b>" + to_string(base_) + "</b></font>");
        table.Get(1, 3).SetContent("<font size=\"4\" color=\"#7F5093\"><b>" + to_string(pot_) + "</b></font>");
        const string result = winner_.has_value() ? PlayerTag_(*winner_) + "获胜" : string("平局");
        table.Get(1, 4).SetContent(game_over_
                ? "<font size=\"4\" color=\"#C00000\"><b>" + result + "</b></font>"
                : string("<font size=\"4\">") + k_step_names[static_cast<int>(step_)] + "</font>");
        return table.ToString();
    }

    // 该玩家的无限牌是否已在某个已揭示的小局中公开
    bool InfiniteExposed_(const int pid) const
    {
        for (int duel = 0; duel < k_duel_num; ++duel) {
            if (!records_[static_cast<size_t>(duel)].revealed_) {
                continue;
            }
            if (const optional<Card> card = players_[pid].DuelCard(duel); card.has_value() && card->IsInfinite()) {
                return true;
            }
        }
        return false;
    }

    // 双方的金币与状态。无限牌的使用情况在公开前仅本人可见
    string PlayerTable_(const optional<int>& viewer, const bool reveal_all) const
    {
        static const char* const k_heads[4] = { "玩家", "金币", "本局无限牌", "本小局投入" };
        static const char* const k_widths[4] = { "style=\"width:370px;\"", "style=\"width:100px;\"",
                "style=\"width:100px;\"", "style=\"width:110px;\"" };
        html::Table table(1 + k_player_num, 4);
        table.SetTableStyle(" align=\"center\" cellpadding=\"4\" cellspacing=\"2\" ");
        for (uint32_t i = 0; i < 4; ++i) {
            table.Get(0, i).SetColor("#EDEDED").SetStyle(k_widths[i])
                .SetContent(string("<font size=\"3\" color=\"#666666\">") + k_heads[i] + "</font>");
        }
        for (uint32_t pid = 0; pid < Global().PlayerNum(); ++pid) {
            const Player& player = players_[pid];
            const uint32_t row = pid + 1;
            const bool acting = !game_over_ && step_ == Step::BET && static_cast<int>(pid) == turn_;
            const string color = player.eliminated_ ? "#E8E8E8" : (acting ? "#FFF2CC" : "#F6F6F6");
            table.Get(row, 0).SetColor(color).SetStyle("style=\"width:370px; text-align:left;\"")
                .SetContent("<font size=\"4\"><b>" + to_string(pid + 1) + "号</b></font> " +
                        Global().PlayerAvatar(PlayerID(pid), 30) + " <font size=\"4\">" +
                        Global().PlayerName(PlayerID(pid)) + "</font>");
            table.Get(row, 1).SetColor(color)
                .SetContent("<font size=\"5\" color=\"#7F5093\"><b>" + to_string(player.coin_) + "</b></font>");
            // 无限牌一旦在已揭示的小局中出现，其使用情况即成为公开信息
            const bool own = viewer.has_value() && *viewer == static_cast<int>(pid);
            const bool visible = own || reveal_all || InfiniteExposed_(static_cast<int>(pid));
            table.Get(row, 2).SetColor(color).SetContent(visible
                    ? (player.used_infinite_ ? "<font size=\"3\" color=\"#7F5093\">已使用</font>"
                                             : "<font size=\"3\" color=\"#00875A\">未使用</font>")
                    : "<font size=\"3\" color=\"#AAAAAA\">未公开</font>");
            table.Get(row, 3).SetColor(color)
                .SetContent("<font size=\"4\">" + to_string(player.bet_) + "</font>");
        }
        return table.ToString();
    }

    // 小局中某位玩家的牌，仅在该小局已揭示、回合结算或查看者为本人时可见
    string DuelCardBox_(const int duel, const int pid, const optional<int>& viewer, const bool reveal_all) const
    {
        const optional<Card> card = players_[pid].DuelCard(duel);
        if (!card.has_value()) {
            return "<font size=\"3\" color=\"#CCCCCC\">—</font>";
        }
        if (reveal_all || records_[static_cast<size_t>(duel)].revealed_ ||
                (viewer.has_value() && *viewer == pid)) {
            return CardBox_(*card);
        }
        return HiddenBox_();
    }

    // 回合结束时消失的那张牌。仅当其为无限牌时公开，因为「未使用无限牌」的奖励已经暴露了该信息
    string DiscardBox_(const int pid, const optional<int>& viewer) const
    {
        const Player& player = players_[pid];
        if (player.hand_.empty()) {
            return "<font size=\"3\" color=\"#CCCCCC\">—</font>";
        }
        const bool own = viewer.has_value() && *viewer == pid;
        string str;
        for (const auto& card : player.hand_) {
            str += (own || card.IsInfinite()) ? CardBox_(card) : HiddenBox_();
        }
        return str;
    }

    // 小局的结果
    string DuelResultBox_(const int duel) const
    {
        const DuelRecord& record = records_[static_cast<size_t>(duel)];
        if (!record.done_) {
            if (duel == duel_ && !game_over_) {
                return "<font size=\"3\" color=\"#C55A11\">进行中</font>";
            }
            return "<font size=\"3\" color=\"#CCCCCC\">—</font>";
        }
        if (record.by_fold_) {
            return "<font size=\"3\" color=\"#999999\">" + PlayerTag_(Opponent_(record.winner_)) + "放弃</font>"
                   "<br><font size=\"3\" color=\"#666666\">+" + to_string(record.pot_) + "</font>";
        }
        if (record.winner_ < 0) {
            return "<font size=\"3\" color=\"#666666\">平局</font>"
                   "<br><font size=\"3\" color=\"#666666\">各收回 " + to_string(record.bets_[0]) + "</font>";
        }
        return "<font size=\"4\" color=\"#00875A\"><b>" + PlayerTag_(record.winner_) + "胜</b></font>"
               "<br><font size=\"3\" color=\"#666666\">+" + to_string(record.pot_) + "</font>";
    }

    // 本回合四个小局的牌与结果。回合结算时额外展示消失的那张牌
    string DuelTable_(const optional<int>& viewer, const bool reveal_all, const bool show_discard) const
    {
        const uint32_t discard_col = show_discard ? static_cast<uint32_t>(k_duel_num + 1) : 0;
        html::Table table(2 + k_player_num, 1 + k_duel_num + (show_discard ? 1 : 0));
        table.SetTableStyle(" align=\"center\" cellpadding=\"4\" cellspacing=\"2\" ");
        table.Get(0, 0).SetColor("#EDEDED").SetStyle("style=\"width:90px;\"")
            .SetContent("<font size=\"3\" color=\"#666666\">小局</font>");
        for (int duel = 0; duel < k_duel_num; ++duel) {
            const bool current = !game_over_ && !reveal_all && duel == duel_;
            table.Get(0, static_cast<uint32_t>(duel + 1)).SetColor(current ? "#FFF2CC" : "#EDEDED")
                .SetStyle("style=\"width:120px;\"")
                .SetContent("<font size=\"3\" color=\"#666666\">第 " + to_string(duel + 1) + " 局</font>");
        }
        if (show_discard) {
            table.Get(0, discard_col).SetColor("#EDEDED").SetStyle("style=\"width:120px;\"")
                .SetContent("<font size=\"3\" color=\"#666666\">消失</font>");
        }
        for (uint32_t pid = 0; pid < Global().PlayerNum(); ++pid) {
            const uint32_t row = pid + 1;
            table.Get(row, 0).SetColor("#FAFAFA")
                .SetContent("<font size=\"3\" color=\"#666666\">" + to_string(pid + 1) + "号</font>");
            for (int duel = 0; duel < k_duel_num; ++duel) {
                table.Get(row, static_cast<uint32_t>(duel + 1))
                    .SetContent(DuelCardBox_(duel, static_cast<int>(pid), viewer, reveal_all));
            }
            if (show_discard) {
                table.Get(row, discard_col).SetColor("#FAFAFA")
                    .SetContent(DiscardBox_(static_cast<int>(pid), viewer));
            }
        }
        const uint32_t result_row = k_player_num + 1;
        table.Get(result_row, 0).SetColor("#FAFAFA")
            .SetContent("<font size=\"3\" color=\"#666666\">结果</font>");
        for (int duel = 0; duel < k_duel_num; ++duel) {
            table.Get(result_row, static_cast<uint32_t>(duel + 1)).SetContent(DuelResultBox_(duel));
        }
        if (show_discard) {
            table.Get(result_row, discard_col).SetColor("#FAFAFA")
                .SetContent("<font size=\"3\" color=\"#CCCCCC\">不计入对局</font>");
        }
        return table.ToString();
    }

    // 本回合最近的操作记录
    string LogTable_() const
    {
        if (logs_.empty()) {
            return "";
        }
        const size_t begin = logs_.size() > k_max_log_num ? logs_.size() - k_max_log_num : 0;
        html::Table table(static_cast<uint32_t>(logs_.size() - begin + 1), 1);
        table.SetTableStyle(" align=\"center\" cellpadding=\"3\" cellspacing=\"0\" ");
        table.Get(0, 0).SetColor("#EDEDED").SetStyle("style=\"width:660px;\"")
            .SetContent("<font size=\"3\" color=\"#666666\">本回合记录</font>");
        for (size_t i = begin; i < logs_.size(); ++i) {
            table.Get(static_cast<uint32_t>(i - begin + 1), 0).SetStyle("style=\"width:660px; text-align:left;\"")
                .SetContent("<font size=\"3\">" + logs_[i] + "</font>");
        }
        return table.ToString();
    }

    // 完整赛况。viewer 为空时仅展示公开信息；reveal_all 为真时公开双方本回合排出的全部牌；
    // show_discard 为真时额外展示回合结束时消失的那张牌
    string BoardImpl_(const optional<int>& viewer, const bool reveal_all, const bool show_discard,
            const string& title) const
    {
        string html = "<h2 align=\"center\" style=\"margin:6px;\">无限牌 · 第 " + to_string(round_) + " 回合" +
                title + "</h2>";
        html += HeadTable_();
        html += PlayerTable_(viewer, reveal_all);
        html += DuelTable_(viewer, reveal_all, show_discard);
        html += LogTable_();
        return html;
    }

    string GetBoard_() const { return BoardImpl_(nullopt, false, false, ""); }

    string GetPrivateBoard_(const int pid) const
    {
        return BoardImpl_(pid, false, false, "") + HandBlock_(pid, "您本回合的牌");
    }

    // 回合结算图：公开双方本回合排出的全部四张牌，消失的那张仅在其为无限牌时公开
    string GetRoundSummary_() const { return BoardImpl_(nullopt, true, true, " 结算"); }

    // 终局图：公开双方本回合排出的全部牌，但不展示未打出的牌
    string GetFinalBoard_() const { return BoardImpl_(nullopt, true, false, ""); }

    void BoardcastBoard_() const { Global().Boardcast() << Markdown(GetBoard_(), k_board_width); }

    // 私人牌面：尚未选组时展示三组牌
    string GroupsBlock_(const int pid, const string& title) const
    {
        const Player& player = players_[pid];
        string html = "<h3 align=\"center\" style=\"margin:6px;\">" + title + "</h3>";
        html::Table table(k_group_num, 2);
        table.SetTableStyle(" align=\"center\" cellpadding=\"4\" cellspacing=\"2\" ");
        for (int group = 0; group < k_group_num; ++group) {
            const vector<Card>& cards = player.groups_[static_cast<size_t>(group)];
            table.Get(static_cast<uint32_t>(group), 0).SetColor("#EDEDED").SetStyle("style=\"width:90px;\"")
                .SetContent("<font size=\"4\"><b>第" + to_string(group + 1) + "组</b></font>");
            table.Get(static_cast<uint32_t>(group), 1).SetStyle("style=\"width:340px; text-align:left;\"")
                .SetContent(cards.empty() ? string("<font size=\"3\" color=\"#AAAAAA\">（已使用）</font>")
                                          : CardsBox_(cards));
        }
        html += table.ToString();
        html += "<p align=\"center\" style=\"margin:4px;\"><font size=\"3\" color=\"#666666\">"
                "尚未排出的数字牌之和：" + to_string(RemainSum_(pid)) + "</font></p>";
        return html;
    }

    // 私人牌面：本回合选定的牌组与已排出的顺序
    string HandBlock_(const int pid, const string& title) const
    {
        const Player& player = players_[pid];
        string html = "<h3 align=\"center\" style=\"margin:6px;\">" + title + "</h3>";
        html::Table table(2, 2);
        table.SetTableStyle(" align=\"center\" cellpadding=\"4\" cellspacing=\"2\" ");
        table.Get(0, 0).SetColor("#EDEDED").SetStyle("style=\"width:90px;\"")
            .SetContent("<font size=\"3\" color=\"#666666\">尚未排出</font>");
        table.Get(0, 1).SetStyle("style=\"width:340px; text-align:left;\"")
            .SetContent(CardsBox_(player.hand_));
        table.Get(1, 0).SetColor("#EDEDED").SetStyle("style=\"width:90px;\"")
            .SetContent("<font size=\"3\" color=\"#666666\">出牌顺序</font>");
        table.Get(1, 1).SetStyle("style=\"width:340px; text-align:left;\"")
            .SetContent(player.order_.empty() ? string("<font size=\"3\" color=\"#AAAAAA\">（尚未排定）</font>")
                                              : CardsBox_(player.order_));
        html += table.ToString();
        html += "<p align=\"center\" style=\"margin:4px;\"><font size=\"3\" color=\"#666666\">"
                "本回合选定：第 " + (player.group_ < 0 ? string("—") : to_string(player.group_ + 1)) + " 组"
                " ｜ 尚未排出的数字牌之和：" + to_string(RemainSum_(pid)) + "</font></p>";
        return html;
    }

    void TellGroups_(const int pid, const string& title) const
    {
        Global().Tell(Pid_(pid)) << Markdown(GroupsBlock_(pid, title), k_hand_width);
    }

    void TellHand_(const int pid, const string& title) const
    {
        Global().Tell(Pid_(pid)) << Markdown(HandBlock_(pid, title), k_hand_width);
    }

    // ---------- 发牌与回合流转 ----------

    void DealCards_()
    {
        const DealResult deal = MakeDeal(rng_);
        for (uint32_t pid = 0; pid < Global().PlayerNum(); ++pid) {
            players_[pid].Deal(deal[pid]);
        }
        Global().Boardcast() << "裁判已分发了三组新牌，每组为四张 1~10 的数字牌与一张无限牌\n"
                             << "双方十二张数字牌之和相同，牌组内容互不相同";
    }

    void StartRound_()
    {
        for (auto& player : players_) {
            player.ResetRound();
        }
        if (players_[0].NeedDeal() || players_[1].NeedDeal()) {
            DealCards_();
        }
        ++round_;
        base_ = BaseOfRound(round_);
        duel_ = 0;
        pot_ = 0;
        logs_.clear();
        for (auto& record : records_) {
            record = DuelRecord{};
        }
        step_ = Step::SELECT;
        Global().Boardcast() << "本回合基础投入：" << base_ << " 金币\n"
                             << "首个小局的先手：" << At(Pid_(round_first_)) << "，此后每小局先手交替";
        BoardcastBoard_();
    }

    // 回合结束：结算未使用无限牌的奖励，并推进基础投入与先手
    void EndRound_()
    {
        vector<int> bonus_players;
        for (uint32_t pid = 0; pid < Global().PlayerNum(); ++pid) {
            Player& player = players_[pid];
            // 金币已归零的玩家不再获得奖励，与规则中「前提是金币还未归0」一致
            if (player.eliminated_ || player.used_infinite_ || player.coin_ <= 0) {
                continue;
            }
            player.coin_ += BonusOfRound(round_);
            AddLog_(PlayerTag_(static_cast<int>(pid)) + " 整回合未使用无限牌，+" + to_string(BonusOfRound(round_)) +
                    " 金币");
            bonus_players.emplace_back(static_cast<int>(pid));
        }
        // 双方都未使用无限牌时合并为一条消息
        if (!bonus_players.empty()) {
            auto sender = Global().Boardcast();
            if (bonus_players.size() == 1) {
                const int pid = bonus_players.front();
                sender << At(Pid_(pid)) << " 本回合未使用无限牌，获得 " << BonusOfRound(round_)
                       << " 金币（现有 " << players_[pid].coin_ << " 金币）";
            } else {
                sender << "双方本回合均未使用无限牌，各获得 " << BonusOfRound(round_) << " 金币";
                for (const int pid : bonus_players) {
                    sender << "\n" << At(Pid_(pid)) << "：现有 " << players_[pid].coin_ << " 金币";
                }
            }
        }
        Global().Boardcast() << "第 " << round_ << " 回合结算" << Markdown(GetRoundSummary_(), k_board_width);
        {
            auto sender = Global().Boardcast();
            sender << "第 " << round_ << " 回合结束，当前金币：";
            for (uint32_t pid = 0; pid < Global().PlayerNum(); ++pid) {
                sender << "\n" << At(PlayerID(pid)) << "：" << players_[pid].coin_ << " 金币";
            }
        }
        if (round_ >= static_cast<int>(GAME_OPTION(回合上限))) {
            Global().Boardcast() << "已达到 " << GAME_OPTION(回合上限) << " 回合上限，按双方金币多少结算";
            if (players_[0].coin_ == players_[1].coin_) {
                GameOver_(nullopt);
            } else {
                GameOver_(players_[0].coin_ > players_[1].coin_ ? 0 : 1);
            }
            return;
        }
        round_first_ = Opponent_(round_first_);
        Global().Boardcast() << "下一回合的基础投入为 " << BaseOfRound(round_ + 1) << " 金币，未使用无限牌的奖励为 "
                             << BonusOfRound(round_ + 1) << " 金币，首个小局的先手为 " << At(Pid_(round_first_));
    }

    void GameOver_(const optional<int>& winner)
    {
        if (game_over_) {
            return;
        }
        game_over_ = true;
        winner_ = winner;
        Global().StopTimer();
        // 分数规则：正常分出胜负时胜者 1、败者 0，平局双方均为 0；中途退出或超时淘汰的玩家记 -1，其对手不因此获得胜者分，仍为 0
        const bool by_exit = players_[0].eliminated_ || players_[1].eliminated_;
        for (uint32_t pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (players_[pid].eliminated_) {
                scores_[pid] = -1;
            } else if (!by_exit && winner.has_value() && static_cast<int>(pid) == *winner) {
                scores_[pid] = 1;
            } else {
                scores_[pid] = 0;
            }
        }
        // 终局公开双方本回合排出的全部牌，与回合结算图保持一致
        Global().Boardcast() << Markdown(GetFinalBoard_(), k_board_width);
        auto sender = Global().Boardcast();
        if (winner.has_value()) {
            sender << "游戏结束！获胜者为 " << At(Pid_(*winner));
        } else {
            sender << "游戏结束！双方平局";
        }
    }

    // 判定金币是否归零，返回 true 表示整场游戏已结束
    bool CheckCoinZero_()
    {
        const bool zero_first = players_[0].coin_ <= 0;
        const bool zero_second = players_[1].coin_ <= 0;
        if (!zero_first && !zero_second) {
            return false;
        }
        if (zero_first && zero_second) {
            GameOver_(nullopt);
        } else {
            GameOver_(zero_first ? 1 : 0);
        }
        return true;
    }

    // ---------- 小局的投入与结算 ----------

    // 投入金币，剩余不足时全额投入
    void PutCoin_(const int pid, const int amount)
    {
        const int pay = std::min(amount, players_[pid].coin_);
        players_[pid].coin_ -= pay;
        players_[pid].bet_ += pay;
        pot_ += pay;
    }

    // 小局开始：重置累积区、确定先手并投入基础投入
    void BeginDuel_()
    {
        pot_ = 0;
        for (auto& player : players_) {
            player.ResetDuel();
        }
        records_[static_cast<size_t>(duel_)] = DuelRecord{};
        duel_first_ = (round_first_ + duel_) % k_player_num;
        turn_ = duel_first_;
        for (uint32_t pid = 0; pid < Global().PlayerNum(); ++pid) {
            PutCoin_(static_cast<int>(pid), base_);
        }
        Global().Boardcast() << "第 " << round_ << " 回合 · 第 " << (duel_ + 1) << " 局\n"
                             << "基础投入 " << base_ << " 金币，累积 " << pot_ << " 金币\n"
                             << "本小局先手：" << At(Pid_(duel_first_));
    }

    // 揭示结算：先退还无法被对手匹配的超额投入，再比牌分配累积区
    void SettleReveal_()
    {
        DuelRecord& record = records_[static_cast<size_t>(duel_)];
        const int cap = std::min(players_[0].bet_, players_[1].bet_);
        for (uint32_t pid = 0; pid < Global().PlayerNum(); ++pid) {
            const int refund = players_[pid].bet_ - cap;
            if (refund > 0) {
                players_[pid].coin_ += refund;
                players_[pid].bet_ -= refund;
                pot_ -= refund;
                Global().Boardcast() << At(PlayerID(pid)) << " 超出对手的 " << refund << " 金币已退回";
            }
        }
        record.done_ = true;
        record.revealed_ = true;
        record.pot_ = pot_;
        record.bets_[0] = players_[0].bet_;
        record.bets_[1] = players_[1].bet_;

        const optional<Card> first_card = players_[0].DuelCard(duel_);
        const optional<Card> second_card = players_[1].DuelCard(duel_);
        // 双方都有牌时才比牌；一方缺牌只可能出现在其已退出的情形，此时按平局退还投入
        if (!first_card.has_value() || !second_card.has_value()) {
            record.revealed_ = false;
            record.winner_ = -1;
            for (uint32_t pid = 0; pid < Global().PlayerNum(); ++pid) {
                players_[pid].coin_ += players_[pid].bet_;
            }
            pot_ = 0;
            CheckCoinZero_();
            return;
        }
        const DuelResult result = CompareCard(*first_card, *second_card);
        {
            auto sender = Global().Boardcast();
            sender << "第 " << (duel_ + 1) << " 小局揭示：" << PlayerTag_(0) << " " << CardName(*first_card)
                   << "　对　" << PlayerTag_(1) << " " << CardName(*second_card) << "\n";
            if (result == DuelResult::DRAW) {
                record.winner_ = -1;
                for (uint32_t pid = 0; pid < Global().PlayerNum(); ++pid) {
                    players_[pid].coin_ += players_[pid].bet_;
                }
                sender << "本小局平局，双方各自收回投入的 " << cap << " 金币";
            } else {
                record.winner_ = result == DuelResult::FIRST_WIN ? 0 : 1;
                players_[static_cast<size_t>(record.winner_)].coin_ += pot_;
                sender << At(Pid_(record.winner_)) << " 获胜，取得累积区的 " << pot_ << " 金币";
            }
        }
        const string outcome = record.winner_ < 0 ? string("平局")
                                                 : PlayerTag_(record.winner_) + "胜 +" + to_string(pot_) + " 金币";
        AddLog_(PlayerTag_(0) + " " + CardName(*first_card) + " 对 " + PlayerTag_(1) + " " + CardName(*second_card) +
                "，" + outcome);
        pot_ = 0;
        if (!CheckCoinZero_()) {    // 游戏结束时由 GameOver_ 统一广播终局赛况
            BoardcastBoard_();
        }
    }

    // 一方放弃：累积区归对手所有，双方的牌均不揭示
    void SettleFold_(const int pid)
    {
        const int winner = Opponent_(pid);
        players_[pid].folded_ = true;
        DuelRecord& record = records_[static_cast<size_t>(duel_)];
        record.done_ = true;
        record.revealed_ = false;
        record.by_fold_ = true;
        record.winner_ = winner;
        record.pot_ = pot_;
        record.bets_[0] = players_[0].bet_;
        record.bets_[1] = players_[1].bet_;
        players_[static_cast<size_t>(winner)].coin_ += pot_;
        Global().Boardcast() << At(Pid_(pid)) << " 放弃本小局，双方的牌不予揭示\n"
                             << At(Pid_(winner)) << " 取得累积区的 " << pot_ << " 金币";
        AddLog_(PlayerTag_(pid) + " 放弃本小局，" + PlayerTag_(winner) + " +" + to_string(pot_) + " 金币");
        pot_ = 0;
        if (!CheckCoinZero_()) {    // 游戏结束时由 GameOver_ 统一广播终局赛况
            BoardcastBoard_();
        }
    }

    // ---------- 超时与退出 ----------

    void EliminatePlayer_(const int pid, const bool notify_match)
    {
        if (players_[pid].eliminated_) {
            return;
        }
        players_[pid].eliminated_ = true;
        AddLog_(PlayerTag_(pid) + " 已退出游戏");
        if (notify_match) {
            Global().Eliminate(Pid_(pid));
        }
    }

    // 有玩家退出后结算胜负，两人局中只要一方退出即可分出胜负
    void SettleAfterEliminate_()
    {
        const bool out_first = players_[0].eliminated_;
        const bool out_second = players_[1].eliminated_;
        if (out_first && out_second) {
            GameOver_(nullopt);
        } else if (out_first) {
            GameOver_(1);
        } else if (out_second) {
            GameOver_(0);
        }
    }

    // 淘汰所有本阶段未行动的玩家
    void EliminateUnready_()
    {
        for (uint32_t pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (Global().IsReady(PlayerID(pid)) || players_[pid].eliminated_) {
                continue;
            }
            Global().Boardcast() << At(PlayerID(pid)) << " 超时判负。";
            EliminatePlayer_(static_cast<int>(pid), true);
        }
        SettleAfterEliminate_();
    }

    void HandleLeave_(const PlayerID pid)
    {
        Global().Boardcast() << At(pid) << " 强退判负。";
        EliminatePlayer_(static_cast<int>(pid), false);
        SettleAfterEliminate_();
    }

  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, ChildMsgSenderBase& reply)
    {
        if (is_public) {
            Global().Boardcast() << Markdown(GetBoard_(), k_board_width);
        } else {
            reply() << Markdown(GetPrivateBoard_(static_cast<int>(pid)), k_board_width);
        }
        return StageErrCode::OK;
    }

    void FirstStageFsm(SubStageFsmSetter setter) override;

    void NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;
};

// ========== 选组阶段 ==========

class SelectGroupStage : public SubGameStage<>
{
  public:
    SelectGroupStage(MainStage& main_stage)
        : StageFsm(main_stage, "选组阶段",
                MakeStageCommand(*this, "秘密选择本回合使用的牌组",
                    CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &SelectGroupStage::Select_,
                    ArithChecker<int>(1, k_group_num, "牌组编号")))
    {}

    virtual void OnStageBegin() override
    {
        Main().step_ = Step::SELECT;
        bool need_action = false;
        for (uint32_t pid = 0; pid < Global().PlayerNum(); ++pid) {
            Player& player = Main().players_[pid];
            if (player.eliminated_) {
                continue;
            }
            if (const int only = player.OnlyGroup(); only != -1) {
                player.SelectGroup(only);
                Global().SetReady(PlayerID(pid));
                continue;
            }
            need_action = true;
            Global().Tell(PlayerID(pid)) << "请发送「编号」秘密选择本回合使用的牌组";
            Main().TellGroups_(static_cast<int>(pid), "您持有的牌组");
        }
        if (need_action) {
            Global().Boardcast() << "请双方私信发送「1」~「" << k_group_num << "」秘密选定本回合使用的牌组，时限 " << GAME_OPTION(时限) << " 秒";
            Global().StartTimer(GAME_OPTION(时限));
        } else {
            Global().Boardcast() << "双方仅剩一组牌，已自动选定";
        }
    }

  private:
    AtomReqErrCode Select_(const PlayerID pid, const bool is_public, ChildMsgSenderBase& reply, const int group)
    {
        Player& player = Main().players_[pid];
        if (player.groups_[static_cast<size_t>(group - 1)].empty()) {
            reply() << "[错误] 第 " << group << " 组牌已在此前的回合中使用过，请选择其它牌组";
            return StageErrCode::FAILED;
        }
        player.SelectGroup(group - 1);
        reply() << "已选定第 " << group << " 组";
        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        Main().EliminateUnready_();
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Main().HandleLeave_(pid);
        return StageErrCode::CHECKOUT;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, ChildMsgSenderBase& reply) override
    {
        if (Global().IsReady(pid) || Main().players_[pid].eliminated_) {
            return StageErrCode::OK;
        }
        Player& player = Main().players_[pid];
        vector<int> candidates;
        for (int group = 0; group < k_group_num; ++group) {
            if (!player.groups_[static_cast<size_t>(group)].empty()) {
                candidates.emplace_back(group);
            }
        }
        if (candidates.empty()) {
            return StageErrCode::OK;
        }
        player.SelectGroup(candidates[RandInt(Main().rng_, 0, static_cast<uint32_t>(candidates.size()) - 1)]);
        return StageErrCode::READY;
    }
};

// ========== 排序阶段（默认） ==========

class OrderStage : public SubGameStage<>
{
  public:
    OrderStage(MainStage& main_stage)
        : StageFsm(main_stage, "排序阶段",
                MakeStageCommand(*this, "从本回合的牌组中选四张牌并指定四个小局的使用顺序",
                    CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &OrderStage::Order_,
                    FixedSizeRepeatableChecker<CardChecker>(k_duel_num)))
    {}

    virtual void OnStageBegin() override
    {
        Main().step_ = Step::ORDER;
        for (uint32_t pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (Main().players_[pid].eliminated_) {
                continue;
            }
            Global().Tell(PlayerID(pid)) << "请发送四张牌，例如「3 5 7 ∞」「1 1 4 无限」。未被选中的那一张将在本回合结束时消失";
            Main().TellHand_(static_cast<int>(pid), "本回合的牌组");
        }
        Global().Boardcast() << "请双方私信排定本回合四个小局依次使用的牌，时限 " << GAME_OPTION(时限) << " 秒";
        Global().StartTimer(GAME_OPTION(时限));
    }

  private:
    AtomReqErrCode Order_(const PlayerID pid, const bool is_public, ChildMsgSenderBase& reply, const vector<Card>& cards)
    {
        Player& player = Main().players_[pid];
        if (!player.TakeFromHand(cards)) {
            reply() << "[错误] 本回合的牌组中没有足够的这些牌：" << CardsName(cards);
            Main().TellHand_(static_cast<int>(pid), "本回合的牌组");
            return StageErrCode::FAILED;
        }
        player.PushOrder(cards);
        const string tip = player.used_infinite_
                ? string("本回合已使用无限牌，回合结束时不会获得奖励金币")
                : "本回合未使用无限牌，回合结束时将获得 " + to_string(BonusOfRound(Main().round_)) + " 金币";
        reply() << "已排定本回合的出牌顺序：" << CardsName(player.order_) << "\n" << tip;
        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        Main().EliminateUnready_();
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Main().HandleLeave_(pid);
        return StageErrCode::CHECKOUT;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, ChildMsgSenderBase& reply) override
    {
        if (Global().IsReady(pid) || Main().players_[pid].eliminated_) {
            return StageErrCode::OK;
        }
        Player& player = Main().players_[pid];
        vector<Card> cards = player.hand_;
        SeededShuffle(cards.begin(), cards.end(), Main().rng_);
        cards.resize(std::min(static_cast<size_t>(k_duel_num), cards.size()));
        if (!player.TakeFromHand(cards)) {
            return StageErrCode::OK;
        }
        player.PushOrder(cards);
        return StageErrCode::READY;
    }
};

// ========== 选牌阶段（逐张） ==========

class PickCardStage : public SubGameStage<>
{
  public:
    PickCardStage(MainStage& main_stage)
        : StageFsm(main_stage, "选牌阶段",
                MakeStageCommand(*this, "从本回合的牌组中选出本小局使用的牌",
                    CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &PickCardStage::Pick_,
                    CardChecker()))
    {}

    virtual void OnStageBegin() override
    {
        Main().step_ = Step::PICK;
        for (uint32_t pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (Main().players_[pid].eliminated_) {
                continue;
            }
            Global().Tell(PlayerID(pid)) << "请选择一张牌，例如「7」「∞」「无限」";
            Main().TellHand_(static_cast<int>(pid), "本回合的牌组");
        }
        Global().Boardcast() << "第 " << (Main().duel_ + 1) << " 小局，请双方私信秘密选出本小局使用的牌，时限 "
                             << GAME_OPTION(时限) << " 秒";
        Global().StartTimer(GAME_OPTION(时限));
    }

  private:
    AtomReqErrCode Pick_(const PlayerID pid, const bool is_public, ChildMsgSenderBase& reply, const Card& card)
    {
        Player& player = Main().players_[pid];
        if (!player.TakeFromHand(vector<Card>{card})) {
            reply() << "[错误] 本回合的牌组中没有 " << CardName(card);
            Main().TellHand_(static_cast<int>(pid), "本回合的牌组");
            return StageErrCode::FAILED;
        }
        player.PushOrder(vector<Card>{card});
        reply() << "本小局将使用 " << CardName(card);
        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        Main().EliminateUnready_();
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Main().HandleLeave_(pid);
        return StageErrCode::CHECKOUT;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, ChildMsgSenderBase& reply) override
    {
        if (Global().IsReady(pid) || Main().players_[pid].eliminated_) {
            return StageErrCode::OK;
        }
        Player& player = Main().players_[pid];
        if (player.hand_.empty()) {
            return StageErrCode::OK;
        }
        const Card card = player.hand_[RandInt(Main().rng_, 0, static_cast<uint32_t>(player.hand_.size()) - 1)];
        player.TakeFromHand(vector<Card>{card});
        player.PushOrder(vector<Card>{card});
        return StageErrCode::READY;
    }
};

// ========== 对局阶段 ==========

class BetStage : public SubGameStage<>
{
  public:
    BetStage(MainStage& main_stage)
        : StageFsm(main_stage, "第 " + to_string(main_stage.duel_ + 1) + " 小局",
                MakeStageCommand(*this, "不追加投入，把行动权交给对手", &BetStage::Check_,
                    VoidChecker("观望", "check", "ck")),
                MakeStageCommand(*this, "追加投入指定数量的金币", &BetStage::Raise_,
                    VoidChecker("加码", "raise", "r"), ArithChecker<int>(1, 999999, "金币")),
                MakeStageCommand(*this, "补齐与对手相同的投入，随即揭示双方的牌", &BetStage::Call_,
                    VoidChecker("跟进", "call", "cl")),
                MakeStageCommand(*this, "放弃本小局，累积区的金币全部归对手", &BetStage::Fold_,
                    VoidChecker("放弃", "fold", "f")))
    {}

    virtual void OnStageBegin() override
    {
        Main().step_ = Step::BET;
        Main().BeginDuel_();
        // 任一方在投入基础投入后已无剩余金币，双方都无法再追加，直接揭示
        if (Main().players_[0].coin_ == 0 || Main().players_[1].coin_ == 0) {
            Global().Boardcast() << "已有玩家全额投入，本小局无法继续追加，直接揭示";
            Reveal_();
            return;
        }
        Main().SetReadyForTurn_();
        BoardcastAction_();
        Global().StartTimer(GAME_OPTION(时限));
    }

  private:
    bool finished_{false};      // 本小局是否已结算
    int acted_{0};              // 自上一次加码以来连续行动的人数，双方均观望即揭示

    // 校验是否轮到该玩家行动
    bool CheckTurn_(const PlayerID pid, ChildMsgSenderBase& reply) const
    {
        if (Main().game_over_ || finished_) {
            reply() << "[错误] 本小局已经结束";
            return false;
        }
        if (Main().players_[pid].eliminated_) {
            reply() << "[错误] 您已退出本场游戏，无法继续行动";
            return false;
        }
        if (Global().IsReady(pid) || static_cast<int>(pid) != Main().turn_) {
            reply() << "[错误] 现在不是您的回合，当前行动方为 " << At(MainStage::Pid_(Main().turn_));
            return false;
        }
        return true;
    }

    // 当前行动方需要补齐的差额
    int Gap_(const int pid) const
    {
        return Main().players_[MainStage::Opponent_(pid)].bet_ - Main().players_[pid].bet_;
    }

    void BoardcastAction_() const
    {
        const int pid = Main().turn_;
        const int gap = Gap_(pid);
        auto sender = Global().Boardcast();
        sender << "轮到 " << At(MainStage::Pid_(pid)) << " 行动，累积共 " << Main().pot_ << " 金币\n";
        if (gap > 0) {
            sender << "投入高出 " << gap << " 金币，可「跟进」补齐并揭示、「加码 数量」继续追加，或「放弃」本小局";
        } else {
            sender << "投入相同，可发送「观望」不追加、「加码 数量」追加投入，或「放弃」本小局";
        }
        sender << "\n剩余金币：" << Main().players_[pid].coin_ << "，时限 " << GAME_OPTION(时限) << " 秒";
    }

    void SetReadyAll_()
    {
        for (uint32_t pid = 0; pid < Global().PlayerNum(); ++pid) {
            Global().SetReady(PlayerID(pid));
        }
    }

    void Reveal_()
    {
        Main().SettleReveal_();
        finished_ = true;
        SetReadyAll_();
    }

    // 把行动权交给对手。对手已无金币可投入时直接揭示
    AtomReqErrCode SwitchTurn_()
    {
        Main().turn_ = MainStage::Opponent_(Main().turn_);
        if (Main().players_[Main().turn_].coin_ == 0) {
            Global().Boardcast() << At(MainStage::Pid_(Main().turn_)) << " 已全额投入，无法继续行动，本小局直接揭示";
            Reveal_();
            return StageErrCode::CHECKOUT;
        }
        Main().SetReadyForTurn_();
        BoardcastAction_();
        Global().StartTimer(GAME_OPTION(时限));
        return StageErrCode::OK;
    }

    AtomReqErrCode Check_(const PlayerID pid, const bool is_public, ChildMsgSenderBase& reply)
    {
        if (!CheckTurn_(pid, reply)) {
            return StageErrCode::FAILED;
        }
        if (Gap_(static_cast<int>(pid)) > 0) {
            reply() << "[错误] 对手的投入高于您，只能【跟进】【加码】或【放弃】";
            return StageErrCode::FAILED;
        }
        Global().Boardcast() << At(pid) << " 选择观望";
        Main().AddLog_(MainStage::PlayerTag_(static_cast<int>(pid)) + " 观望");
        if (++acted_ >= k_player_num) {      // 双方接连观望，进入揭示
            Reveal_();
            return StageErrCode::CHECKOUT;
        }
        return SwitchTurn_();
    }

    AtomReqErrCode Raise_(const PlayerID pid, const bool is_public, ChildMsgSenderBase& reply, const int amount)
    {
        if (!CheckTurn_(pid, reply)) {
            return StageErrCode::FAILED;
        }
        const int me = static_cast<int>(pid);
        const int opponent = MainStage::Opponent_(me);
        if (Main().players_[opponent].coin_ == 0) {
            reply() << "[错误] 对手已全额投入，超出的部分终将退回，只能【跟进】或【放弃】";
            return StageErrCode::FAILED;
        }
        const int gap = Gap_(me);
        if (amount <= gap) {
            reply() << "[错误] 需要先补齐差额 " << gap << " 金币才算追加，请至少投入 " << (gap + 1) << " 金币";
            return StageErrCode::FAILED;
        }
        if (amount > Main().players_[me].coin_) {
            reply() << "[错误] 您只剩 " << Main().players_[me].coin_ << " 金币，无法投入 " << amount << " 金币";
            return StageErrCode::FAILED;
        }
        Main().PutCoin_(me, amount);
        Global().Boardcast() << At(pid) << " 追加投入 " << amount << " 金币"
                             << (Main().players_[me].coin_ == 0 ? "（已全额投入）" : "")
                             << "，累积区共 " << Main().pot_ << " 金币";
        Main().AddLog_(MainStage::PlayerTag_(me) + " 加码 " + to_string(amount) + " 金币");
        acted_ = 1;         // 加码后对手必须回应
        return SwitchTurn_();
    }

    AtomReqErrCode Call_(const PlayerID pid, const bool is_public, ChildMsgSenderBase& reply)
    {
        if (!CheckTurn_(pid, reply)) {
            return StageErrCode::FAILED;
        }
        const int me = static_cast<int>(pid);
        const int gap = Gap_(me);
        if (gap <= 0) {
            reply() << "[错误] 双方投入相同，无需跟进，请选择【观望】或【加码】";
            return StageErrCode::FAILED;
        }
        const int pay = std::min(gap, Main().players_[me].coin_);
        Main().PutCoin_(me, pay);
        Global().Boardcast() << At(pid) << " 跟进 " << pay << " 金币"
                             << (pay < gap ? "（金币不足，已全额投入）" : "");
        Main().AddLog_(MainStage::PlayerTag_(me) + " 跟进 " + to_string(pay) + " 金币");
        Reveal_();
        return StageErrCode::CHECKOUT;
    }

    AtomReqErrCode Fold_(const PlayerID pid, const bool is_public, ChildMsgSenderBase& reply)
    {
        if (!CheckTurn_(pid, reply)) {
            return StageErrCode::FAILED;
        }
        Main().SettleFold_(static_cast<int>(pid));
        finished_ = true;
        SetReadyAll_();
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnStageOver() override { return StageErrCode::CHECKOUT; }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        if (Main().game_over_ || finished_) {
            return StageErrCode::CHECKOUT;
        }
        Main().EliminateUnready_();
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Main().HandleLeave_(pid);
        return StageErrCode::CHECKOUT;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, ChildMsgSenderBase& reply) override
    {
        // 非当前行动方已被标记为已完成，直接返回 OK 让推演循环跳过
        if (Main().game_over_ || finished_ || Global().IsReady(pid) || Main().players_[pid].eliminated_) {
            return StageErrCode::OK;
        }
        const AtomReqErrCode rc = Gap_(static_cast<int>(pid)) > 0 ? Call_(pid, false, reply)
                                                                 : Check_(pid, false, reply);
        if (rc == StageErrCode::CHECKOUT) {
            return StageErrCode::CHECKOUT;
        }
        if (rc == StageErrCode::FAILED) {
            return StageErrCode::OK;        // 行动异常时返回 OK，避免推演循环无法退出
        }
        return StageErrCode::FAILED;        // 行动成功但小局尚未结束，使推演循环继续轮询对手
    }
};

// ========== 回合阶段 ==========

class RoundStage : public SubGameStage<SelectGroupStage, OrderStage, PickCardStage, BetStage>
{
  public:
    RoundStage(MainStage& main_stage)
        : StageFsm(main_stage, "第 " + to_string(main_stage.round_) + " 回合")
    {}

  private:
    // 进入当前小局：逐张模式下先秘密选牌，默认模式下直接对局
    void EmplaceDuel_(SubStageFsmSetter& setter)
    {
        if (GAME_OPTION(逐张)) {
            setter.Emplace<PickCardStage>(Main());
        } else {
            setter.Emplace<BetStage>(Main());
        }
    }

    void FirstStageFsm(SubStageFsmSetter setter) override
    {
        setter.Emplace<SelectGroupStage>(Main());
    }

    void NextStageFsm(SelectGroupStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override
    {
        if (Main().game_over_) {
            return;
        }
        if (GAME_OPTION(逐张)) {
            setter.Emplace<PickCardStage>(Main());
        } else {
            setter.Emplace<OrderStage>(Main());
        }
    }

    void NextStageFsm(OrderStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override
    {
        if (Main().game_over_) {
            return;
        }
        setter.Emplace<BetStage>(Main());
    }

    void NextStageFsm(PickCardStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override
    {
        if (Main().game_over_) {
            return;
        }
        setter.Emplace<BetStage>(Main());
    }

    void NextStageFsm(BetStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override
    {
        if (Main().game_over_) {
            return;
        }
        ++Main().duel_;
        if (Main().duel_ >= k_duel_num) {   // 本回合的四个小局全部结束
            return;
        }
        EmplaceDuel_(setter);
    }
};

void MainStage::FirstStageFsm(SubStageFsmSetter setter)
{
    Global().Boardcast() << "游戏开始！\n"
                         << "双方各持 " << GAME_OPTION(金币) << " 金币，先让对手金币归零者获胜。\n"
                         << "每回合分为四个小局，每三个回合重新分发三组牌。\n"
                         << "基础投入按回合递增：第 1~6 回合为 2~7，第 7~12 回合固定为 " << k_base_flat
                         << "，第 " << k_base_high_round << " 回合起固定为 " << k_base_high << "。\n"
                         << "整回合未使用“无限牌”者，回合结束时获得奖励金币：" << k_bonus_low << "（第 1~6 回合）/ "
                         << k_bonus_flat << "（第 7~12 回合）/ " << k_bonus_high << "（第 "
                         << k_base_high_round << " 回合起）。";
    round_first_ = static_cast<int>(RandInt(rng_, 0, Global().PlayerNum() - 1));
    StartRound_();
    setter.Emplace<RoundStage>(*this);
}

void MainStage::NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    if (game_over_) {
        return;
    }
    EndRound_();
    if (game_over_) {
        return;
    }
    StartRound_();
    setter.Emplace<RoundStage>(*this);
}

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot
