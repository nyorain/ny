#pragma once

#include <windows.h>
#include <gdiplus.h>

#include <ny/include.hpp>

using namespace Gdiplus;

namespace ny
{

    Color colorToWinapi(const color& col);

}
