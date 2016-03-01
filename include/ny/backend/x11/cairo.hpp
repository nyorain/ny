#pragma once

#include <ny/backend/x11/include.hpp>
#include <ny/backend/x11/windowContext.hpp>
#include <ny/draw/cairo.hpp>

namespace ny
{

///CairoDrawContext implementation for rendering on X11 surfaces.
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

///WindowContext implementation on a x11 backend with cairo used for drawing.
class X11CairoWindowContext : public X11WindowContext
{
protected:
	std::unique_ptr<X11CairoDrawContext> drawContext_;

public:
	X11CairoWindowContext(X11AppContext& ctx, const X11WindowSettings& settings = {});
	
	virtual DrawGuard draw() override;
};

}
