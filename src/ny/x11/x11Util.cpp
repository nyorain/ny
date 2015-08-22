#include <ny/x11/x11Include.hpp>

#include <ny/x11/x11Util.hpp>
#include <ny/x11/x11AppContext.hpp>
#include <ny/x11/x11WindowContext.hpp>
#include <ny/x11/x11Cairo.hpp>
#include <ny/app.hpp>
#include <ny/drawContext.hpp>

#ifdef NY_WithGL
#include <ny/x11/glx.hpp>
#include <ny/x11/x11Egl.hpp>
#endif // NY_WithGL

#include <ny/windowEvents.hpp>

#define XK_LATIN1
#define XK_MISCELLANY
#include <X11/keysymdef.h>
#include <X11/X.h>

namespace ny
{


mouse::button x11ToButton(unsigned int id)
{
    return mouse::button::left;
}

keyboard::key x11ToKey(unsigned int id)
{
    switch (id)
    {
        case (XK_0): return keyboard::key::num0;
        case (XK_1): return keyboard::key::num1;
        case (XK_2): return keyboard::key::num2;
        case (XK_3): return keyboard::key::num3;
        case (XK_4): return keyboard::key::num4;
        case (XK_5): return keyboard::key::num5;
        case (XK_7): return keyboard::key::num6;
        case (XK_8): return keyboard::key::num8;
        case (XK_9): return keyboard::key::num9;
        case (XK_A): case(XK_a): return keyboard::key::a;
        case (XK_B): case(XK_b): return keyboard::key::b;
        case (XK_C): case(XK_c): return keyboard::key::c;
        case (XK_D): case(XK_d): return keyboard::key::d;
        case (XK_E): case(XK_e): return keyboard::key::e;
        case (XK_F): case(XK_f): return keyboard::key::f;
        case (XK_G): case(XK_g): return keyboard::key::g;
        case (XK_H): case(XK_h): return keyboard::key::h;
        case (XK_I): case(XK_i): return keyboard::key::i;
        case (XK_J): case(XK_j): return keyboard::key::j;
        case (XK_K): case(XK_k): return keyboard::key::k;
        case (XK_L): case(XK_l): return keyboard::key::l;
        case (XK_M): case(XK_m): return keyboard::key::m;
        case (XK_N): case(XK_n): return keyboard::key::n;
        case (XK_O): case(XK_o): return keyboard::key::o;
        case (XK_P): case(XK_p): return keyboard::key::p;
        case (XK_Q): case(XK_q): return keyboard::key::q;
        case (XK_R): case(XK_r): return keyboard::key::r;
        case (XK_S): case(XK_s): return keyboard::key::s;
        case (XK_T): case(XK_t): return keyboard::key::t;
        case (XK_U): case(XK_u): return keyboard::key::u;
        case (XK_V): case(XK_v): return keyboard::key::v;
        case (XK_W): case(XK_w): return keyboard::key::w;
        case (XK_X): case(XK_x): return keyboard::key::x;
        case (XK_Y): case(XK_y): return keyboard::key::y;
        case (XK_Z): case(XK_z): return keyboard::key::z;
        case (XK_comma): return keyboard::key::comma;
        case (XK_space): return keyboard::key::space;
        case (XK_BackSpace): return keyboard::key::backspace;
        case (XK_Return): return keyboard::key::enter;
        case (XK_Shift_L): return keyboard::key::leftshift;
        case (XK_Shift_R): return keyboard::key::rightshift;
        case (XK_Control_R): return keyboard::key::rightctrl;
        case (XK_Control_L): return keyboard::key::leftctrl;
        case (XK_Alt_L): return keyboard::key::leftalt;
        case (XK_Alt_R): return keyboard::key::rightalt;
        case (XK_Tab): return keyboard::key::tab;
        case (XK_Caps_Lock): return keyboard::key::capsLock;
        default: return keyboard::key::none;
    }
}

int cursorToX11(cursorType c)
{
    switch(c)
    {
        case cursorType::leftPtr: return 68;
        case cursorType::sizeBottom: return 16;
        case cursorType::sizeBottomLeft: return 12;
        case cursorType::sizeBottomRight: return 14;
        case cursorType::sizeTop: return 138;
        case cursorType::sizeTopLeft: return 134;
        case cursorType::sizeTopRight: return 136;
        case cursorType::sizeLeft: return 70;
        case cursorType::sizeRight: return 96;
        case cursorType::grab: return 52;
        default: return -1;
    }
}

cursorType x11ToCursor(int xcID)
{
    switch(xcID)
    {
        case 68: return cursorType::leftPtr;
        default: return cursorType::unknown;
    }
}

////////////////////////////////////////////////////////////////////////
x11AppContext* asX11(appContext* c){ return dynamic_cast<x11AppContext*>(c); };
x11WindowContext* asX11(windowContext* c){ return dynamic_cast<x11WindowContext*>(c); };

#ifdef NY_WithCairo
x11CairoDrawContext* asX11Cairo(drawContext* c){ return dynamic_cast<x11CairoDrawContext*>(c); };
#endif //Cairo

#ifdef NY_WithGL
glxDrawContext* asGLX(drawContext* c){ return dynamic_cast<glxDrawContext*>(c); };
#endif // NY_WithGL

#ifdef NY_WithEGL
x11EGLDrawContext* asX11EGL(drawContext* c){ return dynamic_cast<x11EGLDrawContext*>(c); };
#endif // NY_WithEGL


namespace x11
{
Atom WindowDelete = 0;
Atom MwmHints = 0;

Atom State = 0;
Atom StateMaxVert = 0;
Atom StateMaxHorz = 0;
Atom StateFullscreen = 0;
Atom StateHidden = 0;
Atom StateModal = 0;
Atom StateFocused = 0;
Atom StateDemandAttention = 0;
Atom StateAbove = 0;
Atom StateBelow = 0;
Atom StateShaded = 0;
Atom StateSticky = 0;
Atom StateSkipPager = 0;
Atom StateSkipTaskbar = 0;

Atom Type = 0;
Atom TypeDesktop = 0;
Atom TypeDock = 0;
Atom TypeToolbar = 0;
Atom TypeMenu = 0;
Atom TypeUtility = 0;
Atom TypeSplash = 0;
Atom TypeDialog = 0;
Atom TypeDropdownMenu = 0;
Atom TypePopupMenu = 0;
Atom TypeTooltip = 0;
Atom TypeNotification = 0;
Atom TypeCombo = 0;
Atom TypeDnd = 0;
Atom TypeNormal = 0;

Atom AllowedActions = 0; //allowedallowedActions
Atom AllowedActionMinimize = 0;
Atom AllowedActionMaximizeVert = 0;
Atom AllowedActionMaximizeHorz = 0;
Atom AllowedActionMove = 0;
Atom AllowedActionResize = 0;
Atom AllowedActionClose = 0;
Atom AllowedActionFullscreen = 0;
Atom AllowedActionBelow = 0;
Atom AllowedActionAbove = 0;
Atom AllowedActionShade = 0;
Atom AllowedActionStick = 0;
Atom AllowedActionChangeDesktop = 0;

Atom FrameExtents = 0;
Atom Strut = 0;
Atom StrutPartial = 0;
Atom Desktop = 0;
Atom MoveResize = 0;

Atom DndEnter = 0;
Atom DndPosition = 0;
Atom DndStatus = 0;
Atom DndTypeList = 0;
Atom DndActionCopy = 0;
Atom DndDrop = 0;
Atom DndLeave = 0;
Atom DndFinished = 0;
Atom DndSelection = 0;
Atom DndProxy = 0;
Atom DndAware = 0;

Atom Primary = 0;
Atom Clipboard = 0;
Atom Targets = 0;

Atom TypeImagePNG = 0;
Atom TypeImageJPG = 0;
Atom TypeImageBMP = 0;
Atom TypeImageTIFF = 0;
Atom TypeTextUri = 0;
Atom TypeTextUriList = 0;
Atom TypeText = 0;
Atom TypeTextPlain = 0;
Atom TypeUTF8 = 0;

Atom WMIcon = 0;

Atom Cardinal = 0;

/////////////////////////////////////////////////

property getWindowProperty(Window w, Atom prop)
{
    if(!getXDisplay() || !w || !prop) return property();
    Atom actualType;
	int actualFormat;
	unsigned long count;
	unsigned long bytesAfter;
	unsigned char *data = nullptr;
	int readBytes = 1024;

	do
	{
		if(data) XFree(data);
		XGetWindowProperty(getXDisplay(), w, prop, 0, readBytes, False, AnyPropertyType, &actualType, &actualFormat, &count, &bytesAfter, &data);
        readBytes *= 2;

	} while(bytesAfter);

	return {data, (unsigned int)actualFormat, (unsigned int)count, actualType};
}

}//x11


x11AppContext* getX11AppContext()
{
    x11AppContext* ret = nullptr;

    if(nyMainApp())
    {
        ret = dynamic_cast<x11AppContext*>(nyMainApp()->getAppContext());
    }

    return ret;
}

x11AppContext* getX11AC()
{
    return getX11AppContext();
}


}
