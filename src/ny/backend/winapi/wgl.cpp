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

//Utilty winapi window wrapper following RAII
class WinapiWindow
{
public:
	HWND handle;

public:
	WinapiWindow()
	{
		handle = ::CreateWindow("STATIC", "", WS_DISABLED, 0, 0, 100, 100, nullptr, nullptr,
			GetModuleHandle(nullptr), nullptr);
	}

	~WinapiWindow()
	{
		if(handle) ::DestroyWindow(handle);
	}
};

}

HMODULE WglContext::glLibHandle()
{
	static auto lib = LoadLibrary("opengl32.dll");
	if(!lib) lib = LoadLibrary("opengl32sf.dll");
	return lib;
}

HGLRC WglContext::dummyContext(HDC* hdcOut)
{
	static HGLRC context = nullptr;
	static HDC dc = nullptr;

	if(!context)
	{
		//hidden window
		static WinapiWindow window;
		ShowWindow(window.handle, SW_HIDE);
		dc = GetDC(window.handle);

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

		auto pixelformat = ::ChoosePixelFormat(dc, &pfd);
		::SetPixelFormat(dc, pixelformat, &pfd);
		context = ::wglCreateContext(dc);
	}

	if(hdcOut) *hdcOut = dc;
	return context;
}

void WglContext::assureWglLoaded()
{
	static bool loaded = false;
	if(!loaded)
	{
		HDC hdc;
		auto ctx = dummyContext(&hdc);
		wglMakeCurrent(hdc, ctx);
		gladLoadWGLLoader(&WglContext::wglProcAddr, hdc);
		loaded = true;
	}
}

void* WglContext::wglProcAddr(const char* name)
{
	return reinterpret_cast<void*>(wglGetProcAddress(name));
}

//WglContext
WglContext::WglContext(HWND hwnd, const GlContextSettings& settings) : hwnd_(hwnd)
{
	dc_ = ::GetDC(hwnd_);
	init(settings);
}

WglContext::WglContext(HDC hdc, const GlContextSettings& settings) : dc_(hdc)
{
	init(settings);
}

WglContext::~WglContext()
{
	makeNotCurrent();
	if(wglContext_) ::wglDeleteContext(wglContext_);
	if(hwnd_ && dc_) ::ReleaseDC(hwnd_, dc_);
}

void WglContext::init(const GlContextSettings& settings)
{
	assureWglLoaded(); //will load functions and assure there is a dummy context bound
	initPixelFormat(24, 8);
	auto pfd = pixelFormatDescriptor();
    ::SetPixelFormat(dc_, pixelFormat_, &pfd);

	createContext();
	makeCurrent();

    GlContext::initContext(Api::gl, 24, 8);
	if(settings.vsync) activateVsync();
	if(settings.storeContext) *settings.storeContext = this;
}

void WglContext::initPixelFormat(unsigned int depth, unsigned int stencil)
{
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
		if(wglChoosePixelFormatARB(dc_, attr, nullptr, 1, &pixelFormat_, &c) && c > 0) return;
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

	pixelFormat_ = ::ChoosePixelFormat(dc_, &pfd);
	if(!pixelFormat_)
	{
		//TODO getlasterror
		throw std::runtime_error(errorMessage("ny::WglContext: ChoosePixelFormat failed"));
	}
}

PIXELFORMATDESCRIPTOR WglContext::pixelFormatDescriptor() const
{
	PIXELFORMATDESCRIPTOR ret;
	ret.nSize = sizeof(ret);
	ret.nVersion = 1;
	::DescribePixelFormat(dc_, pixelFormat_, sizeof(ret), &ret);
	return ret;
}

void WglContext::createContext()
{
	if(GLAD_WGL_ARB_create_context)
	{
		int attributes[] =
		{
			WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
			WGL_CONTEXT_MINOR_VERSION_ARB, 5,
			0, 0
		};

		unsigned int versionPairs[][2] = {{3, 3}, {3, 2}, {3, 1}, {3, 0}, {1, 2}};
		for(auto currPair = 0; currPair < sizeof(versionPairs); ++currPair)
		{
			wglContext_ = ::wglCreateContextAttribsARB(dc_, nullptr, attributes);
			if(!wglContext_)
			{
				attributes[1] = versionPairs[currPair][0];
				attributes[3] = versionPairs[currPair][1];

				auto error = ::GetLastError();
				if(error == ERROR_INVALID_VERSION_ARB) continue;
			}
			break;
		}

		if(wglContext_) return;
		warning(errorMessage("ny::WglContext: createContextAttribsARB failed, using old func"));
	}

	wglContext_ = ::wglCreateContext(dc_);
	if(!wglContext_)
		throw std::runtime_error(errorMessage("ny::WglContext: failed to create context"));
}

void WglContext::activateVsync()
{
	if(GLAD_WGL_EXT_swap_control)
		if(!wglSwapIntervalEXT(1)) warning("ny::WglContext: failed to enable vertical sync");
}

void* WglContext::procAddr(const char* name) const
{
	auto ret = reinterpret_cast<void*>(::wglGetProcAddress(name));
	if(!ret) ret = reinterpret_cast<void*>(::GetProcAddress(glLibHandle(), name));
	return ret;
}

bool WglContext::makeCurrentImpl()
{
	auto ret = ::wglMakeCurrent(dc_, wglContext_);
	if(!ret) warning(errorMessage("WglContext::makeCurrentImpl"));
	return ret;
}

bool WglContext::makeNotCurrentImpl()
{
	auto ret = ::wglMakeCurrent(nullptr, nullptr);
	if(!ret) warning(errorMessage("WglContext::makeNotCurrentImpl"));
	return ret;
}

bool WglContext::apply()
{
	GlContext::apply();
    auto ret = ::SwapBuffers(dc_);
	if(!ret) warning(errorMessage("Wgl::apply (SwapBuffer)"));
	return ret;
}

//WglWindowContext
WglWindowContext::WglWindowContext(WinapiAppContext& ctx, const WinapiWindowSettings& settings)
{
	appContext_ = &ctx;
    if(!hinstance()) throw std::runtime_error("winapiWC::create: uninitialized appContext");

	WinapiWindowContext::initWindowClass(settings);
	WinapiWindowContext::setStyle(settings);
	WinapiWindowContext::initWindow(settings);
	WinapiWindowContext::showWindow(settings);

	wglContext_.reset(new WglContext(handle(), settings.gl));
}

WglWindowContext::~WglWindowContext()
{
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
	surface.gl = wglContext_.get();
	return true;
}

}
