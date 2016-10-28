#include <ny/backend/winapi/wgl.hpp>
#include <ny/backend/winapi/windowContext.hpp>
#include <ny/backend/winapi/appContext.hpp>
#include <ny/backend/winapi/util.hpp>
#include <ny/backend/winapi/wglApi.hpp>
#include <ny/backend/integration/surface.hpp>
#include <ny/base/log.hpp>

#include <thread>

namespace ny
{

namespace
{

thread_local WglSetup* gWglSetup;
void* wglLoadFunc(const char* name)
{
	if(!gWglSetup) return nullptr;
	return gWglSetup->procAddr(name);
}

}

class WglSetup::WglContextWrapper
{
public:
	WglContextWrapper(HDC hdc, HGLRC share);
	~WglContextWrapper();

public:
	HGLRC context;
	bool shared;
};

//Throws on failure to create any context.
WglSetup::WglContextWrapper::WglContextWrapper(HDC hdc, HGLRC share)
{
	if(GLAD_WGL_ARB_create_context)
	{
		int attributes[] =
		{
			WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
			WGL_CONTEXT_MINOR_VERSION_ARB, 5,
			0, 0
		};

		unsigned int versionPairs[][2] = {{3, 3}, {3, 2}, {3, 1}, {3, 0}, {1, 2}, {1, 0}};
		for(const auto& p : versionPairs)
		{
			context = ::wglCreateContextAttribsARB(hdc, share, attributes);
			if(!context)
			{
				attributes[1] = p[0];
				attributes[3] = p[1];

				auto error = ::GetLastError();
				if(error == ERROR_INVALID_VERSION_ARB) continue;
			}

			break;
		}

		if(context) return;
		warning(errorMessage("ny::WglContext: wglCreateContextAttribsARB failed, using old func"));
	}

	context = ::wglCreateContext(hdc);
	if(!context) throw winapi::EC::excpetion("ny::WglContext: wglCreateContext failed");

	if(share && !::wglShareLists(share, context))
		warning(errorMessage("ny::WglContext: wglShareLists"));
}

WglSetup::WglContextWrapper::~WglContextWrapper()
{
	if(context) ::wglDeleteContext(context);
}

//WglSetup
WglSetup::WglSetup() = default;

WglSetup::WglSetup(HWND dummy) : dummyWindow_(dummy)
{
	//choose the used pixel format
	dummyDC_ = ::GetDC(dummyWindow_);

	PIXELFORMATDESCRIPTOR pfd {};
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 24;
	pfd.cStencilBits = 8;
	pfd.cAlphaBits = 8;
	pfd.iLayerType = PFD_MAIN_PLANE;

	pixelFormat_ = ::ChoosePixelFormat(dummyDC_, &pfd);
	::SetPixelFormat(dummyDC_, pixelFormat_, &pfd);

	//try to find/open the opengl library
	glLibrary_ = {"opengl32"};
	if(!glLibrary_) glLibrary_ = {"openglsf"};
	if(!glLibrary_) glLibrary_ = {"GL"};
	if(!glLibrary_) glLibrary_ = {"OpenGL"};
	if(!glLibrary_) glLibrary_ = {"wgl"};
	if(!glLibrary_) glLibrary_ = {"WGL"};

	if(!glLibrary_)
		throw std::runtime_error("ny::WinapiAppContext::WglSetup: Failed to load opengl library");

	//create a dummy context
	{
		HGLRC share = nullptr;
		WglContextWrapper dummyGl(dummyDC_, share);

		//load the further needed gl functions/extensions
		gWglSetup = this; //needed by wglLoadFunc
		wglMakeCurrent(dummyDC_, dummyGl.context);
		gladLoadWGLLoader(wglLoadFunc, dummyDC_);
		wglMakeCurrent(nullptr, nullptr);
		gWglSetup = nullptr;
	}
}

WglSetup::~WglSetup()
{
	if(dummyWindow_ && dummyDC_) ::ReleaseDC(dummyWindow_, dummyDC_);
}

WglSetup::WglSetup(WglSetup&& other) noexcept
{
	shared_ = std::move(other.shared_);
	unique_ = std::move(other.unique_);
	glLibrary_ = std::move(other.glLibrary_);

	pixelFormat_ = other.pixelFormat_;
	dummyWindow_ = other.dummyWindow_;
	dummyDC_ = other.dummyDC_;

	other.pixelFormat_ = {};
	other.dummyWindow_ = {};
	other.dummyDC_ = {};
}

WglSetup& WglSetup::operator=(WglSetup&& other) noexcept
{
	if(dummyWindow_ && dummyDC_) ::ReleaseDC(dummyWindow_, dummyDC_);

	shared_ = std::move(other.shared_);
	unique_ = std::move(other.unique_);
	glLibrary_ = std::move(other.glLibrary_);

	pixelFormat_ = other.pixelFormat_;
	dummyWindow_ = other.dummyWindow_;
	dummyDC_ = other.dummyDC_;

	other.pixelFormat_ = {};
	other.dummyWindow_ = {};
	other.dummyDC_ = {};

	return *this;
}

HGLRC WglSetup::sharedContext() const
{
	for(auto& ctx : shared_)
		if(ctx.shared)
			return ctx.context;

	for(auto& ctx : unique_)
		if(ctx.shared)
			return ctx.context;

	return nullptr;
}

HGLRC WglSetup::createContext(bool& shared, bool unique)
{
	shared = false;
	HGLRC sharedctx = nullptr;

	if(shared) sharedctx = sharedContext();

	if(unique)
	{

		unique_.emplace_back(dummyDC_, sharedctx);
		if(shared && !unique_.back().shared) shared = false;
		return unique_.back().context;
	}
	else
	{
		shared_.emplace_back(dummyDC_, sharedctx);
		if(shared && !shared_.back().shared) shared = false;
		return shared_.back().context;
	}
}

HGLRC WglSetup::getContext(bool& shared)
{
	shared = false;

	//first try to find an already existent really shared context
	for(auto& ctx : shared_)
	{
		if(ctx.shared)
		{
			shared = true;
			return ctx.context;
		}
	}

	//otherwise create one
	return createContext(shared, false);
}

void* WglSetup::procAddr(nytl::StringParam name) const
{
	auto ret = reinterpret_cast<void*>(wglGetProcAddress(name));
	if(!ret) ret = glLibrary_.symbol(name);
	return ret;
}

//WglContext
WglContext::WglContext(WglSetup& setup, HDC hdc, const GlContextSettings& settings)
	: setup_(&setup), hdc_(hdc), shared_(true)
{
	PIXELFORMATDESCRIPTOR pfd {};
	::DescribePixelFormat(hdc_, setup.pixelFormat(), sizeof(pfd), &pfd);
	::SetPixelFormat(hdc_, setup.pixelFormat(), &pfd);

	//the initPixelFormat function would use the wgl pixel format arb extensions to
	//let us choose a better pixel format. The problem:
	//If we set the pixel format here to something different than the setup provides
	//the context wont work (if we create the context for our hdc, the loaded function
	//pointers may not work).
	//Therefore TODO: use the code from initPixelFormat in WglSetup to choose a better pixelformat.
	// initPixelFormat(24, 8);

	if(!settings.uniqueContext) wglContext_ = setup.createContext(shared_);
	else wglContext_ = setup.createContext(shared_);

	GlContext::initContext(Api::gl, 24, 8); //dont hardcode this

	if(settings.vsync) activateVsync();
	if(settings.storeContext) *settings.storeContext = this;
}

WglContext::~WglContext()
{
	makeNotCurrent();
}

void WglContext::initPixelFormat(unsigned int depth, unsigned int stencil)
{
	//this function is not used at the moment.
	//TODO use this in the setup code

	if(GLAD_WGL_ARB_pixel_format)
	{
		int attr[] =
		{
			WGL_DRAW_TO_WINDOW_ARB, true,
			WGL_SUPPORT_OPENGL_ARB, true,
			WGL_DOUBLE_BUFFER_ARB,  true,
			WGL_DEPTH_BITS_ARB, 	static_cast<int>(depth),
			WGL_STENCIL_BITS_ARB, 	static_cast<int>(stencil),
			WGL_DOUBLE_BUFFER_ARB,	true,
			WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
			0,                      0
		};

		unsigned int c;
		int pixelFormat;
		if(wglChoosePixelFormatARB(hdc_, attr, nullptr, 1, &pixelFormat, &c) && c > 0) return;
		warning(errorMessage("ny::WglContext: wglChoosePixelFormatARB failed"));
	}

	PIXELFORMATDESCRIPTOR pfd {};
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 24;
	pfd.cStencilBits = 8;
	pfd.cAlphaBits = 8;
	pfd.iLayerType = PFD_MAIN_PLANE;

	auto pixelFormat = ::ChoosePixelFormat(hdc_, &pfd);
	if(!pixelFormat) throw winapi::EC::excpetion("ny::WglContext: ChoosePixelFormat failed");
}

void WglContext::activateVsync()
{
	//wglSwapIntervalEXT has no effect on the current context, but rather on the window
	//that is associated with the current context. Therefore we can use it safely here
	//without effecting other windows that use the same context (at least per spec).
	if(GLAD_WGL_EXT_swap_control)
		if(!wglSwapIntervalEXT(1)) warning("ny::WglContext: failed to enable vertical sync");
}

bool WglContext::sharedWith(const GlContext& other) const
{
	auto* wgl = dynamic_cast<const WglContext*>(&other);
	return wgl && ((shared_ && wgl->shared_) || wglContext_ == wgl->wglContext_);
}

void* WglContext::procAddr(nytl::StringParam name) const
{
	return setup_->procAddr(name);
}

bool WglContext::makeCurrentImpl(std::error_code& ec)
{
	::SetLastError(0);
	if(!::wglMakeCurrent(hdc_, wglContext_))
	{
		warning(errorMessage("ny::WglContext::makeCurrentImpl (wglMakeCurrent) failed"));
		ec = {static_cast<int>(::GetLastError()), WinapiErrorCategory::instance()};
		return false;
	}

	return true;
}

bool WglContext::makeNotCurrentImpl(std::error_code& ec)
{
	::SetLastError(0);
	if(!::wglMakeCurrent(nullptr, nullptr))
	{
		warning(errorMessage("ny::WglContext::makeNotCurrentImpl (wglMakeCurrent) failed"));
		ec = {static_cast<int>(::GetLastError()), WinapiErrorCategory::instance()};
		return false;
	}

	return true;
}

bool WglContext::apply(std::error_code& ec)
{
	::SetLastError(0);
	if(!::SwapBuffers(hdc_))
	{
		warning(errorMessage("ny::WglContext::apply (SwapBuffer) failed"));
		ec = {static_cast<int>(::GetLastError()), WinapiErrorCategory::instance()};
		return false;
	}

	return true;
}

//WglWindowContext
WglWindowContext::WglWindowContext(WinapiAppContext& ac, WglSetup& setup,
	const WinapiWindowSettings& settings)
{
	appContext_ = &ac;

	WinapiWindowContext::initWindowClass(settings);
	WinapiWindowContext::setStyle(settings);
	WinapiWindowContext::initWindow(settings);
	WinapiWindowContext::showWindow(settings);

	hdc_ = ::GetDC(handle());
	context_.reset(new WglContext(setup, hdc_, settings.gl));
}

WglWindowContext::~WglWindowContext()
{
	context_.reset();
	if(hdc_) ::ReleaseDC(handle(), hdc_);
}

WNDCLASSEX WglWindowContext::windowClass(const WinapiWindowSettings& settings)
{
	auto ret = WinapiWindowContext::windowClass(settings);
	ret.style |= CS_OWNDC;
	return ret;
}

bool WglWindowContext::surface(Surface& surface)
{
	surface.type = SurfaceType::gl;
	surface.gl = context_.get();
	return true;
}

}
