#include "bot_core/match_manager.h"

#include <cassert>

#include "bot_core/msg_sender.h"
#include "bot_core/match.h"

// para func can appear only once
#define RETURN_IF_FAILED(func)                                 \
    do {                                                       \
        if (const auto ret = (func); ret != EC_OK) return ret; \
    } while (0);

ErrCode MatchManager::NewMatch(const GameHandle& game_handle, const UserID uid, const std::optional<GroupID> gid,
                               const MatchFlag::BitSet& flags, MsgSenderBase& reply)
{
    std::lock_guard<std::mutex> l(mutex_);
    if (GetMatch_(uid, uid2match_)) {
        reply() << "[错误] 建立失败：您已加入游戏";
        return EC_MATCH_USER_ALREADY_IN_MATCH;
    }
    if (gid.has_value() && GetMatch_(*gid, gid2match_)) {
        reply() << "[错误] 建立失败：该房间已经开始游戏";
        return EC_MATCH_ALREADY_BEGIN;
    }
    const MatchID mid = NewMatchID_();
    std::shared_ptr<Match> new_match = std::make_shared<Match>(bot_, mid, game_handle, uid, gid, flags);
    RETURN_IF_FAILED(AddPlayer_(new_match, uid, reply));
    BindMatch_(mid, mid2match_, new_match);
    if (gid.has_value()) {
        BindMatch_(*gid, gid2match_, new_match);
    }
    return EC_OK;
}

std::variant<ErrCode, std::shared_ptr<Match>> MatchManager::UnsafeGetMatchByHost_(
        const UserID uid, const std::optional<GroupID> gid, MsgSenderBase& reply)
{
    const std::shared_ptr<Match>& match = GetMatch_(uid, uid2match_);
    if (!match) {
        reply() << "[错误] 开始失败：您未加入游戏";
        return EC_MATCH_USER_NOT_IN_MATCH;
    }
    if (match->host_uid() != uid) {
        reply() << "[错误] 开始失败：您不是房主，没有开始游戏的权限";
        return EC_MATCH_NOT_HOST;
    }
    if (gid.has_value() && match->gid() != gid) {
        reply() << "[错误] 开始失败：您未在该房间建立游戏";
        return EC_MATCH_NOT_THIS_GROUP;
    }
    return match;
}

ErrCode MatchManager::ConfigOver(const UserID uid, const std::optional<GroupID> gid, MsgSenderBase& reply)
{
    std::lock_guard<std::mutex> l(mutex_);
    const auto match_or_errcode = UnsafeGetMatchByHost_(uid, gid, reply);
    if (const auto p_errcode = std::get_if<0>(&match_or_errcode)) {
        return *p_errcode;
    }
    const auto match = std::get<1>(match_or_errcode);
    return match->GameConfigOver(reply);
}

ErrCode MatchManager::SetComNum(const UserID uid, const std::optional<GroupID> gid, MsgSenderBase& reply, const uint64_t com_num)
{
    std::lock_guard<std::mutex> l(mutex_);
    const auto match_or_errcode = UnsafeGetMatchByHost_(uid, gid, reply);
    if (const auto p_errcode = std::get_if<0>(&match_or_errcode)) {
        return *p_errcode;
    }
    const auto match = std::get<1>(match_or_errcode);
    return match->GameSetComNum(reply, com_num);
}

ErrCode MatchManager::StartGame(const UserID uid, const std::optional<GroupID> gid, MsgSenderBase& reply)
{
    std::lock_guard<std::mutex> l(mutex_);
    const auto match_or_errcode = UnsafeGetMatchByHost_(uid, gid, reply);
    if (const auto p_errcode = std::get_if<0>(&match_or_errcode)) {
        return *p_errcode;
    }
    const auto match = std::get<1>(match_or_errcode);
    return match->GameStart(gid.has_value(), reply);
}

ErrCode MatchManager::AddPlayerToPrivateGame(const MatchID mid, const UserID uid, MsgSenderBase& reply)
{
    std::lock_guard<std::mutex> l(mutex_);
    const std::shared_ptr<Match> match = GetMatch_(mid, mid2match_);
    if (!match) {
        reply() << "[错误] 加入失败：游戏ID不存在";
        return EC_MATCH_NOT_EXIST;
    }
    if (!match->IsPrivate()) {
        reply() << "[错误] 加入失败：该游戏属于公开比赛，请前往房间加入游戏";
        return EC_MATCH_NEED_REQUEST_PUBLIC;
    }
    return AddPlayer_(match, uid, reply);
}

ErrCode MatchManager::AddPlayerToPublicGame(const GroupID gid, const UserID uid, MsgSenderBase& reply)
{
    std::lock_guard<std::mutex> l(mutex_);
    const std::shared_ptr<Match> match = GetMatch_(gid, gid2match_);
    if (!match) {
        reply() << "[错误] 加入失败：该房间未进行游戏";
        return EC_MATCH_GROUP_NOT_IN_MATCH;
    }
    return AddPlayer_(match, uid, reply);
}

