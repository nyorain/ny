//This example only creates a window with vulkan surface
//Feel free, dear reader, to write an example (using no extra external dependency like
//nyorain/vpp!) that also demonstrates real vulkan rendering into this window.
//Is like really much work.

#include <ny/base.hpp>
#include <ny/backend.hpp>
#include <vulkan/vulkan.h>

class MyEventHandler : public ny::EventHandler
{
public:
	MyEventHandler(ny::LoopControl& mainLoop, ny::WindowContext& wc)
		: loopControl_(mainLoop), wc_(wc) {}

	bool handleEvent(const ny::Event& ev) override
	{
		ny::debug("Received event with type ", ev.type());

		if(ev.type() == ny::eventType::close)
		{
			ny::debug("Window closed. Exiting.");
			loopControl_.stop();
			return true;
		}
		else if(ev.type() == ny::eventType::key)
		{
			if(!static_cast<const ny::KeyEvent&>(ev).pressed) return false;

			ny::debug("Key pressed. Exiting.");
			loopControl_.stop();
			return true;
		}

		return false;
	};

protected:
	ny::LoopControl& loopControl_;
	ny::WindowContext& wc_;
};

VkInstance createInstance(ny::AppContext& ac);

int main()
{
	auto& backend = ny::Backend::choose();
	auto ac = backend.createAppContext();

	//create the vulkan instance
	VkInstance vkInstance = createInstance(*ac);
	if(!vkInstance)
	{
		ny::warning("Failed to create the vulkan instance");
		return EXIT_FAILURE;
	}

	VkSurfaceKHR vkSurface {};

	//specify to create a vulkan surface for the created instance and store this surface
	//in vkSurface
	ny::WindowSettings settings;
	settings.context = ny::ContextType::vulkan;
	settings.vulkan.instance = vkInstance;
	settings.vulkan.storeSurface = &vkSurface;
	auto wc = ac->createWindowContext(settings);

	//here, vkSurface could now be used to create a swapchain and render into it.
	//But since this requires MUCH code^H^H^H^H^H^HThis was left out here intentionally as an
	//exercise for the reader.
	ny::debug("The create vulkan surface: ", vkSurface);

	ny::LoopControl control;
	MyEventHandler handler(control, *wc);

	wc->eventHandler(handler);
	wc->refresh();

	ny::debug("Entering main loop");
	ac->dispatchLoop(control);
}

VkInstance createInstance(ny::AppContext& ac)
{
	auto ext = ac.vulkanExtensions();

	VkApplicationInfo appInfo {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "ny-vulkan";
	appInfo.pEngineName = "*";
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo info {};
	info.enabledExtensionCount = ext.size();
	info.ppEnabledExtensionNames = ext.data();
	info.pApplicationInfo = &appInfo;

	VkInstance ret;
	vkCreateInstance(&info, nullptr, &ret);
	return ret;
}
