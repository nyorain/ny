// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/fwd.hpp>
#include <ny/config.hpp>

#ifndef NY_WithAndroid
	#error ny was built without android. Do not include this header file!
#endif //Android

namespace ny {
	class AndroidBackend;
	class AndroidAppContext;
	class AndroidWindowContext;
	class AndroidBufferSurface;
}
