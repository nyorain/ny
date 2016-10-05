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

//WaylandEglDisplay
WaylandEglDisplay::WaylandEglDisplay(WaylandAppContext& ac)
{
	//init display
	auto ndpy = reinterpret_cast<EGLNativeDisplayType>(&ac.wlDisplay());
    eglDisplay_ = eglGetDisplay(ndpy);

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

	EGLConfig config;
	int configSize;

	//TODO: better config choosing... !important since it might really fail here on some platforms
    eglChooseConfig(eglDisplay_, attribs, &config, 1, &configSize);
    if(!config)
    {
		auto msg = EglContext::errorMessage(eglGetError());
        throw std::runtime_error("WaylandEGLDisplay: Can't choose egl config: " + msg);
    }

	context_ = {eglDisplay_, config, GlApi::gl};

	// chose a config using a for loop and attrib checking
    // for (int i = 0; i < n; i++)
    // {
    //     eglGetConfigAttrib(eglDisplay_, configs[i], EGL_BUFFER_SIZE, &size);
    //     eglGetConfigAttrib(eglDisplay_, configs[i], EGL_RED_SIZE, &size);
    //     // just choose the first one
    //     eglConfig_ = configs[i];
    // }
}

WaylandEglDisplay::~WaylandEglDisplay()
{
	context_ = {};
	if(eglDisplay_) eglTerminate(eglDisplay_);
}

//WaylandEglWindowContext
WaylandEglWindowContext::WaylandEglWindowContext(WaylandAppContext& ac, 
	const WaylandWindowSettings& ws) : WaylandWindowContext(ac, ws)
{
	auto dpy = ac.waylandEglDisplay();
	if(!dpy) throw std::runtime_error("WaylandEglWC: cant retrieve waylandEglDisplay");

	context_ = std::make_unique<EglContext>(dpy->context());

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
