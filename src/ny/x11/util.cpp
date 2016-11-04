#include <ny/x11/util.hpp>
#include <ny/log.hpp>

#define XK_LATIN1
#define XK_MISCELLANY
#include <X11/keysymdef.h>
#include <X11/cursorfont.h>
#include <X11/X.h>

namespace ny
{

// int cursorToX11(CursorType c)
// {
//     switch(c)
//     {
//         case CursorType::leftPtr: return XC_left_ptr;
//         case CursorType::rightPtr: return XC_right_ptr;
//         case CursorType::sizeBottom: return XC_bottom_side;
//         case CursorType::sizeBottomLeft: return XC_bottom_left_corner;
//         case CursorType::sizeBottomRight: return XC_bottom_right_corner;
//         case CursorType::sizeTop: return XC_top_side;
//         case CursorType::sizeTopLeft: return XC_top_left_corner;
//         case CursorType::sizeTopRight: return XC_top_right_corner;
//         case CursorType::sizeLeft: return XC_left_side;
//         case CursorType::sizeRight: return XC_right_side;
//         case CursorType::grab: return XC_fleur;
//         default: return -1;
//     }
// }
// 
// CursorType x11ToCursor(int xcID)
// {
//     switch(xcID)
//     {
//         case 68: return CursorType::leftPtr;
//         default: return CursorType::unknown;
//     }
// }

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
		free(e);
		return false;
	}

	return true;
}

}

}
