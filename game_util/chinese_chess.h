// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#ifdef ENUM_BEGIN
#ifdef ENUM_MEMBER
#ifdef ENUM_END

ENUM_BEGIN(KingdomId)
ENUM_MEMBER(KingdomId, 蓝)
ENUM_MEMBER(KingdomId, 红)
ENUM_MEMBER(KingdomId, 黄)
ENUM_MEMBER(KingdomId, 紫)
ENUM_MEMBER(KingdomId, 黑)
ENUM_MEMBER(KingdomId, 绿)
ENUM_END(KingdomId)

#endif
#endif
#endif

#ifndef CHINESE_CHESS_H_
#define CHINESE_CHESS_H_

#include <iostream>
#include <array>
#include <memory>
#include <vector>
#include <cassert>
#include <random>

#include "utility/html.h"

#define ENUM_FILE "../game_util/chinese_chess.h"
#include "../utility/extend_enum.h"


namespace chinese_chess {

const constexpr static uint32_t k_max_kingdom = 6;

struct Coor
{
    int32_t m_ = -1;
    int32_t n_ = -1;
    bool operator==(const Coor& c) const { return m_ == c.m_ && n_ == c.n_; }
    Coor& operator+=(const Coor& c)
    {
        m_ += c.m_;
        n_ += c.n_;
        return *this;
    }
    Coor operator+(const Coor& c) const { return Coor(*this) += c; }
    std::string ToString () const { return char('A' + m_) + std::to_string(n_); };
};

class ChessRule;

struct Chess
{
    ChessRule* chess_rule_; // cannot be null
    KingdomId kingdom_id_;
};

struct SettleResult
{
    struct EatResult
    {
        Chess eating_chess_;
        Chess ate_chess_;
    };
    std::vector<EatResult> eat_results_;
    std::vector<Chess> crashed_chesses_;
};

enum class ChessType { JU, MA, XIANG, SHI, JIANG, PAO, ZU, PROMOTED_ZU };

class HalfBoard;

class ChessRule
{
  public:
    // |src| and |dst| should be different and valid
    virtual bool CanMove(const HalfBoard& half_board, const Coor& src, const Coor& dst) const = 0;
    virtual ChessType Type() const = 0;
    virtual const char* Name() const = 0;
    virtual const char* ChineseName() const = 0;
};

class Area
{
  public:
    enum class MoveState { FREEZE, MOVABLE, MOVED, CANNOT_EAT };

    Area() : move_state_(MoveState::MOVABLE) {}

    bool Empty() const { return !chess_.has_value(); }

    const std::optional<Chess>& GetChess() const { return chess_; }

    std::optional<Chess>& GetChess() { return chess_; }

    void SetChess(const Chess& chess) { chess_ = chess; }

    void Settle(SettleResult& result)
    {
        if (move_state_ == MoveState::FREEZE || move_state_ == MoveState::CANNOT_EAT) {
            move_state_ = MoveState::MOVABLE;
        } else if (move_state_ == MoveState::MOVED) {
            // keep MOVED state for one round to draw images
            if (chess_ == std::nullopt) {
                move_state_ = MoveState::MOVABLE;
            } else {
                chess_.reset();
            }
        }
        if (chesses_moved_here_.size() > 1) { // crashed
            result.crashed_chesses_.insert(result.crashed_chesses_.end(), chesses_moved_here_.begin(),
                    chesses_moved_here_.end());
        } else if (!chesses_moved_here_.empty()) {
            if (chess_.has_value()) {
                result.eat_results_.emplace_back(chesses_moved_here_[0], *chess_);
                move_state_ = MoveState::FREEZE;
            } else {
                move_state_ = MoveState::CANNOT_EAT;
            }
            chess_ = chesses_moved_here_[0];
        }
        chesses_moved_here_.clear();
    }

    auto move_state() const { return move_state_; }

    static void Move(Area& src, Area& dst, ChessRule* const chess_rule = nullptr)
    {
        assert(src.chess_.has_value());
        src.move_state_ = MoveState::MOVED;
        dst.chesses_moved_here_.emplace_back(chess_rule == nullptr ? src.chess_->chess_rule_ : chess_rule,
                src.chess_->kingdom_id_);
    }

