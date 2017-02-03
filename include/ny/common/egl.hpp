// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/fwd.hpp>
#include <ny/config.hpp>

#ifndef NY_WithEgl
	#error ny was built without egl. Do not include this header.
#endif

#include <ny/common/gl.hpp>

#include <string>
#include <vector>
#include <system_error>

using EGLDisplay = void*; //Singleton per backend. Connection
using EGLConfig = void*; //Represents a context setup. May have more than one
using EGLContext = void*; //The actual context. May have more than one
using EGLSurface = void*; //One surface needed per WindowContext

namespace ny {

/// EGL GlSetup implementation
class EglSetup : public GlSetup {
public:
	EglSetup() = default;
	EglSetup(void* nativeDisplay);
	~EglSetup();

	EglSetup(EglSetup&& other) noexcept;
	EglSetup& operator=(EglSetup&& other) noexcept;

	GlConfig defaultConfig() const override { return defaultConfig_; }
	GlConfig defaultTransparentConfig() const { return defaultTransparentConfig_; }
	std::vector<GlConfig> configs() const override { return configs_; }

	std::unique_ptr<GlContext> createContext(const GlContextSettings& = {}) const override;
	void* procAddr(nytl::StringParam name) const override;

	/// Returns the EGLConfig for the given GlConfigId.
	/// If the given id is invalid returns nullptr.
	EGLConfig eglConfig(GlConfigID id) const;
	EGLDisplay eglDisplay() const { return eglDisplay_; }

	bool valid() const { return (eglDisplay_); }

protected:
	EGLDisplay eglDisplay_ {};

	std::vector<GlConfig> configs_;
	GlConfig defaultConfig_ {};
	GlConfig defaultTransparentConfig_ {};
};

/// EGL GlSurface implementation
class EglSurface : public GlSurface {
public:
	EglSurface(EGLDisplay, void* nativeWindow, GlConfigID, const EglSetup&);
	EglSurface(EGLDisplay, void* nativeWindow, const GlConfig&, EGLConfig eglConfig);
	virtual ~EglSurface();

	NativeHandle nativeHandle() const override { return {eglSurface_}; }
	GlConfig config() const override { return config_; }
	bool apply(std::error_code&) const override;

	EGLDisplay eglDisplay() const { return eglDisplay_; }
	EGLSurface eglSurface() const { return eglSurface_; }

protected:
	EGLDisplay eglDisplay_ {};
	EGLSurface eglSurface_ {};
	GlConfig config_;
};

/// EGL GlContext implementation
class EglContext : public GlContext {
public:
	EglContext(const EglSetup& setup, const GlContextSettings& = {});
	virtual ~EglContext();

	NativeHandle nativeHandle() const override { return {eglContext_}; }
	bool compatible(const GlSurface&) const override;
	GlContextExtensions contextExtensions() const override;
	bool swapInterval(int interval, std::error_code&) const override;

	EGLDisplay eglDisplay() const { return (setup_) ? setup_->eglDisplay() : nullptr; }
	EGLContext eglContext() const { return eglContext_; }
	EGLConfig eglConfig() const;

protected:
	bool makeCurrentImpl(const GlSurface&, std::error_code&) override;
	bool makeNotCurrentImpl(std::error_code&) override;

protected:
	const EglSetup* setup_ {};
	EGLContext eglContext_ {};
};

} // namespace ny
