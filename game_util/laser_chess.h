// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <cassert>
#include <optional>
#include <variant>
#include <vector>
#include <array>
#include <bitset>
#include <iostream>

#include "utility/html.h"

namespace laser {

enum Direct : int { UP = 0, RIGHT = 1, DOWN = 2, LEFT = 3 };

static int ClockWise(const int direct) { return (direct + 1) % 4; }
static int Opposite(const int direct) { return (direct + 2) % 4; }
static int AntiClockWise(const int direct) { return (direct + 3) % 4; }
static int RotateDirect(const int direct, const bool is_clock_wise) { return is_clock_wise ? ClockWise(direct) : AntiClockWise(direct); }

struct Coor
{
    int32_t m_ = -1;
    int32_t n_ = -1;
};

Coor operator+(const Coor& _1, const Coor& _2)
{
    return Coor{_1.m_ + _2.m_, _1.n_ + _2.n_};
}

static Coor DirectMove(const int direct)
{
    return direct == UP    ? Coor{-1, 0}  :
           direct == RIGHT ? Coor{0, 1}  :
           direct == DOWN  ? Coor{1, 0} :
           direct == LEFT  ? Coor{0, -1} : Coor{0, 0};
}

struct LaserResult
{
    std::bitset<4> next_directs_;
    bool is_dead_ = false;
};

static std::string bool_to_rb(const bool b) { return b ? "r" : "b"; }
static std::string direct_to_str(const int direct)
{
    static constexpr std::array<const char*, 4> k_strs = { "u", "r", "d", "l" };
    return k_strs[direct];
}

class EmptyChess
{
  public:
    LaserResult HandleLaser(const int direct) const { return LaserResult{std::bitset<4>().set(direct), false}; }

    bool Rotate(const bool) { return false; }

    bool CanMove() const { return true; }

    bool IsMyChess(const bool pid) const { return false; }

    std::string Image(const std::bitset<4>& laser_tracker) const
    {
        return "empty_" + std::to_string(laser_tracker.test(UP) || laser_tracker.test(DOWN)) +
            std::to_string(laser_tracker.test(LEFT) || laser_tracker.test(RIGHT));
    }

    std::string Image() const { return "empty"; }
};

template <bool k_pid_>
class PlayerChessBase
{
  public:
    PlayerChessBase() {}

    bool IsMyChess(const bool pid) const { return !(pid ^ k_pid_); }
};

template <bool k_pid_>
class KingChess : public PlayerChessBase<k_pid_>
{
  public:
    KingChess() {}

    LaserResult HandleLaser(const int direct) const { return LaserResult{{}, true}; }

    bool Rotate(const bool) { return false; }

    bool CanMove() const { return true; }

    std::string Image(const std::bitset<4>& laser_tracker) const
    {
        return "king_" + bool_to_rb(k_pid_);
    }
};

template <bool k_pid_>
class ShieldChess : public PlayerChessBase<k_pid_>
{
  public:
    ShieldChess(const int direct) : direct_(direct) {}

    LaserResult HandleLaser(const int direct) const
    {
        if (direct == Opposite(direct_)) {
            return LaserResult{std::bitset<4>().set(direct_), false};
        } else {
            return LaserResult{{}, true};
        }
    }

    bool Rotate(const bool is_clock_wise)
    {
        direct_ = RotateDirect(direct_, is_clock_wise);
        return true;
    }

    bool CanMove() const { return true; }

    std::string Image(const std::bitset<4>& laser_tracker) const
    {
        return "shield_" + bool_to_rb(k_pid_) + "_" + direct_to_str(direct_);
    }

  private:
    int direct_;
};

template <bool k_pid_>
class ShooterChess : public PlayerChessBase<k_pid_>
{
  public:
    ShooterChess(const int direct, const std::bitset<4>& avaliable_directs)
        : direct_(direct), avaliable_directs_(avaliable_directs) {}

    LaserResult HandleLaser(const int) const { return LaserResult{std::bitset<4>().set(direct_), false}; }

