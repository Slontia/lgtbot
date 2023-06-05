// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <memory>
#include <variant>
#include <vector>
#include <functional>
#include <filesystem>
#include <thread>
#include <cstring>

#include "bot_core/id.h"
#include "bot_core/image.h"
#include "bot_core/bot_core.h"

class PlayerID;
class UserID;
class GroupID;
class Match;

template <typename IdType> struct At { IdType id_; };
template <typename IdType> struct Name { IdType id_; };
struct Image { std::string path_; };
struct Markdown { std::string_view data_; uint32_t width_ = 600; };

template <typename T> concept CanToString = requires(T&& t) { std::to_string(std::forward<T>(t)); };

class MsgSenderBase
{
  public:
    class MsgSenderGuard
    {
      public:
        MsgSenderGuard(MsgSenderBase& sender) : sender_(&sender) {}
        MsgSenderGuard(const MsgSenderGuard&) = delete;
        MsgSenderGuard(MsgSenderGuard&& other) : sender_(other.sender_)
        {
            other.sender_ = nullptr; // prevent call Flush
        }

        inline ~MsgSenderGuard();

        void Release() { sender_ = nullptr; }

        inline MsgSenderGuard& operator<<(const std::string_view& sv);

        template <CanToString Arg>
        MsgSenderGuard& operator<<(Arg&& arg)
        {
            return (*this) << std::to_string(arg);
        }

        inline MsgSenderGuard& operator<<(const char c) { return (*this) << std::string(1, c); }


        // It is safe to use STL because MsgSenderGuard will not be passed between libraries
        inline MsgSenderGuard& operator<<(const At<UserID>&);
        inline MsgSenderGuard& operator<<(const At<PlayerID>&);
        inline MsgSenderGuard& operator<<(const Name<UserID>&);
        inline MsgSenderGuard& operator<<(const Name<PlayerID>&);
        inline MsgSenderGuard& operator<<(const Image&);
        inline MsgSenderGuard& operator<<(const Markdown&);

      private:
        MsgSenderBase* sender_;
    };

    template<typename> friend class MsgSenderBatch;

  public:
    virtual ~MsgSenderBase() {}
    virtual MsgSenderGuard operator()() { return MsgSenderGuard(*this); }
    virtual void SetMatch(const Match* const match) = 0;

    friend class MsgSenderGuard;

  protected:
    virtual void SaveText(const char* const data, const uint64_t len) = 0;
    virtual void SaveUser(const UserID& id, const bool is_at) = 0;
    virtual void SavePlayer(const PlayerID& id, const bool is_at) = 0;
    virtual void SaveImage(const char* const path) = 0;
    virtual void SaveMarkdown(const char* const markdown, const uint32_t width) = 0;
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
    virtual void SaveImage(const char* const path) override {};
    virtual void SaveMarkdown(const char* const markdown, const uint32_t width) override {};
    virtual void Flush() override {}
    virtual void SetMatch(const Match* const match) override {}

  private:
    EmptyMsgSender() : MsgSenderBase() {}
    ~EmptyMsgSender() {}
};

class MsgSender : public MsgSenderBase
{
  public:
    MsgSender(void* handler, const std::string& image_path, const LGTBot_Callback& callbacks, const UserID& uid, Match* const match = nullptr)
        : handler_(handler), image_path_(&image_path), callbacks_(&callbacks), id_(uid.GetStr()), is_to_user_(true), match_(match) {}

    MsgSender(void* handler, const std::string& image_path, const LGTBot_Callback& callbacks, const GroupID& gid, Match* const match = nullptr)
        : handler_(handler), image_path_(&image_path), callbacks_(&callbacks), id_(gid.GetStr()), is_to_user_(false), match_(match) {}

    MsgSender(const MsgSender&) = delete;
    MsgSender(MsgSender&& o) = default;

    ~MsgSender()
    {
    }

    explicit MsgSender(const Match* const match = nullptr) : match_(match) {}

    virtual void SetMatch(const Match* const match) override { match_ = match; }

    virtual MsgSenderGuard operator()() override { return MsgSenderGuard(*this); }

  protected:
    virtual void SaveText(const char* const data, const uint64_t len) override
    {
        // TODO: len is useless
        if (messages_.empty() || messages_.back().type_ != LGTBot_MessageType::LGTBOT_MSG_TEXT) {
            messages_.emplace_back(std::string(data, len), LGTBot_MessageType::LGTBOT_MSG_TEXT);
        } else {
            messages_.back().str_.append(data, len);
        }
    }

