#pragma once

#include <ny/draw/include.hpp>
#include <nytl/nonCopyable.hpp>

namespace ny
{

///Represents an openGL(ES) resource with a corresponding context.
class GlResource : public nonCopyable
{
protected:
	GlContext* glContext_ = nullptr;

protected:
	bool validContext() const;
	void glContext(GlContext& ctx){ glContext_ = &ctx; }

public:
	GlResource(GlContext* ctx = nullptr);

	GlContext* glContext() const { return glContext_; }
};

}
