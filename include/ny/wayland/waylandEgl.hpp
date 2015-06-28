#pragma once

#include <ny/wayland/waylandInclude.hpp>

#include <ny/wayland/waylandWindowContext.hpp>
#include <ny/gl/glContext.hpp>
#include <ny/gl/glDrawContext.hpp>

#include <EGL/egl.h>
#include <wayland-egl.h>

namespace ny
{

////
class waylandGLContext
{
protected:
    EGLSurface eglSurface_;
    wl_egl_window* wlEGLWindow_;

    glDrawContext* drawContext_;

    void glSetSize(vec2ui size);

public:
    waylandGLContext(waylandWindowContext& wc);
    virtual ~waylandGLContext();

    const EGLSurface& getEGLSurface() const { return eglSurface_; };
    wl_egl_window* getWlEGLWindow() const { return wlEGLWindow_; };
};

////
class waylandGLToplevelWindowContext : public waylandToplevelWindowContext, public waylandGLContext
{
public:
    waylandGLToplevelWindowContext(toplevelWindow& win, const waylandWindowContextSettings& s = waylandWindowContextSettings());

    virtual drawContext& beginDraw();
    virtual void finishDraw();
    virtual void setSize(vec2ui size, bool change = 1);

    virtual bool hasGL() const { return 1; }
};

/////
class waylandGLChildWindowContext : public waylandChildWindowContext, public waylandGLContext
{
public:
    waylandGLChildWindowContext(childWindow& win, const waylandWindowContextSettings& s = waylandWindowContextSettings());

    virtual drawContext& beginDraw();
    virtual void finishDraw();
    virtual void setSize(vec2ui size, bool change = 1);

    virtual bool hasGL() const { return 1; }
};

}