ErrCode MatchManager::AddPlayer_(const std::shared_ptr<Match>& match, const UserID uid, MsgSenderBase& reply)
{
    assert(match);
    if (const std::shared_ptr<Match>& user_match = GetMatch_(uid, uid2match_); user_match == match) {
        reply() << "[错误] 加入失败：您已加入该游戏";
        return EC_MATCH_USER_ALREADY_IN_MATCH;
    } else if (user_match != nullptr) {
        reply() << "[错误] 加入失败：您已加入其它游戏，请先退出";
        return EC_MATCH_USER_ALREADY_IN_OTHER_MATCH;
    }
    RETURN_IF_FAILED(match->Join(uid, reply));
    BindMatch_(uid, uid2match_, match);
    return EC_OK;
}

ErrCode MatchManager::DeletePlayer(const UserID uid, const std::optional<GroupID> gid, MsgSenderBase& reply, const bool force)
{
    std::shared_ptr<Match> match = nullptr;
    {
        std::lock_guard<std::mutex> l(mutex_);
        match = GetMatch_(uid, uid2match_);
        if (!match) {
            reply() << "[错误] 退出失败：您未加入游戏";
            return EC_MATCH_USER_NOT_IN_MATCH;
        }
        if (match->gid() != gid) {
            reply() << "[错误] 退出失败：您未加入本房间游戏";
            return EC_MATCH_NOT_THIS_GROUP;
        }

        RETURN_IF_FAILED(match->Leave(uid, reply, force));

        // It is save to unbind player before game over because erase do nothing when element not in map.
        UnbindMatch_(uid, uid2match_);

        // If host quit, switch host.
        if (uid == match->host_uid() && !match->SwitchHost()) {
            DeleteMatch_(match->mid());
        }
    }
    if (force) {
        // Call LeaveMidway without match lock to prevent dead lock when game over and DeleteMatch.
        return match->LeaveMidway(uid, gid.has_value());
    }
    return EC_OK;
}

ErrCode MatchManager::DeleteMatch(const MatchID mid)
{
    std::lock_guard<std::mutex> l(mutex_);
    DeleteMatch_(mid);
    return EC_OK;
}

void MatchManager::DeleteMatch_(const MatchID mid)
{
    if (const std::shared_ptr<Match> match = GetMatch_(mid, mid2match_); match) {
        UnbindMatch_(mid, mid2match_);
        if (match->gid().has_value()) {
            UnbindMatch_(*match->gid(), gid2match_);
        }
        const auto& ready_uid_set = match->ready_uid_set();
        for (auto it = ready_uid_set.begin(); it != ready_uid_set.end(); ++it) {
            UnbindMatch_(it->first, uid2match_);
        }
    }
}

std::shared_ptr<Match> MatchManager::GetMatch(const MatchID mid)
{
    std::lock_guard<std::mutex> l(mutex_);
    return GetMatch_(mid, mid2match_);
}

std::shared_ptr<Match> MatchManager::GetMatch(const UserID uid, const std::optional<GroupID> gid)
{
    std::lock_guard<std::mutex> l(mutex_);
    std::shared_ptr<Match> match = GetMatch_(uid, uid2match_);
    return (!match || (gid.has_value() && GetMatch_(*gid, gid2match_) != match)) ? nullptr : match;
}

std::shared_ptr<Match> MatchManager::GetMatch(const GroupID gid)
{
    std::lock_guard<std::mutex> l(mutex_);
    return GetMatch_(gid, gid2match_);
}

void MatchManager::ForEachMatch(const std::function<void(const std::shared_ptr<Match>)> handle)
{
    std::lock_guard<std::mutex> l(mutex_);
    for (const auto& [_, match] : mid2match_) {
        handle(match);
    }
}

MatchID MatchManager::NewMatchID_()
{
    while (mid2match_.find(++next_mid_) != mid2match_.end())
        ;
    return next_mid_;
}

template <typename IDType>
std::shared_ptr<Match> MatchManager::GetMatch_(const IDType id,
                                               const std::map<IDType, std::shared_ptr<Match>>& id2match)
{
    const auto it = id2match.find(id);
    return (it == id2match.end()) ? nullptr : it->second;
}

template <typename IDType>
void MatchManager::BindMatch_(const IDType id, std::map<IDType, std::shared_ptr<Match>>& id2match,
                              std::shared_ptr<Match> match)
{
    if (!id2match.emplace(id, match).second) {
        assert(false);
    }
}

template <typename IDType>
void MatchManager::UnbindMatch_(const IDType id, std::map<IDType, std::shared_ptr<Match>>& id2match)
{
    id2match.erase(id);
}

