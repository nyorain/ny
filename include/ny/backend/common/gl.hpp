#pragma once

#include <ny/include.hpp>

#include <nytl/nonCopyable.hpp>
#include <nytl/stringParam.hpp>
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

///Represents a general gl or glsl version.
///Note that e.g. glsl version 330 is represented by major=3, minor=3
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

///Abstract base class for an openGL(ES) context wrapper. This class is implemented e.g.
///with egl, glx or wgl. With the static current() function one can get the current context
///for the calling thread. Does usually not own the associated native context handle,
///but rather connect it with some kind of surface/drawable that is should be made
///current for.
///Implementations should remember to make the context not current in the destructor.
class GlContext : public nytl::NonMovable
{
public:
	using Api = GlApi;
	using Version = GlVersion;

	///The cross-platform error codes used for std::error_code objects
	enum class Error : unsigned int
	{
		alreadyCurrent = 1,
		alreadyNotCurrent = 2,
		currentInAnotherThread = 3
	};

	///Returns the current context in the calling thread, nullptr if there is none.
	static GlContext* current();

public:
	GlContext() = default;
	virtual ~GlContext();

	Api api() const { return version().api; } ///The contexts api (gl or gles)
	Version version() const { return version_; } ///The gl(es) api version

	unsigned int depthBits() const { return depthBits_; } ///The default number of depth bits
	unsigned int stencilBits() const { return stencilBits_; } ///The default number of stencil bits

	bool isCurrent() const; ///Checks if context is current in calling thread
	bool currentAnywhere() const; ///Checks if context is current in any thread

	///Returns all extensions returned by glGetString(GL_EXTENSIONS) for this context.
	const std::vector<std::string>& glExtensions() const { return extensions_; }

	///Returns wheter the given openGL extension name is supported.
	bool glExtensionSupported(const std::string& name) const;

	///Returns all supported glsl versions
	std::vector<Version> glslVersions() const { return glslVersions_; }

	///Returns the latest (or otherwise preferred) glsl version
	Version preferredGlslVersion() const { return preferredGlslVersion_; }

	///Returns whether this object has the same underlaying native context as the
	///given GlContext. Note that if this function returns true, those two contexts
	///must not be made current in different threads at the same time.
	///Trying to do so will reult in an exception.
	virtual bool sameContext(const GlContext& other) const;

	///If both GlContext objects refer to the same native context handle, returns true.
	///If someContext(other) is true, this function returns true as well.
	virtual bool sharedWith(const GlContext& other) const = 0;
	
	///Returns the natvie opengl context handle casted to a void pointer.
	virtual void* nativeHandle() const = 0;

	///Returns a proc addr for a given function name or nullptr if it could not be found.
	///Can be used to query core gl/extension gl or extension platform api functions.
	///Note that just because this call returns a valid function, the extension still has
	///to be supported.
	virtual void* procAddr(nytl::StringParam name) const = 0;

	///Makes the context the current one in the calling thread. If there is another current one
	///before, it will be made not current.
	///If the context is already current in this thread, this function has no effect.
	///\exception std::system_error On implementation failure
	///\excpetion std::system_error If this context or another context associated with the same
	///native context handle is already current in other thread this function .
	void makeCurrent();

	///Same as makeCurrent but does not throw. If the context is current after this call returns
	///true. If the context was already current the error code is set, but true is returned.
	bool makeCurrent(std::error_code& error);

	///Makes this context not current in the current thread. If this context is the current one,
	///it will be made not current so that after this function call the calling thread has no
	///current context.
	///If the context is not current, this function has no effect.
	///Note that there is no way to make the context not current from thread A if it is
	///current in thread B.
	///\exception std::system_failure On failure
	void makeNotCurrent();

	///Same as makeNotCurrent but does not throw. If the context is not current after this call
	///returns true. If the context was already not current the error code is set, but true
	///is returned.
	bool makeNotCurrent(std::error_code& error);

	//NOTE: does the context has to be current when calling this? on some implementations
	//it might be like this...

	///Applies the contents of this context to its surface. Usually this means, the context will
	///swap the back and the front buffer. No guarantess are given that the contents of the
	///context will not be visible before a call to this function (since some backends may
	///use a single buffer to draw its contents). After a call to this function
	///all contents are or will be applied soon.
	///Alternative name would be swapBuffers().
	///\excpetion std::system_error On failure
	virtual void apply();
	virtual bool apply(std::error_code& error) = 0; ///Does not throw

protected:
	///This should be called by implementations at some point of context creation/initialisation
	///to init the context (e.g. function pointer resolution; glew/glbinding).
	///\exception std::system_error if the context could not be made current
	///\exception std::runtime_error if the needed gl functions could not be loaded
	///\exception std::runtime_error if the gl/glsl versions cannot be retrieved
	virtual void initContext(Api api, unsigned int depth = 0, unsigned int stencil = 0);
	virtual bool makeCurrentImpl(std::error_code& error) = 0;
	virtual bool makeNotCurrentImpl(std::error_code& error) = 0;

protected:
	Version version_;

	unsigned int depthBits_ {0};
	unsigned int stencilBits_ {0};

	std::vector<std::string> extensions_;
	std::vector<Version> glslVersions_;
	Version preferredGlslVersion_;
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
