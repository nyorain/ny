#pragma once

#include <ny/backend/x11/include.hpp>
#include <ny/backend/appContext.hpp>

#include <xcb/xcb.h>
typedef struct _XDisplay Display;

#include <map>

namespace ny
{

class X11AppContext : public AppContext
{
protected:
	class LoopControlImpl;

protected:
    Display* xDisplay_  = nullptr;
	xcb_connection_t* xConnection_ = nullptr;
	xcb_window_t xDummyWindow_ = {};

    int xDefaultScreenNumber_ = 0;
    xcb_screen_t* xDefaultScreen_ = nullptr;

    std::map<xcb_window_t, X11WindowContext*> contexts_;
	std::map<std::string, xcb_atom_t> atoms_;

protected:
    bool processEvent(xcb_generic_event_t& ev, EventDispatcher& dispatcher);
    EventHandler* eventHandler(xcb_window_t w);

public:
    X11AppContext();
    virtual ~X11AppContext();

	virtual bool dispatchEvents(EventDispatcher& dispatcher) override;
	virtual bool dispatchLoop(EventDispatcher& dispatcher, LoopControl& control) override;

    Display* xDisplay() const { return xDisplay_; }
	xcb_connection_t* xConnection() const { return xConnection_; }
    int xDefaultScreenNumber() const { return xDefaultScreenNumber_; }
    xcb_screen_t* xDefaultScreen() const { return xDefaultScreen_; }

    void registerContext(xcb_window_t w, X11WindowContext& c);
    void unregisterContext(xcb_window_t w);
    X11WindowContext* windowContext(xcb_window_t win);

	xcb_atom_t atom(const std::string& name) const;
};

}
