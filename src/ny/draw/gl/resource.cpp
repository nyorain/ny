#include <ny/draw/gl/resource.hpp>
#include <ny/draw/gl/context.hpp>

namespace ny
{

GlResource::GlResource(GlContext* ctx) : glContext_(ctx)
{
}

bool GlResource::validContext() const
{
	if(!glContext_ || !GlContext::currentValid()) return 0;
	else if(glContext_ == GlContext::currentValid()) return 1;
	else if(shareable() && GlContext::currentValid()->sharedWith(*glContext_)) return 1;
	else return 0;
}

}
