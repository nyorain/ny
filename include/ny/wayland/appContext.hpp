// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/wayland/include.hpp>
#include <ny/appContext.hpp>

#include <vector>
#include <string>
#include <memory>
#include <system_error>
#include <map>

namespace ny
{

/// Wayland AppContext implementation.
/// Holds the wayland display connection as well as all global resources.
class WaylandAppContext : public AppContext {
public:
	WaylandAppContext();
	virtual ~WaylandAppContext();

	// - AppContext implementation -
	bool dispatchEvents() override;
	bool dispatchLoop(LoopControl& control) override;

	MouseContext* mouseContext() override;
	KeyboardContext* keyboardContext() override;
	WindowContextPtr createWindowContext(const WindowSettings& windowSettings) override;

	bool clipboard(std::unique_ptr<DataSource>&& dataSource) override;
	DataOffer* clipboard() override;
	bool startDragDrop(std::unique_ptr<DataSource>&& dataSource) override;

	std::vector<const char*> vulkanExtensions() const override;
	GlSetup* glSetup() const override;

	// - wayland specific -

	/// Checks the wayland display for errors.
	/// If the wayland display has an error (i.e. it cannot be used any longer) returns an
	/// error code holding either the posix error code returned, or an interface-specific
	/// error code.
	std::error_code checkError() const;
	bool checkErrorWarn() const; /// Outputs warning and returns false on error

	/// Can be called to register custom listeners for fds that the dispatch loop will
	/// then poll for.
	using FdCallbackFunc = std::function<void(int fd, unsigned int events)>;
	using FdCallbackFuncConn = std::function<void(nytl::Connection, int fd, unsigned int events)>;

	nytl::Connection fdCallback(int fd, unsigned int events, const FdCallbackFunc& func);
	nytl::Connection fdCallback(int fd, unsigned int events, const FdCallbackFuncConn& func);

	/// Roundtrips, i.e. waits until all requests are sent and the server has processed
	/// all of them.
	/// This function should be used instead of wl_display_roundtrip since it takes care of
	/// not dispatching any other events by using an extra event queue.
	void roundtrip();

	WaylandKeyboardContext* waylandKeyboardContext() const { return keyboardContext_.get(); }
	WaylandMouseContext* waylandMouseContext() const { return mouseContext_.get(); }
	WaylandDataDevice* waylandDataDevice() const { return dataDevice_.get(); }

	wl_display& wlDisplay() const;
	wl_registry& wlRegistry() const;
	wl_compositor& wlCompositor() const;

	wl_subcompositor* wlSubcompositor() const;
	wl_shm* wlShm() const;
	wl_seat* wlSeat() const;
	wl_shell* wlShell() const;
	xdg_shell* xdgShellV5() const;
	zxdg_shell_v6* xdgShellV6() const;
	wl_data_device_manager* wlDataManager() const;

	wl_cursor_theme* wlCursorTheme() const;
	wl_pointer* wlPointer() const;
	wl_keyboard* wlKeyboard() const;

	WaylandWindowContext* windowContext(wl_surface& surface) const;
	const std::vector<wayland::Output>& outputs() const { return outputs_; }
	bool shmFormatSupported(unsigned int wlShmFormat);

	EglSetup* eglSetup() const;

protected:
	/// Modified version of wl_dispatch_display that performs the same operations but
	/// does also poll for the registered fds.
	/// Returns false on error.
	bool dispatchDisplay();

	/// Polls for all registered fd callbacks as well as for the wayland display fd
	/// with the given events if they are not 0. Uses the given timeout for poll calls.
	/// Returns the value poll returned.
	/// Will not stop on a signal.
	int pollFds(short wlDisplayEvents, int timeout);

	// callback handlers
	void handleRegistryAdd(wl_registry*, uint32_t id, const char* cinterface, uint32_t version);
	void handleRegistryRemove(wl_registry*, uint32_t id);
	void handleSeatCapabilities(wl_seat*, uint32_t caps);
	void handleSeatName(wl_seat*, const char* name);
	void handleShmFormat(wl_shm*, uint32_t format);

	void handleXdgShellV5Ping(xdg_shell*, uint32_t serial);
	void handleXdgShellV6Ping(zxdg_shell_v6*, uint32_t serial);

protected:
	wl_display* wlDisplay_;
	wl_registry* wlRegistry_;
	wl_event_queue* wlRoundtripQueue_;
	wl_cursor_theme* wlCursorTheme_ {};

	unsigned int eventfd_ = 0u;
	std::vector<unsigned int> shmFormats_;
	std::vector<wayland::Output> outputs_;
	std::string seatName_;

	std::unique_ptr<WaylandDataDevice> dataDevice_;
	std::unique_ptr<WaylandKeyboardContext> keyboardContext_;
	std::unique_ptr<WaylandMouseContext> mouseContext_;

	bool wakeup_ {false}; // Set to true from the eventfd callback, causes dispatchDisplay to return

	struct Impl;
	std::unique_ptr<Impl> impl_;
};

} // namespace ny
