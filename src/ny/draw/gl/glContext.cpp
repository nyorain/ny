#include <ny/draw/gl/glContext.hpp>
#include <nytl/log.hpp>

//glcontext backend api
#include <glpbinding/Binding.h>

namespace ny
{

GlContext* GlContext::threadLocalCurrent(bool change, GlContext* newOne)
{
	static thread_local GlContext* current_ = nullptr;
	if(change)
		current_ = newOne;

	return current_;
}

void GlContext::initContext()
{
	auto* saved = current();
	if(!makeCurrent())
	{
		nytl::sendWarning("GlContext::initContext: failed to make current.");
		return;
	}

	glpbinding::Binding::initialize();

	if(saved && !saved->makeCurrent())
	{
		nytl::sendWarning("GlContext::initContext: failed to make saved context current again.");
		return;
	}
}

bool GlContext::makeCurrent()
{
	if(makeCurrentImpl())
	{
		threadLocalCurrent(1, this);
		return 1;
	}

	return 0;
}

bool GlContext::makeNotCurrent()
{
	if(!isCurrent()) return 0;

	//TODO: if branch like in makeCurrent?
	threadLocalCurrent(1, nullptr);
	return makeNotCurrentImpl();
}

bool GlContext::isCurrent() const
{
	return threadLocalCurrent() == this;
}

bool GlContext::glExtensionSupported(const std::string& name) const
{
	for(auto& s : glExtensions())
		if(s == name) return 1;

	return 0;
}

unsigned int GlContext::glpVersion() const
{
	if(api_ == Api::openGL)
	{
		if(version() <= 20) return 0;
		else if(version() < 30) return 20;
		else if(version() < 31) return 30;
		else if(version() < 32) return 31;
		else if(version() >= 32) return 32;
	}
	else if(api_ == Api::openGLES)
	{
		if(version() <= 20) return 0;
		else if(version() < 30) return 20;
		else if(version() < 31) return 30;
		else if(version() < 32) return 31;
		else if(version() >= 32) return 32;
	}

	return 0;
}

}
