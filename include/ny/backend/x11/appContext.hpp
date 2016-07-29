#pragma once

#include <ny/backend/x11/include.hpp>
#include <ny/backend/appContext.hpp>

//TODO: remove this. Needs some typedef magic + void* args...
//problem: xcb uses anonymous typedef structs which cannot be forward declared.
#include <xcb/xcb.h> 

#include <map>
#include <memory>

namespace ny
{

struct DummyEwmhConnection;

class X11AppContext : public AppContext
{
public:
    X11AppContext();
    virtual ~X11AppContext();

	virtual bool dispatchEvents(EventDispatcher& dispatcher) override;
	virtual bool dispatchLoop(EventDispatcher& dispatcher, LoopControl& control) override;

    Display* xDisplay() const { return xDisplay_; }
	xcb_connection_t* xConnection() const { return xConnection_; }
	DummyEwmhConnection* ewmhConnection() const { return ewmhConnection_.get(); }
    int xDefaultScreenNumber() const { return xDefaultScreenNumber_; }
    xcb_screen_t* xDefaultScreen() const { return xDefaultScreen_; }

    void registerContext(xcb_window_t w, X11WindowContext& c);
    void unregisterContext(xcb_window_t w);
    X11WindowContext* windowContext(xcb_window_t win);

	xcb_atom_t atom(const std::string& name);

protected:
	class LoopControlImpl;

protected:
    Display* xDisplay_  = nullptr;
	xcb_connection_t* xConnection_ = nullptr;
	std::unique_ptr<DummyEwmhConnection> ewmhConnection_;

	xcb_window_t xDummyWindow_ = {};

    int xDefaultScreenNumber_ = 0;
    xcb_screen_t* xDefaultScreen_ = nullptr;

    std::map<xcb_window_t, X11WindowContext*> contexts_;
	std::map<std::string, xcb_atom_t> atoms_;

protected:
    bool processEvent(xcb_generic_event_t& ev, EventDispatcher& dispatcher);
	EventHandler* eventHandler(xcb_window_t w);

};

}
