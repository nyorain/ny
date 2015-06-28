#pragma once

#include <ny/backends/wayland/waylandInclude.hpp>

#include <ny/app/surface.hpp>
#include <ny/backends/wayland/windowContext.hpp>
#include <ny/window/windowDefs.hpp>

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
    wayland::shmBuffer* buffer_;
    cairo_surface_t* cairoSurface_;

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
