// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/config.hpp>
#include <ny/backend.hpp>
#include <dlg/dlg.hpp>
#include <cstdlib> // std::getenv
#include <cstring> // std::strcmp

#ifdef NY_WithWinapi
 #include <ny/winapi/backend.hpp>
#endif

#ifdef NY_WithX11
 #include <ny/x11/backend.hpp>
#endif

#ifdef NY_WithWayland
 #include <ny/wayland/backend.hpp>
#endif

#ifdef NY_WithAndroid
 #include <ny/android/backend.hpp>
#endif

namespace ny {

std::vector<Backend*> Backend::backendsFunc(Backend* reg, bool remove)
{
	static std::vector<Backend*> backends_;

	if(reg) {
		if(remove) {
			for(auto it = backends_.cbegin(); it < backends_.cend(); ++it) {
				if(*it == reg) {
					backends_.erase(it);
					break;
				}
			}
		} else {
			backends_.push_back(reg);
		}
	} // TODO: else warn?

	return backends_;
}

Backend& Backend::choose()
{
	// make sure all backends are initialized
	// we explicitly don't rely on static initialization for built-in
	// backends anymore since this causes issues when statically linkin
	// and is - stricly speaking - undefined behvaiour.
#ifdef NY_WithWinapi
	WinapiBackend::initialize();
#endif

#ifdef NY_WithX11
	X11Backend::initialize();
#endif

#ifdef NY_WithWayland
	WaylandBackend::initialize();
#endif

    // TODO
// #ifdef NY_WithAndroid
// 	AndroidBackend::initialize();
// #endif

	// choose the backend
	dlg_tags("Backend", "choose");

	static const std::string waylandString = "wayland";
	static const std::string x11String = "x11";
	static const std::string winapiString = "winapi";

	auto* envBackend = std::getenv("NY_BACKEND");
    if(envBackend) {
        dlg_debug("NY_BACKEND was set to {}", envBackend);
    }

	auto bestScore = -1;
	auto envFound = false;
	Backend* best = nullptr;

	for(auto& backend : backends()) {
		if(!backend->available()) {
			if(envBackend && std::strcmp(backend->name(), envBackend) == 0) {
				dlg_warn("requested NY_BACKEND '{}' (env) found, not available", envBackend);
				envFound = true;
			}

			continue;
		}

		if(envBackend && !std::strcmp(backend->name(), envBackend)) {
            dlg_warn("Using available NY_BACKEND backend {}", envBackend);
			return *backend;
        }

		// score is chosen this way since there might be x servers on winapi
		// but no winapi on linux and we always want the native backend
		// wayland > x11 because of Xwayland
		auto score = 0;
		if(backend->name() == winapiString) score = 3;
		else if(backend->name() == waylandString) score = 2;
		else if(backend->name() == x11String) score = 1;

		if(score > bestScore) {
			bestScore = score;
			best = backend;
		}
	}

	if(envBackend && !envFound) {
		dlg_warn("requested env NY_BACKEND {} not found", envBackend);
	}

	if(!best) {
		throw std::runtime_error("ny::Backend: no backend available");
	}

	return *best;
}

} // namespace ny
