/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2015 (avedissian.david@gmail.com)
 */
#pragma once

NAMESPACE_BEGIN

class Rect
{
public:
    Vec2i begin, end;

    /// Constructors
    Rect();
    Rect(int x1, int y1, int x2, int y2);
    Rect(const Vec2i& _begin, const Vec2i& _end);
    
    /// Merge a rect into this one
    Rect& Merge(const Rect& other);

};

NAMESPACE_END
