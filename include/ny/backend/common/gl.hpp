#pragma once

#include <ny/include.hpp>

#include <nytl/nonCopyable.hpp>
#include <nytl/rect.hpp>

#include <string>
#include <vector>
#include <memory>

namespace ny
{

///The possible api types a context may have.
enum class GlApi : unsigned int
{
	gl,
	gles,
};

///Represents a general gl or glsl version
struct GlVersion
{
	GlApi api;
	unsigned int major = 0;
	unsigned int minor = 0;
};

///Returns a parsed string of a gl version.
std::string name(const GlVersion& version);

///Returns a unique number for a gl version.
inline constexpr unsigned int number(const GlVersion& v) { return v.major * 10 + v.minor; }

///Abstract base class for an openGL(ES) context. This class is implemented e.g.
///with egl, glx or wgl. With the static current() function one can get the current context
///for the calling thread. Classes that deal with openGL(ES) like the gl DrawContext
///implementation or the shader class need a current context for their openGL(ES) operations.
///This class is useful as abstraction for the different backends.
class GlContext : public nytl::NonMovable
{
public:
	using Api = GlApi;
	using Version = GlVersion;

	///Returns the current context in the calling thread, nullptr if there is none.
	static GlContext* current() { return threadLocalCurrent(); }

public:
	GlContext() = default;
	virtual ~GlContext();

	///Returns the api this openGL context has, see the Api enum for more information.
	Api api() const { return version().api; }

	///Return whether the api opengl
	bool gl() const { return api() == Api::gl; }

	///Returns whether the api is opengles
	bool gles() const { return api() == Api::gles; }

	///Returns the version of this context.
	Version version() const { return version_; }

	///Returns the major version of the api this context has. If it runs e.g. on openGL 4.5 this
	///function will return 4.
	unsigned int majorApiVersion() const { return version().major; }

	///Returns the minor version of the api this context has. If it runs e.g. on openGL 4.5 this
	///function will return 5.
	unsigned int minorApiVersion() const { return version().minor; }

	///Reeturns majorVersion * 10 + minorVersion. If the context has e.g. the openGLES version
	///3.1 this function returns 31.
	unsigned int versionNumber() const { return number(version()); }

	///Returns the number of depth bits this context has. For contexts without depth buffer it
	///returns therefore 0.
	unsigned int depthBits() const { return depthBits_; }

	///Returns the number of stencil bits this context has. For contexts without stencil
	///buffer it returns therefore 0.
	unsigned int stencilBits() const { return stencilBits_; }


	///Makes the context the current one in the calling thread. If there is another current one
	///before, it will be made not current. Returns 0 on failure.
	bool makeCurrent();

	///Makes this context not current in the current thread. If it is not current, no changes
	///will be made and the function returns 0. If this context is the current one, it will be
	///made not current so that after this function call the calling thread has no
	///current context. Returns 0 on failure or if the context is not current.
	bool makeNotCurrent();

	///Checks if the context is the current one in the calling thread.
	bool isCurrent() const;


	///Returns all extensions returned by glGetString(GL_EXTENSIONS) for this context.
	const std::vector<std::string>& glExtensions() const { return extensions_; }

	///Returns wheter the given openGL extension name is supported.
	bool glExtensionSupported(const std::string& name) const;

	///Returns all supported glsl versions
	std::vector<Version> glslVersions() const { return glslVersions_; }

	///Returns the preferred glsl version
	Version preferredGlslVersion() const { return preferredGlslVersion_; }

	///Returns a vector of all shared opengl contexts.
	std::vector<GlContext*> sharedContexts() const { return sharedContexts_; }

	///Returns whether the context shares it resources with the other context.
	bool sharedWith(const GlContext& other) const;

	///Applies the contents of this context to its surface. Usually this means, the context will
	///swap the back and the front buffer. No guarantess are given that the contents of the
	///context will not be visible BEFORE a call to this function (since some backends may
	///use a single buffer to draw its contents). AFTER a call to this function
	///all contents should be applied.
	///Alternative name(may not be matching for all backends though) would be swapBuffers().
	virtual bool apply();

	///Returns a proc addr for a given function name or nullptr if it could not be found.
	virtual void* procAddr(const char*) const { return nullptr; }

	///Returns a proc addr for a given function name of nullptr if it could not be found.
	virtual void* procAddr(const std::string& name) const { return procAddr(name.c_str()); }

protected:
	static GlContext* threadLocalCurrent(bool change = 0, GlContext* = nullptr);
	static void assureGlLoaded(const GlContext& ctx);
	static void assureGlesLoaded(const GlContext& ctx);

protected:
	Version version_;

	unsigned int depthBits_ {0};
    unsigned int stencilBits_ {0};

	std::vector<std::string> extensions_;
	std::vector<GlContext*> sharedContexts_;
	std::vector<Version> glslVersions_;
	Version preferredGlslVersion_;

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
};

///A guard to make a context current on construction and make it not current on destruction.
///Note that GlContext does allow to manually make current/make not current since it may
///not be desirable in every case to make the context not current on scope exit.
class GlCurrentGuard
{
public:
	GlCurrentGuard(GlContext& ctx) : context(ctx) { context.makeCurrent(); }
	~GlCurrentGuard() { try { context.makeNotCurrent(); } catch(...){ } }

	GlContext& context;
};

}

#ifndef NY_WithGL
	#error ny was built without gl. Do not include this header.
#endif
