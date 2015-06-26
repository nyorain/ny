#pragma once

#include "include.hpp"

#include "window/windowDefs.hpp"

#include "utils/nonCopyable.hpp"
#include "utils/vec.hpp"

namespace ny
{

/////
class windowContext : public nonCopyable
{
protected:
    window& window_;
    unsigned long hints_;

public:
    windowContext(window& win, unsigned long hints = 0);
    windowContext(window& win, const windowContextSettings&);
    virtual ~windowContext(){}

    window& getWindow() const { return window_; }
    unsigned long getContextHints() const { return hints_; }

    virtual unsigned long getAdditionalHints() const { return 0; }

    virtual bool isVirtual() const { return 0; }
    virtual bool hasGL() const { return 0; }

    virtual void refresh() = 0;
    virtual void redraw();

    virtual drawContext& beginDraw() = 0;
    virtual void finishDraw() = 0;

    virtual void show() = 0;
    virtual void hide() = 0;

    virtual void raise() = 0;
    virtual void lower() = 0;

    virtual void requestFocus() = 0;

    virtual void setDroppable(const dataTypes& type){};
    virtual void addDropType(unsigned char type){};
    virtual void removeDropType(unsigned char type){};

    //virtual void setShape(shape*) = 0;

    //just window hints
    virtual void addWindowHints(unsigned long hints) = 0;
    virtual void removeWindowHints(unsigned long hints) = 0;

    //context specific
    virtual void addContextHints(unsigned long hints) { hints_ |= hints; }
    virtual void removeContextHints(unsigned long hints) { hints_ &= ~hints; }


    virtual void mapEventType(unsigned int t){};
    virtual void unmapEventType(unsigned int t){};

    virtual void setMinSize(vec2ui size){};
    virtual void setMaxSize(vec2ui size){};

    //event
    virtual void sendContextEvent(contextEvent& e){};

    virtual void setSize(vec2ui size, bool change = 1) = 0; //change states if the window on the backend has to be resized
    virtual void setPosition(vec2i position, bool change = 1) = 0; //...

    virtual void setCursor(const cursor& c) = 0;
    virtual void updateCursor(mouseCrossEvent* ev){}; //not needed in all backends
};

class toplevelWindowContext : public virtual windowContext
{
public:
    toplevelWindowContext(toplevelWindow& win, const windowContextSettings& = windowContextSettings());

    toplevelWindow& getToplevelWindow() const;

    virtual void setMaximized() = 0;
    virtual void setMinimized() = 0;
    virtual void setFullscreen() = 0;
    virtual void setNormal() = 0; //or reset()?

    virtual void beginMove(mouseButtonEvent* ev) = 0;
    virtual void beginResize(mouseButtonEvent* ev, windowEdge edges) = 0;

    virtual void setBorderSize(unsigned int size) = 0;
    virtual void setName(std::string name){};

    virtual bool isVirtual() const final { return 0; }
};

class childWindowContext : public virtual windowContext
{
public:
    childWindowContext(childWindow& win, const windowContextSettings& = windowContextSettings());

    childWindow& getChildWindow() const;
};

//virtual///////////////////////////////////////////////////////////////////////////////////////

class virtualWindowContext : public childWindowContext
{
protected:
    redirectDrawContext* drawContext_;
public:
    virtualWindowContext(childWindow& win, const windowContextSettings& = windowContextSettings());

    virtual bool isVirtual() const { return 1; }

    virtual void refresh();

    virtual drawContext& beginDraw();
    virtual void finishDraw();

    virtual void show(){}
    virtual void hide(){}

    virtual void raise(){}
    virtual void lower(){}

    virtual void requestFocus(){}

    virtual void setCursor(const cursor& c){}
    virtual void updateCursor(mouseCrossEvent* ev);

    virtual void setSize(vec2ui size, bool change = 1);
    virtual void setPosition(vec2i position, bool change = 1);

    ///
    virtual void setWindowHints(unsigned long hints){}
    virtual void addWindowHints(unsigned long hint){}
    virtual void removeWindowHints(unsigned long hint){}

    virtual void setContextHints(unsigned long hints){}
    virtual void addContextHints(unsigned long hints){}
    virtual void removeContextHints(unsigned long hints){}

    virtual void setSettings(const windowContextSettings& s){}

    //////////////////
    windowContext* getParentContext() const;
};

}
