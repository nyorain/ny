#include <ny/config.h>

#ifdef NY_WithGL
#include <ny/winapi/wgl.hpp>
#include <ny/winapi/winapiWindowContext.hpp>
#include <ny/window.hpp>

namespace ny
{

wglDrawContext::wglDrawContext(winapiWindowContext& wc) : glDrawContext(wc.getWindow()), wc_(wc)
{

}

wglDrawContext::~wglDrawContext()
{
    if(wglContext_) wglDeleteContext(wglContext_);
}

bool wglDrawContext::setupContext()
{
    //just one big pile of todo...

    PIXELFORMATDESCRIPTOR pfd =
    {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
        PFD_TYPE_RGBA,            //The kind of framebuffer. RGBA or palette.
        32,                        //Colordepth of the framebuffer.
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        24,                        //Number of bits for the depthbuffer
        8,                        //Number of bits for the stencilbuffer
        0,                        //Number of Aux buffers in the framebuffer.
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
    };

    handleDC_ = GetDC(wc_.getHandle());
    int pixelFormatVar;
    pixelFormatVar = ChoosePixelFormat(handleDC_, &pfd);
    SetPixelFormat(handleDC_, pixelFormatVar, &pfd);

    wglContext_ = wglCreateContext(handleDC_);

    init(glApi::openGL);
}

bool wglDrawContext::makeCurrentImpl()
{
    return wglMakeCurrent(handleDC_, wglContext_);
}

bool wglDrawContext::makeNotCurrentImpl()
{
    return wglMakeCurrent(nullptr, nullptr);
}

bool wglDrawContext::swapBuffers()
{
    return wglSwapLayerBuffers(handleDC_, WGL_SWAP_MAIN_PLANE);
}

}
#endif // NY_WithGL


