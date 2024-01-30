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

#include "game_framework/game_stage.h"
#include "utility/html.h"
#include "game_util/numcomb.h"

namespace comb = lgtbot::game_util::numcomb;

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = GameStage<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = GameStage<void, SubStages...>;

const std::string k_game_name = "数字蜂巢";
const uint64_t k_max_player = 0; /* 0 means no max-player limits */
const uint64_t k_multiple = 1;
const std::string k_developer = "森高";
const std::string k_description = "通过放置卡牌，让同数字连成直线获得积分，比拼分数高低的游戏";
const std::vector<RuleCommand> k_rule_commands = {};

std::string GameOption::StatusInfo() const
{
    std::string str = "每回合" + std::to_string(GET_VALUE(局时)) + "秒，共" +
        std::to_string(GET_VALUE(回合数)) + "回合，跳过起始非癞子数量" + std::to_string(GET_VALUE(跳过非癞子)) + "，";
    if (GET_VALUE(种子).empty()) {
        str += "未指定种子";
    } else {
        str += "种子：" + GET_VALUE(种子);
    }
    return str;
}

bool GameOption::ToValid(MsgSenderBase& reply) { return true; }

uint64_t GameOption::BestPlayerNum() const { return 1; }

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
    MainStage(const GameOption& option, MatchBase& match)
        : GameStage(option, match)
        , round_(0)
    {
        srand((unsigned int)time(NULL));
        const std::array<const char*, 6> skin_names = {"random", "pure", "green", "pink", "gold"};
        int skin_num = skin_names.size();
        int skin = GET_OPTION_VALUE(option, 皮肤);
        if (GET_OPTION_VALUE(option, 皮肤) == 0) {
            skin = rand() % (skin_num - 1) + 1;
        }
        imageDir = option.ResourceDir() / std::filesystem::path(skin_names[skin]) / "";

        for (uint64_t i = 0; i < option.PlayerNum(); ++i) {
            players_.emplace_back(option.ResourceDir() / std::filesystem::path(skin_names[skin]) / "");
            if (GET_OPTION_VALUE(option, 皮肤) == 0) {
                skin++;
                if (skin >= skin_num) skin = 1;
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
        for (uint32_t i = 0; i < GET_OPTION_VALUE(option, 癞子); ++i) {
            cards_.emplace_back();
        }

        seed_str = GET_OPTION_VALUE(option, 种子);
        if (seed_str.empty()) {
            std::random_device rd;
            std::uniform_int_distribution<unsigned long long> dis;
            seed_str = std::to_string(dis(rd));
        }
        std::seed_seq seed(seed_str.begin(), seed_str.end());
        std::mt19937 g(seed);
        std::shuffle(cards_.begin(), cards_.end(), g);

        it_ = cards_.begin();
        if (std::all_of(cards_.begin(), cards_.begin() + GET_OPTION_VALUE(option, 跳过非癞子),
                [](const comb::AreaCard& card) { return !card.IsWild(); })) {
            it_ += GET_OPTION_VALUE(option, 跳过非癞子);
        }
    }

    virtual VariantSubStage OnStageBegin() override;

    virtual VariantSubStage NextSubStage(RoundStage& sub_stage, const CheckoutReason reason) override;

    int64_t PlayerScore(const PlayerID pid) const
    {
        return players_[pid].score_;
    }

    std::string CombHtml(const std::string& str)
    {
        html::Table table(players_.size() / 2 + 1, 2);
        table.SetTableStyle(" align=\"center\" cellpadding=\"20\" cellspacing=\"0\"");
        for (PlayerID pid = 0; pid < players_.size(); ++pid) {
            table.Get(pid / 2, pid % 2).SetContent("### " + PlayerAvatar(pid, 40) + "&nbsp;&nbsp; " + PlayerName(pid) +
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
    VariantSubStage NewStage_();

    uint32_t round_;
    std::vector<comb::AreaCard> cards_;
    decltype(cards_)::iterator it_;
};

class RoundStage : public SubGameStage<>
{
  public:
    RoundStage(MainStage& main_stage, const uint64_t round, const comb::AreaCard& card)
            : GameStage(main_stage, "第" + std::to_string(round) + "回合",
                MakeStageCommand("设置数字", &RoundStage::Set_, ArithChecker<uint32_t>(0, 19, "数字")),
                MakeStageCommand("查看本回合开始时蜂巢情况，可用于图片重发", &RoundStage::Info_, VoidChecker("赛况")))
            , card_(card)
            , comb_html_(main_stage.CombHtml("## 第 " + std::to_string(round) + " 回合"))
    {}

    virtual void OnStageBegin() override
    {
        Boardcast() << "本回合砖块为 " << card_.ImageName() << "，请公屏或私信裁判设置数字：";
        SendInfo(BoardcastMsgSender());
        StartTimer(GET_OPTION_VALUE(option(), 局时));
    }

  private:
    void HandleUnreadyPlayers_()
    {
        for (PlayerID pid = 0; pid < option().PlayerNum(); ++pid) {
            if (!IsReady(pid)) {
                auto& player = main_stage().players_[pid];
                const auto [idx, result] = player.comb_->SeqFill(card_);
                auto sender = Boardcast();
                sender << At(pid) << "因为超时未做选择，自动填入空位置 " << idx;
                if (result.point_ > 0) {
                    sender << "，意外收获 " << result.point_ << " 点积分";
                    player.score_ += result.point_;
                    player.line_count_ += result.line_;
                }
                Hook(pid);
            }
        }
    }

    virtual CheckoutErrCode OnTimeout() override
    {
        HandleUnreadyPlayers_();
        return StageErrCode::CHECKOUT;
    }

    virtual void OnAllPlayerReady() override
    {
        HandleUnreadyPlayers_();
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (IsReady(pid)) {
            return StageErrCode::OK;
        }
        auto& player = main_stage().players_[pid];
        if (const auto& [point, line] = player.comb_->SeqFill(card_).second; point > 0) {
            player.score_ += point;
            player.line_count_ += line;
        }
        return StageErrCode::READY;
    }

    AtomReqErrCode Set_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const uint32_t idx)
    {
        auto& player = main_stage().players_[pid];
        if (IsReady(pid)) {
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
        sender() << Image(std::string(main_stage().imageDir / card_.ImageName() / ".png"));
    }

    const comb::AreaCard card_;
    const std::string comb_html_;
};

MainStage::VariantSubStage MainStage::NewStage_()
{
    const auto round = round_;
    const auto& card = *(it_++);
    return std::make_unique<RoundStage>(*this, ++round_, card);
}

MainStage::VariantSubStage MainStage::OnStageBegin()
{
    return NewStage_();
}

MainStage::VariantSubStage MainStage::NextSubStage(RoundStage& sub_stage, const CheckoutReason reason)
{
    if (round_ < GET_OPTION_VALUE(option(), 回合数)) {
        return NewStage_();
    }
    Boardcast() << Markdown(CombHtml("## 终局"));
    if (GET_OPTION_VALUE(option(), 种子).empty()) {
        Boardcast() << "本局随机数种子：" + seed_str;
        for (PlayerID pid = 0; pid < option().PlayerNum(); ++pid) {
            if (players_[pid].line_count_ >= 12) {
                global_info().Achieve(pid, Achievement::处女蜂王);
            }
            if (players_[pid].line_count_ >= 15) {
                global_info().Achieve(pid, Achievement::蜂王);
            }
            if (players_[pid].score_ >= 200) {
                global_info().Achieve(pid, Achievement::幼年蜂);
            }
            if (players_[pid].score_ >= 250) {
                global_info().Achieve(pid, Achievement::青年蜂);
            }
            if (players_[pid].score_ >= 300) {
                global_info().Achieve(pid, Achievement::壮年蜂);
            }
        }
    }
    return {};
}

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

#include "game_framework/make_main_stage.h"
