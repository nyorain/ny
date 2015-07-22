#include <ny/wayland/waylandCairo.hpp>

#include <ny/wayland/waylandUtil.hpp>
#include <ny/wayland/waylandAppContext.hpp>
#include <ny/wayland/waylandWindowContext.hpp>
#include <ny/wayland/waylandInterfaces.hpp>
#include <ny/error.hpp>
#include <ny/cairo.hpp>

#include <ny/util/rect.hpp>


namespace ny
{

using namespace wayland;

//cairo/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
waylandCairoContext::waylandCairoContext(const waylandWindowContext& wc) : wc_(wc)
{
    vec2ui size = wc.getWindow().getSize();
    buffer_ = new wayland::shmBuffer(size, bufferFormat::argb8888);

    //todo: correct dynamic format
    cairoSurface_ = cairo_image_surface_create_for_data((unsigned char*) buffer_->getData(), CAIRO_FORMAT_ARGB32, size.x, size.y, size.x * 4);
    drawContext_ = new cairoDrawContext(wc.getWindow(), *cairoSurface_);
}

waylandCairoContext::~waylandCairoContext()
{
    if(buffer_)delete buffer_;
    if(cairoSurface_)cairo_surface_destroy(cairoSurface_);
    if(drawContext_)delete drawContext_;
}

void waylandCairoContext::cairoSetSize(vec2ui size)
{
    if(cairoSurface_)
        cairo_surface_destroy(cairoSurface_);

    if(drawContext_)
        delete drawContext_;

    buffer_->setSize(size);

    cairoSurface_ = cairo_image_surface_create_for_data((unsigned char*) buffer_->getData(), CAIRO_FORMAT_ARGB32, size.x, size.y, size.x * 4);
    drawContext_ = new cairoDrawContext(wc_.getWindow(), *cairoSurface_);
}


/*
void waylandCairoChildWindowContext::finishDraw()
{
    drawContext_->apply();

    wlFrameCallback_ = wl_surface_frame(wlSurface_);
    wl_callback_add_listener(wlFrameCallback_, &frameListener, this);

    wl_surface_attach(wlSurface_, buffer_->getWlBuffer(), 0, 0);
    wl_surface_damage(wlSurface_, 0, 0, window_.getWidth(), window_.getHeight());
    wl_surface_commit(wlSurface_);
    wl_display_flush(context_->getWlDisplay());
}
*/

}