  private:
    MoveState move_state_;
    std::optional<Chess> chess_;
    std::vector<Chess> chesses_moved_here_;
};

class KingdomInfo;

class HalfBoard
{
  public:
    const constexpr static int32_t k_max_m = 5;
    const constexpr static int32_t k_max_n = 9;

    explicit HalfBoard(const KingdomId kingdom_id);

    const Area& Get(const Coor& c) const
    {
        return c.m_ < k_max_m ? areas_[c.m_][c.n_] : oppo_board_->Get(GetOppoCoor_(c));
    }

    Area& Get(const Coor& c)
    {
        return c.m_ < k_max_m ? areas_[c.m_][c.n_] : oppo_board_->Get(GetOppoCoor_(c));
    }

    std::string Move(const uint32_t player_id, const Coor& src, const Coor& dst,
            const std::vector<std::unique_ptr<KingdomInfo>>& kingdoms);

    void Settle(SettleResult& result)
    {
        for (auto& area_row : areas_) {
            for (auto& area : area_row) {
                area.Settle(result);
            }
        }
    }

    void SetOppoBoard(HalfBoard& oppo_board) { oppo_board_ = &oppo_board; }

    std::string ToHtml(const std::string& image_path, const std::vector<std::unique_ptr<KingdomInfo>>& kingdoms) const;

    static std::string ToHtml(const HalfBoard& b1, const HalfBoard& b2, const std::string& image_path,
        const std::vector<std::unique_ptr<KingdomInfo>>& kingdoms, const bool with_highlight);

    void ChangeKingdom(const KingdomId from_kingdom_id, const KingdomId to_kingdom_id) {
        for (auto& area_row : areas_) {
            for (auto& area : area_row) {
                auto& chess = area.GetChess();
                if (chess.has_value() && chess->kingdom_id_ == from_kingdom_id) {
                    chess->kingdom_id_ = to_kingdom_id;
                }
            }
        }
    }

  private:
    static Coor GetOppoCoor_(const Coor& c) { return Coor{.m_ = k_max_m * 2 - 1 - c.m_, .n_ = k_max_n - 1 - c.n_}; }

    static bool IsValidCoor_(const Coor& c) { return 0 <= c.m_ && 0 <= c.n_ && c.m_ < k_max_m * 2 && c.n_ < k_max_n; }

    static const std::array<std::array<uint32_t, k_max_n>, k_max_m * 2> board_image_nos_;
    const KingdomId kingdom_id_;
    std::array<std::array<Area, k_max_n>, k_max_m> areas_;
    HalfBoard* oppo_board_; // should not be NULL
};

const std::array<std::array<uint32_t, HalfBoard::k_max_n>, HalfBoard::k_max_m * 2> HalfBoard::board_image_nos_{
    std::array<uint32_t, HalfBoard::k_max_n>{ 0,  4,  4, 10, 22, 11,  4,  4,  1},
    std::array<uint32_t, HalfBoard::k_max_n>{ 7, 16, 16, 27, 28, 25, 16, 16,  5},
    std::array<uint32_t, HalfBoard::k_max_n>{ 7, 17, 16, 21, 26, 20, 16, 17,  5},
    std::array<uint32_t, HalfBoard::k_max_n>{ 9, 16, 17, 16, 17, 16, 17, 16,  8},
    std::array<uint32_t, HalfBoard::k_max_n>{ 7, 14, 14, 14, 14, 14, 14, 14,  5},
    std::array<uint32_t, HalfBoard::k_max_n>{ 7, 15, 15, 15, 15, 15, 15, 15,  5},
    std::array<uint32_t, HalfBoard::k_max_n>{ 9, 16, 17, 16, 17, 16, 17, 16,  8},
    std::array<uint32_t, HalfBoard::k_max_n>{ 7, 17, 16, 18, 24, 19, 16, 17,  5},
    std::array<uint32_t, HalfBoard::k_max_n>{ 7, 16, 16, 27, 28, 25, 16, 16,  5},
    std::array<uint32_t, HalfBoard::k_max_n>{ 3,  6,  6, 13, 23, 12,  6,  6,  2},
};

struct KingdomInfo
{
    KingdomInfo(const KingdomId kingdom_id, const uint32_t player_id)
        : kingdom_id_(kingdom_id)
        , player_id_(player_id)
        , half_board_(kingdom_id)
        , state_(State::NOT_MOVED)
        , chess_count_(16)
        , eat_count_(0)
    {}

