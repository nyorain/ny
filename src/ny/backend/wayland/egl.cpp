#include <ny/backend/wayland/egl.hpp>
#include <ny/backend/wayland/windowContext.hpp>
#include <ny/backend/wayland/appContext.hpp>
#include <ny/backend/wayland/util.hpp>
#include <ny/backend/integration/surface.hpp>
#include <ny/base/log.hpp>

#include <wayland-egl.h>
#include <EGL/egl.h>

namespace ny
{

//A small derivate of EglContext that holds a reference to the WindowContext it is current for.
//If the WindowContext is not shown, the EglContext does not swapBuffers on apply()
//Could be done more beatiful with new constructor and protected wc member...
class WaylandEglContext : public EglContext
{
public:
	using EglContext::EglContext;
	WaylandWindowContext* waylandWC_;

public:
	bool apply() override
	{
		if(!waylandWC_->shown()) return true;
		return EglContext::apply();
	}
};

//WaylandEglDisplay
WaylandEglDisplay::WaylandEglDisplay(WaylandAppContext& ac)
{
	//init display
	auto ndpy = reinterpret_cast<EGLNativeDisplayType>(&ac.wlDisplay());
    eglDisplay_ = ::eglGetDisplay(ndpy);

    if(eglDisplay_ == EGL_NO_DISPLAY)
    {
		auto msg = EglContext::errorMessage(eglGetError());
        throw std::runtime_error("WaylandEGLDisplay: Can't create egl display: " + msg);
    }

	int major, minor;
	if(eglInitialize(eglDisplay_, &major, &minor) != EGL_TRUE)
	{
		auto msg = EglContext::errorMessage(eglGetError());
        throw std::runtime_error("WaylandEGLDisplay: Can't init egl display: " + msg);
	}

	log("ny::WaylandEglDisplay: EGL version: ", major, ".", minor);


	//init context
    EGLint renderable = EGL_OPENGL_BIT; //todo
    EGLint attribs[] =
    {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_RENDERABLE_TYPE, renderable,
        EGL_NONE
    };

	int configSize;

	//TODO: better config choosing... !important since it might really fail here on some platforms
    eglChooseConfig(eglDisplay_, attribs, &eglConfig_, 1, &configSize);
    if(!eglConfig_)
    {
		auto msg = EglContext::errorMessage(eglGetError());
        throw std::runtime_error("WaylandEGLDisplay: Can't choose egl config: " + msg);
    }

	//TODO: enable option for gles contexts?
	constexpr auto api = GlApi::gl;

	if(api == GlApi::gles)
	{
		eglBindAPI(EGL_OPENGL_ES_API);
		const int attrib[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
		eglContext_ = eglCreateContext(eglDisplay_, eglConfig_, nullptr, attrib);
	}
	else if(api == GlApi::gl)
	{
		eglBindAPI(EGL_OPENGL_API);
		eglContext_ = eglCreateContext(eglDisplay_, eglConfig_, nullptr, nullptr);
	}

	if(!eglContext_)
	{
		auto msg = EglContext::errorMessage(eglGetError());
		throw std::runtime_error("ny::EglContextGuard: failed to create EGLContext: " + msg);
	}
}

WaylandEglDisplay::~WaylandEglDisplay()
{
	if(eglDisplay_ && eglContext_) eglDestroyContext(eglDisplay_, eglContext_);
	if(eglDisplay_) eglTerminate(eglDisplay_);
}

//WaylandEglWindowContext
WaylandEglWindowContext::WaylandEglWindowContext(WaylandAppContext& ac, 
	const WaylandWindowSettings& ws) : WaylandWindowContext(ac, ws)
{
	auto dpy = ac.waylandEglDisplay();
	if(!dpy) throw std::runtime_error("WaylandEglWC: cant retrieve waylandEglDisplay");

	auto ctx = std::make_unique<WaylandEglContext>(dpy->eglDisplay(), dpy->eglContext(), 
		dpy->eglConfig(), GlApi::gl);
	ctx->waylandWC_ = this;
	context_ = std::move(ctx);

    wlEglWindow_ = wl_egl_window_create(&wlSurface(), ws.size.x, ws.size.y);
    if(!wlEglWindow_) throw std::runtime_error("WaylandEglWC: wl_egl_window_create failed");

    eglSurface_ = eglCreateWindowSurface(context_->eglDisplay(), context_->eglConfig(), 
		(EGLNativeWindowType)wlEglWindow_, nullptr);
    if(!eglSurface_)
    {
		auto msg = EglContext::errorMessage(eglGetError());
        throw std::runtime_error("WaylandEglWC: eglCreateWindowSurface failed: " + msg);
        return;
    }

	context_->eglSurface(eglSurface_);

	//store context if requested so
	if(ws.gl.storeContext) *ws.gl.storeContext = context_.get();
}

WaylandEglWindowContext::~WaylandEglWindowContext()
{
	if(eglSurface_) eglDestroySurface(context_->eglDisplay(), eglSurface_);
	context_.reset();

	if(wlEglWindow_) wl_egl_window_destroy(wlEglWindow_);
}

void WaylandEglWindowContext::size(const nytl::Vec2ui& newSize)
{
	WaylandWindowContext::size(newSize);
    wl_egl_window_resize(wlEglWindow_, newSize.x, newSize.y, 0, 0);
}

bool WaylandEglWindowContext::surface(Surface& surface)
{
	surface.type = SurfaceType::gl;
	surface.gl = context_.get();
	return true;
}

}

