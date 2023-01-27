// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <array>
#include <map>
#include <functional>
#include <memory>
#include <vector>

#include "game_framework/game_main.h"
#include "game_framework/game_stage.h"
#include "game_framework/game_options.h"
#include "game_framework/game_achievements.h"
#include "utility/msg_checker.h"
#include "utility/html.h"
#include "game_util/renju.h"

const std::string k_game_name = "决胜五子";
const uint64_t k_max_player = 2; /* 0 means no max-player limits */
const uint64_t k_multiple = 1;
const std::string k_developer = "森高";
const std::string k_description = "双方一起落子的五子棋游戏";

using namespace renju;

std::string GameOption::StatusInfo() const
{
    return "每步时限 " + std::to_string(GET_VALUE(时限)) + " 秒，" +
        (GET_VALUE(碰撞上限) == 0 ? "碰撞不会造成和棋" : std::to_string(GET_VALUE(碰撞上限)) + " 次碰撞发生时和棋") + "，" +
        (GET_VALUE(回合上限) == 0 ? "没有回合限制" : std::to_string(GET_VALUE(回合上限)) + " 回合结束时和棋") + "，" +
        (GET_VALUE(pass上限) == 0 ? "pass 不会造成和棋" : "双方共 pass " + std::to_string(GET_VALUE(pass上限)) + " 次时和棋") + "，" +
        (GET_VALUE(模式) ? "pass 数量不会影响胜负" : "和棋时 pass 数量较多的玩家获胜");
}

bool GameOption::ToValid(MsgSenderBase& reply)
{
    if (PlayerNum() != 2) {
        reply() << "该游戏为双人游戏，必须为2人参加，当前玩家数为" << PlayerNum();
        return false;
    }
    return true;
}

uint64_t GameOption::BestPlayerNum() const { return 2; }

// ========== GAME STAGES ==========

// Player 1 use fork, player 0 use circle
class MainStage : public MainGameStage<>
{
  public:
    MainStage(const GameOption& option, MatchBase& match)
        : GameStage(option, match,
                MakeStageCommand("查看盘面情况，可用于图片重发", &MainStage::Info_, VoidChecker("赛况")),
                MakeStageCommand("决定落子", &MainStage::Ready_, VoidChecker("落子")),
                MakeStageCommand("跳过本次落子", &MainStage::Pass_, VoidChecker("pass")),
                MakeStageCommand("尝试落子（若附带「落子」参数，则直接落子）", &MainStage::Set_,
                    AnyArg("坐标"), OptionalDefaultChecker<BoolChecker>(true, "落子", "试下")))
        , board_(option.ResourceDir())
        , round_(0)
        , crash_count_(0)
        , extended_(false)
        , last_round_passed_{false, false}
        , last_round_both_passed_(false)
        , last_last_round_both_passed_(false)
        , pass_count_{0, 0}
    {
    }

    virtual void OnStageBegin()
    {
        if (!GET_OPTION_VALUE(option(), 模式)) {
            Boardcast() << "[注意] 本局为竞技模式，和棋时 pass 次数较多的玩家取得胜利";
        }
        StartTimer(GET_OPTION_VALUE(option(), 时限));
        Boardcast() << Markdown(HtmlHead_() + board_.ToHtml());
        Boardcast() << "请私信裁判落子位置";
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply)
    {
        do {
            player_pos_[pid].emplace(rand() % Board::k_size_, rand() % Board::k_size_);
        } while (!board_.CanBeSet(player_pos_[pid]->first, player_pos_[pid]->second));
        return StageErrCode::READY;
    }

    int64_t PlayerScore(const PlayerID pid) const
    {
        return winner_ == pid;
    }

