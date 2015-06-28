/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2015 (avedissian.david@gmail.com)
 */
#pragma once

NAMESPACE_BEGIN

class Vec3i
{
public:
    int x, y, z;

    /// Constructors
    Vec3i();
    Vec3i(int _x, int _y, int _z);
    Vec3i(const Vec3& v);

    /// Operators
    Vec3i& operator=(const Vec3i& other);
    Vec3i& operator+=(const Vec3i& other);
    Vec3i& operator-=(const Vec3i& other);
    Vec3i& operator*=(int scalar);
    Vec3i& operator/=(int scalar);
    const Vec3i operator-() const;
    const Vec3i operator+(const Vec3i& other) const;
    const Vec3i operator-(const Vec3i& other) const;
    const Vec3i operator*(int scalar) const;
    const Vec3i operator/(int scalar) const;
};

NAMESPACE_END