    const KingdomId kingdom_id_;
    const uint32_t player_id_;
    HalfBoard half_board_;
    enum class State { NOT_MOVED, MOVED, DESTROYED } state_;
    uint32_t chess_count_;
    uint32_t eat_count_;
};

class BoardMgr
{
  public:
    BoardMgr(const uint32_t player_num, const uint32_t kingdom_num_each_player)
    {
        assert(player_num * kingdom_num_each_player <= KingdomId::Count());
        for (uint32_t player_id = 0; player_id < player_num; ++player_id) {
            players_.emplace_back(std::make_unique<PlayerInfo>(player_id));
            for (uint32_t i = 0; i < kingdom_num_each_player; ++i) {
                kingdoms_.emplace_back(std::make_unique<KingdomInfo>(KingdomId(player_id * kingdom_num_each_player + i), player_id));
            }
            players_.back()->last_score_ = GetScore(player_id);
        }
        const auto kingdom_num = player_num * kingdom_num_each_player;
        for (uint32_t i = 0; i < kingdom_num / 2; ++i) {
            kingdom_oppo_pairs_.emplace_back(i);
        }
        for (uint32_t i = 0; i < kingdom_num / 2; ++i) {
            kingdom_oppo_pairs_.emplace_back(kingdom_num - 1 - i);
        }
        if (player_num >= 4) {
            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(kingdom_oppo_pairs_.begin(), kingdom_oppo_pairs_.end(), g);
        }
        SetOppoBoards_();
    }

    std::string Pass(const uint32_t player_id, const KingdomId kingdom_id)
    {
        if (kingdom_id.ToUInt() >= kingdoms_.size()) {
            return "该国不存在";
        }
        auto& kingdom = *kingdoms_[kingdom_id.ToUInt()];
        if (kingdom.player_id_ != player_id) {
            return "该国不受您的控制";
        }
        if (kingdom.state_ == KingdomInfo::State::DESTROYED) {
            return "该国已被毁灭";
        }
        if (kingdom.state_ == KingdomInfo::State::MOVED) {
            return "该国棋子已经行动过了";
        }
        kingdom.state_ = KingdomInfo::State::MOVED;
        return "";
    }

    std::string Move(const uint32_t player_id, const uint32_t map_id, const Coor& src, const Coor& dst)
    {
        return kingdoms_[kingdom_oppo_pairs_[map_id].ToUInt()]->half_board_.Move(player_id, src, dst, kingdoms_);
    }

    SettleResult Settle()
    {
        SettleResult result;
        for (const auto& kingdom : kingdoms_) {
            kingdom->half_board_.Settle(result);
            if (kingdom->state_ == KingdomInfo::State::MOVED) {
                kingdom->state_ = KingdomInfo::State::NOT_MOVED;
            }
        }
        std::vector<std::pair<KingdomId, KingdomId>> kingdom_changes;
        for (const auto& eat_result : result.eat_results_) {
            const auto eating_kingdom_id = eat_result.eating_chess_.kingdom_id_;
            const auto ate_kingdom_id = eat_result.ate_chess_.kingdom_id_;
            if (kingdoms_[ate_kingdom_id.ToUInt()]->state_ != KingdomInfo::State::DESTROYED) { // the ate chess is yong
                ++kingdoms_[eating_kingdom_id.ToUInt()]->eat_count_;
                --kingdoms_[ate_kingdom_id.ToUInt()]->chess_count_;
                if (eat_result.ate_chess_.chess_rule_->Type() == ChessType::JIANG) {
                    kingdom_changes.emplace_back(ate_kingdom_id, eating_kingdom_id);
                }
            }
        }
        for (const auto& [from_kingdom_id, to_kingdom_id] : kingdom_changes) {
            if (kingdoms_[to_kingdom_id.ToUInt()]->state_ == KingdomInfo::State::DESTROYED) {
                kingdoms_[from_kingdom_id.ToUInt()]->state_ = KingdomInfo::State::DESTROYED;
                kingdoms_[from_kingdom_id.ToUInt()]->chess_count_ = 0;
            } else {
                kingdoms_[from_kingdom_id.ToUInt()]->state_ = KingdomInfo::State::DESTROYED;
                kingdoms_[to_kingdom_id.ToUInt()]->chess_count_ += kingdoms_[from_kingdom_id.ToUInt()]->chess_count_;
                kingdoms_[from_kingdom_id.ToUInt()]->chess_count_ = 0;
                for (const auto& kingdom : kingdoms_) {
                    kingdom->half_board_.ChangeKingdom(from_kingdom_id, to_kingdom_id);
                }
            }
        }
        for (const auto& chess : result.crashed_chesses_) {
            --kingdoms_[chess.kingdom_id_.ToUInt()]->chess_count_;
            if (chess.chess_rule_->Type() == ChessType::JIANG) {
                kingdoms_[chess.kingdom_id_.ToUInt()]->state_ = KingdomInfo::State::DESTROYED;
                kingdoms_[chess.kingdom_id_.ToUInt()]->chess_count_ = 0;
            }
        }
        return result;
    }

