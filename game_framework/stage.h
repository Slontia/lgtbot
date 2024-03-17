// Copyright (c) 2024-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include "game_framework/stage_utility.h"
#include "game_framework/game_main.h"
#include "game_framework/stage_fsm.h"

#ifndef GAME_MODULE_NAME
#error GAME_MODULE_NAME is not defined
#endif

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

namespace internal {

class StageBaseInternal : public StageBase
{
  public:
    virtual const std::string& StageName() const = 0;
    virtual std::string StageInfo() const = 0;
    virtual std::string CommandInfo(const bool text_mode) const = 0;

    virtual StageErrCode HandleRequest(MsgReader& reader, const uint64_t pid, const bool is_public, MsgSenderBase& reply) = 0;

    void SetOver() { is_over_ = true; }
    bool IsOver() const final { return is_over_; }

  private:
    bool is_over_{false};
};

class AtomicStage : public StageBaseInternal
{
  public:
    AtomicStage(AtomicStageFsm& fsm, std::string upper_stage_info);
    ~AtomicStage() override;

    const std::string& StageName() const final { return fsm_.Name(); }
    std::string StageInfo() const final;
    std::string CommandInfo(const bool text_mode) const final;

    void HandleStageBegin() final;
    StageErrCode HandleTimeout() final;
    StageErrCode HandleRequest(MsgReader& reader, const uint64_t pid, const bool is_public, MsgSenderBase& reply) final;
    StageErrCode HandleLeave(const PlayerID pid) final;
    StageErrCode HandleComputerAct(const uint64_t pid, const bool ready_as_user) final;

  private:
    template <typename Logger>
    Logger& StageLog_(Logger&& logger) const;

    StageErrCode Handle_(StageErrCode rc);
    StageErrCode Handle_(const PlayerID pid, const bool is_user, StageErrCode rc);

    AtomicStageFsm& fsm_;
    std::string upper_stage_info_;
};

class VariantSubStage
{
  public:
    template <typename ...Subs>
    VariantSubStage(CompoundStageFsm<Subs...>& sup_stage_fsm)
        : impl_{std::make_unique<Impl<Subs...>>(sup_stage_fsm)}
    {
    }

    void Init(std::string upper_stage_info) { impl_->Init(std::move(upper_stage_info)); }

    void Checkout(const CheckoutReason reason, std::string upper_stage_info)
    {
        impl_->Checkout(reason, std::move(upper_stage_info));
    }

    StageBaseInternal* Get() { return impl_->Get(); }

    const StageBaseInternal* Get() const { return const_cast<VariantSubStage*>(this)->Get(); }

  private:
    struct Base
    {
        virtual ~Base() = default;

        virtual void Init(std::string upper_stage_info) = 0;

        virtual void Checkout(const CheckoutReason reason, std::string upper_stage_info) = 0;

        virtual StageBaseInternal* Get() = 0;
    };

    template <typename ...Subs>
    class Impl;

    std::unique_ptr<Base> impl_;
};

class CompoundStage : public StageBaseInternal
{
  public:
    template <typename ...Subs>
    CompoundStage(CompoundStageFsm<Subs...>& fsm, std::string upper_stage_info)
        : fsm_(fsm), variant_sub_stage_(fsm), upper_stage_info_(std::move(upper_stage_info))
    {
    }

    const std::string& StageName() const final { return fsm_.Name(); }
    std::string StageInfo() const final;
    std::string CommandInfo(const bool text_mode) const final;

    void HandleStageBegin() final;
    StageErrCode HandleTimeout() final;
    StageErrCode HandleRequest(MsgReader& reader, const uint64_t pid, const bool is_public, MsgSenderBase& reply) final;
    StageErrCode HandleLeave(const PlayerID pid) final;
    StageErrCode HandleComputerAct(const uint64_t pid, const bool ready_as_user) final;

  private:
    void CheckoutSubStage_(const CheckoutReason reason);

    void SwitchSubStage_(StageBaseInternal* const sub_stage, const char* const tag);

    template <typename Logger>
    Logger& StageLog_(Logger&& logger) const;

    template <typename Task>
    StageErrCode PassToSubStage_(const Task& internal_task, const CheckoutReason reason);

