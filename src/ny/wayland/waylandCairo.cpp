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
    buffer_ = new wayland::shmBuffer(size, bufferFormat::argb8888);

    //todo: correct dynamic format
    cairoSurface_ = cairo_image_surface_create_for_data((unsigned char*) buffer_->getData(), CAIRO_FORMAT_ARGB32, size.x, size.y, size.x * 4);
    cairoCR_ = cairo_create(cairoSurface_);
}

waylandCairoDrawContext::~waylandCairoDrawContext()
{
    if(cairoCR_) cairo_destroy(cairoCR_);
    if(cairoSurface_) cairo_surface_destroy(cairoSurface_);
    if(buffer_)delete buffer_;
}

void waylandCairoDrawContext::setSize(vec2ui size)
{
    if(cairoSurface_)
        cairo_surface_destroy(getCairoSurface());

    if(cairoCR_)
        cairo_destroy(cairoCR_);

    buffer_->setSize(size);

    cairoSurface_ = cairo_image_surface_create_for_data((unsigned char*) buffer_->getData(), CAIRO_FORMAT_ARGB32, size.x, size.y, size.x * 4);
    cairoCR_ = cairo_create(cairoSurface_);
}


}
