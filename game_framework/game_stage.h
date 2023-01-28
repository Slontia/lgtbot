// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <cassert>
#include <chrono>
#include <optional>
#include <variant>
#include <concepts>
#include <chrono>

#include "utility/msg_checker.h"
#include "utility/log.h"
#include "bot_core/match_base.h"

#include "game_framework/util.h"

using AtomReqErrCode = StageErrCode::SubSet<StageErrCode::OK,        // act successfully
                                              StageErrCode::FAILED,    // act failed
                                              StageErrCode::READY,     // act successfully and ready to checkout
                                              StageErrCode::CHECKOUT>; // to checkout

using CompReqErrCode = StageErrCode::SubSet<StageErrCode::OK, StageErrCode::FAILED>;

using CheckoutErrCode = StageErrCode::SubSet<StageErrCode::CONTINUE, StageErrCode::CHECKOUT>;

class Masker
{
  public:
    enum class State { SET = 'S', UNSET = 'U', PINNED = 'P' };

    Masker(const uint64_t match_id, const char* const game_name, const size_t size)
        : match_id_(match_id), game_name_(game_name), recorder_(size, State::UNSET), unset_count_(size), any_user_ready_(false) {}

    bool Set(const size_t index, const bool is_user)
    {
        Log_(DebugLog()) << "Set game stage mask is_user=" << Bool2Str(is_user) << " " << ToString_(index);
        if (is_user) {
            any_user_ready_ = true;
        }
        return Record_(index, State::SET);
    }

    void Unset(const size_t index)
    {
        // not reset any_ready to prevent players waiting a left player
        Log_(DebugLog()) << "Unset game stage mask " << ToString_(index);
        Record_(index, State::UNSET);
    }

    bool Pin(const size_t index)
    {
        Log_(DebugLog()) << "Pin game stage mask " << ToString_(index);
        any_user_ready_ = true;
        return Record_(index, State::PINNED);
    }

    bool Unpin(const size_t index)
    {
        Log_(DebugLog()) << "Unpin game stage mask " << ToString_(index);
        auto& state = recorder_[index];
        if (state == State::PINNED) {
            state = State::UNSET;
            ++unset_count_;
            return true;
        }
        return false;
    }

    State Get(const size_t index) const { return recorder_[index]; }

    void Clear()
    {
        any_user_ready_ = false;
        for (auto& state : recorder_) {
            if (state == State::SET) {
                state = State::UNSET;
                ++unset_count_;
            }
        }
        Log_(DebugLog()) << "Clear game stage mask finish " << ToString_();
    }

    bool IsReady() const { return unset_count_ == 0 && any_user_ready_; }

  private:
    std::string ToString_(const size_t index)
    {
        return "index=" + std::to_string(index) + " cur_state=" + static_cast<char>(recorder_[index]) + " " + ToString_();
    }

    std::string ToString_()
    {
        return std::string("any_user_ready=") + Bool2Str(any_user_ready_) + " unset_count=" + std::to_string(unset_count_);
    }

    template <typename Logger>
    Logger& Log_(Logger&& logger)
    {
        return logger << "[mid=" << match_id_ << "] [" << "game=" << game_name_ << "] ";
    }

    bool Record_(const size_t index, const State state)
    {
        const auto old = recorder_[index];
        if (old != State::PINNED) {
            recorder_[index] = state;
            unset_count_ += (state == State::UNSET);
            unset_count_ -= (old == State::UNSET);
        }
        Log_(DebugLog()) << "Record game stage mask finish " << ToString_(index);
        return IsReady();
    }

    const uint64_t match_id_;
    const char* const game_name_;
    std::vector<State> recorder_;
    bool any_user_ready_; // if all players are pinned, IsReady() should return false to prevent substage skipped
    size_t unset_count_;
};

template <typename RetType>
using GameCommand = Command<RetType(const uint64_t, const bool, MsgSenderBase&)>;

enum class CheckoutReason { BY_REQUEST, BY_TIMEOUT, BY_LEAVE, SKIP };

template <typename SubStage, typename RetType>
class SubStageCheckoutHelper
{
  public:
    virtual RetType NextSubStage(SubStage& sub_stage, const CheckoutReason reason) = 0;
};