    bool Rotate(const bool is_clock_wise)
    {
        const auto new_direct = RotateDirect(direct_, is_clock_wise);
        if (avaliable_directs_.test(new_direct)) {
            direct_ = new_direct;
            return true;
        }
        return false;
    }

    bool CanMove() const { return false; }

    std::string Image(const std::bitset<4>& laser_tracker) const
    {
        return "shooter_" + bool_to_rb(k_pid_) + "_" + direct_to_str(direct_) + "_" + std::to_string(laser_tracker.any());
    }

    int Direct() const { return direct_; }

  private:
    int direct_;
    std::bitset<4> avaliable_directs_;
};

template <bool k_pid_>
class SingleMirrorChess : public PlayerChessBase<k_pid_>
{
  public:
    SingleMirrorChess(const int direct) : direct_(direct) {}

    LaserResult HandleLaser(const int direct) const
    {
        if (direct_ == direct) {
            return LaserResult{std::bitset<4>().set(ClockWise(direct)), false};
        } else if (direct_ == ClockWise(direct)) {
            return LaserResult{std::bitset<4>().set(AntiClockWise(direct)), false};
        } else {
            return LaserResult{{}, true};
        }
    }

    bool Rotate(const bool is_clock_wise)
    {
        direct_ = RotateDirect(direct_, is_clock_wise);
        return true;
    }

    bool CanMove() const { return true; }

    std::string Image(const std::bitset<4>& laser_tracker) const
    {
        return "single_" + bool_to_rb(k_pid_) + "_" + direct_to_str(direct_) + "_" +
            std::to_string(laser_tracker.test(direct_) || laser_tracker.test(AntiClockWise(direct_)));
    }

  private:
    int direct_;
};

template <bool k_pid_>
class DoubleMirrorChess : public PlayerChessBase<k_pid_>
{
  public:
    DoubleMirrorChess(const bool is_left_bottom) : is_left_bottom_(is_left_bottom) {}

    LaserResult HandleLaser(const int direct) const
    {
        if ((direct == UP || direct == DOWN) ^ is_left_bottom_) {
            return LaserResult{std::bitset<4>().set(AntiClockWise(direct)), false};
        } else {
            return LaserResult{std::bitset<4>().set(ClockWise(direct)), false};
        }
    }

    bool Rotate(const bool is_clock_wise)
    {
        is_left_bottom_ = !is_left_bottom_;
        return true;
    }

    bool CanMove() const { return true; }

    std::string Image(const std::bitset<4>& laser_tracker) const
    {
        return "double_" + bool_to_rb(k_pid_) + "_" + std::to_string(is_left_bottom_) + "_" +
            std::to_string(laser_tracker.test(DOWN) || (laser_tracker.test(RIGHT) && is_left_bottom_) || (laser_tracker.test(LEFT) && !is_left_bottom_)) +
            std::to_string(laser_tracker.test(UP) || (laser_tracker.test(LEFT) && is_left_bottom_) || (laser_tracker.test(RIGHT) && !is_left_bottom_));
    }

  private:
    bool is_left_bottom_;
};

template <bool k_pid_>
class LensedMirrorChess : public PlayerChessBase<k_pid_>
{
  public:
    LensedMirrorChess(const bool is_left_bottom) : is_left_bottom_(is_left_bottom) {}

    LaserResult HandleLaser(const int direct) const
    {
        if ((direct == UP || direct == DOWN) ^ is_left_bottom_) {
            return LaserResult{std::bitset<4>().set(AntiClockWise(direct)).set(direct), false};
        } else {
            return LaserResult{std::bitset<4>().set(ClockWise(direct)).set(direct), false};
        }
    }

    bool Rotate(const bool is_clock_wise)
    {
        is_left_bottom_ = !is_left_bottom_;
        return true;
    }

    bool CanMove() const { return true; }

