#include "backends/wayland/cairo.hpp"

#include "backends/wayland/utils.hpp"
#include "backends/wayland/appContext.hpp"
#include "app/error.hpp"
#include "graphics/cairo.hpp"

#include "utils/rect.hpp"

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>


namespace ny
{

using namespace wayland;

//cairo/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
waylandCairoContext::waylandCairoContext(const waylandWindowContext& wc) : drawContext_(nullptr), pixels_(nullptr), wlBuffer_(nullptr), wlShmPool_(nullptr), cairoSurface_(nullptr)
{
    vec2ui size = wc.getWindow().getSize();
    if(!createBuffer(size, wc.getAppContext()->getWlShm()))
    {
        throw error(error::Critical, "cant create waylandCairoContext");
        return;
    }

    //todo: correct dynamic format
    cairoSurface_ = cairo_image_surface_create_for_data(pixels_, CAIRO_FORMAT_ARGB32, size.x, size.y, size.x * 4);
    format_ = bufferFormat::argb8888;

    drawContext_ = new cairoDrawContext(wc.getWindow(), *cairoSurface_);
}

waylandCairoContext::~waylandCairoContext()
{
    if(wlBuffer_)wl_buffer_destroy(wlBuffer_);
    if(cairoSurface_)cairo_surface_destroy(cairoSurface_);
    if(drawContext_)delete drawContext_;
    if(wlShmPool_)wl_shm_pool_destroy(wlShmPool_);
}

bool waylandCairoContext::createBuffer(vec2ui size, wl_shm* shm)
{
    int stride = size.x * 4; // 4 bytes per pixel
    int fd;

    fd = osCreateAnonymousFile(100000000);
    if (fd < 0)
    {
        std::cout << "creating a buffer file failed" << std::endl;
        return 0;
    }

    pixels_ = (unsigned char*) mmap(nullptr, 100000000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (pixels_ == MAP_FAILED)
    {
        std::cout << "mmap failed" << std::endl;
        close(fd);
        return 0;
    }

    wlShmPool_ = wl_shm_create_pool(shm, fd, 100000000);
    wlBuffer_ = wl_shm_pool_create_buffer(wlShmPool_, 0, size.x, size.y, stride, WL_SHM_FORMAT_ARGB8888);

    return 1;
}

void waylandCairoContext::cairoSetSize(window& w, vec2ui size)
{
    if(cairoSurface_)
        cairo_surface_destroy(cairoSurface_);

    if(drawContext_)
        delete drawContext_;

    wlBuffer_ = wl_shm_pool_create_buffer(wlShmPool_, 0, size.x, size.y, size.x * 4, WL_SHM_FORMAT_ARGB8888);

    cairoSurface_ = cairo_image_surface_create_for_data(pixels_, CAIRO_FORMAT_ARGB32, size.x, size.y, size.x * 4);

    drawContext_ = new cairoDrawContext(w, *cairoSurface_);
}

//cairoToplevel/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
waylandCairoToplevelWindowContext::waylandCairoToplevelWindowContext(toplevelWindow& win, const waylandWindowContextSettings& settings) : windowContext(win, settings), waylandToplevelWindowContext(win, settings), waylandCairoContext((waylandWindowContext&)*this)
{
    wl_surface_attach(wlSurface_, wlBuffer_, 0, 0);
    refresh();
}

drawContext& waylandCairoToplevelWindowContext::beginDraw()
{
    return *drawContext_;
}

void waylandCairoToplevelWindowContext::finishDraw()
{
    drawContext_->apply();
    wl_surface_attach(wlSurface_, wlBuffer_, 0, 0);
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
    wl_surface_attach(wlSurface_, wlBuffer_, 0, 0);
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

    wl_surface_attach(wlSurface_, wlBuffer_, 0, 0);
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