struct GlobalInfo
{
    GlobalInfo(const uint64_t match_id, const char* const game_name, const size_t player_num)
        : masker_(match_id, game_name, player_num), is_in_deduction_(false) {}
    Masker masker_;
    bool is_in_deduction_;
};

template <bool IS_ATOM>
class StageBaseWrapper : virtual public StageBase
{
  public:
    template <typename String, typename ...Commands>
    StageBaseWrapper(const GameOptionBase& option, MatchBase& match, GlobalInfo& global_info, String&& name, Commands&& ...commands)
        : option_(option)
        , match_(match)
        , global_info_(global_info)
        , name_(std::forward<String>(name))
        , commands_{std::forward<Commands>(commands)...}
    {}

    virtual ~StageBaseWrapper() {}

    virtual StageErrCode HandleRequest(const char* const msg, const uint64_t pid, const bool is_public,
                                       MsgSenderBase& reply) override final
    {
        MsgReader reader(msg);
        if (masker().Unpin(pid)) {
            Tell(pid) << "挂机状态已取消";
        }
        return HandleRequest(reader, pid, is_public, reply);
    }
    virtual const char* StageInfoC() const override final
    {
        thread_local static std::string info_;
        info_ = StageInfo();
        return info_.c_str();
    }
    virtual const char* CommandInfoC(const bool text_mode) const override final
    {
        thread_local static std::string info_;
        info_ = CommandInfo(text_mode);
        return info_.c_str();
    }

    decltype(auto) BoardcastMsgSender() const { return IsInDeduction() ? EmptyMsgSender::Get() : match_.BoardcastMsgSender(); }

    decltype(auto) TellMsgSender(const PlayerID pid) const { return IsInDeduction() ? EmptyMsgSender::Get() : match_.TellMsgSender(pid); }

    decltype(auto) GroupMsgSender() const { return IsInDeduction() ? EmptyMsgSender::Get() : match_.GroupMsgSender(); }

    decltype(auto) Boardcast() const { return BoardcastMsgSender()(); }

    decltype(auto) Tell(const PlayerID pid) const { return TellMsgSender(pid)(); }

    decltype(auto) Group() const { return GroupMsgSender()(); }

    std::string PlayerName(const PlayerID pid) const { return match_.PlayerName(pid); }

    std::string PlayerAvatar(const PlayerID pid, const int32_t size) const { return match_.PlayerAvatar(pid, size); }

    void Eliminate(const PlayerID pid) const
    {
        global_info_.masker_.Pin(pid);
        match_.Eliminate(pid);
    }

    void Hook(const PlayerID pid) const
    {
        if (global_info_.masker_.Get(pid) != Masker::State::PINNED) {
            global_info_.masker_.Pin(pid);
            Tell(pid) << "您已经进入挂机状态，若其他玩家已经行动完成，裁判将不再继续等待您，执行任意游戏请求可恢复至原状态";
        }
    }

    const std::string& name() const { return name_; }

    MatchBase& match() { return match_; }

    GlobalInfo& global_info() { return global_info_; }

    Masker& masker() { return global_info_.masker_; }
    const Masker& masker() const { return global_info_.masker_; }

    void SetInDeduction() { global_info_.is_in_deduction_ = true; }
    bool IsInDeduction() const { return global_info_.is_in_deduction_; }

    virtual void HandleStageBegin() override = 0;
    virtual StageErrCode HandleTimeout() = 0;
    virtual StageErrCode HandleRequest(MsgReader& reader, const uint64_t pid, const bool is_public,
                                       MsgSenderBase& reply) = 0;
    virtual StageErrCode HandleLeave(const PlayerID pid) = 0;
    virtual StageErrCode HandleComputerAct(const uint64_t pid, const bool ready_as_user) = 0;
    virtual std::string StageInfo() const = 0;
    virtual std::string CommandInfo(const bool text_mode) const
    {
        if (commands_.empty()) {
            return "";
        }
        std::string outstr = "\n\n### 游戏命令-" + name();
        uint64_t i = 1;
        for (const auto& cmd : commands_) {
             outstr += "\n" + std::to_string(i++) + ". " + cmd.Info(true /* with_example */, !text_mode /* with_html_color */);
        }
        return outstr;
    }

  protected:
    template <typename Stage, typename RetType, typename... Args, typename... Checkers>
    GameCommand<RetType> MakeStageCommand(const char* const description, RetType (Stage::*cb)(Args...),
            Checkers&&... checkers)
    {
        return GameCommand<RetType>(description, std::bind_front(cb, static_cast<Stage*>(this)),
                std::forward<Checkers>(checkers)...);
    }

