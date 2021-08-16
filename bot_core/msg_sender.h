#pragma once

#include <memory>
#include <variant>
#include <vector>

#include "bot_core/id.h"

void* OpenMessager(uint64_t id, bool is_uid);
void MessagerPostText(void* p, const char* data, uint64_t len);
void MessagerPostUser(void* p, uint64_t uid, bool is_at);
void MessagerFlush(void* p);
void CloseMessager(void* p);
class PlayerID;
class UserID;
class GroupID;
class Match;

template <typename IdType> struct At { IdType id_; };
template <typename IdType> struct Name { IdType id_; };

template <typename T> concept CanToString = requires(T&& t) { std::to_string(std::forward<T>(t)); };

class MsgSenderBase
{
  public:
    class MsgSenderGuard
    {
      public:
        MsgSenderGuard(MsgSenderBase& sender) : enable_at_(false), sender_(&sender) {}
        MsgSenderGuard(const MsgSenderGuard&) = delete;
        MsgSenderGuard(MsgSenderGuard&& other) : enable_at_(other.enable_at_), sender_(other.sender_)
        {
            other.sender_ = nullptr; // prevent call Flush
        }
        inline ~MsgSenderGuard();

        inline MsgSenderGuard& operator<<(const std::string_view& sv);

        template <CanToString Arg>
        MsgSenderGuard& operator<<(Arg&& arg)
        {
            return (*this) << std::to_string(arg);
        }

        inline MsgSenderGuard& operator<<(const At<UserID>&);
        inline MsgSenderGuard& operator<<(const At<PlayerID>&);
        inline MsgSenderGuard& operator<<(const Name<UserID>&);
        inline MsgSenderGuard& operator<<(const Name<PlayerID>&);

      private:
        bool enable_at_;
        MsgSenderBase* sender_;
    };

  public:
    virtual ~MsgSenderBase() {}
    virtual MsgSenderGuard operator()() { return MsgSenderGuard(*this); }

    friend class MsgSenderGuard;

  protected:
    virtual void SaveText(const char* const data, const uint64_t len) = 0;
    virtual void SaveUser(const UserID& id, const bool is_at) = 0;
    virtual void SavePlayer(const PlayerID& id, const bool is_at) = 0;
    virtual void Flush() = 0;
};

class EmptyMsgSender : public MsgSenderBase
{
  public:
    static MsgSenderBase& Get()
    {
        static EmptyMsgSender sender;
        return sender;
    }

  protected:
    virtual void SaveText(const char* const data, const uint64_t len) override {}
    virtual void SaveUser(const UserID& uid, const bool is_at) override {}
    virtual void SavePlayer(const PlayerID& pid, const bool is_at) override {}
    virtual void Flush() override {}

  private:
    EmptyMsgSender() : MsgSenderBase() {}
    ~EmptyMsgSender() {}
};

class MsgSender : public MsgSenderBase
{
  public:
    MsgSender(const UserID& uid) : sender_(Open_(uid)), match_(nullptr) {}
    MsgSender(const GroupID& gid) : sender_(Open_(gid)), match_(nullptr) {}
    MsgSender(const MsgSender&) = delete;
    MsgSender(MsgSender&& o) : sender_(o.sender_), match_(o.match_)
    {
        o.sender_ = nullptr;
        o.match_ = nullptr;
    }
    ~MsgSender() { CloseMessager(sender_); }
    void SetMatch(Match* const match) { match_ = match; }
    virtual MsgSenderGuard operator()() override { return MsgSenderGuard(*this); }

  protected:
    virtual void SaveText(const char* const data, const uint64_t len) override { MessagerPostText(sender_, data, len); }
    virtual void SaveUser(const UserID& uid, const bool is_at) override { MessagerPostUser(sender_, uid, is_at); }
    virtual void SavePlayer(const PlayerID& pid, const bool is_at) override;
    virtual void Flush() override { MessagerFlush(sender_); }
    void SaveText_(const std::string_view& sv) { SaveText(sv.data(), sv.size()); }
    template <typename Container, typename GetFn> friend class MsgSenderBatch;
  private:
    static void* Open_(const UserID& uid) { return OpenMessager(uid, true); }
    static void* Open_(const GroupID& gid) { return OpenMessager(gid, false); }
    void* sender_;
    Match* match_;
};

MsgSenderBase::MsgSenderGuard::~MsgSenderGuard()
{
    if (sender_) {
        sender_->Flush();
    }
}

MsgSenderBase::MsgSenderGuard& MsgSenderBase::MsgSenderGuard::operator<<(const At<UserID>& at_msg)
{
    sender_->SaveUser(at_msg.id_, true);
    return *this;
}

MsgSenderBase::MsgSenderGuard& MsgSenderBase::MsgSenderGuard::operator<<(const At<PlayerID>& at_msg)
{
    sender_->SavePlayer(at_msg.id_, true);
    return *this;
}

MsgSenderBase::MsgSenderGuard& MsgSenderBase::MsgSenderGuard::operator<<(const Name<UserID>& name_msg)
{
    sender_->SaveUser(name_msg.id_, false);
    return *this;
}

MsgSenderBase::MsgSenderGuard& MsgSenderBase::MsgSenderGuard::operator<<(const Name<PlayerID>& name_msg)
{
    sender_->SavePlayer(name_msg.id_, false);
    return *this;
}

MsgSenderBase::MsgSenderGuard& MsgSenderBase::MsgSenderGuard::operator<<(const std::string_view& sv)
{
    sender_->SaveText(sv.data(), sv.size());
    return *this;
}

template <typename Container, typename GetFn>
class MsgSenderBatch : public MsgSenderBase
{
  public:
    MsgSenderBatch(Container& container, const GetFn& get_fn) : container_(container), get_fn_(get_fn) {}

    virtual void SaveText(const char* const data, const uint64_t len) override
    {
        for (auto& e : container_) {
            get_fn_(e).SaveText(data, len);
        }
    }

    virtual void SaveUser(const UserID& uid, const bool is_at) override
    {
        for (auto& e : container_) {
            get_fn_(e).SaveUser(uid, is_at);
        }
    }

    virtual void SavePlayer(const PlayerID& pid, const bool is_at) override
    {
        for (auto& e : container_) {
            get_fn_(e).SavePlayer(pid, is_at);
        }
    }

    virtual void Flush() override
    {
        for (auto& e : container_) {
            get_fn_(e).Flush();
        }
    }

    void SetMatch(Match* const match) {
        for (auto& e : container_) {
            get_fn_(e).SetMatch(match);
        }
    }
  private:
    Container& container_;
    const GetFn get_fn_;
};
