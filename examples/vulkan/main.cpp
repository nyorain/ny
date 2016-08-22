#include <ny/base.hpp>
#include <ny/backend.hpp>
#include <ny/app/events.hpp>

#include <vpp/backend/win32.hpp>
#include <vpp/context.hpp>
#include <vpp/queue.hpp>
#include <vpp/renderPass.hpp>
#include <vpp/renderer.hpp>
#include <vpp/vk.hpp>

///This simple example shows how to integrate raw ny (without created vulkan context)
///with vulkan via vpp.
///This example does only work for win32, but there is only one platform dependent call.

class RendererImpl : public vpp::RendererBuilder
{
	void build(unsigned int id, const vpp::RenderPassInstance& ini) override {}
	std::vector<vk::ClearValue> clearValues(unsigned int id) override
	{
		std::vector<vk::ClearValue> ret(2, vk::ClearValue{});
		ret[0].color = {{0.5f, 0.5f, 0.5f, 0.5f}};
		ret[1].depthStencil = {1.f, 0};
		return ret;
	}
};

int main()
{
	auto& backend = ny::Backend::choose();
	auto ac = backend.createAppContext();

	ny::WindowSettings settings;
	settings.title = "Example ny-vulkan";
	auto wc = ac->createWindowContext(settings);

	///This is the only platform dependent call.
	///Note that this function was included with vpp/backend/win32.hpp
	auto context = vpp::createContext((HWND)wc->nativeHandle().pointer());

	auto rpinfo = vpp::defaultRenderPassCreateInfo(context.swapChain().format());
	auto renderPass = vpp::RenderPass(context.device(), rpinfo);
	auto rendererInfo = vpp::SwapChainRenderer::CreateInfo();
	rendererInfo.queueFamily = context.graphicsComputeQueue()->family();
	rendererInfo.renderPass = renderPass;
	rendererInfo.attachments = {{vpp::ViewableImage::defaultDepth2D()}};

	auto impl = std::make_unique<RendererImpl>();
	auto renderer = vpp::SwapChainRenderer(context.swapChain(), rendererInfo, std::move(impl));
	renderer.record();

	ny::EventDispatcher dispatcher;
	while(ac->dispatchEvents(dispatcher)) renderer.renderBlock();
}
