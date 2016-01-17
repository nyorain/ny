#include <ny/draw/gl/resource.hpp>
#include <ny/draw/gl/context.hpp>

namespace ny
{

GlResource::GlResource(GlContext* ctx) : glContext_(ctx)
{
}

bool GlResource::validContext() const
{
	return (glContext_ && glContext_ == GlContext::currentValid());
}

}
