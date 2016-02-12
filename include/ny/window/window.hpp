#pragma once

#include <ny/include.hpp>
#include <ny/window/cursor.hpp>
#include <ny/window/settings.hpp>
#include <ny/window/defs.hpp>
#include <ny/app/eventHandler.hpp>

#include <nytl/callback.hpp>
#include <nytl/vec.hpp>

#include <memory>
#include <functional>

namespace ny
{

///The Window class represents a surface which can be mapped on the clients screen.
class Window : public EventHandler
{
protected:
    //position and max/min - size. size itself inherited from surface
    Vec2i position_ {0, 0};
    Vec2ui size_ {0, 0};

    Vec2ui minSize_ {0, 0};
    Vec2ui maxSize_ {1 << 30, 1 << 30};

    //states saved in window, not in the context
    bool focus_ {0};
    bool mouseOver_ {0};
    bool shown_ {0};

    //current window cursor
    Cursor cursor_;

    //windowContext. backend specific object. Core element of window. 
	//window itself is not able to change anything, it has to communicate with the backend 
	//through this object
    std::unique_ptr<WindowContext> windowContext_;

    //stores window hints. most of window hints (e.g. Native<> or Maximize/Minimize etc. are not 
	//set by window class itself, but by the specific derivations of window
    //do NOT represent window states (as focus_, mouseOver_ etc) but general information 
	//about the window, which should not be changed by the window or its context after 
	//creation (of course they can be changed from the outside)
    unsigned long hints_ {0};

    //events - have to be protected?
    virtual void mouseMoveEvent(const MouseMoveEvent&);
    virtual void mouseCrossEvent(const MouseCrossEvent&);
    virtual void mouseButtonEvent(const MouseButtonEvent&);
    virtual void mouseWheelEvent(const MouseWheelEvent&);
    virtual void keyEvent(const KeyEvent&);
    virtual void sizeEvent(const SizeEvent&);
    virtual void positionEvent(const PositionEvent&);
    virtual void drawEvent(const DrawEvent&);
    virtual void showEvent(const ShowEvent&);
    virtual void focusEvent(const FocusEvent&);
	virtual void closeEvent(const CloseEvent&);

    //windowContext functions are protected, derived classes can make public aliases if needed. 
	//User should not be able to change backend Hints for button e.g
    void cursor(const Cursor& curs);

    void setWindowHints(unsigned long hints);
    void addWindowHints(unsigned long hints);
    void removeWindowHints(unsigned int hints);

    virtual void draw(DrawContext& dc);

	//init
    Window();
    Window(const Vec2ui& size, const WindowSettings& settings = {});

    void create(const Vec2ui& size, const WindowSettings& settings = {});

public:
    Callback<void(Window&)> onClose;
    Callback<void(Window&, DrawContext&)> onDraw;
    Callback<void(Window&, const Vec2ui&)> onResize;
    Callback<void(Window&, const Vec2i&)> onMove;
    Callback<void(Window&, const FocusEvent&)> onFocus;
    Callback<void(Window&, const ShowEvent&)> onShow;
    Callback<void(Window&, const MouseMoveEvent&)> onMouseMove;
    Callback<void(Window&, const MouseButtonEvent&)> onMouseButton;
    Callback<void(Window&, const MouseCrossEvent&)> onMouseCross;
    Callback<void(Window&, const MouseWheelEvent&)> onMouseWheel;
    Callback<void(Window&, const KeyEvent&)> onKey;

public:
	Window(const NativeWindowHandle& nativeHandle);
    virtual ~Window();

    virtual bool handleEvent(const Event& event) override;
	virtual void close();

    void size(const Vec2ui& size);
    void position(const Vec2i& position);

    std::vector<ChildWindow*> windowChildren() const;

    void move(const Vec2i& delta);

    void minSize(const Vec2ui& size);
    void maxSize(const Vec2ui& size);

    void refresh();

    void show();
    void hide();
    void toggleShow();

    //const getters
    const Vec2i& position() const { return position_; }
    const Vec2ui& size() const { return size_; }

    const Vec2ui& minSize() const { return minSize_; }
    const Vec2ui& maxSize() const { return maxSize_; }

    bool focus() const { return focus_; }
    bool mouseOver() const { return mouseOver_; }
    bool shown() const { return shown_; }

    unsigned long windowHints() const { return hints_; }
    WindowContext* windowContext() const { return windowContext_.get(); }

    const Cursor& cursor() const { return cursor_; }
};

}

