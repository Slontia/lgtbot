#include <array>
#include <functional>
#include <memory>

#include "game_framework/game_main.h"
#include "game_framework/game_stage.h"
#include "game_framework/game_options.h"
#include "utility/msg_checker.h"

const std::string k_game_name = "LIE";
const uint64_t k_max_player = 2; /* 0 means no max-player limits */

const std::string GameOption::StatusInfo() const
{
    std::stringstream ss;
    ss << "集齐全部" << GET_VALUE(数字种类) << "种数字，或持有单个数字数量达到" << GET_VALUE(失败数量) << "时玩家失败";
    return ss.str();
}

class NumberStage : public SubGameStage<>
{
   public:
    NumberStage(const PlayerID questioner, const uint32_t max_number)
            : GameStage("设置数字阶段",
                    MakeStageCommand("设置数字", &NumberStage::Number_, ArithChecker<int>(1, max_number, "数字"))),
              questioner_(questioner),
              num_(0)
    {
    }

    virtual void OnStageBegin() override { StartTimer(60); }
    int num() const { return num_; }

   private:
    AtomStageErrCode Number_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int num)
    {
        reply() << pid << " " << questioner_;
        if (pid != questioner_) {
            reply() << "[错误] 本回合您为猜测者，无法设置数字";
            return FAILED;
        }
        if (is_public) {
            reply() << "[错误] 请私信裁判选择数字，公开选择无效";
            return FAILED;
        }
        num_ = num;
        return CHECKOUT;
    }

    virtual AtomStageErrCode OnPlayerLeave(const PlayerID pid) { return CHECKOUT; }

    const PlayerID questioner_;
    int num_;
};

class LieStage : public SubGameStage<>
{
   public:
    LieStage(const uint64_t questioner, const uint32_t max_number)
            : GameStage("设置数字阶段", MakeStageCommand("提问数字", &LieStage::Lie_, ArithChecker<int>(1, max_number, "数字"))),
              questioner_(questioner),
              lie_num_(0)
    {
    }

    virtual void OnStageBegin() override { StartTimer(60); }
    int lie_num() const { return lie_num_; }

   private:
    AtomStageErrCode Lie_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int lie_num)
    {
        if (pid != questioner_) {
            reply() << "[错误] 本回合您为猜测者，无法提问";
            return FAILED;
        }
        lie_num_ = lie_num;
        return CHECKOUT;
    }

    virtual AtomStageErrCode OnPlayerLeave(const PlayerID pid) override { return CHECKOUT; }

    const uint64_t questioner_;
    int lie_num_;
};

class GuessStage : public SubGameStage<>
{
   public:
    GuessStage(const uint64_t guesser)
            : GameStage("设置数字阶段", MakeStageCommand("猜测", &GuessStage::Guess_, BoolChecker("质疑", "相信"))),
              guesser_(guesser),
              doubt_(false)
    {
    }

    virtual void OnStageBegin() override { StartTimer(60); }

    bool doubt() const { return doubt_; }

   private:
    AtomStageErrCode Guess_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const bool doubt)
    {
        if (pid != guesser_) {
            reply() << "[错误] 本回合您为提问者，无法猜测";
            return FAILED;
        }
        doubt_ = doubt;
        return CHECKOUT;
    }

    virtual AtomStageErrCode OnPlayerLeave(const PlayerID pid) override { return CHECKOUT; }

    const uint64_t guesser_;
    bool doubt_;
};

class RoundStage : public SubGameStage<NumberStage, LieStage, GuessStage>
{
   public:
    RoundStage(const GameOption& option, const uint64_t round, const uint64_t questioner, std::array<std::vector<int>, 2>& player_nums)
            : GameStage("第" + std::to_string(round) + "回合"),
              option_(option),
              questioner_(questioner),
              num_(0),
              lie_num_(0),
              player_nums_(player_nums),
              loser_(0)
    {
    }

    PlayerID loser() const { return loser_; }

    virtual VariantSubStage OnStageBegin() override
    {
        Boardcast() << name_ << "开始，请玩家" << At(questioner_) << "私信裁判选择数字";
        return std::make_unique<NumberStage>(questioner_, option_.GET_VALUE(数字种类));
    }

    virtual VariantSubStage NextSubStage(NumberStage& sub_stage, const CheckoutReason reason) override
    {
        if (reason == CheckoutReason::BY_LEAVE) {
            return {};
        }
        num_ = reason == CheckoutReason::BY_TIMEOUT ? 1 : sub_stage.num();
        Tell(questioner_) << (reason == CheckoutReason::BY_TIMEOUT ? "设置超时，数字设置为默认值1，请提问数字" : "设置成功，请提问数字");
        return std::make_unique<LieStage>(questioner_, option_.GET_VALUE(数字种类));
    }

    virtual VariantSubStage NextSubStage(LieStage& sub_stage, const CheckoutReason reason) override
    {
        if (reason == CheckoutReason::BY_LEAVE) {
            return {};
        }
        lie_num_ = reason == CheckoutReason::BY_TIMEOUT ? 1 : sub_stage.lie_num();
        if (reason == CheckoutReason::BY_TIMEOUT) {
            Tell(questioner_) << "提问超时，默认提问数字1";
        }
        Boardcast() << "玩家" << At(questioner_) << "提问数字" << lie_num_ << "，请玩家" << At(PlayerID{1 - questioner_})
                    << "相信或质疑";
        return std::make_unique<GuessStage>(1 - questioner_);
    }

    virtual VariantSubStage NextSubStage(GuessStage& sub_stage, const CheckoutReason reason) override
    {
        if (reason == CheckoutReason::BY_LEAVE) {
            return {};
        }
        const bool doubt = reason == CheckoutReason::BY_TIMEOUT ? false : sub_stage.doubt();
        if (reason == CheckoutReason::BY_TIMEOUT) {
            Tell(questioner_) << "选择超时，默认为相信";
        }
        const bool suc = doubt ^ (num_ == lie_num_);
        loser_ = suc ? questioner_ : PlayerID{1 - questioner_};
        ++player_nums_[loser_][num_ - 1];
        auto boardcast = Boardcast();
        boardcast << "实际数字为" << num_ << "，" << (doubt ? "怀疑" : "相信") << (suc ? "成功" : "失败") << "，"
                  << "玩家" << At(loser_) << "获得数字" << num_ << "\n数字获得情况：\n"
                  << At(PlayerID(0)) << "：" << At(PlayerID(1));
        for (int num = 1; num <= option_.GET_VALUE(数字种类); ++num) {
            boardcast << "\n" << player_nums_[0][num - 1] << " [" << num << "] " << player_nums_[1][num - 1];
        }
        return {};
    }

   private:
    const GameOption& option_;
    const PlayerID questioner_;
    int num_;
    int lie_num_;
    std::array<std::vector<int>, 2>& player_nums_;
    PlayerID loser_;
};

class MainStage : public MainGameStage<RoundStage>
{
   public:
    MainStage(const GameOption& option)
            : option_(option), questioner_(0), round_(1),
              player_nums_{std::vector<int>(option.GET_VALUE(数字种类), 0), std::vector<int>(option.GET_VALUE(数字种类), 0)}
    {
        std::srand(std::time(0));
    }

    virtual VariantSubStage OnStageBegin() override
    {
        return std::make_unique<RoundStage>(option_, 1, std::rand() % 2, player_nums_);
    }

    virtual VariantSubStage NextSubStage(RoundStage& sub_stage, const CheckoutReason reason) override
    {
        questioner_ = sub_stage.loser();
        if (JudgeOver()) {
            return {};
        }
        return std::make_unique<RoundStage>(option_, ++round_, questioner_, player_nums_);
    }

    int64_t PlayerScore(const PlayerID pid) const
    {
        const PlayerID loser_pid = leaver_.has_value() ? *leaver_ : questioner_;
        return pid == loser_pid ? 0 : 1; }

   private:
    bool JudgeOver()
    {
        if (leaver_.has_value()) {
            return true;
        }
        bool has_all_num = true;
        for (const int count : player_nums_[questioner_]) {
            if (count >= option_.GET_VALUE(失败数量)) {
                return true;
            } else if (count == 0) {
                has_all_num = false;
            }
        }
        return has_all_num;
    }

    virtual void OnPlayerLeave(const PlayerID pid) override { leaver_ = pid; }

    const GameOption& option_;
    PlayerID questioner_;
    uint64_t round_;
    std::array<std::vector<int>, 2> player_nums_;
    std::optional<PlayerID> leaver_;
};

MainStageBase* MakeMainStage(MsgSenderBase& reply, const GameOption& options)
{
    if (options.PlayerNum() != 2) {
        reply() << "该游戏为双人游戏，必须为2人参加，当前玩家数为" << options.PlayerNum();
        return nullptr;
    }
    return new MainStage(options);
}
