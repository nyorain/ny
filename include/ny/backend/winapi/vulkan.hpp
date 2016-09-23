#pragma once

#include <ny/backend/winapi/include.hpp>
#include <ny/backend/winapi/windowContext.hpp>
#include <ny/backend/common/vulkan.hpp>

namespace ny
{

class VulkanWinapiWindowContext : public WinapiWindowContext
{
public:
	VulkanWinapiWindowContext(WinapiAppContext& ac, WindowSettings& ws);
	~VulkanWinapiWindowContext();

protected:
	// std::unique_ptr<evg::VulkanDrawContext> drawContext_;
	// std::unique_ptr<VulkanSurfaceContext> surfaceContext_;
};

}
