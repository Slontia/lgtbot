#include "bot_core/match_manager.h"

#include <cassert>

#include "bot_core/msg_sender.h"
#include "bot_core/match.h"

std::pair<ErrCode, std::shared_ptr<Match>> MatchManager::NewMatch(const GameHandle& game_handle, const UserID uid, const std::optional<GroupID> gid,
                               MsgSenderBase& reply)
{
    std::lock_guard<std::mutex> l(mutex_);
    if (GetMatch_(uid)) {
        reply() << "[错误] 建立失败：您已加入游戏";
        return {EC_MATCH_USER_ALREADY_IN_MATCH, nullptr};
    }
    if (gid.has_value() && GetMatch_(*gid)) {
        reply() << "[错误] 建立失败：该房间已经开始游戏";
        return {EC_MATCH_ALREADY_BEGIN, nullptr};
    }
    const MatchID mid = NewMatchID_();
    const auto new_match = std::make_shared<Match>(bot_, mid, game_handle, uid, gid);
    BindMatch_(mid, new_match);
    BindMatch_(uid, new_match);
    if (gid.has_value()) {
        BindMatch_(*gid, new_match);
    }
    return {EC_OK, new_match};
}

std::vector<std::shared_ptr<Match>> MatchManager::Matches() const
{
    std::lock_guard<std::mutex> l(mutex_);
    std::vector<std::shared_ptr<Match>> matches;
    for (const auto& [_, match] : id2match<MatchID>()) {
        matches.emplace_back(match);
    }
    return matches;
}

MatchID MatchManager::NewMatchID_()
{
    const auto& mid2match = id2match<MatchID>();
    while (mid2match.find(++next_mid_) != mid2match.end())
        ;
    return next_mid_;
}

bool MatchManager::HasMatch() const
{
    return std::apply([&](const auto& ...id2match) { return (!id2match.empty() || ...); }, id2match_);
}
