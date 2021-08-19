#pragma once
#include <cassert>
#include <chrono>
#include <optional>
#include <variant>

#include "game_framework/util.h"

#include "bot_core/match_base.h"
#include "utility/msg_checker.h"

template <typename RetType>
using GameCommand = Command<RetType(const uint64_t, const bool, MsgSenderBase&)>;

enum class CheckoutReason { BY_REQUEST, BY_TIMEOUT, BY_LEAVE };

template <typename SubStage, typename RetType>
class SubStageCheckoutHelper
{
  public:
    virtual RetType NextSubStage(SubStage& sub_stage, const CheckoutReason reason) = 0;
};

template <bool IsMain, typename... SubStages>
class GameStage;

class StageBaseImpl : virtual public StageBase
{
  public:
    StageBaseImpl(std::string&& name)
            : name_(name.empty() ? "（匿名阶段）" : std::move(name)), match_(nullptr)
    {
    }
    virtual ~StageBaseImpl() {}
    virtual void Init(MatchBase* const match)
    {
        match_ = match;
    }
    virtual StageErrCode HandleTimeout() = 0;
    virtual uint64_t CommandInfo(uint64_t i, MsgSenderBase::MsgSenderGuard& sender) const = 0;
    virtual StageErrCode HandleRequest(const char* const msg, const uint64_t player_id, const bool is_public,
                                       MsgSenderBase& reply)
    {
        MsgReader reader(msg);
        return HandleRequest(reader, player_id, is_public, reply);
    }
    virtual StageErrCode HandleRequest(MsgReader& reader, const uint64_t player_id, const bool is_public,
                                       MsgSenderBase& reply) = 0;
    virtual StageErrCode HandleLeave(const PlayerID pid) = 0;
    virtual StageErrCode HandleComputerAct(const uint64_t begin_pid, const uint64_t end_pid) = 0;
    virtual void StageInfo(MsgSenderBase::MsgSenderGuard& sender) const = 0;
    decltype(auto) BoardcastMsgSender() const { return match_->BoardcastMsgSender(); }
    decltype(auto) TellMsgSender(const PlayerID pid) const { return match_->TellMsgSender(pid); }
    decltype(auto) Boardcast() const { return BoardcastMsgSender()(); }
    decltype(auto) Tell(const PlayerID pid) const { return TellMsgSender(pid)(); }

  protected:
    template <typename Stage, typename RetType, typename... Args, typename... Checkers>
    GameCommand<RetType> MakeStageCommand(const char* const description,
            RetType (Stage::*cb)(Args...), Checkers&&... checkers)
    {
        return GameCommand<RetType>(description, std::bind_front(cb, static_cast<Stage*>(this)), std::forward<Checkers>(checkers)...);
    }

    template <typename Command>
    static uint64_t CommandInfo(uint64_t i, MsgSenderBase::MsgSenderGuard& sender, const Command& cmd)
    {
        sender << "\n[" << (++i) << "] " << cmd.Info();
        return i;
    }

    const std::string name_;
    MatchBase* match_;
};

class MainStageBaseImpl : public MainStageBase, public StageBaseImpl
{
   public:
    MainStageBaseImpl(std::string&& name) : StageBaseImpl(name.empty() ? "（主阶段）" : std::move(name)) {}
    virtual int64_t PlayerScore(const PlayerID pid) const = 0;
};

template <typename... SubStages>
using SubGameStage = GameStage<false, SubStages...>;
template <typename... SubStages>
using MainGameStage = GameStage<true, SubStages...>;