    template <typename Logger>
    Logger& StageLog(Logger&& logger) const
    {
        return logger << "[mid=" << match_.MatchId() << "] [game=" << match_.GameName() << "] [stage=" << name_ << "] ";
    }

    const std::string name_;
    const GameOptionBase& option_;
    MatchBase& match_;
    GlobalInfo& global_info_;
    std::vector<GameCommand<std::conditional_t<IS_ATOM, AtomReqErrCode, CompReqErrCode>>> commands_;
};

template <bool IS_ATOM, typename MainStage>
class SubStageBaseWrapper : public StageBaseWrapper<IS_ATOM>
{
  public:
    template <typename String, typename ...Commands>
    SubStageBaseWrapper(MainStage& main_stage, String&& name, Commands&& ...commands)
        : StageBaseWrapper<IS_ATOM>(main_stage.option(), main_stage.match(), main_stage.global_info(), std::forward<String>(name), std::forward<Commands>(commands)...)
        , main_stage_(main_stage)
    {}

    MainStage& main_stage() { return main_stage_; }
    const MainStage& main_stage() const { return main_stage_; }

  private:
    MainStage& main_stage_;
};

template <typename AchievementTuple, size_t I = std::tuple_size_v<AchievementTuple>>
class AchievementVerdictatorHelper : public AchievementVerdictatorHelper<AchievementTuple, I - 1>
{
  protected:
    using AchievementVerdictatorHelper<AchievementTuple, I - 1>::VerdictateAchievement;
    virtual bool VerdictateAchievement(const std::decay_t<decltype(std::get<I - 1>(AchievementTuple{}))>, const PlayerID pid) const = 0;
};

template <typename AchievementTuple>
class AchievementVerdictatorHelper<AchievementTuple, 0>
{
  protected:
    // define this function to avoid compilation error at using statement for non-achievement games
    void VerdictateAchievement() const {}
};

template <bool IS_ATOM, typename Achievement>
class MainStageBaseWrapper : public MainStageBase, public StageBaseWrapper<IS_ATOM>, public AchievementVerdictatorHelper<typename Achievement::Tuple::Type>
{
    using AchievementVerdictatorHelper<typename Achievement::Tuple::Type>::VerdictateAchievement;

  public:
    template <typename ...Commands>
    MainStageBaseWrapper(const GameOptionBase& option, MatchBase& match, Commands&& ...commands)
        : StageBaseWrapper<IS_ATOM>(option, match, global_info_, "主阶段", std::forward<Commands>(commands)...)
        , global_info_(match.MatchId(), match.GameName(), option.PlayerNum())
    {}

    virtual int64_t PlayerScore(const PlayerID pid) const = 0;

    virtual const char* const* VerdictateAchievements(const PlayerID pid) const override
    {
        thread_local static std::array<const char*, Achievement::Count() + 1> achieved_list = {nullptr};
        achieved_list.fill(nullptr);
        int32_t i = 0;
        const auto verdictate = [this, pid, &i](const auto achievement)
            {
                if (VerdictateAchievement(achievement, pid)) {
                    achieved_list[i++] = Achievement(achievement).ToString();
                }
            };
        std::apply([&](const auto ...achievements)
                {
                    (verdictate(achievements), ...);
                }, typename Achievement::Tuple::Type{});
        return achieved_list.data();
    }

  private:
    GlobalInfo global_info_;
};

template <typename GameOption, typename Achievement, typename MainStage, typename... SubStages>
class GameStage;

