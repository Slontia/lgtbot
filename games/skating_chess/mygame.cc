// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).
//
// This file was generated with the assistance of Claude Code (claude.ai/code).

#include <cstdlib>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"

#include "board.h"

using namespace lgtbot::game_util::skating_chess;

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;
const GameProperties k_properties {
    .name_ = "溜冰棋",
    .developer_ = "铁蛋",
    .description_ = "在 5×5 冰面上滑动棋子，先让三子连成直线者获胜",
    .shuffled_player_id_ = true,
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 2; }
uint32_t Multiple(const CustomOptions& options) { return 1; }
const MutableGenericOptions k_default_generic_options;
const std::vector<RuleCommand> k_rule_commands = {};

bool AdaptOptions(ChildMsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() != 2) {
        reply() << "该游戏为双人游戏，必须为 2 人参加，当前玩家数为 " << generic_options_readonly.PlayerNum();
        return false;
    }
    return true;
}

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options)
            {
                generic_options.bench_computers_to_player_num_ = 2;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
};

// ========== GAME STAGES ==========

class RoundStage;

class MainStage : public MainGameStage<RoundStage>
{
  public:
    // 对局结果。DEFAULT_LOSS 表示一方因超时、认输或强退而判负手真正取胜
    enum class Result { NONE, WIN, DRAW, DEFAULT_LOSS };

    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility),
            MakeStageCommand(*this, "查看当前棋盘情况，可用于图片重发", &MainStage::Status_, VoidChecker("赛况")))
        , player_scores_(Global().PlayerNum(), 0)
        , board_()
        , cur_pid_(static_cast<PlayerID>(std::rand() % k_side_num))
        , move_count_(0)
    {}

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }

    std::vector<int64_t> player_scores_;
    // 棋盘
    Board board_;
    // 当前行动玩家，玩家编号同时也是其阵营编号
    PlayerID cur_pid_;
    // 已完成的行动次数
    uint32_t move_count_;

    // 当前回合数，双方各行动一次算一回合
    uint32_t Round() const { return move_count_ / 2 + 1; }

    // 对局是否已经决出结果
    bool IsResultDecided() const { return result_ != Result::NONE; }

    // 记录因超时、认输或强退而判负的玩家
    void SetLoserByDefault(const PlayerID loser)
    {
        result_ = Result::DEFAULT_LOSS;
        loser_ = loser;
    }

    // 对局进行中的棋盘图，会标注当前行动方
    std::string GetBoardHtml() const { return GetHeaderHtml_() + board_.ToHtml(static_cast<int32_t>(cur_pid_), win_coors_); }

    // 对局结束后的棋盘图，不再标注行动方
    std::string GetFinalBoardHtml() const { return GetHeaderHtml_() + board_.ToHtml(k_empty, win_coors_); }

  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, ChildMsgSenderBase& reply)
    {
        reply() << Markdown(IsResultDecided() ? GetFinalBoardHtml() : GetBoardHtml(), k_image_width);
        return StageErrCode::OK;
    }

    void FirstStageFsm(SubStageFsmSetter setter) override
    {
        // 初始局面也计入重复局面的统计
        ++state_counts_[board_.StateKey(static_cast<int32_t>(cur_pid_))];
        Global().Boardcast() << "游戏开始！由 " << At(cur_pid_) << "（" << k_side_names[cur_pid_] << "方）先行";
        setter.Emplace<RoundStage>(*this, Round());
    }

    void NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override
    {
        // 超时、认输或强退已经决出结果，直接结算
        if (IsResultDecided()) {
            Finish_();
            return;
        }
        ++move_count_;
        // 只判定刚行动一方是否连成直线
        win_coors_ = board_.LineCoors(static_cast<int32_t>(cur_pid_));
        if (!win_coors_.empty()) {
            result_ = Result::WIN;
            winner_ = cur_pid_;
            Global().Boardcast() << At(cur_pid_) << "（" << k_side_names[cur_pid_] << "方）的三枚棋子连成直线，获得胜利！";
            Finish_();
            return;
        }
        cur_pid_ = static_cast<PlayerID>(1 - cur_pid_);
        if (++state_counts_[board_.StateKey(static_cast<int32_t>(cur_pid_))] >= k_repetition_limit) {
            result_ = Result::DRAW;
            Global().Boardcast() << "同一局面已第 " << k_repetition_limit << " 次出现";
            Finish_();
            return;
        }
        if (move_count_ / 2 >= GAME_OPTION(回合数)) {
            result_ = Result::DRAW;
            Global().Boardcast() << "已达到最大回合数 " << GAME_OPTION(回合数) << " 回合";
            Finish_();
            return;
        }
        setter.Emplace<RoundStage>(*this, Round());
    }

    // 广播最终盘面并结算分数
    void Finish_()
    {
        Global().Boardcast() << Markdown(GetFinalBoardHtml(), k_image_width);
        if (result_ == Result::WIN) {
            player_scores_[winner_] = 1;
        } else if (result_ == Result::DEFAULT_LOSS) {
            player_scores_[loser_] = -1;
        } else {
            Global().Boardcast() << "本局和棋，游戏结束！";
        }
    }

    // 阵营色标
    static std::string SideChip_(const int32_t side)
    {
        return "<svg width=\"20\" height=\"20\" viewBox=\"0 0 20 20\"><circle cx=\"10\" cy=\"10\" r=\"9\" fill=\"" +
               std::string(k_side_colors[side]) + "\"/></svg>";
    }

    // 标题与双方信息
    std::string GetHeaderHtml_() const
    {
        std::string str = k_page_style;
        str += "<h2 align=\"center\">第 " + std::to_string(Round()) + " 回合</h2>\n\n";
        html::Table table(k_side_num, 4);
        table.SetTableStyle(" align=\"center\" cellpadding=\"4\" cellspacing=\"3\" ");
        for (int32_t side = 0; side < k_side_num; ++side) {
            const PlayerID pid = static_cast<PlayerID>(side);
            const bool is_acting = !IsResultDecided() && side == static_cast<int32_t>(cur_pid_);
            table.Get(side, 0).SetStyle("style=\"width:44px;\"").SetContent(Global().PlayerAvatar(pid, 40));
            table.Get(side, 1).SetStyle("style=\"width:24px;\"").SetContent(SideChip_(side));
            table.Get(side, 2).SetStyle("style=\"width:300px; text-align:left;\"")
                    .SetContent("**" + Global().PlayerName(pid) + "**");
            table.Get(side, 3).SetStyle("style=\"width:56px;\"");
            if (is_acting) {
                table.Get(side, 3).SetColor("#FFD75E").SetContent("行动中");
            }
        }
        str += table.ToString();
        return str + "\n\n";
    }

    // 同一局面出现该次数即判和
    static constexpr uint32_t k_repetition_limit = 3;

    Result result_{Result::NONE};
    PlayerID winner_{0};
    PlayerID loser_{0};
    // 获胜的直线，用于高亮
    std::vector<Coor> win_coors_;
    // 各局面出现的次数
    std::map<std::string, uint32_t> state_counts_;
};

