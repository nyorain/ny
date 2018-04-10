// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/windowListener.hpp>
#include <dlg/dlg.hpp>

namespace ny {

void WindowListener::surfaceDestroyed(const SurfaceDestroyedEvent&) {
	dlg_error("WindowListener::surfaceDestroyed was not overriden");
}

void WindowListener::surfaceCreated(const SurfaceCreatedEvent&) {
	dlg_error("WindowListener::surfaceCreated was not overriden");
}

} // namespace ny
