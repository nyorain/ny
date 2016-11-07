#include <ny/x11/util.hpp>
#include <ny/log.hpp>
#include <X11/Xlib.h>

namespace ny
{

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
		{ 0xFF000000u, 0u, 0u, 0u, ImageDataFormat::a8 }, //should be r8?
		{ 0x0u, 0u, 0u, 0xFF000000u, ImageDataFormat::a8 }
	};

	auto a = 0u;
	if(depth == 32) a = 0xFFFFFFFFu & ~(v.red_mask | v.green_mask | v.blue_mask);

	for(auto& f : formats)
		if(v.red_mask == f.r && v.green_mask == f.g && v.blue_mask == f.b && a == f.a)
			return f.format;

	return ImageDataFormat::none;
}

MouseButton x11ToButton(unsigned int button)
{
	switch(button)
	{
		case 1: return MouseButton::left;
		case 2: return MouseButton::middle;
		case 3: return MouseButton::right;
		default: return MouseButton::unknown;
	}
}

unsigned int buttonToX11(MouseButton button)
{
	switch(button)
	{
		case MouseButton::left: return 1u;
		case MouseButton::middle: return 2u;
		case MouseButton::right: return 3u;
		default: return 0u;
	}
}

//X11ErrorCategory
X11ErrorCategory::X11ErrorCategory(Display& dpy, xcb_connection_t& conn)
	: xDisplay_(&dpy), xConnection_(&conn)
{
}

X11ErrorCategory::X11ErrorCategory(X11ErrorCategory&& other)
	: xDisplay_(other.xDisplay_), xConnection_(other.xConnection_)
{
	other.xDisplay_ = {};
	other.xConnection_ = {};
}

X11ErrorCategory& X11ErrorCategory::operator=(X11ErrorCategory&& other)
{
	xDisplay_ = other.xDisplay_;
	xConnection_ = other.xConnection_;

	other.xDisplay_ = {};
	other.xConnection_ = {};

	return *this;
};

std::string X11ErrorCategory::message(int code) const
{
	return x11::errorMessage(*xDisplay_, code);
}

std::error_code X11ErrorCategory::errorCode(int error) const
{
	return {error, *this};
}

std::error_code X11ErrorCategory::check(xcb_void_cookie_t cookie) const
{
	auto e = xcb_request_check(xConnection_, cookie);
	if(e)
	{
		auto code = std::error_code(e->error_code, *this);
		free(e);
		return code;
	}

	return {};
}

bool X11ErrorCategory::check(xcb_void_cookie_t cookie, std::error_code& ec) const
{
	auto e = xcb_request_check(xConnection_, cookie);
	if(e)
	{
		ec = {e->error_code, *this};
		free(e);
		return false;
	}

	return true;
}

bool X11ErrorCategory::checkWarn(xcb_void_cookie_t cookie, nytl::StringParam msg) const
{
	auto e = xcb_request_check(xConnection_, cookie);
	if(e)
	{
		auto errorMsg = x11::errorMessage(*xDisplay_, e->error_code);

		if(msg) warning("ny::X11: error code ", e->error_code, ", ", errorMsg, ": ", msg);
		else warning("ny::X11: error code ", e->error_code, ", ", errorMsg, ": ", msg);

		free(e);
		return false;
	}

	return true;
}

void X11ErrorCategory::checkThrow(xcb_void_cookie_t cookie, nytl::StringParam msg) const
{
	std::error_code ec;
	if(!check(cookie, ec)) throw std::system_error(ec, msg);
}

namespace x11
{

Property readProperty(xcb_connection_t& connection, xcb_atom_t atom, xcb_window_t window,
	bool del, xcb_generic_error_t* error)
{
	error = nullptr;
	auto length = 1;

	auto cookie = xcb_get_property(&connection, false, window, atom, XCB_ATOM_ANY, 0, length);
	auto reply = xcb_get_property_reply(&connection, cookie, &error);

	if(reply && !error && (reply->bytes_after || del))
	{
		length = reply->length;
		free(reply);

		cookie = xcb_get_property(&connection, del, window, atom, XCB_ATOM_ANY, 0, length);
		reply = xcb_get_property_reply(&connection, cookie, &error);
	}

	Property ret;
	if(!error)
	{
		ret.format = reply->format;
		ret.type = reply->type;

		auto begin = static_cast<uint8_t*>(xcb_get_property_value(reply));
		ret.data = {begin, begin + xcb_get_property_value_length(reply)};
	}

	if(reply) free(reply);
	return ret;
}

xcb_void_cookie_t changeProperty(xcb_connection_t& conn, xcb_atom_t atom, xcb_window_t window,
	const Property& prop, unsigned int mode)
{
	auto count = 4 * prop.data.size() / prop.format;
	auto data = static_cast<const void*>(prop.data.data());
	return xcb_change_property_checked(&conn, mode, window, atom, prop.type, prop.format, count, data);
}

std::string errorMessage(Display& dpy, unsigned int error)
{
	//TODO: any way to implement this in a way that assures our buffer is large enough?
	//What is the return value of XGetErrorText. It is not documented
	char buffer[256];
	::XGetErrorText(&dpy, error, buffer, 255);
	return buffer;
}

}

}
