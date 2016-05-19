#pragma once

#include <ny/app/keyboard.hpp>

namespace ny
{

Keyboard::Key winapiToKey(unsigned int code);
unsigned int keyToWinapi(Keyboard::Key key);

}
