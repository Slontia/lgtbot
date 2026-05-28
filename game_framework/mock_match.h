// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <memory>
#include <optional>
#include <sstream>

#include "bot_core/match_base.h"
#include "bot_core/msg_sender.h"
#include "utility/utils.h"

class MockMsgSender : public MsgSenderBase
{
  public:
    MockMsgSender(std::filesystem::path image_dir)
        : image_dir_(std::move(image_dir))
        , is_public_(true)
    {
    }

    MockMsgSender(std::filesystem::path image_dir, const PlayerID pid, const bool is_public)
        : image_dir_(std::move(image_dir))
        , pid_(pid)
        , is_public_(is_public)
    {
    }

    MsgSenderBase::MsgSenderGuard operator()() const override
    {
        MsgSenderBase::MsgSenderGuard guard(*this);
        if (is_public_ && pid_.has_value()) {
            guard << At(*pid_) << " ";
        }
        return guard;
    }

  private:
    void Flush(std::vector<MsgFragment>&& messages) const override
    {
        if (messages.empty()) {
            return;
        }
        std::stringstream ss;
        for (auto& frag : messages) {
            std::visit(Overload{
                [&](const std::string& text) { ss << text; },
                [&](const At<PlayerID>& at) { ss << "@" << at.id_.Get(); },
                [&](const Name<PlayerID>& name) { ss << "[PLAYER_" << name.id_.Get() << "]"; },
                [&](const Image& image) { ss << "[image=" << image.path_ << "]"; },
                [&](const Markdown& markdown) {
                    if (!image_dir_.empty()) {
                        const std::string path = (image_dir_ / std::to_string(++image_no_) += ".png").string();
                        MarkdownToImage(markdown.content_.c_str(), path.c_str(), markdown.width_);
                        ss << "[image=" << path << "]";
                    }
                },
                [&](const At<UserID>&) { throw std::runtime_error("user should not appear in game"); },
                [&](const Name<UserID>&) { throw std::runtime_error("user should not appear in game"); },
            }, frag);
        }
        if (is_public_) {
            std::cout << "[BOT -> GROUP]";
        } else if (pid_.has_value()) {
            std::cout << "[BOT -> PLAYER_" << *pid_ << "]";
        } else {
            throw std::runtime_error("invalid msg_sender");
        }
        std::cout << std::endl << ss.str() << std::endl;
    }

    void SetMatch(std::weak_ptr<const Match>) override {}

    mutable uint64_t image_no_{0};
    const std::filesystem::path image_dir_;
    const std::optional<PlayerID> pid_;
    const bool is_public_;
};

class MockMatch : public MatchBase
{
  public:
    MockMatch(const std::filesystem::path& image_dir, const uint64_t player_num)
        : image_dir_(image_dir.string())
        , boardcast_sender_(image_dir_)
        , is_eliminated_(player_num, false) {}

    virtual ~MockMatch() {}

    virtual MsgSenderBase& BoardcastMsgSender() override { return boardcast_sender_; }

    virtual MsgSenderBase& TellMsgSender(const PlayerID pid) override
    {
        auto it = tell_senders_.find(pid);
        if (it == tell_senders_.end()) {
            it = tell_senders_.try_emplace(pid, image_dir_, pid, false).first;
        }
        return it->second;
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

    const std::filesystem::path image_dir() const { return image_dir_; }

  private:
    const std::string image_dir_;
    MockMsgSender boardcast_sender_;
    std::map<uint64_t, MockMsgSender> tell_senders_;
    std::vector<bool> is_eliminated_;
};

