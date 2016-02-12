#pragma once

#include <ny/backend/x11/include.hpp>
#include <ny/backend/appContext.hpp>

#include <X11/Xlib.h>
using XWindow = XID;

#include <X11/Xlib-xcb.h> /* for XGetXCBConnection, link with libX11-xcb */
#include <xcb/xcb.h>

#include <map>

namespace ny
{

enum class selectionType
{
    none = 0,

    clipboard,
    primary,
    dnd
};

class X11AppContext : public AppContext
{
protected:
    Display* xDisplay_;
    int xDefaultScreenNumber_;
    Screen* xDefaultScreen_;

	xcb_connection_t* xConnection_;

    XWindow selectionWindow_;

    selectionType lastSelection_ = selectionType::none; //needed?
	bool runMainLoop_ = 0;

    //clipboard
    //bool clipboardRequest_;
    //std::function<void(dataObject*)> clipboardCallback_;
    //dataTypes clipboardTypes_;

    //dataObject* clipboardPaste_ = 0;

    std::map<XWindow, X11WindowContext*> contexts_;

    void sendRedrawEvent(XWindow w);
    bool processEvent(xcb_generic_event_t& ev);
    Window* handler(XWindow w);

public:
    X11AppContext();
    virtual ~X11AppContext();

    virtual int mainLoop() override;
    virtual void exit() override;

    //virtual void setClipboard(dataObject& obj);
    //virtual bool getClipboard(dataTypes types, std::function<void(dataObject*)> Callback);

    Display* xDisplay() const { return xDisplay_; }
    int xDefaultScreenNumber() const { return xDefaultScreenNumber_; }
    Screen* xDefaultScreen() const { return xDefaultScreen_; }

    void registerContext(XWindow w, X11WindowContext& c);
    void unregisterContext(XWindow w);
    X11WindowContext* windowContext(XWindow win);

};

Display* xDisplay();
X11AppContext* x11AppContext();

}
