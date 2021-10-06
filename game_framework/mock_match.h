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
        ss_ << "[image=" << path << "]";
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

  private:
    const std::optional<PlayerID> pid_;
    const bool is_public_;
    std::stringstream ss_;
};

class MockMatch : public MatchBase
{
  public:
    virtual ~MockMatch() {}

    virtual MsgSenderBase& BoardcastMsgSender() override { return boardcast_sender_; }

    virtual MsgSenderBase& TellMsgSender(const PlayerID pid) override
    {
        return tell_senders_.try_emplace(pid, MockMsgSender(pid, false)).first->second;
    }

    virtual void StartTimer(const uint64_t) override {}

    virtual void StopTimer() override {}

  private:
    MockMsgSender boardcast_sender_;
    std::map<uint64_t, MockMsgSender> tell_senders_;
};

