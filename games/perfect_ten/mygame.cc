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
#include <optional>
#include <string>
#include <string_view>
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
    .name_ = "十全牌",
    .developer_ = "铁蛋",
    .description_ = "抽牌凑齐两个公共任务，再连出清空手牌，最先累计3分者获胜。",
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 3; }
uint32_t Multiple(const CustomOptions& options) {
    return 1;
}
const MutableGenericOptions k_default_generic_options{
    .is_formal_{false},
};
const std::vector<RuleCommand> k_rule_commands = {};

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options)
            {
                generic_options.bench_computers_to_player_num_ = 3;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() < 2 || generic_options_readonly.PlayerNum() > 3) {
        reply() << "该游戏为 2 至 3 人游戏，当前玩家数为 " << generic_options_readonly.PlayerNum();
        return false;
    }
    const uint32_t min_require = GET_OPTION_VALUE(game_options, 任务张数下限);
    const uint32_t max_require = GET_OPTION_VALUE(game_options, 任务张数上限);
    const uint32_t min_sum = GET_OPTION_VALUE(game_options, 任务总张数下限);
    const uint32_t max_sum = GET_OPTION_VALUE(game_options, 任务总张数上限);
    if (min_require > max_require) {
        reply() << "[错误] 任务张数下限（" << min_require << "）不能大于任务张数上限（" << max_require << "）";
        return false;
    }
    if (min_sum > max_sum) {
        reply() << "[错误] 任务总张数下限（" << min_sum << "）不能大于任务总张数上限（" << max_sum << "）";
        return false;
    }
    // 两个任务的张数之和只可能落在 [2*下限, 2*上限] 内，与总张数区间无交集时将无法生成任务
    if (min_require * 2 > max_sum || max_require * 2 < min_sum) {
        reply() << "[错误] 单任务张数区间 " << min_require << "~" << max_require << " 与总张数区间 "
                << min_sum << "~" << max_sum << " 无交集，无法生成任务。两个任务的张数之和只能落在 "
                << (min_require * 2) << "~" << (max_require * 2) << " 之间";
        return false;
    }
    // 玩家在任务阶段最多持有 11 张手牌，总张数超过该值时任务将永远无法完成
    if (min_sum > static_cast<uint32_t>(k_init_hand_num + 1)) {
        reply() << "[错误] 任务总张数下限（" << min_sum << "）超过玩家在任务阶段的最大手牌数（"
                << (k_init_hand_num + 1) << "），任务将无法完成";
        return false;
    }
    return true;
}

static constexpr size_t k_max_log_num = 8;          // 赛况图中展示的记录条数
static constexpr uint32_t k_board_width = 700;      // 赛况图的宽度
static constexpr uint32_t k_hand_width = 440;       // 手牌图的宽度
static constexpr uint32_t k_hand_cards_per_row = 5; // 手牌图每行展示的卡牌数

// ========== 指令校验器 ==========

class AreaChecker : public MsgArgChecker<AreaArg>
{
  public:
    virtual ~AreaChecker() {}

    virtual std::string FormatInfo() const override { return "<玩家编号>-<任务区编号>"; }

    virtual std::string EscapedFormatInfo() const override
    {
        return string(HTML_ESCAPE_LT) + "玩家编号" + HTML_ESCAPE_GT + "-" + HTML_ESCAPE_LT + "任务区编号" + HTML_ESCAPE_GT;
    }

    virtual std::string ColoredFormatInfo() const override
    {
        return HTML_COLOR_FONT_HEADER(green) + EscapedFormatInfo() + HTML_FONT_TAIL;
    }

    virtual std::string ExampleInfo() const override { return "1-1"; }

    virtual std::optional<AreaArg> Check(MsgReader& reader) const override
    {
        if (!reader.HasNext()) {
            return std::nullopt;
        }
        const std::string str = reader.NextArg();
        if (str.size() != 3 || str[1] != '-') {
            return std::nullopt;
        }
        if (str[0] < '1' || str[0] > '0' + k_max_player_num || str[2] < '1' || str[2] > '0' + k_area_num) {
            return std::nullopt;
        }
        return AreaArg{str[0] - '1', str[2] - '1'};
    }

    virtual std::string ArgString(const AreaArg& value) const override
    {
        return to_string(value.pid_ + 1) + "-" + to_string(value.area_ + 1);
    }
};

// 无分隔符的牌组，如「红2红4红10」
class CardGroupChecker : public MsgArgChecker<vector<Card>>
{
  public:
    CardGroupChecker(string meaning, string example) : meaning_(std::move(meaning)), example_(std::move(example)) {}
    virtual ~CardGroupChecker() {}

    virtual std::string FormatInfo() const override { return "<" + meaning_ + ">"; }

    virtual std::string EscapedFormatInfo() const override { return HTML_ESCAPE_LT + meaning_ + HTML_ESCAPE_GT; }

    virtual std::string ColoredFormatInfo() const override
    {
        return HTML_COLOR_FONT_HEADER(green) + EscapedFormatInfo() + HTML_FONT_TAIL;
    }

    virtual std::string ExampleInfo() const override { return example_; }

    virtual std::optional<vector<Card>> Check(MsgReader& reader) const override
    {
        if (!reader.HasNext()) {
            return std::nullopt;
        }
        return ParseCards(reader.NextArg());
    }

    virtual std::string ArgString(const vector<Card>& value) const override { return CardsName(value); }

  private:
    const string meaning_;
    const string example_;
};

// ========== GAME STAGES ==========

