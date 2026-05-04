// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <atomic>
#include <chrono>
#include <filesystem>

#include "msg_sender.h"

#include "bot_core/match.h"

std::atomic<uint64_t> MsgSender::markdown_image_seq_{0};

bool DownloadUserAvatar(const char* const uid, const char* const dest_filename);

void MsgSender::SavePlayer(const PlayerID& pid, const bool is_at)
{
    if (!match_ || match_->state() == Match::State::NOT_STARTED) {
        SaveText_("[" + std::to_string(pid) + "号玩家]");
        return;
    }
    SaveText_("[" + std::to_string(pid) + "号：");
    const auto& id = match_->ConvertPid(pid);
    if (const auto pval = std::get_if<ComputerID>(&id)) {
        SaveText_("机器人" + std::to_string(*pval) + "号");
    } else {
        SaveUser(std::get<UserID>(id), is_at);
    }
    SaveText_("]");
}

void MsgSender::SaveMarkdown(const char* const markdown, const uint32_t width)
{
    if (!image_path_) {
        return;
    }
    const std::string path =
            (std::filesystem::path(*image_path_) / "gen" /
                    (std::to_string(markdown_image_seq_.fetch_add(1)) + ".png"))
                    .string();
    MarkdownToImage(markdown, path, width);
    messages_.emplace_back(std::move(path), LGTBot_MessageType::LGTBOT_MSG_IMAGE, true);
}

void MsgSender::Flush()
{
    if (messages_.empty()) {
        return;
    }
    std::vector<LGTBot_Message> raw_messages;
    raw_messages.reserve(messages_.size());
    for (const auto& message : messages_) {
        raw_messages.emplace_back(message.str_.c_str(), message.type_);
    }
    callbacks_->handle_messages(handler_, id_.c_str(), is_to_user_, raw_messages.data(), raw_messages.size());
    if (match_) {
        std::vector<RecordedMsgItem> recorded;
        recorded.reserve(messages_.size());
        for (const auto& message : messages_) {
            recorded.push_back(RecordedMsgItem{message.type_, message.str_});
        }
        const RecordedMessage::Participant sender{RecordedMessage::Bot{}};
        const RecordedMessage::Receiver receiver =
                is_to_user_ ? RecordedMessage::Receiver{UserID{id_}} : RecordedMessage::Receiver{RecordedMessage::Public{}};
        match_->RecordMessages(sender, receiver, std::chrono::system_clock::now(), std::move(recorded));
    }
    for (const auto& message : messages_) {
        if (message.delete_after_send_) {
            std::error_code ec;
            std::filesystem::remove(message.str_, ec);
        }
    }
    messages_.clear();
}

