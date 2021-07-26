#pragma once
#include <cassert>
#include <chrono>
#include <optional>
#include <variant>

#include "util.h"
#include "utility/msg_checker.h"
#include "utility/timer.h"

template <typename RetType>
using GameCommand = Command<RetType(const uint64_t, const bool, const replier_t&)>;

class StageBase
{
   public:
    enum class [[nodiscard]] StageErrCode { OK, CHECKOUT, FAILED, NOT_FOUND };
    StageBase(std::string&& name)
            : name_(name.empty() ? "（匿名阶段）" : std::move(name)), match_(nullptr), is_over_(false)
    {
    }
    virtual ~StageBase() {}
    virtual void Init(void* const match, const std::function<void(const uint64_t)>& start_timer_f,
                      const std::function<void()>& stop_timer_f)
    {
        match_ = match;
        start_timer_f_ = start_timer_f;
        stop_timer_f_ = stop_timer_f;
    }
    virtual StageErrCode HandleTimeout() = 0;
    virtual uint64_t CommandInfo(uint64_t i, MsgSenderWrapper<MsgSenderForGame>& sender) const = 0;
    virtual StageErrCode HandleRequest(MsgReader& reader, const uint64_t player_id, const bool is_public,
                                       const replier_t& reply) = 0;
    virtual StageErrCode HandleLeave(const uint64_t pid) = 0;
    virtual StageErrCode HandleComputerAct(const uint64_t pid) = 0;
    virtual void StageInfo(MsgSenderWrapper<MsgSenderForGame>& sender) const = 0;
    bool IsOver() const { return is_over_; }
    auto Boardcast() const { return ::Boardcast(match_); }
    auto Tell(const uint64_t pid) const { return ::Tell(match_, pid); }

   protected:
    template <typename Stage, typename RetType, typename... Args, typename... Checkers>
    GameCommand<RetType> MakeStageCommand(const char* const description,
            RetType (Stage::*cb)(Args...), Checkers&&... checkers)
    {
        return GameCommand<RetType>(description, std::bind_front(cb, static_cast<Stage*>(this)), std::forward<Checkers>(checkers)...);
    }

    template <typename Command>
    static uint64_t CommandInfo(uint64_t i, MsgSenderWrapper<MsgSenderForGame>& sender,
                                const Command& cmd)
    {
        sender << "\n[" << (++i) << "] " << cmd.Info();
        return i;
    }
    virtual void Over() { is_over_ = true; }
    const std::string name_;
    void* match_;
    std::function<void(const uint64_t)> start_timer_f_;
    std::function<void()> stop_timer_f_;

   private:
    bool is_over_;
};

class MainStageBase : public StageBase
{
   public:
    MainStageBase(std::string&& name) : StageBase(name.empty() ? "（主阶段）" : std::move(name)) {}
    virtual int64_t PlayerScore(const uint64_t pid) const = 0;
};

enum class CheckoutReason { BY_REQUEST, BY_TIMEOUT, BY_LEAVE };

template <typename SubStage, typename RetType>
class SubStageCheckoutHelper
{
  public:
    virtual RetType NextSubStage(SubStage& sub_stage, const CheckoutReason reason) = 0;
};

template <bool IsMain, typename... SubStages>
class GameStage;

template <typename... SubStages>
using SubGameStage = GameStage<false, SubStages...>;
template <typename... SubStages>
using MainGameStage = GameStage<true, SubStages...>;

