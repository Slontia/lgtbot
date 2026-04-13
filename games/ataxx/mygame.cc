// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).
//
// This file was generated with the assistance of Claude Code (claude.ai/code).

#include <random>
#include <numeric>
#include <algorithm>
#include <optional>

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"
#include "utility/coding.h"

using namespace std;

#include "board.h"

using namespace lgtbot::game_util::ataxx;

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;
const GameProperties k_properties {
    .name_ = "同化棋", // the game name which should be unique among all the games
    .developer_ = "铁蛋",
    .description_ = "通过复制和跳跃棋子来同化对手的棋类游戏",
    .shuffled_player_id_ = true,
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 4; }
uint32_t Multiple(const CustomOptions& options) {
     return min(2U, GET_OPTION_VALUE(options, 棋盘大小) / 5);
}
const MutableGenericOptions k_default_generic_options{
        .is_formal_{false},
};
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
    InitOptionsCommand("独自一人开始游戏",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options)
            {
                generic_options.bench_computers_to_player_num_ = 4;
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
        : StageFsm(std::move(utility),
            MakeStageCommand(*this, "查看当前游戏进展情况", &MainStage::Status_, VoidChecker("赛况")))
        , round_(0)
        , player_scores_(Global().PlayerNum(), 0)
        , board_(GAME_OPTION(棋盘大小), Global().PlayerNum())
        , player_eliminated_(Global().PlayerNum(), false)
    {
        // Initialize turn order: 0, 1, 2, ... then shuffle
        turn_order_.resize(Global().PlayerNum());
        std::iota(turn_order_.begin(), turn_order_.end(), 0);
        std::mt19937 rng(std::random_device{}());
        std::shuffle(turn_order_.begin(), turn_order_.end(), rng);
        current_turn_idx_ = 0;
    }

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }

    std::vector<int64_t> player_scores_;

    Board board_;
    std::vector<PlayerID> turn_order_;
    int32_t current_turn_idx_;
    int round_;
    std::vector<bool> player_eliminated_;

    PlayerID CurrentPlayer() const { return turn_order_[current_turn_idx_]; }

    // A player is alive if not eliminated and has pieces on the board
    bool IsPlayerAlive(int32_t pid) const
    {
        return !player_eliminated_[pid] && board_.PieceCount(pid) > 0;
    }

    int32_t CountAlivePlayers() const
    {
        int32_t count = 0;
        for (int p = 0; p < (int)Global().PlayerNum(); ++p) {
            if (IsPlayerAlive(p)) ++count;
        }
        return count;
    }

    // Advance to next player who can act; returns false if game over
    bool AdvanceToNextPlayer()
    {
        const int32_t num_players = Global().PlayerNum();
        for (int i = 0; i < num_players; ++i) {
            current_turn_idx_ = (current_turn_idx_ + 1) % num_players;
            if (current_turn_idx_ == 0) {
                ++round_;
            }
            PlayerID pid = CurrentPlayer();
            if (IsPlayerAlive(pid) && board_.HasLegalMove(pid)) {
                return true;
            }
            // Player is alive but has no legal move: skip with notification
            if (IsPlayerAlive(pid)) {
                Global().Boardcast() << At(pid) << "（" << k_player_color_names[pid] << "）无合法操作，跳过本回合";
            }
        }
        return false; // no one can move
    }

    std::string GetBoardHtml() const
    {
        PlayerID cur = CurrentPlayer();
        auto clone_targets = board_.GetCloneTargets(cur);
        auto jump_only_targets = board_.GetJumpOnlyTargets(cur);
        return GetHeaderHtml_() + board_.ToHtml(cur, clone_targets, jump_only_targets);
    }

    std::string GetBoardHtmlNoHighlight() const
    {
        return GetHeaderHtml_() + board_.ToHtml(-1);
    }

  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << Markdown(GetBoardHtml());
        return StageErrCode::OK;
    }

    void FirstStageFsm(SubStageFsmSetter setter) override
    {
        // Announce turn order
        auto sender = Global().Boardcast();
        sender << "行动顺序：\n";
        for (int i = 0; i < (int)turn_order_.size(); ++i) {
            if (i > 0) sender << "\n";
            sender << At(turn_order_[i]) << "（" << k_player_color_names[turn_order_[i]] << "）";
        }
        setter.Emplace<RoundStage>(*this, round_ + 1);
    }

    void NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override
    {
        // Check if game should end
        if (CountAlivePlayers() <= 1 || board_.IsGameOver() || board_.IsBoardFull() || !AdvanceToNextPlayer()) {
            Global().Boardcast() << Markdown(GetBoardHtmlNoHighlight());
            auto sender = Global().Boardcast();
            sender << "游戏结束！";
            for (int p = 0; p < (int)Global().PlayerNum(); ++p) {
                sender << "\n" << At(PlayerID{static_cast<uint32_t>(p)}) << "（" << k_player_color_names[p] << "）：" << board_.PieceCount(p) << " 枚棋子";
            }
            // Set scores: piece count (skip eliminated players, their score stays 0)
            for (int p = 0; p < (int)Global().PlayerNum(); ++p) {
                if (!player_eliminated_[p]) {
                    player_scores_[p] = board_.PieceCount(p);
                }
            }
            return;
        }
        setter.Emplace<RoundStage>(*this, round_ + 1);
    }

    // Helper: fill one player's info into the player table (piece icon spans 2 rows, name + count on right)
    void FillPlayerCell_(html::Table& table, uint32_t row, uint32_t col, uint32_t pid) const
    {
        table.MergeDown(row, col, 2);
        table.Get(row, col).SetContent(SvgPiece(k_player_colors[pid]));
        std::string name_content = "**" + Global().PlayerName(pid) + "**";
        if (static_cast<uint64_t>(CurrentPlayer()) == pid) {
            name_content = HTML_COLOR_FONT_HEADER(red) + name_content + HTML_FONT_TAIL;
        }
        table.Get(row, col + 1).SetContent(name_content);
        table.Get(row + 1, col + 1).SetContent(
            "棋子数：" HTML_COLOR_FONT_HEADER(yellow) + std::to_string(board_.PieceCount(pid)) + HTML_FONT_TAIL);
    }

    std::string GetHeaderHtml_() const
    {
        std::string str = "## 第 " + std::to_string(round_ + 1) + " 回合\n\n";
        const auto num_players = Global().PlayerNum();

        if (num_players == 2) {
            // 2 players: single row, same as before
            html::Table player_table(2, 4);
            player_table.SetTableStyle(" align=\"center\" cellpadding=\"3\" cellspacing=\"3\" ");
            FillPlayerCell_(player_table, 0, 0, 0);
            FillPlayerCell_(player_table, 0, 2, 1);
            str += player_table.ToString();
        } else {
            // 3-4 players: 2 rows matching board corner positions
            // Row 1: top-left=红(2) + top-right=黑(0)
            // Row 2: bottom-left=白(1) + bottom-right=蓝(3, if 4p)
            html::Table player_table(4, 4);
            player_table.SetTableStyle(" align=\"center\" cellpadding=\"3\" cellspacing=\"3\" ");
            // Top row: 红(pid 2) left, 黑(pid 0) right
            FillPlayerCell_(player_table, 0, 0, 2);
            FillPlayerCell_(player_table, 0, 2, 0);
            // Bottom row: 白(pid 1) left
            FillPlayerCell_(player_table, 2, 0, 1);
            if (num_players >= 4) {
                // 蓝(pid 3) right
                FillPlayerCell_(player_table, 2, 2, 3);
            }
            str += player_table.ToString();
        }

        // Turn order bar: small pieces with arrows
        str += "\n\n<p align=\"center\">";
        for (int i = 0; i < (int)turn_order_.size(); ++i) {
            if (i > 0) {
                str += " " + SvgArrow() + " ";
            }
            str += SvgSmallPiece(k_player_colors[turn_order_[i]]);
        }
        str += "</p>\n\n";
        return str;
    }
};

