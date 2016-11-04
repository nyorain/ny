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

class EglSetup;

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
class ConnectionList : public nytl::Connectable
{
public:
	class Value : public T
	{
	public:
		using T::T;
		nytl::ConnectionID clID_;
	};

	std::vector<Value> items;
	nytl::ConnectionID highestID;

public:
	bool disconnect(nytl::ConnectionID id) override
	{
		for(auto it = items.begin(); it != items.end(); ++it)
		{
			if(it->clID_ == id)
			{
				items.erase(it);
				return true;
			}
		}

		return false;
	}

	nytl::Connection add(const T& value)
	{
		items.emplace_back();
		static_cast<T&>(items.back()) = value;
		items.back().clID_ = nextID();
		return {*this, items.back().clID_};
	}

	nytl::ConnectionID nextID() 
	{ 
		++reinterpret_cast<std::uintptr_t&>(highestID);
		return highestID;
	}
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
	GlSetup* glSetup() const override;

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
	using FdCallback = nytl::CompFunc<void(nytl::ConnectionRef, int fd, unsigned int events)>;
	nytl::Connection fdCallback(int fd, unsigned int events, const FdCallback& func);

	WaylandKeyboardContext& waylandKeyboardContext() const { return *keyboardContext_; }
	WaylandMouseContext& waylandMouseContext() const { return *mouseContext_; }

	EglSetup* eglSetup() const;

	//functions called by wayland callbacks
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

	//if init failed once, will be set to true (and not tried again)
	//mutable since is only some kind of "cache" and will be changed from [e]glSetup() const
	mutable bool eglFailed_ = false; 

	struct ListenerEntry
	{
		int fd;
		unsigned int events;
		FdCallback callback;
	};

	ConnectionList<ListenerEntry> fdCallbacks_;

	struct Impl;
	std::unique_ptr<Impl> impl_;
};

}
