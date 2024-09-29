// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <array>
#include <map>
#include <functional>
#include <memory>
#include <vector>
#include <random>
#include <algorithm>

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"
#include "game_util/numcomb.h"

namespace comb = lgtbot::game_util::numcomb;

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;
const GameProperties k_properties { 
    .name_ = "数字蜂巢",
    .developer_ = "森高",
    .description_ = "通过放置卡牌，让同数字连成直线获得积分，比拼分数高低的游戏",
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 0; } /* 0 means no max-player limits */
uint32_t Multiple(const CustomOptions& options) { return GET_OPTION_VALUE(options, 种子).empty() ? 2 : 0; }
const MutableGenericOptions k_default_generic_options;
const std::vector<RuleCommand> k_rule_commands = {};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options) { return true; }

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options)
            {
                generic_options.bench_computers_to_player_num_ = 1;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
};

// ========== GAME STAGES ==========

static const std::array<std::vector<int32_t>, comb::k_direct_max> k_points{
    std::vector<int32_t>{3, 4, 8},
    std::vector<int32_t>{1, 5, 9},
    std::vector<int32_t>{2, 6, 7}
};

struct Player
{
    Player(std::string resource_path) : score_(0), line_count_(0), comb_(new comb::Comb(std::move(resource_path))) {}
    Player(Player&&) = default;
    int32_t score_;
    int32_t line_count_;
    std::unique_ptr<comb::Comb> comb_;
};

class RoundStage;

class MainStage : public MainGameStage<RoundStage>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility))
        , round_(0)
    {
        srand((unsigned int)time(NULL));
        const std::array<const char*, 5> skin_names = {"random", "pure", "green", "pink", "gold"};
        int skin_num = skin_names.size();
        int skin = GAME_OPTION(皮肤);
        if (GAME_OPTION(皮肤) == 0) {
            skin = rand() % (skin_num - 1) + 1;
        }
        imageDir = Global().ResourceDir() / std::filesystem::path(skin_names[skin]);

        for (uint64_t i = 0; i < Global().PlayerNum(); ++i) {
            players_.emplace_back((Global().ResourceDir() / std::filesystem::path(skin_names[skin]) / "").string());
            if (GAME_OPTION(皮肤) == 0) {
                if (++skin >= skin_num) skin = 1;
            }
        }

        for (const int32_t point_0 : k_points[0]) {
            for (const int32_t point_1 : k_points[1]) {
                for (const int32_t point_2 : k_points[2]) {
                    // two card for each type
                    cards_.emplace_back(point_0, point_1, point_2);
                    cards_.emplace_back(point_0, point_1, point_2);
                }
            }
        }
        for (uint32_t i = 0; i < GAME_OPTION(癞子); ++i) {
            cards_.emplace_back();
        }

        seed_str = GAME_OPTION(种子);
        if (seed_str.empty()) {
            std::random_device rd;
            std::uniform_int_distribution<unsigned long long> dis;
            seed_str = std::to_string(dis(rd));
        }
        std::seed_seq seed(seed_str.begin(), seed_str.end());
        std::mt19937 g(seed);
        std::shuffle(cards_.begin(), cards_.end(), g);

        it_ = cards_.begin();
        if (std::all_of(cards_.begin(), cards_.begin() + GAME_OPTION(跳过非癞子),
                [](const comb::AreaCard& card) { return !card.IsWild(); })) {
            it_ += GAME_OPTION(跳过非癞子);
        }
    }

    virtual void FirstStageFsm(SubStageFsmSetter setter) override;

    virtual void NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;

    int64_t PlayerScore(const PlayerID pid) const
    {
        return players_[pid].score_;
    }

    std::string CombHtml(const std::string& str)
    {
        html::Table table(players_.size() / 2 + 1, 2);
        table.SetTableStyle(" align=\"center\" cellpadding=\"20\" cellspacing=\"0\"");
        for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
            table.Get(pid / 2, pid % 2).SetContent("### " + Global().PlayerAvatar(pid, 40) + "&nbsp;&nbsp; " + Global().PlayerName(pid) +
                    "\n\n### " HTML_COLOR_FONT_HEADER(green) "当前积分：" + std::to_string(players_[pid].score_) + HTML_FONT_TAIL "\n\n" +
                    players_[pid].comb_->ToHtml());
        }
        if (players_.size() % 2) {
            table.MergeRight(table.Row() - 1, 0, 2);
        }
        return str + table.ToString();
    }


    std::vector<Player> players_;

    std::filesystem::path imageDir;
    std::string seed_str;

  private:
    void NewStage_(SubStageFsmSetter& setter);

    uint32_t round_;
    std::vector<comb::AreaCard> cards_;
    decltype(cards_)::iterator it_;
};

