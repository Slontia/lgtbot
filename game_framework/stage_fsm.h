// Copyright (c) 2024-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include "game_framework/stage_utility.h"

#ifndef GAME_MODULE_NAME
#error GAME_MODULE_NAME is not defined
#endif

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

enum class CheckoutReason { BY_REQUEST, BY_TIMEOUT, BY_LEAVE, SKIP };

using AtomReqErrCode = StageErrCode::SubSet<StageErrCode::OK,        // act successfully
                                            StageErrCode::FAILED,    // act failed
                                            StageErrCode::READY,     // act successfully and ready to checkout
                                            StageErrCode::CHECKOUT>; // to checkout

using CompReqErrCode = StageErrCode::SubSet<StageErrCode::OK, StageErrCode::FAILED>;

using CheckoutErrCode = StageErrCode::SubSet<StageErrCode::CONTINUE, StageErrCode::CHECKOUT>;

namespace internal {

template <typename RetType>
using GameCommand = Command<RetType(const uint64_t pid, const bool is_public, MsgSenderBase& reply)>;

template <typename ...Fsms>
    requires (sizeof...(Fsms) > 0)
using VariantStageFsm = std::variant<std::nullopt_t, Fsms...>;

struct StageFsmBase
{
    virtual ~StageFsmBase() = default;

    virtual const std::string& Name() const = 0;

    virtual StageUtility& Global() = 0;
    virtual const StageUtility& Global() const = 0;
};

template <typename ...Fsms>
    requires (sizeof...(Fsms) > 0)
class StageFsmSetter
{
  public:
    StageFsmSetter(VariantStageFsm<Fsms...>& variant_stage_fsm) : variant_stage_fsm_{variant_stage_fsm} {}

    StageFsmSetter(const StageFsmSetter&) = delete;
    StageFsmSetter(StageFsmSetter&&) = delete;

    ~StageFsmSetter()
    {
        if (!emplaced_) {
            variant_stage_fsm_.template emplace<std::nullopt_t>(std::nullopt);
        }
    }

    template <typename Sub, typename ...Args>
        requires (std::is_same_v<Sub, Fsms> || ...)
    void Emplace(Args&& ...args)
    {
        assert(!emplaced_);
        variant_stage_fsm_.template emplace<Sub>(std::forward<Args>(args)...);
        emplaced_ = true;
    }

  private:
    bool emplaced_{false};
    VariantStageFsm<Fsms...>& variant_stage_fsm_;
};

// The finite-state machine corresponding to `AtomicStage`.
struct AtomicStageFsm : virtual public StageFsmBase
{
    using SubStageFsmSetter = void; // AtomicStageFsm need no setters

    // This function is invoked when the atomic stage begins.
    // We can start the timer by overriding this function.
    virtual void OnStageBegin() {}

    // This function is invoked when enough players completed their actions. However, some inactive players (such as
    // left or eliminated players) may still in unready state.
    // We can override this function to determine whether checkout this stage or not.
    //
    // Return value:
    // - CHECKOUT: Checkout this atomic stage. The stage will be over.
    // - CONTINUE: Continue this atomic stage. This function can be invoked again after returning if no players' ready
    //   flag are cleared.
    virtual CheckoutErrCode OnStageOver() { return StageErrCode::CHECKOUT; }

    // This function is invoked when timeout.
    // We can override this function to determine whether checkout this stage or not.
    //
    // Return value:
    // - CHECKOUT: Checkout this atomic stage. The stage will be over.
    // - CONTINUE: Continue this atomic stage. The overrided function can restart the timer before returning, and this
    //   function can be invoked again when timeout again.
    virtual CheckoutErrCode OnStageTimeout() { return StageErrCode::CHECKOUT; }

    // This function is invoked when a player leaves.
    // We can override this function to determine whether checkout this stage or not.
    //
    // Return value:
    // - CHECKOUT: Checkout this atomic stage. The stage will be over.
    // - CONTINUE: Continue this atomic stage.
    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) { return StageErrCode::CONTINUE; }

