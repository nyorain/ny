#include <ny/wayland/waylandCairo.hpp>

#include <ny/wayland/waylandUtil.hpp>
#include <ny/wayland/waylandAppContext.hpp>
#include <ny/wayland/waylandInterfaces.hpp>
#include <ny/error.hpp>
#include <ny/cairo.hpp>

#include <ny/util/rect.hpp>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>


namespace ny
{

using namespace wayland;

//cairo/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
waylandCairoContext::waylandCairoContext(const waylandWindowContext& wc) : drawContext_(nullptr), buffer_(nullptr), cairoSurface_(nullptr)
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

void waylandCairoContext::cairoSetSize(window& w, vec2ui size)
{
    if(cairoSurface_)
        cairo_surface_destroy(cairoSurface_);

    if(drawContext_)
        delete drawContext_;

    buffer_->setSize(size);

    cairoSurface_ = cairo_image_surface_create_for_data((unsigned char*) buffer_->getData(), CAIRO_FORMAT_ARGB32, size.x, size.y, size.x * 4);
    drawContext_ = new cairoDrawContext(w, *cairoSurface_);
}

//cairoToplevel/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
waylandCairoToplevelWindowContext::waylandCairoToplevelWindowContext(toplevelWindow& win, const waylandWindowContextSettings& settings) : windowContext(win, settings), waylandToplevelWindowContext(win, settings), waylandCairoContext((waylandWindowContext&)*this)
{
    refresh();
}

drawContext& waylandCairoToplevelWindowContext::beginDraw()
{
    return *drawContext_;
}

void waylandCairoToplevelWindowContext::finishDraw()
{
    drawContext_->apply();
    wl_surface_attach(wlSurface_, buffer_->getWlBuffer(), 0, 0);
    wl_surface_damage(wlSurface_, 0, 0, window_.getWidth(), window_.getHeight());
    wl_surface_commit(wlSurface_);
    wl_display_flush(context_->getWlDisplay());
}

void waylandCairoToplevelWindowContext::setSize(vec2ui size, bool change)
{
    if(!rect2ui(window_.getMinSize(), window_.getMaxSize() - window_.getMinSize()).contains(size))
        return;

    cairoSetSize(window_, size);
    refresh();
}

//cairoChild/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
waylandCairoChildWindowContext::waylandCairoChildWindowContext(childWindow& win, const waylandWindowContextSettings& settings) : windowContext(win, settings), waylandChildWindowContext(win, settings), waylandCairoContext((waylandWindowContext&)*this)
{
    wl_surface_attach(wlSurface_, buffer_->getWlBuffer(), 0, 0);
    refresh();
}

drawContext& waylandCairoChildWindowContext::beginDraw()
{
    return *drawContext_;
}

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

void waylandCairoChildWindowContext::setSize(vec2ui size, bool change)
{
    if(!rect2ui(window_.getMinSize(), window_.getMaxSize()).contains(size))
        return;

    cairoSetSize(window_, size);
    refresh();
}



}