class RoundStage : public SubGameStage<>
{
  public:
    RoundStage(MainStage& main_stage, const uint32_t round)
        : StageFsm(main_stage, "第 " + std::to_string(round) + " 回合",
            MakeStageCommand(*this, "滑动棋子", &RoundStage::Move_,
                ArithChecker<uint32_t>(1, static_cast<uint32_t>(k_piece_num), "棋子编号"),
                AlterChecker<Direct>(k_direction_map)),
            MakeStageCommand(*this, "认输，判由对手获胜", &RoundStage::Concede_,
                AlterChecker<uint32_t>({{"认输", 0}, {"投降", 0}})),
            MakeStageCommand(*this, "滑动棋子（棋子与方向）", &RoundStage::MoveCompact_,
                AnyArg("棋子与方向", "3左上")))
    {}

    virtual void OnStageBegin() override
    {
        const PlayerID cur = Main().cur_pid_;
        // 轮流行动，非行动方本回合无需等待
        Global().SetReady(static_cast<PlayerID>(1 - cur));
        Global().Boardcast() << Markdown(Main().GetBoardHtml(), k_image_width);
        Global().Boardcast() << "请 " << At(cur) << "（" << k_side_names[cur] << "方）滑动棋子\n"
                             << "格式：棋子编号 方向，如「3 左上」或「3左上」\n"
                             << "时限 " << GAME_OPTION(时限) << " 秒，超时判负";
        Global().StartTimer(GAME_OPTION(时限));
    }

