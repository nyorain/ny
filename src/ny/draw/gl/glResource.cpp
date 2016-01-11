#include <ny/draw/gl/glResource.hpp>
#include <ny/draw/gl/glContext.hpp>

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
