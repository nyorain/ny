// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/winapi/backend.hpp>
#include <ny/winapi/appContext.hpp>

namespace ny {

void WinapiBackend::initialize()
{
	static WinapiBackend instance_ {};
}

std::unique_ptr<AppContext> WinapiBackend::createAppContext()
{
	return std::make_unique<WinapiAppContext>();
}

bool WinapiBackend::gl() const
{
	return builtWithGl();
}

bool WinapiBackend::vulkan() const
{
	return builtWithVulkan();
}

} // namespace ny