template <bool IsMain, typename SubStage, typename... SubStages>
class GameStage<IsMain, SubStage, SubStages...>
        : public std::conditional_t<IsMain, MainStageBase, StageBase>,
          public SubStageCheckoutHelper<SubStage,
                                        std::variant<std::unique_ptr<SubStage>, std::unique_ptr<SubStages>...>>,
          public SubStageCheckoutHelper<SubStages,
                                        std::variant<std::unique_ptr<SubStage>, std::unique_ptr<SubStages>...>>...
{
   private:
    using Base = std::conditional_t<IsMain, MainStageBase, StageBase>;

   protected:
    enum CompStageErrCode { OK, FAILED };

   public:
    using VariantSubStage = std::variant<std::unique_ptr<SubStage>, std::unique_ptr<SubStages>...>;
    using SubStageCheckoutHelper<SubStage, VariantSubStage>::NextSubStage;
    using SubStageCheckoutHelper<SubStages, VariantSubStage>::NextSubStage...;

    template <typename ...Commands>
    GameStage(std::string&& name = "", Commands&&... commands)
            : Base(std::move(name)), sub_stage_(), commands_{std::forward<Commands>(commands)...}
    {
    }

    virtual ~GameStage() {}

    virtual VariantSubStage OnStageBegin() = 0;

    virtual void Init(void* const match, const std::function<void(const uint64_t)>& start_timer_f,
                      const std::function<void()>& stop_timer_f)
    {
        StageBase::Init(match, start_timer_f, stop_timer_f);
        sub_stage_ = OnStageBegin();
        std::visit([match, start_timer_f,
                    stop_timer_f](auto&& sub_stage) { sub_stage->Init(match, start_timer_f, stop_timer_f); },
                   sub_stage_);
    }

    virtual StageBase::StageErrCode HandleRequest(MsgReader& reader, const uint64_t player_id, const bool is_public,
                                                  const replier_t& reply) override
    {
        for (const auto& cmd : commands_) {
            if (const auto rc = cmd.CallIfValid(reader, player_id, is_public, reply); rc.has_value()) {
                return ToStageErrCode(*rc);
            }
        }
        return PassToSubStage_(
                [&](auto&& sub_stage) { return sub_stage->HandleRequest(reader, player_id, is_public, reply); },
                CheckoutReason::BY_REQUEST);
    }

    virtual StageBase::StageErrCode HandleTimeout() override
    {
        return PassToSubStage_(
                [](auto&& sub_stage) { return sub_stage->HandleTimeout(); },
                CheckoutReason::BY_TIMEOUT);
    }

    virtual StageBase::StageErrCode HandleLeave(const uint64_t pid) override
    {
        // We must call CompStage's OnPlayerLeave first so that it can deceide whether to finish game when substage is over.
        this->OnPlayerLeave(pid);
        return PassToSubStage_(
                [pid](auto&& sub_stage) { return sub_stage->HandleLeave(pid); },
                CheckoutReason::BY_LEAVE);
    }

    virtual StageBase::StageErrCode HandleComputerAct(const uint64_t pid) override
    {
        // We must call CompStage's OnPlayerLeave first so that it can deceide whether to finish game when substage is over.
        this->OnComputerAct(pid);
        return PassToSubStage_(
                [pid](auto&& sub_stage) { return sub_stage->HandleComputerAct(pid); },
                CheckoutReason::BY_REQUEST); // game logic not care abort computer
    }

    virtual uint64_t CommandInfo(uint64_t i, MsgSenderWrapper<MsgSenderForGame>& sender) const override
    {
        for (const auto& cmd : commands_) {
            i = StageBase::CommandInfo(i, sender, cmd);
        }
        return std::visit([&i, &sender](auto&& sub_stage) { return sub_stage->CommandInfo(i, sender); }, sub_stage_);
    }

    virtual void StageInfo(MsgSenderWrapper<MsgSenderForGame>& sender) const override
    {
        sender << Base::name_ << " - ";
        std::visit([&sender](auto&& sub_stage) { sub_stage->StageInfo(sender); }, sub_stage_);
    }

    void CheckoutSubStage(const CheckoutReason reason)
    {
        // ensure previous substage is released before next substage built
        sub_stage_ = std::visit(
                [this, reason](auto&& sub_stage) {
                    VariantSubStage new_sub_stage = NextSubStage(*sub_stage, reason);
                    sub_stage = nullptr;
                    return std::move(new_sub_stage);
                },
                sub_stage_);
        std::visit(
                [this](auto&& sub_stage) {
                    if (!sub_stage) {
                        Over();
                    }  // no more substages
                    else {
                        sub_stage->Init(Base::match_, Base::start_timer_f_, Base::stop_timer_f_);
                    }
                },
                sub_stage_);
    }

   private:
    using StageBase::Over;

    // CompStage cannot checkout by itself so return type is void
    virtual void OnPlayerLeave(const uint64_t pid) {}
    virtual void OnComputerAct(const uint64_t pid) {}

    template <typename Task>
    StageBase::StageErrCode PassToSubStage_(const Task& internal_task, const CheckoutReason checkout_reason)
    {
        const auto task = [this, &internal_task, checkout_reason](auto&& sub_stage) {
            // return rc to check in unittest
            const auto rc = internal_task(sub_stage);
            if (sub_stage->IsOver()) {
                this->CheckoutSubStage(checkout_reason);
            }
            return rc;
        };
        return std::visit(task, sub_stage_);
    }

    static StageBase::StageErrCode ToStageErrCode(const CompStageErrCode rc)
    {
        switch (rc) {
        case OK:
            return StageBase::StageErrCode::OK;
        case FAILED:
            return StageBase::StageErrCode::FAILED;
        default:
            assert(false);
            return MainStageBase::StageErrCode::FAILED;
        }
        // no more error code
    }
    VariantSubStage sub_stage_;
    std::vector<GameCommand<CompStageErrCode>> commands_;
};

