// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/fwd.hpp>
#include <ny/windowListener.hpp> // ny::WindowListener
#include <nytl/nonCopyable.hpp> // nytl::NonMovable
#include <functional> // std::reference_wrapper

namespace ny {

// Abstract interface for a window context in the underlaying window system.
// Implementations are not required to store any information (like current size
// or state), but rather communicate them to the application via event
// callbacks. The application can set the WindowListener that will be called
// when the WindowContext receives any window or input events and store only
// the needed information.
class WindowContext : public nytl::NonMovable {
public:
	WindowContext() = default;
	virtual ~WindowContext() { listener().destroyed(); }

	// Changes the registered WindowListener that will be signaled for events.
	// Note that there is always an associated WindowListener, the default is
	// WindowListener::defaultInstance (i.e. use this to reset a registered
	// listener). The passed listener must remain valid until the
	// WindowContext is destructed or it is replaced by another listener.
	virtual void listener(WindowListener& listener) { listener_ = listener; }

	// Returns the WindowListener this WindowContext has.
	// Note that there is always an associated WindowListener, the default is
	// WindowListener::defaultInstance.
	virtual WindowListener& listener() const { return listener_.get(); }

	// Returns the capabilities this WindowContext has.
	// This makes it possible for implementations to at least signal the
	// application when they are not able to implement some of the functions
	// correctly. Applications should not expect all functionality to
	// be available on all backends, think of special backends
	// like android or linux drm which are e.g. useful as pure rendering
	// backends but otherwise don't offer much functionality for windows.
	virtual WindowCapabilities capabilities() const = 0;

	// Shows or hides the window.
	// If the WindowCapability::visibility flag of the capabilities() return
	// value is not set, this function is not supported and may not change the
	// current visibility status.
	// TODO: make those two just one function `void show(bool)`?
	virtual void show() = 0;
	virtual void hide() = 0;

	// Sets the minimal size of the window.
	// If the WindowCapability::sizeLimits flag of the capabilities() return
	// value is not set, this function is not able to set the minimum window
	// size.
	virtual void minSize(nytl::Vec2ui minSize) = 0;

	// Sets the maximal size of the window.
	// If the WindowCapability::sizeLimits flag of the capabilities() return
	// value is not set, this function is not able to set the maximum window
	// size.
	virtual void maxSize(nytl::Vec2ui maxSize) = 0;

	// Resizes the window to the given size.
	// Might trigger a (often delayed) DrawEvent.
	// If the WindowCapability::size flag of the capabilites() return value is
	// not set, this function might not be able to change the size of the
	// window.
	virtual void size(nytl::Vec2ui size) = 0;

	// Sets the position of the window.
	// This function might have no effect if the window is in maximized or
	// fullscreen mode.  If the WindowCapability::position flag of the
	// capabilites() return value is not set, this function might not be able
	// to change the size of the window.  Applications should not rely on this
	// to work.
	virtual void position(nytl::Vec2i position) = 0;

	// Sets the mouse cursor of the window.
	// The mouse cursor will then always have the given cursor when over this
	// window.  If the WindowCapability::cursor flag of the capabilites()
	// return value is not set, this function might not be able to set the
	// cursor.
	virtual void cursor(const Cursor& c) = 0;

	// Returns the underlaying native window handle.  This handle can then be
	// passed on to other window toolkits or used in some backend-specific way.
	// It can also be used to create a child WindowContext for this
	// WindowContext.  Backends may return an empty handle.
	virtual NativeHandle nativeHandle() const = 0;

	// Asks the platform-specific windowing api for a window refresh.  Will
	// basically send a DrawEvent to the registered EventHandler as soon as the
	// window is ready to draw. Will never send an event from within the
	// function so it can safely be used from within the DrawEvent handler to
	// "schedule" the next redraw as soon as it makes sense.
	virtual void refresh() = 0;

	// Notifies ny that a presentation request was done on this window.
	// Should be used when drawing onto the window in a way that ny can't know,
	// e.g. via vulkan or platform-specific means. Only important if the
	// application wishes to synchronize refresh draws with the window system.
	// Should not be called if the window is drawn via a GlSurface or
	// a BufferSurface.
	virtual void frameCallback() {}; // TODO: make pure virtual

	// Returns a Surface object that holds some type of surface object that was
	// created for the WindowContext. If the WindowContext was created without
	// any surface, an empty Surface (with Surface::type == SurfaceType::none)
	// is returned.
	virtual Surface surface() = 0;

	// Maximized the window. If the window was previously in fullscreen or
	// minimized, will first try to reset it to normal state. If the
	// WindowCapability::maximize flag of the capabilites() return value is not
	// set, this function will have no effect on the window.
	virtual void maximize() = 0;

	// Minimized the window. Will otherwise not change the window state, e.g. a
	// window in fullscreen state will be restored to fullscreen when
	// unminimized. If the WindowCapability::minimize flag of the
	// capabilites() return value is not set, this function will have no effect
	// on the window.
	virtual void minimize() = 0;

	// Sets the window in a fullscreen state. Will automatically unminimize it if possible.
	// If the WindowCapability::fullscreen flag of the capabilites() return value is not set,
	// this function will have no effect on the window.
	virtual void fullscreen() = 0;

	// Resets the window in normal toplevel state, i.e. resets fullscreen or minimized state if
	// possible.
	// If it is currently maximized, minimized or in fullscreen, these states will be removed.
	virtual void normalState() = 0;

	// Asks the window manager to start an interactive move for the window.
	// trigger: The EventData of the event to which this action is the response
	// Should be a mouse event.
	virtual void beginMove(const EventData* trigger) = 0;

	// Asks the window manager to start an interactive resizing for the window.
	// trigger: The EventData of the event to which this action is the response.
	// Should be a mouse event.
	virtual void beginResize(const EventData* trigger, WindowEdges edges) = 0;

	// Sets the title for the native window. The title is what is displayed for the
	// window in a potential titlebar or taskbar.
	virtual void title(const char* name) = 0;

	// Sets the icon of the native window. Used e.g. in a titlebar or taskbar.
	// If the given icon pointer variable is an empty image, the icon will be reset/unset.
	virtual void icon(const Image& newicon) = 0;

	// Tries to set/unset the custom decoration of this window.
	// Changing this value might not have success, capabilities() of this
	// WindowContext and the customDecorated() get function can be used to
	// determine whether the window should be custom decorated.
	// The best way is usually to provide a user setting for custom decoration
	// and only follow this setting.
	virtual void customDecorated(bool set) = 0;

	// Returns whether the window should be custom decorated.
	// This function cannot always predict whether the user expects window
	// decorations, it will try to use a sane default for the backend. Can be
	// tried to change using the customDecorated(bool set) function.
	// The best way is usually to provide a user setting for custom decoration
	// and only follow this setting.
	virtual bool customDecorated() const = 0;

protected:
	std::reference_wrapper<WindowListener> listener_
		{WindowListener::defaultInstance()};
};

} // namespace ny
