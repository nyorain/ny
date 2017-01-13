// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/winapi/include.hpp>
#include <ny/winapi/windows.hpp>
#include <ny/windowSettings.hpp>
#include <ny/windowListener.hpp>
#include <nytl/stringParam.hpp>

#include <string>
#include <system_error>

namespace ny {

/// Converts a winapi virtual keycode to a ny::Keycode value.
/// Returns Keycode::unknown for unsupported/invalid keycodes.
Keycode winapiToKeycode(unsigned int vkcode);

/// Converts a ny::Keycode into the associated winapi virtual keycode.
/// If there is none (there are more ny::Keycodes than winapi virtual keycodes), 0 is returned.
unsigned int keycodeToWinapi(Keycode key);

/// Returns the winapi virtual keycode for a given ny::MouseButton value.
/// Note than mouse buttons are decoded as keycodes in winapi, this returned e.g. VK_RBUTTON.
/// Returns 0 for a MouseButton that has associated winapi keycode.
unsigned int buttonToWinapi(MouseButton button);

/// Returns the ny::MouseButton for a given winapi virtual keycode for a mouse button.
/// Returns MouseButton::none if the winapi button is not known or invalid (i.e. it is not
/// a button keycode).
MouseButton winapiToButton(unsigned int code);

/// Typedef for native winapi cursor ids.
/// Note that they are NOT string literals, winapi just used wchar_t pointer to
/// encode them. Examples for winapi cursor ids are e.g. IDC_ARROW.
using WinapiCursorID = const wchar_t*;

/// Returns a native winapi cursor id for the given CursorType.
/// Returns nullptr for unsupported cursor types.
/// Note that this does not return a string literal, but just a (rather meaningless) id.
/// Example: cursorToWinapi(CursorType::hand) returns IDC_HAND
WinapiCursorID cursorToWinapi(CursorType type);

/// Returns a CursorType for a winapi cursor id.
/// Returns CursorType::none for unknown/invalid cursor ids.
CursorType winapiToCursor(WinapiCursorID idc);

/// Conerts the gvien WindowEdges to winapi edges as used by SC_SIZE.
unsigned int edgesToWinapi(WindowEdges edges);

/// Conerts the gvien winapi window edge code as used by SC_SIZE to their corresponding
/// WindowEdge enum value.
WindowEdge winapiToEdges(unsigned int winapiCode);

/// Converts between a (ny, application space) utf8-string and a (winapi space) utf16-wstring.
/// Use this only directly before calling a unicode winapi function, stored and passed strings
/// should still use utf8 encoding.
std::wstring widen(const std::string&);

/// Converts between a (winapi space) utf16-wstring and a (ny, application space) utf8-string.
/// Use this instantly after receiving a wide string from a winapi function or callback.
std::string narrow(const std::wstring&);

/// Winapi EventData. Carries the raw winapi message and the associated WinapiWindowContext,
/// if there is any.
/// \note The WindowContext may already be destroyed, so it should usually not be deferenced
/// if it is not already known in some way.
struct WinapiEventData : public EventData {
	WinapiWindowContext* windowContext {};
	HWND window {};
	UINT message {};
	WPARAM wparam {};
	LPARAM lparam {};
};

/// Winapi std::error_category implementation
/// Can be instantiated without any additional parameters, since all winapi error
/// state is global.
class WinapiErrorCategory : public std::error_category {
public:
	static WinapiErrorCategory& instance();
	static std::system_error exception(nytl::StringParam msg = {});

public:
	const char* name() const noexcept override { return "ny::winapi"; }
	std::string message(int code) const override;
};

namespace winapi {

/// Returns a std::error_code for the last winapi error that occurred.
/// Uses WinapiErrorCategory to create the error code.
/// Always returns an error code, i.e. does not check if GetLastError really returns
/// an error.
std::error_code lastErrorCode();

/// Creates a std::system_error exception for the last winapi error that ocurred.
/// Uses WinapiErrorCategory.
/// Always returns a system_error with a valid error code, i.e. does not check if GetLastError
/// really returns an error.
/// \param msg A message to add into the system error. Can be empty
std::system_error lastErrorException(nytl::StringParam msg = {});

/// Converts between a ny::Imageobject and a native winapi bitmap.
/// The returned HBITMAP is owned and must be freed.
HBITMAP toBitmap(const Image&);

/// Extracts the pixel data from the given winapi bitmap into an owned image.
UniqueImage toImage(HBITMAP bitmap);

/// Returns an error message for the given winapi error code.
/// If msg is not nullptr, starts the error message with it and appends a ":".
std::string errorMessage(unsigned int code, nytl::StringParam msg = {});

/// Returns an error message for the last winapi error code (using GetLastError).
/// If msg is not nullptr, starts the error message with it and appends a ":".
std::string errorMessage(nytl::StringParam msg = {});

} // namespace winapi
} // namespace ny
