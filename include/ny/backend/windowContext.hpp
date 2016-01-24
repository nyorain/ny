#pragma once

#include <ny/include.hpp>
#include <ny/window/defs.hpp>

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

public:
    WindowContext(Window& win) : window_(&win) {}
    virtual ~WindowContext(){}

    Window& window() const { return *window_; }
    virtual unsigned long additionalWindowHints() const { return 0; }

    virtual bool isVirtual() const { return 0; }
    virtual bool hasGL() const = 0; //defines if this window uses gl for rendering

    virtual void refresh() = 0;
    virtual void redraw() {} //???

    virtual DrawContext& beginDraw() = 0; //may throw
    virtual void finishDraw() = 0;

    virtual void show() = 0;
    virtual void hide() = 0;

    virtual void droppable(const DataTypes&){};
    virtual void addDropType(unsigned char){};
    virtual void removeDropType(unsigned char){};

    virtual void addWindowHints(unsigned long) {};
    virtual void removeWindowHints(unsigned long) {};

    virtual void minSize(const vec2ui&){};
    virtual void maxSize(const vec2ui&){};

    virtual void processEvent(const ContextEvent&){};

    virtual void size(const vec2ui& size, bool change = 1) = 0; 
    virtual void position(const vec2i& position, bool change = 1) = 0; //...

    virtual void cursor(const Cursor& c) = 0;
    virtual void updateCursor(const MouseCrossEvent*){}; //not needed in all

	virtual NativeWindowHandle nativeHandle() const = 0;

    //toplevel-specific
    virtual void maximized() = 0;
    virtual void minimized() = 0;
    virtual void fullscreen() = 0;
    virtual void toplevel() = 0; //or reset()?

    virtual void beginMove(const MouseButtonEvent* ev) = 0;
    virtual void beginResize(const MouseButtonEvent* ev, WindowEdge edges) = 0;

    virtual void title(const std::string& name) = 0;
	virtual void icon(const Image*){}; //may be only important for client decoration
};

/*
//virtual
class VirtualWindowContext : public WindowContext
{
protected:
    std::unique_ptr<RedirectDrawContext> drawContext_;

public:
    VirtualWindowContext(ChildWindow& win);
    virtual ~VirtualWindowContext();

    virtual bool isVirtual() const override { return 1; }
    virtual bool hasGL() const override { return parentContext().hasGL(); };

    virtual void refresh() override;

    virtual DrawContext& beginDraw() override;
    DrawContext& beginDraw(DrawContext& dc); //custom overload
    virtual void finishDraw() override;

    virtual void show() override {}
    virtual void hide() override {}

    virtual void cursor(const Cursor& c) override;
    virtual void updateCursor(const MouseCrossEvent* ev) override;

    virtual void size(const vec2ui& size, bool change = 1) override;
    virtual void position(const vec2i& position, bool change = 1) override;

    //throw at these functions? warning at least?
    virtual void maximized() override;
    virtual void minimized() override;
    virtual void fullscreen() override;
    virtual void toplevel() override;

    virtual void beginMove(const MouseButtonEvent*) override {}
    virtual void beginResize(const MouseButtonEvent*, windowEdge) override {}

    virtual void title(const std::string&) override {};

    WindowContext& parentContext() const;
};
*/

}
