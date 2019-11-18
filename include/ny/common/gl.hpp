// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/fwd.hpp>
#include <ny/nativeHandle.hpp>

#include <nytl/nonCopyable.hpp>
#include <nytl/rect.hpp>
#include <nytl/flags.hpp>

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <system_error>

namespace ny {

/// Possible opengl apis a context can have
enum class GlApi : unsigned int { none, gl, gles, };

/// Opaque GlConfig id. Is used by backends as pointer or unsigned int.
using GlConfigID = struct GlConfigIDType*;

/// Describes a possible opengl context and surface configuration.
struct GlConfig {
	unsigned int depth {};
	unsigned int stencil {};
	unsigned int samples {};

	unsigned int red {};
	unsigned int green {};
	unsigned int blue {};
	unsigned int alpha {};

	bool doublebuffer {};
	bool transparent {};

	GlConfigID id {};
};

/// Rates the given GlConfig.
/// The "best" config is chosen to have 24 bits depth, 8 bits stencil, no multisample,
/// rgba8888 format and doulbebuffering.
/// This function is usually used when determining the default GlConfig in the
/// different GlSetup implementations.
/// Returns some value in the range 1 - 100;
unsigned int rate(const GlConfig& config);

/// Returns whether the given extension string that lists extensions sepertaed by
/// whitespace contains the given extensions.
bool glExtensionStringContains(const char* extString, const char* extension);

/// Describes the settings for a created gl context.
/// The shared GlContext must be set to nullptr or a GlContext that was created from the
/// same setup implementation. The returned context will be shared with the given
/// share context and all context the share context is already shared with.
/// If no specific config id is set, the default config is used.
/// Api specifies the api the context should have. Context creation will fail if the
/// api could not be used. Can be set to GlApi::none to just choose any available api.
struct GlContextSettings {
	GlConfigID config {};
	bool compatibility {};
	bool forwardCompatible {};
	bool debug {};
	GlApi api {};

	GlContext* share {};
};

/// Defines extensions that are (in some form) present in the gl implementations
/// and integrations of multiple platforms.
enum class GlContextExtension : unsigned int {
	swapControl, // implements the swapControl function
	swapControlTear // swapControl can be called with -1
};

using GlContextExtensions = nytl::Flags<GlContextExtension>;

/// Holds error code values for platform-independet logic errors when dealing with gl
/// contexts.
/// Note that this enumeration is used for error codes as well as for error conditions
/// since the values are not platform-dependent but also not converted from platform
/// errors but genereted from ny itself.
enum class GlContextErrc : unsigned int {
	invalidConfig = 1, // the config id given on context construction is invalid
	invalidSharedContext, // the opengl context to share with is invalid
	invalidApi, // the api requested on construction cannot be used
	invalidVersion, // an invalid opengl version was detected

	contextAlreadyCurrent, // not critical, context was current before makeCurrent
	contextAlreadyNotCurrent, // not critical, context was not current before makeNotCurernt
	contextCurrentInAnotherThread, // context is already current in another thread
	contextNotCurrent, // function requires context to be current at the moment

	surfaceAlreadyCurrent, // surface current on another context in other thread
	invalidSurface, // the given surface is invalid
	incompatibleSurface, // the given surface has an incompatible configuration

	extensionNotSupported // the extensions for the called function is not supported
};

/// Exception class thrown on general logic errors that can be expressed using a
/// GlContext::ErrorCode.
/// Is usually thrown for arguments or situations that result in some critical logic error.
class GlContextError : public std::logic_error {
public:
	GlContextError(std::error_code, const char* msg = {});
	const std::error_code& code() const { return code_; }

protected:
	std::error_code code_;
};

/// Abstract base class for querying gl configs and creating opengl contexts.
/// An implementation can be retrieved by an AppContext implementation if it
/// is supported.
class GlSetup {
public:
	virtual ~GlSetup() = default;

	/// Returns all available configs
	virtual std::vector<GlConfig> configs() const = 0;

