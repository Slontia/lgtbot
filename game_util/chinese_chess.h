// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <iostream>
#include <array>
#include <memory>
#include <vector>
#include <cassert>

namespace chinese_chess {

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
    uint32_t kingdom_id_;
};

struct ScoreChange
{
    uint32_t kingdom_id_;
    int32_t score_change_;
};


struct YongConvert
{
    uint32_t kingdom_id_;
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
    virtual const char* Name() const { return "某棋子"; }
};

class Area
{
  public:
    Area() : move_state_(MoveState::MOVABLE) {}

    bool Empty() const { return !chess_.has_value(); }

    const std::optional<Chess>& GetChess() const { return chess_; }

    void SetChess(const Chess& chess) { chess_ = chess; }

    void Settle(SettleResult& result)
    {
        if (move_state_ == MoveState::FREEZE) {
            move_state_ = MoveState::MOVABLE;
        } else if (move_state_ == MoveState::MOVED) {
            chess_.reset();
        }
        if (chesses_moved_here_.size() > 1) { // crashed
            result.crashed_chesses_.insert(result.crashed_chesses_.end(), chesses_moved_here_.begin(),
                    chesses_moved_here_.end());
        } else if (!chesses_moved_here_.empty()) {
            if (chess_.has_value()) {
                result.eat_results_.emplace_back(chesses_moved_here_[0], *chess_);
            }
            if (!chess_.has_value() || chess_->chess_rule_->Type() != ChessType::JIANG) {
                chess_ = chesses_moved_here_[0];
            }
            move_state_ = MoveState::FREEZE;
        }
        chesses_moved_here_.clear();
    }

    static std::string Move(Area& src, Area& dst, ChessRule* const chess_rule = nullptr)
    {
        assert(src.chess_.has_value());
        if (src.move_state_ == MoveState::FREEZE) {
            return "您无法连续两回合移动同一棋子";
        }
        src.move_state_ = MoveState::MOVED;
        dst.chesses_moved_here_.emplace_back(chess_rule == nullptr ? src.chess_->chess_rule_ : chess_rule,
                src.chess_->kingdom_id_);
        return "";
    }

  private:
    enum class MoveState { FREEZE, MOVABLE, MOVED } move_state_;
    std::optional<Chess> chess_;
    std::vector<Chess> chesses_moved_here_;
};

class KingdomInfo;

class HalfBoard
{
  public:
    const constexpr static int32_t k_max_m = 5;
    const constexpr static int32_t k_max_n = 9;

    HalfBoard(const uint32_t kingdom_id);

    const Area& Get(const Coor& c) const
    {
        return c.m_ < k_max_m ? areas_[c.m_][c.n_] : oppo_board_->Get(GetOppoCoor_(c));
    }

    Area& Get(const Coor& c)
    {
        return c.m_ < k_max_m ? areas_[c.m_][c.n_] : oppo_board_->Get(GetOppoCoor_(c));
    }

    std::string Move(const uint32_t kingdom_id, const Coor& src, const Coor& dst,
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

  private:
    static Coor GetOppoCoor_(const Coor& c) { return Coor{.m_ = k_max_m * 2 - 1 - c.m_, .n_ = k_max_n - 1 - c.n_}; }

    const uint32_t kingdom_id_;
    std::array<std::array<Area, k_max_n>, k_max_m> areas_;
    HalfBoard* oppo_board_; // should not be NULL
};

struct KingdomInfo
{
    KingdomInfo(const uint32_t kingdom_id)
        : kingdom_id_(kingdom_id), half_board_(kingdom_id), moved_(false), chess_count_(16)
    {}

