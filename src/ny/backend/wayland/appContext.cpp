#include <ny/backend/wayland/appContext.hpp>

#include <ny/backend/wayland/util.hpp>
#include <ny/backend/wayland/interfaces.hpp>
#include <ny/backend/wayland/windowContext.hpp>
#include <ny/backend/wayland/xdg-shell-client-protocol.h>

#include <nytl/misc.hpp>

#include <wayland-client.h>
#include <wayland-cursor.h>
#include <wayland-client-protocol.h>

#include <cassert>

namespace ny
{

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
	if(wlRegistry_) wl_registry_destroy(wlRegistry_);
    if(wlDisplay_) wl_display_disconnect(wlDisplay_);
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
        outputs_.push_back({*static_cast<wl_output*>(ptr)});
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
}

void WaylandAppContext::seatCapabilities(unsigned int caps)
{
	//TODO: some kind of notification or warning if no pointer/keyboard
    if ((caps & WL_SEAT_CAPABILITY_POINTER))
    {
        wlPointer_ = wl_seat_get_pointer(wlSeat_);
        wl_pointer_add_listener(wlPointer_, &pointerListener, this);
    }
    else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && wlPointer_)
    {
        wl_pointer_destroy(wlPointer_);
        wlPointer_ = nullptr;
    }

    if ((caps & WL_SEAT_CAPABILITY_KEYBOARD))
    {
        wlKeyboard_ = wl_seat_get_keyboard(wlSeat_);
        wl_keyboard_add_listener(wlKeyboard_, &keyboardListener, this);
    }
    else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && wlKeyboard_)
    {
        wl_keyboard_destroy(wlKeyboard_);
        wlKeyboard_ = nullptr;
    }
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

void WaylandAppContext::mouseMove(unsigned int time, const Vec2ui& pos)
{
	unused(time);

    auto delta = pos - mouse::getPosition();
    nyMainApp()->mouseMove(make_unique<mouseMoveEvent>(nullptr, pos, Vec2i(), delta));
}

void WaylandAppContext::mouseEnterSurface(unsigned int serial, wl_surface& surf, const Vec2ui& pos)
{
    void* data = wl_surface_get_user_data(surface);
    if(!data) return;

    auto handler = &(static_cast<waylandWC*>(data))->getWindow();
    auto pos = Vec2i(wl_fixed_to_int(sx), wl_fixed_to_int(sy));

    nyMainApp()->mouseCross(make_unique<mouseCrossEvent>(handler, 1, pos, new waylandEventData(serial)));
}

void WaylandAppContext::mouseLeaveSurface(unsigned int serial, wl_surface& surface)
{
    void* data = wl_surface_get_user_data(surface);
    if(!data) return;

    auto handler = &(static_cast<waylandWC*>(data))->getWindow();

    nyMainApp()->mouseCross(make_unique<mouseCrossEvent>(handler, 0, Vec2i(), new waylandEventData(serial)));
}

void WaylandAppContext::mouseButton(unsigned int serial, unsigned int time, unsigned int button, 
	bool pressed)
{
    auto buttn = waylandToButton(button);
    nyMainApp()->mouseButton(make_unique<mouseButtonEvent>(nullptr, buttn, static_cast<bool>(state), mouse::getPosition(), new waylandEventData(serial)));
}

void WaylandAppContext::mouseAxis(unsigned int time, unsigned int axis, int value)
{
    nyMainApp()->sendEvent(make_unique<mouseWheelEvent>(nullptr, value));
}

//keyboard
void WaylandAppContext::keyboardKeymap(unsigned int format, int fd, unsigned int size)
{
    //internal
}

void WaylandAppContext::eyboardEnterSurface(unsigned int serial, wl_surface& surf, wl_array& keys)
{
    void* data = wl_surface_get_user_data(surface);
    if(!data) return;

    auto handler = &(static_cast<waylandWC*>(data))->getWindow();

    nyMainApp()->windowFocus(make_unique<focusEvent>(handler, 1, new waylandEventData(serial)));
}

void WaylandAppContext::keyboardLeaveSurface(unsigned int serial, wl_surface& surface)
{
    void* data = wl_surface_get_user_data(surface);
    if(!data) return;

    auto handler = &(static_cast<waylandWC*>(data))->getWindow();

    nyMainApp()->windowFocus(make_unique<focusEvent>(handler, 0, new waylandEventData(serial)));
}

void WaylandAppContext::keyboardKey(unsigned int serial, unsigned int time, unsigned int key, 
	bool state)
{
    auto ky = waylandToKey(key);
    nyMainApp()->keyboardKey(make_unique<keyEvent>(nullptr, ky, static_cast<bool>(state), new waylandEventData(serial)));
}

void WaylandAppContext::keyboardModifiers(unsigned int serial, unsigned int mdepressed, 
	unsigned int mlatched, unsigned int mlocked, unsigned int group)
{
}

void waylandAppContext::eventWindowResized(wl_shell_surface* shellSurface, unsigned int edges, unsigned int width, unsigned int height)
{
    if(!wl_shell_surface_get_user_data(shellSurface))
    {
        //warning
        return;
    }

    auto handler = &static_cast<waylandWindowContext*>(wl_shell_surface_get_user_data(shellSurface))->getWindow();
    auto size = Vec2ui(width, height);

    nyMainApp()->sendEvent(make_unique<sizeEvent>(handler, size, 1));
}

void waylandAppContext::cursor(std::string cursorName, unsigned int serial)
{
    //TODO: handle errors/unexpected conditions (at least a warning).
	if(!wlCursorTheme_) return;

    auto* wlcursor = wl_cursor_theme_get_cursor(wlCursorTheme_, curse.c_str());
    if(!curs || !wlCursorSurface_ || !wlCursorBuffer_) return;

	//Delete the old buffer image if there is any
    if(customCursorImage_) cursorImageBuffer_.~std::unique_ptr<ShmBuffer>();

    customCursorImage_ = false;
	wlCursorBuffer_ = nullptr; //if we exit early

	//TODO: handle multiple images (animated)
    auto image = curs->images[0];
    if(!image) return;

	//NOTE: the returned buffer is not owned, i.e. it should not be destroyed.
    wlCursorBuffer_ = wl_cursor_image_get_buffer(image);
    if(!wlCursorBuffer_) return;

	//only activly change the cursor if there was a serial given.
	//otherwise just update the cursor surface (it must have been set before).
	auto hx = image->hotspot_x;
	auto hy = image->hotspot_y;
    if(serial) wl_pointer_set_cursor(wlPointer_, serial, wlCursorSurface_, hx, hy);

    wl_surface_attach(wlCursorSurface_, wlCursorBuffer_, 0, 0);
    wl_surface_damage(wlCursorSurface_, 0, 0, image->width, image->height);
    wl_surface_commit(wlCursorSurface_);
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
