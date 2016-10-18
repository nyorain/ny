#pragma once

#include <ny/backend/x11/include.hpp>
#include <ny/backend/windowContext.hpp>
#include <ny/backend/windowSettings.hpp>

#include <cstdint>
#include <vector>

typedef struct xcb_visualtype_t xcb_visualtype_t;

namespace ny
{

///Additional settings for a X11 Window.
class X11WindowSettings : public WindowSettings {};

///The base class for drawing integrations.
class X11DrawIntegration
{
public:
	X11DrawIntegration(X11WindowContext&);
	virtual ~X11DrawIntegration();
	virtual void resize(const nytl::Vec2ui&) {}

protected:
	X11WindowContext& windowContext_;
};

///The X11 implementation of the WindowContext interface.
///Provides some extra functionality for x11.
///Tries to use xcb where possible, for some things (e.g. glx context) xlib is needed though.
class X11WindowContext : public WindowContext
{
public:
    X11WindowContext(X11AppContext& ctx, const X11WindowSettings& settings = {});
    ~X11WindowContext();

    void refresh() override;
    void show() override;
    void hide() override;

	void droppable(const DataTypes&) override {};

    void minSize(const nytl::Vec2ui& size) override;
    void maxSize(const nytl::Vec2ui& size) override;

    void size(const nytl::Vec2ui& size) override;
    void position(const nytl::Vec2i& position) override;

    void cursor(const Cursor& c) override;

	NativeWindowHandle nativeHandle() const override;
    bool handleEvent(const Event& e) override;

	WindowCapabilities capabilities() const override;

    //toplevel window
    void maximize() override;
    void minimize() override;
    void fullscreen() override;
    void normalState() override;

    void beginMove(const MouseButtonEvent* ev) override;
    void beginResize(const MouseButtonEvent* ev, WindowEdges edges) override;

    void title(const std::string& title) override;
	void icon(const ImageData& img) override;
	bool customDecorated() const override;

	void addWindowHints(WindowHints hints) override;
	void removeWindowHints(WindowHints hints) override;

    //x11-specific
public:
	X11AppContext& appContext() const { return *appContext_; } ///The associated AppContext
	std::uint32_t xWindow() const { return xWindow_; } ///The underlaying x window handle
	xcb_connection_t* xConnection() const; ///The associated x conntextion
	x11::EwmhConnection* ewmhConnection() const; ///The associated ewmh connection (helper)
	nytl::Vec2ui size() const; ///Queries the current window size

    void overrideRedirect(bool redirect); ///Sets the overrideRedirect flag for the window
    void transientFor(std::uint32_t win); ///Makes the window transient for another x window

    void raise(); ///tries to raise the window
    void lower(); ///tries to lower the window
    void requestFocus(); ///tries to bring the window focus

	///Sets the motif deco and/or function hints for the window.
	///The hints are declared in ny/backend/x11/util.hpp.
	///Motif hints are outdated, so may not work on every compositor.
    void mwmHints(unsigned long deco, unsigned long func, bool d = true, bool f = true);

    unsigned long mwmDecorationHints() const; ///Returns the current motif deco hints
    unsigned long mwmFunctionHints() const; ///Returns the current motif function hints

	///Adds the given state/states to the window.
	///Look at the ewmh specification for more information and allowed atoms.
	///The changed property is _NET_WM_STATE.
    void addStates(std::uint32_t state1, std::uint32_t state2 = 0);

	///Adds the given state/states to the window.
	///Look at the ewmh specification for more information and allowed atoms.
	///The changed property is _NET_WM_STATE.
    void removeStates(std::uint32_t state1, std::uint32_t state2 = 0);

	///Adds the given state/states to the window.
	///Look at the ewmh specification for more information and allowed atoms.
	///The changed property is _NET_WM_STATE.
    void toggleStates(std::uint32_t state1, std::uint32_t state2 = 0);

	///Returns the states associated with this window.
	///For more information look in the ewmh specification for _NET_WM_STATE.
	std::vector<std::uint32_t> states() const { return states_; };

	///Reloads the stores window states. 
	///XXX: Needed? should they be stored? TODO
    void refreshStates(); 

	///Sets the window type. 
	///For more information look in the ewmh specification for _NET_WM_WINDOW_TYPE.
    void xWindowType(std::uint32_t type);

	///Returns the window type assocated with this window.
	///For more information look in the ewmh specification for _NET_WM_WINDOW_TYPE.
	std::uint32_t xWindowType();

	///Adds an allowed action for the window.
	///For more information look in the ewmh specification for _NET_WM_ALLOWED_ACTIONS.
    void addAllowedAction(std::uint32_t action); //only does something when custom handled

	///Removes an allowed action for the window.
	///For more information look in the ewmh specification for _NET_WM_ALLOWED_ACTIONS.
    void removeAllowedAction(std::uint32_t action);

	///Returns all allowed actions for the window.
	///For more information look in the ewmh specification for _NET_WM_ALLOWED_ACTIONS.
    std::vector<std::uint32_t> allowedActions() const;

	///Returns the x visual id for this window.
	///If it has not been set, 0 is retrned.
	unsigned int xVisualID() const { return visualID_; }

	///Finds and returns the xcb_visualtype_t for this window.
	///If the visual id has not been set or does not have a matching visualtype, nullptr
	///is returned.
	xcb_visualtype_t* xVisualType() const;

	///Returns the depth of the visual (i.e. the bits per pixel)
	///Notice that this might somehow differ from the bits_per_rgb member of the
	///visual type (since it also counts the alpha bits).
	unsigned int visualDepth() const { return depth_; }

	///Sets the integration to the given one.
	///Will return false if there is already such an integration or this implementation
	///does not support them (e.g. vulkan/opengl WindowContext).
	///Calling this function with a nullptr resets the integration.
	virtual bool drawIntegration(X11DrawIntegration* integration);

	///Creates a surface and stores it in the given parameter.
	///Returns false and does not change the given parameter if a surface coult not be
	///created.
	///This could be the case if the WindowContext already has another integration.
	virtual bool surface(Surface& surface);


protected:
	///Default Constructor only for derived classes that later call the create function.
	X11WindowContext() = default;

	///Creates the x11 window.
	///This extra function may be needed by derived drawType classes.
	void create(X11AppContext& ctx, const X11WindowSettings& settings);

	///The different context classes derived from this class may override this function to
	///select a custom visual for the window or query it in a different way connected with
	///more information. 
	///Will automatically be called by the create function if the xVisualtype_ member variable is
	///not set yet (since it is needed for window creation).
	///By default, this just selects the 32 or 24 bit visual with the best format.
    virtual void initVisual();

protected:
	X11AppContext* appContext_ = nullptr;
	X11WindowSettings settings_ {};
	std::uint32_t xWindow_ {};
	std::uint32_t xCursor_ {};

	//we store the visual id instead of the visualtype since e.g. glx does not
	//care/deal with the visualtype in any way.
	//if the xcb_visualtype_t* is needed, just call xVisualType() which will try
	//to query it.
	unsigned int visualID_ {};
	unsigned int depth_ {};

	//Stored EWMH states can be used to check whether it is fullscreen, maximized etc.
	std::vector<std::uint32_t> states_;
    unsigned long mwmFuncHints_ {};
    unsigned long mwmDecoHints_ {};

	//The draw integration for this WindowContext.
	X11DrawIntegration* drawIntegration_ = nullptr;
};

}
