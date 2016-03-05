#pragma once

#include <ny/backend/winapi/include.hpp>
#include <ny/backend/winapi/windowContext.hpp>
#include <ny/draw/gl/context.hpp>

#include <windows.h>
#include <GL/gl.h>

namespace ny
{

///OpenGL context implementation using the wgl api on windows.
class WglContext : public GlContext
{
protected:
    virtual bool makeCurrentImpl() override;
    virtual bool makeNotCurrentImpl() override;

    winapiWindowContext& wc_;

    HDC handleDC_ = nullptr;
    HGLRC wglContext_ = nullptr;

    //on WM_CREATE called from winapiWindowContext
    bool setupContext();

public:
    WglDrawContext(winapiWindowContext& wc);
    virtual ~WglDrawContext();

    virtual bool swapBuffers() override;
};

///Winapi WindowContext using wgl (OpenGL) to draw.
class WglWindowContext : public WinapiWindowContext
{
protected:
	std::unique_ptr<WglContext> wglContext_;
	std::unique_ptr<GlDrawContext> drawContext_;

public:
	WglWindowContext(WinapiAppContext& ctx, const WinapiWindowSettings& settings = {});
	virtual DrawGuard draw() override;
};

}
