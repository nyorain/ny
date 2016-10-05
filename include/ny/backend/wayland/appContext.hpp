#pragma once

#include <ny/backend/wayland/include.hpp>
#include <ny/backend/appContext.hpp>

#include <vector>
#include <string>
#include <memory>

namespace ny
{

class WaylandEglDisplay;
class EglContext;

namespace wayland
{

///Utility template that allows to associate a numerical value (name) with wayland globals.
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
	bool dispatchEvents() override;
	bool dispatchLoop(LoopControl& control) override;
	bool threadedDispatchLoop(EventDispatcher& dispatcher, LoopControl& control) override;

	MouseContext* mouseContext() override; 
	KeyboardContext* keyboardContext() override;
	WindowContextPtr createWindowContext(const WindowSettings& windowSettings) override;

	//TODO. Not implemented at the moment
	bool clipboard(std::unique_ptr<DataSource>&& dataSource) override;
	std::unique_ptr<DataOffer> clipboard() override;
	bool startDragDrop(std::unique_ptr<DataSource>&& dataSource) override;

	std::vector<const char*> vulkanExtensions() const override;

    //wayland specific
	///Changes the cursor a cursor with the given name.
	///Note that serial can only be 0 (or invalid) if the cursor was already set by this
	///application of surface entering.
    void cursor(std::string cursorName, unsigned int serial = 0);

	///Changes the cursor to the content of the given image with the given hotspot.
    void cursor(const ImageData& img, const Vec2i& hotspot, unsigned int serial = 0);

	///Dispatched the given event as soon as possible. Needed by wayland callbacks.
	void dispatch(Event&& event);

    wl_display& wlDisplay() const { return *wlDisplay_; };
    wl_compositor& wlCompositor() const { return *wlCompositor_; };

    wl_subcompositor* wlSubcompositor() const{ return wlSubcompositor_; };
    wl_shm* wlShm() const { return wlShm_; };
    wl_seat* wlSeat() const { return wlSeat_; };
    wl_shell* wlShell() const { return wlShell_; };
    xdg_shell* xdgShell() const { return xdgShell_; }

	WaylandWindowContext* windowContext(wl_surface& surface) const;
    const std::vector<wayland::Output>& outputs() const { return outputs_; }
    bool shmFormatSupported(unsigned int wlShmFormat);

	///Returns an eglContext or nullptr if it could not be initialized or ny was built
	///without egl support.
	WaylandEglDisplay* waylandEglDisplay();
	
	//functions called by wayland callbacks
	void outputDone(const wayland::Output& out); //called by wayland callback
    void registryAdd(unsigned int id, const char* cinterface, unsigned int version);
    void registryRemove(unsigned int id);
    void seatCapabilities(unsigned int caps);
	void seatName(const char* name);
    void addShmFormat(unsigned int format);

protected:
	///Modified version of wl_dispatch_display that performs the same operations but
	///does stop blocking (no matter at which stage) if eventfd_ is signaled (i.e. set to 1).
	int dispatchDisplay();

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
    wl_cursor_theme* wlCursorTheme_ = nullptr;
    wl_surface* wlCursorSurface_ = nullptr;

	//if the current cursor was set from a custom image this will hold an owned pointer
	//to the buffer.
	std::unique_ptr<wayland::ShmBuffer> cursorImageBuffer_;
	unsigned int eventfd_ = 0u;

	std::vector<unsigned int> shmFormats_;
	std::vector<wayland::Output> outputs_;

	std::string seatName_;
	std::unique_ptr<WaylandKeyboardContext> keyboardContext_;
	std::unique_ptr<WaylandMouseContext> mouseContext_;

	//stores all pending events (from the dispatch member function) that will should be send
	//in the next dispatch loop iteration/dispatchEvents call.
	std::vector<std::unique_ptr<Event>> pendingEvents_;

	//only used when built with gl and a gl window is created
	//note that their existence is not conditional to not change the abi here only depending
	//on the build config
	bool eglFailed_ = false; //if init failed once this will set to true (and not tried again)
	std::unique_ptr<WaylandEglDisplay> waylandEglDisplay_;

	//data transfer
    // wl_surface* dataSourceSurface_ = nullptr;
    // wl_surface* dataIconSurface_ = nullptr;
    // wayland::ShmBuffer* dataIconBuffer_ = nullptr;
	// const dataSource* dataSource_ = nullptr;
	// wl_data_source* wlDataSource_ = nullptr;
};

}