class RoundStage : public SubGameStage<>
{
  public:
    RoundStage(MainStage& main_stage, const uint64_t round, const comb::AreaCard& card)
            : StageFsm(main_stage, "第" + std::to_string(round) + "回合",
                MakeStageCommand(*this, "设置数字", &RoundStage::Set_, ArithChecker<uint32_t>(0, 19, "数字")),
                MakeStageCommand(*this, "查看本回合开始时蜂巢情况，可用于图片重发", &RoundStage::Info_, VoidChecker("赛况")))
            , card_(card)
            , comb_html_(main_stage.CombHtml("## 第 " + std::to_string(round) + " 回合"))
    {}

    virtual void OnStageBegin() override
    {
        Global().Boardcast() << "本回合砖块为 " << card_.ImageName() << "，请公屏或私信裁判设置数字：";
        SendInfo(Global().BoardcastMsgSender());
        Global().StartTimer(GAME_OPTION(局时));
    }

  private:
    void HandleUnreadyPlayers_()
    {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (!Global().IsReady(pid)) {
                auto& player = Main().players_[pid];
                const auto [idx, result] = player.comb_->SeqFill(card_);
                auto sender = Global().Boardcast();
                sender << At(pid) << "因为超时未做选择，自动填入空位置 " << idx;
                if (result.point_ > 0) {
                    sender << "，意外收获 " << result.point_ << " 点积分";
                    player.score_ += result.point_;
                    player.line_count_ += result.line_;
                }
            }
        }
        Global().HookUnreadyPlayers();
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        HandleUnreadyPlayers_();
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        HandleUnreadyPlayers_();
        return StageErrCode::CHECKOUT;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        auto& player = Main().players_[pid];
        if (const auto& [point, line] = player.comb_->SeqFill(card_).second; point > 0) {
            player.score_ += point;
            player.line_count_ += line;
        }
        return StageErrCode::READY;
    }

    AtomReqErrCode Set_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const uint32_t idx)
    {
        auto& player = Main().players_[pid];
        if (Global().IsReady(pid)) {
            reply() << "您已经设置过，无法重复设置";
            return StageErrCode::FAILED;
        }
        if (player.comb_->IsFilled(idx)) {
            reply() << "该位置已经被填过了，试试其它位置吧";
            return StageErrCode::FAILED;
        }
        const auto [point, line] = player.comb_->Fill(idx, card_);
        auto sender = reply();
        sender << "设置数字 " << idx << " 成功";
        if (point > 0) {
            sender << "，本次操作收获 " << point << " 点积分";
            player.score_ += point;
            player.line_count_ += line;
        }
        return StageErrCode::READY;
    }

    AtomReqErrCode Info_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        SendInfo(reply);
        return StageErrCode::OK;
    }

    void SendInfo(MsgSenderBase& sender)
    {
        sender() << Markdown{comb_html_};
        sender() << Image((Main().imageDir / std::filesystem::path(card_.ImageName() + ".png")).string());
    }

    const comb::AreaCard card_;
    const std::string comb_html_;
};

void MainStage::NewStage_(SubStageFsmSetter& setter)
{
    const auto round = round_;
    const auto& card = *(it_++);
    setter.Emplace<RoundStage>(*this, ++round_, card);
    return;
}

void MainStage::FirstStageFsm(SubStageFsmSetter setter)
{
    NewStage_(setter);
}

void MainStage::NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    if (round_ < GAME_OPTION(回合数)) {
        NewStage_(setter);
        return;
    }
    Global().Boardcast() << Markdown(CombHtml("## 终局"));
    if (GAME_OPTION(种子).empty()) {
        Global().Boardcast() << "本局随机数种子：" + seed_str;
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (players_[pid].line_count_ >= 12) {
                Global().Achieve(pid, Achievement::处女蜂王);
            }
            if (players_[pid].line_count_ >= 15) {
                Global().Achieve(pid, Achievement::蜂王);
            }
            if (players_[pid].score_ >= 200) {
                Global().Achieve(pid, Achievement::幼年蜂);
            }
            if (players_[pid].score_ >= 250) {
                Global().Achieve(pid, Achievement::青年蜂);
            }
            if (players_[pid].score_ >= 300) {
                Global().Achieve(pid, Achievement::壮年蜂);
            }
        }
    }
}

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