    void Switch()
    {
        SwitchOppoPairs_(kingdom_oppo_pairs_);
        SetOppoBoards_();
    }

    std::vector<KingdomId> GetUnreadyKingdomIds(const uint32_t player_id) const
    {
        std::vector<KingdomId> unready_kingdom_ids;
        for (const auto& kingdom : kingdoms_) {
            if (kingdom->player_id_ == player_id && kingdom->state_ == KingdomInfo::State::NOT_MOVED) {
                unready_kingdom_ids.emplace_back(kingdom->kingdom_id_);
            }
        }
        return unready_kingdom_ids;
    }

    int32_t GetScore(const uint32_t player_id) const
    {
        int32_t score = 0;
        for (const auto& kingdom : kingdoms_) {
            if (kingdom->player_id_ == player_id) {
                score += kingdom->chess_count_ + kingdom->eat_count_;
            }
        }
        return score;
    }

    uint32_t GetChessCount(const KingdomId kingdom_id) const { return kingdoms_[kingdom_id.ToUInt()]->chess_count_; }

    std::string ToHtml() const
    {
        std::string s;
        s += PlayersHtml_();
        s += "<center>\n\n";
        for (uint32_t i = 0; i < kingdom_oppo_pairs_.size() / 2; ++i) {
            s += "<br />\n\n";
            s += "### " + std::to_string(i) + " 号棋盘\n\n";
            s += BoardHtml_(kingdom_oppo_pairs_[i], kingdom_oppo_pairs_[kingdom_oppo_pairs_.size() - 1 - i], true);
        }
        s += "\n\n</center>\n\n";
        s += "<br />\n\n";
        s += OppoInfoHtml_();
        return s;
    }

    std::string ToHtml(const KingdomId k1, const KingdomId k2) const
    {
        return k1.ToUInt() >= kingdoms_.size() || k2.ToUInt() >= kingdoms_.size() ? "" :
            PlayersHtml_() + BoardHtml_(k1, k2, false);
    }

    void SetImagePath(std::string image_path) { image_path_ = std::move(image_path); }

    void SetPlayerName(const uint32_t player_id, std::string name) { players_[player_id]->name_ = std::move(name); }

    uint32_t GetControllerPlayerID(const KingdomId kingdom_id)
    {
        assert(kingdom_id.ToUInt() < kingdoms_.size());
        return kingdoms_[kingdom_id.ToUInt()]->player_id_;
    }

  private:
    std::string JiangImage_(const KingdomId kingdom_id) const
    {
        return "![](file://" + image_path_ + "/0_jiang_" + std::to_string(kingdom_id.ToUInt()) + ".png)";
    }

    std::string ChessAndPlayerHtml_(const KingdomId kingdom_id) const
    {
        html::Table table(1, 2);
        table.SetTableStyle(" align=\"center\" cellpadding=\"3\" cellspacing=\"0\" ");
        table.Get(0, 0).SetContent(JiangImage_(kingdom_id));
        table.Get(0, 1).SetContent("**" + players_[kingdoms_[kingdom_id.ToUInt()]->player_id_]->name_ + "**");
        return table.ToString();
    }

