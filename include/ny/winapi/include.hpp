// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/fwd.hpp>
#include <ny/config.hpp>

#ifndef NY_WithWinapi
	#error ny was built without winapi. Do not include this header.
#endif

namespace ny {
	class WinapiWindowContext;
	class WinapiAppContext;
	class WinapiBufferSurface;
	class WglContext;
	class WglSetup;
	class WglSurface;
	class WinapiVulkanWindowContext;
	class WinapiBufferWindowContext;
	class WinapiEventData;
	class WinapiDataOffer;

	namespace winapi::com {
		class DropTargetImpl;
		class DropSourceImpl;
		class DataObjectImpl;
	}
}
