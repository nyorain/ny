#pragma once

#include <ny/backend/x11/include.hpp>
#include <ny/backend/windowContext.hpp>

#include <xcb/xcb.h>

#include <cstdint>
#include <vector>

namespace ny
{

///Additional settings for a X11 Window.
class X11WindowSettings : public WindowSettings {};

///The X11 implementation of a WindowContext. 
///Tries to use xcb where possible, for some things (e.g. glx context) xlib is needed though.
class X11WindowContext : public WindowContext
{
public:
	struct Property
	{
		std::vector<std::uint8_t> data;
    	unsigned int format;
    	unsigned int count;
    	xcb_atom_t type;
	};

protected:
	X11AppContext* appContext_ = nullptr;
    xcb_window_t xWindow_ = 0;
	xcb_visualid_t xVisualID_ = 0; 

	unsigned long states_ = 0;
    unsigned long mwmFuncHints_ = 0;
    unsigned long mwmDecoHints_ = 0;

protected:
	X11WindowContext() = default;
	void create(X11AppContext& ctx, const X11WindowSettings& settings);
	xcb_connection_t* xConnection() const;

    virtual void initVisual();

public:
    X11WindowContext(X11AppContext& ctx, const X11WindowSettings& settings = {});
    virtual ~X11WindowContext();

    //high-level virtual interface
    virtual void refresh() override;

    virtual DrawGuard draw() override = 0;

    virtual void show() override;
    virtual void hide() override;

    virtual void size(const Vec2ui& size)override;
    virtual void position(const Vec2i& position) override;

    virtual void cursor(const Cursor& c) override;

    virtual void minSize(const Vec2ui& size) override;
    virtual void maxSize(const Vec2ui& size) override;

    virtual bool handleEvent(const Event& e) override;
	virtual NativeWindowHandle nativeHandle() const override;

    //toplevel window
    virtual void maximize() override;
    virtual void minimize() override;
    virtual void fullscreen() override;
    virtual void toplevel() override;

    virtual void beginMove(const MouseButtonEvent* ev) override;
    virtual void beginResize(const MouseButtonEvent* ev, WindowEdge edges) override;

    virtual void title(const std::string& title) override;
	virtual void icon(const Image* img) override;
	virtual bool customDecorated() const override;

	virtual void addWindowHints(WindowHints hints) override;
	virtual void removeWindowHints(WindowHints hints) override;

    //x11-specific
	X11AppContext& appContext() const { return *appContext_; }
    xcb_window_t xWindow() const { return xWindow_; }

	Property property(xcb_atom_t property);

    //general
    void overrideRedirect(bool redirect);
    void transientFor(xcb_window_t win);
    void cursor(unsigned int xCusrsorID);

    void raise();
    void lower();
    void requestFocus();

    //motif
    void mwmDecorationHints(unsigned long hints);
    void mwmFunctionHints(unsigned long hints);
    void mwmHints(unsigned long deco, unsigned long func);

    unsigned long mwmDecorationHints() const;
    unsigned long mwmFunctionHints() const;

    //ewmh
    void addState(xcb_atom_t state);
    void removeState(xcb_atom_t state);
    void toggleState(xcb_atom_t state);

    unsigned long states() const { return states_; };
    void refreshStates();

    void xWindowType(xcb_atom_t type);
    xcb_atom_t xWindowType();

    void addAllowedAction(xcb_atom_t action); //only does something when custom handled
    void removeAllowedAction(xcb_atom_t action);
    std::vector<xcb_atom_t> allowedActions() const;
};

}
