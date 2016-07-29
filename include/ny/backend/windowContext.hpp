#pragma once

#include <ny/include.hpp>
#include <ny/base/eventHandler.hpp>
#include <ny/window/defs.hpp>

#include <nytl/nonCopyable.hpp>
#include <nytl/vec.hpp>
#include <nytl/any.hpp>

#include <memory>
#include <bitset>

namespace ny
{

///\brief Abstract interface for a window context in the underlaying window system.
///The term "window" used in the documentation for this class is used for the underlaying native
///window, ny::WindowContext is totally independent from ny::Window and can be used without it.
class WindowContext : public EventHandler
{
public:
	WindowContext() = default;
    virtual ~WindowContext() = default;

	///Sets the EventHandler that should receive the events associated with this windowContext.
	///The event handler will receive information about window state changes and input.
	virtual void eventHandler(EventHandler& handler) { eventHandler_ = &handler; }

	///Returns the associated EventHandler of this windowContext, nullptr if there is none.
	virtual EventHandler* eventHandler() const { return eventHandler_; }

	///Asks the platform-specific windowing api for a window refresh.
	///Note that this refresh might not take place immediatly.
    virtual void refresh() = 0;

	///Makes the window visible.
    virtual void show() = 0;

	///Hides the window.
    virtual void hide() = 0;

	///Signals that the window accepts drag-and-drops of the given DataTypes.
    virtual void droppable(const DataTypes&) = 0;

	///Sets the minimal size of the window.
	///Might have no effect on certain backends.
    virtual void minSize(const Vec2ui&) = 0;

	///Sets the maximal size of the window.
	///Might have no effect on certain backends.
    virtual void maxSize(const Vec2ui&) = 0;

	///Resizes the window.
    virtual void size(const Vec2ui& size) = 0;

	///Sets the position of the window.
	///Note that on some backends or if the window is in a fullscreen/maximized state, this
	///might have no effect.
    virtual void position(const Vec2i& position) = 0;

	///Sets the mouse cursor of the window. The mouse cursor will only be changed to the given
	///cursor when the pointer is oven this window.
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
	///If it is currently maximized, minimized or in fullscreen, these states will be removed.
	///\warning Shall have only an effect for toplevel windows.
    virtual void normalState() = 0; //or reset()?

	///Asks the window manager to start an interactive move for the window.
	///\param event A pointer to a MouseButtonEvent. Only required for some implementations, so
	///may also be a nullptr (does work then only on some backends!)
	///\warning Shall have only an effect for toplevel windows.
    virtual void beginMove(const MouseButtonEvent* ev) = 0;

	///Asks the window manager to start an interactive resizing for the window.
	///\param event A pointer to a MouseButtonEvent. Only required for some implementations, so
	///may also be a nullptr (does work then only on some backends!)
	///\warning Shall have only an effect for toplevel windows.
    virtual void beginResize(const MouseButtonEvent* event, WindowEdges edges) = 0;

	///Sets the title for the native window.
	///\warning Shall have only an effect for toplevel windows.
    virtual void title(const std::string& name) = 0;

	///Sets the icon of the native window.
	///\warning Shall have only an effect for toplevel windows.
	// virtual void icon(const Image*) = 0; //may be only important for client decoration

	///Returns whether the window should be custom decorated.
	///Custom decoration can either be manually triggered by setting the custom decorated
	///window hint or the backend may tell the client to decrate itself (i.e. wayland).
	///\warning Will only return a valid value for toplevel windows.
	virtual bool customDecorated() const = 0;

	///Tires to adds the given window hints to the window.
	///\warning Window hints are only valid for toplevel windows.
	virtual void addWindowHints(WindowHints hints) = 0;

	///Tries to remove the given window hints from the window.
	///\warning Window  hints are only valid for toplevel windows.
	virtual void removeWindowHints(WindowHints hints) = 0;

	///If this window is a native dialog, a dialog context pointer is returned, nullptr otherwise.
	virtual DialogContext* dialogContext() const { return nullptr; }

protected:
	EventHandler* eventHandler_ {nullptr};
};

///Interface for native dialogs.
///Dialogs are temorary windows that can be used to collect data from the user.
///Such data can be e.g. simply a result (like ok or abort) or a filepath, color or font.
class DialogContext
{
public:
	///Returns the result of the dialog, or DialogResult::none if the dialog
	///is not yet finished.
	virtual DialogResult result() const = 0;

	///Returns only once the dialog has finished and returns the result.
	virtual DialogResult modal() = 0;

	///Returns whether the dialog has already finished.
	virtual bool finished() const = 0;

	///Return the data the dialog was dealing with. This usually be something like
	///Color, filepath or other variable type.
	///The stored type is determined by the DialogType value the Window context associated
	///with this DialogContext was created with.
	///If the dialog is not yet finished, an empty any object should be returned.
	///There can also exist dialog contexts that do not have any data (e.g. messsage boxes)
	///which should return an empty any object as well.
	virtual std::any data() const = 0;
};

}
