// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/x11/include.hpp>
#include <ny/windowContext.hpp>
#include <ny/windowSettings.hpp>

#include <vector>

namespace ny {

/// Additional settings for a X11 Window.
class X11WindowSettings : public WindowSettings {
public:
	bool overrideRedirect {false};
	uint32_t windowType {};
};

/// The X11 implementation of the WindowContext interface.
/// Provides some extra functionality for x11.
/// Tries to use xcb where possible, for some things (e.g. glx context) xlib is needed though.
class X11WindowContext : public WindowContext {
public:
	X11WindowContext(X11AppContext& ctx, const X11WindowSettings& settings = {});
	~X11WindowContext();

	// - WindowContext implementation -
	void refresh() override;
	void frameCallback() override;
	void show() override;
	void hide() override;

	void minSize(nytl::Vec2ui size) override;
	void maxSize(nytl::Vec2ui size) override;

	void size(nytl::Vec2ui size) override;
	void position(nytl::Vec2i position) override;

	void cursor(const Cursor&) override;

	NativeHandle nativeHandle() const override;
	WindowCapabilities capabilities() const override;
	Surface surface() override;

	// toplevel window
	void maximize() override;
	void minimize() override;
	void fullscreen() override;
	void normalState() override;

	void beginMove(const EventData*) override;
	void beginResize(const EventData* ev, WindowEdges edges) override;

	void title(const char*) override;
	void icon(const Image& img) override;

	void customDecorated(bool set) override;
	bool customDecorated() const override;

	// - x11-specific -
	// specific event handlers
	virtual void reparentEvent();
	virtual void resizeEvent(nytl::Vec2ui size, const X11EventData&);
	virtual void presentCompleteEvent(uint32_t serial);

	X11AppContext& appContext() const { return *appContext_; } /// The associated AppContext
	uint32_t xWindow() const { return xWindow_; } /// The underlaying x window handle

	xcb_connection_t& xConnection() const; /// The associated x conntextion
	x11::EwmhConnection& ewmhConnection() const; /// The associated ewmh connection (helper)
	const X11ErrorCategory& errorCategory() const; /// Shortcut for the AppContexts ErrorCategory

	nytl::Vec2ui size() const { return size_; } /// Synchronous size
	nytl::Vec2ui querySize() const; /// Queries the current window size
	void overrideRedirect(bool redirect); /// Sets the overrideRedirect flag for the window
	void transientFor(uint32_t win); /// Makes the window transient for another x window

	void raise(); /// tries to raise the window
	void lower(); /// tries to lower the window
	void requestFocus(); /// tries to bring the window focus

	/// Adds the given state/states to the window.
	/// Look at the ewmh specification for more information and allowed atoms.
	/// The changed property is _NET_WM_STATE.
	void addStates(uint32_t state1, uint32_t state2 = 0);

	/// Adds the given state/states to the window.
	/// Look at the ewmh specification for more information and allowed atoms.
	/// The changed property is _NET_WM_STATE.
	void removeStates(uint32_t state1, uint32_t state2 = 0);

	/// Adds the given state/states to the window.
	/// Look at the ewmh specification for more information and allowed atoms.
	/// The changed property is _NET_WM_STATE.
	void toggleStates(uint32_t state1, uint32_t state2 = 0);

	/// Sets the window type.
	/// For more information look in the ewmh specification for _NET_WM_WINDOW_TYPE.
	void xWindowType(uint32_t type);

	// TODO: implement!
	/// Returns the window type assocated with this window.
	/// For more information look in the ewmh specification for _NET_WM_WINDOW_TYPE.
	// uint32_t xWindowType();

	/// Adds an allowed action for the window.
	/// For more information look in the ewmh specification for _NET_WM_ALLOWED_ACTIONS.
	void addAllowedAction(uint32_t action);

	/// Removes an allowed action for the window.
	/// For more information look in the ewmh specification for _NET_WM_ALLOWED_ACTIONS.
	void removeAllowedAction(uint32_t action);

	/// Returns all allowed actions for the window.
	/// For more information look in the ewmh specification for _NET_WM_ALLOWED_ACTIONS.
	std::vector<uint32_t> allowedActions() const;

	/// Returns the x visual id for this window.
	/// If it has not been set, 0 is retrned.
	unsigned int xVisualID() const { return visualID_; }

	/// Finds and returns the xcb_visualtype_t for this window.
	/// If the visual id has not been set or does not have a matching visualtype, nullptr
	/// is returned.
	xcb_visualtype_t* xVisualType() const;

	/// Returns the depth of the visual (i.e. the bits per pixel)
	/// Notice that this might somehow differ from the bits_per_rgb member of the
	/// visual type (since it also counts the alpha bits).
	unsigned int visualDepth() const { return depth_; }

	void scheduleRedraw();
	uint32_t presentSerial();

protected:
	/// Default Constructor only for derived classes that later call the create function.
	X11WindowContext() = default;

	/// Inits the x11 window from the given settings.
	/// Will initialize the visual if it was not already set.
	/// May be needed by derived classes.
	void create(X11AppContext& ctx, const X11WindowSettings& settings);

	/// Creates the window for the givne WindowSettings.
	/// Will initialize the visual if it was not already set.
	/// Called only during initialization.
	void createWindow(const X11WindowSettings& settings);

	/// The different context classes derived from this class may override this function to
	/// select a custom visual for the window or query it in a different way connected with
	/// more information.
	/// By default, this just selects the 32 or 24 bit visual with the most usual format.
	void initVisual(const X11WindowSettings& settings);

	/// Reads the windows states and sends an event if they changed.
	void reloadStates();

protected:
	X11AppContext* appContext_ {};
	X11WindowSettings settings_ {};
	uint32_t xWindow_ {};
	uint32_t xCursor_ {};
	uint32_t xColormap_ {};

	// we store the visual id instead of the visualtype since e.g. glx does not
	// care/deal with the visualtype in any way.
	// if the xcb_visualtype_t* is needed, just call xVisualType() which will try
	// to query it.
	unsigned int visualID_ {};
	unsigned int depth_ {};

	// Stored EWMH states can be used to check whether it is fullscreen, maximized etc.
	ToplevelState state_ {};
	bool customDecorated_ {};
	nytl::Vec2ui size_ {}; // the latest size

	uint32_t presentID_ {}; // for present extension
	uint32_t presentPending_ {}; // serial of pending present call
	uint32_t presentSerial_ {}; // counter
	bool presentRefresh_ {}; // refresh on present complete

	// flags
	bool resizeEventFlag_ {};
	bool drawEventFlag_ {};

	// TODO
	xcb_pixmap_t xDummyPixmap_ {};
};

} // namespace ny
