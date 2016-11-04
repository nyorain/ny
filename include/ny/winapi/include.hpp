#pragma once

#include <ny/include.hpp>

#ifndef NY_WithWinapi
	#error ny was built without winapi. Do not include this header.
#endif

namespace ny
{
    class WinapiWindowContext;
    class WinapiAppContext;
    class WinapiBufferSurface;

	#ifdef NY_WithGL
     class WglContext;
	#endif //GL

	#ifdef NY_WithVulkan
     class WinapiVulkanWindowContext;
	#endif //GL

	namespace winapi
	{
		namespace com
		{
			class DropTargetImpl;
			class DropSourceImpl;
			class DataObjectImpl;
		}

		class DataOfferImpl;
	}
}
