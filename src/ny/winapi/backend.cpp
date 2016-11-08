#include <ny/winapi/backend.hpp>
#include <ny/winapi/appContext.hpp>

namespace ny
{

WinapiBackend WinapiBackend::instance_;

std::unique_ptr<AppContext> WinapiBackend::createAppContext()
{
	return std::make_unique<WinapiAppContext>();
}

bool X11Backend::gl() const
{
	return builtWithGl();
}

bool X11Backend::vulkan() const
{
	return builtWithVulkan();
}

}