    std::string PlayersHtml_() const
    {
        std::string s;
        for (const auto& player : players_) {
            const auto score = GetScore(player->player_id_);
            s += "### " + player->name_ + "（当前积分：" + std::to_string(player->last_score_);
            if (score < player->last_score_) {
                s += HTML_COLOR_FONT_HEADER(red) " - " + std::to_string(player->last_score_ - score) + HTML_FONT_TAIL;
            } else if (score > player->last_score_) {
                s += HTML_COLOR_FONT_HEADER(green) " + " + std::to_string(score - player->last_score_) + HTML_FONT_TAIL;
            }
            s += "）\n\n";
            player->last_score_ = score;
            html::Table table(2, 0);
            table.SetTableStyle(" align=\"center\" cellpadding=\"3\" cellspacing=\"0\" ");
            for (const auto& kingdom : kingdoms_) {
                if (kingdom->player_id_ == player->player_id_) {
                    table.AppendColumn();
                    table.MergeDown(0, table.Column() - 1, 2);
                    table.GetLastColumn(0).SetContent(JiangImage_(kingdom->kingdom_id_));
                    table.AppendColumn();
                    table.GetLastColumn(0).SetContent("剩余棋子数：" + std::to_string(kingdom->chess_count_));
                    table.GetLastColumn(1).SetContent("歼灭棋子数：" + std::to_string(kingdom->eat_count_));
                }
            }
            s += table.ToString();
            s += "\n\n";
        }
        return s;
    }

    std::string BoardHtml_(const KingdomId k1, const KingdomId k2, const bool with_highlight) const
    {
        return ChessAndPlayerHtml_(k1) +
            HalfBoard::ToHtml(kingdoms_[k1.ToUInt()]->half_board_, kingdoms_[k2.ToUInt()]->half_board_, image_path_,
                    kingdoms_, with_highlight) +
            ChessAndPlayerHtml_(k2);
    }

    std::string OppoInfoHtml_() const
    {
        std::string s;
        s += "<center>\n\n";
        s += "### 后续棋盘组合（从先到后排列）";
        s += "\n\n</center>\n\n";
        auto kingdom_oppo_pairs = kingdom_oppo_pairs_;
        html::Table table(kingdom_oppo_pairs.size() - 1, 2);
        table.SetTableStyle(" align=\"center\" cellpadding=\"3\" cellspacing=\"0\" ");
        for (uint32_t i = 0; i < kingdom_oppo_pairs.size() - 1; ++i) {
            std::string s;
            SwitchOppoPairs_(kingdom_oppo_pairs);
            for (uint32_t i = 0; i < kingdom_oppo_pairs.size() / 2; ++i) {
                s += HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE;
                s += JiangImage_(kingdom_oppo_pairs[i]);
                //s += " VS ";
                s += JiangImage_(kingdom_oppo_pairs[kingdom_oppo_pairs.size() - 1 - i]);
            }
            table.Get(i, 0).SetContent(std::to_string(i + 1) + ".");
            table.Get(i, 1).SetContent(s);
        }
        return s + table.ToString();
    }

    void SetOppoBoards_()
    {
        assert(kingdom_oppo_pairs_.size() == kingdoms_.size());
        const auto get_board = [this](const uint32_t i) -> HalfBoard& {
            return kingdoms_[kingdom_oppo_pairs_[i].ToUInt()]->half_board_;
        };
        for (uint32_t i = 0; i < kingdom_oppo_pairs_.size() / 2; ++i) {
            const auto oppo_id = kingdom_oppo_pairs_.size() - 1 - i;
            get_board(i).SetOppoBoard(get_board(oppo_id));
            get_board(oppo_id).SetOppoBoard(get_board(i));
        }
    }

    static void SwitchOppoPairs_(std::vector<KingdomId>& kingdom_oppo_pairs)
    {
        kingdom_oppo_pairs.insert(std::next(kingdom_oppo_pairs.begin()), kingdom_oppo_pairs.back());
        kingdom_oppo_pairs.pop_back();
    }

    struct PlayerInfo
    {
        PlayerInfo(const uint32_t player_id)
            : player_id_(player_id), last_score_(0), name_("Player " + std::to_string(player_id))
        {}

        const uint32_t player_id_;
        int32_t last_score_;
        std::string name_;
    };