    virtual void SaveUser(const UserID& uid, const bool is_at) override
    {
        if (is_at) {
            messages_.emplace_back(uid.GetStr(), LGTBot_MessageType::LGTBOT_MSG_USER_MENTION);
            return;
        }
        char buffer[128];
        if (is_to_user_) {
            callbacks_->get_user_name(handler_, buffer, sizeof(buffer), uid.GetCStr());
        } else {
            callbacks_->get_user_name_in_group(handler_, buffer, sizeof(buffer), id_.c_str(), uid.GetCStr());
        }
        SaveText(buffer, std::strlen(buffer));
    }

    virtual void SavePlayer(const PlayerID& pid, const bool is_at) override;

    virtual void SaveImage(const char* const path) override
    {
        messages_.emplace_back(std::string(path), LGTBot_MessageType::LGTBOT_MSG_IMAGE);
    }

    virtual void SaveMarkdown(const char* const markdown, const uint32_t width)
    {
        if (!image_path_) {
            return;
        }
        std::stringstream ss;
        ss << std::this_thread::get_id();
        const std::string path =
            (std::filesystem::path(*image_path_) / "gen" / ss.str() += ".png").string();
        MarkdownToImage(markdown, path, width);
        SaveImage(path.c_str());
    }

    virtual void Flush() override
    {
        std::vector<LGTBot_Message> raw_messages;
        raw_messages.reserve(messages_.size());
        for (const auto& message : messages_) {
            raw_messages.emplace_back(message.str_.c_str(), message.type_);
        }
        callbacks_->handle_messages(handler_, id_.c_str(), is_to_user_, raw_messages.data(), raw_messages.size());
        messages_.clear();
    }

    void SaveText_(const std::string_view& sv)
    {
        SaveText(sv.data(), sv.size());
    }

  private:
    struct Message
    {
        std::string str_;
        LGTBot_MessageType type_;
    };
    void* handler_;
    const std::string* image_path_;
    const LGTBot_Callback* callbacks_;
    std::string id_;
    bool is_to_user_;
    const Match* match_;
    std::vector<Message> messages_;
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

MsgSenderBase::MsgSenderGuard& MsgSenderBase::MsgSenderGuard::operator<<(const Image& image_msg)
{
    sender_->SaveImage(image_msg.path_.c_str());
    return *this;
}

MsgSenderBase::MsgSenderGuard& MsgSenderBase::MsgSenderGuard::operator<<(const Markdown& markdown_msg)
{
    sender_->SaveMarkdown(markdown_msg.data_.data(), markdown_msg.width_);
    return *this;
}

MsgSenderBase::MsgSenderGuard& MsgSenderBase::MsgSenderGuard::operator<<(const std::string_view& sv)
{
    sender_->SaveText(sv.data(), sv.size());
    return *this;
}

// `MsgSenderBatch` is a wrapper which support sending the same message to several `MsgSender`s. User-defined class `Fn`
// should override the operator() which handles a function `sender_fn`. When we send something by `MsgSenderBatch`, it
// will invoke the `fn_` and pass a function as `sender_fn`. The user-defined `Fn` would pass each `MsgSender` to the
// `sender_fn` so that the message can be sent by each `MsgSender`.
template <typename Fn>
class MsgSenderBatch : public MsgSenderBase
{
  public:
    MsgSenderBatch(Fn&& fn) : fn_(std::forward<Fn>(fn)) {}

  protected:
    virtual void SaveText(const char* const data, const uint64_t len) override
    {
        fn_([&](MsgSenderBase& sender) { sender.SaveText(data, len); });
    }

    virtual void SaveUser(const UserID& uid, const bool is_at) override
    {
        fn_([&](MsgSenderBase& sender) { sender.SaveUser(uid.GetCStr(), is_at); });
    }

    virtual void SavePlayer(const PlayerID& pid, const bool is_at) override
    {
        fn_([&](MsgSenderBase& sender) { sender.SavePlayer(pid, is_at); });
    }

    virtual void SaveImage(const char* const path) override
    {
        fn_([&](MsgSenderBase& sender) { sender.SaveImage(path); });
    }

    virtual void Flush() override
    {
        fn_([&](MsgSenderBase& sender) { sender.Flush(); });
    }

    virtual void SaveMarkdown(const char* const markdown, const uint32_t width) override
    {
        fn_([&](MsgSenderBase& sender) { sender.SaveMarkdown(markdown, width); });
    };

    virtual void SetMatch(const Match* const match) override
    {
        fn_([&](MsgSenderBase& sender) { sender.SetMatch(match); });
    }

  private:
    Fn fn_;
};
