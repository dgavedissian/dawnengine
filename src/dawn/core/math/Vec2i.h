/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2018 (git@dga.me.uk)
 */
#pragma once
#include "Defs.h"

namespace dw {

class DW_API Vec2i {
public:
    int x, y;

    /// Constructors
    Vec2i();
    Vec2i(int _x, int _y);
    Vec2i(const Vec2& v);

    /// Operators
    Vec2i& operator=(const Vec2i& other);
    Vec2i& operator+=(const Vec2i& other);
    Vec2i& operator-=(const Vec2i& other);
    Vec2i& operator*=(int scalar);
    Vec2i& operator/=(int scalar);
    Vec2i& operator*=(float scalar);
    Vec2i& operator/=(float scalar);
    Vec2i& operator*=(const Vec2& other);
    Vec2i& operator/=(const Vec2& other);
    Vec2i operator-() const;
    Vec2i operator+(const Vec2i& other) const;
    Vec2i operator-(const Vec2i& other) const;
    Vec2i operator*(int scalar) const;
    Vec2i operator/(int scalar) const;
    Vec2i operator*(float scalar) const;
    Vec2i operator/(float scalar) const;
    Vec2i operator*(const Vec2& other) const;
    Vec2i operator/(const Vec2& other) const;
};
}  // namespace dw