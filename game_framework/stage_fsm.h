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

struct AtomicStageFsm : virtual public StageFsmBase
{
    using SubStageFsmSetter = void; // AtomicStageFsm need no setters

    virtual void OnStageBegin() {}

    virtual CheckoutErrCode OnStageOver() { return StageErrCode::CHECKOUT; }

    virtual CheckoutErrCode OnStageTimeout() { return StageErrCode::CHECKOUT; }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) { return StageErrCode::CONTINUE; }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) { return StageErrCode::READY; }

    virtual const std::vector<GameCommand<AtomReqErrCode>>& Commands() const = 0;
};

template <typename CurrentSub, typename ...Subs>
    requires (sizeof...(Subs) > 0)
struct SubStageCheckoutFsm
{
    virtual ~SubStageCheckoutFsm() = default;

    virtual void NextStageFsm(CurrentSub& current_sub_stage, const CheckoutReason reason, StageFsmSetter<Subs...> setter) = 0;
};

template <typename ...Subs>
    requires (sizeof...(Subs) > 0)
struct SubStageInitFsm
{
    virtual ~SubStageInitFsm() = default;

    virtual void FirstStageFsm(StageFsmSetter<Subs...> setter) = 0;
};

struct CompoundStageFsmBase : virtual public StageFsmBase
{
    virtual void OnPlayerLeave(const PlayerID pid) {}

    virtual CompReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) { return StageErrCode::OK; }

    virtual const std::vector<GameCommand<CompReqErrCode>>& Commands() const = 0;
};

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
    MainStageFsm(const StageUtility& utility) : utility_{utility} {}

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
struct StageFsmTypeBaseType
{
    using Type = CompoundStageFsm<Subs...>;
};

template <>
struct StageFsmTypeBaseType<>
{
    using Type = AtomicStageFsm;
};

template <typename Main>
struct StageFsmLevelBaseType
{
    using Type = SubStageFsm<Main>;
};

template <>
struct StageFsmLevelBaseType<void>
{
    using Type = MainStageFsm;
};

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
    StageFsm(std::conditional_t<std::is_void_v<Main>, const StageUtility, Main>& arg, std::string name, Commands&& ...commands)
        : LevelBase(arg), name_(name), commands_{std::forward<Commands>(commands)...}
    {
    }

    template <typename ...Commands> requires ((std::is_same_v<Command, std::decay_t<Commands>> && ...))
    StageFsm(std::conditional_t<std::is_void_v<Main>, const StageUtility, Main>& arg, Commands&& ...commands)
        : LevelBase(arg), name_(std::is_void_v<Main> ? "主阶段" : "匿名子阶段"), commands_{std::forward<Commands>(commands)...}
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
