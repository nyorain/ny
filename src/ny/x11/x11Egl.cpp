#include <ny/config.h>

#ifdef NY_WithGL
#include <ny/x11/x11Egl.hpp>

#include <ny/x11/x11AppContext.hpp>
#include <ny/x11/x11WindowContext.hpp>

#include <ny/gl/glDrawContext.hpp>


namespace ny
{

//x11EGL
x11EGLContext::x11EGLContext(const x11WindowContext& wc)
{
}

x11EGLContext::~x11EGLContext()
{
}

}

#endif // NY_WithGL
