// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/fwd.hpp>
#include <ny/eventHandler.hpp>
#include <ny/nativeHandle.hpp>

#include <nytl/flags.hpp>

#include <memory>
#include <any>

//This header and its functionality can be used without linking to ny.

namespace ny
{

///\brief Abstract interface for a window context in the underlaying window system.
///The term "window" used in the documentation for this class is used for the underlaying native
///window, ny::WindowContext is totally independent from ny::Window and can be used without it.
///Note that all WindowContext functions should just be called from just one thread, normally the
///thread which processes events.
///
///The WindowContext will send the registered Eventhandler (if any) a DrawEvent when it should
///redraw the window. Alternatively the client may wish to redraw the window because of some
///content changes. Then it can call the refresh function.
///Redrawing the window at a random time without receiving a DrawEvent might lead to artefacts
///such as flickering.
class WindowContext
{
public:
	WindowContext() = default;
	virtual ~WindowContext() = default;

	///Sets the EventHandler that should receive the events associated with this windowContext.
	///The event handler will receive information about window state changes and input.
	virtual void eventHandler(EventHandler& handler) { eventHandler_ = &handler; }

	///Returns the associated EventHandler of this windowContext, nullptr if there is none.
	virtual EventHandler* eventHandler() const { return eventHandler_; }

	///Makes the window visible.
	virtual void show() = 0;

	///Hides the window.
	virtual void hide() = 0;

	//TODO: allow to specify regions (or a predicate) in which different types may
	//be dropped in some way.

	///Signals that the window accepts drag-and-drops of the given DataTypes.
	///If this function is called with an emtpy DataTypes object drag-and-drop support
	///should be removed from the window.
	///When the WindowContext receives a drop request for a matching type, it will send
	///a DataOfferEvent to the registered EventHandler.
	virtual void droppable(const DataTypes&) = 0;

	//TODO? XXX: make these functions bool to signal if they had any effect?
	//Should they output a warning if not? it can be queried using capabilites

	///Sets the minimal size of the window.
	///Might have no effect on certain backends.
	virtual void minSize(const nytl::Vec2ui&) = 0;

	///Sets the maximal size of the window.
	///Might have no effect on certain backends.
	virtual void maxSize(const nytl::Vec2ui&) = 0;

	///Resizes the window.
	///Will usually trigger a DrawEvent.
	virtual void size(const nytl::Vec2ui& size) = 0;

	///Sets the position of the window.
	///Note that on some backends or if the window is in a fullscreen/maximized state, this
	///might have no effect.
	virtual void position(const nytl::Vec2i& position) = 0;

	///Sets the mouse cursor of the window. The mouse cursor will only be changed to the given
	///cursor when the pointer is oven this window.
	virtual void cursor(const Cursor& c) = 0;

	///Returns the underlaying native window handle.
	///This handle can then be passed on to other window toolkits or used in some backend-specific
	///way.
	virtual NativeHandle nativeHandle() const = 0;

	///Asks the platform-specific windowing api for a window refresh.
	///Will basically send a DrawEvent to the registered EventHandler as soon as the window is
	///ready to draw. This function might directly dispatch a DrawEvent to the registered
	///EventHandler which might lead to a redraw before this function returns depending on
	///the used event dispatching system.
	virtual void refresh() = 0;

	///Returns a Surface object that holds some type of surface object that was created
	///for the WindowContext.
	///If the WindowContext was created without any surface, an empty Surface (with
	///Surface::type == SurfaceType::none) is returned.
	virtual Surface surface() = 0;



	// - toplevel-specific -
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
	///If the given icon pointer variable is an empty image, the icon will be reset/unset.
	///\warning Shall have only an effect for toplevel windows.
	virtual void icon(const ImageData& newicon) = 0; //may be only important for client decoration

	///Returns whether the window should be custom decorated.
	///Custom decoration can either be manually triggered by setting the custom decorated
	///window hint or the backend may tell the client to decrate itself (i.e. wayland).
	///\warning Will only return a valid value for toplevel windows.
	virtual bool customDecorated() const = 0;

	///Tires to adds the given window hints to the window.
	///\warning Window hints are only valid for toplevel windows.
	///\sa WindowHints
	virtual void addWindowHints(WindowHints hints) = 0;

	///Tries to remove the given window hints from the window.
	///\warning Window  hints are only valid for toplevel windows.
	///\sa WindowHints
	virtual void removeWindowHints(WindowHints hints) = 0;

	///Returns the capabilities this WindowContext has.
	///This makes it possible for implementations like e.g. linux drm to
	///at least signal the application somehow that it is barely able to implement any of
	///the default window functions.
	virtual WindowCapabilities capabilities() const = 0;

protected:
	EventHandler* eventHandler_ {nullptr};
};

}
