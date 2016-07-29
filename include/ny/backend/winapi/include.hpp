#pragma once

#include <ny/include.hpp>

#ifndef NY_WithWinapi
	#error ny was built without winapi. Do not include this header.
#endif

namespace ny
{
    class WinapiWindowContext;
    class WinapiAppContext;
    class GdiDrawContext;
    class WglContext;
}
