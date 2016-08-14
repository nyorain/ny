#include <ny/config.hpp>
#include <ny/backend/x11/include.hpp>

#include <ny/backend/x11/util.hpp>
#include <ny/backend/x11/appContext.hpp>
#include <ny/backend/x11/windowContext.hpp>
#include <ny/backend/x11/cairo.hpp>
#include <ny/window/events.hpp>

#define XK_LATIN1
#define XK_MISCELLANY
#include <X11/keysymdef.h>
#include <X11/X.h>

namespace ny
{

MouseButton x11ToButton(unsigned int id)
{
	switch(id)
	{
		default: return MouseButton::left;
	}
}

Key x11ToKey(unsigned int id)
{
    switch (id)
    {
        case (XK_0): return Key::n0;
        case (XK_1): return Key::n1;
        case (XK_2): return Key::n2;
        case (XK_3): return Key::n3;
        case (XK_4): return Key::n4;
        case (XK_5): return Key::n5;
        case (XK_7): return Key::n6;
        case (XK_8): return Key::n8;
        case (XK_9): return Key::n9;
        case (XK_A): case(XK_a): return Key::a;
        case (XK_B): case(XK_b): return Key::b;
        case (XK_C): case(XK_c): return Key::c;
        case (XK_D): case(XK_d): return Key::d;
        case (XK_E): case(XK_e): return Key::e;
        case (XK_F): case(XK_f): return Key::f;
        case (XK_G): case(XK_g): return Key::g;
        case (XK_H): case(XK_h): return Key::h;
        case (XK_I): case(XK_i): return Key::i;
        case (XK_J): case(XK_j): return Key::j;
        case (XK_K): case(XK_k): return Key::k;
        case (XK_L): case(XK_l): return Key::l;
        case (XK_M): case(XK_m): return Key::m;
        case (XK_N): case(XK_n): return Key::n;
        case (XK_O): case(XK_o): return Key::o;
        case (XK_P): case(XK_p): return Key::p;
        case (XK_Q): case(XK_q): return Key::q;
        case (XK_R): case(XK_r): return Key::r;
        case (XK_S): case(XK_s): return Key::s;
        case (XK_T): case(XK_t): return Key::t;
        case (XK_U): case(XK_u): return Key::u;
        case (XK_V): case(XK_v): return Key::v;
        case (XK_W): case(XK_w): return Key::w;
        case (XK_X): case(XK_x): return Key::x;
        case (XK_Y): case(XK_y): return Key::y;
        case (XK_Z): case(XK_z): return Key::z;
        case (XK_comma): return Key::comma;
        case (XK_space): return Key::space;
        case (XK_BackSpace): return Key::backspace;
        case (XK_Return): return Key::enter;
        case (XK_Shift_L): return Key::leftshift;
        case (XK_Shift_R): return Key::rightshift;
        case (XK_Control_R): return Key::rightctrl;
        case (XK_Control_L): return Key::leftctrl;
        case (XK_Alt_L): return Key::leftalt;
        case (XK_Alt_R): return Key::rightalt;
        case (XK_Tab): return Key::tab;
        case (XK_Caps_Lock): return Key::capsLock;
        default: return Key::none;
    }
}

int cursorToX11(CursorType c)
{
    switch(c)
    {
        case CursorType::leftPtr: return 68;
        case CursorType::sizeBottom: return 16;
        case CursorType::sizeBottomLeft: return 12;
        case CursorType::sizeBottomRight: return 14;
        case CursorType::sizeTop: return 138;
        case CursorType::sizeTopLeft: return 134;
        case CursorType::sizeTopRight: return 136;
        case CursorType::sizeLeft: return 70;
        case CursorType::sizeRight: return 96;
        case CursorType::grab: return 52;
        default: return -1;
    }
}

CursorType x11ToCursor(int xcID)
{
    switch(xcID)
    {
        case 68: return CursorType::leftPtr;
        default: return CursorType::unknown;
    }
}

}
