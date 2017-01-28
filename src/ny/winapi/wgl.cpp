// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/winapi/wgl.hpp>
#include <ny/winapi/windowContext.hpp>
#include <ny/winapi/appContext.hpp>
#include <ny/winapi/util.hpp>
// #include <ny/winapi/wglApi.h>

#include <ny/surface.hpp>
#include <ny/log.hpp>

#include <nytl/scope.hpp>

#include <Wingdi.h>

#include <thread>
#include <mutex>

// we assume that the wgl* function pointers are the same for all
// contexts and pixel formats. This assumption might not be correct in all
// cases but there is NO sane way to assume something else.

// TODO: add support for implementation that do not support choosePixelFormatARB?
// see git history for (rather bad) implementation

namespace ny {
namespace ext {

// Extension functions typedefs
using PfnWglCreateContextAttribsARB = HGLRC (*)(HDC, HGLRC, const int*);
using PfnWglSwapIntervalEXT = BOOL (*)(int);
using PfnWglGetExtensionStringARB = const char* (*)(HDC);
		hasProfile = glExtensionStringContains(extensions, "WGL_EXT_create_context_profile");
using PfnWglGetPixelFormatAttribivARB = BOOL(*)(HDC, int, int, UINT, const int*, int*);
using PfnWglChoosePixelFormatARB =  BOOL (*)(HDC, const int*,
	const FLOAT*, UINT, int*, UINT*);

// Extension function pointers
// loaded exactly once in a threadsafe manner
PfnWglGetExtensionStringARB getExtensionStringARB {};
PfnWglChoosePixelFormatARB choosePixelFormatARB {};
PfnWglGetPixelFormatAttribivARB getPixelFormatAttribivARB {};
PfnWglCreateContextAttribsARB createContextAttribsARB {};
PfnWglSwapIntervalEXT swapIntervalEXT {};

// all supported extensions string
const char* extensions;

// bool flags only for extensions that have to functions pointers
bool hasMultisample;
bool hasSwapControlTear;
bool hasProfileES;

/// Tries to load the needed setup function pointers
/// Returns false if initialization fails
/// Will only initialize the functions pointer once in a threadsafe manner
/// Expects a wgl context to be current for the given hdc.
bool loadExtensions(HDC hdc)
{
	static std::once_flag of {};
	static bool valid {};

	// simply fail if the basic extensions are not supported
	// they usually are, even on ancient implementations
	std::call_once(of, [&] {
		auto ptr = ::wglGetProcAddress("wglGetExtensionsStringARB");
		if(!ptr) {
			warning("ny::Wgl::initChoosePixelFormat: could not load wglGetExtensionsString");
			return;
		}

		getExtensionStringARB = reinterpret_cast<PfnWglGetExtensionStringARB>(ptr);
		extensions = getExtensionStringARB(hdc);

		auto pixelFormat = glExtensionStringContains(extensions, "WGL_ARB_pixel_format");
		auto contextAttribs = glExtensionStringContains(extensions, "WGL_ARB_create_context");
		auto swapControl = glExtensionStringContains(extensions, "WGL_EXT_swap_control");

		hasMultisample = glExtensionStringContains(extensions, "GLX_ARB_multisample");
		hasSwapControlTear = glExtensionStringContains(extensions, "GLX_ARB_swap_control_tear");
		hasProfile = glExtensionStringContains(extensions, "WGL_EXT_create_context_profile");
		hasProfileES = glExtensionStringContains(extensions, "WGL_EXT_create_context_es2_profile");

		using PfnVoid = void (*)();
		struct LoadFunc {
			const char* name;
			PfnVoid& func;
			bool load {true};
		} funcs[] = {
			{"wglChoosePixelFormatARB", (PfnVoid&) choosePixelFormatARB, pixelFormat},
			{"wglGetPixelFormatAttribivARB", (PfnVoid&) getPixelFormatAttribivARB, pixelFormat},
			{"wglCeateContextAttribsARB", (PfnVoid&) createContextAttribsARB, contextAttribs},
			{"wglSwapIntervalEXT", (PfnVoid&) swapIntervalEXT, swapControl},
		};

		for(const auto& f : funcs)
			f.func = f.load ? reinterpret_cast<PfnVoid>(::wglGetProcAddress(f.name)) : nullptr;

		valid = true;
	});

	return valid;
}

#define WGL_CONTEXT_DEBUG_BIT_ARB 0x00000001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x00000002
#define WGL_CONTEXT_ES2_PROFILE_BIT_EXT	0x00000004
#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB 0x2093
#define WGL_CONTEXT_FLAGS_ARB 0x2094
#define ERROR_INVALID_VERSION_ARB 0x2095
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#define ERROR_INVALID_PROFILE_ARB 0x2096
#define WGL_SAMPLE_BUFFERS_ARB 0x2041
#define WGL_SAMPLES_ARB 0x2042
#define WGL_NUMBER_PIXEL_FORMATS_ARB 0x2000
#define WGL_DRAW_TO_WINDOW_ARB 0x2001
#define WGL_DRAW_TO_BITMAP_ARB 0x2002
#define WGL_ACCELERATION_ARB 0x2003
#define WGL_TRANSPARENT_ARB 0x200A
#define WGL_SHARE_DEPTH_ARB 0x200C
#define WGL_SHARE_STENCIL_ARB 0x200D
#define WGL_SHARE_ACCUM_ARB 0x200E
#define WGL_SUPPORT_GDI_ARB 0x200F
#define WGL_SUPPORT_OPENGL_ARB 0x2010
#define WGL_DOUBLE_BUFFER_ARB 0x2011
#define WGL_STEREO_ARB 0x2012
#define WGL_PIXEL_TYPE_ARB 0x2013
#define WGL_COLOR_BITS_ARB 0x2014
#define WGL_RED_BITS_ARB 0x2015
#define WGL_RED_SHIFT_ARB 0x2017
#define WGL_GREEN_BITS_ARB 0x2017
#define WGL_GREEN_SHIFT_ARB 0x2018
#define WGL_BLUE_BITS_ARB 0x2019
#define WGL_BLUE_SHIFT_ARB 0x201A
#define WGL_ALPHA_BITS_ARB 0x201B
#define WGL_ALPHA_SHIFT_ARB 0x201C
#define WGL_DEPTH_BITS_ARB 0x2022
#define WGL_STENCIL_BITS_ARB 0x2023
#define WGL_AUX_BUFFERS_ARB 0x2024
#define WGL_NO_ACCELERATION_ARB 0x2025
#define WGL_GENERIC_ACCELERATION_ARB 0x2026
#define WGL_FULL_ACCELERATION_ARB 0x2027
#define WGL_TYPE_RGBA_ARB 0x202B

} // namespace ext

//WglSetup
WglSetup::WglSetup(HWND dummy) : dummyWindow_(dummy)
{
	dummyDC_ = ::GetDC(dummyWindow_);

	// we set the pixel format for the dummy dc and then create an opengl context for it
	// this context is used to load all the basic required functions
	// note that we load extensions functions like wglChoosePixelFormatARB that work
	// independent from a current context
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

	HGLRC dummyContext = ::wglCreateContext(dummyDC_);
	if(!dummyContext)
		throw winapi::lastErrorException("ny::WglSetup: unable to create dummy context");

	{
		::wglMakeCurrent(dummyDC_, dummyContext);
		auto notCurrentGuard = nytl::makeScopeGuard([&]{
			::wglMakeCurrent(nullptr, nullptr);
			::wglDeleteContext(dummyContext);
		});

		if(!ext::loadExtensions(dummyDC_))
			throw std::runtime_error("ny::WglSetup: failed ot load wglChoosePixelFormat");
	}

	// we require the pixel format extensions
	// should be supported everywhere
	if(!ext::choosePixelFormatARB || !ext::getPixelFormatAttribivARB)
		throw std::runtime_error("ny::WglSetup: pixel_format wgl extension is required");

	// now enumerate all gl configs
	// if the extension is available retrieve all formats with the required attributes
	// otherwise just retrieve and parse parse a few formats
	int pfAttribs[] = {
		WGL_DRAW_TO_WINDOW_ARB, true,
		WGL_SUPPORT_OPENGL_ARB, true,
		0, 0
	};

	int formats[1024] {};
	UINT nformats = 0;
	bool ret = ext::choosePixelFormatARB(dummyDC_, pfAttribs, nullptr, 1024, formats, &nformats);

	if(!ret || nformats == 0) {
		auto message = winapi::errorMessage("ny::WglSetup: wglChoosePixelFormat");
		throw std::runtime_error(message);
	}

	std::vector<int> attribs = {
		WGL_ACCELERATION_ARB,
		WGL_RED_BITS_ARB,
		WGL_GREEN_BITS_ARB,
		WGL_BLUE_BITS_ARB,
		WGL_ALPHA_BITS_ARB,
		WGL_DEPTH_BITS_ARB,
		WGL_STENCIL_BITS_ARB,
		WGL_DOUBLE_BUFFER_ARB,
		WGL_TRANSPARENT_ARB // id: 8
	};

	if(ext::hasMultisample) {
		attribs.push_back(WGL_SAMPLE_BUFFERS_ARB);
		attribs.push_back(WGL_SAMPLES_ARB); // id: 10
	}

	configs_.reserve(nformats);

	std::vector<int> values;
	values.resize(attribs.size());

	auto bestRating = 0u;
	auto bestTransparentRating = 0u;

	for(auto& format : nytl::Span<int>(formats, nformats)) {
		auto succ = ext::getPixelFormatAttribivARB(dummyDC_, format, PFD_MAIN_PLANE,
			attribs.size(), attribs.data(), values.data());

		if(!succ) {
			warning(winapi::errorMessage("ny::WglSetup: wglGetPixelFormatAttrib"));
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
		configs_.back().transparent = values[8];

		if(ext::hasMultisample && values[9])
			configs_.back().samples = values[10];

		auto rating = rate(configs_.back()) + values[0] * 50;
		if(rating > bestRating) {
			bestRating = rating;
			defaultConfig_ = configs_.back();
		}

		if(configs_.back().transparent && rating > bestTransparentRating) {
			bestTransparentRating = rating;
			defaultTransparentConfig_ = configs.back();
		}
	}
}

WglSetup::~WglSetup()
{
	if(dummyWindow_ && dummyDC_)
		::ReleaseDC(dummyWindow_, dummyDC_);
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

// WglSurface
bool WglSurface::apply(std::error_code& ec) const
{
	ec.clear();
	::SetLastError(0);
	if(!::SwapBuffers(hdc_)) {
		warning(winapi::errorMessage("ny::WglContext::apply (SwapBuffer) failed"));
		ec = winapi::lastErrorCode();
		return false;
	}

	return true;
}

// WglContext
WglContext::WglContext(const WglSetup& setup, const GlContextSettings& settings)
	: setup_(&setup)
{
	::SetLastError(0);

	// test for config errors
	auto major = settings.version.major;
	auto minor = settings.version.minor;
	auto api = settings.version.api;

	if(settings.forceVersion && !ext::createContextAttribsARB)
		throw std::runtime_error("ny::WglContext: need createContext ext to force version");

	if(api == GlApi::gl && !ext::hasProfileES) {
		if(!settings.forceVersion) api = GlApi::gl;
		else throw std::runtime_error("ny::WglContext: cannot force gles context");
	}

	// we create our own dummyDC that is compatibly to the default dummy dc
	// and then select the chosen config (pixel format) into the dc
	auto dummyDC = ::CreateCompatibleDC(setup.dummyDC());
	if(!dummyDC) throw winapi::lastErrorException("ny::WglContext: failed to create dummy dc");

	auto dcGuard = nytl::makeScopeGuard([&]{ ::DeleteDC(dummyDC); });

	if(settings.config == nullptr) config_ = setup.defaultConfig();
	else config_ = setup.config(settings.config);

	auto pixelformat = glConfigNumber(config_.id);

	PIXELFORMATDESCRIPTOR pfd {};
	if(!::DescribePixelFormat(dummyDC, pixelformat, sizeof(pfd), &pfd))
		throw GlContextError(GlContextErrc::invalidConfig, "ny::WglContext");

	if(!::SetPixelFormat(dummyDC, pixelformat, &pfd))
		throw winapi::lastErrorException("ny::WglContext: failed to set pixel format");

	HGLRC share = nullptr;
	if(settings.share) {
		auto shareCtx = dynamic_cast<WglContext*>(settings.share);
		if(!shareCtx || shareCtx->config().id != config_.id)
			throw GlContextError(GlContextErrc::invalidSharedContext, "ny::WglContext");

		share = shareCtx->wglContext();
	}

	::SetLastError(0);

	if(ext::createContextAttribsARB) {
		std::vector<int> attributes;
		attributes.reserve(20);

		std::vector<std::pair<unsigned int, unsigned int>> versionPairs;

		// switch default versions and checked version pairs
		// also perform version sanity checks
		if(api == GlApi::gles) {
			if(major == 0 && minor == 0) {
				major = 3;
				minor = 2;
			}

			versionPairs = {{3, 2}, {3, 1}, {3, 0}, {2, 0}, {1, 1}, {1, 0}};
			if(major < 1 || major > 3 || minor > 2)
				throw GlContextError(GlContextErrc::invalidVersion, "ny::WglContext");

		} else {
			if(major == 0 && minor == 0) {
				major = 4;
				minor = 5;
			}

			versionPairs = {{4, 5}, {3, 3}, {3, 2}, {3, 1}, {3, 0}, {1, 2}, {1, 0}};
			if(major < 1 || major > 4 || minor > 5)
				throw GlContextError(GlContextErrc::invalidVersion, "ny::WglContext");
		}

		attributes.push_back(WGL_CONTEXT_MAJOR_VERSION_ARB);
		attributes.push_back(major);
		attributes.push_back(WGL_CONTEXT_MINOR_VERSION_ARB);
		attributes.push_back(minor);

		if(ext::hasProfile) {
			attributes.push_back(WGL_CONTEXT_PROFILE_MASK_ARB);

			if(ext::hasProfileES && api == GlApi::gles)
				attributes.push_back(WGL_CONTEXT_ES2_PROFILE_BIT_EXT);
			else if(settings.compatibility)
				attributes.push_back(WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB);
			else
				attributes.push_back(GLX_CONTEXT_CORE_PROFILE_BIT_ARB);
		}

		auto flags = 0;
		if(settings.forwardCompatible) flags |= WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
		if(settings.debug) flags |= WGL_CONTEXT_DEBUG_BIT_ARB;

		if(flags) {
			attributes.push_back(WGL_CONTEXT_FLAGS_ARB);
			attributes.push_back(flags);
		}

		// double null-terminated
		attributes.push_back(0);
		attributes.push_back(0);

		wglContext_ = ext::createContextAttribsARB(dummyDC, share, attributes.data());

		// those versions will be tried to create when the version specified in
		// the passed settings fails and the passed version should not be forced.
		bool tryAgain = settings.forceVersion;
		if(!wglContext_ && ::GetLastError() == ERROR_INVALID_VERSION_ARB && tryAgain) {
			for(const auto& p : versionPairs) {
				attributes[1] = p.first;
				attributes[3] = p.second;

				wglContext_ = ext::createContextAttribsARB(dummyDC, share, attributes.data());
				if(!wglContext_ && ::GetLastError() == ERROR_INVALID_VERSION_ARB) continue;
				break;
			}
		}
	}

	// try legacy version for gl api
	if(!wglContext_ && !settings.forceVersion) {
		wglContext_ = ::wglCreateContext(dummyDC);
		if(share && !::wglShareLists(share, wglContext_)) {
			::wglDeleteContext(wglContext_);
			throw winapi::lastErrorException("ny::WglContext: wglShareLists failed");
		}
	}

	if(!wglContext_)
		throw winapi::lastErrorException("ny::WglContext: failed to create context");

	GlContext::initContext(api, config_, settings.share);
}

WglContext::~WglContext()
{
	if(wglContext_) {
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
	if(!::wglMakeCurrent(wglSurface->hdc(), wglContext_)) {
		warning(winapi::errorMessage("ny::WglContext::makeCurrentImpl: wglMakeCurrent failed"));
		ec = winapi::lastErrorCode();
		return false;
	}

	return true;
}

bool WglContext::makeNotCurrentImpl(std::error_code& ec)
{
	ec.clear();

	::SetLastError(0);
	if(!::wglMakeCurrent(nullptr, nullptr)) {
		warning(winapi::errorMessage("ny::WglContext::makeNotCurrentImpl: wglMakeCurrent failed"));
		ec = winapi::lastErrorCode();
		return false;
	}

	return true;
}

GlContextExtensions WglContext::contextExtensions() const
{
	GlContextExtensions ret;
	if(ext::swapIntervalEXT) ret |= GlContextExtension::swapControl;
	if(ext::hasSwapControlTear) ret |= GlContextExtension::swapControlTear;
	return ret;
}

bool WglContext::swapInterval(int interval, std::error_code& ec) const
{
	if(!ext::swapIntervalEXT) {
		ec = {GlContextErrc::extensionNotSupported};
		return false;
	}

	if(!ext::swapIntervalEXT(interval)) {
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

// WglWindowContext
WglWindowContext::WglWindowContext(WinapiAppContext& ac, WglSetup& setup,
	const WinapiWindowSettings& settings)
{
	appContext_ = &ac;

	WinapiWindowContext::initWindowClass(settings);
	WinapiWindowContext::setStyle(settings);
	WinapiWindowContext::initWindow(settings);
	WinapiWindowContext::showWindow(settings);

	hdc_ = ::GetDC(handle());

	auto config = GlConfig {};
	if(!settings.gl.config) {
		if(!settings.transparent) config = setup.defaultConfig();
		else config.setup.defaultTransparentConfig();
	} else {
		config = settings.config(glConfigID(config));
	}

	auto pixelformat = glConfigNumber(config.id);

	PIXELFORMATDESCRIPTOR pfd {};
	if(!::DescribePixelFormat(hdc_, pixelformat, sizeof(pfd), &pfd))
		throw GlContextError(GlContextErrc::invalidConfig, "ny::WglWC");

	if(!::SetPixelFormat(hdc_, pixelformat, &pfd))
		throw winapi::lastErrorException("ny::WglWC: failed to set pixel format");

	surface_.reset(new WglSurface(hdc_, config));
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

} // namespace ny
