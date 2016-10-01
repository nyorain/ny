#include <ny/backend/x11/util.hpp>
#include <ny/base/log.hpp>

#define XK_LATIN1
#define XK_MISCELLANY
#include <X11/keysymdef.h>
#include <X11/cursorfont.h>
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
        case CursorType::leftPtr: return XC_left_ptr;
        case CursorType::sizeBottom: return XC_bottom_side;
        case CursorType::sizeBottomLeft: return XC_bottom_left_corner;
        case CursorType::sizeBottomRight: return XC_bottom_right_corner;
        case CursorType::sizeTop: return XC_top_side;
        case CursorType::sizeTopLeft: return XC_top_left_corner;
        case CursorType::sizeTopRight: return XC_top_right_corner;
        case CursorType::sizeLeft: return XC_left_side;
        case CursorType::sizeRight: return XC_right_side;
        case CursorType::grab: return XC_fleur;
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

const char* cursorToX11Char(CursorType c)
{
    switch(c)
    {
        case CursorType::leftPtr: return "left_ptr";
        case CursorType::sizeBottom: return "bottom_side";
        case CursorType::sizeBottomLeft: return "bottom_left_corner";
        case CursorType::sizeBottomRight: return "bottom_right_corner";
        case CursorType::sizeTop: return "top_side";
        case CursorType::sizeTopLeft: return "top_left_corner";
        case CursorType::sizeTopRight: return "top_right_corner";
        case CursorType::sizeLeft: return "left_side";
        case CursorType::sizeRight: return "right_side";
        case CursorType::grab: return "fleur";
        default: return nullptr;
    }
}

ImageDataFormat visualToFormat(const xcb_visualtype_t& v, unsigned int depth)
{
	if(depth != 24 && depth != 32) return ImageDataFormat::none;

	//A simple format map that maps the rgb[a] mask values of the visualtype to a format
	//Note that only the rgb[a] masks of some visuals will result in a valid format,
	//usually ImageDataFormat::none is returned
	struct  
	{
		std::uint32_t r, g, b, a;
		ImageDataFormat format;
	} static formats[] = 
	{
		{ 0xFF000000u, 0x00FF0000u, 0x0000FF00u, 0x000000FFu, ImageDataFormat::rgba8888 },
		{ 0x0000FF00u, 0x00FF0000u, 0xFF000000u, 0x000000FFu, ImageDataFormat::bgra8888 },
		{ 0x00FF0000u, 0x0000FF00u, 0x000000FFu, 0xFF000000u, ImageDataFormat::argb8888 },
		{ 0xFF000000u, 0x00FF0000u, 0x0000FF00u, 0u, ImageDataFormat::rgb888 },
		{ 0x0000FF00u, 0x00FF0000u, 0xFF000000u, 0u, ImageDataFormat::bgr888 },
		// { 0xFF000000u, 0u, 0u, 0u, ImageDataFormat::a8 } //XXX: does this format exist?
	};

	auto a = 0u;
	if(depth == 32) a = 0xFFFFFFFFu & ~(v.red_mask | v.green_mask | v.blue_mask);

	for(auto& f : formats)
		if(v.red_mask == f.r && v.green_mask == f.g && v.blue_mask == f.b && a == f.a)
			return f.format;

	return ImageDataFormat::none;
}

namespace x11
{

bool testCookie(xcb_connection_t& xconn, const xcb_void_cookie_t& cookie, const char* msg)
{
	auto e = xcb_request_check(&xconn, cookie);
	if(e)
	{
		if(!msg) msg = "<unknown>";
		warning(msg, ": received xcb protocol error ", (int) e->error_code);
		return false;
	}

	return true;
}

}

}
