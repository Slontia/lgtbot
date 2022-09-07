// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <array>
#include <functional>
#include <memory>
#include <set>

#include "game_framework/game_main.h"
#include "game_framework/game_stage.h"
#include "game_framework/game_options.h"
#include "utility/msg_checker.h"
#include "utility/html.h"

const std::string k_game_name = "伊甸园"; // the game name which should be unique among all the games
const uint64_t k_max_player = 0; // 0 indicates no max-player limits
const uint64_t k_multiple = 0; // the default score multiple for the game, 0 for a testing game, 1 for a formal game, 2 or 3 for a long formal game

std::string GameOption::StatusInfo() const
{
    return "共 " + std::to_string(GET_VALUE(回合数)) + " 回合，每回合超时时间 " + std::to_string(GET_VALUE(时限)) + " 秒";
}

bool GameOption::ToValid(MsgSenderBase& reply)
{
    if (PlayerNum() < 3) {
        reply() << "该游戏至少 3 人参加，当前玩家数为 " << PlayerNum();
        return false;
    }
    return true;
}

uint64_t GameOption::BestPlayerNum() const { return 10; }

// ========== GAME STAGES ==========

static constexpr const uint32_t k_apple_type_num = 3;
enum class AppleType { RED, GOLD, SILVER };

static const char* AppleTypeStr(const AppleType type)
{
    static const char* k_apple_type_str[] =
    {
        [static_cast<int>(AppleType::RED)] = "红",
        [static_cast<int>(AppleType::GOLD)] = "金",
        [static_cast<int>(AppleType::SILVER)] = "银",
    };
    return k_apple_type_str[static_cast<int>(type)];
}

struct Player
{
    Player(const GameOption& option)
        : score_(0)
        , remain_golden_(option.GET_VALUE(金苹果))
        , chosen_apples_(option.GET_VALUE(回合数), AppleType::RED) // RED is the default apple
    {}
    int64_t score_;
    int32_t remain_golden_;
    std::vector<AppleType> chosen_apples_;
};

class MainStage : public MainGameStage<>
{
  public:
    MainStage(const GameOption& option, MatchBase& match)
        : GameStage(option, match,
                MakeStageCommand("查看当前游戏进展情况", &MainStage::Status_, VoidChecker("赛况")),
                MakeStageCommand("选择苹果", &MainStage::Choose_, AlterChecker<AppleType>(
                        {{"红", AppleType::RED}, {"金", AppleType::GOLD}, {"银", AppleType::SILVER}})))
        , round_(0)
        , players_(option.PlayerNum(), option)
    {
    }

    virtual void OnStageBegin() override
    {
        StartTimer(option().GET_VALUE(时限));
        Boardcast() << "第 1 回合开始，请私信裁判选择，时限 " << option().GET_VALUE(时限) << " 秒，超时则默认食用「红苹果」";
    }

    virtual int64_t PlayerScore(const PlayerID pid) const override { return players_[pid].score_; }

    CheckoutErrCode OnTimeout()
    {
        HookUnreadyPlayers();
        return CheckoutErrCode::Condition(Over_(), StageErrCode::CHECKOUT, StageErrCode::CONTINUE);
    }

    virtual void OnAllPlayerReady() override
    {
        Over_();
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (IsReady(pid)) {
            return StageErrCode::OK;
        }
        auto type = static_cast<AppleType>(rand() % k_apple_type_num);
        if (type == AppleType::GOLD) {
            if (players_[pid].remain_golden_ == 0) {
                type = static_cast<AppleType>(rand() % 2 ? AppleType::RED : AppleType::SILVER);
            } else {
                --players_[pid].remain_golden_;
            }
        }
        players_[pid].chosen_apples_[round_] = type;
        return StageErrCode::READY;
    }

  private:

    std::array<int, k_apple_type_num> AppleCounts_() const
    {
        std::array<int, k_apple_type_num> result{0};
        for (PlayerID pid = 0; pid < option().PlayerNum(); ++pid) {
            ++result[static_cast<int>(players_[pid].chosen_apples_[round_])];
        }
        return result;
    }

    AppleType WinnerAppleFirstRound_(const std::array<int, k_apple_type_num>& apple_counts, MsgSenderBase::MsgSenderGuard& sender) const
    {
        const int min_apple_count = *std::min_element(apple_counts.begin(), apple_counts.end());
        std::optional<AppleType> result;
        for (const auto apple_type : { AppleType::RED, AppleType::GOLD, AppleType::SILVER }) {
            if (apple_counts[static_cast<int>(apple_type)] != min_apple_count) {
                // do nothing
            } else if (result.has_value()) {
                sender << "有多种苹果食用数最低，因此";
                return AppleType::GOLD;
            } else {
                result = apple_type;
            }
        }
        return *result;
    }

