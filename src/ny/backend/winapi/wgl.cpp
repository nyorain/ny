#include <ny/backend/winapi/wgl.hpp>
#include <ny/backend/winapi/windowContext.hpp>
#include <ny/backend/winapi/appContext.hpp>
#include <ny/draw/gl/drawContext.hpp>
#include <ny/base/log.hpp>

namespace ny
{

//WglContext
WglContext::WglContext(WinapiWindowContext& wc) : GlContext(), wc_(&wc)
{
	dc_ = ::GetDC(wc.handle());

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

    int pixelFormatVar;
    pixelFormatVar = ::ChoosePixelFormat(dc_, &pfd);
    ::SetPixelFormat(dc_, pixelFormatVar, &pfd);

    wglContext_ = ::wglCreateContext(dc_);

    GlContext::initContext(Api::openGL, 24, 8);
}

WglContext::~WglContext()
{
	if(wglContext_) ::wglDeleteContext(wglContext_);
}

bool WglContext::makeCurrentImpl()
{
    if(!isCurrent()) return ::wglMakeCurrent(dc_, wglContext_);
    return true;
}

bool WglContext::makeNotCurrentImpl()
{
    if(isCurrent()) return ::wglMakeCurrent(nullptr, nullptr);
    return true;
}

bool WglContext::apply()
{
    //return wglSwapLayerBuffers(handleDC_, WGL_SWAP_MAIN_PLANE);
	debug("swap");
	GlContext::apply();
    return ::SwapBuffers(dc_);
}

//WglWindowContext
WglWindowContext::WglWindowContext(WinapiAppContext& ctx, const WinapiWindowSettings& settings)
{
	appContext_ = &ctx;

    if(!appContext_->hinstance())
	{
		throw std::runtime_error("winapiWC::create: uninitialized appContext");
	}

	WinapiWindowContext::initWindowClass(settings);
	wndClass_.style |= CS_OWNDC;

	if(!::RegisterClassEx(&wndClass_))
	{
		throw std::runtime_error("winapiWC::create: could not register window class");
		return;
	}

	WinapiWindowContext::setStyle(settings);
	WinapiWindowContext::initWindow(settings);

	wglContext_.reset(new WglContext(*this));
	drawContext_.reset(new GlDrawContext());
}

WglWindowContext::~WglWindowContext()
{
}

DrawGuard WglWindowContext::draw()
{
	if(!wglContext_->makeCurrent())
		throw std::runtime_error("WglWC::draw: Failed to make wgl Context current");

	return DrawGuard(*drawContext_);
}

}