    const uint32_t kingdom_id_;
    HalfBoard half_board_;
    std::optional<uint32_t> player_id_;
    bool moved_;
    uint32_t chess_count_;
};

class BoardMgr
{
  public:
    BoardMgr(const uint32_t player_num, const uint32_t kingdom_num_each_player)
    {
        for (uint32_t player_id = 0; player_id < player_num; ++player_id) {
            players_.emplace_back(std::make_unique<PlayerInfo>(player_id));
            for (uint32_t i = 0; i < kingdom_num_each_player; ++i) {
                kingdoms_.emplace_back(std::make_unique<KingdomInfo>(player_id * kingdom_num_each_player + i));
                kingdoms_.back()->player_id_ = player_id;
            }
        }
        const auto kingdom_num = player_num * kingdom_num_each_player;
        for (uint32_t i = 0; i < kingdom_num / 2; ++i) {
            kingdom_oppo_pairs_.emplace_back(i);
        }
        for (uint32_t i = 0; i < kingdom_num / 2; ++i) {
            kingdom_oppo_pairs_.emplace_back(kingdom_num - 1 - i);
        }
        Switch();
    }

    std::string Move(const uint32_t player_id, const uint32_t map_id, const Coor& src, const Coor& dst)
    {
        return kingdoms_[map_id]->half_board_.Move(player_id, src, dst, kingdoms_);
    }

    SettleResult Settle()
    {
        SettleResult result;
        for (const auto& kingdom : kingdoms_) {
            kingdom->half_board_.Settle(result);
            kingdom->moved_ = false;
        }
        for (const auto& eat_result : result.eat_results_) {
            const auto eating_kingdom_id = eat_result.eating_chess_.kingdom_id_;
            const auto ate_kingdom_id = eat_result.ate_chess_.kingdom_id_;
            const auto eating_player_id = kingdoms_[eating_kingdom_id]->player_id_;
            const auto ate_player_id = kingdoms_[ate_kingdom_id]->player_id_;
            assert(eating_player_id.has_value());
            if (!ate_player_id.has_value()) { // the ate chess is yong
                --kingdoms_[ate_kingdom_id]->chess_count_;
                //std::cout << "a";
            } else if (eat_result.ate_chess_.chess_rule_->Type() == ChessType::JIANG) {
                const auto chess_count = kingdoms_[ate_kingdom_id]->chess_count_;
                players_[*eating_player_id]->score_ += chess_count;
                players_[*ate_player_id]->score_ -= chess_count;
                --kingdoms_[eating_kingdom_id]->chess_count_; // eating chess become new jiang
                kingdoms_[ate_kingdom_id]->player_id_ = eating_player_id;
                if (eat_result.eating_chess_.chess_rule_->Type() == ChessType::JIANG) {
                    kingdoms_[eating_kingdom_id]->player_id_ = std::nullopt; // eating jiang's kingdom is destroyed
                }
                //std::cout << "b";
            } else {
                ++players_[*eating_player_id]->score_;
                --players_[*ate_player_id]->score_;
                --kingdoms_[ate_kingdom_id]->chess_count_;
                //std::cout << "c";
            }
        }
        for (const auto& chess : result.crashed_chesses_) {
            --kingdoms_[chess.kingdom_id_]->chess_count_;
            if (chess.chess_rule_->Type() == ChessType::JIANG) {
                kingdoms_[chess.kingdom_id_]->player_id_ = std::nullopt;
            }
        }
        //std::cout << std::endl;
        return result;
    }

    void Switch()
    {
        assert(kingdom_oppo_pairs_.size() == kingdoms_.size());
        const auto get_board = [this](const uint32_t i) -> HalfBoard& {
            return kingdoms_[kingdom_oppo_pairs_[i]]->half_board_;
        };
        for (uint32_t i = 0; i < kingdom_oppo_pairs_.size() / 2; ++i) {
            get_board(i).SetOppoBoard(get_board(kingdom_oppo_pairs_.size() - 1 - i));
        }
        const uint32_t last_kingdom_id = kingdom_oppo_pairs_.back();
        kingdom_oppo_pairs_.pop_back();
        kingdom_oppo_pairs_.insert(std::next(kingdom_oppo_pairs_.begin()), last_kingdom_id);
    }

