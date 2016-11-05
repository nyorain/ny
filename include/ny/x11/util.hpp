#pragma once

#include <ny/x11/include.hpp>

#include <ny/keyboardContext.hpp>
#include <ny/mouseContext.hpp>
#include <ny/cursor.hpp>

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>

namespace ny
{

///X11EventData stores the native xcb event for later use.
///To see where this might be neede look at the X11WC::beginResize and X11WC::beginMove functions.
class X11EventData : public EventData
{
public:
    X11EventData(const xcb_generic_event_t& e) : event(e) {};
    xcb_generic_event_t event;
};


namespace x11
{

//dummy used for unnamed struct typedef declaration
struct EwmhConnection : public xcb_ewmh_connection_t {};

struct Atoms
{
	xcb_atom_t xdndEnter;
	xcb_atom_t xdndPosition;
	xcb_atom_t xdndStatus;
	xcb_atom_t xdndTypeList;
	xcb_atom_t xdndActionCopy;
	xcb_atom_t xdndActionMove;
	xcb_atom_t xdndActionAsk;
	xcb_atom_t xdndDrop;
	xcb_atom_t xdndLeave;
	xcb_atom_t xdndFinished;
	xcb_atom_t xdndSelection;
	xcb_atom_t xdndProxy;
	xcb_atom_t xdndAware;

	xcb_atom_t primary;
	xcb_atom_t clipboard;

	xcb_atom_t targets;
	xcb_atom_t text;
	xcb_atom_t string;
	xcb_atom_t utf8string;

	xcb_atom_t wmDeleteWindow;
	xcb_atom_t motifWmHints;

	struct
	{
		xcb_atom_t textPlain;
		xcb_atom_t textPlainUtf8;
		xcb_atom_t textUriList;

		xcb_atom_t imageJpeg;
		xcb_atom_t imageGif;
		xcb_atom_t imagePng;
		xcb_atom_t imageBmp;

		xcb_atom_t imageData; //image/x-ny-data
		xcb_atom_t timePoint; //x-special/ny-time-point
		xcb_atom_t timeDuration; //x-special/ny-time-duration
		xcb_atom_t raw; //x-special/ny-raw-buffer
	} mime;
};

bool testCookie(xcb_connection_t& conn, const xcb_void_cookie_t& cookie, const char* msg = nullptr);

// std::vector<std::uint8_t> getProperty(xcb_connection_t& connection, xcb_window_t window,
// 	xcb_atom_t property, xcb_atom_t type);
// 
// void setProperty(xcb_connection_t& connection, xcb_window_t window, xcb_atom_t property,
// 	xcb_atom_t type, nytl::Range<uint8_t> data);
// 
// void setProperty(xcb_connection_t& connection, xcb_window_t window, xcb_atom_t property,
// 	xcb_atom_t type, nytl::Range<uint32_t> data);

}

//utility conversions.
ImageDataFormat visualToFormat(const xcb_visualtype_t& visual, unsigned int depth);

MouseButton x11ToButton(unsigned int button);
unsigned int buttonToX11(MouseButton);


//Both poorly implemented
//Int values instead of names
// int cursorToX11(CursorType cursor);
// CursorType x11ToCursor(int xcID);

}
