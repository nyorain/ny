#pragma once

#include <ny/include.hpp>
#include <ny/base/nativeHandle.hpp>

#include <nytl/nonCopyable.hpp>
#include <nytl/stringParam.hpp>
#include <nytl/rect.hpp>

#include <string>
#include <vector>
#include <memory>
#include <thread>

namespace ny
{

enum class GlApi : unsigned int { gl, gles, }; ///Possible opengl apis a context can have
struct GlConfigId { unsigned int value {UINT_MAX}; }; ///Backend-specifc unique id of a GlConfig

///Represents a general gl or glsl version.
///Note that e.g. glsl version 330 is represented by major=3, minor=3
struct GlVersion
{
	GlApi api;
	unsigned int major = 0;
	unsigned int minor = 0;
};

///Returns a name holding the information of the given GlVersion.
std::string name(const GlVersion& version);

///Returns a unique number for a gl version.
constexpr unsigned int number(const GlVersion& v) { return v.major * 10 + v.minor; }

///Describes a possible opengl context and surface configuration.
struct GlConfig
{
	unsigned int depth;
	unsigned int stencil;
	unsigned int multisample;

	bool alpha;
	bool doublebuffer;

	GlConfigId id;
};

///Describe the settings for a created gl context.
struct GlContextSettings
{
	GlConfigId config;
	GlVersion api;
	bool compatibility {};
	bool forwardCompatible {};
	bool debug {};

	GlContext* share {};
};

///Defines extensions that are (in some form) present in the gl implementations
///and integrations of multiple platforms.
enum class GlContextExtension : unsigned int
{
	swapControl, //implements the swapControl function
	swapControlTear //swapControl can be called with -1
};

using GlContextExtensions = nytl::Flags<GlContextExtension>;

///Holds error code values for platform-independet logic errors when dealing with gl
///contexts.
enum class GlContextErrorCode : unsigned int
{
	invalidConfig, //the config given on context construction is invalid
	invalidSharedContext, //the opengl context to share with is invalid

	contextAlreadyCurrent, //not critical, context was current before makeCurrent
	contextAlreadyNotCurrent, //not critical, context was not current before makeNotCurernt
	contextCurrentInAnotherThread, //context is already current in another thread

	surfaceAlreadyCurrent, //surface current on another context in other thread
	invalidSurface, //the given surface is invalid/unknown implementation
	incompatibleSurface //the given surface has an incompatible configuration
};

///Exception class thrown on general logic errors that can be expressed using a
///GlContext::ErrorCode.
///Is usually thrown for arguments or situations that result in some critical logic error.
class GlContextError : public std::logic_error
{
public:
	GlContextError(GlContextErrorCode error, const char* msg);
	std::error_code code;
};

///Abstract base class for some kind of openGL(ES) surface that can be drawn on.
///Surfaces are usually native windows or pixel buffers.
class GlSurface : public nytl::NonMovable
{
public:
	virtual ~GlSurface() = default;

	virtual NativeHandle nativeHandle() const = 0; ///The native surface handle
	virtual GlConfig config() const = 0; ///The config this surface was created with

	///Returns whether the surface is current in the calling thread.
	///
	virtual bool current(const GlContext** currentContext = nullptr) const = 0;
	virtual bool currentInAnyThread(const GlContext** currentContext = nullptr) const = 0;

	///Applies the pending contents of the surface, swaps buffers for multibuffered surfaces.
	///\excpetion std::system_error If calling the native function fails.
	virtual void apply();
	virtual bool apply(std::error_code& ec) = 0;
};

///Abstract base class for native gl context implementations. This class is implemented e.g.
///with egl, glx or wgl. With the static current() function one can get the current context
///for the calling thread.
///Implementations should remember to make the context not current in the destructor.
class GlContext : public nytl::NonMovable
{
public:
	using Api = GlApi;
	using Version = GlVersion;
	using Config = GlConfig;
	using Extension = GlContextExtension;
	using Extensions = GlContextExtensions;
	using ErrorCode = GlContextErrorCode;
	using Error = GlContextError;

	///Returns the GlContext that is current in the calling thread or nullptr if there is none.
	///If a non-null pointer to a GlSurface* parameter is passed, sets it to the current surface.
	static GlContext* current(const GlSurface** currentSurface = nullptr);

public:
	GlContext() = default;
	virtual ~GlContext() = default;

