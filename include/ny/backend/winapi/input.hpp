#pragma once

#include <ny/backend/winapi/include.hpp>
#include <ny/backend/mouse.hpp>
#include <ny/backend/keyboard.hpp>

namespace ny
{

///Winapi MouseContext implementation.
class WinapiMouseContext : public MouseContext
{
};


///Winapi KeyboardContext implementation.
class WinapiKeyboardContext : public KeyboardContext
{
};

}
