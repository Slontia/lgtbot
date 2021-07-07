#pragma once
#include <memory>
#include <variant>
#include <vector>

struct AtMsg {
    const uint64_t uid_;
};
struct UserMsg {
    const uint64_t uid_;
};
struct GroupUserMsg {
    const uint64_t uid_;
    const uint64_t gid_;
};
struct PlayerMsg {
    const uint64_t pid_;
};

class MsgSenderForGame
{
   public:
    virtual ~MsgSenderForGame() {}
    virtual void String(const char* const str, const size_t len) = 0;
    virtual void PlayerName(const uint64_t id) = 0;
    virtual void AtPlayer(const uint64_t id) = 0;

    MsgSenderForGame& operator<<(const std::string_view s)
    {
        String(s.data(), s.size());
        return *this;
    }

    template <typename Arith>
    requires std::is_arithmetic_v<Arith> MsgSenderForGame& operator<<(const Arith arith)
    {
        const std::string s(std::to_string(arith));
        String(s.data(), s.size());
        return *this;
    }

    MsgSenderForGame& operator<<(const AtMsg& msg)
    {
        AtPlayer(msg.uid_);
        return *this;
    }

    MsgSenderForGame& operator<<(const PlayerMsg& msg)
    {
        PlayerName(msg.pid_);
        return *this;
    }
};

class MsgSenderForBot
{
   public:
    virtual ~MsgSenderForBot() {}
    virtual void String(const char* const str, const size_t len) = 0;
    virtual void GroupUserName(const uint64_t uid, const uint64_t gid) = 0;
    virtual void UserName(const uint64_t id) = 0;
    virtual void AtUser(const uint64_t id) = 0;

    MsgSenderForBot& operator<<(const std::string_view s)
    {
        String(s.data(), s.size());
        return *this;
    }

    template <typename Arith>
    requires std::is_arithmetic_v<Arith> MsgSenderForBot& operator<<(const Arith arith)
    {
        const std::string s(std::to_string(arith));
        String(s.data(), s.size());
        return *this;
    }

    MsgSenderForBot& operator<<(const AtMsg& msg)
    {
        AtUser(msg.uid_);
        return *this;
    }

    MsgSenderForBot& operator<<(const UserMsg& msg)
    {
        UserName(msg.uid_);
        return *this;
    }

    MsgSenderForBot& operator<<(const GroupUserMsg& msg)
    {
        GroupUserName(msg.uid_, msg.gid_);
        return *this;
    }
};

template <typename Sender, typename Deleter = void (*)(Sender* const)>
class MsgSenderWrapper
{
   public:
    using Container = std::vector<std::unique_ptr<Sender, Deleter>>;
    MsgSenderWrapper() {}
    MsgSenderWrapper(Container&& senders) : senders_(std::move(senders)) {}
    MsgSenderWrapper(std::unique_ptr<Sender, Deleter>&& sender) { senders_.emplace_back(std::move(sender)); }
    MsgSenderWrapper(Sender* const sender, const Deleter& deleter)
            : MsgSenderWrapper(std::unique_ptr<Sender, Deleter>(sender, deleter))
    {
    }
    MsgSenderWrapper(const MsgSenderWrapper&) = delete;
    MsgSenderWrapper(MsgSenderWrapper&&) = default;

    template <typename Arg>
    MsgSenderWrapper& operator<<(Arg&& arg)
    {
        for (const auto& sender : senders_) {
            *sender << std::forward<Arg>(arg);
        }
        return *this;
    }

    void Over() { senders_.clear(); }

   private:
    Container senders_;
};
