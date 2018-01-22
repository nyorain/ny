// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/winapi/include.hpp>
#include <ny/winapi/windowContext.hpp>
#include <ny/winapi/windows.hpp>
#include <ny/common/gl.hpp>

namespace ny {

/// Wgl GlSetup implementation
class WglSetup : public GlSetup {
public:
	WglSetup() = default;
	WglSetup(HWND dummy);
	~WglSetup();

	WglSetup(WglSetup&& other) noexcept;
	WglSetup& operator=(WglSetup&& other) noexcept;

	GlConfig defaultConfig() const override { return defaultConfig_; }
	std::vector<GlConfig> configs() const override { return configs_; }
	std::unique_ptr<GlContext> createContext(const GlContextSettings& = {}) const override;
	void* procAddr(const char* name) const override;

	HDC dummyDC() const { return dummyDC_; }
	bool valid() const { return (dummyDC_); }

protected:
	std::vector<GlConfig> configs_;
	GlConfig defaultConfig_ {};
	HWND dummyWindow_ {};
	HDC dummyDC_ {};
};

/// Wgl GlSurface implementation
class WglSurface : public GlSurface {
public:
	WglSurface(HDC hdc, const GlConfig& config) : hdc_(hdc), config_(config) {}
	~WglSurface();

	NativeHandle nativeHandle() const override { return {hdc_}; }
	GlConfig config() const override { return config_; }
	bool apply(std::error_code& ec) const override;

	HDC hdc() const { return hdc_; }

protected:
	HDC hdc_;
	GlConfig config_;
};

/// Wgl GlContext implementation.
class WglContext : public GlContext {
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

/// Winapi WindowContext implementation that also creates a GlSurface using wgl.
class WglWindowContext : public WinapiWindowContext {
public:
	WglWindowContext(WinapiAppContext&, WglSetup&, const WinapiWindowSettings& = {});
	~WglWindowContext();

	Surface surface() override;

protected:
	WNDCLASSEX windowClass(const WinapiWindowSettings& settings) override;

protected:
	std::unique_ptr<WglSurface> surface_;
	HDC hdc_ {};
};

} // namespace ny
