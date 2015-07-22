#include <ny/config.h>

#ifdef NY_WithGL
#include <ny/x11/x11Egl.hpp>

#include <ny/x11/x11AppContext.hpp>
#include <ny/x11/x11WindowContext.hpp>

#include <ny/gl/glDrawContext.hpp>

#include <ny/window.hpp>


namespace ny
{

//x11EGL
x11EGLDrawContext::x11EGLDrawContext(const x11WindowContext& wc) : eglDrawContext(wc.getWindow())
{
}

x11EGLDrawContext::~x11EGLDrawContext()
{
}

}

#endif // NY_WithGL
