#include <ny/winapi/gdiDrawContext.hpp>

#include <ny/winapi/winapiUtil.hpp>
#include <ny/winapi/winapiWindowContext.hpp>
#include <ny/window.hpp>
#include <ny/shape.hpp>

#include <iostream>

namespace ny
{

gdiDrawContext::gdiDrawContext(winapiWindowContext& wc) : drawContext(wc.getWindow()), wc_(wc)
{

}

void gdiDrawContext::clear(color col)
{
    if(!painting_)
        return;

    graphics_->Clear(colorToWinapi(col));
}

void gdiDrawContext::beginDraw()
{
    hdc_ = BeginPaint(wc_.getHandle(), &ps_);
    graphics_ = new Graphics(hdc_);

    painting_ = 1;
}

void gdiDrawContext::finishDraw()
{
    EndPaint(wc_.getHandle(), &ps_);

    delete graphics_;

    graphics_ = nullptr;
    hdc_ = nullptr;

    painting_ = 0;
}

//
void gdiDrawContext::mask(const rectangle& obj)
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

}