    std::vector<std::unique_ptr<KingdomInfo>> kingdoms_;
    std::vector<std::unique_ptr<PlayerInfo>> players_;
    std::vector<KingdomId> kingdom_oppo_pairs_;
    std::string image_path_;
};

static int32_t CountChessBetween(const HalfBoard& half_board, const Coor& src, const Coor& dst)
{
    const Coor step =
        src.m_ == dst.m_ && src.n_ < dst.n_ ? Coor{.m_ = 0, .n_ = 1} :
        src.m_ == dst.m_ && src.n_ > dst.n_ ? Coor{.m_ = 0, .n_ = -1} :
        src.n_ == dst.n_ && src.m_ < dst.m_ ? Coor{.m_ = 1, .n_ = 0} :
        src.n_ == dst.n_ && src.m_ > dst.m_ ? Coor{.m_ = -1, .n_ = 0} : Coor{0, 0};
    if (step == Coor{0, 0}) {
        return -1;
    }
    int32_t count = 0;
    for (Coor coor = src + step; coor != dst; coor += step) {
        count += !half_board.Get(coor).Empty();
    }
    return count;
}

static bool IsInHouse(const Coor& c)
{
    return (c.m_ <= 2 || c.m_ >= 7) && c.n_ >= 3 && c.n_ <= 5;
}

template <typename T>
class EnableSingleton
{
  public:
    static T& Singleton()
    {
        static T obj;
        return obj;
    }
};

class JuChessRule : public ChessRule, public EnableSingleton<JuChessRule>
{
  public:
    virtual bool CanMove(const HalfBoard& half_board, const Coor& src, const Coor& dst) const override
    {
        return CountChessBetween(half_board, src, dst) == 0;
    }

    virtual ChessType Type() const override { return ChessType::JU; }

    virtual const char* Name() const override { return "che"; }

    virtual const char* ChineseName() const override { return "车"; }
};

class MaChessRule : public ChessRule, public EnableSingleton<MaChessRule>
{
  public:
    virtual bool CanMove(const HalfBoard& half_board, const Coor& src, const Coor& dst) const override
    {
        const auto m_offset = std::abs(dst.m_ - src.m_);
        const auto n_offset = std::abs(dst.n_ - src.n_);
        if ((m_offset != 1 || n_offset != 2) && (m_offset != 2 || n_offset != 1)) {
            return false;
        }
        return half_board.Get(Coor{.m_ = src.m_ - (src.m_ - dst.m_) / 2, .n_ = src.n_ - (src.n_ - dst.n_) / 2}).Empty();
    }

    virtual ChessType Type() const override { return ChessType::MA; }

    virtual const char* Name() const override { return "ma"; }

    virtual const char* ChineseName() const override { return "马"; }
};

class XiangChessRule : public ChessRule, public EnableSingleton<XiangChessRule>
{
  public:
    virtual bool CanMove(const HalfBoard& half_board, const Coor& src, const Coor& dst) const override
    {
        return std::abs(src.m_ - dst.m_) == 2 && std::abs(src.n_ - dst.n_) == 2 &&
            (src.m_ + dst.m_ ) / 2 != HalfBoard::k_max_m && (src.m_ + dst.m_ ) / 2 != HalfBoard::k_max_m - 1;
    }

    virtual ChessType Type() const override { return ChessType::XIANG; }

    virtual const char* Name() const override { return "xiang"; }

    virtual const char* ChineseName() const override { return "象"; }
};

class ShiChessRule : public ChessRule, public EnableSingleton<ShiChessRule>
{
  public:
    virtual bool CanMove(const HalfBoard& half_board, const Coor& src, const Coor& dst) const override
    {
        return IsInHouse(dst) && std::abs(src.m_ - dst.m_) == 1 && std::abs(src.n_ - dst.n_) == 1;
    }

    virtual ChessType Type() const override { return ChessType::SHI; }

    virtual const char* Name() const override { return "shi"; }

