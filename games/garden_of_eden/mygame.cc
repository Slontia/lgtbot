// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <array>
#include <functional>
#include <memory>
#include <set>

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
    .name_ = "伊甸园", // the game name which should be unique among all the games
    .developer_ = "森高",
    .description_ = "吃下禁果，获取分数的游戏",
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 0; } // 0 indicates no max-player limits
uint32_t Multiple(const CustomOptions& options) { return GET_OPTION_VALUE(options, 回合数) / 6; }
const MutableGenericOptions k_default_generic_options;
const std::vector<RuleCommand> k_rule_commands = {};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() < 3) {
        reply() << "该游戏至少 3 人参加，当前玩家数为 " << generic_options_readonly.PlayerNum();
        return false;
    }
    return true;
}

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options)
            {
                generic_options.bench_computers_to_player_num_ = 10;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
};

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

static const char* AppleTypeLightColor(const AppleType type)
{
    static const char* k_apple_type_color[] =
    {
        [static_cast<int>(AppleType::RED)] = "#FFC1C1",
        [static_cast<int>(AppleType::GOLD)] = "#FFDEAD",
        [static_cast<int>(AppleType::SILVER)] = "#DCDCDC",
    };
    return k_apple_type_color[static_cast<int>(type)];
}

static const char* AppleTypeDeepColor(const AppleType type)
{
    static const char* k_apple_type_color[] =
    {
        [static_cast<int>(AppleType::RED)] = "#B22222",
        [static_cast<int>(AppleType::GOLD)] = "#DAA520",
        [static_cast<int>(AppleType::SILVER)] = "#696969",
    };
    return k_apple_type_color[static_cast<int>(type)];
}

static const char* AppleTypeName(const AppleType type)
{
    static const char* k_apple_type_name[] =
    {
        [static_cast<int>(AppleType::RED)] = "red",
        [static_cast<int>(AppleType::GOLD)] = "gold",
        [static_cast<int>(AppleType::SILVER)] = "silver",
    };
    return k_apple_type_name[static_cast<int>(type)];
}

struct Player
{
    Player(const CustomOptions& option)
        : score_(GET_OPTION_VALUE(option, 回合数), 0)
        , remain_golden_(GET_OPTION_VALUE(option, 金苹果))
        , chosen_apples_(GET_OPTION_VALUE(option, 回合数), AppleType::RED) // RED is the default apple
    {}
    std::vector<int64_t> score_;
    int32_t remain_golden_;
    std::vector<AppleType> chosen_apples_;
};

