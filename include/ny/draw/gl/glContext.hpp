#pragma once

#include <ny/draw/include.hpp>
#include <nytl/nonCopyable.hpp>

#include <string>
#include <vector>

namespace ny
{

///Abstract base class for an openGL(ES) context. This class is implemented e.g.
///with egl, glx or wgl. With the static current() function one can get the current context
///for the calling thread. Classes that deal with openGL(ES) like the gl DrawContext
///implementation or the shader class need a current context for their openGL(ES) operations.
///This class is useful as abstraction for the different backends.
class GlContext : public nonCopyable
{
protected:
	static GlContext* threadLocalCurrent(bool change = 0, GlContext* = nullptr);

public:
	///The possible apis a context may have. 
	///TODO: What about openVG, support for that?
	enum class Api
	{
		openGL,
		openGLES,
	};

	///Returns the current context in the calling thread, nullptr if there is none.
	static GlContext* current()
		{ return threadLocalCurrent(); }

	///Returns the current context in the calling thread, nullptr if there is none or the found
	///one is not in a valid state. This function should be prefered over current.
	///TODO: add some automatical warning/makeNotCurrent system for invalid contexts?
	static GlContext* currentValid()
		{ return current() && current()->valid()? current() : nullptr; }

protected:
	Api api_ {Api::openGL};

	unsigned int depthBits_ {0};
    unsigned int stencilBits_ {0};

    unsigned int majorVersion_ {0};
    unsigned int minorVersion_ {0};

	std::vector<std::string> extensions_;

	///This should be called by implementations at some point of context creation/initialisation
	///to init the context (e.g. function pointer resolution; glew/glbinding). 
	virtual void initContext();

	///This function will be called by makeCurrent() and should be implemented by derived 
	///classes.
	virtual bool makeCurrentImpl() = 0;

	///This function will be called by makeNotCurrent() and should be implemented by derived 
	///classes.
	virtual bool makeNotCurrentImpl() = 0;

public:
	GlContext() = default;
	virtual ~GlContext() = default;

	///Returns the api this openGL context has, see the Api enum for more information.
	Api api() const { return api_; }

	///Returns the major version of the api this context has. If it runs e.g. on openGL 4.5 this
	///function will return 4.
	unsigned int majorApiVersion() const { return majorVersion_; }

	///Returns the minor version of the api this context has. If it runs e.g. on openGL 4.5 this
	///function will return 5.
	unsigned int minorApiVersion() const { return minorVersion_; }

	///Reeturns majorVersion * 10 + minorVersion. If the context has e.g. the openGLES version
	///3.1 this function returns 31.
	unsigned int version() const { return majorVersion_ * 10 + minorVersion_; }

	///Returns the version in the opengl opengles compatiblity layer {20, 30, 31, 32}.
	unsigned int glpVersion() const;


	///Returns the number of depth bits this context has. For contexts without depth buffer it
	///returns therefore 0.
	unsigned int depthBits() const { return depthBits_; }

	///Returns the number of stencil bits this context has. For contexts without stencil buffer it
	///returns therefore 0.
	unsigned int stencilBits() const { return stencilBits_; }


	///Makes the context the current one in the calling thread. If there is another current one
	///before, it will be made not current. Returns 0 on failure.
	bool makeCurrent();

	///Makes this context not current in the current thread. If it is not current, no changes
	///will be made and the function returns 0. If this context is the current one, it will be
	///made not current so that after this function call the calling thread has no current context.
	///Returns 0 on failure or if the context is not current.
	bool makeNotCurrent();

	///Checks if the context is the current one in the calling thread.
	bool isCurrent() const;


	///Returns all extensions returned by glGetString(GL_EXTENSIONS) for this context.
	const std::vector<std::string>& glExtensions() const { return extensions_; }

	///Returns wheter the given openGL extension name is supported.
	bool glExtensionSupported(const std::string& name) const;


	///Applies the contents of this context to its surface. Usually this means, the context will
	///swap the back and the front buffer. No guarantess are given that the contents of the
	///context will not be visible BEFORE a call to this function (since some backends may
	///use a single buffer to draw its contents). AFTER a call to this function
	///all contents should be applied.
	///Alternative name(not matching for all backends, though): swapBuffers()
	virtual bool apply() = 0;

	///Checks if this context is in a valid state. Usually all contexts that exist should
	///be in a valid state (RAII) but there may be cases where the used backend is not able
	///to guarantee it, so it should usually be checked before using the context. 
	virtual bool valid() const { return 1; }
};


}
