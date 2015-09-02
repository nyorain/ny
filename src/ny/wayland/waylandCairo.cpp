#include <ny/wayland/waylandCairo.hpp>

#include <ny/wayland/waylandUtil.hpp>
#include <ny/wayland/waylandAppContext.hpp>
#include <ny/wayland/waylandWindowContext.hpp>
#include <ny/wayland/waylandInterfaces.hpp>
#include <ny/error.hpp>
#include <ny/cairo.hpp>

#include <nyutil/rect.hpp>


namespace ny
{

using namespace wayland;

//cairo/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
waylandCairoDrawContext::waylandCairoDrawContext(const waylandWindowContext& wc) : cairoDrawContext(wc.getWindow()), wc_(wc)
{
    vec2ui size = wc.getWindow().getSize();
    buffer_[0] = new wayland::shmBuffer(size, bufferFormat::argb8888); //front buffer
    buffer_[1] = new wayland::shmBuffer(size, bufferFormat::argb8888);

    //todo: correct dynamic format
    cairoSurface_ = cairo_image_surface_create_for_data((unsigned char*) frontBuffer()->getData(), CAIRO_FORMAT_ARGB32, size.x, size.y, size.x * 4);
    cairoBackSurface_ = cairo_image_surface_create_for_data((unsigned char*) backBuffer()->getData(), CAIRO_FORMAT_ARGB32, size.x, size.y, size.x * 4);

    cairoCR_ = cairo_create(cairoSurface_);
    cairoBackCR_ = cairo_create(cairoBackSurface_);
}

waylandCairoDrawContext::~waylandCairoDrawContext()
{
    if(cairoCR_) cairo_destroy(cairoCR_);
    if(cairoSurface_) cairo_surface_destroy(cairoSurface_);

    if(cairoBackCR_) cairo_destroy(cairoBackCR_);
    if(cairoBackSurface_) cairo_surface_destroy(cairoBackSurface_);

    if(frontBuffer()) delete frontBuffer();
    if(backBuffer()) delete backBuffer();
}

void waylandCairoDrawContext::updateSize(const vec2ui& size)
{
    if(size != frontBuffer()->getSize())
    {
        if(cairoCR_) cairo_destroy(cairoCR_);
        if(cairoSurface_) cairo_surface_destroy(cairoSurface_);

        frontBuffer()->setSize(size);

        cairoSurface_ = cairo_image_surface_create_for_data((unsigned char*) frontBuffer()->getData(), CAIRO_FORMAT_ARGB32, size.x, size.y, size.x * 4);
        cairoCR_ = cairo_create(cairoSurface_);
    }
}

void waylandCairoDrawContext::swapBuffers()
{
    frontID_ ^= 1;

    std::swap(cairoSurface_, cairoBackSurface_);
    std::swap(cairoCR_, cairoBackCR_);
}

void waylandCairoDrawContext::attach(const vec2i& pos)
{
    wl_surface_attach(wc_.getWlSurface(), frontBuffer()->getWlBuffer(), pos.x, pos.y);
    frontBuffer()->wasAttached();
}

bool waylandCairoDrawContext::frontBufferUsed() const
{
    return frontBuffer()->used();
}


}
