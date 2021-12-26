#pragma once

#include <cassert>

#include <map>
#include <set>
#include <bitset>
#include <variant>

#include "utility/msg_checker.h"

#include "bot_core/match_base.h"
#include "bot_core/msg_sender.h"
#include "bot_core/timer.h"
#include "bot_core/game_handle.h"
#include "bot_core/bot_ctx.h"
#include "bot_core/db_manager.h"

#define INVALID_LOBBY (QQ)0

#define INVALID_MATCH (MatchID)0

inline bool match_is_valid(MatchID id) { return id != INVALID_MATCH; }

typedef enum { PRIVATE_MATCH, GROUP_MATCH, DISCUSS_MATCH } MatchType;

class GameBase;
class Match;
class PrivateMatch;
class GroupMatch;
class DiscussMatch;
class MatchManager;
class GameHandle;

template <typename ...Ts>
class Overload : public Ts...
{
  public:
    Overload(Ts&& ...ts) : Ts(std::forward<Ts>(ts))... {}
    using Ts::operator()...;
};

struct ParticipantUser
{
    enum class State { ACTIVE, LEFT };
    ParticipantUser(const UserID uid)
        : uid_(uid)
        , sender_(uid)
        , state_(State::ACTIVE)
        , leave_when_config_changed_(true)
    {}
    const UserID uid_;
    std::vector<PlayerID> pids_;
    MsgSender sender_;
    State state_;
    bool leave_when_config_changed_;
};

class Match : public MatchBase, public std::enable_shared_from_this<Match>
{
  public:
    using VariantID = std::variant<UserID, ComputerID>;
    enum State { NOT_STARTED = 'N', IS_STARTED = 'S', IS_OVER = 'O' };
    static const uint32_t kAvgScoreOffset = 10;

    Match(BotCtx& bot, const MatchID id, const GameHandle& game_handle, const UserID host_uid,
          const std::optional<GroupID> gid);
    ~Match();

    ErrCode SetBenchTo(const UserID uid, MsgSenderBase& reply, const std::optional<uint64_t> com_num);
    ErrCode SetMultiple(const UserID uid, MsgSenderBase& reply, const uint32_t multiple);

    ErrCode Request(const UserID uid, const std::optional<GroupID> gid, const std::string& msg, MsgSender& reply);
    ErrCode GameConfigOver(MsgSenderBase& reply);
    ErrCode GameStart(const UserID uid, const bool is_public, MsgSenderBase& reply);
    ErrCode Join(const UserID uid, MsgSenderBase& reply);
    ErrCode Leave(const UserID uid, MsgSenderBase& reply, const bool force);
    ErrCode LeaveMidway(const UserID uid, const bool is_public);
    virtual MsgSenderBase& BoardcastMsgSender() override;
    virtual MsgSenderBase& TellMsgSender(const PlayerID pid) override;
    virtual const char* PlayerName(const PlayerID& pid)
    {
        thread_local static std::string str;
        const auto& id = ConvertPid(pid);
        if (const auto pval = std::get_if<ComputerID>(&id)) {
            return (str = "机器人" + std::to_string(*pval) + "号").c_str();
        }
        if (!gid().has_value()) {
            return GetUserName(std::get<UserID>(id), nullptr);
        }
        const uint64_t gid_value = *gid();
        return GetUserName(std::get<UserID>(id), &gid_value);
    }
    MsgSenderBase::MsgSenderGuard Boardcast() { return BoardcastMsgSender()(); }
    MsgSenderBase::MsgSenderGuard BoardcastAtAll();
    MsgSenderBase::MsgSenderGuard Tell(const PlayerID pid) { return TellMsgSender(pid)(); }
    virtual void StartTimer(const uint64_t sec, void* p, void(*cb)(void*, uint64_t)) override;
    virtual void StopTimer() override;
    virtual void Eliminate(const PlayerID pid) override;
    void ShowInfo(const std::optional<GroupID>&, MsgSenderBase& reply) const;

    bool SwitchHost();

    bool Has(const UserID uid) const;
    bool IsPrivate() const { return !gid_.has_value(); }
    auto PlayerNum() const { return players_.size(); }

    VariantID ConvertPid(const PlayerID pid) const;

    ErrCode Terminate(const bool is_force);

    const GameHandle& game_handle() const { return game_handle_; }
    MatchID mid() const { return mid_; }
    std::optional<GroupID> gid() const { return gid_; }
    UserID host_uid() const { return host_uid_; }
    const auto& users() const { return users_; }
    const State state() const { return state_; }
    MatchManager& match_manager() { return bot_.match_manager(); }

    const uint64_t user_controlled_player_num() const { return users_.size() * player_num_each_user_; }
    const uint64_t com_num() const { return std::max(int64_t(0), static_cast<int64_t>(bench_to_player_num_ - user_controlled_player_num())); }

   private:
    static constexpr auto k_zero_sum_score_multi_ = 1000;
    static constexpr auto k_top_score_multi_ = 10;

    ErrCode CheckScoreEnough_(const UserID uid, MsgSenderBase& reply, const uint32_t multiple) const;

    std::string State2String()
    {
        switch (state_) {
        case State::NOT_STARTED:
            return "未开始";
        case State::IS_STARTED:
            return "已开始";
        case State::IS_OVER:
            return "已结束";
        }
    }
    static std::vector<ScoreInfo> CalScores_(const std::vector<std::pair<UserID, int64_t>>& scores,
            const uint64_t multiple = 1);
    void OnGameOver_();
    void Help_(MsgSenderBase& reply, const bool text_mode);
    void Routine_();
    std::string OptionInfo_() const;
    void KickForConfigChange_();
    void Unbind_();
    void Terminate_();

    mutable std::mutex mutex_;

    // bot
    BotCtx& bot_;

    // basic info
    const MatchID mid_;
    const GameHandle& game_handle_;
    UserID host_uid_;
    const std::optional<GroupID> gid_;
    State state_;

    // time info
    std::shared_ptr<bool> timer_is_over_; // must before match because atom stage will call StopTimer
    std::unique_ptr<Timer> timer_;
    //std::chrono::time_point<std::chrono::system_clock> start_time_;
    //std::chrono::time_point<std::chrono::system_clock> end_time_;

    // game
    GameHandle::game_options_ptr options_;
    GameHandle::main_stage_ptr main_stage_;

    // player info
    std::map<UserID, ParticipantUser> users_;
    std::variant<MsgSender, MsgSenderBatch> boardcast_sender_;

    // other options
    uint64_t bench_to_player_num_;
    uint64_t player_num_each_user_;
    uint16_t multiple_;

    // player info (fill when game ready to start)
    struct Player
    {
        Player(const VariantID& id) : id_(id), is_eliminated_(false) {}
        VariantID id_;
        bool is_eliminated_;
    };
    std::vector<Player> players_; // all players, include computers

    const Command<void(MsgSenderBase&)> help_cmd_;
};
