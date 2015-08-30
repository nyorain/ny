#pragma once

#include <ny/x11/x11Include.hpp>
#include <ny/appContext.hpp>
#include <ny/windowContext.hpp>
#include <ny/backend.hpp>
#include <ny/windowEvents.hpp>

#include <X11/Xutil.h>
#include <memory>
#include <mutex>

namespace ny
{

//x11Event///////////////////////////////////////////
class x11EventData : public eventData
{
public:
    x11EventData(const XEvent& e) : ev(e) {};
    XEvent ev;
};

////
constexpr unsigned int X11Reparent = 11;
class x11ReparentEvent : public contextEvent
{
public:
    x11ReparentEvent(eventHandler* h = nullptr, const XReparentEvent& e = XReparentEvent()) : contextEvent(h), ev(e) {};
    virtual unsigned int getContextEventType() const override { return X11Reparent; }

    XReparentEvent ev;
};

//x11WindowContextSettings
class x11WindowContextSettings : public windowContextSettings
{
};

//enum
enum class x11WindowType : unsigned char
{
    none,

    toplevel,
    child
};

enum class x11DrawType : unsigned char
{
    none,

    cairo,
    glx,
    egl
};

struct glxFBC;

//x11AppContext///////////////////////////////////////7
class x11WindowContext : public virtual windowContext
{
protected:
    Window xWindow_ = 0;
    int xScreenNumber_ = 0;

    XVisualInfo* xVinfo_ = nullptr;
    bool ownedXVinfo_ = 0;

    unsigned long states_ = 0;
    unsigned long mwmFuncHints_ = 0;
    unsigned long mwmDecoHints_ = 0;

    x11WindowType windowType_ = x11WindowType::none;
    x11DrawType drawType_ = x11DrawType::none;
    union
    {
        std::unique_ptr<x11CairoDrawContext> cairo_;
        std::unique_ptr<x11EGLDrawContext> egl_;
        struct
        {
            std::unique_ptr<glxDrawContext> ctx;
            std::unique_ptr<glxFBC> fbc;
        } glx_;
    };

    void matchVisualInfo();
    void matchGLXVisualInfo();

public:
    x11WindowContext(window& win, const x11WindowContextSettings& settings = x11WindowContextSettings());
    virtual ~x11WindowContext();

    //high-level, have to be implemented/////////////////////////////////
    virtual void refresh() override;

    virtual drawContext& beginDraw() override;
    virtual void finishDraw() override;

    virtual void show() override;
    virtual void hide() override;

    virtual void setSize(vec2ui size, bool change = 1) override;
    virtual void setPosition(vec2i position, bool change = 1) override;

    virtual void setCursor(const cursor& c) override;

    virtual void setMinSize(vec2ui size) override;
    virtual void setMaxSize(vec2ui size) override;

    virtual void addWindowHints(unsigned long hints) override;
    virtual void removeWindowHints(unsigned long hints) override;

    virtual void addContextHints(unsigned long hints) override;
    virtual void removeContextHints(unsigned long hints) override;

    virtual void sendContextEvent(contextEvent& e) override;

    //toplevel////////////////////
    virtual void setMaximized() override;
    virtual void setMinimized() override;
    virtual void setFullscreen() override;
    virtual void setNormal() override;

    virtual void beginMove(mouseButtonEvent* ev) override;
    virtual void beginResize(mouseButtonEvent* ev, windowEdge edges) override;

    virtual void setTitle(const std::string& title) override;
	virtual void setIcon(const image* img) override;

	virtual bool hasGL() const override { return (drawType_ == x11DrawType::glx || drawType_ == x11DrawType::egl); };

    //x11-specific////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////
    //get
    Window getXWindow() const { return xWindow_; }
    XVisualInfo* getXVinfo() const { return xVinfo_; }

    x11DrawType getDrawType() const { return drawType_; }
    x11WindowType getWindowType() const { return windowType_; }

    x11CairoDrawContext* getCairo() const { return (drawType_ == x11DrawType::cairo) ? cairo_.get() : nullptr; }
    x11EGLDrawContext* getEGL() const { return (drawType_ == x11DrawType::egl) ? egl_.get() : nullptr; }
    glxDrawContext* getGLX() const { return (drawType_ == x11DrawType::glx) ? glx_.ctx.get() : nullptr; }
    glxFBC* getGLXFBC() const { return (drawType_ == x11DrawType::glx) ? glx_.fbc.get() : nullptr; }

    //general
    void setOverrideRedirect(bool redirect = 1);
    void setTransientFor(window* win);
    void setCursor(unsigned int xCusrsorID);

    void raise();
    void lower();
    void requestFocus();

    //motif
    void setMwmDecorationHints(unsigned long hints); //maybe only in toplevelWC? see also -> add and remove window hints
    void setMwmFunctionHints(unsigned long hints);
    void setMwmHints(unsigned long deco, unsigned long func);

    unsigned long getMwmDecorationHints();
    unsigned long getMwmFunctionHints();

    //ewmh
    void addState(Atom state);
    void removeState(Atom state);
    void toggleState(Atom state);

    unsigned long getStates() const { return states_; };
    void refreshStates();

    void setType(Atom type);
    Atom getType();

    void addAllowedAction(Atom action); //only does something when custom handled
    void removeAllowedAction(Atom action);
    std::vector<Atom> getAllowedAction();

    void wasReparented(x11ReparentEvent& ev); //called from appContext through contextEvent
};



}