    std::vector<uint32_t> GetUnreadyKingdomIds(const uint32_t player_id) const
    {
        std::vector<uint32_t> unready_kingdom_ids;
        for (const auto& kingdom : kingdoms_) {
            if (kingdom->player_id_ == player_id && !kingdom->moved_) {
                unready_kingdom_ids.emplace_back(kingdom->kingdom_id_);
            }
        }
        return unready_kingdom_ids;
    }

    int32_t GetScore(const uint32_t player_id) const { return players_[player_id]->score_; }

    uint32_t GetChessCount(const uint32_t kingdom_id) const { return kingdoms_[kingdom_id]->chess_count_; }

  private:
    struct PlayerInfo
    {
        PlayerInfo(const uint32_t player_id)
            : player_id_(player_id), score_(0)
        {}

        const uint32_t player_id_;
        int32_t score_;
    };

    std::vector<std::unique_ptr<KingdomInfo>> kingdoms_;
    std::vector<std::unique_ptr<PlayerInfo>> players_;
    std::vector<uint32_t> kingdom_oppo_pairs_;
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
        return half_board.Get(Coor{.m_ = (src.m_ + dst.m_) / 2, .n_ = (src.n_ + dst.n_) / 2}).Empty();
    }

    virtual ChessType Type() const override { return ChessType::MA; }
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
};

class ShiChessRule : public ChessRule, public EnableSingleton<ShiChessRule>
{
  public:
    virtual bool CanMove(const HalfBoard& half_board, const Coor& src, const Coor& dst) const override
    {
        return IsInHouse(dst) && std::abs(src.m_ - dst.m_) == 1 && std::abs(src.n_ - dst.n_) == 1;
    }

    virtual ChessType Type() const override { return ChessType::SHI; }
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
};

class PaoChessRule : public ChessRule, public EnableSingleton<PaoChessRule>
{
  public:
    virtual bool CanMove(const HalfBoard& half_board, const Coor& src, const Coor& dst) const override
    {
        return CountChessBetween(half_board, src, dst) == (half_board.Get(dst).Empty() ? 0 : 1);
    }

    virtual ChessType Type() const override { return ChessType::PAO; }
};

class ZuChessRule : public ChessRule, public EnableSingleton<ZuChessRule>
{
  public:
    virtual bool CanMove(const HalfBoard& half_board, const Coor& src, const Coor& dst) const override
    {
        return src.n_ == dst.n_ && src.m_ + (src.m_ < HalfBoard::k_max_m ? 1 : -1) == dst.m_;
    }

    virtual ChessType Type() const override { return ChessType::ZU; }
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
};

HalfBoard::HalfBoard(const uint32_t kingdom_id) : kingdom_id_(kingdom_id), oppo_board_(this)
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
    auto& src_area = Get(src);
    auto& dst_area = Get(dst);
    if (!src_area.GetChess().has_value() || kingdoms[src_area.GetChess()->kingdom_id_]->player_id_ != player_id) {
        return src.ToString() + " 位置无本方棋子";
    }
    if (kingdoms[src_area.GetChess()->kingdom_id_]->moved_) {
        return "该阵营棋子已经行动过了";
    }
    if (!src_area.GetChess()->chess_rule_->CanMove(*this, src, dst)) {
        return "您无法移动棋子到目标位置";
    }
    if (dst_area.GetChess().has_value() && dst_area.GetChess()->kingdom_id_ == src_area.GetChess()->kingdom_id_) {
        return "您无法吃掉本方棋子";
    }
    const bool need_promote_zu = src_area.GetChess()->chess_rule_->Type() == ChessType::ZU &&
             ((src.m_ == 4 && dst.m_ == 5) || (src.m_ == 5 && dst.m_ == 4));
    if (const auto errstr = Area::Move(src_area, dst_area, need_promote_zu ? &PromotedZuChessRule::Singleton() : nullptr);
            !errstr.empty()) {
        return errstr;
    }
    kingdoms[src_area.GetChess()->kingdom_id_]->moved_ = true;
    return "";
}

}
