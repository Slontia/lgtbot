#include <array>
#include <map>
#include <functional>
#include <memory>
#include <vector>
#include <random>

#include "game_framework/game_main.h"
#include "game_framework/game_stage.h"
#include "game_framework/game_options.h"
#include "utility/msg_checker.h"
#include "utility/html.h"
#include "game_util/numcomb.h"

const std::string k_game_name = "数字蜂巢";
const uint64_t k_max_player = 0; /* 0 means no max-player limits */

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

bool GameOption::IsValid(MsgSenderBase& reply) const { return true; }

uint64_t GameOption::BestPlayerNum() const { return 1; }

// ========== GAME STAGES ==========

static const std::array<std::vector<int32_t>, comb::k_direct_max> k_points{
    std::vector<int32_t>{3, 4, 8},
    std::vector<int32_t>{1, 5, 9},
    std::vector<int32_t>{2, 6, 7}
};

struct Player
{
    Player(std::string resource_path) : score_(0), comb_(new comb::Comb(std::move(resource_path))) {}
    Player(Player&&) = default;
    int32_t score_;
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
        for (uint64_t i = 0; i < option.PlayerNum(); ++i) {
            players_.emplace_back(option.ResourceDir());
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
        for (uint32_t i = 0; i < option.GET_VALUE(癞子); ++i) {
            cards_.emplace_back();
        }
        const std::string& seed_str = option.GET_VALUE(种子);
        if (seed_str.empty()) {
            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(cards_.begin(), cards_.end(), g);
        } else {
            std::seed_seq seed(seed_str.begin(), seed_str.end());
            std::mt19937 g(seed);
            std::shuffle(cards_.begin(), cards_.end(), g);
        }

        it_ = cards_.begin();
        if (std::all_of(cards_.begin(), cards_.begin() + option.GET_VALUE(跳过非癞子),
                [](const comb::AreaCard& card) { return !card.IsWild(); })) {
            it_ += option.GET_VALUE(跳过非癞子);
        }
    }

    virtual VariantSubStage OnStageBegin() override;

    virtual VariantSubStage NextSubStage(RoundStage& sub_stage, const CheckoutReason reason) override;

    int64_t PlayerScore(const PlayerID pid) const
    {
        return players_[pid].score_;
    }

    std::string CombHtml(std::string str)
    {
        for (PlayerID pid = 0; pid < players_.size(); ++pid) {
            str += "\n\n### ";
            str += PlayerName(pid);
            str += "（当前积分：";
            str += std::to_string(players_[pid].score_);
            str += "）\n\n";
            str += players_[pid].comb_->ToHtml();
        }
        return str;
    }


    std::vector<Player> players_;

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
            , comb_html_(main_stage.CombHtml("## 第" + std::to_string(round) + "回合"))
    {}

    virtual void OnStageBegin() override
    {
        Boardcast() << "本回合砖块如下，请公屏或私信裁判设置数字：";
        SendInfo(BoardcastMsgSender());
        StartTimer(option().GET_VALUE(局时));
    }

  private:
    CheckoutErrCode OnTimeout()
    {
        for (PlayerID pid = 0; pid < option().PlayerNum(); ++pid) {
            if (!IsReady(pid)) {
                Boardcast() << At(pid) << "因为超时未做选择，被淘汰";
                Eliminate(pid);
            }
        }
        return StageErrCode::CHECKOUT;
    }

    AtomReqErrCode Set_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const uint32_t idx)
    {
        auto& player = main_stage().players_[pid];
        if (IsReady(pid)) {
            reply() << "您已经设置过，无法重复设置";
            return StageErrCode::FAILED;
        }
        if (player.comb_->IsFilled(idx)) {
            reply() << "改位置已经被填过了，试试其它位置吧";
            return StageErrCode::FAILED;
        }
        const auto point = player.comb_->Fill(idx, card_);
        auto sender = reply();
        sender << "设置数字" << idx << "成功";
        if (point > 0) {
            sender << "，本次操作收获" << point << "点积分";
        }
        player.score_ += point;
        return StageErrCode::READY;
    }

    AtomReqErrCode Info_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        SendInfo(reply);
        return StageErrCode::OK;
    }

    void SendInfo(MsgSenderBase& sender)
    {
        sender() << Markdown(comb_html_);
        sender() << Image(std::string(option().ResourceDir() + card_.ImageName()) + ".png");
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
    if (round_ == option().GET_VALUE(回合数)) {
        Boardcast() << Markdown(CombHtml("## 终局"));
        return {};
    }
    return NewStage_();
}

MainStageBase* MakeMainStage(MsgSenderBase& reply, const GameOption& options, MatchBase& match)
{
    if (!options.IsValid(reply)) {
        return nullptr;
    }
    return new MainStage(options, match);
}
