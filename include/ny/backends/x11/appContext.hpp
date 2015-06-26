#pragma once

#include "x11Include.hpp"
#include "backends/appContext.hpp"

#include <X11/Xlib.h>
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

class x11AppContext : public appContext
{
protected:
    Display* xDisplay_;
    int xDefaultScreenNumber_;
    Screen* xDefaultScreen_;

    Window selectionWindow_;

    selectionType lastSelection_ = selectionType::none; //needed?

    //clipboard
    //bool clipboardRequest_;
    //std::function<void(dataObject*)> clipboardCallback_;
    //dataTypes clipboardTypes_;

    //dataObject* clipboardPaste_ = 0;

    std::map<Window, x11WindowContext*> contexts_;

    void sendRedrawEvent(Window w);

public:
    x11AppContext();
    virtual ~x11AppContext();

    virtual void init();

    virtual bool mainLoopCall();
    virtual bool mainLoopCallNonBlocking();

    //virtual void setClipboard(dataObject& obj);
    //virtual bool getClipboard(dataTypes types, std::function<void(dataObject*)> callback);

    Display* getXDisplay() const { return xDisplay_; }
    int getXDefaultScreenNumber() const { return xDefaultScreenNumber_; }
    Screen* getXDefaultScreen() const { return xDefaultScreen_; }

    void registerContext(Window w, x11WindowContext* c);
    void unregisterContext(Window w);
    void unregisterContext(x11WindowContext* c){ /*todo: implement */};
    x11WindowContext* getWindowContext(Window win);

    virtual void exit();
};

Display* getXDisplay();

}
