#pragma once

#include <ny/config.hpp>
#include <ny/backend/x11/include.hpp>
#include <ny/backend/windowContext.hpp>

#include <X11/Xutil.h>
#include <X11/Xlib.h>
using XWindow = XID;
using XCursor = XID;
typedef struct __GLXFBConfigRec* GLXFBConfig;

#include <memory>

namespace ny
{

//x11WindowContextSettings
class X11WindowContextSettings : public WindowContextSettings {};

//x11AppContext
class X11WindowContext : public WindowContext
{
public:
	enum class DrawType
	{
		glx,
		cairo
	};

protected:
    XWindow xWindow_ = 0;
    int xScreenNumber_ = 0;

    XVisualInfo* xVinfo_ = nullptr;
    bool ownedXVinfo_ = 0;

    unsigned long states_ = 0;
    unsigned long mwmFuncHints_ = 0;
    unsigned long mwmDecoHints_ = 0;

    DrawType drawType_ = DrawType::cairo;
    union
    {
        std::unique_ptr<X11CairoDrawContext> cairo_ {nullptr};
        std::unique_ptr<GlxContext> glx_;
    };

protected:
    void matchVisualInfo();
    GLXFBConfig matchGLXVisualInfo();
    void reparented(const XReparentEvent& ev); //called from appContext through contextEvent

public:
    X11WindowContext(Window& win, const X11WindowContextSettings& settings = {});
    virtual ~X11WindowContext();

    //high-level, have to be implemented/////////////////////////////////
    virtual void refresh() override;

    virtual DrawContext& beginDraw() override;
    virtual void finishDraw() override;

    virtual void show() override;
    virtual void hide() override;

    virtual void size(const vec2ui& size, bool change = 1) override;
    virtual void position(const vec2i& position, bool change = 1) override;

    virtual void cursor(const Cursor& c) override;

    virtual void minSize(const vec2ui& size) override;
    virtual void maxSize(const vec2ui& size) override;

    virtual void addWindowHints(unsigned long hints) override;
    virtual void removeWindowHints(unsigned long hints) override;

    virtual void addContextHints(unsigned long hints) override;
    virtual void removeContextHints(unsigned long hints) override;

    virtual void processEvent(const ContextEvent& e) override;

    //toplevel
    virtual void maximized() override;
    virtual void minimized() override;
    virtual void fullscreen() override;
    virtual void toplevel() override;

    virtual void beginMove(const MouseButtonEvent* ev) override;
    virtual void beginResize(const MouseButtonEvent* ev, windowEdge edges) override;

    virtual void title(const std::string& title) override;
	virtual void icon(const Image* img) override;

	virtual bool hasGL() const override { return (drawType_ == DrawType::glx); };

    //x11-specific
    XWindow xWindow() const { return xWindow_; }
    XVisualInfo* xVinfo() const { return xVinfo_; }

    DrawType drawType() const { return drawType_; }

    X11CairoDrawContext* cairo() const 
		{ return (drawType_ == DrawType::cairo) ? cairo_.get() : nullptr; }
    GlxDrawContext* glx() const
		{ return (drawType_ == DrawType::glx) ? glx_.get() : nullptr; }

    //general
    void overrideRedirect(bool redirect);
    void transientFor(Window& win);
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
    void addState(Atom state);
    void removeState(Atom state);
    void toggleState(Atom state);

    unsigned long states() const { return states_; };
    void refreshStates();

    void xWindowType(Atom type);
    Atom xWindowType();

    void addAllowedAction(Atom action); //only does something when custom handled
    void removeAllowedAction(Atom action);
    std::vector<Atom> allowedActions() const;
};



}
