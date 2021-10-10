#include <array>
#include <functional>
#include <memory>

#include "game_framework/game_main.h"
#include "game_framework/game_stage.h"
#include "game_framework/game_options.h"
#include "utility/msg_checker.h"

const std::string k_game_name = "猜拳游戏";
const uint64_t k_max_player = 2; /* 0 means no max-player limits */

const std::string GameOption::StatusInfo() const
{
    std::stringstream ss;
    const auto max_win_count = GET_VALUE(胜利局数);
    ss << 2 * max_win_count - 1 << "局" << max_win_count << "胜，局时" << GET_VALUE(局时) << "秒";
    return ss.str();
}

enum Choise { NONE_CHOISE, ROCK_CHOISE, SCISSORS_CHOISE, PAPER_CHOISE };

static std::string Choise2Str(const Choise& choise)
{
    return choise == SCISSORS_CHOISE ? "剪刀"
                                     : choise == ROCK_CHOISE ? "石头" : choise == PAPER_CHOISE ? "布" : "未选择";
}

class RoundStage : public SubGameStage<>
{
   public:
    RoundStage(MainStage& main_stage, const uint64_t round, const uint32_t max_round_sec)
            : GameStage(main_stage, "第" + std::to_string(round) + "回合",
                                MakeStageCommand("出拳", &RoundStage::Act_,
                                        AlterChecker<Choise>(std::map<std::string, Choise>{
                                            {"剪刀", SCISSORS_CHOISE}, {"石头", ROCK_CHOISE}, {"布", PAPER_CHOISE}}))
                        ),
              max_round_sec_(max_round_sec),
              cur_choise_{NONE_CHOISE, NONE_CHOISE}
    {
    }

    void OnStageBegin()
    {
        Boardcast() << name_ << "开始，请私信裁判进行选择";
        StartTimer(max_round_sec_);
    }

    std::optional<uint64_t> Winner() const
    {
        if (cur_choise_[0] == NONE_CHOISE && cur_choise_[1] == NONE_CHOISE) {
            return {};
        }
        Boardcast() << "玩家" << At(PlayerID{0}) << "：" << Choise2Str(cur_choise_[0]) << "\n玩家" << At(PlayerID{1}) << "："
                    << Choise2Str(cur_choise_[1]);
        const auto is_win = [&cur_choise = cur_choise_](const PlayerID pid) {
            const Choise& my_choise = cur_choise[pid];
            const Choise& oppo_choise = cur_choise[1 - pid];
            return (my_choise == PAPER_CHOISE && oppo_choise == ROCK_CHOISE) ||
                   (my_choise == ROCK_CHOISE && oppo_choise == SCISSORS_CHOISE) ||
                   (my_choise == SCISSORS_CHOISE && oppo_choise == PAPER_CHOISE) || oppo_choise == NONE_CHOISE;
        };
        if (is_win(0)) {
            return 0;
        } else if (is_win(1)) {
            return 1;
        }
        return {};
    }

  protected:
    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        const auto n = std::rand();
        return Act_(pid, false, reply, n % 3 == 0 ? Choise::PAPER_CHOISE :
                                       n % 3 == 1 ? Choise::ROCK_CHOISE  : Choise::SCISSORS_CHOISE);
    }

   private:
    AtomReqErrCode Act_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, Choise choise)
    {
        if (is_public) {
            reply() << "请私信裁判选择，公开选择无效";
            return StageErrCode::FAILED;
        }
        Choise& cur_choise = cur_choise_[pid];
        cur_choise = choise;
        reply() << "选择成功，您当前选择为：" << Choise2Str(cur_choise);
        return AtomReqErrCode::Condition(cur_choise_[0] != NONE_CHOISE && cur_choise_[1] != NONE_CHOISE, StageErrCode::CHECKOUT, StageErrCode::OK);
    }

    // The other player win. Game Over.
    virtual LeaveErrCode OnPlayerLeave(const PlayerID pid) { return StageErrCode::CHECKOUT; }

    const uint32_t max_round_sec_;
    std::array<Choise, 2> cur_choise_;
};

class MainStage : public MainGameStage<RoundStage>
{
   public:
    MainStage(const GameOption& option, MatchBase& match)
            : GameStage(option, match)
            , k_max_win_count_(option.GET_VALUE(胜利局数))
            , k_max_round_sec_(option.GET_VALUE(局时))
            , round_(1)
            , win_count_{0, 0}
    {
    }

    virtual VariantSubStage OnStageBegin() override { return std::make_unique<RoundStage>(*this, 1, k_max_round_sec_); }

    virtual VariantSubStage NextSubStage(RoundStage& sub_stage, const CheckoutReason reason) override
    {
        if (!winner_.has_value()) {
            std::optional<uint64_t> winner = sub_stage.Winner();
            if (reason == CheckoutReason::BY_TIMEOUT && !winner.has_value()) {
                Boardcast() << "双方无响应";
                // both score is 0, no winners
                return {};
            }
            HandleRoundResult_(winner);
            if (win_count_[0] == k_max_win_count_) {
                winner_ = 0;
            } else if (win_count_[1] == k_max_win_count_) {
                winner_ = 1;
            } else {
                return std::make_unique<RoundStage>(*this, ++round_, k_max_round_sec_);
            }
        }
        return {};
    }

    virtual void OnPlayerLeave(const PlayerID pid)
    {
        assert(!winner_.has_value());
        winner_ = 1 - pid;
    }

    int64_t PlayerScore(const PlayerID pid) const { return (winner_.has_value() && *winner_ == pid) ? 1 : 0; }

   private:
    void HandleRoundResult_(const std::optional<uint64_t>& winner)
    {
        auto boardcast = Boardcast();
        const auto on_win = [this, &boardcast, &win_count = win_count_](const PlayerID pid) {
            boardcast << "玩家" << At(pid) << "胜利\n";
            ++win_count[pid];
        };
        if (winner.has_value()) {
            on_win(*winner);
        } else {
            boardcast << "平局\n";
        }
        boardcast << "目前比分：\n";
        boardcast << At(PlayerID{0}) << " " << win_count_[0] << " - " << win_count_[1] << " " << At(PlayerID{1});
    }

    const uint32_t k_max_win_count_;
    const uint32_t k_max_round_sec_;
    uint64_t round_;
    std::array<uint64_t, 2> win_count_;
    std::optional<uint64_t> winner_;
};

MainStageBase* MakeMainStage(MsgSenderBase& reply, const GameOption& options, MatchBase& match)
{
    if (options.PlayerNum() != 2) {
        reply() << "该游戏为双人游戏，必须为2人参加，当前玩家数为" << options.PlayerNum();
        return nullptr;
    }
    return new MainStage(options, match);
}
