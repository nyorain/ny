#include <ny/backend/wayland/appContext.hpp>

#include <ny/backend/wayland/util.hpp>
#include <ny/backend/wayland/interfaces.hpp>
#include <ny/backend/wayland/windowContext.hpp>
#include <ny/backend/wayland/input.hpp>
#include <ny/backend/wayland/xdg-shell-client-protocol.h>

#include <ny/base/eventDispatcher.hpp>
#include <ny/base/loopControl.hpp>

#ifdef NY_WithEGL
 #include <ny/backend/wayland/egl.hpp>
#endif //WithGL

#ifdef NY_WithVulkan
 #include <ny/backend/wayland/vulkan.hpp>
#endif //WithVulkan

#include <nytl/scope.hpp>

#include <wayland-client.h>
#include <wayland-cursor.h>
#include <wayland-client-protocol.h>

#include <algorithm>

namespace ny
{

namespace
{

void callbackDestroy(void*, wl_callback* callback, unsigned int) { wl_callback_destroy(callback); }
const wl_callback_listener callbackDestroyListener { &callbackDestroy };

//LoopControl
class WaylandLoopControlImpl : public ny::LoopControlImpl
{
public:
	std::atomic<bool>* run;
	wl_callback* callback;
	wl_display* display;

public:
	WaylandLoopControlImpl(std::atomic<bool>& prun, wl_display& wldisplay)
		: run(&prun), display(&wldisplay)
	{
	}

