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

class GlxContext: public GlContext
{
protected:
    X11WindowContext* wc_;
    GLXContext glxContext_ = nullptr;
	GlDrawContext drawContext_;
	unsigned int glxWindow_;

    virtual bool makeCurrentImpl() override;
    virtual bool makeNotCurrentImpl() override;

public:
    GlxContext(X11WindowContext& wc, GLXFBConfig fbc);
    ~GlxContext();

    void size(const Vec2ui& size);
    virtual bool apply() override;

	GlDrawContext& drawContext() { return drawContext_; }
};

class GlxWindowContext : public X11WindowContext
{
};

}