  private:
    AtomReqErrCode Move_(const PlayerID pid, const bool is_public, ChildMsgSenderBase& reply, const uint32_t number,
                         const Direct direct)
    {
        if (Global().IsReady(pid)) {
            reply() << "[错误] 当前不是您的回合";
            return StageErrCode::FAILED;
        }
        const int32_t side = static_cast<int32_t>(pid);
        const int32_t index = static_cast<int32_t>(number) - 1;
        if (!Main().board_.TrySlide(side, index, direct).has_value()) {
            auto sender = reply();
            sender << "[错误] " << number << " 号棋子无法向" << k_direct_names[static_cast<int32_t>(direct)]
                   << "滑动，紧邻的位置是墙壁或棋子";
            const auto directs = Main().board_.LegalDirects(side, index);
            if (directs.empty()) {
                sender << "\n该棋子已被完全堵住，请改用其他棋子";
            } else {
                sender << "\n该棋子可滑行的方向：";
                for (const Direct legal_direct : directs) {
                    sender << k_direct_names[static_cast<int32_t>(legal_direct)] << " ";
                }
            }
            return StageErrCode::FAILED;
        }
        return DoMove_(pid, index, direct);
    }

    AtomReqErrCode MoveCompact_(const PlayerID pid, const bool is_public, ChildMsgSenderBase& reply, const std::string& str)
    {
        if (str.size() < 2 || str[0] < '1' || str[0] > static_cast<char>('0' + k_piece_num)) {
            reply() << "[错误] 格式错误，正确格式为「棋子编号 方向」，如「3 左上」或「3左上」";
            return StageErrCode::FAILED;
        }
        const std::string direct_str = str.substr(1);
        const auto it = k_direction_map.find(direct_str);
        if (it == k_direction_map.end()) {
            reply() << "[错误] 未知的方向，可用方向：上 右上 右 右下 下 左下 左 左上";
            return StageErrCode::FAILED;
        }
        return Move_(pid, is_public, reply, static_cast<uint32_t>(str[0] - '0'), it->second);
    }

    AtomReqErrCode Concede_(const PlayerID pid, const bool is_public, ChildMsgSenderBase& reply, const uint32_t)
    {
        Global().Boardcast() << At(pid) << "（" << k_side_names[pid] << "方）认输，判负";
        Main().SetLoserByDefault(pid);
        return StageErrCode::CHECKOUT;
    }

    // 执行滑行并广播
    AtomReqErrCode DoMove_(const PlayerID pid, const int32_t index, const Direct direct)
    {
        const int32_t side = static_cast<int32_t>(pid);
        const Coor from = Main().board_.PiecePos(side, index);
        const Coor to = Main().board_.Slide(side, index, direct);
        Global().Boardcast() << At(pid) << " 将 " << (index + 1) << " 号棋子从 " << CoorToStr(from) << " 向"
                             << k_direct_names[static_cast<int32_t>(direct)] << "滑动至 " << CoorToStr(to);
        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        const PlayerID cur = Main().cur_pid_;
        const PlayerID loser = Global().IsReady(cur) ? static_cast<PlayerID>(1 - cur) : cur;
        Global().Boardcast() << At(loser) << "（" << k_side_names[loser] << "方）超时未行动，判负";
        Main().SetLoserByDefault(loser);
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Global().Boardcast() << At(pid) << "（" << k_side_names[pid] << "方）中途退出游戏，判负";
        Main().SetLoserByDefault(pid);
        return StageErrCode::CHECKOUT;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, ChildMsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        const int32_t side = static_cast<int32_t>(pid);
        const int32_t opp = 1 - side;
        const auto moves = Main().board_.LegalMoves(side);
        if (moves.empty()) {
            return StageErrCode::READY;
        }
        // 优先取胜，其次避开会让对手立刻连成直线的走法
        std::vector<std::pair<int32_t, Direct>> safe_moves;
        for (const auto& [index, direct] : moves) {
            Board trial = Main().board_;
            trial.Slide(side, index, direct);
            if (trial.HasLine(side)) {
                return DoMove_(pid, index, direct);
            }
            bool opp_can_win = false;
            for (const auto& [opp_index, opp_direct] : trial.LegalMoves(opp)) {
                Board counter = trial;
                counter.Slide(opp, opp_index, opp_direct);
                if (counter.HasLine(opp)) {
                    opp_can_win = true;
                    break;
                }
            }
            if (!opp_can_win) {
                safe_moves.emplace_back(index, direct);
            }
        }
        const auto& candidates = safe_moves.empty() ? moves : safe_moves;
        const auto& [index, direct] = candidates[std::rand() % candidates.size()];
        return DoMove_(pid, index, direct);
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
