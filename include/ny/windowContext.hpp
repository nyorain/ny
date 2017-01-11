// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/fwd.hpp>
#include <ny/windowListener.hpp> // ny::WindowListener
#include <functional> // std::reference_wrapper

namespace ny {

/// Abstract interface for a window context in the underlaying window system.
/// Implementations are not required to store any information (like current size or state),
/// but rather communicate them to the application via event callbacks.
/// The application can set the WindowListener that will be called when the WindowContext
/// receives any window or input events and store only the needed information.
class WindowContext {
public:
	WindowContext() = default;
	virtual ~WindowContext() { listener().destroyed(); }

	/// Changes the registered WindowListener that will be called when events occurr.
	/// Note that there is always an associated WindowListener, the default is
	/// WindowListener::defaultInstance (i.e. use this to reset a registered listener).
	/// The passed listener must remain valid until the WindowContext is destructed or it is
	/// replaced by another listener.
	virtual void listener(WindowListener& listener) { listener_ = listener; }

	/// Returns the WindowListener this WindowContext has.
	/// Note that there is always an associated WindowListener, the default is
	/// WindowListener::defaultInstance.
	virtual WindowListener& listener() const { return listener_.get(); }

	/// Returns the capabilities this WindowContext has.
	/// This makes it possible for implementations like e.g. linux drm to
	/// at least signal the application somehow that it is barely able to implement any of
	/// the default window functions.
	virtual WindowCapabilities capabilities() const = 0;

	/// Makes the window visible.
	virtual void show() = 0;

	/// Hides the window.
	virtual void hide() = 0;

	/// Sets the minimal size of the window.
	/// Might have no effect on certain backends.
	virtual void minSize(nytl::Vec2ui minSize) = 0;

	/// Sets the maximal size of the window.
	/// Might have no effect on certain backends.
	virtual void maxSize(nytl::Vec2ui maxSize) = 0;

	/// Resizes the window.
	/// Will usually trigger a DrawEvent.
	virtual void size(nytl::Vec2ui size) = 0;

	/// Sets the position of the window.
	/// Note that on some backends or if the window is in a fullscreen/maximized state, this
	/// might have no effect.
	virtual void position(nytl::Vec2i position) = 0;

	/// Sets the mouse cursor of the window. The mouse cursor will only be changed to the given
	/// cursor when the pointer is oven this window.
	virtual void cursor(const Cursor& c) = 0;

	/// Returns the underlaying native window handle.
	/// This handle can then be passed on to other window toolkits or used in some backend-specific
	/// way. It can also be used to create a child WindowContext for this one.
	virtual NativeHandle nativeHandle() const = 0;

	/// Asks the platform-specific windowing api for a window refresh.
	/// Will basically send a DrawEvent to the registered EventHandler as soon as the window is
	/// ready to draw. This function might directly dispatch a DrawEvent to the registered
	/// EventHandler which might lead to a redraw before this function returns depending on
	/// the used event dispatching system.
	virtual void refresh() = 0;

	/// Returns a Surface object that holds some type of surface object that was created
	/// for the WindowContext.
	/// If the WindowContext was created without any surface, an empty Surface (with
	/// Surface::type == SurfaceType::none) is returned.
	virtual Surface surface() = 0;


	// - toplevel-specific -

	/// Maximized the window.
	/// \warning Shall have only an effect for toplevel windows.
	virtual void maximize() = 0;

	/// Minimized the window.
	/// \warning Shall have only an effect for toplevel windows.
	virtual void minimize() = 0;

	/// Sets the window in a fullscreen state.
	/// \warning Shall have only an effect for toplevel windows.
	virtual void fullscreen() = 0;

	/// Resets the window in normal toplevel state.
	/// If it is currently maximized, minimized or in fullscreen, these states will be removed.
	/// \warning Shall have only an effect for toplevel windows.
	virtual void normalState() = 0; //or reset()?

	/// Asks the window manager to start an interactive move for the window.
	/// \param event A pointer to an EventData object [optional]. Might fail if nullptr is given.
	/// \warning Shall have only an effect for toplevel windows.
	virtual void beginMove(const EventData* event) = 0;

	/// Asks the window manager to start an interactive resizing for the window.
	/// \param event A pointer to an EventData object [optional]. Might fail if nullptr is given.
	/// \warning Shall have only an effect for toplevel windows.
	virtual void beginResize(const EventData* event, WindowEdges edges) = 0;

	/// Sets the title for the native window. The title is what is displayed for the
	/// window in a potential titlebar or taskbar.
	/// \warning Shall have only an effect for toplevel windows.
	virtual void title(nytl::StringParam name) = 0;

	/// Sets the icon of the native window. Used e.g. in a titlebar or taskbar.
	/// If the given icon pointer variable is an empty image, the icon will be reset/unset.
	/// \warning Shall have only an effect for toplevel windows.
	virtual void icon(const Image& newicon) = 0;

	/// Returns whether the window should be custom decorated.
	/// Custom decoration can either be manually triggered by setting the custom decorated
	/// window hint or the backend may tell the client to decrate itself (i.e. wayland).
	/// \warning Will only return a valid value for toplevel windows.
	virtual bool customDecorated() const = 0;

	/// Tires to adds the given window hints to the window.
	/// \warning Window hints are only valid for toplevel windows.
	/// \sa WindowHints
	virtual void addWindowHints(WindowHints hints) = 0;

	/// Tries to remove the given window hints from the window.
	/// \warning Window  hints are only valid for toplevel windows.
	/// \sa WindowHints
	virtual void removeWindowHints(WindowHints hints) = 0;

protected:
	std::reference_wrapper<WindowListener> listener_ {WindowListener::defaultInstance()};
};

} // namespace ny
