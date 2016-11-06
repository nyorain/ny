#include <ny/android/appContext.hpp>
#include <ny/log.hpp>

#ifdef NY_WithVulkan
 #define VK_USE_PLATFORM_ANDROID_KHR
 #include <ny/android/vulkan.hpp>
 #include <vulkan/vulkan.h>
#endif //Vulkan

#ifdef NY_WithEGL
 #include <ny/common/egl.hpp>
 #include <ny/android/egl.hpp>
#endif //GL

namespace ny
{

struct AndroidAppContext::Impl
{
#ifdef NY_WithEGL
	bool eglFailed;
	EglSetup eglSetup;
#endif //WithEGL
};

//AndroidAppContext
AndroidAppContext::AndroidAppContext()
{
	impl_ = std::make_unique<Impl>();
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
	return eglSetup();
}

EglSetup* AndroidAppContext::eglSetup() const
{
	#ifdef NY_WithEGL
		if(impl_->eglFailed) return nullptr;

		if(!impl_->eglSetup.valid())
		{
			try { impl_->eglSetup = {nullptr}; }
			catch(const std::exception& error)
			{
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

}