	///Makes the context current for the given GlSurface in the calling thread.
	///Note that there is no way to make a context current in another thread.
	///\exception std::system_error If calling the native function fails.
	///\excpetion GlContext::Error If making the context current is logically not possible
	///This can happen if it is already current in another thread or the surface is invalid.
	///If the context is already current, no exception is thrown.
	virtual void makeCurrent(const GlSurface& surface);

	///Makes the context not current in the calling thread.
	///Note that there is no way to make a context not current in another thread.
	///\excpetion std::system_error If calling the native function fails.
	///If the context was not current before this call, no excpetion is thrown.
	virtual void makeNotCurrent();

	//Those functions behave like their equivalents without error_code parameter, but they
	//do not throw on error, but simply set the error code and return 0 on failure.
	//Note that the error code might be set to a non-critical error when true is returned.
	virtual bool makeCurrent(const GlSurface& surface, std::error_code& ec);
	virtual bool makeNotCurrent(std::error_code& ec);

	///Returns whether the context is current in the calling thread.
	///If so and a non-null GlSurface** parameter was given, sets it to the associated surface.
	virtual bool isCurrent(const GlSurface* currentSurface = nullptr) const;

	///Returns whether the context is current in any thread.
	///If so and a non-null GlSurface** parameter was given, sets it to the associated surface.
	virtual bool currentInAnyThread(const GlSurface* currentSurface = nullptr) const;

	///Returns whether this context is shared with the given other GlContext.
	virtual bool sharedWith(const GlContext& other) const;

	virtual GlConfig config() const { return config_; } ///The associated config
	virtual GlVersion version() const { return version_; } ///The contexts api version
	virtual const GlContext* shared() const { return shared_; } ///The reference shared context
	virtual const std::vector<GlVersion>& glslVersions() const { return glslVersions_; }
	virtual NativeHandle nativeHandle() const = 0; ///Native context handle

	///Returns whether this context could theoretically be made current for the given surface.
	///Note that if context and surface are not compatible this may be due to different configs
	///or different implementations.
	///Note that this does not check if context or surface are already current in some way
	///and can therefore not be made current at the moment.
	virtual bool compatible(const GlSurface&) const = 0;

	///Can be used to determine whether special extensions that are available on multiple
	///backends can be used. The extensions-specifc functions should only be called
	///if the returned Extension flags contain the required extension.
	virtual GlContextExtensions contextExtensions() const = 0;