class MainStage : public MainGameStage<>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility),
                MakeStageCommand(*this, "查看当前游戏进展情况", &MainStage::Status_, VoidChecker("赛况")),
                MakeStageCommand(*this, "选择苹果", &MainStage::Choose_, AlterChecker<AppleType>(
                        {{"红", AppleType::RED}, {"金", AppleType::GOLD}, {"银", AppleType::SILVER}})))
        , round_(0)
        , players_(Global().PlayerNum(), Global().Options())
    {
    }

    virtual void OnStageBegin() override
    {
        Global().StartTimer(GAME_OPTION(时限));
        Global().Boardcast() << "第 1 回合开始，请私信裁判选择，时限 " << GAME_OPTION(时限) << " 秒，超时则默认食用「红苹果」";
    }

    virtual int64_t PlayerScore(const PlayerID pid) const override { return players_[pid].score_.back(); }

    CheckoutErrCode OnStageTimeout()
    {
        Global().HookUnreadyPlayers();
        return CheckoutErrCode::Condition(Over_(), StageErrCode::CHECKOUT, StageErrCode::CONTINUE);
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        return CheckoutErrCode::Condition(Over_(), StageErrCode::CHECKOUT, StageErrCode::CONTINUE);
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
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
    std::string Image_(const AppleType apple_type, const int size) const
    {
        return std::string("![](file:///") + Global().ResourceDir() + "/" + AppleTypeName(apple_type) + "_" + std::to_string(size) + ".png)";
    }

    std::array<int, k_apple_type_num> AppleCounts_() const
    {
        std::array<int, k_apple_type_num> result{0};
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            ++result[static_cast<int>(players_[pid].chosen_apples_[round_])];
        }
        return result;
    }

    AppleType WinnerAppleFirstRound_(const std::array<int, k_apple_type_num>& apple_counts, const char *& hint) const
    {
        const int min_apple_count = *std::min_element(apple_counts.begin(), apple_counts.end());
        std::optional<AppleType> result;
        for (const auto apple_type : { AppleType::RED, AppleType::GOLD, AppleType::SILVER }) {
            if (apple_counts[static_cast<int>(apple_type)] != min_apple_count) {
                // do nothing
            } else if (result.has_value()) {
                hint = "有多种苹果食用数最低，因此";
                return AppleType::GOLD;
            } else {
                result = apple_type;
            }
        }
        return *result;
    }

    AppleType WinnerApple_(const std::array<int, k_apple_type_num>& apple_counts, const char*& hint) const
    {
        const auto is_satisfied_red_apple_requirement = [&](const Player& player)
        {
            return player.chosen_apples_[round_ - 1] != AppleType::RED || player.chosen_apples_[round_] == AppleType::RED;
        };
        if (std::all_of(players_.begin(), players_.end(), is_satisfied_red_apple_requirement)) {
            hint = "上一回合食用「红苹果」的玩家，本回合依然食用「红苹果」，因此";
            return AppleType::RED;
        }
        return apple_counts[static_cast<int>(AppleType::GOLD)] <= apple_counts[static_cast<int>(AppleType::SILVER)] ?
            AppleType::GOLD : AppleType::SILVER;
    }

    std::string Html_(const std::array<int, k_apple_type_num>& apple_counts, const AppleType& winner_apple) const
    {
        std::string s = "## 第 " + std::to_string(round_ + 1) + " / " + std::to_string(GAME_OPTION(回合数)) + " 回合\n\n";

        {
            html::Table count_table(2, k_apple_type_num);
            count_table.SetTableStyle(" align=\"center\" cellpadding=\"1\" cellspacing=\"1\" ");
            for (const auto apple_type : { AppleType::RED, AppleType::GOLD, AppleType::SILVER }) {
                count_table.Get(0, static_cast<int>(apple_type)).SetContent(Image_(apple_type, 128));
                if (winner_apple == apple_type) {
                    count_table.Get(0, static_cast<int>(apple_type)).SetColor(AppleTypeLightColor(apple_type));
                }
                count_table.Get(1, static_cast<int>(apple_type)).SetContent("<font size=\"9\" color=" +
                        std::string(winner_apple == apple_type ? AppleTypeDeepColor(apple_type) : "black") + "> <b> " +
                        std::to_string(apple_counts[static_cast<int>(apple_type)]) + " </b> </font>");
            }
            s += count_table.ToString() + "\n\n";
        }

        std::vector<PlayerID> sorted_players(Global().PlayerNum());
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            sorted_players[pid] = pid;
        }
        std::sort(sorted_players.begin(), sorted_players.end(),
                [&](const PlayerID& _1, const PlayerID& _2) { return players_[_1].score_[round_] > players_[_2].score_[round_]; });

        constexpr const uint32_t k_battery_len = 5;
        for (const auto& pid : sorted_players) {
            const auto& player = players_[pid];
            const int64_t last_score = round_ == 0 ? 0 : player.score_[round_ - 1];
            const int64_t cur_score = player.score_[round_];
            const bool is_quit_red_apple = round_ > 0 && player.chosen_apples_[round_ - 1] == AppleType::RED &&
                player.chosen_apples_[round_] != AppleType::RED;
            s += "### " + Global().PlayerAvatar(pid, 40) + "&nbsp;&nbsp; ";
            if (is_quit_red_apple) {
                s += HTML_COLOR_FONT_HEADER(red);
            }
            s += Global().PlayerName(pid);
            if (is_quit_red_apple) {
                s += HTML_FONT_TAIL;
            }
            s += "（" + std::string(player.remain_golden_ == 0 ? HTML_COLOR_FONT_HEADER(#696969) : HTML_COLOR_FONT_HEADER(#DAA520)) + "剩余金苹果：" +
                std::to_string(player.remain_golden_) + HTML_FONT_TAIL "，得分：" + std::to_string(last_score) + " → ";
            if (cur_score > last_score) {
                s += HTML_COLOR_FONT_HEADER(green);
            } else if (cur_score < last_score) {
                s += HTML_COLOR_FONT_HEADER(red);
            } else {
                s += HTML_COLOR_FONT_HEADER(black);
            }
            s += std::to_string(cur_score) + HTML_FONT_TAIL "）\n\n";
            html::Table table(2, round_ + 1 + round_ / k_battery_len);
            table.SetTableStyle(" align=\"center\" cellpadding=\"1\" cellspacing=\"1\" ");
            for (int i = k_battery_len; i < table.Column(); i += 1 + k_battery_len) {
                table.Get(0, i).SetContent("&nbsp;&nbsp;"); // a gap between each 5 apples
            }
            for (int i = 0; i < round_; ++i) {
                const uint32_t col = i + i / k_battery_len;
                table.Get(0, col).SetContent(std::to_string(player.score_[i]));
                table.Get(1, col).SetContent(Image_(player.chosen_apples_[i], 32));
                if (player.score_[i] > (i == 0 ? 0 : player.score_[i - 1])) {
                    table.Get(1, col).SetColor(AppleTypeLightColor(player.chosen_apples_[i]));
                }
            }
            table.MergeColumn(table.Column() - 1);
            table.GetLastColumn(0).SetContent(Image_(player.chosen_apples_[round_], 64));
            if (cur_score > last_score) {
                table.GetLastColumn(0).SetColor(AppleTypeLightColor(player.chosen_apples_[round_]));
            }
            s += table.ToString() + "\n\n";
        }

        return s;
    }

    bool Over_()
    {
        const auto apple_counts = AppleCounts_();
        Global().Boardcast() << "第 " << (round_ + 1) << " 回合结束，下面公布结果";
        {
            if (round_ > 0 && apple_counts[static_cast<int>(AppleType::RED)] == Global().PlayerNum()) {
                for (auto& player : players_) {
                    player.score_[round_] = -player.score_[round_ - 1];
                }
                Global().Boardcast() << Markdown{html_ = Html_(apple_counts, AppleType::RED), k_markdown_width_};
                Global().Boardcast() << "不得了了，竟然全员都食用了「红苹果」，全员分数因此取反";
            } else {
                const char* hint = "";
                const auto winner_apple =
                    round_ == 0 ? WinnerAppleFirstRound_(apple_counts, hint) : WinnerApple_(apple_counts, hint);
                for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
                    auto& player = players_[pid];
                    const int64_t last_score = round_ == 0 ? 0 : player.score_[round_ - 1];
                    if (player.chosen_apples_[round_] == winner_apple) {
                        player.score_[round_] = last_score + Global().PlayerNum() - apple_counts[static_cast<int>(winner_apple)];
                    } else {
                        player.score_[round_] = last_score - apple_counts[static_cast<int>(winner_apple)];
                    }
                }
                Global().Boardcast() << Markdown{html_ = Html_(apple_counts, winner_apple), k_markdown_width_};
                if (apple_counts[static_cast<int>(winner_apple)] == 0) {
                    Global().Boardcast() << hint << "禁果为无人食用的「" << AppleTypeStr(winner_apple) << "苹果」";
                } else {
                    auto sender = Global().Boardcast();
                    sender << hint << "禁果为「" << AppleTypeStr(winner_apple) << "苹果」，获胜玩家分别为————\n";
                    for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
                        if (players_[pid].chosen_apples_[round_] == winner_apple) {
                            sender << At(pid) << "\n";
                        }
                    }
                }
            }
        }
        if (++round_ < GAME_OPTION(回合数)) {
            Global().ClearReady();
            Global().StartTimer(GAME_OPTION(时限));
            Global().Boardcast() << "第 " << (round_ + 1) << " 回合开始，请私信裁判选择，时限 " << GAME_OPTION(时限)
                << " 秒，超时则默认食用「红苹果」";
            return false;
        }
        return true;
    }

    AtomReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (html_.empty()) {
            reply() << "暂无赛况";
            return StageErrCode::OK;
        }
        reply() << Markdown{html_, k_markdown_width_};
        return StageErrCode::OK;
    }

    AtomReqErrCode Choose_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const AppleType type)
    {
        if (is_public) {
            reply() << "选择失败：请私信裁判进行选择";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
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
    std::string html_;
    static constexpr uint32_t k_markdown_width_ = 650;
};

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

