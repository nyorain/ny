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
	static void* glLibHandle();
	static void* glesLibHandle();

protected:
    virtual bool makeCurrentImpl() override;
    virtual bool makeNotCurrentImpl() override;

    WinapiWindowContext* wc_;

    HDC dc_ = nullptr;
    HGLRC wglContext_ = nullptr;

public:
    WglContext(WinapiWindowContext& wc);
    virtual ~WglContext();

    virtual bool apply() override;
	virtual void* procAddr(const char* name) const override;
};

///Winapi WindowContext using wgl (OpenGL) to draw.
class WglWindowContext : public WinapiWindowContext
{
protected:
	std::unique_ptr<WglContext> wglContext_;
	std::unique_ptr<GlDrawContext> drawContext_;

public:
	WglWindowContext(WinapiAppContext& ctx, const WinapiWindowSettings& settings = {});
	~WglWindowContext();
	virtual DrawGuard draw() override;
};

}