	// -- Extension specific, might not be available --
	virtual void swapInterval(int interval) const;
	virtual bool swapInterval(int interval, std::error_code& ec) const;

protected:
	virtual void initContext(Api api, const Config& config, const GlContext* shared);
	virtual bool makeCurrentImpl(const GlSurface& surface, std::error_code& ec) = 0;
	virtual bool makeNotCurrentImpl(std::error_code& ec) = 0;

protected:
	GlConfig config_;
	GlVersion version_;
	std::vector<Version> glslVersions_;
	const GlContext* shared_ {};
};

///Abstract base class for an openGL(ES) context wrapper. This class is implemented e.g.
///with egl, glx or wgl. With the static current() function one can get the current context
///for the calling thread. Does usually not own the associated native context handle,
///but rather connect it with some kind of surface/drawable that is should be made
///current for.
///Implementations should remember to make the context not current in the destructor.
// class GlContext : public nytl::NonMovable
// {
// public:
// 	using Api = GlApi;
// 	using Version = GlVersion;
//
// 	///The cross-platform error codes used for std::error_code objects
// 	enum class Error : unsigned int
// 	{
// 		alreadyCurrent = 1,
// 		alreadyNotCurrent = 2,
// 		currentInAnotherThread = 3
// 	};
//
// 	///Returns the current context in the calling thread, nullptr if there is none.
// 	static GlContext* current();
//
// public:
// 	GlContext() = default;
// 	virtual ~GlContext();
//
// 	Api api() const { return version().api; } ///The contexts api (gl or gles)
// 	Version version() const { return version_; } ///The gl(es) api version
//
// 	unsigned int depthBits() const { return depthBits_; } ///The default number of depth bits
// 	unsigned int stencilBits() const { return stencilBits_; } ///The default number of stencil bits
//
// 	bool isCurrent() const; ///Checks if context is current in calling thread
// 	bool currentAnywhere() const; ///Checks if context is current in any thread
//
// 	///Returns all extensions returned by glGetString(GL_EXTENSIONS) for this context.
// 	const std::vector<std::string>& glExtensions() const { return extensions_; }
//
// 	///Returns wheter the given openGL extension name is supported.
// 	bool glExtensionSupported(const std::string& name) const;
//
// 	///Returns all supported glsl versions
// 	std::vector<Version> glslVersions() const { return glslVersions_; }
//
// 	///Returns the latest (or otherwise preferred) glsl version
// 	Version preferredGlslVersion() const { return preferredGlslVersion_; }
//
// 	///Returns whether this object has the same underlaying native context as the
// 	///given GlContext. Note that if this function returns true, those two contexts
// 	///must not be made current in different threads at the same time.
// 	///Trying to do so will reult in an exception.
// 	virtual bool sameContext(const GlContext& other) const;
//
// 	///If both GlContext objects refer to the same native context handle, returns true.
// 	///If someContext(other) is true, this function returns true as well.
// 	virtual bool sharedWith(const GlContext&) const { return false; }
//
// 	///Returns the natvie opengl context handle casted to a void pointer.
// 	virtual void* nativeHandle() const = 0;
//
// 	///Returns a proc addr for a given function name or nullptr if it could not be found.
// 	///Can be used to query core gl/extension gl or extension platform api functions.
// 	///Note that just because this call returns a valid function, the extension still has
// 	///to be supported.
// 	virtual void* procAddr(nytl::StringParam name) const = 0;
//
// 	///Makes the context the current one in the calling thread. If there is another current one
// 	///before, it will be made not current.
// 	///If the context is already current in this thread, this function has no effect.
// 	///\exception std::system_error On implementation failure
// 	///\excpetion std::system_error If this context or another context associated with the same
// 	///native context handle is already current in other thread this function .
// 	void makeCurrent();
//
// 	///Same as makeCurrent but does not throw. If the context is current after this call returns
// 	///true. If the context was already current the error code is set, but true is returned.
// 	bool makeCurrent(std::error_code& error);
//
// 	///Makes this context not current in the current thread. If this context is the current one,
// 	///it will be made not current so that after this function call the calling thread has no
// 	///current context.
// 	///If the context is not current, this function has no effect.
// 	///Note that there is no way to make the context not current from thread A if it is
// 	///current in thread B.
// 	///\exception std::system_failure On failure
// 	void makeNotCurrent();
//
// 	///Same as makeNotCurrent but does not throw. If the context is not current after this call
// 	///returns true. If the context was already not current the error code is set, but true
// 	///is returned.
// 	bool makeNotCurrent(std::error_code& error);
//
// 	//NOTE: does the context has to be current when calling this? on some implementations
// 	//it might be like this...
//
// 	///Applies the contents of this context to its surface. Usually this means, the context will
// 	///swap the back and the front buffer. No guarantess are given that the contents of the
// 	///context will not be visible before a call to this function (since some backends may
// 	///use a single buffer to draw its contents). After a call to this function
// 	///all contents are or will be applied soon.
// 	///Alternative name would be swapBuffers().
// 	///\excpetion std::system_error On failure
// 	virtual void apply();
// 	virtual bool apply(std::error_code& error) = 0; ///Does not throw
//
// protected:
// 	///This should be called by implementations at some point of context creation/initialisation
// 	///to init the context (e.g. function pointer resolution; glew/glbinding).
// 	///\exception std::system_error if the context could not be made current
// 	///\exception std::runtime_error if the needed gl functions could not be loaded
// 	///\exception std::runtime_error if the gl/glsl versions cannot be retrieved
// 	virtual void initContext(Api api, unsigned int depth = 0, unsigned int stencil = 0);
// 	virtual bool makeCurrentImpl(std::error_code& error) = 0;
// 	virtual bool makeNotCurrentImpl(std::error_code& error) = 0;
//
// protected:
// 	Version version_;
//
// 	unsigned int depthBits_ {0};
// 	unsigned int stencilBits_ {0};
//
// 	std::vector<std::string> extensions_;
// 	std::vector<Version> glslVersions_;
// 	Version preferredGlslVersion_;
// };

///A guard to make a context current on construction and make it not current on destruction.
///Note that GlContext does allow to manually make current/make not current since it may
///not be desirable in every case to make the context not current on scope exit.
class GlCurrentGuard
{
public:
	GlCurrentGuard(GlContext& ctx, const GlSurface& surf) : ctx(ctx) { ctx.makeCurrent(surf); }
	~GlCurrentGuard() { try { ctx.makeNotCurrent(); } catch(...){ } } //TODO!

	GlContext& ctx;
};

}

#ifndef NY_WithGL
	#error ny was built without gl. Do not include this header.
#endif