	virtual void stop() override
	{
		run->store(0);
		callback = wl_display_sync(display);
		wl_callback_add_listener(callback, &callbackDestroyListener, this);
	};
};

}

using namespace wayland;

WaylandAppContext::WaylandAppContext()
{
    wlDisplay_ = wl_display_connect(nullptr);

    if(!wlDisplay_) throw std::runtime_error("ny::WaylandAC: could not connect to display");

    wlRegistry_ = wl_display_get_registry(wlDisplay_);
    wl_registry_add_listener(wlRegistry_, &globalRegistryListener, this);

    wl_display_dispatch(wlDisplay_);
    wl_display_roundtrip(wlDisplay_);

    //compositor added by registry Callback listener
    if(!wlCompositor_) throw std::runtime_error("ny::WaylandAC: could not get compositor");

    wl_callback* syncer = wl_display_sync(wlDisplay_);
    wl_callback_add_listener(syncer, &displaySyncListener, this);
}

WaylandAppContext::~WaylandAppContext()
{
	if(keyboardContext_) keyboardContext_.reset();
	if(mouseContext_) mouseContext_.reset();

	if(wlRegistry_) wl_registry_destroy(wlRegistry_);
    if(wlDisplay_) wl_display_disconnect(wlDisplay_);
}

//TODO: exception safety!
bool WaylandAppContext::dispatchEvents()
{
	for(auto& e : pendingEvents_) if(e->handler) e->handler->handleEvent(*e);
	pendingEvents_.clear();

	auto ret = wl_display_dispatch_pending(wlDisplay_);
	return ret != -1;
}

bool WaylandAppContext::dispatchLoop(LoopControl& control)
{
	auto guard = nytl::makeScopeGuard([&]{ control.impl_.reset(); });

	std::atomic<bool> run {true};
	control.impl_.reset(new WaylandLoopControlImpl(run, *wlDisplay_));

	auto ret = 0;
	while(run && ret != -1)
	{
		for(auto& e : pendingEvents_) if(e->handler) e->handler->handleEvent(*e);
		pendingEvents_.clear();
		ret = wl_display_dispatch(wlDisplay_);
	}

	return ret != -1;
}

bool WaylandAppContext::threadedDispatchLoop(EventDispatcher& dispatcher, LoopControl& control)
{
	std::atomic<bool> run {true};
	control.impl_.reset(new WaylandLoopControlImpl(run, *wlDisplay_));
	dispatcher_ = &dispatcher;

	auto guard = nytl::makeScopeGuard([&]{
		dispatcher_ = nullptr;
		control.impl_.reset();
	});

	//wake the loop up every time an event is dispatched from another thread
	nytl::CbConnGuard conn = dispatcher.onDispatch.add([&]{
		wl_display_roundtrip(wlDisplay_);
	});


	auto ret = 0;
	while(run && ret != -1)
	{
		for(auto& e : pendingEvents_) if(e->handler) e->handler->handleEvent(*e);
		pendingEvents_.clear();

		ret = wl_display_dispatch(wlDisplay_);
		dispatcher.processEvents();
	}

	return ret != -1;
}

KeyboardContext* WaylandAppContext::keyboardContext()
{
	return keyboardContext_.get();
}

MouseContext* WaylandAppContext::mouseContext()
{
	return mouseContext_.get();
}

WindowContextPtr WaylandAppContext::createWindowContext(const WindowSettings& settings)
{
    WaylandWindowSettings waylandSettings;
    const auto* ws = dynamic_cast<const WaylandWindowSettings*>(&settings);

    if(ws) waylandSettings = *ws;
    else waylandSettings.WindowSettings::operator=(settings);

	auto contextType = settings.context;
	if(contextType == ContextType::vulkan)
	{
		#ifdef NY_WithVulkan
		 // if(!vulkanContext_) vulkanContext_ = std::make_unique<WalandVulkanContext>(*this);
		 // return std::make_unique<WaylandVulkanWindowContext>(*xac, settings);
		#else
		 throw std::logic_error("ny::WaylandAC::createWC: ny built without vulkan support");
		#endif
	}
	else if(contextType == ContextType::gl)
	{
		#ifdef NY_WithGL
		 // return std::make_unique<WaylandEglWindowContext>(*this, waylandSettings);
		#else
		 throw std::logic_error("ny::WaylandAC::createWC: ny built without GL suppport");
		#endif
	}

	return std::make_unique<WaylandWindowContext>(*this, waylandSettings);
}

bool WaylandAppContext::clipboard(std::unique_ptr<DataSource>&& dataSource)
{
}
std::unique_ptr<DataOffer> WaylandAppContext::clipboard()
{
}
bool WaylandAppContext::startDragDrop(std::unique_ptr<DataSource>&& dataSource)
{
}

void WaylandAppContext::registryAdd(unsigned int id, const char* cinterface, unsigned int)
{
	std::string interface = cinterface;
    if(interface == "wl_compositor" && !wlCompositor_)
    {
		auto ptr = wl_registry_bind(wlRegistry_, id, &wl_compositor_interface, 1);
        wlCompositor_ = {static_cast<wl_compositor*>(ptr), id};
    }
    else if(interface == "wl_shell" && !wlShell_)
    {
		auto ptr = wl_registry_bind(wlRegistry_, id, &wl_shell_interface, 1);
        wlShell_ = {static_cast<wl_shell*>(ptr), id};
    }
    else if(interface == "wl_shm" && !wlShm_)
    {
		auto ptr = wl_registry_bind(wlRegistry_, id, &wl_shm_interface, 1);
        wlShm_ = {static_cast<wl_shm*>(ptr), id};
        wl_shm_add_listener(wlShm_, &shmListener, this);

		//TODO
        wlCursorTheme_ = wl_cursor_theme_load("default", 32, wlShm_);
    }
    else if(interface == "wl_subcompositor" && !wlSubcompositor_)
    {
		auto ptr = wl_registry_bind(wlRegistry_, id, &wl_subcompositor_interface, 1);
        wlSubcompositor_ = {static_cast<wl_subcompositor*>(ptr), id};
    }
    else if(interface == "wl_output")
    {
		auto ptr = wl_registry_bind(wlRegistry_, id, &wl_output_interface, 1);
        outputs_.push_back({*this, *static_cast<wl_output*>(ptr), id});
    }
    else if(interface == "wl_data_device_manager" && !wlDataManager_)
    {
		auto ptr = wl_registry_bind(wlRegistry_, id, &wl_data_device_manager_interface, 1);
        wlDataManager_ = {static_cast<wl_data_device_manager*>(ptr), id};
        if(wlSeat_ && !wlDataDevice_)
        {
            wlDataDevice_ = wl_data_device_manager_get_data_device(wlDataManager_, wlSeat_);
            wl_data_device_add_listener(wlDataDevice_, &dataDeviceListener, this);
        }
    }
    else if(interface == "wl_seat" && !wlSeat_)
    {
		auto ptr = wl_registry_bind(wlRegistry_, id, &wl_seat_interface, 1);
        wlSeat_ = {static_cast<wl_seat*>(ptr), id};
        wl_seat_add_listener(wlSeat_, &seatListener, this);

        if(wlDataManager_ && !wlDataDevice_)
        {
            wlDataDevice_ = wl_data_device_manager_get_data_device(wlDataManager_, wlSeat_);
            wl_data_device_add_listener(wlDataDevice_, &dataDeviceListener, this);
        }
    }
    else if(interface == "xdg_shell" && !xdgShell_)
    {
		auto ptr = wl_registry_bind(wlRegistry_, id, &xdg_shell_interface, 1);
        xdgShell_ = {static_cast<xdg_shell*>(ptr), id};
        xdg_shell_add_listener(xdgShell_, &xdgShellListener, this);
    }
}

void WaylandAppContext::outputDone(const Output& out)
{
	std::remove_if(outputs_.begin(), outputs_.end(), [=](const Output& o){ return &o == &out; });
}

void WaylandAppContext::registryRemove(unsigned int id)
{
	//TODO: usually stop the application/main loop here.
	//TODO: check other globals here
	if(id == wlCompositor_.name)
	{
		wl_compositor_destroy(wlCompositor_);
		wlCompositor_ = {};
	}
	else if(id == wlSubcompositor_.name)
	{
		wl_subcompositor_destroy(wlSubcompositor_);
		wlSubcompositor_ = {};
	}
	else if(id == wlShell_.name)
	{
		wl_shell_destroy(wlShell_);
		wlShell_ = {};
	}
	else
	{
		std::remove_if(outputs_.begin(), outputs_.end(),
			[=](const Output& output){ return output.globalID == id; });
	}
}

void WaylandAppContext::seatCapabilities(unsigned int caps)
{
	//TODO: some kind of notification or warning if no pointer/keyboard
    if ((caps & WL_SEAT_CAPABILITY_POINTER) && !mouseContext_)
    {
		mouseContext_ = std::make_unique<WaylandMouseContext>(*this, *wlSeat_.global);
    }
    else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && mouseContext_)
    {
		mouseContext_.reset();
    }

