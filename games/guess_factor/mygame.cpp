#include <array>
#include <functional>
#include <memory>
#include <numeric>

#include "game_framework/game_main.h"
#include "game_framework/game_stage.h"
#include "utility/msg_checker.h"

const std::string k_game_name = "因数游戏";
const uint64_t k_max_player = 0;

const std::string GameOption::StatusInfo() const
{
    std::stringstream ss;
    ss << "每回合" << GET_VALUE(局时)
       << "秒。"
          "从第"
       << GET_VALUE(淘汰回合)
       << "回合开始，"
          "每隔"
       << GET_VALUE(淘汰间隔)
       << "回合进行一次淘汰判定："
          "若最高分玩家达到"
       << GET_VALUE(淘汰分数)
       << "分或以上，"
          "且分数最末尾玩家与次末尾玩家相差达到"
       << GET_VALUE(淘汰分差)
       << "分或以上时，最末尾玩家被淘汰。"
          "当第"
       << GET_VALUE(最大回合)
       << "回合结束，或仅剩一名玩家存活时，游戏结束。"
          "玩家可预测因数范围为1~"
       << GET_VALUE(最大数字) << "。";
    return ss.str();
}

class Player
{
   public:
    Player(const uint64_t pid) : pid_(pid), score_(0), state_(ONLINE) {}

    friend auto operator<=>(const Player& _1, const Player& _2) { return _1.score_ <=> _2.score_; }

    uint64_t FactorSum() const { return std::accumulate(factor_pool_.begin(), factor_pool_.end(), 0); }

    template <typename Sender>
    void Info(Sender& sender, const bool show_current = false) const
    {
        sender << AtMsg(pid_) << "（" << score_ << "分";
        if (state_ == ELIMINATED) {
            sender << "，已淘汰）";
            return;
        }
        sender << "）：";
        for (const uint32_t factor : factor_pool_) {
            sender << factor << " ";
        }
        if (show_current) {
            if (!current_factor_.has_value() || *current_factor_ == 0) {
                sender << "[未猜测]\n";
            } else {
                sender << "[" << *current_factor_ << "]\n";
            }
        }
    }

    template <typename Sender>
    void AddScore(Sender&& sender, const uint64_t sum)
    {
        sender << AtMsg(pid_) << "：" << score_;
        if (state_ == ELIMINATED) {
            sender << "分，已淘汰";
            return;
        }
        if (current_factor_.has_value() && current_factor_ != 0) {
            factor_pool_.emplace_back(*current_factor_);
        }
        ResetCurrentFactor();
        for (auto it = factor_pool_.begin(); it != factor_pool_.end();) {
            if (sum % *it == 0) {
                sender << " + " << *it;
                score_ += *it;
                it = factor_pool_.erase(it);
            } else {
                ++it;
            }
        }
        sender << " = " << score_ << "分";
    }

    void ResetCurrentFactor()
    {
        if (state_ == LEAVED) {
            current_factor_ = 0;
        } else {
            current_factor_.reset();
        }
    }

    void Eliminate()
    {
        state_ = ELIMINATED;
        factor_pool_.clear();
        current_factor_.reset();
    }

    void Leave()
    {
        if (state_ == ONLINE) {
          state_ = LEAVED;
        }
        if (!current_factor_.has_value()) {
            current_factor_ = 0; // pass if has not choose
        }
    }

    template <typename Sender>
    bool Guess(Sender&& sender, const uint32_t factor)
    {
        if (state_ == ELIMINATED) {
            sender << "不好意思，您已经被淘汰，无法选择数字";
            return false;
        }
        if (const std::optional<uint32_t> old_factor = std::exchange(current_factor_, factor);
            !old_factor.has_value() || *old_factor == 0) {
            sender << "猜测成功：您猜测的数字为" << factor;
        } else {
            sender << "修改猜测成功，旧猜测数字为" << *old_factor << "，当前猜测数字为" << factor;
        }
        return true;
    }

    template <typename Sender>
    bool Pass(Sender&& sender)
    {
        if (state_ == ELIMINATED) {
            sender << "不好意思，您已经被淘汰";
            return false;
        }
        if (const std::optional<uint32_t> old_factor = std::exchange(current_factor_, 0);
            !old_factor.has_value() || *old_factor == 0) {
            sender << "您选择了不猜测";
        } else {
            sender << "您将选择改为了不猜测，旧猜测数字为" << *old_factor;
        }
        return true;
    }

