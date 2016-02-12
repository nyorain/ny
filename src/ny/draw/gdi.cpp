#include <ny/draw/gdi.hpp>

namespace ny
{

GdiDrawContext::GdiDrawContext(Graphics& graphics) : graphics_(graphics)
{
}

/*
void gdiDrawContext::clear(color col)
{
    if(!painting_)
        return;

    graphics_->Clear(colorToWinapi(col));
}

//
void gdiDrawContext::mask(const Rectangle& obj)
{
    currentPath_.AddRectangle(Rect(obj.getPosition().x, obj.getPosition().y, obj.getSize().x, obj.getSize().y));
}

void gdiDrawContext::mask(const customPath& obj)
{
}

void gdiDrawContext::mask(const text& obj)
{
}

void gdiDrawContext::resetMask()
{
    currentPath_.Reset();
}

void gdiDrawContext::fillPreserve(const brush& col)
{
    SolidBrush bb(Color(col.a, col.r, col.g, col.b));
    graphics_->FillPath(&bb, &currentPath_);
}

void gdiDrawContext::fill(const brush& col)
{
    SolidBrush bb(Color(col.a, col.r, col.g, col.b));
    graphics_->FillPath(&bb, &currentPath_);

    resetMask();
}
*/

}
