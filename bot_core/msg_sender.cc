// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "msg_sender.h"

#include <filesystem>
#include <sstream>

#include "bot_core/match.h"
#include "utility/utils.h"

bool DownloadUserAvatar(const char* const uid, const char* const dest_filename);

namespace {

struct OutMessage {
    std::string text_;
    LGTBot_MessageType type_{LGTBot_MessageType::LGTBOT_MSG_TEXT};
};

void AppendText(std::vector<OutMessage>& out, std::string text)
{
    if (text.empty()) {
        return;
    }
    out.push_back(OutMessage{std::move(text), LGTBot_MessageType::LGTBOT_MSG_TEXT});
}

void AppendAtUser(std::vector<OutMessage>& out, const UserID& uid)
{
    out.push_back(OutMessage{uid.GetStr(), LGTBot_MessageType::LGTBOT_MSG_USER_MENTION});
}

void AppendNameUser(std::vector<OutMessage>& out, void* const handler, const LGTBot_Callback& callbacks,
        const bool is_to_user, const std::string& dest_id, const UserID& uid)
{
    char buffer[128];
    if (is_to_user) {
        callbacks.get_user_name(handler, buffer, sizeof(buffer), uid.GetCStr());
    } else {
        callbacks.get_user_name_in_group(handler, buffer, sizeof(buffer), dest_id.c_str(), uid.GetCStr());
    }
    AppendText(out, buffer);
}

void AppendAtPlayer(std::vector<OutMessage>& out, const std::shared_ptr<const Match>& match, const PlayerID& pid)
{
    if (!match || match->state() == Match::State::NOT_STARTED) {
        AppendText(out, "[" + std::to_string(pid.Get()) + "号玩家]");
        return;
    }
    AppendText(out, "[" + std::to_string(pid.Get()) + "号：");
    const auto& id = match->ConvertPid(pid);
    if (const auto pval = std::get_if<ComputerID>(&id)) {
        AppendText(out, "机器人" + std::to_string(*pval) + "号");
    } else {
        AppendAtUser(out, std::get<UserID>(id));
    }
    AppendText(out, "]");
}

void AppendNamePlayer(std::vector<OutMessage>& out, void* const handler, const LGTBot_Callback& callbacks,
        const bool is_to_user, const std::string& dest_id, const std::shared_ptr<const Match>& match, const PlayerID& pid)
{
    if (!match || match->state() == Match::State::NOT_STARTED) {
        AppendText(out, "[" + std::to_string(pid.Get()) + "号玩家]");
        return;
    }
    AppendText(out, "[" + std::to_string(pid.Get()) + "号：");
    const auto& id = match->ConvertPid(pid);
    if (const auto pval = std::get_if<ComputerID>(&id)) {
        AppendText(out, "机器人" + std::to_string(*pval) + "号");
    } else {
        AppendNameUser(out, handler, callbacks, is_to_user, dest_id, std::get<UserID>(id));
    }
    AppendText(out, "]");
}

void AppendImage(std::vector<OutMessage>& out, const Image& image)
{
    out.push_back(OutMessage{image.path_, LGTBot_MessageType::LGTBOT_MSG_IMAGE});
}

void AppendMarkdown(std::vector<OutMessage>& out, const std::string& image_path, const Markdown& markdown)
{
    if (image_path.empty()) {
        return;
    }
    std::stringstream ss;
    ss << std::this_thread::get_id();
    OutMessage pending;
    pending.text_ = (std::filesystem::path(image_path) / "gen" / ss.str() += ".png").string();
    pending.type_ = LGTBot_MessageType::LGTBOT_MSG_IMAGE;
    MarkdownToImage(markdown.content_.c_str(), pending.text_.c_str(), markdown.width_);
    out.push_back(std::move(pending));
}

} // namespace

MsgSender::MsgSender(void* handler, const std::string& image_path, const LGTBot_Callback& callbacks, const UserID& uid,
        std::weak_ptr<Match> match)
    : handler_(handler)
    , image_path_(&image_path)
    , callbacks_(&callbacks)
    , id_(uid.GetStr())
    , is_to_user_(true)
    , match_wk_(match)
{
}

MsgSender::MsgSender(void* handler, const std::string& image_path, const LGTBot_Callback& callbacks, const GroupID& gid,
        std::weak_ptr<Match> match)
    : handler_(handler)
    , image_path_(&image_path)
    , callbacks_(&callbacks)
    , id_(gid.GetStr())
    , is_to_user_(false)
    , match_wk_(match)
{
}

MsgSender::MsgSender(MsgSender&& o) noexcept
    : handler_(o.handler_)
    , image_path_(o.image_path_)
    , callbacks_(o.callbacks_)
    , id_(std::move(o.id_))
    , is_to_user_(o.is_to_user_)
    , match_wk_(std::move(o.match_wk_))
{
}

MsgSender& MsgSender::operator=(MsgSender&& o) noexcept
{
    if (this != &o) {
        handler_ = o.handler_;
        image_path_ = o.image_path_;
        callbacks_ = o.callbacks_;
        id_ = std::move(o.id_);
        is_to_user_ = o.is_to_user_;
        {
            std::lock_guard lock(match_wk_mutex_);
            std::lock_guard olock(o.match_wk_mutex_);
            match_wk_ = std::move(o.match_wk_);
        }
    }
    return *this;
}

void MsgSender::SetMatch(std::weak_ptr<const Match> match)
{
    std::lock_guard lock(match_wk_mutex_);
    match_wk_ = std::move(match);
}

MsgSenderBase::MsgSenderGuard MsgSender::operator()() const
{
    return MsgSenderGuard(*this);
}

std::shared_ptr<const Match> MsgSender::LockMatch_() const
{
    std::lock_guard lock(match_wk_mutex_);
    return match_wk_.lock();
}

void MsgSender::Flush(std::vector<MsgFragment>&& messages) const
{
    if (messages.empty()) {
        return;
    }
    const auto match = LockMatch_();
    const std::string image_path = image_path_ ? *image_path_ : std::string{};
    std::vector<OutMessage> out;
    out.reserve(messages.size() * 2);

    for (auto& frag : messages) {
        std::visit(Overload{
            [&](std::string& text) { AppendText(out, std::move(text)); },
            [&](At<UserID>& at) { AppendAtUser(out, at.id_); },
            [&](Name<UserID>& name) { AppendNameUser(out, handler_, *callbacks_, is_to_user_, id_, name.id_); },
            [&](At<PlayerID>& at) { AppendAtPlayer(out, match, at.id_); },
            [&](Name<PlayerID>& name) {
                AppendNamePlayer(out, handler_, *callbacks_, is_to_user_, id_, match, name.id_);
            },
            [&](Image& image) { AppendImage(out, image); },
            [&](Markdown& markdown) { AppendMarkdown(out, image_path, markdown); },
        }, frag);
    }

    std::vector<LGTBot_Message> raw;
    raw.reserve(out.size());
    for (const auto& message : out) {
        raw.emplace_back(message.text_.c_str(), message.type_);
    }
    callbacks_->handle_messages(handler_, id_.c_str(), is_to_user_, raw.data(), raw.size());
}
