#pragma once

#include <ny//include.hpp"
#include <ny/backends/windowContext.hpp"

#include <ny/backends/winapi/gdiDrawContext.hpp>

#include <windows.h>

namespace ny
{

class winapiWindowContext;
class winapiToplevelWindowContext;
class winapiChildWindowContext;
class winapiAppContext;
class winapiGdiDrawContext;

typedef winapiWindowContext winapiWC;
typedef winapiToplevelWindowContext winapiToplevelWC;
typedef winapiChildWindowContext winapiChildWC;

class winapiWindowContextSettings : public windowContextSettings
{
public:

};

typedef winapiWindowContextSettings winapiWS;

//wc
class winapiWindowContext : public virtual windowContext
{
protected:
    static unsigned int highestID;

    HWND m_handle;
    HINSTANCE m_instance;
    WNDCLASSEX m_wndClass;

    drawContext* m_drawContext;

public:
    winapiWindowContext(window* win, const winapiWindowContextSettings& settings = winapiWindowContextSettings());
    virtual ~winapiWindowContext();

    virtual void refresh();

    virtual drawContext& beginDraw();
    virtual void finishDraw();

    virtual void show();
    virtual void hide();

    virtual void raise();
    virtual void lower();

    virtual void requestFocus();

    virtual void setWindowHints(const unsigned long hints);
    virtual void addWindowHints(const unsigned long hint);
    virtual void removeWindowHints(const unsigned long hint);

    virtual void setContextHints(const unsigned long hints);
    virtual void addContextHints(const unsigned long hints);
    virtual void removeContextHints(const unsigned long hints);

    virtual void setSettings(const windowContextSettings& s);

    virtual void setSize(vec2ui size, bool change = 1);
    virtual void setPosition(vec2i position, bool change = 1); //...

    virtual void setWindowCursor(const cursor& c);

    /////////////////////////////////////////////////////////////////////////////////
    //winapi specific

    HWND getHandle() const { return m_handle; }
    HINSTANCE getInstance() const { return m_instance; }
    WNDCLASSEX getWndClassEx() const { return m_wndClass; }
};

//toplevel
class winapiToplevelWindowContext : public toplevelWindowContext, public winapiWindowContext
{
public:
    winapiToplevelWindowContext(toplevelWindow* win, const winapiWindowContextSettings& settings = winapiWindowContextSettings());

    virtual void setMaximized();
    virtual void setMinimized();
    virtual void setFullscreen();
    virtual void setNormal();

    virtual void setMinSize();
    virtual void setMaxSize();

    virtual void beginMove(mouseButtonEvent* ev);
    virtual void beginResize(mouseButtonEvent* ev, windowEdge edges);

    virtual void setBorderSize(unsigned int size){};
};

//child
class winapiChildWindowContext : public childWindowContext, public winapiWindowContext
{
public:
    winapiChildWindowContext(childWindow* win, const winapiWindowContextSettings& settings = winapiWindowContextSettings());
};


}