class RoundStage : public SubGameStage<>
{
  public:
    RoundStage(MainStage& main_stage, const uint64_t round)
        : StageFsm(main_stage, "第 " + std::to_string(round) + " 回合",
            MakeStageCommand(*this, "复制移动（在己方棋子周围放置新棋子）", &RoundStage::Clone_, AnyArg("目标位置", "D4")),
            MakeStageCommand(*this, "跳跃移动（移动棋子到两格外）", &RoundStage::Jump_, AnyArg("起始位置", "A1"), AnyArg("目标位置", "C3")))
    {}

    virtual void OnStageBegin() override
    {
        PlayerID cur = Main().CurrentPlayer();
        // Set all other players as ready
        for (uint64_t p = 0; p < Main().Global().PlayerNum(); ++p) {
            if (p != static_cast<uint64_t>(cur)) {
                Global().SetReady(p);
            }
        }
        Global().Boardcast() << Markdown(Main().GetBoardHtml());
        Global().Boardcast() << "请 " << At(cur) << " 行动，可选操作：\n"
            << "1. 复制：发送目标坐标（如「D4」），在己方棋子旁放置新棋子\n"
            << "2. 跳跃：发送「起始坐标 目标坐标」（如「A1 C3」），移动棋子两格\n"
            << "时限 " << GAME_OPTION(时限) << " 秒";
        Global().StartTimer(GAME_OPTION(时限));
    }