    // This function is invoked when a computer attempts to take actions.
    // We can override this function to determine whether each computer has completed their actions.
    //
    // The Return value can be OK / READY / FAILED / CHECKOUT:
    // - Players controlled by users can not take actions until this funtion returns OK or READY for each players
    //   controlled by computers.
    // - The stage will be checked out immediately if CHECKOUT is returned.
    // - The return value of READY indicates the corresponding player has completed his action. The stage will not be
    //   checked out until all players completed their actions. However, this function can be invoked again during the
    //   same stage for the same player, even if this function has returned READY in the previous invocation. To avoid
    //   repeated action, it can be necessary to check whether the player has completed its action by `Global().IsReady(pid)`.
    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) { return StageErrCode::READY; }

    virtual const std::vector<GameCommand<AtomReqErrCode>>& Commands() const = 0;
};

template <typename ...Subs>
    requires (sizeof...(Subs) > 0)
struct SubStageInitFsm
{
    virtual ~SubStageInitFsm() = default;

    // This function is invoked when the current compound stage starts. The overrided function can create a substage by
    // calling `setter.Emplace<NewStageFsm>(...)` (`NewStageFsm` is a developer-defined class). If `setter.Emplace` is
    // not/ invoked, the current compound stage will be over.
    virtual void FirstStageFsm(StageFsmSetter<Subs...> setter) = 0;
};

template <typename CurrentSub, typename ...Subs>
    requires (sizeof...(Subs) > 0)
struct SubStageCheckoutFsm
{
    virtual ~SubStageCheckoutFsm() = default;

    // This function is invoked when the substage is over. The overrided function can create a new substage by calling
    // `setter.Emplace<NewStageFsm>(...)` (`NewStageFsm` is a developer-defined class). If `setter.Emplace` is not
    // invoked, the current compound stage will be over.
    // Note that once `setter.Emplace` is invoked, `current_sub_stage` will be destructed. To prevent the issue of
    // use-after-free, we should no longer visit it.
    virtual void NextStageFsm(CurrentSub& current_sub_stage, const CheckoutReason reason, StageFsmSetter<Subs...> setter) = 0;
};

struct CompoundStageFsmBase : virtual public StageFsmBase
{
    // This function is invoked when a player leaves.
    // No matter what this function does, the `OnPlayerLeave` of its substage's FSM will always be invoked.
    virtual void OnPlayerLeave(const PlayerID pid) {}

    // This function is invoked when a computer attempts to take actions.
    // We can override this function to determine whether each computer has completed their actions.
    //
    // The return value can be OK / FAILED:
    // Only when `CompoundStageFsmBase::OnComputerAct` returns OK can the stage proceed to invoke `OnComputerAct` of its
    // sub stage's FSM. Players controlled by users can not take actions until `AtomicStageFsm::OnComputerAct` returns
    // OK or READY for each players controlled by computers.
    virtual CompReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) { return StageErrCode::OK; }

    virtual const std::vector<GameCommand<CompReqErrCode>>& Commands() const = 0;
};

// The finite-state machine corresponding to `CompoundStage`.
template <typename ...Subs>
    requires (sizeof...(Subs) > 0)
struct CompoundStageFsm
    : public CompoundStageFsmBase
    , public SubStageInitFsm<Subs...>
    , public SubStageCheckoutFsm<Subs, Subs...>...
{
    using SubStageCheckoutFsm<Subs, Subs...>::NextStageFsm...;

    using SubStageFsmSetter = StageFsmSetter<Subs...>;
};

class MainStageFsm : virtual public StageFsmBase
{
  public:
    MainStageFsm(StageUtility utility) : utility_{std::move(utility)} {}

    virtual ~MainStageFsm() = default;

    virtual int64_t PlayerScore(const PlayerID pid) const = 0;

    StageUtility& Global() final { return utility_; }
    const StageUtility& Global() const final { return utility_; }

  private:
    StageUtility utility_;
};

template <typename MainFsm>
struct SubStageFsm : virtual public StageFsmBase
{
  public:
    SubStageFsm(MainStageFsm& main_stage_fsm) : main_stage_fsm_{main_stage_fsm} {}

    StageUtility& Global() final { return main_stage_fsm_.Global(); }
    const StageUtility& Global() const final { return main_stage_fsm_.Global(); }