    uint64_t pid() const { return pid_; }
    uint64_t score() const { return score_; }
    bool eliminated() const { return state_ == ELIMINATED; }
    bool online() const { return state_ == ONLINE; }
    const auto& factor_pool() const { return factor_pool_; }
    const std::optional<uint32_t>& current_factor() const { return current_factor_; }

   private:
    const uint64_t pid_;
    uint64_t score_;
    enum { LEAVED, ELIMINATED, ONLINE } state_;
    std::list<uint32_t> factor_pool_;
    std::optional<uint32_t> current_factor_;
};

class RoundStage : public SubGameStage<>
{
   public:
    RoundStage(const GameOption& option, const uint64_t round, std::vector<Player>& players, const bool eliminate_round)
        : GameStage("第" + std::to_string(round) + "回合",
                  MakeStageCommand("预测因数", &RoundStage::Guess_,
                      ArithChecker<uint32_t>(1, option.GET_VALUE(最大数字), "选择")),
                  MakeStageCommand("不猜测", &RoundStage::Pass_, VoidChecker("pass"))
              ),
              option_(option),
              eliminate_round_(eliminate_round),
              players_(players)
    {
    }

    virtual void OnStageBegin() override
    {
        auto sender = Boardcast();
        sender << name_ << "开始，请私信裁判猜测因数";
        if (eliminate_round_) {
            sender << "（本回合可能会淘汰分数末尾玩家）";
        }
        StartTimer(option_.GET_VALUE(局时));
    }

   private:
    AtomStageErrCode Guess_(const uint64_t pid, const bool is_public, const replier_t& reply, const uint32_t factor)
    {
        if (is_public) {
            reply() << "猜测失败：请私信裁判猜测，不要暴露自己的数字哦~";
            return FAILED;
        }
        assert(factor != 0 && factor <= option_.GET_VALUE(最大数字));
        for (const auto& p : players_) {
            if (std::any_of(p.factor_pool().begin(), p.factor_pool().end(),
                            [factor](const uint64_t& f) { return f == factor; })) {
                reply() << "猜测失败：因数" << factor << "已经在玩家" << PlayerMsg(p.pid()) << "的因数池中，无法选择该因数";
                return FAILED;
            }
        }
        if (!players_[pid].Guess(reply(), factor)) {
            return FAILED;
        }
        return CanOver_() ? CHECKOUT : OK;
    }

    AtomStageErrCode Pass_(const uint64_t pid, const bool is_public, const replier_t& reply)
    {
        if (is_public) {
            reply() << "pass失败：请私信裁判进行pass操作~";
            return FAILED;
        }
        if (!players_[pid].Pass(reply())) {
            return FAILED;
        }
        return CanOver_() ? CHECKOUT : OK;
    }

    virtual AtomStageErrCode OnPlayerLeave(const uint64_t pid) override
    {
        players_[pid].Leave();
        return CanOver_() ? CHECKOUT : OK;
    }

    bool CanOver_() const
    {
        return std::all_of(players_.begin(), players_.end(),
                           [](const Player& p) { return p.eliminated() || p.current_factor().has_value(); });
    }

    const GameOption& option_;
    const bool eliminate_round_;
    std::vector<Player>& players_;
};

class MainStage : public MainGameStage<RoundStage>
{
   public:
    MainStage(const GameOption& option) : option_(option), round_(1)
    {
        for (uint64_t pid = 0; pid < option_.PlayerNum(); ++pid) {
            players_.emplace_back(pid);
        }
    }

    virtual VariantSubStage OnStageBegin() override
    {
        return std::make_unique<RoundStage>(option_, 1, players_, MayEliminated_());
    }

    virtual VariantSubStage NextSubStage(RoundStage& sub_stage, const CheckoutReason reason) override
    {
        while (true) {
            if (const auto may_eliminated = NextSubStageMayEliminated_(); may_eliminated.has_value()) {
                if (HasOnline_()) {
                    return std::make_unique<RoundStage>(option_, round_, players_, *may_eliminated);
                } else {
                    Boardcast() << "所有玩家都已经退出游戏，游戏自动进行";
                    continue;
                }
            } else {
                return {};
            }
        }
    }

    int64_t PlayerScore(const uint64_t pid) const { return players_[pid].score(); }

   private:
    virtual std::optional<bool> NextSubStageMayEliminated_()
    {
        auto sender = Boardcast();
        const uint64_t sum = SumPool_(sender);
        AddScore_(sender, sum);
        if (MayEliminated_() && (Eliminate_(sender), SurviveCount_() == 1 || round_ == option_.GET_VALUE(最大回合))) {
            return std::nullopt;
        }
        ShowScore_(sender);
        ++round_;
        return MayEliminated_();
    }

