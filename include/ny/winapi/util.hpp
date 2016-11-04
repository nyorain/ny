#pragma once

#include <ny/include.hpp>
#include <ny/windowSettings.hpp>
#include <nytl/stringParam.hpp>

#include <string>
#include <system_error>

namespace ny
{

Keycode winapiToKeycode(unsigned int code);
unsigned int keycodeToWinapi(Keycode key);

unsigned int buttonToWinapi(MouseButton button);
MouseButton winapiToButton(unsigned int code);

std::string errorMessage(unsigned int code, const char* msg = nullptr);
std::string errorMessage(const char* msg = nullptr);

//Note: there isnt a string literal returned here. Just a (char?) pointer to some windows
//resource. Better return void pointer or sth...
const char* cursorToWinapi(CursorType type);

unsigned int edgesToWinapi(WindowEdges edges);

///Winapi std::error_category
class WinapiErrorCategory : public std::error_category
{
public:
	static WinapiErrorCategory& instance();
	static std::system_error exception(nytl::StringParam msg = "");

public:
	const char* name() const noexcept override { return "ny::WinapiErrorCategory"; }
	std::string message(int code) const override;
};

namespace winapi
{
	using EC = WinapiErrorCategory;
	std::error_code lastErrorCode();
}

}