    if((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !keyboardContext_)
    {
		keyboardContext_ = std::make_unique<WaylandKeyboardContext>(*this, *wlSeat_.global);
    }
    else if(!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && keyboardContext_)
    {
		keyboardContext_.reset();
    }
}

void WaylandAppContext::seatName(const char* name)
{
	seatName_ = name;
}

void WaylandAppContext::addShmFormat(unsigned int format)
{
    shmFormats_.push_back(format);
}

bool WaylandAppContext::shmFormatSupported(unsigned int wlShmFormat)
{
    for(auto format : shmFormats_) if(format == wlShmFormat) return true;
    return false;
}


void WaylandAppContext::cursor(std::string cursorName, unsigned int serial)
{
    //TODO: handle errors/unexpected conditions (at least a warning).
	if(!wlCursorTheme_ || !wlCursorSurface_ || !mouseContext_) return;

    auto* wlcursor = wl_cursor_theme_get_cursor(wlCursorTheme_, cursorName.c_str());
    if(!wlcursor) return;

	//Delete the old buffer image if there is any
    if(cursorImageBuffer_) cursorImageBuffer_ = {};

	//TODO: handle multiple images (animated)
    auto image = wlcursor->images[0];
    if(!image) return;

	//NOTE: the returned buffer is not owned, i.e. it should not be destroyed.
    auto wlCursorBuffer = wl_cursor_image_get_buffer(image);
    if(!wlCursorBuffer) return;

	//only activly change the cursor if there was a serial given.
	//otherwise just update the cursor surface (it must have been set before).
	auto wlpointer = mouseContext_->wlPointer();
	auto hx = image->hotspot_x;
	auto hy = image->hotspot_y;
    if(serial) wl_pointer_set_cursor(wlpointer, serial, wlCursorSurface_, hx, hy);

    wl_surface_attach(wlCursorSurface_, wlCursorBuffer, 0, 0);
    wl_surface_damage(wlCursorSurface_, 0, 0, image->width, image->height);
    wl_surface_commit(wlCursorSurface_);
}

void WaylandAppContext::dispatch(Event&& event)
{
	// if(dispatcher_) dispatcher_->dispatch(std::move(event));
	// else if(event.handler) pendingEvents_.push_back(nytl::cloneMove(event));

	pendingEvents_.push_back(nytl::cloneMove(event));
}

WaylandWindowContext* WaylandAppContext::windowContext(wl_surface& surface) const
{
	auto data = wl_surface_get_user_data(&surface);
	return static_cast<WaylandWindowContext*>(data);
}

/*
void waylandAppContext::setCursor(const image* img, Vec2i hotspot, unsigned int serial)
{
    if(cursorIsCustomImage_)
    {
        if(cursorImageBuffer_) delete cursorImageBuffer_;
    }
    else
    {
        if(wlCursorBuffer_) wl_buffer_destroy(wlCursorBuffer_);
    }
    cursorIsCustomImage_ = 1;

    cursorImageBuffer_ = new wayland::shmBuffer(img->getSize(), bufferFormat::argb8888);
    //TODO: data
    if(serial != 0) wl_pointer_set_cursor(wlPointer_, serial, wlCursorSurface_, hotspot.x, hotspot.y);
    wl_surface_attach(wlCursorSurface_, cursorImageBuffer_->getWlBuffer(), 0, 0);
    wl_surface_damage(wlCursorSurface_, 0, 0, cursorImageBuffer_->getSize().x, cursorImageBuffer_->getSize().y);
    wl_surface_commit(wlCursorSurface_);
}
*/

// void waylandAppContext::startDataOffer(dataSource& source, const image& img, const window& w, const event* ev)
// {

//     if(!wlDataDevice_)
//     {
//         if(!wlSeat_ || !wlDataManager_)
//             return;

//         wlDataDevice_ = wl_data_device_manager_get_data_device(wlDataManager_, wlSeat_);
//     }

//     if(!dataIconBuffer_)
//     {
//         dataIconBuffer_ = new shmBuffer(img.getSize());
//     }
//     else
//     {
//         dataIconBuffer_->setSize(img.getSize());
//     }

//     waylandWindowContext* wwc = dynamic_cast<waylandWindowContext*>(w.getWindowContext());
//     dataSourceSurface_ = wwc->getWlSurface();

//     unsigned char* buffData = (unsigned char*) dataIconBuffer_->getData();
//     unsigned char imgData[img.getBufferSize()];

//     img.getData(imgData);

//     for(unsigned int i(0); i < img.getBufferSize(); i++)
//     {
//         buffData[i] = imgData[i];
//         //buffData[i] = 0x66;
//     }

//     if(!dataIconSurface_)
//     {
//         dataIconSurface_ = wl_compositor_create_surface(wlCompositor_);

//     }

// 	dataSource_ = &source;

// 	wlDataSource_ = wl_data_device_manager_create_data_source(wlDataManager_);
// 	wl_data_source_add_listener(wlDataSource_, &dataSourceListener, &source);

//     std::vector<std::string> Vec = dataTypesToString(source.getPossibleTypes(), 1); //only mime
// 	for(size_t i(0); i < Vec.size(); i++)
//         wl_data_source_offer(wlDataSource_, Vec[i].c_str());

//     waylandEventData* data;
//     if(!ev->data || !(data = dynamic_cast<waylandEventData*>(ev->data.get())))
//     {
//         return;
//     }

//     wl_data_device_start_drag(wlDataDevice_, wlDataSource_, dataSourceSurface_, dataIconSurface_, data->serial);

//     wl_surface_attach(dataIconSurface_, dataIconBuffer_->getWlBuffer(), 0, 0);
//     wl_surface_damage(dataIconSurface_, 0, 0, img.getSize().x, img.getSize().y);
//     wl_surface_commit(dataIconSurface_);
// }

// bool waylandAppContext::isOffering() const
// {
// 	return (dataSource_ != nullptr);
// }

// void waylandAppContext::endDataOffer()
// {
//     if(isOffering())
//     {
//         //TODO
//     }
// }

// dataOffer* waylandAppContext::getClipboard()
// {
//     return nullptr;
// }

// void waylandAppContext::setClipboard(ny::dataSource& source, const event* ev)
// {

// }


}
