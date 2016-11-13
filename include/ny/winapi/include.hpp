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
	class WglContext;
	class WglSetup;
	class WglSurface;
	class WinapiVulkanWindowContext;
	class WinapiBufferWindowContext;

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