	/// Returns the config for the given id.
	/// The default implementation will just call configs() and then search
	/// for the config with the given id.
	/// Should return an empty config (i.e. with no config id) if the given config
	/// could not be found.
	virtual GlConfig config(GlConfigID id) const;

	/// Returns the default GlConfig.
	/// Is expected to be roughly chosen by useful attributes like depth, stencil, format,
	/// hardware acceleration or doublebuffering.
	virtual GlConfig defaultConfig() const = 0;

	/// Constructs and returns a new opengl implementation object. Implementations
	/// should always return a valid unique_ptr and throw an excpetion on error.
	/// Some exceptions can be triggered by passing invalid settings.
	/// \exception GlContextError For invalid or incompatible share context,
	/// invalid api, version or config.
	virtual std::unique_ptr<GlContext> createContext(const GlContextSettings& = {}) const = 0;

	/// Creates a context with the given settings that can for sure be used to
	/// render on the given surface. Will ignore the given glConfig setting.
	/// Will use the glConfig of the given surface. The GlSurface must be retrieved
	/// from the same backend as this setup.
	/// \exception GlContextError If the context cannot be created ().
	/// Internally calls the createContext implementation.
	/// Passing an invalid surface results in undefined behvaiour, implementations
	/// may override this function and throw in this case.
	virtual std::unique_ptr<GlContext> createContext(const GlSurface& surface,
		GlContextSettings = {}) const;

	/// Returns the proc address for the given opengl extension function name.
	/// If nullptr is returned, the function could not be found. Can be used to query
	/// core and extensions or platform-specific opengl functions.
	/// Note that some specifications (namely, windows wgl) state that the returned
	/// function is only guaranteed to be valid for the current context, so using
	/// the returned function on another context may result in undefined
	/// behaviour.
	/// Note that this functions returning a value != nullptr is not a guarantee that
	/// the returned value is actually a valid and callable function. One must always
	/// query the supported extensions. Some implementations even return values
	/// != nullptr for non-existent functions.
	virtual void* procAddr(const char* name) const = 0;
};

/// Abstract base class for some kind of openGL(ES) surface that can be drawn on.
/// Surfaces are usually native windows or pixel buffers, something that can be
/// drawn on using gl.
/// By design NonMovable since it may be referenced as current surface.
class GlSurface : public nytl::NonMovable {
public:
	virtual ~GlSurface();

	virtual NativeHandle nativeHandle() const = 0; /// The native surface handle
	virtual GlConfig config() const = 0; /// The config this surface was created with

	/// Returns whether the surface is current in the calling thread.
	/// If so and a non-null GlContext** parameter was given, sets it to the current context.
	virtual bool isCurrent(GlContext** currentContext = nullptr) const;
	virtual bool isCurrentInAnyThread(GlContext** currentContext = nullptr,
		std::thread::id* currentThread = nullptr) const;

	/// Applies the pending contents of the surface, swaps buffers for multibuffered surfaces.
	/// \excpetion std::system_error If calling the native function fails.
	virtual void apply() const;
	virtual bool apply(std::error_code&) const = 0;
};

/// Abstract base class for native gl context implementations. This class is implemented e.g.
/// with egl, glx or wgl. With the static current() function one can get the current context
/// for the calling thread.
/// Implementations should remember to make the context not current in the destructor.
/// By design NonMovable since it may be referenced as current context.
class GlContext : public nytl::NonMovable {
public:
	using Extension = GlContextExtension;
	using Extensions = GlContextExtensions;
	using Error = GlContextError;
	using Errc = GlContextErrc;

	/// Returns the GlContext that is current in the calling thread or nullptr if there is none.
	/// If a non-null pointer to a GlSurface* parameter is passed, sets it to the current surface.
	static GlContext* current(const GlSurface** currentSurface = nullptr);

public:
	GlContext() = default;
	virtual ~GlContext();

	/// Makes the context current for the given GlSurface in the calling thread.
	/// Note that there is no way to make a context current in another thread.
	/// \exception std::system_error If calling the native function fails.
	/// \excpetion GlContextError If making the context current is logically not possible
	/// This can happen if it is already current in another thread or the surface is invalid.
	/// If the context is already current, no exception is thrown.
	virtual void makeCurrent(const GlSurface&);