    std::string Image(const std::bitset<4>& laser_tracker) const
    {
        const std::string type = laser_tracker.none()       ? "x" :
                                 laser_tracker.count() >= 2 ? "f" :
                                 laser_tracker.test(UP)     ? "u" :
                                 laser_tracker.test(LEFT)   ? "l" :
                                 laser_tracker.test(RIGHT)  ? "r" :
                                 laser_tracker.test(DOWN)   ? "d" : "?";
        return "lensed_" + bool_to_rb(k_pid_) + "_" + std::to_string(is_left_bottom_) + "_" + type;
    }

  private:
    bool is_left_bottom_;
};

using VariantChess = std::variant<EmptyChess,
        KingChess<true>, SingleMirrorChess<true>, DoubleMirrorChess<true>, LensedMirrorChess<true>, ShooterChess<true>, ShieldChess<true>,
        KingChess<false>, SingleMirrorChess<false>, DoubleMirrorChess<false>, LensedMirrorChess<false>, ShooterChess<false>, ShieldChess<false>
    >;

struct SettleResult
{
    std::array<uint32_t, 2> king_alive_num_ = {0};
    std::array<uint32_t, 2> chess_dead_num_ = {0};
    bool crashed_ = false;
    std::string html_;
};

class Area
{
  public:
    Area() : chess_(std::in_place_type<EmptyChess>), is_dead_(false), state_(Area::IDL) {}

    // Return 1 if the player's king is dead
    void Settle(SettleResult& result)
    {
        laser_tracker_.reset();
        result.crashed_ |= state_ == Area::DST && Empty();
        state_ = Area::IDL;
        if (!is_dead_) {
            if (auto* const king_chess = std::get_if<KingChess<true>>(&chess_)) {
                ++result.king_alive_num_[true];
            } else if (auto* const king_chess = std::get_if<KingChess<false>>(&chess_)) {
                ++result.king_alive_num_[false];
            }
            return;
        }
        result.chess_dead_num_[IsMyChess(true)] = 1;
        chess_ = EmptyChess();
        is_dead_ = false;
        return;
    }

    bool IsMyChess(const bool pid) const
    {
        return std::visit([pid](const auto& chess) { return chess.IsMyChess(pid); }, chess_);
    }

    bool CanMove() const
    {
        return std::visit([](const auto& chess) { return chess.CanMove(); }, chess_);
    }

    bool Rotate(const bool is_clock_wise)
    {
        return std::visit([is_clock_wise](auto& chess) { return chess.Rotate(is_clock_wise); }, chess_);
    }

    std::bitset<4> HandleLaser(const int direct)
    {
        if (laser_tracker_.test(direct)) {
            return {};
        }
        laser_tracker_.set(direct);
        const auto result = std::visit([direct](const auto& chess) { return chess.HandleLaser(direct); }, chess_);
        is_dead_ |= result.is_dead_;
        return result.next_directs_;
    }

    std::string Image() const
    {
        return std::visit([&](const auto& chess) { return chess.Image(laser_tracker_); }, chess_);
    }

    bool Empty() const { return std::get_if<EmptyChess>(&chess_) != nullptr; }

    template <typename T>
    void SetChess(T&& chess) { chess_ = std::forward<T>(chess); }

    template <typename T>
    const T& GetChess() const { return std::get<T>(chess_); }

    template <typename T>
    bool CheckType() const { return std::get_if<T>(&chess_); }

    enum State { IDL, SRC, DST };

    State SetState(const State state)
    {
        return std::exchange(state_, state);
    }

    State GetState() const { return state_; }

    static void SwapChess(Area& _1, Area& _2) { std::swap(_1.chess_, _2.chess_); }

  private:
    VariantChess chess_;
    std::bitset<4> laser_tracker_;
    bool is_dead_;
    State state_;
};

auto coor_to_str (const Coor& coor) { return char('A' + coor.m_) + std::to_string(coor.n_); };

class Board
{
  public:
    Board(const uint32_t max_m, const uint32_t max_n, std::string image_path)
        : max_m_(max_m)
        , max_n_(max_n)
        , image_path_(std::move(image_path))
        , areas_(max_m * max_n)
        , chess_count_{0}
        , is_shooting_(false)
    {
        assert(max_m > 0);
        assert(max_n > 0);
    }

