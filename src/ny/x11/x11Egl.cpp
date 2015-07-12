#include <ny/config.h>

#ifdef NY_WithGL
#include <ny/x11/x11Egl.hpp>

#include <ny/x11/x11AppContext.hpp>
#include <ny/x11/x11WindowContext.hpp>

#include <ny/gl/glDrawContext.hpp>


namespace ny
{

//x11EGL
x11EGLWindowContext::x11EGLWindowContext(x11WindowContext& wc)
{
    //drawContext_ = new glDrawContext(wc.getWindow());
}

x11EGLWindowContext::~x11EGLWindowContext()
{
    delete drawContext_;
}

//toplevel
x11EGLToplevelWindowContext::x11EGLToplevelWindowContext(toplevelWindow& win, const x11WindowContextSettings& s) : windowContext((window&)win, s), x11ToplevelWindowContext(win, s, 0), x11EGLWindowContext((x11WindowContext&)*this)
{

}

drawContext& x11EGLToplevelWindowContext::beginDraw()
{
    return *drawContext_;
}

void x11EGLToplevelWindowContext::finishDraw()
{

}

void x11EGLToplevelWindowContext::setSize(vec2ui size, bool change)
{

}

//child
x11EGLChildWindowContext::x11EGLChildWindowContext(childWindow& win, const x11WindowContextSettings& s) : windowContext((window&)win, s), x11ChildWindowContext(win, s, 0), x11EGLWindowContext((x11WindowContext&)*this)
{
}

drawContext& x11EGLChildWindowContext::beginDraw()
{
    return *drawContext_;
}

void x11EGLChildWindowContext::finishDraw()
{

}

void x11EGLChildWindowContext::setSize(vec2ui size, bool change)
{

}

}

#endif // NY_WithGL
