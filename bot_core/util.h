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

#ifndef UTIL_H
#define UTIL_H
#include <atomic>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <cassert>
#include <variant>
#include <bitset>

#include "bot_core.h"
#include "game_framework/game_main.h"
#include "bot_core/msg_sender.h"
#include "utility/spinlock.h"

#define ENUM_FILE "bot_core/util.h"
#include "utility/extend_enum.h"


template <typename TRef, typename T>
concept UniRef = std::is_same_v<std::decay_t<TRef>, T>;

class GameOptionBase;
class MainStageBase;
class MsgSenderBase;

struct GameHandle {
    using ModGuard = std::function<void()>;
    using game_options_allocator = GameOptionBase*(*)();
    using game_options_deleter = void(*)(const GameOptionBase*);
    using game_options_ptr = std::unique_ptr<GameOptionBase, game_options_deleter>;
    using main_stage_allocator = MainStageBase*(*)(MsgSenderBase&, const GameOptionBase&);
    using main_stage_deleter = void(*)(const MainStageBase*);
    using main_stage_ptr = std::unique_ptr<MainStageBase, main_stage_deleter>;

    GameHandle(const std::optional<uint64_t> game_id, const std::string& name, const uint64_t max_player,
               const std::string& rule,
               const game_options_allocator& game_options_allocator_fn,
               const game_options_deleter& game_options_deleter_fn,
               const main_stage_allocator& main_stage_allocator_fn,
               const main_stage_deleter& main_stage_deleter_fn,
               ModGuard&& mod_guard)
        : is_released_(game_id.has_value())
        , game_id_(game_id.has_value() ? *game_id : 0)
        , name_(name)
        , max_player_(max_player)
        , rule_(rule)
        , game_options_allocator_(game_options_allocator_fn)
        , game_options_deleter_(game_options_deleter_fn)
        , main_stage_allocator_(main_stage_allocator_fn)
        , main_stage_deleter_(main_stage_deleter_fn)
        , mod_guard_(std::forward<ModGuard>(mod_guard))
    {
    }

    GameHandle(GameHandle&&) = delete;

    std::optional<uint64_t> game_id() const
    {
        if (is_released_.load()) {
            return game_id_.load();
	    } else {
            return std::nullopt;
	    }
    }

    void set_game_id(const uint64_t game_id)
    {
        assert(!is_released_.load());
	    game_id_.store(game_id);
	    is_released_.store(true);
    }

    void clear_game_id()
    {
        assert(is_released_.load());
	    is_released_.store(false);
	    game_id_.store(0);
    }

    game_options_ptr make_game_options() const
    {
        return game_options_ptr(game_options_allocator_(), game_options_deleter_);
    }

    main_stage_ptr make_main_stage(MsgSenderBase& reply, const GameOptionBase& game_options) const
    {
        return main_stage_ptr(main_stage_allocator_(reply, game_options), main_stage_deleter_);
    }

    const std::string name_;
    const uint64_t max_player_;
    const std::string rule_;
    const game_options_allocator game_options_allocator_;
    const game_options_deleter game_options_deleter_;
    const main_stage_allocator main_stage_allocator_;
    const main_stage_deleter main_stage_deleter_;
    const ModGuard mod_guard_;
 
  private:
    std::atomic<bool> is_released_;
    std::atomic<uint64_t> game_id_;
};

extern const int32_t LGT_AC;
extern const UserID INVALID_USER_ID;
extern const GroupID INVALID_GROUP_ID;

class Match;
class BotCtx;
class MsgSenderBase;

class MatchManager
{
   public:
    MatchManager(BotCtx& bot) : bot_(bot), next_mid_(0) {}
    ErrCode NewMatch(const GameHandle& game_handle, const UserID uid, const std::optional<GroupID> gid,
                     const std::bitset<MatchFlag::Count()>& flags, MsgSenderBase& reply);
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
    std::map<MatchID, std::shared_ptr<Match>> mid2match_;
    std::map<UserID, std::shared_ptr<Match>> uid2match_;
    std::map<GroupID, std::shared_ptr<Match>> gid2match_;
    MatchID next_mid_;
    SpinLock spinlock_;
};

using GameHandleMap = std::map<std::string, std::unique_ptr<GameHandle>>;

void* OpenMessager(uint64_t id, bool is_uid);
void MessagerPostText(void* p, const char* data, uint64_t len);
void MessagerPostUserName(void* p, uint64_t uid);
void MessagerPostUserNameInGroup(void* p, uint64_t uid, uint64_t gid);
void MessagerPostAtUser(void* p, uint64_t uid);
void CloseMessagerAndFlush(void* p);

class BotCtx
{
  public:
    BotCtx(const uint64_t this_uid, const char* const game_path, const uint64_t* const admins, const uint64_t admin_count)
            : this_uid_(this_uid),
              match_manager_(*this)
    {
        if (game_path) {
            LoadGameModules_(game_path);
        }
        if (admins && admin_count > 0) {
            LoadAdmins_(admins, admin_count);
        }
    }

    MatchManager& match_manager() { return match_manager_; }

#ifdef TEST_BOT
    auto& game_handles() { return game_handles_; }
#else
    const auto& game_handles() const { return game_handles_; }
#endif

    bool HasAdmin(const UserID uid) const { return admins_.find(uid) != admins_.end(); }

  private:
    void LoadGameModules_(const char* const games_path);
    void LoadAdmins_(const uint64_t* const admins, const uint64_t admin_count);

    const UserID this_uid_;
    std::mutex mutex_;
    std::map<std::string, std::unique_ptr<GameHandle>> game_handles_;
    std::set<UserID> admins_;
    MatchManager match_manager_;
};
#endif
