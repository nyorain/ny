// Copyright (c) 2016 nyorain
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

Keycode winapiToKeycode(unsigned int code);
unsigned int keycodeToWinapi(Keycode key);

unsigned int buttonToWinapi(MouseButton button);
MouseButton winapiToButton(unsigned int code);

std::string errorMessage(unsigned int code, const char* msg = nullptr);
std::string errorMessage(const char* msg = nullptr);

// Note: there isnt a string literal returned here. Just a (char?) pointer to some windows
// resource. Better return void pointer or sth...
const wchar_t* cursorToWinapi(CursorType type);
CursorType winapiToCursor(const wchar_t* idc);

unsigned int edgesToWinapi(WindowEdges edges);
WindowEdge winapiToEdges(unsigned int winapiCode);

/// Converts between (ny,application space) utf8-string and (winapi space) utf16-wstring.
std::wstring widen(const std::string&);
std::string narrow(const std::wstring&);

/// Winapi EventData. Carries the winapi message.
struct WinapiEventData : public EventData {
	WinapiWindowContext* windowContext {};
	HWND window {};
	UINT message {};
	WPARAM wparam {};
	LPARAM lparam {};
};

/// Winapi std::error_category implementation
class WinapiErrorCategory : public std::error_category {
public:
	static WinapiErrorCategory& instance();
	static std::system_error exception(nytl::StringParam msg = "");

public:
	const char* name() const noexcept override { return "ny::winapi"; }
	std::string message(int code) const override;
};

namespace winapi {

using EC = WinapiErrorCategory;
std::error_code lastErrorCode();

/// Converts between a ny::ImageData object and a native winapi bitmap.
/// The returned HBITMAP is owned and must be freed.
HBITMAP toBitmap(const Image&);
UniqueImage toImage(HBITMAP bitmap);

} // namespace winapi
} // namespace ny