// Is Comp Stage
template <typename GameOption, typename Achievement, typename MainStage, typename... SubStages> requires (sizeof...(SubStages) > 0)
class GameStage<GameOption, Achievement, MainStage, SubStages...>
    : public std::conditional_t<std::is_void_v<MainStage>, MainStageBaseWrapper<false, Achievement>, SubStageBaseWrapper<false, MainStage>>
    , public SubStageCheckoutHelper<SubStages, std::variant<std::unique_ptr<SubStages>...>>...
{
  public:
    using Base = std::conditional_t<std::is_void_v<MainStage>, MainStageBaseWrapper<false, Achievement>, SubStageBaseWrapper<false, MainStage>>;
    using VariantSubStage = std::variant<std::unique_ptr<SubStages>...>;
    using SubStageCheckoutHelper<SubStages, VariantSubStage>::NextSubStage...;

    template <typename ...Args>
    GameStage(Args&& ...args) : Base(std::forward<Args>(args)...) {}

    virtual ~GameStage() {}

    virtual void HandleStageBegin() override
    {
        StageLog_(InfoLog()) << "HandleStageBegin begin";
        sub_stage_ = OnStageBegin();
        std::visit(
                [this](auto&& sub_stage)
                {
                    sub_stage->HandleStageBegin();
                    if (sub_stage->IsOver()) {
                        StageLog_(WarnLog()) << "begin substage skipped";
                        this->CheckoutSubStage(CheckoutReason::SKIP);
                    }
                }, sub_stage_);
    }

    virtual StageErrCode HandleRequest(MsgReader& reader, const uint64_t pid, const bool is_public,
                                                  MsgSenderBase& reply) override
    {
        for (const auto& cmd : Base::commands_) {
            if (const auto rc = cmd.CallIfValid(reader, pid, is_public, reply); rc.has_value()) {
                StageLog_(InfoLog()) << "handle request pid=" << pid << " is_public="
                    << Bool2Str(is_public) << " rc=" << *rc;
                return *rc;
            }
        }
        return PassToSubStage_(
                [&](auto&& sub_stage) { return sub_stage->HandleRequest(reader, pid, is_public, reply); },
                CheckoutReason::BY_REQUEST);
    }

    virtual StageErrCode HandleTimeout() override
    {
        StageLog_(InfoLog()) << "HandleTimeout begin";
        return PassToSubStage_([](auto&& sub_stage) { return sub_stage->HandleTimeout(); }, CheckoutReason::BY_TIMEOUT);
    }

    virtual StageErrCode HandleLeave(const PlayerID pid) override
    {
        // We must call CompStage's OnPlayerLeave first so that it can deceide whether to finish game when substage is over.
        StageLog_(InfoLog()) << "HandleLeave begin pid=" << pid;
        this->OnPlayerLeave(pid);
        return PassToSubStage_(
                [pid](auto&& sub_stage) { return sub_stage->HandleLeave(pid); },
                CheckoutReason::BY_LEAVE);
    }

    virtual StageErrCode HandleComputerAct(const uint64_t pid, const bool ready_as_user) override
    {
        // For run_game_xxx, the tell msg will be output, so do not use EmptyMsgSender here.
        StageLog_(InfoLog()) << "HandleComputerAct begin pid=" << pid << " ready_as_user=" << Bool2Str(ready_as_user);
        const auto rc = OnComputerAct(pid, Base::TellMsgSender(pid));
        if (rc != StageErrCode::OK) {
            return rc;
        }
        return PassToSubStage_(
                [&](auto&& sub_stage) { return sub_stage->HandleComputerAct(pid, ready_as_user); },
                CheckoutReason::BY_REQUEST); // game logic not care abort computer
    }

    virtual std::string CommandInfo(const bool text_mode) const override
    {
        return std::visit([&](auto&& sub_stage) { return Base::CommandInfo(text_mode) + sub_stage->CommandInfo(text_mode); }, sub_stage_);
    }

    virtual std::string StageInfo() const override
    {
        return std::visit([this](auto&& sub_stage) { return Base::name_ + " >> " + sub_stage->StageInfo(); }, sub_stage_);
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
#ifndef TEST_BOT
        if (!Base::IsInDeduction()) {
            std::this_thread::sleep_for(std::chrono::seconds(1)); // avoid banned by chatting service
        }
#endif
        Base::masker().Clear();
        std::visit(
                [this](auto&& sub_stage)
                {
                    if (!sub_stage) {
                        StageLog_(InfoLog()) << "checkout no more substages";
                        Over();
                    } else {
                        sub_stage->HandleStageBegin();
                        if (sub_stage->IsOver()) {
                            StageLog_(WarnLog()) << "checkout substage skipped";
                            this->CheckoutSubStage(CheckoutReason::SKIP);
                        } else {
                            StageLog_(InfoLog()) << "checkout substage to \"" << sub_stage->name() << "\"";
                        }
                    }
                },
                sub_stage_);
    }

    const GameOption& option() const { return static_cast<const GameOption&>(Base::option_); }

  private:
    virtual void Over() override final
    {
        StageBase::Over();
        StageLog_(InfoLog()) << "Over";
    }

    virtual VariantSubStage OnStageBegin() = 0;
    // CompStage cannot checkout by itself so return type is void
    virtual void OnPlayerLeave(const PlayerID pid) {}
    virtual CompReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) { return StageErrCode::OK; }

    template <typename Logger>
    Logger& StageLog_(Logger&& logger) const { return Base::StageLog(logger) << "[complex_stage] "; }

    template <typename Task>
    StageErrCode PassToSubStage_(const Task& internal_task, const CheckoutReason checkout_reason)
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

    VariantSubStage sub_stage_;
};

