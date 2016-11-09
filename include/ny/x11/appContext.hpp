#pragma once

#include <ny/x11/include.hpp>
#include <ny/x11/util.hpp>

#include <ny/appContext.hpp>

//TODO: remove this. Needs some typedef magic + void* args...
//problem: xcb uses anonymous typedef structs which cannot be forward declared.
//but the header is really huge and pulls in tons of global x symbols/macros
#include <xcb/xcb.h>

#include <map>
#include <memory>

namespace ny
{

class GlxSetup;

///X11 AppContext implementation.
class X11AppContext : public AppContext
{
public:
    X11AppContext();
    ~X11AppContext();

	//AppContext
	KeyboardContext* keyboardContext() override;
	MouseContext* mouseContext() override;
	WindowContextPtr createWindowContext(const WindowSettings& settings) override;

	bool dispatchEvents() override;
	bool dispatchLoop(LoopControl& control) override;
	bool threadedDispatchLoop(EventDispatcher& dispatcher, LoopControl& control) override;

	//TODO
	bool clipboard(std::unique_ptr<DataSource>&& dataSource) override;
	DataOffer* clipboard() override;
	bool startDragDrop(std::unique_ptr<DataSource>&& dataSource) override;

	std::vector<const char*> vulkanExtensions() const override;
	GlSetup* glSetup() const override;

	//custom
    Display* xDisplay() const { return xDisplay_; }
	xcb_connection_t* xConnection() const { return xConnection_; }
	x11::EwmhConnection* ewmhConnection() const { return ewmhConnection_.get(); }
    int xDefaultScreenNumber() const { return xDefaultScreenNumber_; }
    xcb_screen_t* xDefaultScreen() const { return xDefaultScreen_; }
	const X11ErrorCategory& errorCategory() const { return errorCategory_; }

	GlxSetup* glxSetup() const;

    void registerContext(xcb_window_t w, X11WindowContext& c);
    void unregisterContext(xcb_window_t w);
    X11WindowContext* windowContext(xcb_window_t win);
	EventHandler* eventHandler(xcb_window_t w);
	void bell();

	xcb_atom_t atom(const std::string& name);
	const x11::Atoms& atoms() const { return atoms_; }

protected:
    Display* xDisplay_  = nullptr;
	xcb_connection_t* xConnection_ = nullptr;
	std::unique_ptr<x11::EwmhConnection> ewmhConnection_;

	xcb_window_t xDummyWindow_ = {};

    int xDefaultScreenNumber_ = 0;
    xcb_screen_t* xDefaultScreen_ = nullptr;

    std::map<xcb_window_t, X11WindowContext*> contexts_;
	x11::Atoms atoms_;
	X11ErrorCategory errorCategory_;

	std::unique_ptr<X11MouseContext> mouseContext_;
	std::unique_ptr<X11KeyboardContext> keyboardContext_;

	std::unique_ptr<DataSource> clipboardSource_;
	// std::unique_ptr<X11DataManager> dataManager_;

	//used for built-config dependent members like e.g. GlxContexts
	//can also be used to add members while keeping the changes abi-compatible
	struct Impl;
	std::unique_ptr<Impl> impl_;

protected:
    bool processEvent(xcb_generic_event_t& ev, EventDispatcher* dispatcher = nullptr);
};

}