class MainStage : public MainGameStage<>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility),
                MakeStageCommand(*this, "查看当前赛况（私信时附带自己的手牌）", &MainStage::Status_,
                    VoidChecker("赛况")),
                MakeStageCommand(*this, "跳过当前的可选步骤（任务阶段 / 连出阶段）", &MainStage::Pass_,
                    VoidChecker("跳过", "pass", "PASS")),
                MakeStageCommand(*this, "抽牌阶段：P 从抽牌堆抽牌，Q 拾取弃牌堆顶的牌", &MainStage::Draw_,
                    AlterChecker<int>({{"P", 0}, {"p", 0}, {"Q", 1}, {"q", 1}})),
                MakeStageCommand(*this, "连出阶段：向任意玩家已完成的任务区追加卡牌", &MainStage::Chain_,
                    AreaChecker(), CardGroupChecker("追加的牌", "红5")),
                MakeStageCommand(*this, "任务阶段：一次性向自己的两个任务区打出卡牌", &MainStage::Task_,
                    CardGroupChecker("任务1的牌", "红2红4红10"), CardGroupChecker("任务2的牌", "黄5红6灰7灰8")),
                MakeStageCommand(*this, "弃牌阶段：弃掉一张手牌并结束本回合", &MainStage::Discard_,
                    CardGroupChecker("弃掉的牌", "蓝3")))
    {
        players_.resize(Global().PlayerNum());
        rng_ = MakeRng(GAME_OPTION(种子));
    }

    virtual int64_t PlayerScore(const PlayerID pid) const override { return players_[pid].score_; }

  private:
    // ---------- 状态 ----------

    vector<Player> players_;
    mt19937 rng_;
    array<Task, k_area_num> tasks_{};   // 本局的两个全局任务，每局开始时由 GenerateTasks_ 生成
    vector<Card> deck_;             // 抽牌堆，末尾为顶部
    vector<Card> discard_;          // 弃牌堆，末尾为顶部
    vector<string> logs_;           // 本局的操作记录
    int round_{0};                  // 当前局数，从 1 开始
    Step step_{Step::DRAW};
    uint32_t turn_{0};              // 当前行动玩家
    uint32_t round_first_{0};       // 本局先手玩家
    optional<uint32_t> round_winner_;   // 本局清空手牌的玩家
    vector<uint32_t> winners_;          // 整场游戏的胜者，并列时可能有多人
    bool game_over_{false};

    // ---------- 通用查询 ----------

    uint32_t AliveNum_() const
    {
        uint32_t num = 0;
        for (const auto& player : players_) {
            if (!player.eliminated_) {
                ++num;
            }
        }
        return num;
    }

    // 顺时针方向的下一位未退出玩家，全部退出时返回原玩家
    uint32_t NextAlive_(const uint32_t pid) const
    {
        const uint32_t num = Global().PlayerNum();
        for (uint32_t i = 1; i <= num; ++i) {
            const uint32_t next = (pid + i) % num;
            if (!players_[next].eliminated_) {
                return next;
            }
        }
        return pid;
    }

    // 赛况图记录中的玩家标识，图片内无法渲染 @，改用与玩家列表一致的编号
    static string PlayerTag_(const uint32_t pid) { return to_string(pid + 1) + "号"; }

    void AddLog_(const string& log)
    {
        logs_.emplace_back(log);
    }

    // 仅当前行动玩家处于未完成状态，其余玩家标记为已完成以免框架等待其行动
    void SetReadyForTurn_()
    {
        Global().ClearReady();
        for (uint32_t pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (pid != turn_ && !players_[pid].eliminated_) {
                Global().SetReady(PlayerID(pid));
            }
        }
    }

    // 校验是否轮到该玩家在指定步骤行动
    bool CheckAction_(const PlayerID pid, MsgSenderBase& reply, const Step step) const
    {
        if (game_over_) {
            reply() << "[错误] 游戏已经结束";
            return false;
        }
        if (players_[pid].eliminated_) {
            reply() << "[错误] 您已退出本场游戏，无法继续行动";
            return false;
        }
        if (pid != turn_) {
            reply() << "[错误] 现在不是您的回合，当前行动玩家为 " << At(PlayerID(turn_));
            return false;
        }
        if (step_ != step) {
            reply() << "[错误] 当前为【" << k_step_names[static_cast<int>(step_)] << "】，无法执行该操作";
            return false;
        }
        return true;
    }

    // ---------- 图片 UI ----------

    // 单张卡牌
    static string CardBox_(const Card& card)
    {
        return "<div style=\"display:inline-block; min-width:56px; margin:2px; padding:5px 4px; border-radius:8px; "
               "background:" + string(k_color_backs[static_cast<int>(card.color_)]) + "; color:#FFFFFF; "
               "font-size:20px; font-weight:bold; text-align:center;\">" + CardName(card) + "</div>";
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

    // 本局的两个全局任务
    string TaskTable_() const
    {
        html::Table table(k_area_num, 3);
        table.SetTableStyle(" align=\"center\" cellpadding=\"4\" cellspacing=\"2\" ");
        for (int area = 0; area < k_area_num; ++area) {
            table.Get(area, 0).SetColor("#EDEDED").SetStyle("style=\"width:80px;\"")
                .SetContent("<font size=\"4\"><b>任务" + to_string(area + 1) + "</b></font>");
            table.Get(area, 1).SetColor("#F6F6F6").SetStyle("style=\"width:110px;\"")
                .SetContent("<font size=\"5\" color=\"#2F5597\"><b>" + tasks_[area].Name() + "</b></font>");
            table.Get(area, 2).SetColor("#F6F6F6").SetStyle("style=\"width:340px; text-align:left;\"")
                .SetContent("<font size=\"3\" color=\"#666666\">至少 " + to_string(tasks_[area].require_) + " 张：" +
                        tasks_[area].Desc() + "</font>");
        }
        return table.ToString();
    }

    // 抽牌堆、弃牌堆与当前行动信息
    string DeskTable_() const
    {
        html::Table table(2, 3);
        table.SetTableStyle(" align=\"center\" cellpadding=\"4\" cellspacing=\"2\" ");
        table.Get(0, 0).SetColor("#EDEDED").SetStyle("style=\"width:150px;\"")
            .SetContent("<font size=\"3\" color=\"#666666\">抽牌堆</font>");
        table.Get(0, 1).SetColor("#EDEDED").SetStyle("style=\"width:150px;\"")
            .SetContent("<font size=\"3\" color=\"#666666\">弃牌堆顶</font>");
        table.Get(0, 2).SetColor("#EDEDED").SetStyle("style=\"width:230px;\"")
            .SetContent("<font size=\"3\" color=\"#666666\">当前行动</font>");
        table.Get(1, 0).SetContent("<font size=\"4\"><b>" + to_string(deck_.size()) + "</b> 张</font>");
        table.Get(1, 1).SetContent(discard_.empty() ? "<font size=\"3\" color=\"#AAAAAA\">（空）</font>"
                                                    : CardBox_(discard_.back()));
        table.Get(1, 2).SetContent(game_over_
                ? "<font size=\"4\" color=\"#C00000\"><b>游戏已结束</b></font>"
                : "<font size=\"4\">" + to_string(turn_ + 1) + "号 · " +
                        k_step_names[static_cast<int>(step_)] + "</font>");
        return table.ToString();
    }

    // 所有玩家的信息与任务区，统一放在一张表中以保证各列对齐
    string PlayerTable_() const
    {
        const uint32_t player_num = Global().PlayerNum();
        html::Table table(player_num * (1 + k_area_num), 4);
        table.SetTableStyle(" align=\"center\" cellpadding=\"4\" cellspacing=\"2\" ");
        for (uint32_t pid = 0; pid < player_num; ++pid) {
            const Player& player = players_[pid];
            const uint32_t row = pid * (1 + k_area_num);
            string tag;
            if (player.eliminated_) {
                tag = "<font size=\"3\" color=\"#999999\">【已退出】</font>";
            } else if (player.task_done_) {
                tag = "<font size=\"3\" color=\"#00875A\">【已完成】</font>";
            }
            const string head_color = player.eliminated_ ? "#E8E8E8"
                                                         : (!game_over_ && pid == turn_ ? "#FFF2CC" : "#F6F6F6");
            table.Get(row, 0).SetColor(head_color).SetStyle("style=\"width:60px; text-align:right;\"")
                .SetContent("<font size=\"4\"><b>" + to_string(pid + 1) + "号</b></font>");
            table.Get(row, 1).SetColor(head_color).SetStyle("style=\"width:44px;\"")
                .SetContent(Global().PlayerAvatar(PlayerID(pid), 30));
            table.Get(row, 2).SetColor(head_color).SetStyle("style=\"width:300px; text-align:left;\"")
                .SetContent("<font size=\"4\">" + Global().PlayerName(PlayerID(pid)) + "</font> " + tag);
            table.Get(row, 3).SetColor(head_color).SetStyle("style=\"width:190px; text-align:left;\"")
                .SetContent("<font size=\"4\" color=\"#7F5093\"><b>" + to_string(player.score_) + " 分</b></font>"
                        "<font size=\"3\" color=\"#666666\"> ｜ 手牌 " + to_string(player.hand_.size()) + " 张</font>");
            for (uint32_t area = 0; area < k_area_num; ++area) {
                table.Get(row + 1 + area, 0).SetColor("#FAFAFA")
                    .SetContent("<font size=\"3\" color=\"#666666\">任务" + to_string(area + 1) + "</font>");
                table.MergeRight(row + 1 + area, 1, 3);
                table.Get(row + 1 + area, 1).SetStyle("style=\"text-align:left;\"")
                    .SetContent(CardsBox_(player.areas_[area]));
            }
        }
        return table.ToString();
    }

    // 本局最近的操作记录
    string LogTable_() const
    {
        if (logs_.empty()) {
            return "";
        }
        const size_t begin = logs_.size() > k_max_log_num ? logs_.size() - k_max_log_num : 0;
        html::Table table(static_cast<uint32_t>(logs_.size() - begin + 1), 1);
        table.SetTableStyle(" align=\"center\" cellpadding=\"3\" cellspacing=\"0\" ");
        table.Get(0, 0).SetColor("#EDEDED").SetStyle("style=\"width:604px;\"")
            .SetContent("<font size=\"3\" color=\"#666666\">本局记录</font>");
        for (size_t i = begin; i < logs_.size(); ++i) {
            table.Get(static_cast<uint32_t>(i - begin + 1), 0).SetStyle("style=\"width:604px; text-align:left;\"")
                .SetContent("<font size=\"3\">" + logs_[i] + "</font>");
        }
        return table.ToString();
    }

    // 完整赛况
    string GetBoard_() const
    {
        string html = "<h2 align=\"center\" style=\"margin:6px;\">十全牌 · 第 " + to_string(round_) + " 局</h2>";
        html += TaskTable_();
        html += DeskTable_();
        html += PlayerTable_();
        html += LogTable_();
        return html;
    }

    // 手牌牌组，每行固定展示 k_hand_cards_per_row 张
    static string HandCardsTable_(const vector<Card>& cards)
    {
        if (cards.empty()) {
            return "<font size=\"3\" color=\"#AAAAAA\">（空）</font>";
        }
        const uint32_t total = static_cast<uint32_t>(cards.size());
        const uint32_t rows = (total + k_hand_cards_per_row - 1) / k_hand_cards_per_row;
        html::Table table(rows, k_hand_cards_per_row);
        table.SetTableStyle(" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" ");
        for (uint32_t i = 0; i < rows * k_hand_cards_per_row; ++i) {
            auto& box = table.Get(i / k_hand_cards_per_row, i % k_hand_cards_per_row);
            box.SetStyle("style=\"width:72px;\"");
            if (i < total) {
                box.SetContent(CardBox_(cards[i]));
            }
        }
        return table.ToString();
    }

    // 私人手牌，与公屏赛况使用同一套卡牌样式
    string HandBlock_(const uint32_t pid, const string& title) const
    {
        const Player& player = players_[pid];
        return "<h3 align=\"center\" style=\"margin:6px;\">" + title + "（" + to_string(player.hand_.size()) +
               " 张）</h3>" + HandCardsTable_(player.hand_);
    }

    // 私人赛况：公开棋盘 + 自己的手牌
    string GetHandBoard_(const uint32_t pid) const { return GetBoard_() + HandBlock_(pid, "您的手牌"); }

    void BoardcastBoard_() const { Global().Boardcast() << Markdown(GetBoard_(), k_board_width); }

    // 私信手牌图片
    Markdown HandImage_(const uint32_t pid, const string& title) const
    {
        return Markdown(HandBlock_(pid, title), k_hand_width);
    }

    // ---------- 局与回合的流转 ----------

    virtual void OnStageBegin() override
    {
        Global().Boardcast() << "【十全牌】游戏开始！\n"
                             << "共 " << Global().PlayerNum() << " 位玩家，最先累计 " << k_target_score << " 分者直接获胜。\n"
                             << "单局完成两个基础任务得 1 分，单局打空手牌额外得 1 分。\n"
                             << "每个步骤思考时限 " << GAME_OPTION(时限) << " 秒，超时或中途退出的玩家将被淘汰。";
        round_first_ = RandInt(rng_, 0, Global().PlayerNum() - 1);
        StartRound_();
    }

    // 生成本局的两个全局任务
    void GenerateTasks_()
    {
        while (true) {
            const int type0 = static_cast<int>(RandInt(rng_, 0, k_task_type_num - 1));
            const int type1 = static_cast<int>(RandInt(rng_, 0, k_task_type_num - 1));
            if (type0 == type1) {   // 两个任务类型不重复
                continue;
            }
            const bool is_odd_even =
                (type0 == static_cast<int>(TaskType::ODD) && type1 == static_cast<int>(TaskType::EVEN)) ||
                (type0 == static_cast<int>(TaskType::EVEN) && type1 == static_cast<int>(TaskType::ODD));
            if (is_odd_even) {      // 奇数与偶数不会同时出现
                continue;
            }
            tasks_[0].type_ = static_cast<TaskType>(type0);
            tasks_[1].type_ = static_cast<TaskType>(type1);
            break;
        }
        const uint32_t min_require = GAME_OPTION(任务张数下限);
        const uint32_t max_require = GAME_OPTION(任务张数上限);
        const uint32_t min_sum = GAME_OPTION(任务总张数下限);
        const uint32_t max_sum = GAME_OPTION(任务总张数上限);
        while (true) {
            const uint32_t require0 = RandInt(rng_, min_require, max_require);
            const uint32_t require1 = RandInt(rng_, min_require, max_require);
            const uint32_t sum = require0 + require1;
            if (sum < min_sum || sum > max_sum) {
                continue;
            }
            tasks_[0].require_ = static_cast<int>(require0);
            tasks_[1].require_ = static_cast<int>(require1);
            break;
        }
    }

    void StartRound_()
    {
        ++round_;
        logs_.clear();
        round_winner_ = nullopt;
        GenerateTasks_();
        deck_ = MakeDeck();
        SeededShuffle(deck_.begin(), deck_.end(), rng_);
        discard_.clear();
        for (uint32_t pid = 0; pid < Global().PlayerNum(); ++pid) {
            players_[pid].ResetRound();
            if (players_[pid].eliminated_) {
                continue;
            }
            for (int i = 0; i < k_init_hand_num; ++i) {
                players_[pid].hand_.emplace_back(deck_.back());
                deck_.pop_back();
            }
            players_[pid].SortHand();
        }
        discard_.emplace_back(deck_.back());    // 翻开弃牌堆的首张卡牌
        deck_.pop_back();
        if (players_[round_first_].eliminated_) {
            round_first_ = NextAlive_(round_first_);
        }
        turn_ = round_first_;
        Global().Boardcast() << "========== 第 " << round_ << " 局开始 ==========\n"
                             << "任务1：" << tasks_[0].Name() << "（至少 " << tasks_[0].require_ << " 张，" << tasks_[0].Desc() << "）\n"
                             << "任务2：" << tasks_[1].Name() << "（至少 " << tasks_[1].require_ << " 张，" << tasks_[1].Desc() << "）\n"
                             << "本局先手：" << At(PlayerID(round_first_));
        for (uint32_t pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (!players_[pid].eliminated_) {
                TellHand_(pid, "第 " + to_string(round_) + " 局 · 您的手牌");
            }
        }
        StartTurn_();
    }

    void StartTurn_()
    {
        if (game_over_) {
            return;
        }
        // 回合开始时抽牌堆已空，则本局直接结束
        if (deck_.empty()) {
            AddLog_("抽牌堆已空，本局结束");
            EndRound_();
            return;
        }
        step_ = Step::DRAW;
        SetReadyForTurn_();
        BoardcastBoard_();
        Global().Boardcast() << "轮到 " << At(PlayerID(turn_)) << " 行动\n"
                             << "【抽牌阶段】请发送 P 从抽牌堆抽牌，或发送 Q 拾取弃牌堆顶的 "
                             << CardName(discard_.back());
        TellHand_(turn_, "您的手牌");
        Global().StartTimer(GAME_OPTION(时限));
    }

    void TellHand_(const uint32_t pid, const string& title) const
    {
        Global().Tell(PlayerID(pid)) << HandImage_(pid, title);
    }

    void ToTaskStep_()
    {
        if (players_[turn_].task_done_) {    // 已完成基础任务的玩家自动跳过任务阶段
            ToChainStep_();
            return;
        }
        step_ = Step::TASK;
        Global().Boardcast() << At(PlayerID(turn_)) << " 进入【任务阶段】\n"
                             << "可一次性向两个任务区打出卡牌（如：红2红4红10 黄5红6灰7灰8），或发送「跳过」";
        Global().StartTimer(GAME_OPTION(时限));
    }

    void ToChainStep_()
    {
        if (!players_[turn_].task_done_) {   // 未完成基础任务的玩家无法连出
            ToDiscardStep_();
            return;
        }
        step_ = Step::CHAIN;
        Global().Boardcast() << At(PlayerID(turn_)) << " 进入【连出阶段】\n"
                             << "可向任意玩家已完成的任务区追加卡牌（如：1-1 红5），或发送「跳过」";
        Global().StartTimer(GAME_OPTION(时限));
    }

    void ToDiscardStep_()
    {
        step_ = Step::DISCARD;
        Global().Boardcast() << At(PlayerID(turn_)) << " 进入【弃牌阶段】\n"
                             << "请弃掉一张手牌以结束本回合（如：蓝3）";
        Global().StartTimer(GAME_OPTION(时限));
    }

    void NextTurn_()
    {
        if (game_over_) {
            return;
        }
        turn_ = NextAlive_(turn_);
        StartTurn_();
    }

    // 玩家得分。达到目标分数不会立即结束游戏，需等本局结束后结算
    void AddScore_(const uint32_t pid, const int score, const string& reason)
    {
        players_[pid].score_ += score;
        AddLog_(PlayerTag_(pid) + " " + reason + "，+" + to_string(score) + " 分（累计 " +
                to_string(players_[pid].score_) + " 分）");
        auto sender = Global().Boardcast();
        sender << At(PlayerID(pid)) << " " << reason << "，获得 " << score << " 分（累计 "
               << players_[pid].score_ << " 分）";
        if (players_[pid].score_ >= k_target_score) {
            sender << "\n已达到目标分数，本局结束后游戏即告结束";
        }
    }

    // 本局结束时，累计分数达到目标分数的未退出玩家，可能有多人并列
    vector<uint32_t> ScoreWinners_() const
    {
        int best = k_target_score;
        for (uint32_t pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (!players_[pid].eliminated_ && players_[pid].score_ > best) {
                best = players_[pid].score_;
            }
        }
        vector<uint32_t> winners;
        for (uint32_t pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (!players_[pid].eliminated_ && players_[pid].score_ >= best) {
                winners.emplace_back(pid);
            }
        }
        return winners;
    }

    // 本局结束。结束原因由 round_winner_ 决定：有人打空手牌，或抽牌堆已空
    void EndRound_()
    {
        BoardcastBoard_();
        {
            auto sender = Global().Boardcast();
            sender << "第 " << round_ << " 局结束（";
            if (round_winner_.has_value()) {
                sender << At(PlayerID(*round_winner_)) << " 打空了全部手牌";
            } else {
                sender << "抽牌堆已空";
            }
            sender << "）\n当前累计分数：";
            for (uint32_t pid = 0; pid < Global().PlayerNum(); ++pid) {
                sender << "\n" << At(PlayerID(pid)) << "：" << players_[pid].score_ << " 分"
                       << (players_[pid].eliminated_ ? "（已退出）" : "");
            }
        }
        // 有玩家累计分数达到目标分数，整场游戏在本局结束后结束
        if (const auto winners = ScoreWinners_(); !winners.empty()) {
            GameOver_(winners);
            return;
        }
        // 下一局先手为本局清空手牌者的下一位，无人清空则由上一局先手顺延
        round_first_ = NextAlive_(round_winner_.has_value() ? *round_winner_ : round_first_);
        if (AliveNum_() <= 1) {
            GameOver_(AliveNum_() == 1 ? vector<uint32_t>{NextAlive_(0)} : vector<uint32_t>{});
            return;
        }
        StartRound_();
    }

    void GameOver_(const vector<uint32_t>& winners)
    {
        if (game_over_) {
            return;
        }
        game_over_ = true;
        winners_ = winners;
        Global().StopTimer();
        BoardcastBoard_();
        auto sender = Global().Boardcast();
        if (winners_.empty()) {
            sender << "游戏结束！所有玩家均已退出，本场游戏无人获胜";
        } else if (winners_.size() == 1) {
            sender << "游戏结束！获胜者为 " << At(PlayerID(winners_.front())) << "（累计 "
                   << players_[winners_.front()].score_ << " 分）";
        } else {
            sender << "游戏结束！以下玩家以 " << players_[winners_.front()].score_ << " 分并列获胜：";
            for (const uint32_t pid : winners_) {
                sender << "\n" << At(PlayerID(pid));
            }
        }
    }

    // 玩家退出游戏（超时判负或主动退出）
    void EliminatePlayer_(const uint32_t pid, const bool notify_match)
    {
        if (players_[pid].eliminated_) {
            return;
        }
        players_[pid].eliminated_ = true;
        players_[pid].hand_.clear();
        AddLog_(PlayerTag_(pid) + " 已退出游戏");
        if (notify_match) {
            Global().Eliminate(PlayerID(pid));
        }
        if (AliveNum_() == 0) {
            GameOver_({});
            return;
        }
        if (AliveNum_() == 1) {     // 仅剩一名玩家时该玩家直接获胜
            GameOver_({NextAlive_(pid)});
            return;
        }
        if (pid == turn_) {         // 当前行动玩家退出，轮转至下一位
            NextTurn_();
        }
    }

    // ---------- 指令处理 ----------

    // 行动成功后的统一返回值。行动可能间接结束整场游戏（如弃牌后轮转发现抽牌堆已空），
    // 此时必须返回 CHECKOUT，否则框架不会结算本场比赛
    AtomReqErrCode ActionResult_() const
    {
        if (game_over_) {
            return StageErrCode::CHECKOUT;
        }
        return StageErrCode::OK;
    }

    AtomReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (is_public) {
            Global().Boardcast() << Markdown(GetBoard_(), k_board_width);
        } else {
            reply() << Markdown(GetHandBoard_(pid), k_board_width);
        }
        return StageErrCode::OK;
    }

    AtomReqErrCode Pass_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (game_over_) {
            reply() << "[错误] 游戏已经结束";
            return StageErrCode::FAILED;
        }
        if (players_[pid].eliminated_) {
            reply() << "[错误] 您已退出本场游戏，无法继续行动";
            return StageErrCode::FAILED;
        }
        if (pid != turn_) {
            reply() << "[错误] 现在不是您的回合，当前行动玩家为 " << At(PlayerID(turn_));
            return StageErrCode::FAILED;
        }
        if (step_ != Step::TASK && step_ != Step::CHAIN) {
            reply() << "[错误] 【" << k_step_names[static_cast<int>(step_)] << "】为必做操作，无法跳过";
            return StageErrCode::FAILED;
        }
        const bool is_task_step = step_ == Step::TASK;
        Global().Boardcast() << At(PlayerID(pid)) << " 跳过了【" << (is_task_step ? "任务阶段" : "连出阶段") << "】";
        if (is_task_step) {
            ToChainStep_();
        } else {
            ToDiscardStep_();
        }
        return ActionResult_();
    }

    AtomReqErrCode Draw_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int from)
    {
        if (!CheckAction_(pid, reply, Step::DRAW)) {
            return StageErrCode::FAILED;
        }
        Card card{Color::RED, 1};
        if (from == 0) {
            if (deck_.empty()) {
                reply() << "[错误] 抽牌堆已空，无法抽牌";
                return StageErrCode::FAILED;
            }
            card = deck_.back();
            deck_.pop_back();
            AddLog_(PlayerTag_(pid) + " 从抽牌堆抽牌");
            Global().Boardcast() << At(PlayerID(pid)) << " 从抽牌堆抽了一张牌（剩余 " << deck_.size() << " 张）";
        } else {
            if (discard_.empty()) {
                reply() << "[错误] 弃牌堆为空，无法拾取";
                return StageErrCode::FAILED;
            }
            card = discard_.back();
            discard_.pop_back();
            AddLog_(PlayerTag_(pid) + " 拾取弃牌堆的 " + CardName(card));
            Global().Boardcast() << At(PlayerID(pid)) << " 拾取了弃牌堆顶的 " << CardName(card);
        }
        players_[pid].hand_.emplace_back(card);
        players_[pid].SortHand();
        Global().Tell(pid) << "您获得了 " << CardName(card) << HandImage_(pid, "当前手牌");
        ToTaskStep_();
        return ActionResult_();
    }

    AtomReqErrCode Task_(const PlayerID pid, const bool is_public, MsgSenderBase& reply,
            const vector<Card>& cards0, const vector<Card>& cards1)
    {
        if (!CheckAction_(pid, reply, Step::TASK)) {
            return StageErrCode::FAILED;
        }
        const vector<Card>* const groups[k_area_num] = { &cards0, &cards1 };
        for (int area = 0; area < k_area_num; ++area) {
            if (!IsQualified(tasks_[area], *groups[area])) {
                reply() << "[错误] 第 " << (area + 1) << " 组牌不满足任务" << (area + 1) << "【" << tasks_[area].Name()
                        << "】的要求：至少 " << tasks_[area].require_ << " 张，" << tasks_[area].Desc();
                return StageErrCode::FAILED;
            }
        }
        vector<Card> all = cards0;
        all.insert(all.end(), cards1.begin(), cards1.end());
        if (!players_[pid].TakeFromHand(all)) {
            reply() << "[错误] 您的手牌中没有足够的这些牌：" << CardsName(all) << HandImage_(pid, "当前手牌");
            return StageErrCode::FAILED;
        }
        for (int area = 0; area < k_area_num; ++area) {
            players_[pid].areas_[area] = *groups[area];
            sort(players_[pid].areas_[area].begin(), players_[pid].areas_[area].end());
        }
        players_[pid].task_done_ = true;
        Global().Boardcast() << At(PlayerID(pid)) << " 完成了本局的两个基础任务\n任务1："
                             << CardsName(players_[pid].areas_[0]) << "\n任务2：" << CardsName(players_[pid].areas_[1]);
        BoardcastBoard_();
        AddScore_(pid, 1, "完成本局两个基础任务");
        if (players_[pid].hand_.empty()) {
            return FinishByEmptyHand_(pid);
        }
        ToChainStep_();
        return ActionResult_();
    }

    AtomReqErrCode Chain_(const PlayerID pid, const bool is_public, MsgSenderBase& reply,
            const AreaArg& target, const vector<Card>& cards)
    {
        if (!CheckAction_(pid, reply, Step::CHAIN)) {
            return StageErrCode::FAILED;
        }
        if (static_cast<uint32_t>(target.pid_) >= Global().PlayerNum()) {
            reply() << "[错误] 不存在 " << (target.pid_ + 1) << "号玩家，本场游戏共 " << Global().PlayerNum() << " 位玩家";
            return StageErrCode::FAILED;
        }
        Player& target_player = players_[target.pid_];
        if (!target_player.task_done_) {
            reply() << "[错误] " << At(PlayerID(target.pid_)) << " 尚未完成本局的基础任务，其任务区无法连出";
            return StageErrCode::FAILED;
        }
        vector<Card> merged = target_player.areas_[target.area_];
        merged.insert(merged.end(), cards.begin(), cards.end());
        if (!MatchRule(tasks_[target.area_].type_, merged)) {
            reply() << "[错误] 追加后该任务区不再满足任务" << (target.area_ + 1) << "【"
                    << tasks_[target.area_].Name() << "】的规则：" << tasks_[target.area_].Desc();
            return StageErrCode::FAILED;
        }
        if (!players_[pid].TakeFromHand(cards)) {
            reply() << "[错误] 您的手牌中没有足够的这些牌：" << CardsName(cards) << HandImage_(pid, "当前手牌");
            return StageErrCode::FAILED;
        }
        sort(merged.begin(), merged.end());
        target_player.areas_[target.area_] = merged;
        AddLog_(PlayerTag_(pid) + " 向 " + to_string(target.pid_ + 1) + "号 任务" + to_string(target.area_ + 1) +
                " 连出 " + CardsName(cards));
        {
            auto sender = Global().Boardcast();
            sender << At(PlayerID(pid));
            if (static_cast<uint32_t>(target.pid_) == pid) {
                sender << " 向自己的任务";
            } else {
                sender << " 向 " << At(PlayerID(target.pid_)) << " 的任务";
            }
            sender << (target.area_ + 1) << " 连出了 " << CardsName(cards) << "\n该任务区现为："
                   << CardsName(target_player.areas_[target.area_]);
        }
        if (players_[pid].hand_.empty()) {
            return FinishByEmptyHand_(pid);
        }
        Global().Tell(pid) << "可继续连出，或发送「跳过」进入弃牌阶段" << HandImage_(pid, "当前手牌");
        Global().StartTimer(GAME_OPTION(时限));
        return ActionResult_();
    }

    AtomReqErrCode Discard_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const vector<Card>& cards)
    {
        if (!CheckAction_(pid, reply, Step::DISCARD)) {
            return StageErrCode::FAILED;
        }
        if (cards.size() != 1) {
            reply() << "[错误] 弃牌阶段每次只能弃掉一张手牌";
            return StageErrCode::FAILED;
        }
        if (!players_[pid].TakeFromHand(cards)) {
            reply() << "[错误] 您的手牌中没有 " << CardsName(cards) << HandImage_(pid, "当前手牌");
            return StageErrCode::FAILED;
        }
        discard_.emplace_back(cards.front());
        AddLog_(PlayerTag_(pid) + " 弃掉 " + CardName(cards.front()));
        Global().Boardcast() << At(PlayerID(pid)) << " 弃掉了 " << CardName(cards.front());
        if (players_[pid].hand_.empty()) {
            return FinishByEmptyHand_(pid);
        }
        NextTurn_();
        return ActionResult_();
    }

    // 玩家打空手牌，额外得分并结束本局
    AtomReqErrCode FinishByEmptyHand_(const uint32_t pid)
    {
        round_winner_ = pid;
        AddScore_(pid, 1, "打空全部手牌");
        EndRound_();
        return ActionResult_();
    }

    // ---------- 超时与退出 ----------

    virtual CheckoutErrCode OnStageTimeout() override
    {
        if (game_over_) {
            return StageErrCode::CHECKOUT;
        }
        Global().Boardcast() << At(PlayerID(turn_)) << " 超时未行动，被淘汰出局";
        EliminatePlayer_(turn_, true);
        if (game_over_) {
            return StageErrCode::CHECKOUT;
        }
        return StageErrCode::CONTINUE;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        if (game_over_) {
            return StageErrCode::CHECKOUT;
        }
        Global().Boardcast() << At(PlayerID(pid)) << " 中途退出游戏";
        EliminatePlayer_(pid, false);
        if (game_over_) {
            return StageErrCode::CHECKOUT;
        }
        return StageErrCode::CONTINUE;
    }

    virtual CheckoutErrCode OnStageOver() override { return StageErrCode::CHECKOUT; }

    // ---------- 电脑行动 ----------

    // 从手牌中找出符合任务规则、张数恰好等于要求的候选牌组
    vector<vector<Card>> FindGroups_(const Task& task, const vector<Card>& hand) const
    {
        vector<vector<Card>> groups;
        const auto pick = [&](const vector<Card>& pool)
            {
                if (static_cast<int>(pool.size()) >= task.require_) {
                    groups.emplace_back(pool.begin(), pool.begin() + task.require_);
                }
            };
        switch (task.type_) {
        case TaskType::SAME_POINT:
            for (int point = 1; point <= k_max_point; ++point) {
                vector<Card> pool;
                for (const auto& card : hand) {
                    if (card.point_ == point) {
                        pool.emplace_back(card);
                    }
                }
                pick(pool);
            }
            break;
        case TaskType::SAME_COLOR:
            for (int color = 0; color < k_color_num; ++color) {
                vector<Card> pool;
                for (const auto& card : hand) {
                    if (static_cast<int>(card.color_) == color) {
                        pool.emplace_back(card);
                    }
                }
                pick(pool);
            }
            break;
        case TaskType::ODD:
        case TaskType::EVEN: {
            const int remainder = task.type_ == TaskType::ODD ? 1 : 0;
            vector<Card> pool;
            for (const auto& card : hand) {
                if (card.point_ % 2 == remainder) {
                    pool.emplace_back(card);
                }
            }
            pick(pool);
            break;
        }
        case TaskType::CONSECUTIVE:
            for (int begin = 1; begin + task.require_ - 1 <= k_max_point; ++begin) {
                vector<Card> pool;
                for (int point = begin; point < begin + task.require_; ++point) {
                    for (const auto& card : hand) {
                        if (card.point_ == point) {
                            pool.emplace_back(card);
                            break;
                        }
                    }
                }
                if (static_cast<int>(pool.size()) == task.require_) {
                    groups.emplace_back(pool);
                }
            }
            break;
        }
        return groups;
    }

    // 从手牌中扣除指定的牌，手牌不足时返回空
    static optional<vector<Card>> RemoveCards_(const vector<Card>& hand, const vector<Card>& cards)
    {
        vector<Card> rest = hand;
        for (const auto& card : cards) {
            const auto it = find(rest.begin(), rest.end(), card);
            if (it == rest.end()) {
                return nullopt;
            }
            rest.erase(it);
        }
        return rest;
    }

    // 寻找一组可同时完成两个任务的出牌方案
    optional<pair<vector<Card>, vector<Card>>> FindTaskPlan_(const vector<Card>& hand) const
    {
        for (int order = 0; order < k_area_num; ++order) {
            const Task& first_task = tasks_[order];
            const Task& second_task = tasks_[1 - order];
            for (const auto& first : FindGroups_(first_task, hand)) {
                const auto rest = RemoveCards_(hand, first);
                if (!rest.has_value()) {
                    continue;
                }
                const auto seconds = FindGroups_(second_task, *rest);
                if (seconds.empty()) {
                    continue;
                }
                if (order == 0) {
                    return make_pair(first, seconds.front());
                }
                return make_pair(seconds.front(), first);
            }
        }
        return nullopt;
    }

    // 寻找一次可行的连出
    optional<pair<AreaArg, Card>> FindChainPlan_(const uint32_t pid) const
    {
        for (uint32_t target = 0; target < Global().PlayerNum(); ++target) {
            if (!players_[target].task_done_) {
                continue;
            }
            for (int area = 0; area < k_area_num; ++area) {
                for (const auto& card : players_[pid].hand_) {
                    vector<Card> merged = players_[target].areas_[area];
                    merged.emplace_back(card);
                    if (MatchRule(tasks_[area].type_, merged)) {
                        return make_pair(AreaArg{static_cast<int>(target), area}, card);
                    }
                }
            }
        }
        return nullopt;
    }

    AtomReqErrCode ComputerAct_(const PlayerID pid, MsgSenderBase& reply)
    {
        switch (step_) {
        case Step::DRAW:
            return Draw_(pid, false, reply, discard_.empty() ? 0 : static_cast<int>(RandInt(rng_, 0, 1)));
        case Step::TASK: {
            const auto plan = FindTaskPlan_(players_[pid].hand_);
            if (plan.has_value()) {
                return Task_(pid, false, reply, plan->first, plan->second);
            }
            return Pass_(pid, false, reply);
        }
        case Step::CHAIN: {
            const auto plan = FindChainPlan_(pid);
            if (plan.has_value()) {
                return Chain_(pid, false, reply, plan->first, vector<Card>{plan->second});
            }
            return Pass_(pid, false, reply);
        }
        case Step::DISCARD:
            if (players_[pid].hand_.empty()) {
                return StageErrCode::FAILED;
            }
            return Discard_(pid, false, reply, vector<Card>{players_[pid].hand_[
                    RandInt(rng_, 0, static_cast<uint32_t>(players_[pid].hand_.size()) - 1)]});
        }
        return StageErrCode::FAILED;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        // 非当前行动玩家已被标记为已完成，直接返回 OK 让推演循环跳过
        if (game_over_ || players_[pid].eliminated_ || Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        const auto rc = ComputerAct_(pid, reply);
        if (rc == StageErrCode::CHECKOUT) {
            return StageErrCode::CHECKOUT;
        }
        if (rc == StageErrCode::FAILED) {
            return StageErrCode::OK;        // 行动异常时返回 OK，避免推演循环无法退出
        }
        return StageErrCode::FAILED;        // 行动成功但游戏尚未结束，使推演循环继续轮询其余玩家
    }
};

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot
