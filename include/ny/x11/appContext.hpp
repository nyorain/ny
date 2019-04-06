// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/x11/include.hpp>
#include <ny/appContext.hpp>
#include <ny/deferred.hpp>
#include <ny/windowSettings.hpp>

#include <map>
#include <memory>

namespace ny {

/// X11 AppContext implementation.
class X11AppContext : public AppContext {
public:
	DeferredOperator<void(), const WindowContext*> deferred;

public:
	X11AppContext();
	~X11AppContext();

	// - AppContext -
	KeyboardContext* keyboardContext() override;
	MouseContext* mouseContext() override;
	WindowContextPtr createWindowContext(const WindowSettings& settings) override;

	void pollEvents() override;
	void waitEvents() override;
	void wakeupWait() override;

	bool clipboard(std::unique_ptr<DataSource>&& dataSource) override;
	DataOffer* clipboard() override;
	bool dragDrop(const EventData* trigger,
		std::unique_ptr<DataSource>&& dataSource) override;

	std::vector<const char*> vulkanExtensions() const override;
	GlSetup* glSetup() const override;

	// - x11 specific -
	void processEvent(const x11::GenericEvent& ev, const x11::GenericEvent* next);
	X11WindowContext* windowContext(xcb_window_t);
	void checkError();

	Display& xDisplay() const { return *xDisplay_; }
	xcb_connection_t& xConnection() const { return *xConnection_; }
	x11::EwmhConnection& ewmhConnection() const;
	int xDefaultScreenNumber() const { return xDefaultScreenNumber_; }
	xcb_screen_t& xDefaultScreen() const { return *xDefaultScreen_; }
	xcb_window_t xDummyWindow() const { return xDummyWindow_; }
	xcb_timestamp_t time() const { return time_; }
	X11ErrorCategory& errorCategory() const;

	GlxSetup* glxSetup() const;
	X11DataManager& dataManager() const;

	void registerContext(xcb_window_t xWindow, X11WindowContext& context);
	void destroyed(const X11WindowContext& win);
	void bell(unsigned val = 100);

	xcb_atom_t atom(const std::string& name);
	const x11::Atoms& atoms() const;
	auto ewmhWindowCaps() const { return ewmhWindowCaps_; }

	bool xinput() const { return xiOpcode_; }
	void time(xcb_timestamp_t t) { time_ = t; }

protected:
	Display* xDisplay_  = nullptr;
	xcb_connection_t* xConnection_ = nullptr;
	xcb_window_t xDummyWindow_ = {};
	xcb_timestamp_t time_ {};

	int xDefaultScreenNumber_ = 0;
	xcb_screen_t* xDefaultScreen_ = nullptr;

	std::map<xcb_window_t, X11WindowContext*> contexts_;
	std::map<std::string, xcb_atom_t> additionalAtoms_;

	std::unique_ptr<X11MouseContext> mouseContext_;
	std::unique_ptr<X11KeyboardContext> keyboardContext_;
	x11::GenericEvent* next_ {};

	WindowCapabilities ewmhWindowCaps_ {};
	int xiOpcode_ {};

	struct Impl;
	std::unique_ptr<Impl> impl_;
};

} // namespace ny
