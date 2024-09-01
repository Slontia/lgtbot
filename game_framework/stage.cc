// Copyright (c) 2024-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/stage.h"

#ifndef GAME_MODULE_NAME
#error GAME_MODULE_NAME is not defined
#endif

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

namespace internal {

template <typename Command>
static std::string CommandInfo(const std::vector<Command>& commands, const std::string_view stage_name, const bool text_mode)
{
    if (commands.empty()) {
        return "";
    }
    std::string outstr = "\n\n### 游戏命令-";
    outstr += stage_name;
    uint64_t i = 1;
    for (const auto& cmd : commands) {
        outstr += "\n" + std::to_string(i++) + ". " + cmd.Info(true /* with_example */, !text_mode /* with_html_color */);
    }
    return outstr;
}

AtomicStage::AtomicStage(AtomicStageFsm& fsm, std::string upper_stage_info)
    : fsm_(fsm), upper_stage_info_(std::move(upper_stage_info))
{
}

AtomicStage::~AtomicStage() { fsm_.Global().StopTimer(); }

std::string AtomicStage::StageInfo() const
{
    std::string info = upper_stage_info_ + fsm_.Name();
    if (const auto& timer_finish_time = fsm_.Global().TimerFinishTime(); timer_finish_time.has_value()) {
        const auto remain_duration = *timer_finish_time - std::chrono::steady_clock::now();
        info += "（剩余时间：";
        info += std::to_string(std::chrono::duration_cast<std::chrono::seconds>(remain_duration).count());
        info += "秒）";
    }
    return info;
}

std::string AtomicStage::CommandInfo(const bool text_mode) const
{
    return internal::CommandInfo(fsm_.Commands(), fsm_.Name(), text_mode);
}

void AtomicStage::HandleStageBegin()
{
    StageLog_(InfoLog()) << "HandleStageBegin begin";
    fsm_.Global().Boardcast() << "【当前阶段】\n" << StageInfo();
    fsm_.OnStageBegin();
    Handle_(StageErrCode::OK);
}

StageErrCode AtomicStage::HandleTimeout()
{
    StageLog_(InfoLog()) << "HandleTimeout begin";
    return Handle_(fsm_.OnStageTimeout());
}

StageErrCode AtomicStage::HandleRequest(MsgReader& reader, const uint64_t pid, const bool is_public, MsgSenderBase& reply)
{
    for (const auto& cmd : fsm_.Commands()) {
        if (const auto rc = cmd.CallIfValid(reader, pid, is_public, reply); rc.has_value()) {
            StageLog_(InfoLog()) << "HandleRequest matched pid=" << pid << " is_public="
                << Bool2Str(is_public) << " rc=" << *rc;
            return Handle_(pid, true, *rc);
        }
    }
    return StageErrCode::NOT_FOUND;
}

void AtomicStage::Terminate()
{
    StageLog_(InfoLog()) << " terminate";
    Handle_(StageErrCode::CHECKOUT);
}

StageErrCode AtomicStage::HandleLeave(const PlayerID pid)
{
    StageLog_(InfoLog()) << "HandleLeave begin pid=" << pid;
    fsm_.Global().Leave(pid);
    return Handle_(pid, true, fsm_.OnPlayerLeave(pid));
}

StageErrCode AtomicStage::HandleComputerAct(const uint64_t pid, const bool ready_as_user)
{
    // For run_game_xxx, the tell msg will be output, so do not use EmptyMsgSender here.
    StageLog_(InfoLog()) << "HandleComputerAct begin pid=" << pid << " ready_as_user=" << Bool2Str(ready_as_user);
    return Handle_(pid, ready_as_user, fsm_.OnComputerAct(pid, fsm_.Global().TellMsgSender(pid)));
}

StageErrCode AtomicStage::Handle_(StageErrCode rc)
{
    StageLog_(InfoLog()) << "Handle errcode rc=" << rc << " is_ok_to_checkout=" << Bool2Str(fsm_.Global().IsOkToCheckout());
    const auto trigger_all_player_ready = [&]() { return rc != StageErrCode::CHECKOUT && fsm_.Global().IsOkToCheckout(); };
    if (trigger_all_player_ready()) {
        while (true) {
            // We do not check IsReady only when rc is READY to handle all player force exit.
            rc = fsm_.OnStageOver();
            StageLog_(InfoLog()) << "OnStageOver finish rc=" << rc;
            if (!trigger_all_player_ready()) {
                break;
            }
#ifndef TEST_BOT
            std::this_thread::sleep_for(std::chrono::seconds(5)); // prevent frequent messages
#endif
        }
    }
    if (rc == StageErrCode::CHECKOUT) {
        fsm_.Global().StopTimer();
        SetOver();
        StageLog_(InfoLog()) << "Over";
    }
    return rc;
}

StageErrCode AtomicStage::Handle_(const PlayerID pid, const bool is_user, StageErrCode rc)
{
    if (rc == StageErrCode::READY) {
        StageLog_(InfoLog()) << "Handle READY pid=" << pid << " is_user=" << Bool2Str(is_user);
        if (is_user) {
            fsm_.Global().SetReady(pid);
        } else {
            fsm_.Global().SetReadyAsComputer(pid);
        }
        rc = StageErrCode::OK;
    }
#ifndef TEST_BOT
    if (!is_user && fsm_.Global().IsOkToCheckout()) { // computer action
        std::this_thread::sleep_for(std::chrono::seconds(5)); // prevent frequent messages
    }
#endif
    return Handle_(rc);
}

std::string CompoundStage::StageInfo() const
{
    return variant_sub_stage_.Get()->StageInfo();
}

std::string CompoundStage::CommandInfo(const bool text_mode) const
{
    return internal::CommandInfo(fsm_.Commands(), fsm_.Name(), text_mode) + variant_sub_stage_.Get()->CommandInfo(text_mode);
}

void CompoundStage::HandleStageBegin()
{
    StageLog_(InfoLog()) << "HandleStageBegin begin";
    variant_sub_stage_.Init(upper_stage_info_ + fsm_.Name());
    SwitchSubStage_(variant_sub_stage_.Get(), "begin");
}

StageErrCode CompoundStage::HandleTimeout()
{
    StageLog_(InfoLog()) << "HandleTimeout begin";
    return PassToSubStage_([](StageBaseInternal& sub_stage) { return sub_stage.HandleTimeout(); }, CheckoutReason::BY_TIMEOUT);
}

StageErrCode CompoundStage::HandleRequest(MsgReader& reader, const uint64_t pid, const bool is_public, MsgSenderBase& reply)
{
    for (const auto& cmd : fsm_.Commands()) {
        const auto rc = cmd.CallIfValid(reader, pid, is_public, reply);
        if (!rc.has_value()) {
            continue;
        }
        StageLog_(InfoLog()) << "handle request pid=" << pid << " is_public="
            << Bool2Str(is_public) << " rc=" << *rc;
        if (*rc == StageErrCode::CHECKOUT) {
            Terminate();
        }
        return *rc;
    }
    return PassToSubStage_(
            [&](StageBaseInternal& sub_stage) { return sub_stage.HandleRequest(reader, pid, is_public, reply); },
            CheckoutReason::BY_REQUEST);
}

void CompoundStage::Terminate()
{
    StageLog_(InfoLog()) << " terminate";
    SetOver();
    variant_sub_stage_.Get()->Terminate();
}

StageErrCode CompoundStage::HandleLeave(const PlayerID pid)
{
    // We must call CompoundStage's OnPlayerLeave first so that it can deceide whether to finish game when substage is over.
    StageLog_(InfoLog()) << "HandleLeave begin pid=" << pid;
    fsm_.OnPlayerLeave(pid);
    return PassToSubStage_(
            [pid](StageBaseInternal& sub_stage) { return sub_stage.HandleLeave(pid); },
            CheckoutReason::BY_LEAVE);
}

StageErrCode CompoundStage::HandleComputerAct(const uint64_t pid, const bool ready_as_user)
{
    // For run_game_xxx, the tell msg will be output, so do not use EmptyMsgSender here.
    StageLog_(InfoLog()) << "HandleComputerAct begin pid=" << pid << " ready_as_user=" << Bool2Str(ready_as_user);
    const auto rc = fsm_.OnComputerAct(pid, fsm_.Global().TellMsgSender(pid));
    if (rc == StageErrCode::CHECKOUT) {
        Terminate();
        return rc;
    }
    if (rc != StageErrCode::OK) {
        return rc;
    }
    return PassToSubStage_(
            [&](StageBaseInternal& sub_stage) { return sub_stage.HandleComputerAct(pid, ready_as_user); },
            CheckoutReason::BY_REQUEST); // game logic not care abort computer
}

void CompoundStage::CheckoutSubStage_(const CheckoutReason reason)
{
    variant_sub_stage_.Checkout(reason, upper_stage_info_ + fsm_.Name());
#ifndef TEST_BOT
    if (!fsm_.Global().IsInDeduction()) {
        std::this_thread::sleep_for(std::chrono::seconds(1)); // avoid banned by chatting service
    }
#endif
    fsm_.Global().ClearReady();
    SwitchSubStage_(variant_sub_stage_.Get(), "checkout");
}

void CompoundStage::SwitchSubStage_(StageBaseInternal* const sub_stage, const char* const tag)
{
    if (!sub_stage) {
        StageLog_(InfoLog()) << tag << " no more substages";
        SetOver();
        StageLog_(InfoLog()) << "Over";
        return;
    }
    sub_stage->HandleStageBegin();
    if (sub_stage->IsOver()) {
        StageLog_(WarnLog()) << tag << " substage skipped";
        CheckoutSubStage_(CheckoutReason::SKIP);
    } else {
        StageLog_(InfoLog()) << tag << " substage to \"" << sub_stage->StageName() << "\"";
    }
}

template <typename Task>
StageErrCode CompoundStage::PassToSubStage_(const Task& internal_task, const CheckoutReason reason)
{
    // return rc to check in unittest
    const auto rc = internal_task(*variant_sub_stage_.Get());
    if (variant_sub_stage_.Get()->IsOver()) {
        CheckoutSubStage_(reason);
    }
    return rc;
}

void MainStage::HandleStageBegin()
{
    return Stage_().HandleStageBegin();
}

StageErrCode MainStage::HandleTimeout() { return Stage_().HandleTimeout(); }

StageErrCode MainStage::HandleLeave(const PlayerID pid) { return Stage_().HandleLeave(pid); }

StageErrCode MainStage::HandleComputerAct(const uint64_t pid, const bool ready_as_user)
{
    return Stage_().HandleComputerAct(pid, ready_as_user);
}

bool MainStage::IsOver() const { return Stage_().IsOver(); }

StageErrCode MainStage::HandleRequest(const char* const msg, const uint64_t player_id, const bool is_public,
                                    MsgSenderBase& reply)
{
    fsm_->Global().Activate(player_id);
    MsgReader reader(msg);
    return Stage_().HandleRequest(reader, player_id, is_public, reply);
}

const char* MainStage::StageInfoC() const
{
    thread_local std::string result;
    result = Stage_().StageInfo();
    return result.c_str();
}

const char* MainStage::CommandInfoC(const bool text_mode) const
{
    thread_local std::string result;
    result = Stage_().CommandInfo(text_mode);
    return result.c_str();
}

int64_t MainStage::PlayerScore(const PlayerID pid) const { return fsm_->PlayerScore(pid); }

const char* const* MainStage::VerdictateAchievements(const PlayerID pid) const
{
    thread_local static std::vector<const char*> achieved_list;
    achieved_list.clear();
    for (const auto achievement : Achievement::Members()) {
        for (uint8_t i = 0; i < fsm_->Global().AchievementCount(pid, achievement); ++i) {
            achieved_list.emplace_back(achievement.ToString());
        }
    }
    achieved_list.emplace_back(nullptr);
    return achieved_list.data();
}

inline StageBaseInternal& MainStage::Stage_()
{
    return std::visit([](auto& stage) -> StageBaseInternal& { return stage; }, internal_stage_);
}

inline const StageBaseInternal& MainStage::Stage_() const
{
    return std::visit([](const auto& stage) -> const StageBaseInternal& { return stage; }, internal_stage_);
}

} // namespace internal

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot
