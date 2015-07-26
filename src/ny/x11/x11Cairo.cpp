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
    cairoSurface_ = cairo_xlib_surface_create(getXDisplay(), wc.getXWindow(), wc.getXVinfo()->visual, wc.getWindow().getWidth(),wc.getWindow().getHeight());
    cairoCR_ = cairo_create(cairoSurface_);
}

x11CairoDrawContext::~x11CairoDrawContext()
{
}

void x11CairoDrawContext::setSize(vec2ui size)
{
    cairo_xlib_surface_set_size(cairoSurface_, size.x, size.y);
    resetClip();
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
