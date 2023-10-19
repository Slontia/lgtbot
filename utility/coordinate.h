// Copyright (c) 2023-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

struct Coordinate
{
    Coordinate& operator+=(const Coordinate coordinate)
    {
        row_ += coordinate.row_;
        col_ += coordinate.col_;
        return *this;
    }

    Coordinate& operator-=(const Coordinate coordinate)
    {
        row_ -= coordinate.row_;
        col_ -= coordinate.col_;
        return *this;
    }

    Coordinate& operator*=(const int32_t n)
    {
        row_ *= n;
        col_ *= n;
        return *this;
    }

    int32_t row_;
    int32_t col_;
};

inline Coordinate operator+(Coordinate _1, const Coordinate _2) {
    return _1 += _2;
}

inline Coordinate operator-(Coordinate _1, const Coordinate _2) {
    return _1 -= _2;
}

inline Coordinate operator*(Coordinate coordinate, const int32_t n) {
    return coordinate *= n;
}
