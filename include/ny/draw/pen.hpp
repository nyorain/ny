#pragma once

#include <ny/draw/include.hpp>
#include <ny/draw/brush.hpp>

namespace ny
{

///The DashStyle class represnts a pen dash style.
///The points member does hold an array of positive float values. If the array is e.g. {2.0, 1.0}
///it menas that 2 points of the line are drawn, then 1 is not drawn and it continues from the
///beginning. The size member holds the size of valued specified in points.
///TODO: think about pointer ownership, unique_ptr, raw pointer, potential_ptr?
struct DashStyle
{
    float* points = nullptr;
    std::size_t size = 0;
};

///The Pen class represnts a style to draw lines (stroke shapes).
class Pen
{
public:
    static const Pen none;

protected:
    float width_ = 0;
    DashStyle dashStyle_;
    Brush brush_;

public:
    Pen() = default;
    Pen(const Brush& brush, float width = 0, const DashStyle& style = DashStyle());

    void width(float width){ width_ = width; }
    float width() const { return width_; }

    void dashStyle(const DashStyle& style){ dashStyle_ = style; }
    const DashStyle& dashStyle() const { return dashStyle_; }

    void brush(const Brush& b){ brush_ = b; }
    const Brush& brush() const { return brush_; }
};

}
