#pragma once
#include <deque>
#include <string>
#include <optional>
#include <chrono>

template <typename Chip>
class BiddingManager
{
  public:
    BiddingManager(const uint64_t player_num)
    {
        for (uint64_t i = 0; i < player_num; ++i) {
            players_.emplace_back(i);
        }
    }

    template <typename Sender>
    bool Bid(const PlayerID pid, const std::optional<Chip>& chip, Sender&& sender)
    {
        auto& player = players_.at(pid);
        if (!player.is_allow_) {
            sender << "投标失败：您不是上一轮出价最高者，当前不可参与投标哦~";
            return false;
        }
        if (!chip.has_value() && player.chip_.has_value()) {
            sender << "撤标失败：非首轮投标不允许撤标";
            return false;
        }
        if (chip < player.chip_) {
            assert(player.chip_.has_value());
            sender << "投标失败：本轮投标额不能低于上一轮投标额" << *player.chip_;
            return false;
        }
        player.cur_chip_ = chip;
        if (chip.has_value()) {
            sender << "投标成功，您当前的投标额为：" << *chip;
        } else {
            sender << "选择成功，您决定不参与投标";
        }
        return true;
    }

    std::pair<std::optional<Chip>, std::vector<PlayerID>> BidOver(MsgSenderBase& sender)
    {
        sender() << "投标结束，下面公布结果：";
        std::map<Chip, std::vector<PlayerID>> chips;
        for (auto& player : players_) {
            player.chip_ = player.cur_chip_;
            if (player.is_allow_ && player.chip_.has_value()) {
                chips[*player.chip_].emplace_back(player.pid_);
            } else {
                player.is_allow_ = false;
            }
        }
#ifndef TEST_BOT
        std::this_thread::sleep_for(std::chrono::seconds(1));
#endif
        for (auto it = chips.begin(); it != chips.end(); ++it) {
            for (auto& pid : it->second) {
                auto& player = players_[pid];
                sender() << At(pid) << "出价：" << *player.chip_;
#ifndef TEST_BOT
                std::this_thread::sleep_for(std::chrono::seconds(1));
#endif
                if (std::next(it) != chips.end()) {
                    player.is_allow_ = false;
                }
            }
            if (std::next(it) == chips.end()) {
                return {it->first, std::move(it->second)};
            }
        }
        sender() << "无人投标！";
        return {};
    }

    const auto& TotalChip(const PlayerID pid)
    {
        return players_.at(pid).total_chip_;
    }

  private:
    friend auto operator<=>(const std::optional<Chip>& _1, const std::optional<Chip>& _2)
    {
        if (_1.has_value() && _2.has_value()) {
            return *_1 <=> *_2;
        } else if (!_1.has_value() == !_2.has_value()) {
            return 0;
        } else if (_1.has_value()) {
            return 1;
        } else {
            return -1;
        }
    }

    friend auto& operator+=(std::optional<Chip>& _1, const std::optional<Chip>& _2)
    {
        if (_2.has_value()) {
            if (_1.has_value()) {
                *_1 += *_2;
            } else {
                _1.emplace(*_2);
            }
        }
        return _1;
    }

    struct Player
    {
        Player(const PlayerID pid) : pid_(pid) {}
        const PlayerID pid_;
        bool is_allow_ = true;
        std::optional<Chip> chip_;
        std::optional<Chip> cur_chip_;
        friend auto operator<=>(const auto& _1, const auto& _2) { return _1.cur_chip_ <=> _2.cur_chip_; }
    };

    std::vector<Player> players_;
};

