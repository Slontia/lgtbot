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
#include "game_util/alchemist.h"

const std::string k_game_name = "炼金术士";
const uint64_t k_max_player = 0; /* 0 means no max-player limits */

std::string GameOption::StatusInfo() const
{
    std::string str = "\n每回合" + std::to_string(GET_VALUE(局时)) + "秒\n当有玩家达到" +
        std::to_string(GET_VALUE(胜利分数)) + "分，或游戏已经进行了" +
        std::to_string(GET_VALUE(回合数)) + "回合时，游戏结束\n卡片包含" +
        std::to_string(GET_VALUE(颜色)) + "种颜色和" +
        std::to_string(GET_VALUE(点数)) + "种点数，每种相同卡片共有" +
        std::to_string(GET_VALUE(副数)) + "张\n";
    if (GET_VALUE(种子).empty()) {
        str += "未指定种子";
    } else {
        str += "种子：" + GET_VALUE(种子);
    }
    return str;
}

bool GameOption::IsValid(MsgSenderBase& reply) const
{
    const auto card_num = GET_VALUE(颜色) * GET_VALUE(点数) * GET_VALUE(副数);
    if (GET_VALUE(回合数) > card_num) {
        reply() << "回合数" << GET_VALUE(回合数) << "不能大于卡片总数量" << card_num;
        return false;
    }
    return true;
}

uint64_t GameOption::BestPlayerNum() const { return 1; }

// ========== GAME STAGES ==========

const std::array<alchemist::Color, 6> k_colors {
    alchemist::Color::RED,
    alchemist::Color::BLUE,
    alchemist::Color::YELLOW,
    alchemist::Color::GREY,
    alchemist::Color::ORANGE,
    alchemist::Color::PURPLE
};

const std::array<alchemist::Point, 6> k_points {
    alchemist::Point::ONE,
    alchemist::Point::TWO,
    alchemist::Point::THREE,
    alchemist::Point::FOUR,
    alchemist::Point::FIVE,
    alchemist::Point::SIX,
};

struct Player
{
    Player(std::string resource_path) : score_(0), board_(new alchemist::Board(std::move(resource_path))) {}
    Player(Player&&) = default;
    int32_t score_;
    std::unique_ptr<alchemist::Board> board_;
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
            std::cout << "path: " << option.ResourceDir() << std::endl;
            players_.emplace_back(option.ResourceDir());
        }

        for (uint32_t color_idx = 0; color_idx < option.GET_VALUE(颜色); ++color_idx) {
            for (uint32_t point_idx = 0; point_idx < option.GET_VALUE(点数); ++point_idx) {
                for (uint32_t i = 0; i < option.GET_VALUE(副数); ++i) {
                    cards_.emplace_back(std::in_place, k_colors[color_idx], k_points[point_idx]);
                }
            }
        }
        for (uint32_t i = 0; i < option.GET_VALUE(副数); ++i) {
            cards_.emplace_back(std::nullopt);
        }
        const std::string& seed_str = option.GET_VALUE(种子);
        std::variant<std::random_device, std::seed_seq> rd;
        std::mt19937 g([&]
            {
                if (seed_str.empty()) {
                    auto& real_rd = rd.emplace<std::random_device>();
                    return std::mt19937(real_rd());
                } else {
                    auto& real_rd = rd.emplace<std::seed_seq>(seed_str.begin(), seed_str.end());
                    return std::mt19937(real_rd);
                }
            }());
        std::shuffle(cards_.begin(), cards_.end(), g);

        std::uniform_int_distribution<uint32_t> distrib(0, alchemist::Board::k_size - 1);
        static constexpr const uint32_t k_stone_num = 2;
        for (int i = 0; i < k_stone_num; ++i) {
            const auto row = distrib(g);
            const auto col = distrib(g);
            for (auto& player : players_) {
                player.board_->SetStone(row, col);
            }
        }
    }

    virtual VariantSubStage OnStageBegin() override;

    virtual VariantSubStage NextSubStage(RoundStage& sub_stage, const CheckoutReason reason) override;

    int64_t PlayerScore(const PlayerID pid) const
    {
        return players_[pid].score_;
    }

    std::string BoardHtml(std::string str)
    {
        for (PlayerID pid = 0; pid < players_.size(); ++pid) {
            str += "\n\n### ";
            str += PlayerName(pid);
            str += "（当前积分：";
            str += std::to_string(players_[pid].score_);
            str += "）\n\n";
            str += players_[pid].board_->ToHtml();
        }
        return str;
    }


    std::vector<Player> players_;

  private:
    VariantSubStage NewStage_();

    uint32_t round_;
    std::vector<std::optional<alchemist::Card>> cards_;
};

class RoundStage : public SubGameStage<>
{
  public:
    RoundStage(MainStage& main_stage, const uint64_t round, const std::optional<alchemist::Card>& card)
            : GameStage(main_stage, "第" + std::to_string(round) + "回合",
                MakeStageCommand("查看本回合开始时盘面情况，可用于图片重发", &RoundStage::Info_, VoidChecker("赛况")),
                MakeStageCommand("跳过该回合行动", &RoundStage::Pass_, VoidChecker("pass")),
                MakeStageCommand("设置卡片", &RoundStage::Set_, AnyArg("坐标", "C5")))
            , card_(card)
            , board_html_(main_stage.BoardHtml("## 第" + std::to_string(round) + "回合"))
    {}

