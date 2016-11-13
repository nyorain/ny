#include <ny/config.hpp>

namespace ny
{

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

}
