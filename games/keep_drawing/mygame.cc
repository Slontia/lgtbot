// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <algorithm>

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;

const GameProperties k_properties {
    .name_ = "抽抽抽",
    .developer_ = "铁蛋",
    .description_ = "抽是胆识，停是智慧",
    .shuffled_player_id_ = true,
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 5; }
uint32_t Multiple(const CustomOptions& options) { return 1; }
const MutableGenericOptions k_default_generic_options;
const std::vector<RuleCommand> k_rule_commands = {};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() < 2) {
        reply() << "该游戏至少 2 人参加，当前玩家数为 " << generic_options_readonly.PlayerNum();
        return false;
    }
    return true;
}

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("设置边长和胜利分数",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options, const uint32_t& size, const uint32_t& score)
            {
                GET_OPTION_VALUE(game_options, 边长) = size;
                GET_OPTION_VALUE(game_options, 分数) = score;
                return NewGameMode::MULTIPLE_USERS;
            },
            ArithChecker<uint32_t>(3, 5, "边长"), OptionalDefaultChecker<ArithChecker<uint32_t>>(60, 30, 600, "分数")),
    InitOptionsCommand("独自一人开始游戏",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options)
            {
                generic_options.bench_computers_to_player_num_ = 5;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
};

// ========== GAME STAGES ==========

class RoundStage;

class MainStage : public MainGameStage<RoundStage>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility))
        , round_(0)
        , player_scores_(Global().PlayerNum(), 0)
    {}

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }

    std::vector<int64_t> player_scores_;

    // 局数
    int round_;
    // 当前行动玩家
    PlayerID currentPlayer;

    // 棋盘
    std::vector<std::vector<int>> board;

    bool GameEnd() { return std::any_of(player_scores_.begin(), player_scores_.end(), [this](int64_t s) { return s >= GAME_OPTION(分数); }); }

  private:
    void FirstStageFsm(SubStageFsmSetter setter) override
    {
        srand((unsigned int)time(NULL));

        currentPlayer = 0;
        board.assign(GAME_OPTION(边长), std::vector<int>(GAME_OPTION(边长), 0));

        setter.Emplace<RoundStage>(*this, ++round_);
    }

    void NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override
    {
        if ((++round_) <= GAME_OPTION(局数) && !GameEnd()) {
            setter.Emplace<RoundStage>(*this, round_);
            return;
        }
        if (GameEnd()) {
            Global().Boardcast() << "有玩家到达胜利分数，游戏结束";
        } else {
            Global().Boardcast() << "局数到达上限，游戏结束";
        }
    }
};


class RoundStage : public SubGameStage<>
{
  public:
    RoundStage(MainStage& main_stage, const uint64_t round)
        : StageFsm(main_stage, "第 " + std::to_string(round) + " 局",
                MakeStageCommand(*this, "继续 / 停止", &RoundStage::Action_, AlterChecker<bool>({{"抽", true}, {"继续", true}, {"c", true}, {"停", false}, {"停止", false}, {"t", false}})),
                MakeStageCommand(*this, "查看当前游戏进展情况", &RoundStage::Status_, VoidChecker("赛况")))
        , player_stop_(Global().PlayerNum(), false)
    {}

    // 玩家停止
    std::vector<bool> player_stop_;
    // 爆炸掩码：标记哪些格子属于爆炸的行/列/对角线
    std::vector<std::vector<bool>> exploded_mask_;

