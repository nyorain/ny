#pragma once

#include <ny/backend/winapi/include.hpp>
#include <ny/backend/winapi/windowContext.hpp>
#include <ny/backend/winapi/windows.hpp>
#include <ny/backend/common/gl.hpp>
#include <evg/gl/drawContext.hpp>

namespace ny
{

///OpenGL context implementation using the wgl api on windows.
class WglContext : public GlContext
{
public:
    WglContext(WinapiWindowContext& wc);
    virtual ~WglContext();

    virtual bool apply() override;
	virtual void* procAddr(const char* name) const override;

protected:
	static HMODULE glLibHandle();
	static HMODULE glesLibHandle();

	static HGLRC dummyContext(HDC* hdcOut = nullptr);
	static void assureWglLoaded();
	static void* wglProcAddr(const char* name);

protected:
    virtual bool makeCurrentImpl() override;
    virtual bool makeNotCurrentImpl() override;

	void initPixelFormat(unsigned int depth, unsigned int stencil);
	PIXELFORMATDESCRIPTOR pixelFormatDescriptor() const;
	void createContext();
	void activateVsync();

protected:
	int pixelFormat_ = 0;
    WinapiWindowContext* wc_ = nullptr;
    HDC dc_ = nullptr;
    HGLRC wglContext_ = nullptr;
};

///Winapi WindowContext using wgl (OpenGL) to draw.
class WglWindowContext : public WinapiWindowContext
{
public:
	WglWindowContext(WinapiAppContext& ctx, const WinapiWindowSettings& settings = {});
	~WglWindowContext();

protected:
	virtual WNDCLASSEX windowClass(const WinapiWindowSettings& settings) override;

protected:
	std::unique_ptr<WglContext> wglContext_;
	std::unique_ptr<evg::GlDrawContext> drawContext_;
};

}
