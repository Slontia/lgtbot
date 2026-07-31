// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <functional>
#include <filesystem>
#include <thread>
#include <cstring>

#include "bot_core/id.h"
#include "bot_core/image.h"
#include "bot_core/bot_core.h"

class Match;

template <typename IdType> struct At { IdType id_; };
template <typename IdType> struct Name { IdType id_; };

template <typename IdType>
At(IdType) -> At<IdType>;
template <typename IdType>
Name(IdType) -> Name<IdType>;

struct Image { std::string path_; };
// Own markdown text: many call sites pass `Markdown(F(), w)` where `F()` returns a temporary
// `std::string`; a `string_view` into that temporary would dangle before flush runs.
struct Markdown {
    std::string content_;
    uint32_t width_{600};

    Markdown()
        : content_()
        , width_(600)
    {
    }

    explicit Markdown(std::string content, const uint32_t width = 600)
        : content_(std::move(content))
        , width_(width)
    {
    }

    explicit Markdown(const char* const content, const uint32_t width = 600)
        : content_(content ? content : "")
        , width_(width)
    {
    }

    explicit Markdown(const std::string_view sv, const uint32_t width = 600)
        : content_(sv)
        , width_(width)
    {
    }
};

template <typename T> concept CanToString = requires(T&& t) { std::to_string(std::forward<T>(t)); };

using MsgFragment = std::variant<std::string, At<UserID>, At<PlayerID>, Name<UserID>, Name<PlayerID>, Image, Markdown>;

inline void AppendMsgFragmentText(std::vector<MsgFragment>& buf, std::string text)
{
    if (text.empty()) {
        return;
    }
    if (buf.empty() || !std::holds_alternative<std::string>(buf.back())) {
        buf.emplace_back(std::move(text));
    } else {
        std::get<std::string>(buf.back()) += std::move(text);
    }
}

inline void AppendMsgFragmentText(std::vector<MsgFragment>& buf, const char* const data, const uint64_t len)
{
    if (len == 0) {
        return;
    }
    if (buf.empty() || !std::holds_alternative<std::string>(buf.back())) {
        buf.emplace_back(std::string(data, len));
    } else {
        std::get<std::string>(buf.back()).append(data, len);
    }
}

inline void AppendMsgFragmentText(std::vector<MsgFragment>& buf, const std::string_view sv)
{
    AppendMsgFragmentText(buf, sv.data(), sv.size());
}

class MsgSenderBase
{
  public:
    class MsgSenderGuard
    {
      public:
        explicit MsgSenderGuard(const MsgSenderBase& sender) : sender_(&sender) {}
        MsgSenderGuard(const MsgSenderGuard&) = delete;
        MsgSenderGuard(MsgSenderGuard&& other) noexcept;
        MsgSenderGuard& operator=(const MsgSenderGuard&) = delete;
        MsgSenderGuard& operator=(MsgSenderGuard&& other) noexcept;
        ~MsgSenderGuard();

        void Release();

        MsgSenderGuard& operator<<(const std::string_view& sv);

        template <CanToString Arg>
        MsgSenderGuard& operator<<(Arg&& arg)
        {
            return (*this) << std::to_string(arg);
        }

        MsgSenderGuard& operator<<(const char c) { return (*this) << std::string(1, c); }

        MsgSenderGuard& operator<<(At<UserID> at) { messages_.emplace_back(std::move(at)); return *this; }
        MsgSenderGuard& operator<<(At<PlayerID> at) { messages_.emplace_back(std::move(at)); return *this; }
        MsgSenderGuard& operator<<(Name<UserID> name) { messages_.emplace_back(std::move(name)); return *this; }
        MsgSenderGuard& operator<<(Name<PlayerID> name) { messages_.emplace_back(std::move(name)); return *this; }
        MsgSenderGuard& operator<<(Image image) { messages_.emplace_back(std::move(image)); return *this; }
        MsgSenderGuard& operator<<(Markdown markdown) { messages_.emplace_back(std::move(markdown)); return *this; }

      private:
        const MsgSenderBase* sender_{nullptr};
        std::vector<MsgFragment> messages_;