    MainFsm& Main() { return static_cast<MainFsm&>(main_stage_fsm_); }
    const MainFsm& Main() const { return static_cast<const MainFsm&>(main_stage_fsm_); }

  private:
    MainStageFsm& main_stage_fsm_;
};

template <typename ...Subs>
struct StageFsmTypeBaseType { using Type = CompoundStageFsm<Subs...>; };

template <>
struct StageFsmTypeBaseType<> { using Type = AtomicStageFsm; };

template <typename Main>
struct StageFsmLevelBaseType { using Type = SubStageFsm<Main>; };

template <>
struct StageFsmLevelBaseType<void> { using Type = MainStageFsm; };

template <typename Main>
struct StageFsmConstructArgType { using Type = Main&; };

template <>
struct StageFsmConstructArgType<void> { using Type = StageUtility&&; };

} // namespace internal

template <typename Main, typename ...Subs>
class StageFsm : public internal::StageFsmTypeBaseType<Subs...>::Type
               , public internal::StageFsmLevelBaseType<Main>::Type
{
    using TypeBase = internal::StageFsmTypeBaseType<Subs...>::Type;
    using LevelBase = typename internal::StageFsmLevelBaseType<Main>::Type;
    using Command = internal::GameCommand<std::conditional_t<sizeof...(Subs) == 0, AtomReqErrCode, CompReqErrCode>>;

  public:
    using LevelBase::Global;

    template <typename ...Commands> requires ((std::is_same_v<Command, std::decay_t<Commands>> && ...))
    StageFsm(internal::StageFsmConstructArgType<Main>::Type arg, std::string name, Commands&& ...commands)
        : LevelBase(std::forward<typename internal::StageFsmConstructArgType<Main>::Type>(arg))
        , name_(name)
        , commands_{std::forward<Commands>(commands)...}
    {
    }

    template <typename ...Commands> requires ((std::is_same_v<Command, std::decay_t<Commands>> && ...))
    StageFsm(internal::StageFsmConstructArgType<Main>::Type arg, Commands&& ...commands)
        : LevelBase(std::forward<typename internal::StageFsmConstructArgType<Main>::Type>(arg))
        , name_(std::is_void_v<Main> ? "主阶段" : "匿名子阶段")
        , commands_{std::forward<Commands>(commands)...}
    {
    }

    const std::string& Name() const final { return name_; }

    const std::vector<Command>& Commands() const final { return commands_; }

  protected:
    using TypeBase::SubStageFsmSetter;

  private:
    std::string name_;
    std::vector<Command> commands_;
};

enum CommandFlag : uint8_t
{
    PRIVATE_ONLY = 0x01,
    UNREADY_ONLY = 0x02,
};

template <typename Fsm, typename RetType, typename... Args, typename... Checkers>
internal::GameCommand<RetType> MakeStageCommand(Fsm& fsm, const char* const description, RetType (Fsm::*cb)(Args...),
        Checkers&&... checkers)
{
    return MakeStageCommand(fsm, description, 0, cb, std::forward<Checkers>(checkers)...);
}

template <typename Fsm, typename RetType, typename... Args, typename... Checkers>
internal::GameCommand<RetType> MakeStageCommand(Fsm& fsm, const char* const description, const uint8_t flags, RetType (Fsm::*cb)(Args...),
        Checkers&&... checkers)
{
    auto callback = [flags, cb, &fsm]<typename ...CbArgs>(const uint64_t pid, const bool is_public, MsgSenderBase& reply, CbArgs&& ...args) -> RetType
    {
        if ((flags & CommandFlag::PRIVATE_ONLY) && is_public) {
            reply() << "[错误] 请私信执行该指令";
            return StageErrCode::FAILED;
        }
        if ((flags & CommandFlag::UNREADY_ONLY) && fsm.Global().IsReady(pid)) {
            reply() << "[错误] 您已完成行动";
            return StageErrCode::FAILED;
        }
        return std::invoke(cb, &fsm, pid, is_public, reply, std::forward<CbArgs>(args)...);
    };
    return internal::GameCommand<RetType>(description, std::move(callback), std::forward<Checkers>(checkers)...);
}

#define GAME_OPTION(name) GET_OPTION_VALUE(this->Global().Options(), name)

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot
