#pragma once

#include <ny/include.hpp>
#include <ny/draw/gl/resource.hpp>
#include <nytl/nonCopyable.hpp>
#include <nytl/rect.hpp>

#include <string>
#include <vector>

namespace ny
{

///Abstract base class for an openGL(ES) context. This class is implemented e.g.
///with egl, glx or wgl. With the static current() function one can get the current context
///for the calling thread. Classes that deal with openGL(ES) like the gl DrawContext
///implementation or the shader class need a current context for their openGL(ES) operations.
///This class is useful as abstraction for the different backends.
class GlContext : public NonCopyable
{
public:
	///The possible apis a context may have.
	enum class Api
	{
		gl,
		gles,
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
	static GlContext* threadLocalCurrent(bool change = 0, GlContext* = nullptr);
	static void assureGlLoaded(const GlContext& ctx);
	static void assureGlesLoaded(const GlContext& ctx);

protected:
	Api api_ {Api::gl};

	unsigned int depthBits_ {0};
    unsigned int stencilBits_ {0};

    unsigned int majorVersion_ {0};
    unsigned int minorVersion_ {0};

	std::vector<std::string> extensions_;
	std::vector<GlContext*> sharedContexts_;
	std::vector<unsigned int> glslVersions_;
	unsigned int preferredGlslVersion_;

	std::vector<std::unique_ptr<GlResource>> resources_; //TODO

protected:
	///This should be called by implementations at some point of context creation/initialisation
	///to init the context (e.g. function pointer resolution; glew/glbinding).
	virtual void initContext(Api api, unsigned int depth = 0, unsigned int stencil = 0);

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

	///Return whether the api opengl
	bool gl() const { return api() == Api::gl; }

	///Returns whether the api is opengles
	bool gles() const { return api() == Api::gles; }

	///Returns the major version of the api this context has. If it runs e.g. on openGL 4.5 this
	///function will return 4.
	unsigned int majorApiVersion() const { return majorVersion_; }

	///Returns the minor version of the api this context has. If it runs e.g. on openGL 4.5 this
	///function will return 5.
	unsigned int minorApiVersion() const { return minorVersion_; }

	///Reeturns majorVersion * 10 + minorVersion. If the context has e.g. the openGLES version
	///3.1 this function returns 31.
	unsigned int version() const { return majorVersion_ * 10 + minorVersion_; }

	/*
	///Returns an unsigned int that roughly specified the functionality of the used api,
	///independent from gl or gles.
	///gles {10, 20: 20, 30: 30, 31: 31, 32: 32}
	///gl {10, >=30: 20, 33: 30, 44: 31, 45: 32}
	unsigned int glCompVersion() const;
	*/


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

	///Returns all supported glsl versions
	std::vector<unsigned int> glslVersions() const { return glslVersions_; }

	///Returns the preferred glsl version
	unsigned int preferredGlslVersion() const { return preferredGlslVersion_; }

	///Returns a vector of all shared opengl contexts.
	std::vector<GlContext*> sharedContexts() const { return sharedContexts_; }

	///Returns whether the context shares it resources with the other context.
	bool sharedWith(const GlContext& other) const;

	//XXX: should be deprecated?
	///Updates the openGL viewport
	void updateViewport(const Rect2f& viewport);

	///Applies the contents of this context to its surface. Usually this means, the context will
	///swap the back and the front buffer. No guarantess are given that the contents of the
	///context will not be visible BEFORE a call to this function (since some backends may
	///use a single buffer to draw its contents). AFTER a call to this function
	///all contents should be applied.
	///Alternative name(may not be matching for all backends though) would be swapBuffers().
	virtual bool apply();

	///Checks if this context is in a valid state. Usually all contexts that exist should
	///be in a valid state (RAII) but there may be cases where the used backend is not able
	///to guarantee it, so it should usually be checked before using the context.
	virtual bool valid() const { return 1; }

	///Returns a proc addr for a given function name of nullptr if it could not be found.
	virtual void* procAddr(const char* name) const { return nullptr; }

	///Returns a proc addr for a given function name of nullptr if it could not be found.
	virtual void* procAddr(const std::string& name) const { return procAddr(name.c_str()); }
};


}
