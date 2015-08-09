#pragma once

#include <ny/wayland/waylandInclude.hpp>
#include <ny/appContext.hpp>
#include <nyutil/vec.hpp>

#include <wayland-client.h>
#include <wayland-cursor.h>

#ifdef NY_WithGL
#include <wayland-egl.h>
#include <EGL/egl.h>
#endif //GL

#include <vector>
#include <string>

struct xdg_shell;

namespace ny
{

//waylandAppContext//////////////////////////////////////////////////
class waylandAppContext : public appContext
{
protected:
    wl_display* wlDisplay_ = nullptr;
    wl_compositor* wlCompositor_ = nullptr;
    wl_subcompositor* wlSubcompositor_ = nullptr;
    wl_shell* wlShell_ = nullptr;
    wl_shm* wlShm_ = nullptr;
    wl_data_device_manager* wlDataManager_ = nullptr;
    wl_data_device* wlDataDevice_ = nullptr;

    xdg_shell* xdgShell_ = nullptr;

    wl_seat* wlSeat_ = nullptr;
    wl_pointer* wlPointer_ = nullptr;
    wl_keyboard* wlKeyboard_ = nullptr;

    wl_cursor_theme* wlCursorTheme_ = nullptr;
    wl_surface* wlCursorSurface_ = nullptr;

    bool cursorIsCustomImage_ = 0;
    union
    {
        wl_buffer* wlCursorBuffer_ = nullptr;
        wayland::shmBuffer* cursorImageBuffer_;
    };

    wl_surface* dataSourceSurface_ = nullptr;
    wl_surface* dataIconSurface_ = nullptr;
    wayland::shmBuffer* dataIconBuffer_ = nullptr;
	const dataSource* dataSource_ = nullptr;
	wl_data_source* wlDataSource_ = nullptr;

	std::vector<unsigned int> supportedShm_;
	std::vector<wayland::output> wlOutputs_;

    #ifdef NY_WithEGL
    waylandEGLAppContext* egl_ = nullptr;
    #endif // NY_WithEGL

public:
    waylandAppContext();
    virtual ~waylandAppContext();

    void init();

    bool mainLoop();

    #ifdef NY_WithEGL
    virtual eglAppContext* getEGLAppContext() const override;
    #endif // NY_WithEGL

	void startDataOffer(dataSource& source, const image& img, const window& w, const event* ev);
	bool isOffering() const;
	void endDataOffer();

	dataOffer* getClipboard();
	void setClipboard(dataSource& source, const event* ev);

    //specific
    void setCursor(std::string curs, unsigned int serial = 0);
    void setCursor(image* img, vec2i hotspot, unsigned int serial = 0);

    void registryHandler(wl_registry *registry, unsigned int id, std::string interface, unsigned int version);
    void registryRemover(wl_registry *registry, unsigned int id);

    void seatCapabilities(wl_seat* seat, unsigned int caps);

    wl_display* getWlDisplay() const                            { return wlDisplay_; };
    wl_compositor* getWlCompositor() const                      { return wlCompositor_; };
    wl_subcompositor* getWlSubcompositor() const                { return wlSubcompositor_; };
    wl_shm* getWlShm() const                                    { return wlShm_; };
    wl_shell* getWlShell() const                                { return wlShell_; };
    wl_seat* getWlSeat() const                                  { return wlSeat_; };
    wl_pointer* getWlPointer() const                            { return wlPointer_; };
    wl_keyboard* getWlKeyboard() const                          { return wlKeyboard_; };
    const std::vector<wayland::output>& getWlOutputs() const    { return wlOutputs_; }
    xdg_shell* getXDGShell() const                              { return xdgShell_; }

    void eventMouseMove(wl_pointer *pointer, unsigned int time, int sx, int sy);
    void eventMouseEnterSurface(wl_pointer *pointer, unsigned int serial, wl_surface *surface, int sx, int sy);
    void eventMouseLeaveSurface(wl_pointer *pointer, unsigned int serial, wl_surface *surface);
    void eventMouseButton(wl_pointer *wl_pointer, unsigned int serial, unsigned int time, unsigned int button, unsigned int state);
    void eventMouseAxis(wl_pointer *pointer, unsigned int time, unsigned int axis, int value);

    void eventKeyboardKeymap(wl_keyboard *keyboard, unsigned int format, int fd, unsigned int size);
    void eventKeyboardEnterSurface(wl_keyboard *keyboard, unsigned int serial, wl_surface *surface, wl_array *keys);
    void eventKeyboardLeaveSurface(wl_keyboard *keyboard, unsigned int serial, wl_surface *surface);
    void eventKeyboardKey(wl_keyboard *keyboard, unsigned int serial, unsigned int time, unsigned int key, unsigned int state);
    void eventKeyboardModifiers(wl_keyboard *keyboard, unsigned int serial, unsigned int mods_depressed, unsigned int mods_latched, unsigned int mods_locked, unsigned int group);

    void eventWindowResized(wl_shell_surface* shellSurface, unsigned int edges, unsigned int width, unsigned int height);

    void shmFormat(wl_shm* shm, unsigned int format);

    bool bufferFormatSupported(unsigned int wlBufferType);
    bool bufferFormatSupported(bufferFormat format);
};

}