    AppleType WinnerApple_(const std::array<int, k_apple_type_num>& apple_counts, MsgSenderBase::MsgSenderGuard& sender) const
    {
        const auto is_satisfied_red_apple_requirement = [&](const Player& player)
        {
            return player.chosen_apples_[round_ - 1] != AppleType::RED || player.chosen_apples_[round_] == AppleType::RED;
        };
        if (std::all_of(players_.begin(), players_.end(), is_satisfied_red_apple_requirement)) {
            sender << "上一回合食用「红苹果」的玩家，本回合依然食用「红苹果」，因此";
            return AppleType::RED;
        }
        return apple_counts[static_cast<int>(AppleType::GOLD)] <= apple_counts[static_cast<int>(AppleType::SILVER)] ?
            AppleType::GOLD : AppleType::SILVER;
    }

    bool Over_()
    {
        const auto apple_counts = AppleCounts_();
        {
            auto sender = Boardcast();
            sender << "第 " << (round_ + 1) << " 回合结束，下面公布结果\n\n";
            for (PlayerID pid = 0; pid < players_.size(); ++pid) {
                sender << At(pid) << " " << AppleTypeStr(players_[pid].chosen_apples_[round_]) << "\n";
            }
            sender << "\n红:金:银 = " << apple_counts[static_cast<int>(AppleType::RED)] << ":" << apple_counts[static_cast<int>(AppleType::GOLD)] << ":" << apple_counts[static_cast<int>(AppleType::SILVER)];
        }
        {
            auto sender = Boardcast();
            if (apple_counts[static_cast<int>(AppleType::RED)] == option().PlayerNum()) {
                sender << "不得了了，竟然全员都食用了「红苹果」，全员分数因此取反";
                for (auto& player : players_) {
                    player.score_ = -player.score_;
                }
            } else {
                const auto winner_apple =
                    round_ == 0 ? WinnerAppleFirstRound_(apple_counts, sender) : WinnerApple_(apple_counts, sender);
                if (apple_counts[static_cast<int>(winner_apple)] == 0) {
                    sender << "禁果为无人食用的「" << AppleTypeStr(winner_apple) << "苹果」";
                } else {
                    sender << "禁果为「" << AppleTypeStr(winner_apple) << "苹果」，获胜玩家分别为————\n";
                    for (PlayerID pid = 0; pid < option().PlayerNum(); ++pid) {
                        if (players_[pid].chosen_apples_[round_] == winner_apple) {
                            sender << At(pid) << "\n";
                            players_[pid].score_ += option().PlayerNum() - apple_counts[static_cast<int>(winner_apple)];
                        } else {
                            players_[pid].score_ -= apple_counts[static_cast<int>(winner_apple)];
                        }
                    }
                }
            }
        }
        {
            auto sender = Boardcast();
            sender << "公布玩家积分————\n\n";
            for (PlayerID pid = 0; pid < players_.size(); ++pid) {
                sender << At(pid) << "(剩余金苹果 " << players_[pid].remain_golden_ << ") 积分 " << players_[pid].score_ << "\n";
            }
        }
        if (++round_ < option().GET_VALUE(回合数)) {
            ClearReady();
            StartTimer(option().GET_VALUE(时限));
            Boardcast() << "第 " << (round_ + 1) << " 回合开始，请私信裁判选择，时限 " << option().GET_VALUE(时限)
                << " 秒，超时则默认食用「红苹果」";
            return false;
        }
        return true;
    }

    AtomReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << "试玩版暂不支持";
        return StageErrCode::OK;
    }

    AtomReqErrCode Choose_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const AppleType type)
    {
        if (is_public) {
            reply() << "选择失败：请私信裁判进行选择";
            return StageErrCode::FAILED;
        }
        if (IsReady(pid)) {
            reply() << "选择失败：您已经选择了「" << AppleTypeStr(players_[pid].chosen_apples_[round_]) << "苹果」";
            return StageErrCode::FAILED;
        }
        if (type == AppleType::GOLD) {
            if (players_[pid].remain_golden_ == 0) {
                reply() << "选择失败：您的「金苹果」已用尽";
                return StageErrCode::FAILED;
            }
            --players_[pid].remain_golden_;
        }
        players_[pid].chosen_apples_[round_] = type;
        reply() << "选择「" << AppleTypeStr(type) << "苹果」成功";
        return StageErrCode::READY;
    }

    int round_;
    std::vector<Player> players_;
};

MainStageBase* MakeMainStage(MsgSenderBase& reply, GameOption& options, MatchBase& match)
{
    if (!options.ToValid(reply)) {
        return nullptr;
    }
    return new MainStage(options, match);
}
