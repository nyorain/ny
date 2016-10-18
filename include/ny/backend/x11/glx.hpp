#pragma once

#include <ny/backend/x11/include.hpp>
#include <ny/backend/x11/windowContext.hpp>
#include <ny/backend/common/gl.hpp>
#include <nytl/vec.hpp>

//prototypes to include glx.h
typedef struct __GLXcontextRec* GLXContext;
typedef struct __GLXFBConfigRec* GLXFBConfig;

namespace ny
{

// RAII wrapper around GLXContext.
class GlxContextWrapper
{
public:
	Display* xDisplay;
	GLXContext context;

public:
	GlxContextWrapper() = default;
	GlxContextWrapper(Display*, GLXFBConfig);
	~GlxContextWrapper();

	GlxContextWrapper(GlxContextWrapper&& other) noexcept;
	GlxContextWrapper& operator=(GlxContextWrapper&& other) noexcept;
};

///GlContext implementation that associates a GLXContext with an x11 window.
///Does neither own the GLXContext nor the x drawable it holds.
class GlxContext : public GlContext
{
public:
    GlxContext(Display* dpy, unsigned int drawable, GLXContext, GLXFBConfig = nullptr);
    ~GlxContext();

    bool apply() override;
	void* procAddr(const char* name) const override;

protected:
	Display* xDisplay_ {};
	unsigned int drawable_ {};
    GLXContext glxContext_ {};

	// std::uint32_t glxWindow_;

protected:
    virtual bool makeCurrentImpl() override;
    virtual bool makeNotCurrentImpl() override;

protected:
	static void* glLibHandle();
};

///WindowContext implementation on an x11 backend with opengl (glx) used for rendering.
class GlxWindowContext : public X11WindowContext
{
public:
	GlxWindowContext(X11AppContext&, const X11WindowSettings&);

	bool drawIntegration(X11DrawIntegration*) override { return false; }
	bool surface(Surface&) override;

protected:
	void initVisual() override {};
	void initFbcVisual(GLXFBConfig& config);

protected:
	std::unique_ptr<GlxContext> glxContext_;
};

}
