#pragma once

#include <ny/backend/wayland/include.hpp>
#include <ny/backend/appContext.hpp>

#include <vector>
#include <string>
#include <memory>

namespace ny
{

namespace wayland
{

///Utility template that allows to associate a numerical name with wayland globals.
template<typename T>
struct NamedGlobal
{
	T* global = nullptr;
	unsigned int name = 0;

	operator T*() const { return global; }
};

}

///Wayland AppContext implementation.
///Holds a wayland display connection as well as all global resurces.
class WaylandAppContext : public AppContext
{
public:
    WaylandAppContext();
    virtual ~WaylandAppContext();

	//AppContext
	bool dispatchEvents(EventDispatcher&) override;
	bool dispatchLoop(EventDispatcher&, LoopControl&) override;
	bool threadedDispatchLoop(ThreadedEventDispatcher&, LoopControl&) override;

	MouseContext* mouseContext() override; 
	KeyboardContext* keyboardContext() override;
	WindowContextPtr createWindowContext(const WindowSettings& windowSettings) override;

    //wayland specific
	///Changes the cursor a cursor with the given name.
	///Note that serial can only be 0 (or invalid) if the cursor was already set by this
	///application of surface entering.
    void cursor(std::string cursorName, unsigned int serial = 0);

	///Changes the cursor to the content of the given image with the given hotspot.
    void cursor(const Image* img, const Vec2i& hotspot, unsigned int serial = 0);

	///Dispatched the given event.
	void dispatch(Event&& event);

    void registryAdd(unsigned int id, const char* cinterface, unsigned int version);
    void registryRemove(unsigned int id);

    void seatCapabilities(unsigned int caps);

    void addShmFormat(unsigned int format);
    bool shmFormatSupported(unsigned int wlShmFormat);

    wl_display& wlDisplay() const { return *wlDisplay_; };
    wl_compositor& wlCompositor() const { return *wlCompositor_; };
    wl_subcompositor& wlSubcompositor() const{ return *wlSubcompositor_; };
    wl_shm& wlShm() const { return *wlShm_; };
    wl_shell& wlShell() const { return *wlShell_; };
    wl_seat& wlSeat() const { return *wlSeat_; };
    xdg_shell& xdgShell() const { return *xdgShell_; }

	WindowContext* windowContext(wl_surface& surface) const;
    const std::vector<wayland::Output>& outputs() const { return outputs_; }

	// void startDataOffer(dataSource& source, const image& img, const window& w, const event* ev);
	// bool isOffering() const;
	// void endDataOffer();
	// dataOffer* getClipboard();
	// void setClipboard(dataSource& source, const event* ev);

protected:
	wl_display* wlDisplay_;
	wl_registry* wlRegistry_;

	//wayland global resources
	//see registryAdd and registryRemove for why the need to be NamedGlobals
	wayland::NamedGlobal<wl_compositor> wlCompositor_;
	wayland::NamedGlobal<wl_subcompositor> wlSubcompositor_;
	wayland::NamedGlobal<wl_shell> wlShell_;
	wayland::NamedGlobal<wl_shm> wlShm_;
	wayland::NamedGlobal<wl_data_device_manager> wlDataManager_;
	wayland::NamedGlobal<wl_seat> wlSeat_;
	wayland::NamedGlobal<xdg_shell> xdgShell_;

    wl_data_device* wlDataDevice_ = nullptr;

	//cursor
    wl_cursor_theme* wlCursorTheme_ = nullptr;
    wl_surface* wlCursorSurface_ = nullptr;

	//if the current cursor was set from a custom image this will hold an owned pointer
	//to the buffer.
	std::unique_ptr<wayland::ShmBuffer> cursorImageBuffer_;

	//data transfer
    // wl_surface* dataSourceSurface_ = nullptr;
    // wl_surface* dataIconSurface_ = nullptr;
    // wayland::ShmBuffer* dataIconBuffer_ = nullptr;
	// const dataSource* dataSource_ = nullptr;
	// wl_data_source* wlDataSource_ = nullptr;

	std::vector<unsigned int> shmFormats_;
	std::vector<wayland::Output> outputs_;

	std::unique_ptr<WaylandKeyboardContext> keyboardContext_;
	std::unique_ptr<WaylandMouseContext> mouseContext_;
};

}

