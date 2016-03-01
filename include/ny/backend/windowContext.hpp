#pragma once

#include <ny/include.hpp>
#include <ny/base/eventHandler.hpp>
#include <ny/window/defs.hpp>

#include <nytl/nonCopyable.hpp>
#include <nytl/vec.hpp>

#include <memory>
#include <bitset>

namespace ny
{

///\brief Abstract interface for a window context in the underlaying window system.
///The term "window" used in the documentation for this class is used for the underlaying native
///window, ny::WindowContext is totally independent from ny::Window and can even be used without it.
class WindowContext : public EventHandler
{
protected:
	EventHandler* eventHandler_ {nullptr};

public:
	WindowContext() = default;
    virtual ~WindowContext() = default;

	///Starts to draw on the window.
	///This function can always be called. The return (wrapped-up) DrawContext will only
	///be valid as long as the DrawGuard exists. 
	///\warning There shall be always only one DrawGuard (= valid and active DrawContext, 
	///drawing operation) per thread and only one drawing thread per DrawGuard/DrawContext.
	///\return A DrawGuard wrapper instance that holds the DrawContext that can be used to draw
	///the windows contents.
	virtual DrawGuard draw() = 0;

	///Sets the EventHandler that should receive the events associated with this windowContext.
	virtual void eventHandler(EventHandler& handler) { eventHandler_ = &handler; }

	///Returns the associated EventHandler of this windowContext, nullptr if there is none.
	virtual EventHandler* eventHandler() const { return eventHandler_; }

	///Asks the platform-specific windowing api for a window refresh.
    virtual void refresh() = 0;

	///Makes the window visible.
    virtual void show() = 0;

	///Hides the window.
    virtual void hide() = 0;

	///Signals that the window accepts drag-and-drops of the given DataTypes.
    virtual void droppable(const DataTypes&) = 0;

	///Sets the minimal size of the window.
    virtual void minSize(const Vec2ui&) = 0;

	///Sets the maximal size of the window.
    virtual void maxSize(const Vec2ui&) = 0;

	///Resizes the window.
    virtual void size(const Vec2ui& size) = 0; 

	///Sets the position of the window.
    virtual void position(const Vec2i& position) = 0; //...

	///Sets the mouse cursor of the window.
    virtual void cursor(const Cursor& c) = 0;

	///Returns the underlaying native window handle.
	virtual NativeWindowHandle nativeHandle() const = 0;

    //toplevel-specific
	///Maximized the window.
	///\warning Shall have only an effect for toplevel windows.
    virtual void maximize() = 0;

	///Minimized the window.
	///\warning Shall have only an effect for toplevel windows.
    virtual void minimize() = 0;

	///Sets the window in a fullscreen state.
	///\warning Shall have only an effect for toplevel windows.
    virtual void fullscreen() = 0;

	///Resets the window in normal toplevel state.
	///\warning Shall have only an effect for toplevel windows.
    virtual void toplevel() = 0; //or reset()?

	///Asks the window manager to start an interactive move for the window.
	///\param event A pointer to a MouseButtonEvent. Only required for some implementations, so
	///may also be a nullptr (does work then only on some backends!)
	///\warning Shall have only an effect for toplevel windows.
    virtual void beginMove(const MouseButtonEvent* ev) = 0;

	///Asks the window manager to start an interactive resizing for the window.
	///\param event A pointer to a MouseButtonEvent. Only required for some implementations, so
	///may also be a nullptr (does work then only on some backends!)
	///\warning Shall have only an effect for toplevel windows.
    virtual void beginResize(const MouseButtonEvent* event, WindowEdge edges) = 0;

	///Sets the title for the native window.
	///\warning Shall have only an effect for toplevel windows.
    virtual void title(const std::string& name) = 0;

	///Sets the icon of the native window.
	///\warning Shall have only an effect for toplevel windows.
	virtual void icon(const Image*) = 0; //may be only important for client decoration

	///Returns whether the window should be custom decorated.
	///\warning Will only return a valid value for toplevel windows.
	virtual bool customDecorated() const = 0;

	///Tires to adds the given window hints to the window.
	///\warning Window hints are only valid for toplevel windows.
	virtual void addWindowHints(WindowHints hints) = 0;

	///Tries to remove the given window hints from the window.
	///\warning Window  hints are only valid for toplevel windows.
	virtual void removeWindowHints(WindowHints hints) = 0;
};

}