  private:
    AtomReqErrCode Ready_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (IsReady(pid)) {
            reply() << "落子失败：您已经完成落子或已经决定跳过，无法再次落子";
            return StageErrCode::FAILED;
        }
        if (!player_pos_[pid].has_value()) {
            reply() << "落子失败：您还未选择落子位置，可以通过私信裁判坐标，来选择落子位置";
            return StageErrCode::FAILED;
        }
        reply() << "落子成功，您在 " << static_cast<char>('A' + player_pos_[pid]->first) << player_pos_[pid]->second << " 位置落子";
        return StageErrCode::READY;
    }

    AtomReqErrCode Pass_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (is_public) {
            reply() << "跳过失败：请私信裁判决定跳过";
            return StageErrCode::FAILED;
        }
        if (IsReady(pid)) {
            reply() << "跳过失败：您已经完成落子，无法跳过";
            return StageErrCode::FAILED;
        }
        player_pos_[pid].reset();
        reply() << "跳过成功，本回合您不进行落子";
        return StageErrCode::READY;
    }

    AtomReqErrCode Set_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::string& pos,
            const bool ready)
    {
        if (is_public) {
            reply() << "选择失败：请私信裁判选择落子位置";
            return StageErrCode::FAILED;
        }
        if (IsReady(pid)) {
            reply() << "选择失败：您已经完成落子，无法变更落子位置";
            return StageErrCode::FAILED;
        }
        if (!(player_pos_[pid] = ToPos_(reply, pos)).has_value()) {
            return StageErrCode::FAILED;
        }
        if (!board_.CanBeSet(player_pos_[pid]->first, player_pos_[pid]->second)) {
            player_pos_[pid].reset();
            reply() << "选择失败：该位置无法落子，请试试其它位置吧";
            return StageErrCode::FAILED;
        }
        if (round_ == 0 && player_pos_[pid]->first == Board::k_size_ / 2 &&
                player_pos_[pid]->second == Board::k_size_ / 2) {
            player_pos_[pid].reset();
            reply() << "选择失败：首回合不允许落子天元";
            return StageErrCode::FAILED;
        }
        if (ready) {
            reply() << "落子成功，您在 " << static_cast<char>('A' + player_pos_[pid]->first) << player_pos_[pid]->second << " 位置落子";
            return StageErrCode::READY;
        }
        reply() << Markdown(HtmlHead_() + board_.SetAndToHtml(player_pos_[pid]->first, player_pos_[pid]->second,
                    pid == 0 ? AreaType::BLACK : AreaType::WHITE));
        reply() << "选择位置成功，但是您还需要使用「落子」命令进行实际落子，落子前您可多次变更位置";
        return StageErrCode::OK;
    }

    AtomReqErrCode Info_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << Markdown(HtmlHead_() + board_.ToHtml());
        return StageErrCode::OK;
    }

    static std::optional<std::pair<uint32_t, uint32_t>> ToPos_(MsgSenderBase& reply, const std::string& str)
    {
        if (str.size() != 2 && str.size() != 3) {
            reply() << "[错误] 非法的坐标长度 " << str.size() << " ，应为 2 或 3";
            return std::nullopt;
        }
        std::pair<uint32_t, uint32_t> coor;
        const char row_c = str[0];
        if ('A' <= row_c && row_c < 'A' + Board::k_size_) {
            coor.first = row_c - 'A';
        } else if ('a' <= row_c && row_c < 'a' + Board::k_size_) {
            coor.first = row_c - 'a';
        } else {
            reply() << "[错误] 非法的横坐标「" << row_c << "」，应在 A 和 "
                    << std::string(1, static_cast<char>('A' + Board::k_size_ - 1)) << " 之间";
            return std::nullopt;
        }
        if (std::isdigit(str[1])) {
            coor.second = str[1] - '0';
        } else {
            reply() << "[错误] 非法的纵坐标「" << str.substr(1) << "」，应在 0 和 " << Board::k_size_ - 1 << " 之间";
            return std::nullopt;
        }
        if (str.size() == 2) {
            // do nothing
        } else if (std::isdigit(str[2])) {
            coor.second = 10 * coor.second + str[2] - '0';
        } else {
            reply() << "[错误] 非法的纵坐标「" << str.substr(1) << "」，应在 0 和 " << Board::k_size_ - 1 << " 之间";
            return std::nullopt;
        }
        return coor;
    }

    std::string HtmlHead_() const
    {
        std::string str = "## 第 " + std::to_string(round_ + 1);
        if (GET_OPTION_VALUE(option(), 回合上限) > 0) {
            str += " / " + std::to_string(GET_OPTION_VALUE(option(), 回合上限));
        }
        str +=  " 回合\n\n";
        html::Table player_table(3, 4);
        player_table.SetTableStyle(" align=\"center\" cellpadding=\"0\" cellspacing=\"10\" ");
        player_table.Get(0, 0).SetContent(
                "**扩展后碰撞次数：" + std::string(last_round_crashed_ && extended_ ? HTML_COLOR_FONT_HEADER(red) : "") +
                std::to_string(crash_count_) + std::string(last_round_crashed_ && extended_ ? HTML_FONT_TAIL : "") +
                " / " + std::to_string(GET_OPTION_VALUE(option(), 碰撞上限)) + "**");
        player_table.MergeRight(0, 0, 4);
        const auto print_player = [&](const PlayerID pid)
            {
                player_table.Get(1, 0 + pid * 2).SetContent(
                        std::string("![](file://") + option().ResourceDir() + (pid == 0 ? "c_b.bmp)" : "c_w.bmp)"));
                player_table.MergeDown(1, 0 + pid * 2, 2);
                player_table.Get(1, 1 + pid * 2).SetContent("**" + PlayerName(pid) + "**");
                if (last_round_passed_[pid] && !last_last_round_both_passed_) {
                    player_table.Get(2, 1 + pid * 2).SetContent(
                            HTML_COLOR_FONT_HEADER(red) "pass 次数：" + std::to_string(pass_count_[pid]) + HTML_FONT_TAIL);
                } else {
                    player_table.Get(2, 1 + pid * 2).SetContent("pass 次数：" + std::to_string(pass_count_[pid]));
                }
            };
        print_player(0);
        print_player(1);
        str += player_table.ToString();
        str += "\n\n";
        return str;
    }

    virtual void OnAllPlayerReady()
    {
        if (!SetToBoard_(BoardcastMsgSender())) {
            ClearReady();
            StartTimer(GET_OPTION_VALUE(option(), 时限));
        }
    }

    CheckoutErrCode OnTimeout()
    {
        HookUnreadyPlayers();
        if (!SetToBoard_(BoardcastMsgSender())) {
            ClearReady();
            StartTimer(GET_OPTION_VALUE(option(), 时限));
            return StageErrCode::CONTINUE;
        }
        return StageErrCode::CHECKOUT;
    }

    bool SetToBoard_(MsgSenderBase& reply)
    {
        board_.ClearHighlight();
        const auto ret =
            player_pos_[0].has_value() && player_pos_[1].has_value() ?
                board_.Set(player_pos_[0]->first, player_pos_[0]->second, player_pos_[1]->first, player_pos_[1]->second) :
            player_pos_[0].has_value() ? board_.Set(player_pos_[0]->first, player_pos_[0]->second, Pid2Type_(0)) :
            player_pos_[1].has_value() ? board_.Set(player_pos_[1]->first, player_pos_[1]->second, Pid2Type_(1)) :
                                         Result::CONTINUE_OK;
        for (int i = 0; i < 2; ++i) {
            pass_count_[i] += (last_round_passed_[i] = !player_pos_[i].has_value()) && !last_round_both_passed_;
        }
        last_last_round_both_passed_ = last_round_both_passed_;
        last_round_both_passed_ = last_round_passed_[0] && last_round_passed_[1];
        crash_count_ += (last_round_crashed_ = (ret == Result::CONTINUE_CRASH || last_round_both_passed_)) && extended_;
        reply() << Markdown(HtmlHead_() + board_.ToHtml());
        auto sender = reply();
        bool is_over = true;
        if (ret == Result::WIN_BLACK) {
            sender << "黑方连成五子，宣告胜利！";
            winner_ = 0;
        } else if (ret == Result::WIN_WHITE) {
            sender << "白方连成五子，宣告胜利！";
            winner_ = 1;
        } else if (ret == Result::TIE_DOUBLE_WIN) {
            sender << "双方均连成五子，满足了和棋条件";
        } else if (ret == Result::TIE_FULL_BOARD) {
            sender << "棋盘上没有可落子位置了，满足了和棋条件";
        } else if (GET_OPTION_VALUE(option(), pass上限) > 0 &&
                pass_count_[0] + pass_count_[1] >= GET_OPTION_VALUE(option(), pass上限)) {
            sender << "双方 pass 总次数达到上限，满足了和棋条件";
        } else {
            if (last_round_crashed_) {
                sender << "双方行动相同，发生碰撞";
                if (GET_OPTION_VALUE(option(), 碰撞上限) > 0 && extended_) {
                    sender << "，目前已发生 " << crash_count_ << " 次碰撞（当发生 " << GET_OPTION_VALUE(option(), 碰撞上限)
                           << " 次碰撞时，将满足和棋条件）";
                }
                if (last_round_both_passed_) {
                    sender << "\n\n下一回合允许继续 pass，但不推荐，理由是下回合的 pass 不会记在个人的 pass 数上";
                }
            } else if (ret == Result::CONTINUE_OK) {
                sender << "双方落子成功";
            } else if (ret == Result::CONTINUE_EXTEND) {
                sender << "双方落子成功，我们的战场得到了扩展！";
                extended_ = true;
            } else {
                abort();
            }
            if (GET_OPTION_VALUE(option(), pass上限) > 0 &&
                    (!player_pos_[0].has_value() || !player_pos_[1].has_value())) {
                sender << "\n\n本回合有玩家选择了 pass（当双方 pass 次数总和达到 "
                    << GET_OPTION_VALUE(option(), pass上限) << " 次时，将满足和棋条件）";
            }
            sender << "\n\n";
            if (++round_ == GET_OPTION_VALUE(option(), 回合上限)) {
                sender << "达到回合上限，满足了和棋条件";
            } else if (crash_count_ > 0 && crash_count_ == GET_OPTION_VALUE(option(), 碰撞上限)) {
                sender << "达到碰撞上限，满足了和棋条件";
            } else {
                sender << "游戏继续，请双方继续落子";
                is_over = false;
            }
        }
        if (is_over && !winner_.has_value() && GET_OPTION_VALUE(option(), 模式) == false) {
            if (pass_count_[0] > pass_count_[1]) {
                sender << "，但由于黑方 pass 次数多于白方，故黑方取得胜利";
                winner_ = 0;
            } else if (pass_count_[1] > pass_count_[0]) {
                sender << "，但由于白方 pass 次数多于黑方，故白方取得胜利";
                winner_ = 1;
            } else {
                sender << "，由于双方 pass 次数相同，游戏平局";
            }
        }
        player_pos_[0].reset();
        player_pos_[1].reset();
        return is_over;
    }

    static AreaType Pid2Type_(const PlayerID pid) { return pid == 0 ? AreaType::BLACK : AreaType::WHITE; }

    renju::Board board_;
    uint32_t round_;
    uint32_t crash_count_;
    bool extended_;
    std::array<std::optional<std::pair<uint32_t, uint32_t>>, 2> player_pos_;
    std::array<bool, 2> last_round_passed_;
    bool last_round_both_passed_;
    bool last_last_round_both_passed_;
    bool last_round_crashed_;
    std::array<int32_t, 2> pass_count_;
    std::optional<PlayerID> winner_;
};

MainStageBase* MakeMainStage(MsgSenderBase& reply, GameOption& options, MatchBase& match)
{
    if (!options.ToValid(reply)) {
        return nullptr;
    }
    return new MainStage(options, match);
}

