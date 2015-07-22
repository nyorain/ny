#pragma once

#include <ny/wayland/waylandInclude.hpp>

#include <ny/gl/egl.hpp>
#include <ny/util/nonCopyable.hpp>
#include <ny/util/vec.hpp>
#include <wayland-egl.h>

namespace ny
{

//waylandEGLAppContext//////////////////////////////////////////////////
class waylandEGLAppContext : public eglAppContext
{
public:
    waylandEGLAppContext(waylandAppContext* ac);
    ~waylandEGLAppContext();
};


////
class waylandEGLContext: public eglContext
{
protected:
    wl_egl_window* wlEGLWindow_ = nullptr;
    glDrawContext* drawContext_ = nullptr;

    void setSize(vec2ui size);

public:
    waylandEGLContext(const waylandWindowContext& wc);
    virtual ~waylandEGLContext();

    wl_egl_window& getWlEGLWindow() const { return *wlEGLWindow_; };
    glDrawContext& getDC() const { return *drawContext_; }
};


}
