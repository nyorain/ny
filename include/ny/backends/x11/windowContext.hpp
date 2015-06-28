#pragma once

#include <ny/backends/x11/x11Include.hpp>
#include <ny/backends/appContext.hpp>
#include <ny/backends/windowContext.hpp>
#include <ny/backends/backend.hpp>
#include <ny/window/windowEvents.hpp>

#include <X11/Xutil.h>

#include <map>
#include <mutex>

namespace ny
{

//x11Event//////////////////////////////////////////////////////////////////////////////////////
class x11EventData : public eventData
{
public:
    x11EventData(const XEvent& e) : ev(e) {};
    XEvent ev;
};

/////
const unsigned int X11Reparent = 1;
class x11ReparentEvent : public contextEvent
{
public:
    x11ReparentEvent(const XReparentEvent& e) : contextEvent(X11, X11Reparent), ev(e) {};
    XReparentEvent ev;
};

//x11WindowContextSettings
class x11WindowContextSettings : public windowContextSettings
{

};

//x11AppContext//////////////////////////////////////////////////////////////////////////////7

class x11WindowContext : public virtual windowContext
{
protected:
    x11AppContext* context_;
    Display* xDisplay_;

    Window xWindow_;
    XVisualInfo* xVinfo_;
    int xScreenNumber_;

    unsigned long states_;
    unsigned long mwmFuncHints_;
    unsigned long mwmDecoHints_;

    bool matchVisualInfo();

    void create(unsigned int winType = InputOutput);
    virtual void create(unsigned int winType, unsigned long attrMask, XSetWindowAttributes attr) = 0; //create window with the options set

public:
    x11WindowContext(window& win, const x11WindowContextSettings& settings = x11WindowContextSettings());
    virtual ~x11WindowContext();

    //high-level, have to be implemented/////////////////////////////////////////////////////////////////
    virtual void refresh();

    virtual drawContext& beginDraw() = 0;
    virtual void finishDraw();

    virtual void show();
    virtual void hide();

    virtual void raise();
    virtual void lower();

    virtual void requestFocus();

    virtual void setSize(vec2ui size, bool change = 1);
    virtual void setPosition(vec2i position, bool change = 1);

    virtual void setCursor(const cursor& c);

    virtual void setMinSize(vec2ui size);
    virtual void setMaxSize(vec2ui size);

    virtual void addWindowHints(unsigned long hints);
    virtual void removeWindowHints(unsigned long hints);

    virtual void addContextHints(unsigned long hints);
    virtual void removeContextHints(unsigned long hints);

    virtual void mapEventType(unsigned int t);
    virtual void unmapEventType(unsigned int t);

    virtual void sendContextEvent(contextEvent& e);

    //x11-specific///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //get
    Window getXWindow() const { return xWindow_; }
    XVisualInfo* getXVinfo() const { return xVinfo_; }
    Display* getXDisplay() const { return xDisplay_; }
    //Visual* getXVisual() const { return xVinfo_->visual; }

    //general
    void setOverrideRedirect(bool redirect = 1);
    void setTransientFor(window* win);
    void setCursor(unsigned int xCusrsorID);

    //motif
    void setMwmDecorationHints(unsigned long hints); //maybe only in toplevelWC? see also -> add and remove window hints
    void setMwmFunctionHints(unsigned long hints);
    void setMwmHints(unsigned long deco, unsigned long func);

    unsigned long getMwmDecorationHints();
    unsigned long getMwmFunctionHints();

    //ewmh
    void addState(Atom state);
    void removeState(Atom state);
    void toggleState(Atom state);

    unsigned long getStates() const { return states_; };
    void refreshStates();

    void setType(Atom type);
    Atom getType();

    void addAllowedAction(Atom action); //only does something when custom handled
    void removeAllowedAction(Atom action);
    std::vector<Atom> getAllowedAction();

    void wasReparented(x11ReparentEvent& ev); //called from appContext through contextEvent
};

//
////
class x11ToplevelWindowContext : public toplevelWindowContext, public x11WindowContext
{
protected:
    using x11WindowContext::create;
    void create(unsigned int winType, unsigned long attrMask, XSetWindowAttributes attr); //create window with the options set

public:
    x11ToplevelWindowContext(toplevelWindow& win, const x11WindowContextSettings& settings = x11WindowContextSettings(), bool pcreate = 1);

    virtual void setMaximized();
    virtual void setMinimized();
    virtual void setFullscreen();
    virtual void setNormal();

    virtual void beginMove(mouseButtonEvent* ev);
    virtual void beginResize(mouseButtonEvent* ev, windowEdge wedge);

    virtual void setBorderSize(unsigned int size);

    virtual void setIcon(const image* img);
};


////
class x11ChildWindowContext : public childWindowContext, public x11WindowContext
{
protected:
    using x11WindowContext::create;
    void create(unsigned int winType, unsigned long attrMask, XSetWindowAttributes attr); //create window with the options set

public:
    x11ChildWindowContext(childWindow& win, const x11WindowContextSettings& settings = x11WindowContextSettings(), bool pcreate = 1);
};




}
