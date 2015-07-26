#include <ny/winapi/gdiDrawContext.hpp>

#include <ny/winapi/winapiUtil.hpp>
#include <ny/winapi/winapiWindowContext.hpp>
#include <ny/window.hpp>

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

}
