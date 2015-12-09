#pragma once

#include <ny/draw/include.hpp>
#include <ny/draw/drawContext.hpp>
#include <ny/draw/shape.hpp>
#include <ny/draw/gl/glContext.hpp>

namespace ny
{

///Abstract base class for all implementations for the DrawContext base class, that
///use some kind of openGL(ES) backend.
class GlDrawContext : public DelayedDrawContext
{
protected:
    rect2f clip_{};

public:
    GlDrawContext() = default;
    virtual ~GlDrawContext() = default;

	//openGL
    virtual void updateViewport(const vec2ui& size) = 0;
	virtual GlContext::Api api() const = 0;
	virtual bool supportsVersion(unsigned int major, unsigned int minor) const = 0;
};

inline GlDrawContext* asGl(DrawContext* dc){ return dynamic_cast<GlDrawContext*>(dc); }
inline GlDrawContext* asGl(DrawContext& dc){ return dynamic_cast<GlDrawContext*>(&dc); }

}
