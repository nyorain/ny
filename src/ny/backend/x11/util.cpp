#include <ny/config.hpp>
#include <ny/backend/x11/include.hpp>

#include <ny/backend/x11/util.hpp>
#include <ny/backend/x11/appContext.hpp>
#include <ny/backend/x11/windowContext.hpp>
#include <ny/backend/x11/cairo.hpp>
#include <ny/app/app.hpp>
#include <ny/window/events.hpp>

#define XK_LATIN1
#define XK_MISCELLANY
#include <X11/keysymdef.h>
#include <X11/X.h>

namespace ny
{

Mouse::Button x11ToButton(unsigned int id)
{
	switch(id)
	{
		default: return Mouse::Button::left;
	}
}

Keyboard::Key x11ToKey(unsigned int id)
{
    switch (id)
    {
        case (XK_0): return Keyboard::Key::n0;
        case (XK_1): return Keyboard::Key::n1;
        case (XK_2): return Keyboard::Key::n2;
        case (XK_3): return Keyboard::Key::n3;
        case (XK_4): return Keyboard::Key::n4;
        case (XK_5): return Keyboard::Key::n5;
        case (XK_7): return Keyboard::Key::n6;
        case (XK_8): return Keyboard::Key::n8;
        case (XK_9): return Keyboard::Key::n9;
        case (XK_A): case(XK_a): return Keyboard::Key::a;
        case (XK_B): case(XK_b): return Keyboard::Key::b;
        case (XK_C): case(XK_c): return Keyboard::Key::c;
        case (XK_D): case(XK_d): return Keyboard::Key::d;
        case (XK_E): case(XK_e): return Keyboard::Key::e;
        case (XK_F): case(XK_f): return Keyboard::Key::f;
        case (XK_G): case(XK_g): return Keyboard::Key::g;
        case (XK_H): case(XK_h): return Keyboard::Key::h;
        case (XK_I): case(XK_i): return Keyboard::Key::i;
        case (XK_J): case(XK_j): return Keyboard::Key::j;
        case (XK_K): case(XK_k): return Keyboard::Key::k;
        case (XK_L): case(XK_l): return Keyboard::Key::l;
        case (XK_M): case(XK_m): return Keyboard::Key::m;
        case (XK_N): case(XK_n): return Keyboard::Key::n;
        case (XK_O): case(XK_o): return Keyboard::Key::o;
        case (XK_P): case(XK_p): return Keyboard::Key::p;
        case (XK_Q): case(XK_q): return Keyboard::Key::q;
        case (XK_R): case(XK_r): return Keyboard::Key::r;
        case (XK_S): case(XK_s): return Keyboard::Key::s;
        case (XK_T): case(XK_t): return Keyboard::Key::t;
        case (XK_U): case(XK_u): return Keyboard::Key::u;
        case (XK_V): case(XK_v): return Keyboard::Key::v;
        case (XK_W): case(XK_w): return Keyboard::Key::w;
        case (XK_X): case(XK_x): return Keyboard::Key::x;
        case (XK_Y): case(XK_y): return Keyboard::Key::y;
        case (XK_Z): case(XK_z): return Keyboard::Key::z;
        case (XK_comma): return Keyboard::Key::comma;
        case (XK_space): return Keyboard::Key::space;
        case (XK_BackSpace): return Keyboard::Key::backspace;
        case (XK_Return): return Keyboard::Key::enter;
        case (XK_Shift_L): return Keyboard::Key::leftshift;
        case (XK_Shift_R): return Keyboard::Key::rightshift;
        case (XK_Control_R): return Keyboard::Key::rightctrl;
        case (XK_Control_L): return Keyboard::Key::leftctrl;
        case (XK_Alt_L): return Keyboard::Key::leftalt;
        case (XK_Alt_R): return Keyboard::Key::rightalt;
        case (XK_Tab): return Keyboard::Key::tab;
        case (XK_Caps_Lock): return Keyboard::Key::capsLock;
        default: return Keyboard::Key::none;
    }
}

int cursorToX11(Cursor::Type c)
{
    switch(c)
    {
        case Cursor::Type::leftPtr: return 68;
        case Cursor::Type::sizeBottom: return 16;
        case Cursor::Type::sizeBottomLeft: return 12;
        case Cursor::Type::sizeBottomRight: return 14;
        case Cursor::Type::sizeTop: return 138;
        case Cursor::Type::sizeTopLeft: return 134;
        case Cursor::Type::sizeTopRight: return 136;
        case Cursor::Type::sizeLeft: return 70;
        case Cursor::Type::sizeRight: return 96;
        case Cursor::Type::grab: return 52;
        default: return -1;
    }
}

Cursor::Type x11ToCursor(int xcID)
{
    switch(xcID)
    {
        case 68: return Cursor::Type::leftPtr;
        default: return Cursor::Type::unknown;
    }
}

}