template <bool IsMain, typename SubStage, typename... SubStages>
class GameStage<IsMain, SubStage, SubStages...>
        : public std::conditional_t<IsMain, MainStageBaseImpl, StageBaseImpl>,
          public SubStageCheckoutHelper<SubStage,
                                        std::variant<std::unique_ptr<SubStage>, std::unique_ptr<SubStages>...>>,
          public SubStageCheckoutHelper<SubStages,
                                        std::variant<std::unique_ptr<SubStage>, std::unique_ptr<SubStages>...>>...
{
   private:
    using Base = std::conditional_t<IsMain, MainStageBaseImpl, StageBaseImpl>;

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

    virtual void Init(MatchBase* const match)
    {
        Base::Init(match);
        sub_stage_ = OnStageBegin();
        std::visit([match](auto&& sub_stage) { sub_stage->Init(match); }, sub_stage_);
    }

    virtual StageBase::StageErrCode HandleRequest(MsgReader& reader, const uint64_t player_id, const bool is_public,
                                                  MsgSenderBase& reply) override
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

    virtual StageBase::StageErrCode HandleLeave(const PlayerID pid) override
    {
        // We must call CompStage's OnPlayerLeave first so that it can deceide whether to finish game when substage is over.
        this->OnPlayerLeave(pid);
        return PassToSubStage_(
                [pid](auto&& sub_stage) { return sub_stage->HandleLeave(pid); },
                CheckoutReason::BY_LEAVE);
    }

    virtual StageBase::StageErrCode HandleComputerAct(const uint64_t begin_pid, const uint64_t end_pid) override
    {
        for (PlayerID pid = begin_pid; pid < end_pid; ++pid) {
            const auto rc = OnComputerAct(pid, Base::TellMsgSender(pid));
            if (rc != OK) {
                return ToStageErrCode(rc);
            }
        }
        return PassToSubStage_(
                [begin_pid, end_pid](auto&& sub_stage) { return sub_stage->HandleComputerAct(begin_pid, end_pid); },
                CheckoutReason::BY_REQUEST); // game logic not care abort computer
    }

    virtual uint64_t CommandInfo(uint64_t i, MsgSenderBase::MsgSenderGuard& sender) const override
    {
        for (const auto& cmd : commands_) {
            i = Base::CommandInfo(i, sender, cmd);
        }
        return std::visit([&i, &sender](auto&& sub_stage) { return sub_stage->CommandInfo(i, sender); }, sub_stage_);
    }

    virtual void StageInfo(MsgSenderBase::MsgSenderGuard& sender) const override
    {
        sender << Base::name_ << " - ";
        std::visit([&sender](auto&& sub_stage) { sub_stage->StageInfo(sender); }, sub_stage_);
    }

    void CheckoutSubStage(const CheckoutReason reason)
    {
        // ensure previous substage is released before next substage built
        sub_stage_ = std::visit(
                [this, reason](auto&& sub_stage)
                {
                    VariantSubStage new_sub_stage = NextSubStage(*sub_stage, reason);
                    sub_stage = nullptr;
                    return std::move(new_sub_stage);
                },
                sub_stage_);
        std::visit(
                [this](auto&& sub_stage)
                {
                    if (!sub_stage) {
                        Over();
                        // no more substages
                    } else {
                        sub_stage->Init(Base::match_);
                    }
                },
                sub_stage_);
    }

   private:
    using StageBase::Over;

    // CompStage cannot checkout by itself so return type is void
    virtual void OnPlayerLeave(const PlayerID pid) {}
    virtual CompStageErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) { return CompStageErrCode::OK; }

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
            return StageBase::StageErrCode::FAILED;
        }
        // no more error code
    }
    VariantSubStage sub_stage_;
    std::vector<GameCommand<CompStageErrCode>> commands_;
};

template <bool IsMain>
class GameStage<IsMain> : public std::conditional_t<IsMain, MainStageBaseImpl, StageBaseImpl>
{
   private:
    using Base = std::conditional_t<IsMain, MainStageBaseImpl, StageBaseImpl>;

   protected:
    enum AtomStageErrCode { OK, CHECKOUT, FAILED };

   public:
    template <typename ...Commands>
    GameStage(std::string&& name = "", Commands&&... commands)
            : Base(std::move(name)), commands_{std::forward<Commands>(commands)...}
    {
    }
    virtual ~GameStage() { Base::match_->StopTimer(); }
    virtual void OnStageBegin() {}
    virtual void Init(MatchBase* const match)
    {
        Base::Init(match);
        OnStageBegin();
    }

    virtual StageBase::StageErrCode HandleTimeout() override final
    {
        return Handle_([this] { return OnTimeout(); });
    }

    virtual StageBase::StageErrCode HandleLeave(const PlayerID pid) override final
    {
        return Handle_([this, pid] { return OnPlayerLeave(pid); });
    }

    virtual StageBase::StageErrCode HandleRequest(MsgReader& reader, const uint64_t player_id, const bool is_public,
                                                  MsgSenderBase& reply) override final
    {
        for (const auto& cmd : commands_) {
            if (const auto rc = cmd.CallIfValid(reader, player_id, is_public, reply); rc.has_value()) {
                return Handle_([rc] { return *rc; });
            }
        }
        return StageBase::StageErrCode::NOT_FOUND;
    }

    virtual StageBase::StageErrCode HandleComputerAct(const uint64_t begin_pid, const uint64_t end_pid) override final
    {
        return Handle_([this, begin_pid, end_pid]
                {
                    for (PlayerID pid = begin_pid; pid < end_pid; ++pid) {
                        const auto rc = OnComputerAct(pid, Base::TellMsgSender(pid));
                        if (rc != OK) {
                            return rc;
                        }
                    }
                    return OK;
                });
    }

    virtual uint64_t CommandInfo(uint64_t i, MsgSenderBase::MsgSenderGuard& sender) const override
    {
        for (const auto& cmd : commands_) {
            i = Base::CommandInfo(i, sender, cmd);
        }
        return i;
    }

    virtual void StageInfo(MsgSenderBase::MsgSenderGuard& sender) const override
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
    virtual AtomStageErrCode OnPlayerLeave(const PlayerID pid) { return AtomStageErrCode::OK; }
    virtual AtomStageErrCode OnTimeout() { return AtomStageErrCode::CHECKOUT; }
    virtual AtomStageErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) { return AtomStageErrCode::OK; }

    void StartTimer(const uint64_t sec)
    {
        finish_time_ = std::chrono::steady_clock::now() + std::chrono::seconds(sec);
        Base::match_->StartTimer(sec);
    }

    void StopTimer()
    {
        StopTimer(Base::match_);
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
            return StageBase::StageErrCode::FAILED;
        }
        // no more error code
    }
    virtual void Over() override final
    {
        StageBase::Over();
    }
    // if command return true, the stage will be over
    std::vector<GameCommand<AtomStageErrCode>> commands_;
    std::optional<std::chrono::time_point<std::chrono::steady_clock>> finish_time_;
};