    uint64_t SumPool_(MsgSenderWrapper<MsgSenderForGame>& sender)
    {
        uint64_t sum = 0;
        uint64_t addition = 0;
        sender << "回合结束，各玩家预测池情况（中括号内为本回合预测）：\n";
        for (const auto& player : players_) {
            player.Info(sender, true);
            sum += player.FactorSum();
            if (const auto& current_factor = player.current_factor(); current_factor.has_value()) {
                addition += *current_factor;
            }
        }
        sender << "总和为：" << sum << " => " << (sum + addition);
        return sum + addition;
    }

    void AddScore_(MsgSenderWrapper<MsgSenderForGame>& sender, const uint64_t sum)
    {
        [[unlikely]] if (sum == 0) {
            sender << "\n\n无人猜测";
            for (auto& player : players_) {
                player.ResetCurrentFactor();
            }
        } else {
            sender << "\n\n" << sum << "的" << option_.GET_VALUE(最大数字) << "以内因数包括：";
            for (uint32_t factor = 1; factor <= option_.GET_VALUE(最大数字); ++factor) {
                if (sum % factor == 0) {
                    sender << factor << " ";
                }
            }
            sender << "\n得分情况：";
            for (auto& player : players_) {
                sender << "\n";
                player.AddScore(sender, sum);
            }
        }
    }

    void ShowScore_(MsgSenderWrapper<MsgSenderForGame>& sender)
    {
        uint64_t sum = 0;
        sender << "\n\n当前各玩家预测池情况：\n";
        for (const auto& player : players_) {
            player.Info(sender, false);
            sum += player.FactorSum();
            sender << "\n";
        }
        sender << "总和为：" << sum;
    }

    bool MayEliminated_() const
    {
        return round_ >= option_.GET_VALUE(淘汰回合) &&
               (round_ - option_.GET_VALUE(淘汰回合)) % option_.GET_VALUE(淘汰间隔) == 0;
    }

    void Eliminate_(MsgSenderWrapper<MsgSenderForGame>& sender)
    {
        bool achieve_high_score = false;
        bool achieve_diff_score = true;
        Player* player_to_eliminate = nullptr;
        // eliminate nobody if all players' scores are same
        for (auto& player : players_) {
            if (player.eliminated()) {
                continue;
            }
            if (player.score() >= option_.GET_VALUE(淘汰分数)) {
                achieve_high_score = true;
            }
            if (!player_to_eliminate) {
                player_to_eliminate = &player;
            } else if (player_to_eliminate->score() > player.score()) {
                achieve_diff_score = player_to_eliminate->score() - player.score() >= option_.GET_VALUE(淘汰分差);
                player_to_eliminate = &player;
            } else {
                achieve_diff_score &= player.score() - player_to_eliminate->score() >= option_.GET_VALUE(淘汰分差);
            }
        }
        assert(player_to_eliminate != nullptr);
        if (!achieve_high_score) {
            sender << "\n无人淘汰：最高分玩家未达到" << option_.GET_VALUE(淘汰分数) << "分";
        } else if (!achieve_diff_score) {
            sender << "\n无人淘汰：分数最末尾玩家与次末尾玩家相差未达到" << option_.GET_VALUE(淘汰分差) << "分";
        } else {
            sender << "\n本回合淘汰：" << AtMsg(player_to_eliminate->pid());
            player_to_eliminate->Eliminate();
        }
    }

    uint64_t SurviveCount_() const
    {
        return std::count_if(players_.begin(), players_.end(), [](const Player& p) { return !p.eliminated(); });
    }

    uint64_t HasOnline_() const
    {
        return std::any_of(players_.begin(), players_.end(), [](const Player& p) { return p.online(); });
    }

    const GameOption& option_;
    uint32_t round_;
    std::vector<Player> players_;
};

std::unique_ptr<MainStageBase> MakeMainStage(const replier_t& reply, const GameOption& options)
{
    if (options.PlayerNum() < 2) {
        reply() << "该游戏至少两人参加";
        return nullptr;
    }
    if (options.GET_VALUE(最大回合) <= options.GET_VALUE(淘汰回合)) {
        reply() << "游戏最大回合数必须大于开始淘汰的回合数，当前设置的最大回合数" << options.GET_VALUE(最大回合)
                << "，当前设置的开始淘汰的回合数为" << options.GET_VALUE(淘汰回合);
        return nullptr;
    }
    return std::make_unique<MainStage>(options);
}
