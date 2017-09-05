#include <ny/ny.hpp>
#include <vulkan/vulkan.h>

//This example only creates a window with vulkan surface
//It does not draw on the window with vulkan since it requires really much code to write that
//it totally not needed here.

class MyEventHandler : public ny::EventHandler
{
public:
	MyEventHandler(ny::LoopControl& mainLoop, ny::WindowContext& wc) : lc_(mainLoop), wc_(wc) {}
	bool handleEvent(const ny::Event& ev) override;

protected:
	ny::LoopControl& lc_;
	ny::WindowContext& wc_;
};

VkInstance createInstance(ny::AppContext& ac);

int main()
{
	auto& backend = ny::Backend::choose();
	if(!backend.vulkan())
	{
		ny::error("The ny library on this system was built without vulkan support!");
		return EXIT_FAILURE;
	}

	auto ac = backend.createAppContext();

	//create the vulkan instance
	VkInstance vkInstance = createInstance(*ac);
	if(!vkInstance)
	{
		ny::error("Failed to create the vulkan instance");
		return EXIT_FAILURE;
	}

	VkSurfaceKHR vkSurface {};

	//specify to create a vulkan surface for the created instance and store this surface
	//in vkSurface
	ny::WindowSettings settings;
	settings.surface = ny::SurfaceType::vulkan;
	settings.vulkan.instance = vkInstance;
	settings.vulkan.storeSurface = reinterpret_cast<std::uintptr_t*>(&vkSurface);
	auto wc = ac->createWindowContext(settings);

	//here, vkSurface could now be used to create a swapchain and render into it.
	//This is really much to write so^W^W^W^W^W^W^WThis was left out here intentionally as an
	//exercise for the reader.
	ny::log("The created vulkan surface: ", vkSurface);

	ny::LoopControl control;
	MyEventHandler handler(control, *wc);

	wc->eventHandler(handler);
	wc->refresh();

	ny::log("Entering main loop");
	ac->dispatchLoop(control);
}

VkInstance createInstance(ny::AppContext& ac)
{
	//XXX: Note that when creating the vulkan instance, you have to enable the extensions
	//that are needed by the AppContext to create the vulkan surface.
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

	VkInstance ret {};
	vkCreateInstance(&info, nullptr, &ret);
	return ret;
}

bool MyEventHandler::handleEvent(const ny::Event& ev)
{
	ny::log("Received event with type ", ev.type());

	if(ev.type() == ny::eventType::close)
	{
		ny::log("Window closed. Exiting.");
		lc_.stop();
		return true;
	}
	else if(ev.type() == ny::eventType::key)
	{
		if(!static_cast<const ny::KeyEvent&>(ev).pressed) return false;

		ny::log("Key pressed. Exiting.");
		lc_.stop();
		return true;
	}

	return false;
}
