// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <memory>
#include <optional>

#include "bot_core/match_base.h"
#include "bot_core/msg_sender.h"

class MockMsgSender : public MsgSenderBase
{
  public:
    MockMsgSender() : is_public_(true) {}
    MockMsgSender(const PlayerID pid, const bool is_public) : pid_(pid), is_public_(is_public) {}
    virtual MsgSenderGuard operator()() override
    {
        if (is_public_ && pid_.has_value())
        {
            SavePlayer(*pid_, true);
            SaveText(" ", 1);
        }
        return MsgSenderGuard(*this);
    }

  protected:
    virtual void SaveText(const char* const data, const uint64_t len) override
    {
        ss_ << std::string_view(data, len);
    }

    virtual void SaveUser(const UserID& pid, const bool is_at) override
    {
        throw std::runtime_error("user should not appear in game");
    }

    virtual void SavePlayer(const PlayerID& pid, const bool is_at)
    {
        if (is_at) {
            ss_ << "@" << pid;
        } else {
            ss_ << "[PLAYER_" << pid << "]";
        }
    }

    virtual void SaveImage(const char* const path)
    {
	std::basic_string<char> path_str(path);
        ss_ << "[image=" << std::string(path_str.begin(), path_str.end()) << "]";
    }

    virtual void Flush() override
    {
        if (is_public_) {
            std::cout << "[BOT -> GROUP]";
        } else if (pid_.has_value()) {
            std::cout << "[BOT -> PLAYER_" << *pid_ << "]";
        } else {
            throw std::runtime_error("invalid msg_sender");
        }
        std::cout << std::endl << ss_.str() << std::endl;
        ss_.str("");
    }

    virtual void SetMatch(const Match* const) override {}

  private:
    const std::optional<PlayerID> pid_;
    const bool is_public_;
    std::stringstream ss_;
    const Match* match_;
};

class MockMatch : public MatchBase
{
  public:
    MockMatch(const uint64_t player_num) : is_eliminated_(player_num, false) {}

    virtual ~MockMatch() {}

    virtual MsgSenderBase& BoardcastMsgSender() override { return boardcast_sender_; }

    virtual MsgSenderBase& TellMsgSender(const PlayerID pid) override
    {
        return tell_senders_.try_emplace(pid, MockMsgSender(pid, false)).first->second;
    }

    virtual MockMsgSender& GroupMsgSender() override { return boardcast_sender_; }

    virtual MsgSenderBase& BoardcastAiInfoMsgSender() override { return boardcast_sender_; }

    virtual const char* PlayerName(const PlayerID& pid) override
    {
        thread_local static std::string str;
        str = "PLAYER_" + std::to_string(pid);
        return str.c_str();
    }

    virtual const char* PlayerAvatar(const PlayerID& pid, const int32_t /*size*/) override { return ""; }

    virtual void StartTimer(const uint64_t, void* p, void(*cb)(void*, uint64_t)) override {}

    virtual void StopTimer() override {}

    virtual void Eliminate(const PlayerID pid) override { is_eliminated_[pid] = true; }

    virtual void Hook(const PlayerID pid) override {}

    virtual void Activate(const PlayerID pid) override {}

    virtual bool IsInDeduction() const override { return false; }

    virtual uint64_t MatchId() const override
    {
        static uint64_t match_id = 0;
        return ++match_id;
    }

    virtual const char* GameName() const override { return "测试游戏"; }

    bool IsEliminated(const PlayerID pid) const { return is_eliminated_[pid]; }

  private:
    MockMsgSender boardcast_sender_;
    std::map<uint64_t, MockMsgSender> tell_senders_;
    std::vector<bool> is_eliminated_;
};

