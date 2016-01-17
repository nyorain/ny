#pragma once

#include <ny/include.hpp>
#include <ny/window/windowDefs.hpp>

#include <nytl/nonCopyable.hpp>
#include <nytl/vec.hpp>

#include <memory>

namespace ny
{

//windowContex
class WindowContext : public nonCopyable
{
protected:
    Window* window_;
    unsigned long hints_; //specific context hints. can be declared by every backend

public:
    WindowContext(Window& win, unsigned long hints = 0) : window_(&win), hints_(hints) {}
    WindowContext(Window& win, const WindowContextSettings& s) : window_(&win), hints_(s.hints) {}
    virtual ~WindowContext(){}

    Window& window() const { return *window_; }
    unsigned long contextHints() const { return hints_; }
    virtual unsigned long additionalWindowHints() const { return 0; }

    virtual bool isVirtual() const { return 0; }
    virtual bool hasGL() const = 0; //defines if this window uses gl for rendering

    virtual void refresh() = 0;
    virtual void redraw(); //needed?

	//may throw
    virtual DrawContext& beginDraw() = 0;
    virtual void finishDraw() = 0;

    virtual void show() = 0;
    virtual void hide() = 0;

    virtual void droppable(const DataTypes&){};
    virtual void addDropType(unsigned char){};
    virtual void removeDropType(unsigned char){};

    //window hints
    virtual void addWindowHints(unsigned long hints) = 0;
    virtual void removeWindowHints(unsigned long hints) = 0;

    virtual void addContextHints(unsigned long hints) { hints_ |= hints; }
    virtual void removeContextHints(unsigned long hints) { hints_ &= ~hints; }

    virtual void minSize(const vec2ui&){};
    virtual void maxSize(const vec2ui&){};

    //event
    virtual void processEvent(const ContextEvent&){};

    virtual void size(const vec2ui& size, bool change = 1) = 0; 
    virtual void position(const vec2i& position, bool change = 1) = 0; //...

    virtual void cursor(const Cursor& c) = 0;
    virtual void updateCursor(const MouseCrossEvent*){}; //not needed in all


    //toplevel-specific/////
    virtual void maximized() = 0;
    virtual void minimized() = 0;
    virtual void fullscreen() = 0;
    virtual void normal() = 0; //or reset()?

    virtual void beginMove(const MouseButtonEvent* ev) = 0;
    virtual void beginResize(const MouseButtonEvent* ev, windowEdge edges) = 0;

    virtual void title(const std::string& name) = 0;
	virtual void icon(const Image*){}; //may be only important for client decoration
};


/*
//deprecated
//virtual////////////////////////////////////////////
class virtualWindowContext : public windowContext
{
protected:
    std::unique_ptr<redirectDrawContext> drawContext_;

public:
    virtualWindowContext(childWindow& win, const windowContextSettings& = windowContextSettings());
    virtual ~virtualWindowContext();

    virtual bool isVirtual() const override { return 1; }
    virtual bool hasGL() const override { return getParentContext()->hasGL(); };

    virtual void refresh() override;

    virtual drawContext* beginDraw() override;
    drawContext* beginDraw(drawContext& dc); //custom overload
    virtual void finishDraw() override;

    virtual void show() override {}
    virtual void hide() override {}

    virtual void setCursor(const cursor& c) override {}
    virtual void updateCursor(const mouseCrossEvent* ev) override;

    virtual void setSize(vec2ui size, bool change = 1) override;
    virtual void setPosition(vec2i position, bool change = 1) override;

    //
    virtual void addWindowHints(unsigned long hint) override {}
    virtual void removeWindowHints(unsigned long hint) override {}

    virtual void addContextHints(unsigned long hints) override {}
    virtual void removeContextHints(unsigned long hints) override {}

    //throw at these functions? warning at least?
    virtual void setMaximized() override {}
    virtual void setMinimized() override {}
    virtual void setFullscreen() override {}
    virtual void setNormal() override {} //or reset()?

    virtual void beginMove(const mouseButtonEvent* ev) override {}
    virtual void beginResize(const mouseButtonEvent* ev, windowEdge edges) override {}

    virtual void setTitle(const std::string& name) override {};

    /////////
    windowContext* getParentContext() const;
};
*/

}
