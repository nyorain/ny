#pragma once

#include <ny/backends/x11/x11Include.hpp>
#include <X11/Xutil.h>

namespace ny
{

x11AppContext* asX11(appContext* c);
x11WindowContext* asX11(windowContext* c);
x11ToplevelWindowContext* asX11(toplevelWindowContext* c);
x11ChildWindowContext* asX11(childWindowContext* c);
x11ChildWindowContext* asX11Child(windowContext* c);
x11ToplevelWindowContext* asX11Toplevel(windowContext* c);

x11CairoToplevelWindowContext* asX11Cairo(toplevelWindowContext* c);
x11CairoChildWindowContext* asX11Cairo(childWindowContext* c);
x11CairoContext* asX11Cairo(windowContext* c);

#ifdef NY_WithGL
glxToplevelWindowContext* asGLX(toplevelWindowContext* c);
glxChildWindowContext* asGLX(childWindowContext* c);
glxWindowContext* asGLX(windowContext* c);
glxContext* asGLX(glContext* c);

x11EGLToplevelWindowContext* asx11EGL(toplevelWindowContext* c);
x11EGLChildWindowContext* asx11EGL(childWindowContext* c);
x11EGLContext* asx11EGL(windowContext* c);
#endif // NY_WithGL

namespace x11
{
//context hints
const unsigned int hintOverrideRedirect = (1 << 1);

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

const unsigned long MwmDecoBorder = (1L << 1);
const unsigned long MwmDecoResize = (1L << 2);
const unsigned long MwmDecoTitle = (1L << 3);
const unsigned long MwmDecoMenu = (1L << 4);
const unsigned long MwmDecoMinimize = (1L << 5);
const unsigned long MwmDecoMaximize = (1L << 5);
const unsigned long MwmDecoAll = MwmDecoBorder | MwmDecoResize | MwmDecoTitle | MwmDecoMenu | MwmDecoMinimize | MwmDecoMaximize;

const unsigned long MwmFuncResize = MwmDecoBorder;
const unsigned long MwmFuncMove = MwmDecoResize;
const unsigned long MwmFuncMinimize = MwmDecoTitle;
const unsigned long MwmFuncMaximize = MwmDecoMenu;
const unsigned long MwmFuncClose = MwmDecoMinimize;
const unsigned long MwmFuncAll = MwmFuncResize | MwmFuncMove | MwmFuncMaximize | MwmFuncMinimize | MwmFuncClose;

const unsigned long MwmHintsFunc = (1L << 0);
const unsigned long MwmHintsDeco = MwmDecoBorder;
const unsigned long MwmHintsInput = MwmDecoResize;
const unsigned long MwmHintsStatus = MwmDecoTitle;

const unsigned long MwmInputModeless = (1L << 0);
const unsigned long MwmInputPrimaryModal = MwmDecoBorder; //MWM_INPUT_PRIMARY_APPLICATION_MODAL
const unsigned long MwmInputSystemModal = MwmDecoResize;
const unsigned long MwmInputFullModal = MwmDecoTitle; //MWM_INPUT_FULL_APPLICATION_MODAL

const unsigned long MwmTearoffWindow = (1L << 0);

const unsigned long MwmInfoStartupStandard = (1L << 0);
const unsigned long MwmInfoStartupCustom = MwmDecoBorder;

struct mwmHints
{
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
    long inputMode;
    unsigned long status;
};

//info
struct mwmInfo
{
    long flags;
    Window wm_window;
};

//Property
struct property
{
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


///////////////////////////////////////////////////////////////7


property getWindowProperty(Window w, Atom property);

}

/* for reference, not needed
#define _XA_MOTIF_BINDINGS		"_MOTIF_BINDINGS"
#define _XA_MOTIF_WM_HINTS		"_MOTIF_WM_HINTS"
#define _XA_MOTIF_WM_MESSAGES	"_MOTIF_WM_MESSAGES"
#define _XA_MOTIF_WM_OFFSET		"_MOTIF_WM_OFFSET"
#define _XA_MOTIF_WM_MENU		"_MOTIF_WM_MENU"
#define _XA_MOTIF_WM_INFO		"_MOTIF_WM_INFO"
#define _XA_MWM_HINTS			_XA_MOTIF_WM_HINTS
#define _XA_MWM_MESSAGES		_XA_MOTIF_WM_MESSAGES
#define _XA_MWM_MENU			_XA_MOTIF_WM_MENU
#define _XA_MWM_INFO			_XA_MOTIF_WM_INFO
*/


}
