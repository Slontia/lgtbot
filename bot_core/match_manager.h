#ifdef ENUM_BEGIN
#ifdef ENUM_MEMBER
#ifdef ENUM_END

ENUM_BEGIN(MatchFlag)
ENUM_MEMBER(MatchFlag, 配置)
ENUM_MEMBER(MatchFlag, 电脑)
ENUM_END(MatchFlag)

#endif
#endif
#endif

#ifndef MATCH_MANAGER_H
#define MATCH_MANAGER_H

#include <optional>
#include <bitset>
#include <memory>
#include <map>
#include <functional>
#include <variant>
#include <mutex>

#include "bot_core/bot_core.h"

#define ENUM_FILE "bot_core/match_manager.h"
#include "utility/extend_enum.h"

class Match;
class BotCtx;
class MsgSenderBase;

class MatchManager
{
   public:
    MatchManager(BotCtx& bot) : bot_(bot), next_mid_(0) {}
    ErrCode NewMatch(const GameHandle& game_handle, const UserID uid, const std::optional<GroupID> gid,
                     const MatchFlag::BitSet& flags, MsgSenderBase& reply);
    ErrCode ConfigOver(const UserID uid, const std::optional<GroupID> gid, MsgSenderBase& reply);
    ErrCode SetComNum(const UserID uid, const std::optional<GroupID> gid, MsgSenderBase& reply, const uint64_t com_num);
    ErrCode StartGame(const UserID uid, const std::optional<GroupID> gid, MsgSenderBase& reply);
    ErrCode AddPlayerToPrivateGame(const MatchID mid, const UserID uid, MsgSenderBase& reply);
    ErrCode AddPlayerToPublicGame(const GroupID gid, const UserID uid, MsgSenderBase& reply);
    ErrCode DeletePlayer(const UserID uid, const std::optional<GroupID> gid, MsgSenderBase& reply, const bool force);
    ErrCode DeleteMatch(const MatchID id);
    std::shared_ptr<Match> GetMatch(const MatchID mid);
    std::shared_ptr<Match> GetMatch(const UserID uid, const std::optional<GroupID> gid);
    std::shared_ptr<Match> GetMatch(const GroupID gid);
    void ForEachMatch(const std::function<void(const std::shared_ptr<Match>)>);

   private:
    std::variant<ErrCode, std::shared_ptr<Match>> UnsafeGetMatchByHost_(
            const UserID uid, const std::optional<GroupID> gid, MsgSenderBase& reply);
    ErrCode AddPlayer_(const std::shared_ptr<Match>& match, const UserID, MsgSenderBase& reply);
    void DeleteMatch_(const MatchID id);
    template <typename IDType>
    std::shared_ptr<Match> GetMatch_(const IDType id, const std::map<IDType, std::shared_ptr<Match>>& id2match);
    template <typename IDType>
    void BindMatch_(const IDType id, std::map<IDType, std::shared_ptr<Match>>& id2match, std::shared_ptr<Match> match);
    template <typename IDType>
    void UnbindMatch_(const IDType id, std::map<IDType, std::shared_ptr<Match>>& id2match);
    MatchID NewMatchID_();

    BotCtx& bot_;
    std::mutex mutex_;
    std::map<MatchID, std::shared_ptr<Match>> mid2match_;
    std::map<UserID, std::shared_ptr<Match>> uid2match_;
    std::map<GroupID, std::shared_ptr<Match>> gid2match_;
    MatchID next_mid_;
};

#endif
