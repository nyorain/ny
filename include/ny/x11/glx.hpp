// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/x11/include.hpp>
#include <ny/x11/windowContext.hpp>
#include <ny/common/gl.hpp>
#include <nytl/vec.hpp>

// prototypes to not include glx.h
typedef struct __GLXcontextRec* GLXContext;
typedef struct __GLXFBConfigRec* GLXFBConfig;

namespace ny {

/// Glx GlSetup implementation.
class GlxSetup : public GlSetup {
public:
	GlxSetup() = default;
	GlxSetup(const X11AppContext&, unsigned int screenNumber);
	~GlxSetup() = default;

	GlxSetup(GlxSetup&&) noexcept;
	GlxSetup& operator=(GlxSetup&&) noexcept;

	GlConfig defaultConfig() const override { return defaultConfig_; }
	GlConfig defaultTransparentConfig() const { return defaultTransparentConfig_; }

	std::vector<GlConfig> configs() const override { return configs_; }

	std::unique_ptr<GlContext> createContext(const GlContextSettings& = {}) const override;
	void* procAddr(const char* name) const override;

	GLXFBConfig glxConfig(GlConfigID id) const;
	unsigned int visualID(GlConfigID id) const;

	const X11AppContext& appContext() const { return *appContext_; }
	Display& xDisplay() const;

	bool valid() const { return (appContext_); }

protected:
	const X11AppContext* appContext_ {};
	std::vector<GlConfig> configs_;

	GlConfig defaultConfig_ {};
	GlConfig defaultTransparentConfig_ {};
};

/// Glx GlSurface implementation
class GlxSurface : public GlSurface {
public:
	GlxSurface(const GlxSetup&, X11WindowContext& ctx, const GlConfig&);
	GlxSurface(const GlxSetup&, unsigned int xDrawable, const GlConfig&);
	~GlxSurface();

	NativeHandle nativeHandle() const override { return {xDrawable_}; }
	GlConfig config() const override { return config_; }
	bool apply(std::error_code&) const override;

	unsigned int xDrawable() const { return xDrawable_; }

	const GlxSetup& setup() const { return setup_; }
	const X11AppContext& appContext() const { return setup().appContext(); }
	Display& xDisplay() const { return setup().xDisplay(); }

protected:
	const GlxSetup& setup_;
	unsigned int xDrawable_ {};
	GlConfig config_ {};
	X11WindowContext* wc_ {};
};

/// Glx GlContext implementation
class GlxContext : public GlContext {
public:
	GlxContext(const GlxSetup& setup, const GlContextSettings& settings);
	GlxContext(const GlxSetup& setup, GLXContext context, const GlConfig& config);
	~GlxContext();

	NativeHandle nativeHandle() const override { return {glxContext_}; }
	bool compatible(const GlSurface&) const override;
	GlContextExtensions contextExtensions() const override;
	bool swapInterval(int interval, std::error_code&) const override;

	const GlxSetup& setup() const { return setup_; }
	const X11AppContext& appContext() const { return setup().appContext(); }
	Display& xDisplay() const { return setup().xDisplay(); }
	GLXContext glxContext() const { return glxContext_; }

protected:
	virtual bool makeCurrentImpl(const GlSurface&, std::error_code&) override;
	virtual bool makeNotCurrentImpl(std::error_code&) override;

protected:
	const GlxSetup& setup_;
	GLXContext glxContext_ {};
};

/// X11 WindowContext implementation that also creates a GlSurface using glx.
class GlxWindowContext : public X11WindowContext {
public:
	GlxWindowContext(X11AppContext&, const GlxSetup& setup, const X11WindowSettings& = {});

	Surface surface() override;
	GlxSurface& glxSurface() const { return *surface_; }

protected:
	std::unique_ptr<GlxSurface> surface_;
};

} // namespace ny
