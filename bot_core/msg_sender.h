// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <memory>
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

template <typename IdType>
using MsgFragmentT = std::variant<std::string, At<IdType>, Name<IdType>, Image, Markdown>;

template <typename IdType>
inline void AppendMsgFragmentText(std::vector<MsgFragmentT<IdType>>& buf, std::string text)
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

template <typename IdType>
inline void AppendMsgFragmentText(std::vector<MsgFragmentT<IdType>>& buf, const char* const data,
        const uint64_t len)
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

template <typename IdType>
inline void AppendMsgFragmentText(std::vector<MsgFragmentT<IdType>>& buf, const std::string_view sv)
{
    AppendMsgFragmentText<IdType>(buf, sv.data(), sv.size());
}

template <typename IdType>
class MsgSenderBaseT
{
  public:
    using Fragment = MsgFragmentT<IdType>;

    class MsgSenderGuard
    {
      public:
        explicit MsgSenderGuard(const MsgSenderBaseT& sender) : sender_(&sender) {}
        explicit MsgSenderGuard(std::unique_ptr<MsgSenderBaseT> owned)
            : owned_(std::move(owned))
            , sender_(owned_.get())
        {}
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

        MsgSenderGuard& operator<<(At<IdType> at) { messages_.emplace_back(std::move(at)); return *this; }
        MsgSenderGuard& operator<<(Name<IdType> name) { messages_.emplace_back(std::move(name)); return *this; }
        MsgSenderGuard& operator<<(Image image) { messages_.emplace_back(std::move(image)); return *this; }
        MsgSenderGuard& operator<<(Markdown markdown) { messages_.emplace_back(std::move(markdown)); return *this; }

      private:
        std::unique_ptr<MsgSenderBaseT> owned_;
        const MsgSenderBaseT* sender_{nullptr};
        std::vector<Fragment> messages_;

        friend class MsgSenderBaseT;
    };

  public:
    virtual ~MsgSenderBaseT() = default;
    virtual MsgSenderGuard operator()() const { return MsgSenderGuard(*this); }
    void DeliverMessages(std::vector<Fragment>&& messages) const { Flush(std::move(messages)); }

  protected:
    virtual void Flush(std::vector<Fragment>&& messages) const = 0;

    friend class MsgSenderGuard;
};

using HostMsgSenderBase  = MsgSenderBaseT<UserID>;
using ChildMsgSenderBase = MsgSenderBaseT<PlayerID>;
using HostMsgFragment    = MsgFragmentT<UserID>;
using ChildMsgFragment   = MsgFragmentT<PlayerID>;

template <typename IdType>
class EmptyMsgSenderT : public MsgSenderBaseT<IdType>
{
  public:
    static MsgSenderBaseT<IdType>& Get()
    {
        static EmptyMsgSenderT sender;
        return sender;
    }

  private:
    void Flush(std::vector<MsgFragmentT<IdType>>&&) const override {}

    EmptyMsgSenderT() = default;
    ~EmptyMsgSenderT() = default;
};

using HostEmptyMsgSender  = EmptyMsgSenderT<UserID>;
using ChildEmptyMsgSender = EmptyMsgSenderT<PlayerID>;

class Match;

class MsgSender : public HostMsgSenderBase
{
  public:
    MsgSender(void* handler, const std::string& image_path, const LGTBot_Callback& callbacks, const UserID& uid);
    MsgSender(void* handler, const std::string& image_path, const LGTBot_Callback& callbacks, const GroupID& gid);

    MsgSender(const MsgSender&) = delete;
    MsgSender(MsgSender&& o) noexcept;
    MsgSender& operator=(const MsgSender&) = delete;
    MsgSender& operator=(MsgSender&& o) noexcept;

    ~MsgSender() override = default;

    HostMsgSenderBase::MsgSenderGuard operator()() const override;

  private:
    void Flush(std::vector<HostMsgFragment>&& messages) const override;

    void* handler_{nullptr};
    const std::string* image_path_{nullptr};
    const LGTBot_Callback* callbacks_{nullptr};
    std::string id_;
    bool is_to_user_{false};
};

template <typename IdType>
inline MsgSenderBaseT<IdType>::MsgSenderGuard::MsgSenderGuard(MsgSenderGuard&& other) noexcept
    : owned_(std::move(other.owned_))
    , sender_(other.sender_)
    , messages_(std::move(other.messages_))
{
    if (owned_) {
        sender_ = owned_.get();
    }
    other.sender_ = nullptr;
}

template <typename IdType>
inline typename MsgSenderBaseT<IdType>::MsgSenderGuard& MsgSenderBaseT<IdType>::MsgSenderGuard::operator=(
        MsgSenderGuard&& other) noexcept
{
    if (this != &other) {
        owned_ = std::move(other.owned_);
        sender_ = other.sender_;
        messages_ = std::move(other.messages_);
        if (owned_) {
            sender_ = owned_.get();
        }
        other.sender_ = nullptr;
    }
    return *this;
}

template <typename IdType>
inline MsgSenderBaseT<IdType>::MsgSenderGuard::~MsgSenderGuard()
{
    if (sender_) {
        sender_->Flush(std::move(messages_));
    }
}

template <typename IdType>
inline void MsgSenderBaseT<IdType>::MsgSenderGuard::Release()
{
    owned_.reset();
    sender_ = nullptr;
    messages_.clear();
}

template <typename IdType>
inline typename MsgSenderBaseT<IdType>::MsgSenderGuard& MsgSenderBaseT<IdType>::MsgSenderGuard::operator<<(
        const std::string_view& sv)
{
    AppendMsgFragmentText<IdType>(messages_, sv);
    return *this;
}
