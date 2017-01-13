// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/x11/include.hpp>
#include <ny/windowListener.hpp>
#include <ny/image.hpp>
#include <nytl/stringParam.hpp>

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>

#include <system_error>
#include <vector>
#include <string>

// Needed sice Xlib defines this macro
#ifdef GenericEvent
	#undef GenericEvent
#endif

namespace ny {

/// X11EventData stores the native xcb event for later use.
/// To see where this might be needed look at the X11WC::beginResize and X11WC::beginMove functions.
struct X11EventData : public EventData {
	X11EventData(const xcb_generic_event_t& e) : event(e) {};
	xcb_generic_event_t event;
};

/// Implements std::error_category for x11 protocol errors codes.
/// Needs a Xlib Display to correctly obtain error messages for error codes.
class X11ErrorCategory : public std::error_category {
public:
	X11ErrorCategory() = default;
	X11ErrorCategory(Display&, xcb_connection_t&);
	~X11ErrorCategory() = default;

	X11ErrorCategory(X11ErrorCategory&& other);
	X11ErrorCategory& operator=(X11ErrorCategory&& other);

	const char* name() const noexcept override { return "ny::x11"; }
	std::string message(int code) const override;

	/// Returns an error_code of this category for a valid x11 error value.
	/// Otherwise returns an empty error_code.
	std::error_code errorCode(int error) const;

	/// Checks the given cookie and returns an error_code object of this category if the
	/// cookie contains an error. Otherwise returns an empty error_code.
	std::error_code check(xcb_void_cookie_t cookie) const;

	/// Checks if the cookie contains an error and if so, sets the given error_code and returns
	/// false. Returns true and resets the error code if the cookie is ok.
	bool check(xcb_void_cookie_t, std::error_code&) const;

	/// Checks whether the cookie contains and error and if so, outputs a warning with the
	/// meaning of the error code and the given msg, if any.
	/// Returns false if the cookie contained an error, true otherwise.
	bool checkWarn(xcb_void_cookie_t, nytl::StringParam msg = {}) const;

	/// Checks whether the cookie contains and error and if so, throws a std::system_error
	/// with an error_code for the cookie error and the given msg, if any.
	void checkThrow(xcb_void_cookie_t, nytl::StringParam msg = {}) const;

	Display& xDisplay() const { return *xDisplay_; }
	xcb_connection_t& xConnection() const { return *xConnection_; }

protected:
	Display* xDisplay_ {}; // Needed to obtain the error message
	xcb_connection_t* xConnection_ {};
};

/// Returns the MouseButton enumeration value for the given x11 button id.
/// Note that default x11 does only support 3 buttons for sure, so custom1 and custom2
/// may not be correctly detected. If the button id
/// is not known, MouseButton::unknown is returned.
MouseButton x11ToButton(unsigned int button);

/// Returns the x11 button id for the given MouseButton enumeration value.
/// If the given MouseButton is invalid or has no corresponding x11 button id, 0 is returned.
unsigned int buttonToX11(MouseButton);

namespace x11 {

// Forward declaration dummys, since some xcb types are anonymous typedefs and
// we don't want to pull all xcb headers everwhere.
struct EwmhConnection : public xcb_ewmh_connection_t {};
struct GenericEvent : public xcb_generic_event_t {};

/// All non-predefined atoms that will be used by some backend component.
/// X11AppContext contains an Atoms object and tries to load each of these atoms in
/// the constructor.
struct Atoms {
	xcb_atom_t xdndEnter;
	xcb_atom_t xdndPosition;
	xcb_atom_t xdndStatus;
	xcb_atom_t xdndTypeList;
	xcb_atom_t xdndActionCopy;
	xcb_atom_t xdndActionMove;
	xcb_atom_t xdndActionAsk;
	xcb_atom_t xdndActionLink;
	xcb_atom_t xdndDrop;
	xcb_atom_t xdndLeave;
	xcb_atom_t xdndFinished;
	xcb_atom_t xdndSelection;
	xcb_atom_t xdndProxy;
	xcb_atom_t xdndAware;

	xcb_atom_t clipboard;
	xcb_atom_t targets;
	xcb_atom_t text;
	xcb_atom_t utf8string;
	xcb_atom_t fileName;

	xcb_atom_t wmDeleteWindow;
	xcb_atom_t motifWmHints;

	struct
	{
		xcb_atom_t textPlain; // text/plain
		xcb_atom_t textPlainUtf8; // text/plain;charset=utf8
		xcb_atom_t textUriList; // text/uri-list

		xcb_atom_t imageData; // image/x-ny-data
		xcb_atom_t raw; // application/octet-stream
	} mime;
};

/// Represents a Property value of an x11 window for a property atom.
/// Can be retrieved using readProperty or used to change a property using changeProperty
struct Property {
	std::vector<uint8_t> data; /// The raw property data. Use data.size() to determine the length
	int format = 8; /// How the data should be interpreted, i.e. how many bits are grouped together
	xcb_atom_t type = XCB_ATOM_ANY; /// An atom representing the type of the data
};

/// Reads the given x11 atom property for the given window.
/// Returns an empty property and sets error if an error occurred during the property reading.
/// \param deleteProp Whether the property should be deleted after reading it
Property readProperty(xcb_connection_t&, xcb_atom_t prop, xcb_window_t,
	xcb_generic_error_t* error = nullptr, bool deleteProp = false);

/// Returns an error string for an x11 error code.
std::string errorMessage(Display&, unsigned int error);

/// Tries to convert the given visual description with the given depth to an ImageDataFormat
/// enumeration value. If there is no corresponding ImageDataFormat value, returns
/// imageFormats::none.
ImageFormat visualToFormat(const xcb_visualtype_t& visual, unsigned int depth);

} // namespace x11
} // namespace ny
