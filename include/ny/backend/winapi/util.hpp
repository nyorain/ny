#pragma once

#include <ny/include.hpp>
#include <string>

namespace ny
{

Key winapiToKey(unsigned int code);
unsigned int keyToWinapi(Key key);

unsigned int buttonToWinapi(MouseButton button);
MouseButton winapiToButton(unsigned int code);

std::string errorMessage(unsigned int code, const char* msg = nullptr);
std::string errorMessage(const char* msg = nullptr);

//Note: not a string literal returned here. Just a (char?) pointer to some windows
//resource. Better return void pointer or sth...
const char* cursorToWinapi(CursorType type);

}
