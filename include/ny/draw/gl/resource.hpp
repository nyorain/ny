#pragma once

#include <ny/include.hpp>
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
	virtual bool shareable() const = 0;
};

///OpenGL(ES) VertexArray
class GlVertexArray : public GlResource
{
protected:
	unsigned int handle_;

public:
	GlVertexArray(unsigned int handle) : handle_(handle) {}
	virtual ~GlVertexArray();

	void bind() const;
	void unbind() const;
	
	unsigned int handle() const { return handle_; }

	virtual bool shareable() const override { return 0; }
};

///OpenGL(ES) Buffer
class GlBuffer : public GlResource
{
protected:
	unsigned int handle_;
	unsigned int type_;

public:
	GlBuffer(unsigned int handle, unsigned int type) : handle_(handle), type_(type) {}
	virtual ~GlBuffer();

	void bind() const;
	void unbind() const;

	unsigned int handle() const { return handle_; }
	unsigned int type() const { return type_; }

	virtual bool shareable() const override { return 1; }
};

}
