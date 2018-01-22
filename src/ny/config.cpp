// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/config.hpp>

namespace ny {

bool builtWithAndroid()
{
	#ifdef NY_WithAndroid
		return true;
	#else
		return false;
	#endif
}

bool builtWithWayland()
{
	#ifdef NY_WithWayland
		return true;
	#else
		return false;
	#endif
}

bool builtWithWinapi()
{
	#ifdef NY_WithWinapi
		return true;
	#else
		return false;
	#endif
}

bool builtWithX11()
{
	#ifdef NY_WithX11
		return true;
	#else
		return false;
	#endif
}

bool builtWithXkbcommon()
{
	#ifdef NY_WithXkbcommon
		return true;
	#else
		return false;
	#endif
}

bool builtWithGl()
{
	#ifdef NY_WithGl
		return true;
	#else
		return false;
	#endif
}

bool builtWithEgl()
{
	#ifdef NY_WithEgl
		return true;
	#else
		return false;
	#endif
}

bool builtWithVulkan()
{
	#ifdef NY_WithVulkan
		return true;
	#else
		return false;
	#endif
}

unsigned int majorVersion() { return NY_VMajor; }
unsigned int minorVersion() { return NY_VMinor; }
unsigned int patchVersion() { return NY_VPatch; }
unsigned int version() { return NY_Version; }

} // namespace ny