	/// Makes the context not current in the calling thread.
	/// Note that there is no way to make a context not current in another thread.
	/// \excpetion std::system_error If calling the native function fails.
	/// If the context was not current before this call, no excpetion is thrown.
	virtual void makeNotCurrent();

	// Those functions behave like their equivalents without error_code parameter, but they
	// do not throw on error, but simply set the error code and return 0 on failure.
	// Note that the error code might be set to a non-critical error when true is returned.
	virtual bool makeCurrent(const GlSurface&, std::error_code&);
	virtual bool makeNotCurrent(std::error_code&);

	/// Returns whether the context is current in the calling thread.
	/// If so and a non-null GlSurface** parameter was given, sets it to the associated surface.
	virtual bool isCurrent(const GlSurface** currentSurface = nullptr) const;

	/// Returns whether the context is current in any thread.
	/// If so and a non-null GlSurface** parameter was given, sets it to the associated surface.
	virtual bool isCurrentInAnyThread(const GlSurface** currentSurface = nullptr,
		std::thread::id* currentThread = nullptr) const;

	virtual GlApi api() const { return api_; } /// The api of this context
	virtual GlConfig config() const { return config_; } /// The associated config
	virtual NativeHandle nativeHandle() const = 0; /// Native context handle

	/// Returns whether this context could theoretically be made current for the given surface.
	/// Note that if context and surface are not compatible this may be due to different configs
	/// or different implementations.
	/// Note that this does not check if context or surface are already current in some way
	/// and can therefore not be made current at the moment.
	virtual bool compatible(const GlSurface&) const = 0;

	/// Can be used to determine whether special extensions that are available on multiple
	/// backends can be used. The extensions-specifc functions should only be called
	/// if the returned Extension flags contain the required extension.
	virtual GlContextExtensions contextExtensions() const = 0;

	// - Extension specific, might not be available -

	/// Calls the backend-specific swapInterval function with the given interval.
	/// Note that this function will set the swap interval on the surface that
	/// is current for this context.
	/// Default implementations implement it as not supported.
	/// \excpetion GlContextError If the extensions needed for this function is not available
	/// or there is no current surface for this context.
	/// \excpetion std::system_error If calling the native function failed
	virtual void swapInterval(int interval) const;
	virtual bool swapInterval(int interval, std::error_code&) const;

protected:
	virtual void initContext(GlApi, const GlConfig&);
	virtual bool makeCurrentImpl(const GlSurface&, std::error_code&) = 0;
	virtual bool makeNotCurrentImpl(std::error_code&) = 0;

protected:
	GlConfig config_;
	GlApi api_;
};

/// A guard to make a context current on construction and make it not current on destruction.
/// Note that GlContext does allow to manually make current/make not current since it may
/// not be desirable in every case to make the context not current on scope exit, e.g. sometimes
/// application want one context to be current all the time and not only when used.
class GlCurrentGuard : public nytl::NonMovable {
public:
	GlContext& context;

public:
	GlCurrentGuard(GlContext&, const GlSurface&);
	~GlCurrentGuard();
};

// small conversion helpers for GlConfigId
// mainly used by implementations
std::uintptr_t& glConfigNumber(GlConfigID& id);
GlConfigID& glConfigID(std::uintptr_t& number);

std::uintptr_t glConfigNumber(const GlConfigID& id);
GlConfigID glConfigID(const std::uintptr_t& number);

/// Returns a std::error_code for the given GlContextErrorCode.
/// Note that this also enables constructing std::error_code objects directly
/// only from a GlContextErrorCode value (which should be used over this).
/// Therefore the function has to use this naming.
std::error_condition make_error_condition(GlContextErrc);
std::error_code make_error_code(GlContextErrc);

} // namespace ny

namespace std {
	template<> struct is_error_condition_enum<ny::GlContextErrc> : public std::true_type {};
	template<> struct is_error_code_enum<ny::GlContextErrc> : public std::true_type {};
} // namespace std
