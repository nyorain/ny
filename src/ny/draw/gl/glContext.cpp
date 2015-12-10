#include <ny/draw/gl/glContext.hpp>
#include <nytl/log.hpp>

//glcontext backend api
#if defined(NY_WithGlbinding)
 #include <glbinding/Binding.h>

 #if defined(NY_WithGlesbinding)
  #include <glesbinding/Binding.h>
 #endif //glesLoader

#elif defined(NY_WithGLEW)
 #include <GL/glew.h>
#endif //glLoader

namespace ny
{

GlContext* GlContext::threadLocalCurrent(bool change, GlContext* newOne)
{
	static thread_local GlContext* current_;
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

	if(api_ == Api::openGL)
	{
		#if defined(NY_WithGlbinding)
		 glBinding::Binding::initialize();

		#elif defined(NY_WithGLEW)
		 glewExperimental = 1;
		 glewInit();

		#else
		 //static loading
		 nytl::sendWarning("GlContext::initContext: no dynamic openGL loader available.");

		#endif //glLoader
	}
	else if(api_ == Api::openGLES)
	{
		#if defined(NY_WithGlesbinding)
		 glesBinding::Binding::initialize();

		#else
		 //static loading
		 nytl::sendWarning("GlContext::initContext: no dynamic openGLES loader available.");

		#endif //glesBinding
	}

	if(!saved->makeCurrent())
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

}