    Board(Board&&) = default;

    bool IsMyChess(const Coor& coor, const bool pid) const
    {
        return GetArea_(coor).IsMyChess(pid);
    }

    bool IsEmpty(const Coor& coor) const
    {
        return GetArea_(coor).Empty();
    }

    template <typename T>
    inline bool SetChess(const Coor& coor, T&& chess)
    {
        constexpr const bool is_shooter =
            std::is_same_v<std::decay_t<T>, ShooterChess<true>> || std::is_same_v<std::decay_t<T>, ShooterChess<false>>;
        const bool pid = chess.IsMyChess(1);
        auto& shooter_pos = shooter_pos_[pid];
        assert(!is_shooting_);
        auto& area = GetArea_(coor);
        if (!area.Empty() || (is_shooter && IsValidCoor(shooter_pos))) {
            return false;
        }
        area.SetChess(std::forward<T>(chess));
        ++chess_count_[pid];
        if (is_shooter) {
            shooter_pos = coor;
        }
        return true;
    }

    void DefaultBehavior(const bool pid)
    {
        Rotate(shooter_pos_[pid], true, pid).empty() || Rotate(shooter_pos_[pid], false, pid).empty();
    }

    std::string Move(const Coor& src, const Coor& dst, const bool pid)
    {
        assert(!is_shooting_);
        if (!IsValidCoor(src)) {
            return std::string("位置 ") + coor_to_str(src) + " 并不位于棋盘上";
        }
        if (!IsValidCoor(dst)) {
            return "您无法将棋子移动至棋盘外";
        }
        auto& src_area = GetArea_(src);
        auto& dst_area = GetArea_(dst);
        if (!src_area.IsMyChess(pid) || src_area.GetState() != Area::IDL) {
            return std::string("移动前位置 ") + coor_to_str(src) + " 上无可移动的本方棋子";
        }
        if (IsNearbyKing(src)) {
            return std::string("移动前位置 ") + coor_to_str(src) + " 与王相邻，无法移动";
        }
        if (!src_area.CanMove()) {
            return std::string("移动前位置 ") + coor_to_str(src) + " 上的棋子无法被移动";
        }
        if (dst_area.GetState() == Area::DST) { // crash
            src_area.SetChess(EmptyChess());
            src_area.SetState(Area::SRC);
            dst_area.SetChess(EmptyChess());
            return "";
        }
        if (dst_area.IsMyChess(1 - pid) || dst_area.GetState() != Area::IDL) {
            return std::string("移动后位置 ") + coor_to_str(src) + " 上无可移动的本方棋子，故无法交换棋子位置";
        }
        if (!dst_area.Empty()) {
            if (IsNearbyKing(dst)) {
                return std::string("移动后位置 ") + coor_to_str(src) + " 与王相邻，无法移动，故无法交换棋子位置";
            }
            if (!src_area.CanMove()) {
                return std::string("移动后位置 ") + coor_to_str(src) + " 上的棋子无法被移动，故无法交换棋子位置";
            }
        }
        src_area.SetState(Area::SRC);
        dst_area.SetState(dst_area.Empty() ? Area::DST : Area::SRC);
        Area::SwapChess(dst_area, src_area);
        return "";
    }

    std::string Rotate(const Coor& coor, const bool is_clock_wise, const bool pid)
    {
        assert(!is_shooting_);
        if (!IsValidCoor(coor)) {
            return std::string("位置 ") + coor_to_str(coor) + " 并不位于棋盘上";
        }
        if (IsNearbyKing(coor)) {
            return std::string("位置") + coor_to_str(coor) + " 与王相邻，无法旋转";
        }
        auto& area = GetArea_(coor);
        if (!area.IsMyChess(pid) || area.GetState() != Area::IDL) {
            return std::string("位置 ") + coor_to_str(coor) + " 上无可旋转的本方棋子";
        }
        if (!area.Rotate(is_clock_wise)) {
            return std::string("位置 ") + coor_to_str(coor) + " 上的棋子无法被如此旋转";
        }
        area.SetState(Area::SRC);
        return "";
    }

