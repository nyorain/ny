#pragma once

#include <ny/backend/x11/include.hpp>
#include <ny/draw/cairo.hpp>

namespace ny
{

class X11CairoDrawContext : public CairoDrawContext
{
protected:
    cairo_surface_t* xlibSurface_ = nullptr; //to make it double buffered

public:
    X11CairoDrawContext(X11WindowContext& wc);
    virtual ~X11CairoDrawContext();

    void size(const Vec2ui& size);
    virtual void apply() override;
};

}
