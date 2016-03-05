#pragma once

#include <ny/backend/x11/include.hpp>
#include <ny/backend/x11/windowContext.hpp>
#include <ny/draw/gl/context.hpp>
#include <ny/draw/gl/drawContext.hpp>

#include <nytl/vec.hpp>

typedef struct __GLXcontextRec* GLXContext;
typedef struct __GLXFBConfigRec* GLXFBConfig;

namespace ny
{

///GLX GL Context implementation.
class GlxContext: public GlContext
{
protected:
    X11WindowContext* wc_;
    GLXContext glxContext_ = nullptr;
	unsigned int glxWindow_;

    virtual bool makeCurrentImpl() override;
    virtual bool makeNotCurrentImpl() override;

public:
    GlxContext(X11WindowContext& wc, GLXFBConfig fbc);
    ~GlxContext();

    void size(const Vec2ui& size);
    virtual bool apply() override;
};

///WindowContext implementation on a x11 backend with opengl (glx) used for rendering.
class GlxWindowContext : public X11WindowContext
{
protected:
	std::unique_ptr<GlxContext> glxContext_;
	std::unique_ptr<GlDrawContext> drawContext_;

protected:
	virtual void initVisual() override;

public:
	GlxWindowContext(X11AppContext& ctx, const X11WindowSettings& settings = {});
	
	virtual DrawGuard draw() override;
};

}
