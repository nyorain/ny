#pragma once

#include <ny/backend/x11/include.hpp>
#include <ny/backend/x11/windowContext.hpp>
#include <ny/backend/common/gl.hpp>
#include <nytl/vec.hpp>

namespace evg { class GlDrawContext; }

//prototypes to include glx.h
typedef struct __GLXcontextRec* GLXContext;
typedef struct __GLXFBConfigRec* GLXFBConfig;

namespace ny
{

///GLX GL Context implementation.
class GlxContext: public GlContext
{
public:
    GlxContext(X11WindowContext& wc, GLXFBConfig fbc);
    ~GlxContext();

    void size(const Vec2ui& size);
    bool apply() override;
	void* procAddr(const char* name) const override;

protected:
    X11WindowContext* wc_;
    GLXContext glxContext_ = nullptr;
	std::uint32_t glxWindow_;

    virtual bool makeCurrentImpl() override;
    virtual bool makeNotCurrentImpl() override;

protected:
	static void* glLibHandle();
};

///WindowContext implementation on an x11 backend with opengl (glx) used for rendering.
class GlxWindowContext : public X11WindowContext
{
public:
	GlxWindowContext(X11AppContext& ctx, const X11WindowSettings& settings);

	bool drawIntegration(X11DrawIntegration*) override { return false; }
	bool surface(Surface& surface) override;

protected:
	///Overrides the X11WindowContext initVisual function to query a glx framebuffer config
	///and setting a matching visualid.
	void initVisual() override;

protected:
	std::unique_ptr<GlxContext> glxContext_;
	std::unique_ptr<evg::GlDrawContext> drawContext_;

};

}
