#pragma once

#include <ny/backend/wayland/include.hpp>
#include <ny/backend/appContext.hpp>
#include <nytl/callback.hpp>

#include <vector>
#include <string>
#include <memory>
#include <map>

namespace ny
{

class WaylandEglDisplay;
class WaylandDataDevice;
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

///Utility template
///TODO: does not belong here... rather some common util file
template<typename T>
class ConnectionList : public nytl::Connectable<nytl::CbIdType>
{
public:
	class Value : public T 
	{ 
	public:
		using T::T;
		nytl::ConnectionDataPtr<unsigned int> clID_;
	};

	std::vector<Value> items;
	unsigned int highestID;

public:
	void removeConnection(unsigned int id) override
	{
		for(auto it = items.begin(); it != items.end(); ++it)
		{
			if(*it->clID_.get() == id) 
			{
				items.erase(it);
				return;
			}
		}
	}

	nytl::CbConn add(const T& value) 
	{ 
		items.emplace_back(); 
		static_cast<T&>(items.back()) = value;
		items.back().clID_ = std::make_shared<unsigned int>(nextID());
		return {*this, items.back().clID_};
	}

	unsigned int nextID() { return ++highestID; }
};

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
	DataOffer* clipboard() override;
	bool startDragDrop(std::unique_ptr<DataSource>&& dataSource) override;

	std::vector<const char*> vulkanExtensions() const override;

    //wayland specific
	///Dispatched the given event as soon as possible. Needed by wayland callbacks.
	void dispatch(Event&& event);

    wl_display& wlDisplay() const { return *wlDisplay_; };
    wl_compositor& wlCompositor() const { return *wlCompositor_; };

    wl_subcompositor* wlSubcompositor() const{ return wlSubcompositor_; };
    wl_shm* wlShm() const { return wlShm_; };
    wl_seat* wlSeat() const { return wlSeat_; };
    wl_shell* wlShell() const { return wlShell_; };
    xdg_shell* xdgShell() const { return xdgShell_; }
	wl_cursor_theme* wlCursorTheme() const { return wlCursorTheme_; }
	wl_data_device_manager* wlDataManager() const { return wlDataManager_; }

	wl_pointer* wlPointer() const;
	wl_keyboard* wlKeyboard() const;

	WaylandWindowContext* windowContext(wl_surface& surface) const;
    const std::vector<wayland::Output>& outputs() const { return outputs_; }
    bool shmFormatSupported(unsigned int wlShmFormat);

	///Can be called to register custom listeners for fds that the dispatch loop will
	///then poll for.
	using FdCallback = nytl::CompFunc<void(nytl::CbConnRef, int fd, unsigned int events)>;
	nytl::CbConn fdCallback(int fd, unsigned int events, const FdCallback& func);

	WaylandEglDisplay* waylandEglDisplay(); //nullptr if egl not available

	WaylandKeyboardContext& waylandKeyboardContext() const { return *keyboardContext_; }
	WaylandMouseContext& waylandMouseContext() const { return *mouseContext_; }
	
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
	///Returns 0 if stopped because of eventfd, -1 on error and a value > 0 otherwise.
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

    wl_cursor_theme* wlCursorTheme_ {};
    wl_surface* wlCursorSurface_ {};

	std::unique_ptr<WaylandDataDevice> dataDevice_;

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

	struct ListenerEntry
	{
		int fd;
		unsigned int events;
		FdCallback callback;
	};

	ConnectionList<ListenerEntry> fdCallbacks_;
	// std::nytl::Callback<void(int fd, unsigned int events)> fdListeners_;

	//data transfer
    // wl_surface* dataSourceSurface_ = nullptr;
    // wl_surface* dataIconSurface_ = nullptr;
    // wayland::ShmBuffer* dataIconBuffer_ = nullptr;
	// const dataSource* dataSource_ = nullptr;
	// wl_data_source* wlDataSource_ = nullptr;
};

}

