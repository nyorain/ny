#pragma once

#include <ny/include.hpp>

#include <ny/windowDefs.hpp>

#include <nyutil/nonCopyable.hpp>
#include <nyutil/vec.hpp>

#include <memory>

namespace ny
{

//windowContex////////////////////////////////////////////////////
class windowContext : public nonCopyable
{
protected:
    window& window_;
    unsigned long hints_; //specific context hints. can be declared by every backend

public:
    windowContext(window& win, unsigned long hints = 0);
    windowContext(window& win, const windowContextSettings&);
    virtual ~windowContext(){}

    window& getWindow() const { return window_; }
    unsigned long getContextHints() const { return hints_; }
    virtual unsigned long getAdditionalWindowHints() const { return 0; }

    virtual bool isVirtual() const { return 0; }
    virtual bool hasGL() const = 0;

    virtual void refresh() = 0;
    virtual void redraw(); //needed?

    virtual drawContext& beginDraw() = 0;
    virtual void finishDraw() = 0;

    virtual void show() = 0;
    virtual void hide() = 0;

    virtual void setDroppable(const dataTypes& type){};
    virtual void addDropType(unsigned char type){};
    virtual void removeDropType(unsigned char type){};

    //window hints
    virtual void addWindowHints(unsigned long hints) = 0;
    virtual void removeWindowHints(unsigned long hints) = 0;

    virtual void addContextHints(unsigned long hints) { hints_ |= hints; }
    virtual void removeContextHints(unsigned long hints) { hints_ &= ~hints; }

    virtual void setMinSize(vec2ui size){};
    virtual void setMaxSize(vec2ui size){};

    //event
    virtual void sendContextEvent(const contextEvent& e){};

    virtual void setSize(vec2ui size, bool change = 1) = 0; //change states if the window on the backend has to be resized
    virtual void setPosition(vec2i position, bool change = 1) = 0; //...

    virtual void setCursor(const cursor& c) = 0;
    virtual void updateCursor(mouseCrossEvent* ev){}; //not needed in all


    //toplevel-specific//////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual void setMaximized() = 0;
    virtual void setMinimized() = 0;
    virtual void setFullscreen() = 0;
    virtual void setNormal() = 0; //or reset()?

    virtual void beginMove(const mouseButtonEvent* ev) = 0;
    virtual void beginResize(const mouseButtonEvent* ev, windowEdge edges) = 0;

    virtual void setTitle(const std::string& name) = 0;
	virtual void setIcon(const image* img){}; //may be only important for client decoration
};


//virtual////////////////////////////////////////////

class virtualWindowContext : public windowContext
{
protected:
    redirectDrawContext* drawContext_ = nullptr;

public:
    virtualWindowContext(childWindow& win, const windowContextSettings& = windowContextSettings());

    virtual bool isVirtual() const override { return 1; }
    virtual bool hasGL() const override { return getParentContext()->hasGL(); };

    virtual void refresh() override;

    virtual drawContext& beginDraw() override;
    drawContext& beginDraw(drawContext& dc); //custom overload
    virtual void finishDraw() override;

    virtual void show() override {}
    virtual void hide() override {}

    virtual void setCursor(const cursor& c) override {}
    virtual void updateCursor(mouseCrossEvent* ev) override;

    virtual void setSize(vec2ui size, bool change = 1) override;
    virtual void setPosition(vec2i position, bool change = 1) override;

    //
    virtual void addWindowHints(unsigned long hint) override {}
    virtual void removeWindowHints(unsigned long hint) override {}

    virtual void addContextHints(unsigned long hints) override {}
    virtual void removeContextHints(unsigned long hints) override {}

    //throw at these functions? warning at least?
    virtual void setMaximized() override {}
    virtual void setMinimized() override {}
    virtual void setFullscreen() override {}
    virtual void setNormal() override {} //or reset()?

    virtual void beginMove(const mouseButtonEvent* ev) override {}
    virtual void beginResize(const mouseButtonEvent* ev, windowEdge edges) override {}

    virtual void setTitle(const std::string& name) override {};

    /////////
    windowContext* getParentContext() const;
};

}
