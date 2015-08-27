#include <ny/x11/x11Cairo.hpp>

#include <ny/x11/x11AppContext.hpp>
#include <ny/x11/x11WindowContext.hpp>

#include <ny/cairo.hpp>
#include <ny/window.hpp>

#include <cairo/cairo-xlib.h>

namespace ny
{

//x11CairoContext
x11CairoDrawContext::x11CairoDrawContext(x11WindowContext& wc) : cairoDrawContext(wc.getWindow())
{
    xlibSurface_ = cairo_xlib_surface_create(getXDisplay(), wc.getXWindow(), wc.getXVinfo()->visual, wc.getWindow().getWidth(),wc.getWindow().getHeight());
    cairoSurface_ = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, wc.getWindow().getWidth(),wc.getWindow().getHeight());
    cairoCR_ = cairo_create(cairoSurface_);
}

x11CairoDrawContext::~x11CairoDrawContext()
{
    cairo_surface_destroy(xlibSurface_);
    cairo_surface_destroy(cairoSurface_);
    cairo_destroy(cairoCR_);
}

void x11CairoDrawContext::setSize(vec2ui size)
{
    cairo_surface_destroy(cairoSurface_);
    cairo_destroy(cairoCR_);

    cairoSurface_ = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size.x, size.y);
    cairoCR_ = cairo_create(cairoSurface_);

    cairo_xlib_surface_set_size(xlibSurface_, size.x, size.y);
    resetClip();
}

void x11CairoDrawContext::apply()
{
    cairo_surface_flush(cairoSurface_);
    cairo_surface_copy_page(cairoSurface_);

    cairo_t* cr = cairo_create(xlibSurface_);

    //cairo_set_source_rgba(cr, 1., 1., 1., 1.);
    //cairo_paint(cr);

    cairo_set_source_surface(cr, cairoSurface_, 0, 0);
    cairo_paint(cr);

    cairo_destroy(cr);
}

/*
//x11CairoToplevel
x11CairoToplevelWindowContext::x11CairoToplevelWindowContext(toplevelWindow& win, const x11WindowContextSettings& settings) : windowContext(win, settings), x11ToplevelWindowContext(win, settings), x11CairoContext((x11WindowContext&)*this)
{

    XVisualInfo vi;
    if(!XMatchVisualInfo(xDisplay_, 0, 32, TrueColor, &vi))
    {
        std::cout << "fail" << std::endl;
        return;
    }

    xVinfo_ = new XVisualInfo;
    *xVinfo_ = vi;

    XSetWindowAttributes attr1;
    attr1.colormap = XCreateColormap(xDisplay_, DefaultRootWindow(xDisplay_), xVinfo_->visual, AllocNone);
    attr1.background_pixel = 0;
    attr1.border_pixel = 0;
    attr1.event_mask = StructureNotifyMask | PointerMotionHintMask | ExposureMask;

    xWindow_ = XCreateWindow(xDisplay_, DefaultRootWindow(xDisplay_), win.getPositionX(), win.getPositionY(), win.getWidth(), win.getHeight(), 1, vi.depth, InputOutput, vi.visual, CWEventMask | CWColormap | CWBackPixel | CWBorderPixel, &attr1);

    context_->registerContext(xWindow_, this);


    //cairoSurface_ = cairo_xlib_surface_create(getXDisplay(), getXWindow(), getXVinfo()->visual, getWindow().getWidth(),getWindow().getHeight());
    //drawContext_ = new cairoDrawContext(getWindow(), *cairoSurface_);
}

drawContext& x11CairoToplevelWindowContext::beginDraw()
{
    return *drawContext_;
}

void x11CairoToplevelWindowContext::setSize(vec2ui size, bool change)
{
    x11ToplevelWindowContext::setSize(size, change);
    cairoSetSize(size);
}


//x11CairoChild
x11CairoChildWindowContext::x11CairoChildWindowContext(childWindow& win, const x11WindowContextSettings& settings) : windowContext(win, settings), x11ChildWindowContext(win, settings), x11CairoContext((x11WindowContext&)*this)
{
}

drawContext& x11CairoChildWindowContext::beginDraw()
{
    return *drawContext_;
}

void x11CairoChildWindowContext::setSize(vec2ui size, bool change)
{
    x11ChildWindowContext::setSize(size, change);

    cairoSetSize(size);
}
*/


}
