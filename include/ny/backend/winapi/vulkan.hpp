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
};

}
