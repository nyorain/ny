#pragma once

#include <ny/backend/x11/include.hpp>

#include <ny/app/keyboard.hpp>
#include <ny/app/mouse.hpp>
#include <ny/window/cursor.hpp>
#include <ny/app/data.hpp>

#include <X11/Xutil.h>
#include <X11/Xlib.h>
using XWindow = XID;

#include <map>

namespace ny
{

Mouse::Button x11ToButton(unsigned int id);
Keyboard::Key x11ToKey(unsigned int id);

int cursorToX11(Cursor::Type c);
Cursor::Type x11ToCursor(int xcID);

X11AppContext* asX11(AppContext* c);
X11WindowContext* asX11(WindowContext* c);

X11CairoDrawContext* asX11Cairo(DrawContext* c);
GlxContext* asGlx(DrawContext* c);

//x11Event
class X11EventData : public DeriveCloneable<EventData, X11EventData>
{
public:
    X11EventData(const xcb_generic_event_t& e) : event(e) {};
    xcb_generic_event_t event;
};

namespace eventType
{
	namespace context
	{
		constexpr unsigned int x11Reparent = 11;
	}
}
class X11ReparentEvent : public ContextEventBase<eventType::context::x11Reparent, X11ReparentEvent>
{
public:
	using ContextEvBase::ContextEvBase;
    XReparentEvent event;
};


namespace x11
{

//
extern Atom WindowDelete;
extern Atom MwmHints;

extern Atom State;
extern Atom StateMaxVert;
extern Atom StateMaxHorz;
extern Atom StateFullscreen;
extern Atom StateHidden;
extern Atom StateModal;
extern Atom StateFocused;
extern Atom StateDemandAttention;
extern Atom StateAbove;
extern Atom StateBelow;
extern Atom StateShaded;
extern Atom StateSticky;
extern Atom StateSkipPager;
extern Atom StateSkipTaskbar;

extern Atom Type;
extern Atom TypeDesktop;
extern Atom TypeDock;
extern Atom TypeToolbar;
extern Atom TypeMenu;
extern Atom TypeUtility;
extern Atom TypeSplash;
extern Atom TypeDialog;
extern Atom TypeDropdownMenu;
extern Atom TypePopupMenu;
extern Atom TypeTooltip;
extern Atom TypeNotification;
extern Atom TypeCombo;
extern Atom TypeDnd;
extern Atom TypeNormal;

extern Atom AllowedActions; //allowedallowedActions
extern Atom AllowedActionMinimize;
extern Atom AllowedActionMaximizeVert;
extern Atom AllowedActionMaximizeHorz;
extern Atom AllowedActionMove;
extern Atom AllowedActionResize;
extern Atom AllowedActionClose;
extern Atom AllowedActionFullscreen;
extern Atom AllowedActionBelow;
extern Atom AllowedActionAbove;
extern Atom AllowedActionShade;
extern Atom AllowedActionStick;
extern Atom AllowedActionChangeDesktop;

extern Atom FrameExtents;
extern Atom Strut;
extern Atom StrutPartial;
extern Atom Desktop;
extern Atom MoveResize;

extern Atom DndEnter;
extern Atom DndPosition;
extern Atom DndStatus;
extern Atom DndTypeList;
extern Atom DndActionCopy;
extern Atom DndDrop;
extern Atom DndLeave;
extern Atom DndFinished;
extern Atom DndSelection;
extern Atom DndProxy;
extern Atom DndAware;

extern Atom Primary;
extern Atom Clipboard;
extern Atom Targets;

extern Atom TypeImagePNG;
extern Atom TypeImageJPG;
extern Atom TypeImageBMP;
extern Atom TypeImageTIFF;
extern Atom TypeTextUri;
extern Atom TypeTextUriList;
extern Atom TypeText;
extern Atom TypeTextPlain;
extern Atom TypeUTF8;

extern Atom WMIcon;

extern Atom Cardinal;


//Property
class Property //ny::x11::property -> dont need to be called xproperty
{
public:
    unsigned char* data;
    unsigned int format;
    unsigned int count;
    Atom type;
};

const unsigned char MoveResizeSizeTopLeft      = 0;
const unsigned char MoveResizeSizeTop          = 1;
const unsigned char MoveResizeSizeTopRight     = 2;
const unsigned char MoveResizeSizeRight        = 3;
const unsigned char MoveResizeSizeBottomRight  = 4;
const unsigned char MoveResizeSizeBottom       = 5;
const unsigned char MoveResizeSizeBottomLeft   = 6;
const unsigned char MoveResizeSizeLeft         = 7;
const unsigned char MoveResizeMove             = 8;
const unsigned char MoveResizeSizeKeyboard     = 9;
const unsigned char MoveResizeMoveKeyboard     = 10;
const unsigned char MoveResizeCancel           = 11;

Property windowProperty(XWindow w, Atom property);

}

}