    virtual const char* ChineseName() const override { return "士"; }
};

class JiangChessRule : public ChessRule, public EnableSingleton<JiangChessRule>
{
  public:
    virtual bool CanMove(const HalfBoard& half_board, const Coor& src, const Coor& dst) const override
    {
        return IsInHouse(dst) && (
                (src.m_ == dst.m_ && std::abs(src.n_ - dst.n_) == 1) ||
                (src.n_ == dst.n_ && std::abs(src.m_ - dst.m_) == 1) ||
                (CountChessBetween(half_board, src, dst) == 0 && !half_board.Get(dst).Empty() &&
                    half_board.Get(dst).GetChess()->chess_rule_->Type() == ChessType::JIANG));
    }

    virtual ChessType Type() const override { return ChessType::JIANG; }

    virtual const char* Name() const override { return "jiang"; }

    virtual const char* ChineseName() const override { return "将"; }
};

class PaoChessRule : public ChessRule, public EnableSingleton<PaoChessRule>
{
  public:
    virtual bool CanMove(const HalfBoard& half_board, const Coor& src, const Coor& dst) const override
    {
        return CountChessBetween(half_board, src, dst) == (half_board.Get(dst).Empty() ? 0 : 1);
    }

    virtual ChessType Type() const override { return ChessType::PAO; }

    virtual const char* Name() const override { return "pao"; }

    virtual const char* ChineseName() const override { return "炮"; }
};

class ZuChessRule : public ChessRule, public EnableSingleton<ZuChessRule>
{
  public:
    virtual bool CanMove(const HalfBoard& half_board, const Coor& src, const Coor& dst) const override
    {
        return src.n_ == dst.n_ && src.m_ + (src.m_ < HalfBoard::k_max_m ? 1 : -1) == dst.m_;
    }

    virtual ChessType Type() const override { return ChessType::ZU; }

    virtual const char* Name() const override { return "zu"; }

    virtual const char* ChineseName() const override { return "卒"; }
};

class PromotedZuChessRule : public ChessRule, public EnableSingleton<PromotedZuChessRule>
{
  public:
    virtual bool CanMove(const HalfBoard& half_board, const Coor& src, const Coor& dst) const override
    {
        return (src.n_ == dst.n_ && src.m_ + (src.m_ < HalfBoard::k_max_m ? -1 : 1) == dst.m_) ||
            (src.m_ == dst.m_ && std::abs(src.n_ - dst.n_) == 1);
    }

    virtual ChessType Type() const override { return ChessType::PROMOTED_ZU; }

    virtual const char* Name() const override { return "pzu"; }

