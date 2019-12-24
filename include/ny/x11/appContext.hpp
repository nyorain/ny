// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/x11/include.hpp>
#include <ny/appContext.hpp>
#include <ny/windowSettings.hpp>
#include <ny/common/unix.hpp>

#include <unordered_map>
#include <memory>
#include <deque>

namespace ny {

/// X11 AppContext implementation.
class X11AppContext : public AppContext {
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
	/// Can be called to register custom listeners for fds that the dispatch loop will
	/// then poll for. Should return false if it wants to be disconnected.
	using FdCallbackFunc = std::function<bool(int fd, unsigned int events)>;
	nytl::Connection fdCallback(int fd, unsigned int events, FdCallbackFunc func);

	void processEvent(const void* ev, const void* next);
	X11WindowContext* windowContext(xcb_window_t);
	void checkError();

	Display& xDisplay() const { return *xDisplay_; }
	xcb_connection_t& xConnection() const { return *xConnection_; }
	x11::EwmhConnection& ewmhConnection() const;
	int xDefaultScreenNumber() const { return xDefaultScreenNumber_; }
	xcb_screen_t& xDefaultScreen() const { return *xDefaultScreen_; }
	xcb_window_t xDummyWindow() const { return xDummyWindow_; }
	xcb_window_t xDummyPixmap() const { return xDummyPixmap_; }
	uint32_t xEmptyRegion() const { return xEmptyRegion_; }
	xcb_timestamp_t time() const { return time_; }
	X11ErrorCategory& errorCategory() const;

	GlxSetup* glxSetup() const;
	X11DataManager& dataManager() const;

	using DeferFunc = void(*)(X11WindowContext&);
	void defer(X11WindowContext&, DeferFunc);
	void registerContext(xcb_window_t xWindow, X11WindowContext& context);
	void destroyed(const X11WindowContext& win);
	void bell(unsigned val = 100);

	xcb_atom_t atom(const std::string& name);
	const x11::Atoms& atoms() const;
	auto ewmhWindowCaps() const { return ewmhWindowCaps_; }
	void time(xcb_timestamp_t t) { time_ = t; }

	bool xinputExt() const { return (xinputOpcode_); }
	bool presentExt() const { return (presentOpcode_); }
	bool shmExt() const { return shmExt_; }

protected:
	/// Polls all internal fds. If wait is true, will block until event arrives.
	/// Will process all xcb events.
	bool poll(bool wait);
	bool dispatchPending();
	unsigned execDeferred();

protected:
	Display* xDisplay_  = nullptr;
	xcb_connection_t* xConnection_ = nullptr;
	xcb_window_t xDummyWindow_ = {};
	xcb_pixmap_t xDummyPixmap_ = {};
	uint32_t xEmptyRegion_ = {};

	/// The last timestamp received from the server.
	xcb_timestamp_t time_ {};

	int xDefaultScreenNumber_ = 0;
	xcb_screen_t* xDefaultScreen_ = nullptr;
	EventFD eventfd_; // for wakeup

	std::unordered_map<xcb_window_t, X11WindowContext*> contexts_;
	std::unordered_map<std::string, xcb_atom_t> additionalAtoms_;

	std::unique_ptr<X11MouseContext> mouseContext_;
	std::unique_ptr<X11KeyboardContext> keyboardContext_;

	/// Optionally contains the next event ot process.
	/// Peeked by dispatchPending since the next event is
	/// sometimes needed (e.g. checking for repeated key press)
	void* next_ {};

	WindowCapabilities ewmhWindowCaps_ {};

	// extensions
	int xinputOpcode_ {};
	int presentOpcode_ {};
	bool shmExt_ {};

	struct Impl;
	std::unique_ptr<Impl> impl_;

	struct Defer {
		DeferFunc func;
		X11WindowContext* wc;
	};

	std::deque<Defer> defer_;
};

} // namespace ny
