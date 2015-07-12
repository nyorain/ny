#pragma once

#include <ny/x11/x11Include.hpp>
#include <ny/x11/x11WindowContext.hpp>
#include <ny/gl/glContext.hpp>

namespace ny
{

//wrappers are needed because glx.h should not be inculded in a header file due to incompatibility with glbinding
struct glxFBC;
struct glxc;

class glxWindowContextSettings : public x11WindowContextSettings
{
public:
    unsigned int depth = 24;
    unsigned int stencil = 8;
};

////////////////////////////
class glxContext : public glContext
{
protected:
    Drawable drawable_;
    glxc* glxContext_;

public:
    glxContext(Drawable d, glxFBC* conf);
    ~glxContext();

    bool makeCurrent();
    bool makeNotCurrent();
};

////////////////////////////////
class glxWindowContext
{
protected:
    glxContext* glContext_;
    glDrawContext* drawContext_;
    x11WindowContext& wc_;
    glxFBC* fbc_;

    XVisualInfo* initFBConfig(const glxWindowContextSettings& s);
    void init();

public:
    glxWindowContext(x11WindowContext& wctx);
    virtual ~glxWindowContext();
};


////////////////////////////////////////////////
class glxToplevelWindowContext : public x11ToplevelWindowContext, public glxWindowContext
{
public:
    glxToplevelWindowContext(toplevelWindow& win, const glxWindowContextSettings& s = glxWindowContextSettings());

    virtual drawContext& beginDraw();
    virtual void finishDraw();
    virtual void setSize(vec2ui size, bool change = 1);

    virtual bool hasGL() const { return 1; }
};

//
class glxChildWindowContext : public x11ChildWindowContext, public glxWindowContext
{
public:
    glxChildWindowContext(childWindow& win, const glxWindowContextSettings& s = glxWindowContextSettings());

    virtual drawContext& beginDraw();
    virtual void finishDraw();
    virtual void setSize(vec2ui size, bool change = 1);

    virtual bool hasGL() const { return 1; }
};


}
