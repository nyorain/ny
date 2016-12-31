// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/winapi/wgl.hpp>
#include <ny/winapi/windowContext.hpp>
#include <ny/winapi/appContext.hpp>
#include <ny/winapi/util.hpp>
#include <ny/winapi/wglApi.hpp>

#include <ny/surface.hpp>
#include <ny/log.hpp>

#include <nytl/scope.hpp>

#include <thread>

namespace ny
{

namespace
{

thread_local WglSetup* gWglSetup;
void* wglLoadFunc(const char* name) { return gWglSetup->procAddr(name); }

}

//WglSetup
WglSetup::WglSetup(HWND dummy) : dummyWindow_(dummy)
{
	dummyDC_ = ::GetDC(dummyWindow_);

	//we set the pixel format for the dummy dc and then create an opengl context for it
	//this context is used to load all the basic required functions
	//note that we load extensions functions like wglChoosePixelFormatARB that work
	//independent from a current context
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

	int pf = ::ChoosePixelFormat(dummyDC_, &pfd);
	::SetPixelFormat(dummyDC_, pf, &pfd);

	//try to find/open the opengl library
	glLibrary_ = {"opengl32"};
	if(!glLibrary_) glLibrary_ = {"openglsf"};
	if(!glLibrary_) glLibrary_ = {"GL"};
	if(!glLibrary_) glLibrary_ = {"OpenGL"};
	if(!glLibrary_) glLibrary_ = {"wgl"};
	if(!glLibrary_) glLibrary_ = {"WGL"};

	if(!glLibrary_) throw std::runtime_error("ny::WglSetup: Failed to load opengl library");

	HGLRC dummyContext = ::wglCreateContext(dummyDC_);
	if(!dummyContext) throw winapi::EC::exception("ny::WglSetup: unable to create dummy context");

	gWglSetup = this; //needed by wglLoadFunc
	wglMakeCurrent(dummyDC_, dummyContext);
	gladLoadWGLLoader(wglLoadFunc, dummyDC_);
	wglMakeCurrent(nullptr, nullptr);
	gWglSetup = nullptr;

	//function to rate a GlConfig

	//now enumerate all gl configs
	//if the extension is available retrieve all formats with the required attributes
	//otherwise just retrieve and parse parse a few formats
	if(GLAD_WGL_ARB_pixel_format)
	{
		int pfAttribs[] =
		{
			WGL_DRAW_TO_WINDOW_ARB, true,
			WGL_SUPPORT_OPENGL_ARB, true,
			0, 0
		};

		int formats[1024] {};
		UINT nformats = 0;
		bool ret = wglChoosePixelFormatARB(dummyDC_, pfAttribs, nullptr, 1024, formats, &nformats);

		if(ret && nformats)
		{
			std::vector<int> attribs =
			{
				WGL_ACCELERATION_ARB,
				WGL_RED_BITS_ARB,
				WGL_GREEN_BITS_ARB,
				WGL_BLUE_BITS_ARB,
				WGL_ALPHA_BITS_ARB,
				WGL_DEPTH_BITS_ARB,
				WGL_STENCIL_BITS_ARB,
				WGL_DOUBLE_BUFFER_ARB //7
			};

			if(GLAD_WGL_ARB_multisample)
			{
				attribs.push_back(WGL_SAMPLE_BUFFERS_ARB);
				attribs.push_back(WGL_SAMPLES_ARB); //9
			}

			configs_.reserve(nformats);

			std::vector<int> values;
			values.resize(attribs.size());

			auto bestRating = 0u;
			for(auto& format : nytl::Span<int>(formats, nformats))
			{
				if(!wglGetPixelFormatAttribivARB(dummyDC_, format, PFD_MAIN_PLANE, attribs.size(),
					attribs.data(), values.data()))
				{
					warning(errorMessage("ny::WglSetup: wglGetPixelFormatAttrib"));
					continue;
				}

				configs_.emplace_back();
				configs_.back().id = reinterpret_cast<GlConfigID>(format);
				configs_.back().red = values[1];
				configs_.back().green = values[2];
				configs_.back().blue = values[3];
				configs_.back().alpha = values[4];
				configs_.back().depth = values[5];
				configs_.back().stencil = values[6];
				configs_.back().doublebuffer = values[7];

				if(GLAD_WGL_ARB_multisample && values[8]) configs_.back().samples = values[9];

				auto rating = rate(configs_.back()) + values[0] * 50;
				if(rating >= bestRating)
				{
					bestRating = rating;
					defaultConfig_ = &configs_.back(); //configs_ wont reallocate
				}
			}
		}
		else
		{
			warning(errorMessage("ny::WglSetup: wglChoosePixelFormat"));
		}
	}
	else
	{
		warning("ny::WglSetup: extension WGL_ARB_pixel_format could not be loaded.");
	}

	if(!defaultConfig_)
	{
		auto pixelformat = 0;
		auto bestRating = 0u;

		PIXELFORMATDESCRIPTOR pfd {};
		pfd.nSize = sizeof(pfd);
		pfd.nVersion = 1;
		pfd.iLayerType = PFD_MAIN_PLANE;
		pfd.iPixelType = PFD_TYPE_RGBA;

		auto testFormat = [&](int color, int depth, int stencil, bool db) {
			pfd.cColorBits = color;
			pfd.cDepthBits = depth;
			pfd.cStencilBits = stencil;
			pfd.cAlphaBits = color - 24;

			if(db) pfd.dwFlags |= PFD_DOUBLEBUFFER;
			else pfd.dwFlags &= ~PFD_DOUBLEBUFFER;

			//We do not check for acceleration here since this function should
			//always choose the accelerated one if it is available
			pixelformat = ::ChoosePixelFormat(dummyDC_, &pfd);
			if(!pixelformat) return; //dont error here - its ok

			//first check that we dont already have this format
			auto pixelformatu = static_cast<unsigned int>(pixelformat);
			for(auto& fmt : configs_)
				if(glConfigNumber(fmt.id) == pixelformatu) return;

			//check the actual values since ChoosePixelFormat returns only the closest format
			PIXELFORMATDESCRIPTOR realPfd {};
			::DescribePixelFormat(dummyDC_, pixelformat, sizeof(realPfd), &realPfd);

			GlConfig config;
			config.depth = realPfd.cDepthBits;
			config.stencil = realPfd.cStencilBits;
			config.red = (realPfd.cColorBits - realPfd.cAlphaBits) / 3;
			config.green = (realPfd.cColorBits - realPfd.cAlphaBits) / 3;
			config.blue = realPfd.cColorBits - (config.red + config.green);
			config.alpha = realPfd.cAlphaBits;
			config.doublebuffer = realPfd.dwFlags & PFD_DOUBLEBUFFER;
			glConfigNumber(config.id) = pixelformat;

			configs_.push_back(config);

			auto rating = rate(config);
			if(rating > bestRating)
			{
				bestRating = rating;
				defaultConfig_ = &configs_.back();
			}
		};

		int colors[] = {24, 32};
		int depth[] = {16, 24, 32};
		int stencil[] = {0, 8};
		bool doubleBuffer[] = {true, false};

		configs_.reserve(2 * 3 * 2 * 2); //the possible number of configs

		for(auto c : colors)
			for(auto d : depth)
				for(auto s : stencil)
					for(auto db : doubleBuffer)
						testFormat(c, d, s, db);
	}
}

WglSetup::~WglSetup()
{
	if(dummyWindow_ && dummyDC_) ::ReleaseDC(dummyWindow_, dummyDC_);
}

WglSetup::WglSetup(WglSetup&& other) noexcept
{
	configs_ = std::move(other.configs_);
	glLibrary_ = std::move(other.glLibrary_);

	dummyWindow_ = other.dummyWindow_;
	dummyDC_ = other.dummyDC_;
	defaultConfig_ = other.defaultConfig_;

	other.dummyWindow_ = {};
	other.dummyDC_ = {};
	other.defaultConfig_ = {};
}

WglSetup& WglSetup::operator=(WglSetup&& other) noexcept
{
	if(dummyWindow_ && dummyDC_) ::ReleaseDC(dummyWindow_, dummyDC_);

	configs_ = std::move(other.configs_);
	glLibrary_ = std::move(other.glLibrary_);

	dummyWindow_ = other.dummyWindow_;
	dummyDC_ = other.dummyDC_;
	defaultConfig_ = other.defaultConfig_;

	other.dummyWindow_ = {};
	other.dummyDC_ = {};
	other.defaultConfig_ = {};

	return *this;
}

GlConfig WglSetup::defaultConfig() const
{
	return *defaultConfig_;
}

std::unique_ptr<GlContext> WglSetup::createContext(const GlContextSettings& settings) const
{
	return std::make_unique<WglContext>(*this, settings);
}

void* WglSetup::procAddr(nytl::StringParam name) const
{
	auto ret = glLibrary_.symbol(name);
	if(!ret) ret = reinterpret_cast<void*>(::wglGetProcAddress(name));
	return ret;
}

//WglSurface
bool WglSurface::apply(std::error_code& ec) const
{
	ec.clear();
	::SetLastError(0);
	if(!::SwapBuffers(hdc_))
	{
		warning(errorMessage("ny::WglContext::apply (SwapBuffer) failed"));
		ec = winapi::lastErrorCode();
		return false;
	}

	return true;
}

//WglContext
WglContext::WglContext(const WglSetup& setup, const GlContextSettings& settings)
	: setup_(&setup)
{
	::SetLastError(0);

	//test for logic errors
	if(settings.version.api != GlApi::gl)
		throw GlContextError(GlContextErrc::invalidApi, "ny::WglContext");

	auto major = settings.version.major;
	auto minor = settings.version.minor;

	if(major == 0 && minor == 0)
	{
		major = 4;
		minor = 5;
	}

	if(major < 1 || major > 4 || minor > 5)
		throw GlContextError(GlContextErrc::invalidVersion, "ny::WglContext");

	//we create our own dummyDC that is compatibly to the default dummy dc
	//and then select the chosen config (pixel format) into the dc
	auto dummyDC = ::CreateCompatibleDC(setup.dummyDC());
	if(!dummyDC) throw winapi::EC::exception("ny::WglContext: failed to create dummy dc");

	auto dcGuard = nytl::makeScopeGuard([&]{ ::DeleteDC(dummyDC); });

	if(settings.config == nullptr) config_ = setup.defaultConfig();
	else config_ = setup.config(settings.config);

	auto pixelformat = glConfigNumber(config_.id);

	PIXELFORMATDESCRIPTOR pfd {};
	if(!::DescribePixelFormat(dummyDC, pixelformat, sizeof(pfd), &pfd))
		throw GlContextError(GlContextErrc::invalidConfig, "ny::WglContext");

	if(!::SetPixelFormat(dummyDC, pixelformat, &pfd))
		throw winapi::EC::exception("ny::WglContext: failed to set pixel format");

	HGLRC share = nullptr;
	if(settings.share)
	{
		auto shareCtx = dynamic_cast<WglContext*>(settings.share);
		if(!shareCtx || shareCtx->config().id != config_.id)
			throw GlContextError(GlContextErrc::invalidSharedContext, "ny::WglContext");

		share = shareCtx->wglContext();
	}

	::SetLastError(0);

	if(GLAD_WGL_ARB_create_context)
	{
		std::vector<int> attributes;
		attributes.reserve(20);

		attributes.push_back(WGL_CONTEXT_MAJOR_VERSION_ARB);
		attributes.push_back(major);
		attributes.push_back(WGL_CONTEXT_MINOR_VERSION_ARB);
		attributes.push_back(minor);

		if(settings.compatibility)
		{
			attributes.push_back(WGL_CONTEXT_PROFILE_MASK_ARB);
			attributes.push_back(WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB);
		}

		auto flags = 0;
		if(settings.forwardCompatible) flags |= WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
		if(settings.debug) flags |= WGL_CONTEXT_DEBUG_BIT_ARB;

		if(flags)
		{
			attributes.push_back(WGL_CONTEXT_FLAGS_ARB);
			attributes.push_back(flags);
		}

		attributes.push_back(0);
		attributes.push_back(0);

		wglContext_ = ::wglCreateContextAttribsARB(dummyDC, share, attributes.data());
		if(!wglContext_ && ::GetLastError() == ERROR_INVALID_VERSION_ARB && !settings.forceVersion)
		{
			//those versions will be tried to create when the version specified in
			//the passed settings fails and the passed version should not be forced.
			constexpr std::pair<unsigned int, unsigned int> versionPairs[] =
				{{4, 5}, {3, 3}, {3, 2}, {3, 1}, {3, 0}, {1, 2}, {1, 0}};

			for(const auto& p : versionPairs)
			{
				attributes[1] = p.first;
				attributes[3] = p.second;

				wglContext_ = ::wglCreateContextAttribsARB(dummyDC, share, attributes.data());
				if(!wglContext_ && ::GetLastError() == ERROR_INVALID_VERSION_ARB) continue;
				break;
			}
		}
	}

	if(!wglContext_ && !settings.forceVersion)
	{
		//try legacy version
		wglContext_ = ::wglCreateContext(dummyDC);
		if(share && !::wglShareLists(share, wglContext_))
		{
			::wglDeleteContext(wglContext_);
			throw winapi::EC::exception("ny::WglContext: wglShareLists");
		}
	}

	if(!wglContext_)
		throw winapi::EC::exception("ny::WglContext: failed to create context");

	GlContext::initContext(settings.version.api, config_, settings.share);
}

WglContext::~WglContext()
{
	if(wglContext_)
	{
		std::error_code ec;
		if(!makeNotCurrent(ec))
			warning("ny::~WglContext: failed to make the context not current: ", ec.message());

		::wglDeleteContext(wglContext_);
	}
}

bool WglContext::makeCurrentImpl(const GlSurface& surf, std::error_code& ec)
{
	ec.clear();

	::SetLastError(0);
	auto wglSurface = dynamic_cast<const WglSurface*>(&surf);
	if(!::wglMakeCurrent(wglSurface->hdc(), wglContext_))
	{
		warning(errorMessage("ny::WglContext::makeCurrentImpl (wglMakeCurrent) failed"));
		ec = winapi::lastErrorCode();
		return false;
	}

	return true;
}

bool WglContext::makeNotCurrentImpl(std::error_code& ec)
{
	ec.clear();

	::SetLastError(0);
	if(!::wglMakeCurrent(nullptr, nullptr))
	{
		warning(errorMessage("ny::WglContext::makeNotCurrentImpl (wglMakeCurrent) failed"));
		ec = winapi::lastErrorCode();
		return false;
	}

	return true;
}

GlContextExtensions WglContext::contextExtensions() const
{
	GlContextExtensions ret;
	if(GLAD_WGL_EXT_swap_control) ret |= GlContextExtension::swapControl;
	// if(GLAD_WGL_EXT_swap_control_tear) ret |= GlContextExtension::swapControlTear;
	return ret;
}

bool WglContext::swapInterval(int interval, std::error_code& ec) const
{
	if(!GLAD_WGL_EXT_swap_control || !::wglSwapIntervalEXT)
	{
		ec = {GlContextErrc::extensionNotSupported};
		return false;
	}

	if(!::wglSwapIntervalEXT(interval))
	{
		ec = winapi::lastErrorCode();
		return false;
	}

	return true;
}

bool WglContext::compatible(const GlSurface& surface) const
{
	if(!GlContext::compatible(surface)) return false;

	auto wglSurface = dynamic_cast<const WglSurface*>(&surface);
	return (wglSurface);
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
	auto pixelformat = glConfigNumber(settings.gl.config);
	if(!settings.gl.config) pixelformat = glConfigNumber(setup.defaultConfig().id);

	PIXELFORMATDESCRIPTOR pfd {};
	if(!::DescribePixelFormat(hdc_, pixelformat, sizeof(pfd), &pfd))
		throw GlContextError(GlContextErrc::invalidConfig, "ny::WglWC");

	if(!::SetPixelFormat(hdc_, pixelformat, &pfd))
		throw winapi::EC::exception("ny::WglWC: failed to set pixel format");

	surface_.reset(new WglSurface(hdc_, setup.config(glConfigID(pixelformat))));
	if(settings.gl.storeSurface) *settings.gl.storeSurface = surface_.get();
}

WglWindowContext::~WglWindowContext()
{
	surface_.reset();
	if(hdc_) ::ReleaseDC(handle(), hdc_);
}

WNDCLASSEX WglWindowContext::windowClass(const WinapiWindowSettings& settings)
{
	auto ret = WinapiWindowContext::windowClass(settings);
	ret.style |= CS_OWNDC;
	return ret;
}

Surface WglWindowContext::surface()
{
	return {*surface_};
}

}
