#pragma once

#include <ny/config.hpp>
#ifdef NY_WithGL

#include <ny/backend/x11/include.hpp>
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

    virtual bool makeCurrentImpl() override;
    virtual bool makeNotCurrentImpl() override;

public:
    GlxContext(X11WindowContext& wc, GLXFBConfig fbc);
    ~GlxContext();

    void size(const vec2ui& size);
    virtual bool apply() override;

	GlDrawContext& drawContext() { return drawContext_; }
};

}

#endif
