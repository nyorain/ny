#pragma once

#include <ny/backend/winapi/include.hpp>
#include <ny/backend/winapi/windowContext.hpp>
#include <ny/backend/winapi/windows.hpp>

#include <ny/backend/common/gl.hpp>
#include <ny/backend/common/library.hpp>

namespace ny
{

//NOTE: the documentation is kinda confusing.
//There are 2 different types of sharing: The sharing on glcontext level
//e.g. wglShareLists and the shared using of one context handle.
//Maybe give one of them another name? document naming in gl.hpp (GlContext)

///Creates and manages WglContexts and loading of the wgl api.
///Does also query and hold the pixel format that the opengl functions were loaded for.
///If a context is created for another pixel format, the wgl speicification states that
///the loaded functions may be invalid (e.g. if using a different pixel format
///triggers anothe gpu/driver implementation to be used).
///All contexts are shared with each other, if possible (wglCreateContextAttribsARB).
class WglSetup
{
public:
	WglSetup();
	WglSetup(HWND dummy);
	~WglSetup();

	WglSetup(WglSetup&& other) noexcept;
	WglSetup& operator=(WglSetup&& other) noexcept;

	///Creates a new context and returns it. The returned context is still owned by this
	///object and should not be destroyed.
	///\param shared Whether the created should be shared with all already shared contexts.
	///If this is set to true, all existent and later created contexts will be shared with
	///the created one. Will be changed to false if sharing was requested but failed.
	///\param unique Whether the created context may not be returned from getContext.
	HGLRC createContext(bool& shared, bool unique = true);

	///Returns a gl context. The returned context is owned by this object and should not
	///be destroyed. The returned object may be shared with other GlContext objects, i.e. it
	///should always only be used in the ui thread and there are no guarantees for state after
	///making the context current/not current.
	///\param shared Whether the created should be shared with all already shared contexts.
	///If this is set to true, all existent and later created contexts will be shared with
	///the created one. Will be changed to false if sharing was requested but failed.
	HGLRC getContext(bool& shared);

	///The pixelformat the wgl api was loaded for.
	int pixelFormat() const { return pixelFormat_; }

	///Returns all shared context handles.
	std::vector<HGLRC> sharedContexts() const;

	///Returns one context that is shared with all contexts in the share chain.
	///If there is none such contexts, returns nullptr.
	HGLRC sharedContext() const;

	///Returns the symbol address for the given name or nullptr.
	void* procAddr(nytl::StringParam name) const;

	///Returns whether the state of this object is valid.
	bool valid() const { return dummyDC_; }

protected:
	class WglContextWrapper;
	std::vector<WglContextWrapper> shared_;
	std::vector<WglContextWrapper> unique_;

	Library glLibrary_;
	int pixelFormat_ {};

	HWND dummyWindow_ {};
	HDC dummyDC_ {};
};

///OpenGL context implementation using the wgl api on windows.
class WglContext : public GlContext
{
public:
	WglContext(WglSetup& setup, HDC hdc, const GlContextSettings& settings = {});
	virtual ~WglContext();

	bool apply(std::error_code& ec) override;
	void* procAddr(nytl::StringParam name) const override;
	void* nativeHandle() const override { return static_cast<void*>(wglContext_); }
	bool sharedWith(const GlContext& other) const override;

protected:
	virtual bool makeCurrentImpl(std::error_code& ec) override;
	virtual bool makeNotCurrentImpl(std::error_code& ec) override;

	void initPixelFormat(unsigned int depth, unsigned int stencil); //TODO, src
	void activateVsync();

protected:
	WglSetup* setup_ {};
	HDC hdc_ {};
	HGLRC wglContext_ {};
	bool shared_ {};
};

///Winapi WindowContext using wgl (OpenGL) to draw.
class WglWindowContext : public WinapiWindowContext
{
public:
	WglWindowContext(WinapiAppContext&, WglSetup&, const WinapiWindowSettings& = {});
	~WglWindowContext();

	bool surface(Surface&) override;
	bool drawIntegration(WinapiDrawIntegration*) override { return false; }

protected:
	WNDCLASSEX windowClass(const WinapiWindowSettings& settings) override;

protected:
	std::unique_ptr<WglContext> context_;
	HDC hdc_ {};
};

}
