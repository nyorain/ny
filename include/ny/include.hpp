#pragma once

#include <ny/fwd.hpp>
#include <ny/config.hpp>

namespace ny
{

//to be removed in future.
//try to use the nytl namespace prefix everywhere.
using namespace nytl;

//backend typedefs
//XXX: move these and the config.hpp include to fwd.hpp?
#ifdef NY_WithX11
 class X11Backend;
 class X11WindowContext;
 class X11AppContext;
#endif //WithX11

#ifdef NY_WithWayland
 class WaylandBackend;
 class WaylandWindowContext;
 class WaylandAppContext;
#endif //WithWayland

#ifdef NY_WithWinapi
 class WinapiBackend;
 class WinapiWindowContext;
 class WinapiAppContext;
#endif //WithWinapi

#ifdef NY_WithGL
 class GlContext;
#endif //GL

#ifdef NY_WithVulkan
 class VulkanInstance;
 class VulkanSurface;
#endif //Vulkan

}

//XXX: duh...
#ifdef NY_WithVulkan
 #define NY_VK_DEFINE_HANDLE(object) typedef struct object##_T* object;
 #if defined(__LP64__) || \
	defined(_WIN64) || \
	(defined(__x86_64__) && !defined(__ILP32__)) || \
	defined(_M_X64) || \
	defined(__ia64) || \
	defined (_M_IA64) || \
	defined(__aarch64__) || \
	defined(__powerpc64__)
    #define NY_VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef struct object##_T *object;
 #else
	#define NY_VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef std::uint64_t object;
 #endif

 NY_VK_DEFINE_HANDLE(VkInstance);
 NY_VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkSurfaceKHR);
#endif //Vulkan
