// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/android/appContext.hpp>
#include <ny/log.hpp>

#ifdef NY_WithVulkan
 #define VK_USE_PLATFORM_ANDROID_KHR
 #include <ny/android/vulkan.hpp>
 #include <vulkan/vulkan.h>
#endif // Vulkan

#ifdef NY_WithEgl
 #include <ny/common/egl.hpp>
 #include <ny/android/egl.hpp>
#endif // Egl

namespace ny {

struct AndroidAppContext::Impl {
// #ifdef NY_WithEgl
// 	bool eglFailed;
// 	EglSetup eglSetup;
// #endif //WithEGL
};

// AndroidAppContext
AndroidAppContext::AndroidAppContext()
{
	impl_ = std::make_unique<Impl>();
}

AndroidAppContext::~AndroidAppContext()
{

}

WindowContextPtr AndroidAppContext::createWindowContext(const WindowSettings& ws)
{
	if(!acitivty_.window())
		throw std::runtime_error("ny::AndroidAppContext::createWindowContext: no window");
}

bool AndroidAppContext::dispatchEvents()
{

}

bool AndroidAppContext::dispatchLoop(LoopControl& loopControl)
{

}

std::vector<const char*> AndroidAppContext::vulkanExtensions() const
{
	#ifdef NY_WithVulkan
		return {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_ANDOIRD_SURFACE_EXTENSION_NAME};
	#else
		return {};
	#endif
}

GlSetup* AndroidAppContext::glSetup() const
{
	#ifdef NY_WithEgl
		return eglSetup();
	#else
		return nullptr;
	#endif // WithEgl
}

EglSetup* AndroidAppContext::eglSetup() const
{
	#ifdef NY_WithEgl
		if(impl_->eglFailed) return nullptr;

		if(!impl_->eglSetup.valid()) {
			try { impl_->eglSetup = {nullptr}; }
			catch(const std::exception& error) {
				warning("WaylandAppContext::eglSetup: creating failed: ", error.what());
				impl_->eglFailed_ = true;
				impl_->eglFailed_ = {};
				return nullptr;
			}
		}

		return &impl_->eglSetup;

	#else
		return nullptr;
	#endif
}

} // namespace ny