// Is Atom Stage
template <typename GameOption, typename Achievement, typename MainStage>
class GameStage<GameOption, Achievement, MainStage>
    : public std::conditional_t<std::is_void_v<MainStage>, MainStageBaseWrapper<true, Achievement>, SubStageBaseWrapper<true, MainStage>>
{
   public:
    using Base = std::conditional_t<std::is_void_v<MainStage>, MainStageBaseWrapper<true, Achievement>, SubStageBaseWrapper<true, MainStage>>;

    template <typename ...Args>
    GameStage(Args&& ...args) : Base(std::forward<Args>(args)...) {}

    virtual ~GameStage() { Base::match_.StopTimer(); }

    virtual void HandleStageBegin() override
    {
        StageLog_(InfoLog()) << "HandleStageBegin begin";
        if constexpr (!std::is_void_v<MainStage>) {
            Base::Boardcast() << "【当前阶段】\n" << Base::main_stage().StageInfo();
        }
        TrySetDeduction_();
        OnStageBegin();
        Handle_(StageErrCode::OK);
    }

    virtual StageErrCode HandleTimeout() override final
    {
        StageLog_(InfoLog()) << "HandleTimeout begin";
        return Handle_(OnTimeout());
    }

    virtual StageErrCode HandleLeave(const PlayerID pid) override final
    {
        StageLog_(InfoLog()) << "HandleLeave begin pid=" << pid;
        Base::masker().Pin(pid);
        return Handle_(pid, true, OnPlayerLeave(pid));
    }

    virtual StageErrCode HandleRequest(MsgReader& reader, const uint64_t pid, const bool is_public,
                                                  MsgSenderBase& reply) override final
    {
        for (const auto& cmd : Base::commands_) {
            if (const auto rc = cmd.CallIfValid(reader, pid, is_public, reply); rc.has_value()) {
                StageLog_(InfoLog()) << "HandleRequest matched pid=" << pid << " is_public="
                    << Bool2Str(is_public) << " rc=" << *rc;
                return Handle_(pid, true, *rc);
            }
        }
        return StageErrCode::NOT_FOUND;
    }

    virtual StageErrCode HandleComputerAct(const uint64_t pid, const bool ready_as_user) override final
    {
        // For run_game_xxx, the tell msg will be output, so do not use EmptyMsgSender here.
        StageLog_(InfoLog()) << "HandleComputerAct begin pid=" << pid << " ready_as_user=" << Bool2Str(ready_as_user);
        return Handle_(pid, ready_as_user, OnComputerAct(pid, Base::TellMsgSender(pid)));
    }

    virtual std::string StageInfo() const override
    {
        std::string outstr = Base::name_;
        if (finish_time_.has_value()) {
            outstr += "（剩余时间：";
            outstr += std::to_string(
                    std::chrono::duration_cast<std::chrono::seconds>(*finish_time_ - std::chrono::steady_clock::now()).count());
            outstr += "秒）";
        }
        return outstr;
    }

    const GameOption& option() const { return static_cast<const GameOption&>(Base::option_); }

  protected:
    virtual void OnStageBegin() {}
    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) { return StageErrCode::CONTINUE; }
    virtual CheckoutErrCode OnTimeout() { return StageErrCode::CHECKOUT; }
    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) { return StageErrCode::READY; }
    // User can use ClearReady to unset masker. In this case, stage will not checkout.
    virtual void OnAllPlayerReady() {}

    void StartTimer(const uint64_t sec)
    {
        finish_time_ = std::chrono::steady_clock::now() + std::chrono::seconds(sec);
        // cannot pass substage pointer because substage may has been released when alert
        Base::match_.StartTimer(sec, this,
                option().global_options_.public_timer_alert_ ? TimerCallbackPublic_ : TimerCallbackPrivate_);
    }

    void StopTimer()
    {
        finish_time_ = nullptr;
    }

    void ClearReady() { Base::masker().Clear(); }
    void ClearReady(const PlayerID pid) { Base::masker().Unset(pid); }
    void SetReady(const PlayerID pid) { Base::masker().Set(pid, true); }
    bool IsReady(const PlayerID pid) const { return Base::masker().Get(pid) == Masker::State::SET; }
    bool IsAllReady() const { return Base::IsInDeduction() || Base::masker().IsReady(); }

    void HookUnreadyPlayers() const
    {
        for (PlayerID pid = 0; pid < option().PlayerNum(); ++pid) {
            if (!IsReady(pid)) {
                Base::Hook(pid);
            }
        }
    }

   private:
    static void TimerCallbackPublic_(void* const p, const uint64_t alert_sec)
    {
        auto& stage = *static_cast<GameStage*>(p);
        auto sender = stage.Boardcast();
        sender << "剩余时间" << (alert_sec / 60) << "分" << (alert_sec % 60) <<
            "秒\n\n以下玩家还未选择，要抓紧了，机会不等人\n";
        for (PlayerID pid = 0; pid < stage.option().PlayerNum(); ++pid) {
            if (stage.masker().Get(pid) == Masker::State::UNSET) {
                sender << At(pid);
            }
        }
    }

    static void TimerCallbackPrivate_(void* const p, const uint64_t alert_sec)
    {
        auto& stage = *static_cast<GameStage*>(p);
        stage.Boardcast() << "剩余时间" << (alert_sec / 60) << "分" << (alert_sec % 60) << "秒";
        for (PlayerID pid = 0; pid < stage.option().PlayerNum(); ++pid) {
            if (stage.masker().Get(pid) == Masker::State::UNSET) {
                stage.Tell(pid) << "您还未选择，要抓紧了，机会不等人";
            }
        }
    }

    template <typename Logger>
    Logger& StageLog_(Logger&& logger) const { return Base::StageLog(logger) << "[atomic_stage] "; }

    void TrySetDeduction_()
    {
        if (!Base::IsInDeduction() && Base::match_.IsInDeduction()) {
            // Because boardcast will be disabled after |is_in_deduction_|'s being set, we should boardcast at first.
            Base::Boardcast() << "所有玩家都失去了行动能力，于是游戏将直接推演至终局";
            StageLog_(WarnLog()) << "Begin deduction";
            Base::SetInDeduction();
        }
    }

    StageErrCode Handle_(StageErrCode rc)
    {
        StageLog_(InfoLog()) << "Handle errcode rc=" << rc << " is_all_ready=" << Bool2Str(IsAllReady());
        TrySetDeduction_();
        if (rc != StageErrCode::CHECKOUT && IsAllReady()) {
            // We do not check IsReady only when rc is READY to handle all player force exit.
            OnAllPlayerReady();
            rc = StageErrCode::Condition(IsAllReady(), StageErrCode::CHECKOUT, StageErrCode::CONTINUE);
            StageLog_(InfoLog()) << "OnAllPlayerReady finish rc=" << rc;
        }
        if (rc == StageErrCode::CHECKOUT) {
            Over();
        }
        return rc;
    }

    StageErrCode Handle_(const PlayerID pid, const bool is_user, StageErrCode rc)
    {
        if (rc == StageErrCode::READY) {
            StageLog_(InfoLog()) << "Handle READY pid=" << pid << " is_user=" << Bool2Str(is_user);
            Base::masker().Set(pid, is_user);
            rc = StageErrCode::OK;
        }
        return Handle_(rc);
    }

    virtual void Over() override final
    {
        Base::match_.StopTimer();
        StageBase::Over();
        StageLog_(InfoLog()) << "Over";
    }
    // if command return true, the stage will be over
    std::optional<std::chrono::time_point<std::chrono::steady_clock>> finish_time_;
};

class GameOption;
class Achievement;
class MainStage;

template <typename... SubStages>
using SubGameStage = GameStage<GameOption, Achievement, MainStage, SubStages...>;

template <typename... SubStages>
using MainGameStage = GameStage<GameOption, Achievement, void, SubStages...>;

