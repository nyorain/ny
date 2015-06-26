#include <ny/backends/x11/x11Include.hpp>

#include <ny/backends/x11/utils.hpp>
#include <ny/window/windowEvents.hpp>

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
        case cursorType::LeftPtr: return 68;
        case cursorType::SizeBottom: return 16;
        case cursorType::SizeBottomLeft: return 12;
        case cursorType::SizeBottomRight: return 14;
        case cursorType::SizeTop: return 138;
        case cursorType::SizeTopLeft: return 134;
        case cursorType::SizeTopRight: return 136;
        case cursorType::SizeLeft: return 70;
        case cursorType::SizeRight: return 96;
        case cursorType::Grab: return 52;
        default: return -1;
    }
}

cursorType x11ToCursor(int xcID)
{
    switch(xcID)
    {
        case 68: return cursorType::LeftPtr;
        default: return cursorType::Unknown;
    }
}

long eventTypeToX11(unsigned int evType)
{
    switch(evType)
    {
        case eventType::windowDraw: return ExposureMask;
        case eventType::windowFocus: return FocusChangeMask;
        case eventType::windowSize: return StructureNotifyMask;
        case eventType::windowPosition: return StructureNotifyMask;
        case eventType::windowShow: return StructureNotifyMask;
        case eventType::mouseMove: return PointerMotionMask;
        case eventType::mouseCross: return EnterWindowMask | LeaveWindowMask;
        case eventType::mouseButton: return ButtonPressMask | ButtonReleaseMask;
        case eventType::key: return KeyPressMask | KeyReleaseMask;
        default: return -1;
    }
}

long eventMapToX11(const std::map<unsigned int, bool>& evMap)
{
    long ret = 0;

    for(std::map<unsigned int, bool>::const_iterator it = evMap.begin(); it != evMap.end(); it++)
    {
        if(!it->second) continue;
        long xCode = eventTypeToX11(it->first);
        if(xCode != -1) ret |= xCode;
    }

    return ret;
}

}
