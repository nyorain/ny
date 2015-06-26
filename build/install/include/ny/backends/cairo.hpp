#pragma once

#include "x11Include.hpp"
#include "windowContext.hpp"
#include <cairo/cairo.h>

namespace ny
{

class x11CairoContext
{
protected:
    void cairoSetSize(vec2ui size);

    cairo_surface_t* cairoSurface_;
    cairoDrawContext* drawContext_;

public:
    x11CairoContext(x11WindowContext& wc);
    virtual ~x11CairoContext();

    cairo_surface_t* getCairoSurface() const { return cairoSurface_; }
};

//
class x11CairoToplevelWindowContext : public x11ToplevelWindowContext, public x11CairoContext
{
public:
    x11CairoToplevelWindowContext(toplevelWindow& win, const x11WindowContextSettings& s = x11WindowContextSettings());

    virtual drawContext& beginDraw();
    virtual void setSize(vec2ui size, bool change = 1);
};

//
class x11CairoChildWindowContext : public x11ChildWindowContext, public x11CairoContext
{
public:
    x11CairoChildWindowContext(childWindow& win, const x11WindowContextSettings& s = x11WindowContextSettings());

    virtual drawContext& beginDraw();
    virtual void setSize(vec2ui size, bool change = 1);
};

}
