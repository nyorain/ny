#pragma once

#include <ny/backend/winapi/include.hpp>
#include <ny/backend/winapi/windowContext.hpp>
#include <ny/backend/winapi/windows.hpp>

#include <ny/backend/common/gl.hpp>
#include <ny/backend/common/library.hpp>

namespace ny
{

///Creates and manages WglContexts and loading of the wgl api.
class WglSetup
{
public:
	WglSetup();
	~WglSetup();

	///Creates a new context and returns it. The returned context is still owned by this
	///object and should not be destroyed.
	///The returned context will never be returned from getContext(), therefore it can
	///be used without restrictions (since no one else will ever use the context).
	HGLRC createContext();

	///Returns a gl context. The returned context is owned by this object and should not
	///be destroyed. The returned object may be shared with other users, i.e. it should
	///always only be used in the ui thread and there are no guarantees for state after
	///making the context current/not current.
	HGLRC getContext();

protected:
	class WglContextWrapper;
	std::vector<WglContextWrapper> shared_; //contexts_[0] is the dummy context
	std::vector<WglContextWrapper> unique_;
	Library glLibrary_;
};

///OpenGL context implementation using the wgl api on windows.
class WglContext : public GlContext
{
public:
	WglContext(HWND hwnd, const GlContextSettings& settings);
	WglContext(HDC hdc, const GlContextSettings& settings);
	virtual ~WglContext();

	bool apply(std::error_code& ec) override;
	void* procAddr(nytl::StringParam name) const override;
	void* nativeHandle() const override { return static_cast<void*>(wglContext_); }

protected:
	static HMODULE glLibHandle();
	static HGLRC dummyContext(HDC* hdcOut = nullptr);
	static void assureWglLoaded();
	static void* wglProcAddr(const char* name);

protected:
	virtual bool makeCurrentImpl(std::error_code& ec) override;
	virtual bool makeNotCurrentImpl(std::error_code& ec) override;

	void init(const GlContextSettings& settings);
	void initPixelFormat(unsigned int depth, unsigned int stencil);
	PIXELFORMATDESCRIPTOR pixelFormatDescriptor() const;
	void createContext();
	void activateVsync();

protected:
	int pixelFormat_ = 0;
	HDC dc_ = nullptr;
	HWND hwnd_ = nullptr;
	HGLRC wglContext_ = nullptr;
};

///Winapi WindowContext using wgl (OpenGL) to draw.
class WglWindowContext : public WinapiWindowContext
{
public:
	WglWindowContext(WinapiAppContext& ctx, const WinapiWindowSettings& settings = {});
	~WglWindowContext();

	bool surface(Surface&) override;
	bool drawIntegration(WinapiDrawIntegration*) override { return false; }

protected:
	WNDCLASSEX windowClass(const WinapiWindowSettings& settings) override;

protected:
	std::unique_ptr<WglContext> wglContext_;
	HDC hdc_ {};
};

}
