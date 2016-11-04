#pragma once

#include <ny/backend/winapi/include.hpp>
#include <ny/backend/winapi/windowContext.hpp>
#include <ny/backend/winapi/windows.hpp>

#include <ny/backend/common/gl.hpp>
#include <ny/backend/common/library.hpp>

namespace ny
{

class WglSetup : public GlSetup
{
public:
	WglSetup() = default;
	WglSetup(HWND dummy);
	~WglSetup();

	WglSetup(WglSetup&& other) noexcept;
	WglSetup& operator=(WglSetup&& other) noexcept;

	GlConfig defaultConfig() const override;
	std::vector<GlConfig> configs() const override { return configs_; }
	std::unique_ptr<GlContext> createContext(const GlContextSettings& = {}) const override;
	void* procAddr(nytl::StringParam name) const;

	const Library& glLibrary() const { return glLibrary_; }
	HDC dummyDC() const { return dummyDC_; }
	bool valid() const { return (dummyDC_); }

protected:
	std::vector<GlConfig> configs_;
	GlConfig* defaultConfig_ {};
	Library glLibrary_;

	HWND dummyWindow_ {};
	HDC dummyDC_ {};
};

class WglSurface : public GlSurface
{
public:
	WglSurface(HDC hdc, const GlConfig& config) : hdc_(hdc), config_(config) {}
	~WglSurface() = default;

	NativeHandle nativeHandle() const override { return {hdc_}; }
	GlConfig config() const override { return config_; }
	bool apply(std::error_code& ec) const override;

	HDC hdc() const { return hdc_; }

protected:
	HDC hdc_;
	GlConfig config_;
};

///OpenGL context implementation using the wgl api on windows.
class WglContext : public GlContext
{
public:
	WglContext(const WglSetup& setup, const GlContextSettings& settings = {});
	virtual ~WglContext();

	NativeHandle nativeHandle() const override { return {wglContext_}; }
	GlContextExtensions contextExtensions() const override;
	bool swapInterval(int interval, std::error_code& ec) const override;
	bool compatible(const GlSurface&) const override;

	HGLRC wglContext() const { return wglContext_; }

protected:
	bool makeCurrentImpl(const GlSurface&, std::error_code& ec) override;
	bool makeNotCurrentImpl(std::error_code& ec) override;

protected:
	const WglSetup* setup_ {};
	HGLRC wglContext_ {};
};

///Winapi WindowContext using wgl (OpenGL) to draw.
class WglWindowContext : public WinapiWindowContext
{
public:
	WglWindowContext(WinapiAppContext&, WglSetup&, const WinapiWindowSettings& = {});
	~WglWindowContext();

	bool surface(Surface&) override;
	bool drawIntegration(WinapiDrawIntegration*) override { return false; }

protected:
	WNDCLASSEX windowClass(const WinapiWindowSettings& settings) override;

protected:
	std::unique_ptr<WglSurface> surface_;
	HDC hdc_ {};
};

}
