#pragma once

#include <ny/app/keyboard.hpp>

namespace ny
{

Keyboard::Key winapiToKey(unsigned int code);
unsigned int keyToWinapi(Keyboard::Key key);

std::string errorMessage(unsigned int code, const char* msg = nullptr);
std::string errorMessage(const char* msg = nullptr);

}
