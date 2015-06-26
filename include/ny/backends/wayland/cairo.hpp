#pragma once

#include "app/surface.hpp"
#include "backends/wayland/windowContext.hpp"
#include "window/windowDefs.hpp"

#include <cairo/cairo.h>

namespace ny
{

//
class waylandCairoContext
{
private:
    waylandCairoContext(const waylandCairoContext& other) = delete;
protected:
    cairoDrawContext* drawContext_;

    unsigned char* pixels_;
    wl_buffer* wlBuffer_;
    wl_shm_pool* wlShmPool_;
    cairo_surface_t* cairoSurface_;
    bufferFormat format_;

    bool createBuffer(vec2ui size, wl_shm* shm);
    void cairoSetSize(window& w, vec2ui size);

public:
    waylandCairoContext(const waylandWindowContext& wc);
    virtual ~waylandCairoContext();

    cairo_surface_t* getCairoSurface() const { return cairoSurface_; }
};

//
class waylandCairoToplevelWindowContext : public waylandToplevelWindowContext, public waylandCairoContext
{
public:
    waylandCairoToplevelWindowContext(toplevelWindow& win, const waylandWindowContextSettings& s = waylandWindowContextSettings());

    virtual drawContext& beginDraw();
    virtual void finishDraw();
    virtual void setSize(vec2ui size, bool change = 1);
};

//
class waylandCairoChildWindowContext : public waylandChildWindowContext, public waylandCairoContext
{
public:
    waylandCairoChildWindowContext(childWindow& win, const waylandWindowContextSettings& s = waylandWindowContextSettings());

    virtual drawContext& beginDraw();
    virtual void finishDraw();
    virtual void setSize(vec2ui size, bool change = 1);
};

}
