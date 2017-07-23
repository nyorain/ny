// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/x11/backend.hpp>
#include <ny/x11/appContext.hpp>
#include <xcb/xcb.h>

namespace ny {

void X11Backend::initialize()
{
	static X11Backend instance_;
}

bool X11Backend::available() const
{
	auto connection = xcb_connect(nullptr, nullptr);
	bool ret = xcb_connection_has_error(connection);
	xcb_disconnect(connection);
	return !ret;
}

std::unique_ptr<AppContext> X11Backend::createAppContext()
{
	return std::make_unique<X11AppContext>();
}

bool X11Backend::gl() const
{
	return builtWithGl();
}

bool X11Backend::vulkan() const
{
	return builtWithVulkan();
}

} // namespace ny
