#pragma once

#include "x11Include.hpp"
#include "windowContext.hpp"
#include "graphics/gl/glContext.hpp"

#include <EGL/egl.h>

namespace ny
{

class x11EGLContext : public glContext
{

};

//
class x11EGLWindowContext
{
protected:
    glDrawContext* drawContext_;

public:
    x11EGLWindowContext(x11WindowContext& wc);
    virtual ~x11EGLWindowContext();
};

//
class x11EGLToplevelWindowContext : public x11ToplevelWindowContext, public x11EGLWindowContext
{
public:
    x11EGLToplevelWindowContext(toplevelWindow& win, const x11WindowContextSettings& s = x11WindowContextSettings());

    virtual drawContext& beginDraw();
    virtual void finishDraw();
    virtual void setSize(vec2ui size, bool change = 1);

    virtual bool hasGL() const { return 1; }
};

//
class x11EGLChildWindowContext : public x11ChildWindowContext, public x11EGLWindowContext
{
public:
    x11EGLChildWindowContext(childWindow& win, const x11WindowContextSettings& s = x11WindowContextSettings());

    virtual drawContext& beginDraw();
    virtual void finishDraw();
    virtual void setSize(vec2ui size, bool change = 1);

    virtual bool hasGL() const { return 1; }
};


}