  private:
    // Decode a coordinate string (e.g. "A1") to internal 0-based Coor. Returns nullopt on failure.
    std::optional<Coor> DecodeCoor_(const std::string& str, MsgSenderBase& reply)
    {
        const auto decode_result = DecodePos(str);
        if (const auto* errstr = std::get_if<std::string>(&decode_result)) {
            reply() << "操作失败：" << *errstr;
            return std::nullopt;
        }
        const auto [col, row_1based] = std::get<std::pair<uint32_t, uint32_t>>(decode_result);
        if (row_1based == 0) {
            reply() << "操作失败：纵坐标从 1 开始编号，请使用如「A1」的格式";
            return std::nullopt;
        }
        Coor coor{static_cast<int32_t>(row_1based - 1), static_cast<int32_t>(col)};
        if (!Main().board_.IsValid(coor)) {
            reply() << "操作失败：坐标超出棋盘范围";
            return std::nullopt;
        }
        return coor;
    }

    AtomReqErrCode Clone_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::string& pos_str)
    {
        if (Global().IsReady(pid)) {
            reply() << "[错误] 当前不是您的回合";
            return StageErrCode::FAILED;
        }

        const auto target_opt = DecodeCoor_(pos_str, reply);
        if (!target_opt.has_value()) {
            return StageErrCode::FAILED;
        }
        const Coor target = *target_opt;

        if (!Main().board_.IsCloneTarget(pid, target)) {
            reply() << "操作失败：该位置不是合法的复制目标，需要是己方棋子周围的空位";
            return StageErrCode::FAILED;
        }

        int32_t converted = Main().board_.DoClone(pid, target);
        auto sender = reply();
        sender << "复制成功！放置新棋子于 " << pos_str;
        if (converted > 0) {
            sender << "，同化了 " << converted << " 枚对手棋子";
        }
        return StageErrCode::READY;
    }

    AtomReqErrCode Jump_(const PlayerID pid, const bool is_public, MsgSenderBase& reply,
                         const std::string& from_str, const std::string& to_str)
    {
        if (Global().IsReady(pid)) {
            reply() << "[错误] 当前不是您的回合";
            return StageErrCode::FAILED;
        }

        const auto from_opt = DecodeCoor_(from_str, reply);
        if (!from_opt.has_value()) {
            return StageErrCode::FAILED;
        }
        const auto to_opt = DecodeCoor_(to_str, reply);
        if (!to_opt.has_value()) {
            return StageErrCode::FAILED;
        }
        const Coor from = *from_opt;
        const Coor to = *to_opt;

        if (Main().board_.GetCell(from) != static_cast<int32_t>(pid)) {
            reply() << "操作失败：起始位置没有您的棋子";
            return StageErrCode::FAILED;
        }

        if (!Main().board_.IsJumpMove(pid, from, to)) {
            reply() << "操作失败：不是合法的跳跃移动，需要沿直线或斜线方向移动恰好 2 格至空位";
            return StageErrCode::FAILED;
        }

        int32_t converted = Main().board_.DoJump(pid, from, to);
        auto sender = reply();
        sender << "跳跃成功！从 " << from_str << " 移动到 " << to_str;
        if (converted > 0) {
            sender << "，同化了 " << converted << " 枚对手棋子";
        }
        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        PlayerID cur = Main().CurrentPlayer();
        Main().player_scores_[cur] = 0;
        Main().player_eliminated_[cur] = true;
        Global().Boardcast() << At(cur) << " 超时判负，分数清零";
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Main().player_scores_[pid] = 0;
        Main().player_eliminated_[pid] = true;
        Global().Boardcast() << At(pid) << "（" << k_player_color_names[pid] << "）强退认负，分数清零";
        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        // Simple AI: try clone first, then jump
        auto clone_targets = Main().board_.GetCloneTargets(pid);
        if (!clone_targets.empty()) {
            // Pick the clone target that converts the most opponents
            Coor best_target = clone_targets[0];
            int best_score = -1;
            for (const auto& t : clone_targets) {
                int score = 0;
                for (const auto& d : k_directions) {
                    Coor adj{t.row_ + d.row_, t.col_ + d.col_};
                    if (Main().board_.IsValid(adj)) {
                        int32_t cell = Main().board_.GetCell(adj);
                        if (cell != k_empty && cell != static_cast<int32_t>(pid)) {
                            ++score;
                        }
                    }
                }
                if (score > best_score) {
                    best_score = score;
                    best_target = t;
                }
            }
            Main().board_.DoClone(pid, best_target);
            return StageErrCode::READY;
        }
        auto jump_moves = Main().board_.GetJumpMoves(pid);
        if (!jump_moves.empty()) {
            // Pick the jump that converts the most opponents
            auto best = jump_moves[0];
            int best_score = -1;
            for (const auto& [from, to] : jump_moves) {
                int score = 0;
                for (const auto& d : k_directions) {
                    Coor adj{to.row_ + d.row_, to.col_ + d.col_};
                    if (Main().board_.IsValid(adj)) {
                        int32_t cell = Main().board_.GetCell(adj);
                        if (cell != k_empty && cell != static_cast<int32_t>(pid)) {
                            ++score;
                        }
                    }
                }
                if (score > best_score) {
                    best_score = score;
                    best = {from, to};
                }
            }
            Main().board_.DoJump(pid, best.first, best.second);
        }
        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        return StageErrCode::CHECKOUT;
    }
};

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot
