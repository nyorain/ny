#pragma once

#include <ny/backend/wayland/include.hpp>
#include <ny/backend/appContext.hpp>
#include <nytl/vec.hpp>

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

	//implementation
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
    // void cursor(const Image* img, cosnt Vec2i& hotspot, unsigned int serial = 0);

    void registryAdd(unsigned int id, const char* cinterface, unsigned int version);
    void registryRemove(unsigned int id);

    void seatCapabilities(unsigned int caps);

    void addShmFormat(unsigned int format);
    bool shmFormatSupported(unsigned int wlShmFormat);

	void mouseMove(unsigned int time, const Vec2ui& pos);
	void mouseEnterSurface(unsigned int serial, wl_surface& surface, const Vec2ui& pos);
	void mouseLeaveSurface(unsigned int serial, wl_surface& surface);
	void mouseButton(unsigned int serial, unsigned int time, unsigned int button, bool pressed);
	void mouseAxis(unsigned int time, unsigned int axis, int value);

    void keyboardKeymap(unsigned int format, int fd, unsigned int size);
    void keyboardEnterSurface(unsigned int serial, wl_surface& surface, wl_array& keys);
    void keyboardLeaveSurface(unsigned int serial, wl_surface& surface);
    void keyboardKey(unsigned int serial, unsigned int time, unsigned int key, bool pressed);
    void keyboardModifiers(unsigned int serial, unsigned int mdepressed, unsigned int mlatched, 
		unsigned int mlocked, unsigned int group);

    wl_display& wlDisplay() const { return *wlDisplay_; };
    wl_compositor& wlCompositor() const { return *wlCompositor_; };
    wl_subcompositor& wlSubcompositor() const{ return *wlSubcompositor_; };
    wl_shm& wlShm() const { return *wlShm_; };
    wl_shell& wlShell() const { return *wlShell_; };
    wl_seat& wlSeat() const { return *wlSeat_; };
    wl_pointer& wlPointer() const { return *wlPointer_; };
    wl_keyboard& wlKeyboard() const { return *wlKeyboard_; };
    xdg_shell& xdgShell() const { return *xdgShell_; }

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

    wl_pointer* wlPointer_ = nullptr;
    wl_keyboard* wlKeyboard_ = nullptr;
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

	struct Impl;
	std::unique_ptr<Impl> pImpl_;
};

}

