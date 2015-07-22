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
class waylandEGLDrawContext: public eglDrawContext
{
protected:
    wl_egl_window* wlEGLWindow_ = nullptr;

public:
    waylandEGLDrawContext(const waylandWindowContext& wc);
    virtual ~waylandEGLDrawContext();

    wl_egl_window& getWlEGLWindow() const { return *wlEGLWindow_; };
    void setSize(vec2ui size);
};


}