    uint32_t ChessCount(const bool pid) const { return chess_count_[pid]; }

    SettleResult Settle()
    {
        is_shooting_ = true;
        RecursiveHandleLaser_(shooter_pos_[0], UP);
        RecursiveHandleLaser_(shooter_pos_[1], UP);

        assert(is_shooting_);
        is_shooting_ = false;
        SettleResult result {0};

        result.html_ += ToHtml();

        for (auto& area : areas_) {
            area.Settle(result);
        }
        chess_count_[0] -= result.chess_dead_num_[0] + result.crashed_;
        chess_count_[1] -= result.chess_dead_num_[1] + result.crashed_;

        result.html_ += "<br />\n\n" + ToHtml();

        return result;
    }

    bool IsValidCoor(const Coor& coor) const
    {
        return coor.m_ >= 0 && coor.n_ >= 0 && coor.m_ < max_m_ && coor.n_ < max_n_;
    }

    std::string ToHtml() const
    {
        html::Table table(max_m_ + 2, max_n_ + 2);
        table.SetTableStyle(" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" ");
        for (int32_t m = 1; m <= max_m_; ++m) {
            for (int32_t n = 1; n <= max_n_; ++n) {
                const auto& area = GetArea_(Coor{m - 1, n - 1});
                table.Get(m, n).SetColor(area.GetState() != Area::IDL ?
                        ((m % 2) ^ (n % 2) ? "eade00" : "fff860") : ((m % 2) ^ (n % 2) ? "#bcbcbc" : "d4d4d4"));
                table.Get(m, n).SetContent("![](file://" + image_path_ + "/" + area.Image() + ".png)");
            }
        }
        for (int32_t m = 0; m < max_m_; ++m) {
            table.Get(m + 1, 0).SetContent(std::string(1, 'A' + m));
            table.GetLastColumn(m + 1).SetContent(std::string(1, 'A' + m));
        }
        for (int32_t n = 0; n < max_n_; ++n) {
            table.Get(0, n + 1).SetContent(std::to_string(n));
            table.GetLastRow(n + 1).SetContent(std::to_string(n));
        }
        return table.ToString();
    }

    uint32_t max_m() const { return max_m_; }
    uint32_t max_n() const { return max_n_; }

  private:
    bool IsNearbyKing(const Coor& coor) const
    {
        for (int32_t m = coor.m_ - 1; m <= coor.m_ + 1; ++m) {
            for (int32_t n = coor.n_ - 1; n <= coor.n_ + 1; ++n) {
                const Coor cur_coor{.m_ = m, .n_ = n};
                if ((m != coor.m_ || n != coor.n_) && IsValidCoor(cur_coor) &&
                        (GetArea_(cur_coor).CheckType<KingChess<0>>() || GetArea_(cur_coor).CheckType<KingChess<1>>())) {
                    return true;
                }
            }
        }
        return false;
    }

    void RecursiveHandleLaser_(const Coor& coor, const int direct)
    {
        if (!IsValidCoor(coor)) {
            return;
        }
        const auto next_directs = GetArea_(coor).HandleLaser(direct);
        for (const Direct next_direct : {UP, RIGHT, DOWN, LEFT}) {
            if (next_directs.test(next_direct)) {
                RecursiveHandleLaser_(coor + DirectMove(next_direct), next_direct);
            }
        }
    }

    Area& GetArea_(const Coor& coor) { return areas_[coor.m_ * max_n_ + coor.n_]; }
    const Area& GetArea_(const Coor& coor) const { return areas_[coor.m_ * max_n_ + coor.n_]; }

    const uint32_t max_m_;
    const uint32_t max_n_;
    const std::string image_path_;
    std::vector<Area> areas_;
    std::array<Coor, 2> shooter_pos_;
    std::array<uint32_t, 2> chess_count_;
    bool is_shooting_;
};

};
