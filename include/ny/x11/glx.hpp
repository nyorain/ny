#pragma once

#include <ny/x11/include.hpp>
#include <ny/x11/windowContext.hpp>
#include <ny/common/gl.hpp>
#include <ny/library.hpp>
#include <nytl/vec.hpp>

//prototypes to include glx.h
typedef struct __GLXcontextRec* GLXContext;
typedef struct __GLXFBConfigRec* GLXFBConfig;

namespace ny
{

//TODO: for glx calls: correct error handling

//TODO: api loading, glLibrary, context extensions, swapInterval, screen number
class GlxSetup : public GlSetup
{
public:
	GlxSetup() = default;
	GlxSetup(const X11AppContext&, unsigned int screenNumber = 0);
	~GlxSetup() = default;

	GlxSetup(GlxSetup&&) noexcept;
	GlxSetup& operator=(GlxSetup&&) noexcept;

	GlConfig defaultConfig() const override { return *defaultConfig_; }
	std::vector<GlConfig> configs() const override { return configs_; }

	std::unique_ptr<GlContext> createContext(const GlContextSettings& = {}) const override;
	void* procAddr(nytl::StringParam name) const override;

	GLXFBConfig glxConfig(GlConfigId id) const;
	unsigned int visualID(GlConfigId id) const;
	Display* xDisplay() const { return xDisplay_; }

	bool valid() const { return (xDisplay_); }

protected:
	Display* xDisplay_ {};

	std::vector<GlConfig> configs_;
	GlConfig* defaultConfig_ {};

	//TODO
	Library glLibrary_;
};

class GlxSurface : public GlSurface
{
public:
	GlxSurface(Display& xdpy, unsigned int xDrawable, const GlConfig& config);
	~GlxSurface() = default;

	NativeHandle nativeHandle() const override { return {xDrawable_}; }
	GlConfig config() const override { return config_; }
	bool apply(std::error_code&) const override;

	unsigned int xDrawable() const { return xDrawable_; }
	Display* xDisplay() const { return xDisplay_; }

protected:
	Display* xDisplay_ {};
	unsigned int xDrawable_ {};
	GlConfig config_ {};
};

class GlxContext : public GlContext
{
public:
	GlxContext(const GlxSetup& setup, const GlContextSettings& settings);
    GlxContext(const GlxSetup& setup, GLXContext context, const GlConfig& config);
    ~GlxContext();

	NativeHandle nativeHandle() const override { return {glxContext_}; }
	bool compatible(const GlSurface&) const override;
	GlContextExtensions contextExtensions() const override;
	bool swapInterval(int interval, std::error_code&) const override;

	Display* xDisplay() const { return (setup_) ? setup_->xDisplay() : nullptr; }
	GLXContext glxContext() const { return glxContext_; }

protected:
    virtual bool makeCurrentImpl(const GlSurface&, std::error_code&) override;
    virtual bool makeNotCurrentImpl(std::error_code&) override;

protected:
	const GlxSetup* setup_ {};
    GLXContext glxContext_ {};
};

class GlxWindowContext : public X11WindowContext
{
public:
	GlxWindowContext(X11AppContext&, const GlxSetup& setup, const X11WindowSettings& = {});

	Surface surface() override;
	GlxSurface& glxSurface() const { return *surface_; }

protected:
	std::unique_ptr<GlxSurface> surface_;
};

}
