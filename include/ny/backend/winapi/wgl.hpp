#pragma once

#include <ny/backend/winapi/include.hpp>
#include <ny/backend/winapi/windowContext.hpp>
#include <ny/backend/winapi/windows.hpp>
#include <ny/backend/common/gl.hpp>

namespace ny
{

///OpenGL context implementation using the wgl api on windows.
class WglContext : public GlContext
{
public:
    WglContext(HWND hwnd, const GlContextSettings& settings);
    WglContext(HDC hdc, const GlContextSettings& settings);
    virtual ~WglContext();

    virtual bool apply() override;
	virtual void* procAddr(const char* name) const override;

protected:
	static HMODULE glLibHandle();
	static HGLRC dummyContext(HDC* hdcOut = nullptr);
	static void assureWglLoaded();
	static void* wglProcAddr(const char* name);

protected:
    virtual bool makeCurrentImpl() override;
    virtual bool makeNotCurrentImpl() override;

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
};

}
