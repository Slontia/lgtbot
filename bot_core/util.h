#pragma once
#include <atomic>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <set>

#include "bot_core.h"
#include "game_framework/game_main.h"
#include "utility/msg_sender.h"
#include "utility/spinlock.h"

using ModGuard = std::function<void()>;
using replier_t = std::function<MsgSenderWrapper<MsgSenderForBot>()>;

template <typename TRef, typename T>
concept UniRef = std::is_same_v<std::decay_t<TRef>, T>;

struct GameHandle {
    GameHandle(const std::optional<uint64_t> game_id, const std::string& name, const uint64_t max_player,
               const std::string& rule, const std::function<GameBase*(void* const)>& new_game,
               const std::function<void(GameBase* const)>& delete_game, ModGuard&& mod_guard)
            : game_id_(game_id),
              name_(name),
              max_player_(max_player),
              rule_(rule),
              new_game_(new_game),
              delete_game_(delete_game),
              mod_guard_(std::forward<ModGuard>(mod_guard))
    {
    }
    GameHandle(GameHandle&&) = delete;

    std::atomic<std::optional<uint64_t>> game_id_;
    const std::string name_;
    const uint64_t max_player_;
    const std::string rule_;
    const std::function<GameBase*(void* const)> new_game_;
    const std::function<void(GameBase* const)> delete_game_;
    ModGuard mod_guard_;
};

extern const int32_t LGT_AC;
extern const UserID INVALID_USER_ID;
extern const GroupID INVALID_GROUP_ID;

class Match;
class BotCtx;

class MatchManager
{
   public:
    MatchManager(BotCtx& bot) : bot_(bot) {}
    ErrCode NewMatch(const GameHandle& game_handle, const UserID uid, const std::optional<GroupID> gid,
                     const bool skip_config, const replier_t reply);
    ErrCode ConfigOver(const UserID uid, const std::optional<GroupID> gid, const replier_t reply);
    ErrCode StartGame(const UserID uid, const std::optional<GroupID> gid, const replier_t reply);
    ErrCode AddPlayerToPrivateGame(const MatchId mid, const UserID uid, const replier_t reply);
    ErrCode AddPlayerToPublicGame(const GroupID gid, const UserID uid, const replier_t reply);
    ErrCode DeletePlayer(const UserID uid, const std::optional<GroupID> gid, const replier_t reply);
    ErrCode DeleteMatch(const MatchId id);
    std::shared_ptr<Match> GetMatch(const MatchId mid);
    std::shared_ptr<Match> GetMatch(const UserID uid, const std::optional<GroupID> gid);
    std::shared_ptr<Match> GetMatchWithGroupID(const GroupID gid);
    void ForEachMatch(const std::function<void(const std::shared_ptr<Match>)>);

   private:
    ErrCode AddPlayer_(const std::shared_ptr<Match>& match, const UserID, const replier_t reply);
    void DeleteMatch_(const MatchId id);
    template <typename IDType>
    std::shared_ptr<Match> GetMatch_(const IDType id, const std::map<IDType, std::shared_ptr<Match>>& id2match);
    template <typename IDType>
    void BindMatch_(const IDType id, std::map<IDType, std::shared_ptr<Match>>& id2match, std::shared_ptr<Match> match);
    template <typename IDType>
    void UnbindMatch_(const IDType id, std::map<IDType, std::shared_ptr<Match>>& id2match);
    MatchId NewMatchID_();

    BotCtx& bot_;
    std::map<MatchId, std::shared_ptr<Match>> mid2match_;
    std::map<UserID, std::shared_ptr<Match>> uid2match_;
    std::map<GroupID, std::shared_ptr<Match>> gid2match_;
    MatchId next_mid_;
    SpinLock spinlock_;
};

using GameHandleMap = std::map<std::string, std::unique_ptr<GameHandle>>;

class BotCtx
{
   public:
    BotCtx(const UserID this_uid, const NEW_MSG_SENDER_CALLBACK new_msg_sender_cb,
           const DELETE_MSG_SENDER_CALLBACK delete_msg_sender_cb, const std::string_view game_path,
           const uint64_t* const admins, const uint64_t admin_count)
            : this_uid_(this_uid),
              new_msg_sender_cb_(new_msg_sender_cb),
              delete_msg_sender_cb_(delete_msg_sender_cb),
              match_manager_(*this)
    {
        LoadGameModules_(game_path);
        LoadAdmins_(admins, admin_count);
    }

    MatchManager& match_manager() { return match_manager_; }
    const auto& game_handles() const { return game_handles_; }

    bool HasAdmin(const UserID uid) const { return admins_.find(uid) != admins_.end(); }

    auto ToUserRaw(const UserID uid) const
    {
        return std::unique_ptr<MsgSenderForBot, DELETE_MSG_SENDER_CALLBACK>(new_msg_sender_cb_(Target::TO_USER, uid),
                                                                            delete_msg_sender_cb_);
    }

    auto ToGroupRaw(const GroupID gid) const
    {
        return std::unique_ptr<MsgSenderForBot, DELETE_MSG_SENDER_CALLBACK>(new_msg_sender_cb_(Target::TO_GROUP, gid),
                                                                            delete_msg_sender_cb_);
    }

    auto ToUser(const UserID uid) const { return MsgSenderWrapper(ToUserRaw(uid)); }

    auto ToGroup(const GroupID gid) const { return MsgSenderWrapper(ToGroupRaw(gid)); }

   private:
    void LoadGameModules_(const std::string_view games_path);
    void LoadAdmins_(const uint64_t* const admins, const uint64_t admin_count);

    const UserID this_uid_;
    const NEW_MSG_SENDER_CALLBACK new_msg_sender_cb_;
    const DELETE_MSG_SENDER_CALLBACK delete_msg_sender_cb_;
    std::mutex mutex_;
    std::map<std::string, std::unique_ptr<GameHandle>> game_handles_;
    std::set<UserID> admins_;
    MatchManager match_manager_;
};