        friend class MsgSenderBase;
    };

  public:
    virtual ~MsgSenderBase() = default;
    virtual MsgSenderGuard operator()() const { return MsgSenderGuard(*this); }
    virtual void SetMatch(std::weak_ptr<const Match> match) = 0;

    template <typename> friend class MsgSenderBatch;

  protected:
    virtual void Flush(std::vector<MsgFragment>&& messages) const = 0;

    friend class MsgSenderGuard;
};

class EmptyMsgSender : public MsgSenderBase
{
  public:
    static MsgSenderBase& Get()
    {
        static EmptyMsgSender sender;
        return sender;
    }

  private:
    void Flush(std::vector<MsgFragment>&&) const override {}
    void SetMatch(std::weak_ptr<const Match>) override {}

    EmptyMsgSender() = default;
    ~EmptyMsgSender() = default;
};

class MsgSender : public MsgSenderBase
{
  public:
    MsgSender(void* handler, const std::string& image_path, const LGTBot_Callback& callbacks, const UserID& uid,
            std::weak_ptr<Match> match = {});
    MsgSender(void* handler, const std::string& image_path, const LGTBot_Callback& callbacks, const GroupID& gid,
            std::weak_ptr<Match> match = {});

    MsgSender(const MsgSender&) = delete;
    MsgSender(MsgSender&& o) noexcept;
    MsgSender& operator=(const MsgSender&) = delete;
    MsgSender& operator=(MsgSender&& o) noexcept;

    ~MsgSender() override = default;

    void SetMatch(std::weak_ptr<const Match> match) override;
    MsgSenderBase::MsgSenderGuard operator()() const override;

  private:
    void Flush(std::vector<MsgFragment>&& messages) const override;

    std::shared_ptr<const Match> LockMatch_() const;

    void* handler_{nullptr};
    const std::string* image_path_{nullptr};
    const LGTBot_Callback* callbacks_{nullptr};
    std::string id_;
    bool is_to_user_{false};
    mutable std::mutex match_wk_mutex_;
    std::weak_ptr<const Match> match_wk_;
};

template <typename Fn>
class MsgSenderBatch : public MsgSenderBase
{
  public:
    explicit MsgSenderBatch(Fn&& fn) : fn_(std::forward<Fn>(fn)) {}

  private:
    void Flush(std::vector<MsgFragment>&& messages) const override;
    void SetMatch(std::weak_ptr<const Match> match) override;

    Fn fn_;
};

template <typename Fn>
void MsgSenderBatch<Fn>::Flush(std::vector<MsgFragment>&& messages) const
{
    if (messages.empty()) {
        return;
    }
    fn_([&](MsgSenderBase& sender) {
        auto copy = messages;
        sender.Flush(std::move(copy));
    });
}

template <typename Fn>
void MsgSenderBatch<Fn>::SetMatch(std::weak_ptr<const Match> match)
{
    fn_([&](MsgSenderBase& sender) { sender.SetMatch(match); });
}

inline MsgSenderBase::MsgSenderGuard::MsgSenderGuard(MsgSenderGuard&& other) noexcept
    : sender_(other.sender_)
    , messages_(std::move(other.messages_))
{
    other.sender_ = nullptr;
}

inline MsgSenderBase::MsgSenderGuard& MsgSenderBase::MsgSenderGuard::operator=(MsgSenderGuard&& other) noexcept
{
    if (this != &other) {
        sender_ = other.sender_;
        messages_ = std::move(other.messages_);
        other.sender_ = nullptr;
    }
    return *this;
}

inline MsgSenderBase::MsgSenderGuard::~MsgSenderGuard()
{
    if (sender_) {
        sender_->Flush(std::move(messages_));
    }
}

inline void MsgSenderBase::MsgSenderGuard::Release()
{
    sender_ = nullptr;
    messages_.clear();
}

inline MsgSenderBase::MsgSenderGuard& MsgSenderBase::MsgSenderGuard::operator<<(const std::string_view& sv)
{
    AppendMsgFragmentText(messages_, sv);
    return *this;
}