    virtual void OnStageBegin() override
    {
        Global().Boardcast() << Markdown(GetBoardHtml());

        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (pid != Main().currentPlayer) {
                Global().SetReady(pid);
            }
        }
        Global().Boardcast() << "请 " << At(Main().currentPlayer) << " 选择行动，时限 " << GAME_OPTION(时限) << " 秒：继续(c) / 停止(t)";
        Global().StartTimer(GAME_OPTION(时限));
    }

  private:
    AtomReqErrCode Action_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const bool draw)
    {
        if (pid != Main().currentPlayer) {
            reply() << "[错误] 不是您的回合，当前玩家是：" << Global().PlayerName(Main().currentPlayer);
            return StageErrCode::FAILED;
        }

        if (draw) {
            int num = ActionDraw();
            reply() << "您选择继续！新增数字：" << num;
        } else {
            int score = ActionStop(pid);
            reply() << "您选择停止！本局计入 " << score << " 分，当前分数为 " << Main().player_scores_[pid];
        }
        
        return StageErrCode::READY;
    }

    int ActionDraw()
    {
        int size = GAME_OPTION(边长);
        int num = rand() % (size * size) + 1;
        int col = (num - 1) / size;
        int row = num - col * size - 1;
        Main().board[col][row]++;
        return num;
    }

    int ActionStop(const PlayerID pid)
    {
        int score = GetBoardSum();
        Main().player_scores_[pid] += score;
        player_stop_[pid] = true;
        return score;
    }

    AtomReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << Markdown(GetBoardHtml());
        return StageErrCode::OK;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        return HandleStageOver();
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        return HandleStageOver();
    }

    CheckoutErrCode HandleStageOver()
    {
        if (!Global().IsReady(Main().currentPlayer)) {
            Global().SetReady(Main().currentPlayer);
            int score = ActionStop(Main().currentPlayer);
            Global().Boardcast() << "玩家 " << At(Main().currentPlayer) << " 行动超时，自动选择停止，本局计入 " << score << " 分";
        }

        // 游戏结束
        if (Main().GameEnd()) {
            return StageErrCode::CHECKOUT;
        }

        bool round_end = false;
        int active = CountActivePlayers();
        // 所有玩家停止
        if (active == 0) {
            round_end = true;
            Global().Boardcast() << Markdown(GetBoardHtml());
            Global().Boardcast() << "所有玩家均已停止，本局结束";
        }
        // 触发三连
        if (CheckBoardExplode()) {
            round_end = true;
            Global().Boardcast() << Markdown(GetBoardHtml());
            if (active == 1) {
                Global().Boardcast() << At(Main().currentPlayer) << " 触发三连！本局结束";
            } else {
                int score = GetBoardSum() / (active - 1);
                for (int pid = 0; pid < Global().PlayerNum(); pid++) {
                    if (!player_stop_[pid] && pid != Main().currentPlayer) {
                        Main().player_scores_[pid] += score;
                    }
                }
                Global().Boardcast() << At(Main().currentPlayer) << " 触发三连！本局结束，未停止玩家分得 " << score << " 分";
            }
        }
        // 本局结束
        if (round_end) {
            ClearBoard();   // 重置盘面
            return StageErrCode::CHECKOUT;
        }
        // 下一个玩家行动
        do {
            Main().currentPlayer = (Main().currentPlayer + 1) % Global().PlayerNum();
        } while (player_stop_[Main().currentPlayer]);
        Global().Boardcast() << Markdown(GetBoardHtml());
        Global().Boardcast() << "请 " << At(Main().currentPlayer) << " 选择行动，时限 " << GAME_OPTION(时限) << " 秒：继续(c) / 停止(t)";
        Global().ClearReady(Main().currentPlayer);
        Global().StartTimer(GAME_OPTION(时限));
        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }

        int sum = GetBoardSum();
        int range = GAME_OPTION(边长) * GAME_OPTION(边长);
        int limit = (range + 1) * range / 4;
        if ((sum < limit || rand() % 5 == 0) && Main().player_scores_[pid] + sum < GAME_OPTION(分数)) {
            int num = ActionDraw();
            Global().Boardcast() << At(pid) << " 新增数字：" << num;
        } else {
            int score = ActionStop(pid);
            Global().Boardcast() << At(pid) << " 停止！本局计入 " << score << " 分，当前分数为 " << Main().player_scores_[pid];
        }
        return StageErrCode::READY;
    }

    int CountActivePlayers() { return std::count(player_stop_.begin(), player_stop_.end(), false); }

    std::string GetBoardHtml();
    bool CheckBoardExplode();
    int GetBoardSum();
    void ClearBoard();
};


