#pragma once

#include <ny/winapi/winapiInclude.hpp>
#include <ny/gl/glDrawContext.hpp>

#include <windows.h>
#include <GL/gl.h>

namespace ny
{

class wglDrawContext : public glDrawContext
{
friend class winapiWindowContext;

protected:
    virtual bool makeCurrentImpl() override;
    virtual bool makeNotCurrentImpl() override;

    winapiWindowContext& wc_;

    HDC handleDC_ = nullptr;
    HGLRC wglContext_ = nullptr;

    //on WM_CREATE called from winapiWindowContext
    bool setupContext();

public:
    wglDrawContext(winapiWindowContext& wc);
    ~wglDrawContext();

    virtual bool swapBuffers() override;
};

}