    virtual const char* ChineseName() const override { return "过河卒"; }
};

HalfBoard::HalfBoard(const KingdomId kingdom_id) : kingdom_id_(kingdom_id), oppo_board_(this)
{
    areas_[0][0].SetChess(Chess(&JuChessRule::Singleton(), kingdom_id));
    areas_[0][1].SetChess(Chess(&MaChessRule::Singleton(), kingdom_id));
    areas_[0][2].SetChess(Chess(&XiangChessRule::Singleton(), kingdom_id));
    areas_[0][3].SetChess(Chess(&ShiChessRule::Singleton(), kingdom_id));
    areas_[0][4].SetChess(Chess(&JiangChessRule::Singleton(), kingdom_id));
    areas_[0][5].SetChess(Chess(&ShiChessRule::Singleton(), kingdom_id));
    areas_[0][6].SetChess(Chess(&XiangChessRule::Singleton(), kingdom_id));
    areas_[0][7].SetChess(Chess(&MaChessRule::Singleton(), kingdom_id));
    areas_[0][8].SetChess(Chess(&JuChessRule::Singleton(), kingdom_id));
    areas_[2][1].SetChess(Chess(&PaoChessRule::Singleton(), kingdom_id));
    areas_[2][7].SetChess(Chess(&PaoChessRule::Singleton(), kingdom_id));
    areas_[3][0].SetChess(Chess(&ZuChessRule::Singleton(), kingdom_id));
    areas_[3][2].SetChess(Chess(&ZuChessRule::Singleton(), kingdom_id));
    areas_[3][4].SetChess(Chess(&ZuChessRule::Singleton(), kingdom_id));
    areas_[3][6].SetChess(Chess(&ZuChessRule::Singleton(), kingdom_id));
    areas_[3][8].SetChess(Chess(&ZuChessRule::Singleton(), kingdom_id));
}

std::string HalfBoard::Move(const uint32_t player_id, const Coor& src, const Coor& dst,
        const std::vector<std::unique_ptr<KingdomInfo>>& kingdoms)
{
    if (!IsValidCoor_(src)) {
        return src.ToString() + " 并非棋盘上的坐标";
    }
    if (!IsValidCoor_(dst)) {
        return src.ToString() + " 并非棋盘上的坐标";
    }
    auto& src_area = Get(src);
    auto& dst_area = Get(dst);
    if (!src_area.GetChess().has_value()) {
        return src.ToString() + " 位置无棋子";
    }
    auto& kingdom = *kingdoms[src_area.GetChess()->kingdom_id_.ToUInt()];
    if (kingdom.player_id_ != player_id || kingdom.state_ == KingdomInfo::State::DESTROYED) {
        return src.ToString() + " 位置棋子不受您控制";
    }
    if (kingdom.state_ == KingdomInfo::State::MOVED) {
        return "该国棋子已经行动过了";
    }
    if (!src_area.GetChess()->chess_rule_->CanMove(*this, src, dst)) {
        return "该棋子无法移动到目标位置";
    }
    if (src_area.move_state() == Area::MoveState::FREEZE) {
        return "同一棋子无法连续两回合移动";
    }
    if (dst_area.GetChess().has_value()) {
        if (dst_area.GetChess()->kingdom_id_ == src_area.GetChess()->kingdom_id_) {
            return "无法吃掉本国棋子";
        }
        if (src_area.move_state() == Area::MoveState::CANNOT_EAT) {
            return "上一回合移动的棋子无法吃子";
        }
    }
    const bool need_promote_zu = src_area.GetChess()->chess_rule_->Type() == ChessType::ZU &&
             ((src.m_ == 4 && dst.m_ == 5) || (src.m_ == 5 && dst.m_ == 4));
    Area::Move(src_area, dst_area, need_promote_zu ? &PromotedZuChessRule::Singleton() : nullptr);
    kingdom.state_ = KingdomInfo::State::MOVED;
    return "";
}

std::string HalfBoard::ToHtml(const HalfBoard& b1, const HalfBoard& b2, const std::string& image_path,
        const std::vector<std::unique_ptr<KingdomInfo>>& kingdoms, const bool with_highlight)
{
    const std::string image_prefix = "![](file://" + image_path + "/";
    const auto board_image = [&image_prefix, with_highlight](const bool highlight, const int32_t m, const int32_t n)
        {
            return image_prefix + std::to_string(highlight && with_highlight) + "_board_" +
                std::to_string(board_image_nos_[m][n]) + ".bmp)";
        };
    const auto chess_image = [&image_prefix, &kingdoms, with_highlight](const bool highlight, const Chess& chess)
        {
            std::string chess_name = kingdoms[chess.kingdom_id_.ToUInt()]->state_ == KingdomInfo::State::DESTROYED ? "yong" :
                std::string(chess.chess_rule_->Name()) + "_" + std::to_string(chess.kingdom_id_.ToUInt());
            return image_prefix + std::to_string(highlight && with_highlight) + "_" + std::move(chess_name) + ".png)";
        };
    html::Table table(k_max_m * 2 + 2, k_max_n + 2);
    table.SetTableStyle(" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" ");
    for (int32_t m = 0; m < k_max_m * 2; ++m) {
        for (int32_t n = 0; n < k_max_n; ++n) {
            const auto& area = (m < k_max_m ? b1 : *b2.oppo_board_).Get(Coor{m, n});
            const auto& chess = area.GetChess();
            const bool highlight = area.move_state() != Area::MoveState::MOVABLE;
            table.Get(m + 1, n + 1).SetContent(chess.has_value() ? chess_image(highlight, *chess) : board_image(highlight, m, n));
        }
    }
    for (int32_t m = 0; m < k_max_m * 2; ++m) {
        table.Get(m + 1, 0).SetContent(std::string(1, 'A' + m));
        table.GetLastColumn(m + 1).SetContent(std::string(1, 'A' + m));
    }
    for (int32_t n = 0; n < k_max_n; ++n) {
        table.Get(0, n + 1).SetContent(std::to_string(n));
        table.GetLastRow(n + 1).SetContent(std::to_string(n));
    }
    return "<style>html,body{color:#6b421d; background:#d8bf81;}</style>\n" + table.ToString();
}

}

#endif