    CompoundStageFsmBase& fsm_;
    VariantSubStage variant_sub_stage_;
    std::string upper_stage_info_;
};

template <typename Fsm>
struct StageType;

template <typename Fsm>
requires std::is_base_of_v<AtomicStageFsm, Fsm>
struct StageType<Fsm>
{
    using Type = AtomicStage;
};

template <typename Fsm>
requires std::is_base_of_v<CompoundStageFsmBase, Fsm>
struct StageType<Fsm>
{
    using Type = CompoundStage;
};

class MainStage : public MainStageBase
{
  public:
    template <typename Fsm>
    MainStage(std::unique_ptr<Fsm> fsm)
        : fsm_{std::move(fsm)}
        , internal_stage_(std::in_place_type<typename StageType<Fsm>::Type>, static_cast<Fsm&>(*fsm_), "")
    {
    }

    void HandleStageBegin() final;
    StageErrCode HandleTimeout() final;
    StageErrCode HandleRequest(const char* const msg, const uint64_t player_id, const bool is_public, MsgSenderBase& reply) final;
    StageErrCode HandleLeave(const PlayerID pid) final;
    StageErrCode HandleComputerAct(const uint64_t pid, const bool ready_as_user) final;

    bool IsOver() const final;

    const char* StageInfoC() const final;
    const char* CommandInfoC(const bool text_mode) const final;

    int64_t PlayerScore(const PlayerID pid) const final;
    const char* const* VerdictateAchievements(const PlayerID pid) const final;

  private:
    inline StageBaseInternal& Stage_();
    inline const StageBaseInternal& Stage_() const;

    std::unique_ptr<MainStageFsm> fsm_;
    std::variant<AtomicStage, CompoundStage> internal_stage_;
};

template <typename ...Subs>
class VariantSubStage::Impl : public VariantSubStage::Base
{
  public:
    Impl(CompoundStageFsm<Subs...>& sup_stage_fsm) : sup_stage_fsm_{sup_stage_fsm} {}

    void Init(std::string upper_stage_info) final
    {
        assert(std::get_if<std::nullopt_t>(&variant_sub_stage_fsm_));
        UpdateSubStage_(
                [&](VariantStageFsm<Subs...>& variant_stage_fsm) { sup_stage_fsm_.FirstStageFsm(StageFsmSetter{variant_stage_fsm}); },
                std::move(upper_stage_info));
    }

    void Checkout(const CheckoutReason reason, std::string upper_stage_info) final
    {
        std::visit([&](auto& sub_stage_fsm)
                {
                    if constexpr (!std::is_same_v<std::nullopt_t, std::decay_t<decltype(sub_stage_fsm)>>) {
                        UpdateSubStage_([&](VariantStageFsm<Subs...>& variant_stage_fsm)
                                {
                                    sup_stage_fsm_.NextStageFsm(sub_stage_fsm, reason, StageFsmSetter{variant_stage_fsm});
                                }, std::move(upper_stage_info));
                    } else {
                        assert(false);
                    }
                }, variant_sub_stage_fsm_);
    }

    StageBaseInternal* Get() final
    {
        return std::visit([]<typename Stage>(Stage& sub_stage) -> StageBaseInternal*
                {
                    if constexpr (std::is_same_v<std::nullopt_t, Stage>) {
                        return nullptr;
                    } else {
                        return &sub_stage;
                    }
                }, variant_sub_stage_);
    }

private:
    template <typename Updater>
    void UpdateSubStage_(const Updater& updater, std::string&& upper_stage_info)
    {
        // stage must be destructed before fsm
        variant_sub_stage_.emplace<std::nullopt_t>(std::nullopt);
        updater(variant_sub_stage_fsm_);
        std::visit([&]<typename Fsm>(Fsm& fsm)
                {
                    if constexpr (!std::is_same_v<Fsm, std::nullopt_t>) {
                        variant_sub_stage_.emplace<typename StageType<Fsm>::Type>(fsm, std::move(upper_stage_info) + " >> ");
                    }
                }, variant_sub_stage_fsm_);
    }

    CompoundStageFsm<Subs...>& sup_stage_fsm_;
    std::variant<std::nullopt_t, Subs...> variant_sub_stage_fsm_{std::nullopt};
    std::variant<std::nullopt_t, AtomicStage, CompoundStage> variant_sub_stage_{std::nullopt};
};

} // namespace internal

class MainStageFactory
{
  public:
    MainStageFactory(GameOption& options, MatchBase& match) : options_(options), match_(match) {}

    template <typename Fsm>
    internal::MainStage* Create()
    {
        return new internal::MainStage(std::make_unique<Fsm>(StageUtility{options_, match_}));
    }

    const MyGameOption& Options() const { return options_; }

  private:
    GameOption& options_;
    MatchBase& match_;
};

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot
