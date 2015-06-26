#pragma once

#include <windows.h>
#include <gdiplus.h>

#include "../include/include.hpp"

using namespace Gdiplus;

namespace ny
{

    Color colorToWinapi(const color& col);

}
