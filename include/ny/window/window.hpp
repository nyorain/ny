#pragma once

#include <ny/include.hpp>
#include <ny/window/cursor.hpp>
#include <ny/window/settings.hpp>
#include <ny/window/defs.hpp>
#include <ny/base/eventHandler.hpp>

#include <nytl/vec.hpp>
#include <nytl/callback.hpp>

#include <memory>
#include <functional>

namespace ny
{

///The Window class represents a surface which can be mapped on the clients screen.
///The Window class is fully platform independent, if you want better control about (especially
///platform dependent) window features, see ny::WindowContext.
///Every Window is associated with an App instance and has a backend-specific
///WindowContext implementation. Windows that have no valid WindowContext are called
///invalid and exist e.g. if the native Window was closed (and the WindowContext object therefore
///destroyed) but the applications Window object still exists.
///Windows store information like position/size, maximal and minimal size.
///The Window contents can be drawn on a various DrawContext, so they can e.g. easily rendered
///to an image or an implementation-specific texture.
///The Window class provides many Callbacks for various events.
class Window : public EventHandler
{
protected:
	//The app this windows belongs to
	App* app_;

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

protected:
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
	///Sets the cursor apperance that should be used for this window.
    void cursor(const Cursor& curs);

	///Draws the windows contents on a given DrawContext.
    virtual void draw(DrawContext& dc);

	///Basically default-constructs the window. If this function is called by a derived class
	///there MUST be a call to create() later, or all important variables have to be
	///initialized later on manually.
    Window();

	///Constructs the Window object with all needed parameters to create a underlaying
	///WindowContext.
    Window(App& app, const Vec2ui& size, const WindowSettings& settings = {});

	///Creates a WindowContext for the window and initializes all variables.
    void create(App& app, const Vec2ui& size, const WindowSettings& settings = {});

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

	///Closes the window. Closing a window is equal to destroying its context, which means
	///that it cannot be reopened again.
	virtual void close();

	///Sets the size of the window.
    void size(const Vec2ui& size);

	///Sets the position of the window. Might have no real effect on some platforms (e.g. tiling
	///window manager).
    void position(const Vec2i& position);

	///Moves the windows (i.e. changes its positions) by the given difference.
    void move(const Vec2i& delta);

	///Specifies the minimal size this window can have.
    void minSize(const Vec2ui& size);

	///Specifies the maximal size this window can have.
    void maxSize(const Vec2ui& size);

	///Refreshes the window.
	///In some implementations this might trigger an direct redraw. Usually this function
	///requests some kind of redraw at the window manager.
    void refresh();

	///Makes the window visibile.
    void show();

	///Makes the window invisible.
    void hide();

	///If the window is visible, it will be hidden; if it is hidden it will be made visible.
    void toggleShow();

	///Returns the position this window has.
    const Vec2i& position() const { return position_; }

	///Returns the size of the window.
    const Vec2ui& size() const { return size_; }

	///Returns the specified minimal size this window can have.
    const Vec2ui& minSize() const { return minSize_; }

	///Returns the specified maximal size this window can have.
    const Vec2ui& maxSize() const { return maxSize_; }

	///Returns the app that is associated with this window.
	///Note that this returns a reference, which means that after construction, there shall exist
	///no window that does not have an associated app instance.
	App& app() const { return *app_; }

	///Returns whether this window has (keyboard) focus.
    bool focus() const { return focus_; }

	///Returns whether the mouse is currently over this window.
    bool mouseOver() const { return mouseOver_; }

	///Returns whether the window is shown (visible).
    bool shown() const { return shown_; }

	///Returns the underlaying WindowContext object of this window.
    WindowContext* windowContext() const { return windowContext_.get(); }

	///Returns the cursor that is used for this window.
    const Cursor& cursor() const { return cursor_; }
};

}