template <bool IsMain>
class GameStage<IsMain> : public std::conditional_t<IsMain, MainStageBase, StageBase>
{
   private:
    using Base = std::conditional_t<IsMain, MainStageBase, StageBase>;

   protected:
    enum AtomStageErrCode { OK, CHECKOUT, FAILED };

   public:
    template <typename ...Commands>
    GameStage(std::string&& name, Commands&&... commands)
            : Base(std::move(name)), timer_(nullptr), commands_{std::forward<Commands>(commands)...}
    {
    }
    virtual ~GameStage() { Base::stop_timer_f_(); }
    virtual void OnStageBegin() {}
    virtual void Init(void* const match, const std::function<void(const uint64_t)>& start_timer_f,
                      const std::function<void()>& stop_timer_f)
    {
        StageBase::Init(match, start_timer_f, stop_timer_f);
        OnStageBegin();
    }

    virtual StageBase::StageErrCode HandleTimeout() override final
    {
        return Handle_([this] { return OnTimeout(); });
    }

    virtual StageBase::StageErrCode HandleLeave(const uint64_t pid) override final
    {
        return Handle_([this, pid] { return OnPlayerLeave(pid); });
    }

    virtual StageBase::StageErrCode HandleRequest(MsgReader& reader, const uint64_t player_id, const bool is_public,
                                                  const replier_t& reply) override final
    {
        for (const auto& cmd : commands_) {
            if (const auto rc = cmd.CallIfValid(reader, player_id, is_public, reply); rc.has_value()) {
                return Handle_([rc] { return *rc; });
            }
        }
        return StageBase::StageErrCode::NOT_FOUND;
    }

    virtual StageBase::StageErrCode HandleComputerAct(const uint64_t pid) override final
    {
        return Handle_([this, pid] { return OnComputerAct(pid); });
    }

    virtual uint64_t CommandInfo(uint64_t i, MsgSenderWrapper<MsgSenderForGame>& sender) const override
    {
        for (const auto& cmd : commands_) {
            i = StageBase::CommandInfo(i, sender, cmd);
        }
        return i;
    }

    virtual void StageInfo(MsgSenderWrapper<MsgSenderForGame>& sender) const override
    {
        sender << Base::name_;
        if (finish_time_.has_value()) {
            sender << " 剩余时间："
                   << std::chrono::duration_cast<std::chrono::seconds>(*finish_time_ - std::chrono::steady_clock::now())
                              .count()
                   << "秒";
        }
    }

  protected:
    virtual AtomStageErrCode OnPlayerLeave(const uint64_t pid) { return AtomStageErrCode::OK; }
    virtual AtomStageErrCode OnTimeout() { return AtomStageErrCode::CHECKOUT; }
    virtual AtomStageErrCode OnComputerAct(const uint64_t pid) { return AtomStageErrCode::OK; }

    void StartTimer(const uint64_t sec)
    {
        finish_time_ = std::chrono::steady_clock::now() + std::chrono::seconds(sec);
        Base::start_timer_f_(sec);
    }

    void StopTimer()
    {
        Base::stop_timer_f();
        finish_time_ = nullptr;
    }

   private:
    template <typename Task>
    StageBase::StageErrCode Handle_(const Task& task)
    {
        const auto rc = task();
        if (rc == CHECKOUT) {
            Over();
        }
        return ToStageErrCode(rc);
    }

    static StageBase::StageErrCode ToStageErrCode(const AtomStageErrCode rc)
    {
        switch (rc) {
        case OK:
            return StageBase::StageErrCode::OK;
        case CHECKOUT:
            return StageBase::StageErrCode::CHECKOUT;
        case FAILED:
            return StageBase::StageErrCode::FAILED;
        default:
            assert(false);
            return MainStageBase::StageErrCode::FAILED;
        }
        // no more error code
    }
    virtual void Over() override final
    {
        timer_ = nullptr;
        StageBase::Over();
    }
    std::unique_ptr<Timer, std::function<void(Timer*)>> timer_;
    // if command return true, the stage will be over
    std::vector<GameCommand<AtomStageErrCode>> commands_;
    std::optional<std::chrono::time_point<std::chrono::steady_clock>> finish_time_;
};