    virtual void OnStageBegin() override
    {
        Boardcast() << "本回合卡片如下，请公屏或私信裁判设置坐标：";
        SendInfo(BoardcastMsgSender());
        StartTimer(option().GET_VALUE(局时));
    }

  private:
    CheckoutErrCode OnTimeout()
    {
        for (PlayerID pid = 0; pid < option().PlayerNum(); ++pid) {
            if (!IsReady(pid)) {
                Hook(pid);
            }
        }
        return StageErrCode::CHECKOUT;
    }

    std::optional<std::pair<uint32_t, uint32_t>> ToCoor(MsgSenderBase& reply, const std::string& coor_str)
    {
        if (coor_str.size() != 2) {
            reply() << "[错误] 非法的坐标长度 " << coor_str.size() << " ，应为 2";
            return std::nullopt;
        }
        const char row_c = coor_str[0];
        const char col_c = coor_str[1];
        std::pair<uint32_t, uint32_t> coor;
        if ('A' <= row_c && row_c < 'A' + alchemist::Board::k_size) {
            coor.first = row_c - 'A';
        } else if ('a' <= row_c && row_c < 'a' + alchemist::Board::k_size) {
            coor.first = row_c - 'a';
        } else {
            reply() << "[错误] 非法的横坐标「" << row_c << "」，应在 A 和 "
                    << static_cast<char>('A' + alchemist::Board::k_size - 1) << " 之间";
            return std::nullopt;
        }
        if ('1' <= col_c && col_c < '1' + alchemist::Board::k_size) {
            coor.second = col_c - '1';
        } else {
            reply() << "[错误] 非法的纵坐标「" << col_c << "」，应在 1 和 "
                    << static_cast<char>('1' + alchemist::Board::k_size - 1) << " 之间";
            return std::nullopt;
        }
        return coor;
    }

    AtomReqErrCode Set_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::string& coor_str)
    {
        auto& player = main_stage().players_[pid];
        if (IsReady(pid)) {
            reply() << "您已经设置过，无法重复设置";
            return StageErrCode::FAILED;
        }
        const auto coor = ToCoor(reply, coor_str);
        if (!coor.has_value()) {
            return StageErrCode::FAILED;
        }
        if (card_.has_value()) {
            const auto ret = player.board_->SetOrClearLine(coor->first, coor->second, *card_);
            if (ret == alchemist::FAIL_ALREADY_SET) {
                reply() << "[错误] 该位置已被占用，试试其它位置吧";
                return StageErrCode::FAILED;
            } else if (ret == alchemist::FAIL_NON_ADJ_CARDS) {
                reply() << "[错误] 该位置旁边没有符文或石头，不允许空放，试试其它位置吧";
                return StageErrCode::FAILED;
            } else if (ret == alchemist::FAIL_ADJ_CARDS_MISMATCH) {
                reply() << "[错误] 该位置相邻符文非法，须满足颜色和点数至少一种相同，试试其它位置吧";
                return StageErrCode::FAILED;
            } else if (ret == 0) {
                reply() << "设置成功！";
                return StageErrCode::READY;
            } else {
                reply() << "设置成功！本次操作收获 " << ret << " 点积分";
                player.score_ += ret;
                return StageErrCode::READY;
            }
        } else {
            if (player.board_->Unset(coor->first, coor->second)) {
                reply() << "清除成功！";
                return StageErrCode::READY;
            } else {
                reply() << "[错误] 清除失败，该位置为空，试试其它位置吧";
                return StageErrCode::FAILED;
            }
        }
    }

    AtomReqErrCode Pass_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << "您选择跳过该回合";
        return StageErrCode::READY;
    }

    AtomReqErrCode Info_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        SendInfo(reply);
        return StageErrCode::OK;
    }

    void SendInfo(MsgSenderBase& sender)
    {
        sender() << Markdown(board_html_);
        sender() << Image(std::string(option().ResourceDir() + (card_.has_value() ? card_->ImageName() : "erase")) + ".png");
    }

    const std::optional<alchemist::Card> card_;
    const std::string board_html_;
};

MainStage::VariantSubStage MainStage::NewStage_()
{
    const auto round = round_;
    const auto& card = cards_[round];
    return std::make_unique<RoundStage>(*this, ++round_, card);
}

MainStage::VariantSubStage MainStage::OnStageBegin()
{
    return NewStage_();
}

MainStage::VariantSubStage MainStage::NextSubStage(RoundStage& sub_stage, const CheckoutReason reason)
{
    if (round_ == option().GET_VALUE(回合数)) {
        Boardcast() << "游戏达到最大回合数，游戏结束";
        Boardcast() << Markdown(BoardHtml("## 终局"));
        return {};
    }
    if (std::any_of(players_.begin(), players_.end(),
                [&](const Player& player) { return player.score_ >= option().GET_VALUE(胜利分数); })) {
        Boardcast() << "有玩家达到胜利分数，游戏结束";
        Boardcast() << Markdown(BoardHtml("## 终局"));
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