std::string RoundStage::GetBoardHtml()
{
    const char* style = R"(
<style>
    .win {
        margin-top: 12px;
        text-align: center;
        font-size: 18px;
        font-weight: bold;
        color: #444444;
    }
    .win b{
        color: #2E7D32;
    }
    .board {
        border-collapse: separate;
        border-spacing: 4px;
        margin-top: 12px;
    }
    .grid {
        position: relative;
        width: 70px;
        height: 70px;
        box-sizing: border-box;
        border-radius: 8px;
        display: flex;
        align-items: center;
        justify-content: center;
        font-size: 30px;
        font-weight: bold;
    }
    .grid.empty {
        background-color: #EEEEEE;
        border: 3px solid #777777;
    }
    .grid.filled {
        background-color: #FFFFFF;
        border: 3px solid #D0D0D0;
    }
    .grid.explode {
        border-color: #E53935;
        box-shadow: 0 0 8px rgba(229, 57, 53, 0.18);
    }
    .corner {
        position: absolute;
        top: 4px;
        left: 6px;
        font-size: 11px;
        color: #8A8A8A;
        font-weight: normal;
        pointer-events: none;
    }
    .sub {
        position: absolute;
        right: 4px;
        bottom: 4px;
        min-width: 16px;
        height: 16px;
        line-height: 16px;
        border-radius: 8px;
        background-color: #E53935;
        color: #FFFFFF;
        font-size: 11px;
        text-align: center;
        font-weight: bold;
        padding: 0 4px;
        box-sizing: border-box;
    }
    .sum-container {
        margin-top: 15px;
        text-align: center;
        font-size: 18px;
        color: #444444;
        font-weight: bold;
    }
    .sum-value {
        display: inline-block;
        margin-top: 8px;
        padding: 6px 18px;
        border: 2px solid #D4AF37;
        border-radius: 18px;
        color: #D4AF37;
        font-size: 22px;
    }
    .boom {
        margin-top: 8px;
        text-align: center;
        font-size: 40px;
        font-weight: bold;
        color: #E53935;
    }
</style>)";

    bool boom = CheckBoardExplode();

    std::string round_info = "## 第 " + std::to_string(Main().round_) + " 局"; 

    html::Table playerTable(Global().PlayerNum(), 3);
    playerTable.SetTableStyle("align=\"center\" cellpadding=\"2\"");
    for (int pid = 0; pid < Global().PlayerNum(); pid++) {
        if (player_stop_[pid]) {
            playerTable.Get(pid, 1).SetColor("#E5E5E5");
        } else if (pid == Main().currentPlayer) {
            if (boom) playerTable.Get(pid, 1).SetColor("#FFA07A");
            else playerTable.Get(pid, 1).SetColor("#FFEBA3");
        }
        playerTable.Get(pid, 0).SetStyle("style=\"width:40px;\"").SetContent(Global().PlayerAvatar(pid, 40));
        playerTable.Get(pid, 1).SetStyle("style=\"width:280px; text-align:left;\"").SetContent(Global().PlayerName(pid));
        playerTable.Get(pid, 2).SetStyle("style=\"width:120px;\"").SetContent("分数：<span style='color:blue;'><b>" + std::to_string(Main().player_scores_[pid]) + "</b></span>");
    }

    std::string win_info = "<div class='win'>胜利分数：<b>" + std::to_string(GAME_OPTION(分数)) + "</b></div>";

    const auto& board = Main().board;
    int size = board.size();
    html::Table boardTable(size, size);
    boardTable.SetTableStyle("align='center' cellpadding='6' class='board'");
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            auto& cell = boardTable.Get(i, j);
            int value = i * size + j + 1;
            int count = board[i][j];
            std::string classes = "grid ";
            classes += (count <= 0) ? "empty" : "filled";
            if (boom && i < (int)exploded_mask_.size() && j < (int)exploded_mask_[i].size() && exploded_mask_[i][j]) classes += " explode";
            std::string style_attr = "class='" + classes + "'";
            cell.SetStyle(style_attr);
            if (count <= 0) {
                cell.SetContent("<span class='corner'>" + std::to_string(value) + "</span>");
                continue;
            }
            if (count == 1) {
                cell.SetContent(
                    "<span class='corner'>" + std::to_string(value) + "</span>" +
                    "<span>" + std::to_string(value) + "</span>"
                );
            } else {
                cell.SetContent(
                    "<span class='corner'>" + std::to_string(value) + "</span>" +
                    "<span>" + std::to_string(value) + "</span>" +
                    "<span class='sub'>" + std::to_string(count) + "</span>"
                );
            }
        }
    }

    std::string boom_info;
    if (boom) boom_info = "<div class='boom'>你爆啦</div>";

    std::string sum_info = "<div class='sum-container'><div>桌上总点数</div><div class='sum-value'>" + std::to_string(GetBoardSum()) + "</div></div>";

    return round_info + style + playerTable.ToString() + win_info + boardTable.ToString() + boom_info + sum_info;
}

bool RoundStage::CheckBoardExplode()
{
    const auto& board = Main().board;
    int size = board.size();
    if (size == 0) return false;
    // 重置爆炸掩码
    exploded_mask_.assign(size, std::vector<bool>(size, false));
    // 4 个方向：右、下、右下、左下
    const int dx[4] = {0, 1, 1, 1};
    const int dy[4] = {1, 0, 1, -1};

    for (int x = 0; x < size; ++x) {
        for (int y = 0; y < size; ++y) {
            if (board[x][y] <= 0) continue;
            for (int dir = 0; dir < 4; ++dir) {
                int nx1 = x + dx[dir];
                int ny1 = y + dy[dir];
                int nx2 = x + 2 * dx[dir];
                int ny2 = y + 2 * dy[dir];
                if (nx2 < 0 || nx2 >= size || ny2 < 0 || ny2 >= size)
                    continue;
                // 三连判定
                if (board[nx1][ny1] > 0 && board[nx2][ny2] > 0) {
                    exploded_mask_[x][y] = true;
                    exploded_mask_[nx1][ny1] = true;
                    exploded_mask_[nx2][ny2] = true;
                    return true;
                }
            }
        }
    }

    return false;
}

int RoundStage::GetBoardSum()
{
    const auto& board = Main().board;
    int size = board.size();
    int sum = 0;
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            int value = i * size + j + 1;
            sum += board[i][j] * value;
        }
    }
    return sum;
}

void RoundStage::ClearBoard()
{
    auto& board = Main().board;
    int size = board.size();
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            board[i][j] = 0;
        }
    }
    // 清空爆炸标记
    exploded_mask_.assign(size, std::vector<bool>(size, false));
}


auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot

