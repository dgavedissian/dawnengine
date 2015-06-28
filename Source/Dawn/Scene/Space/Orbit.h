/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2015 (avedissian.david@gmail.com)
 */
#pragma once

NAMESPACE_BEGIN

// Interface for different orbit types
class DW_API Orbit
{
public:
    virtual ~Orbit(){};

    // Calculate a position given a time
    virtual Position CalculatePosition(double time) = 0;
};

// Circular Orbit
// e = 0
class DW_API CircularOrbit : public Orbit
{
public:
    CircularOrbit(float radius, float period, float phaseDifference = 0.0f);
    virtual ~CircularOrbit();

    virtual Position CalculatePosition(double time) override;

private:
    float mRadius;
    float mPeroid;
    float mPhaseDiff;
};

// Elliptical Orbit
// 0 < e < 1

// Parabolic
// e = 1

// Hyperbolic
// e > 1

NAMESPACE_END
