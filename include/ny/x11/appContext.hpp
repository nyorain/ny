// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once
#include <ny/x11/include.hpp>
#include <ny/appContext.hpp>

#include <map>
#include <memory>

namespace ny {

///X11 AppContext implementation.
class X11AppContext : public AppContext {
public:
    X11AppContext();
    ~X11AppContext();

	// - AppContext -
	KeyboardContext* keyboardContext() override;
	MouseContext* mouseContext() override;
	WindowContextPtr createWindowContext(const WindowSettings& settings) override;

	bool dispatchEvents() override;
	bool dispatchLoop(LoopControl& control) override;

	bool clipboard(std::unique_ptr<DataSource>&& dataSource) override;
	DataOffer* clipboard() override;
	bool startDragDrop(std::unique_ptr<DataSource>&& dataSource) override;

	std::vector<const char*> vulkanExtensions() const override;
	GlSetup* glSetup() const override;

	// - x11 specific -
    void processEvent(const x11::GenericEvent& xEvent);
    X11WindowContext* windowContext(xcb_window_t);
	bool checkErrorWarn();

    Display& xDisplay() const { return *xDisplay_; }
	xcb_connection_t& xConnection() const { return *xConnection_; }
	x11::EwmhConnection& ewmhConnection() const;
    int xDefaultScreenNumber() const { return xDefaultScreenNumber_; }
    xcb_screen_t& xDefaultScreen() const { return *xDefaultScreen_; }
	xcb_window_t xDummyWindow() const { return xDummyWindow_; }
	const X11ErrorCategory& errorCategory() const;

	GlxSetup* glxSetup() const;
	X11DataManager& dataManager() const;

    void registerContext(xcb_window_t xWindow, X11WindowContext& context);
    void unregisterContext(xcb_window_t xWindow);
	void bell();

	xcb_atom_t atom(const std::string& name);
	const x11::Atoms& atoms() const;

protected:
    Display* xDisplay_  = nullptr;
	xcb_connection_t* xConnection_ = nullptr;
	xcb_window_t xDummyWindow_ = {};

    int xDefaultScreenNumber_ = 0;
    xcb_screen_t* xDefaultScreen_ = nullptr;

    std::map<xcb_window_t, X11WindowContext*> contexts_;
	std::map<std::string, xcb_atom_t> additionalAtoms_;

	std::unique_ptr<X11MouseContext> mouseContext_;
	std::unique_ptr<X11KeyboardContext> keyboardContext_;

	struct Impl;
	std::unique_ptr<Impl> impl_;
};

} // namespace ny
