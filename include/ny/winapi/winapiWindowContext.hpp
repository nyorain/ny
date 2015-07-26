#pragma once

#include <ny/winapi/winapiInclude.hpp>
#include <ny/windowContext.hpp>

#include <ny/winapi/gdiDrawContext.hpp>

#include <windows.h>

namespace ny
{

class winapiWindowContextSettings : public windowContextSettings
{
};

typedef winapiWindowContextSettings winapiWS;

enum class winapiDrawType
{
    none,

    gdi,
    wgl
};

//windowContext
class winapiWindowContext : public virtual windowContext
{
protected:
    static unsigned int highestID;

    HWND handle_;
    HINSTANCE instance_;
    WNDCLASSEX wndClass_;

    winapiDrawType drawType_ = winapiDrawType::none;
    union
    {
        gdiDrawContext* gdi_ = nullptr;

        #ifdef NY_WithGL
        wglDrawContext* wgl_ = nullptr;
        #endif //GL
    };

public:
    winapiWindowContext(window& win, const winapiWindowContextSettings& settings = winapiWindowContextSettings());
    virtual ~winapiWindowContext();

    virtual void refresh();

    virtual drawContext& beginDraw();
    virtual void finishDraw();

    virtual void show();
    virtual void hide();

    virtual void setWindowHints(const unsigned long hints);
    virtual void addWindowHints(const unsigned long hint);
    virtual void removeWindowHints(const unsigned long hint);

    virtual void setContextHints(const unsigned long hints);
    virtual void addContextHints(const unsigned long hints);
    virtual void removeContextHints(const unsigned long hints);

    virtual void setSettings(const windowContextSettings& s);

    virtual void setSize(vec2ui size, bool change = 1);
    virtual void setPosition(vec2i position, bool change = 1);

    virtual void setCursor(const cursor& c) override;

    virtual bool hasGL() const override;

    //toplevel
    virtual void setMaximized() override;
    virtual void setMinimized() override;
    virtual void setFullscreen() override;
    virtual void setNormal() override;

    virtual void setMinSize(vec2ui size) override;
    virtual void setMaxSize(vec2ui size) override;

    virtual void beginMove(mouseButtonEvent* ev) override;
    virtual void beginResize(mouseButtonEvent* ev, windowEdge edges) override;

    virtual void setIcon(const image* img) override;
    virtual void setName(std::string name) override;

    /////////////////////////////////////////
    //winapi specific

    HWND getHandle() const { return handle_; }
    HINSTANCE getInstance() const { return instance_; }
    WNDCLASSEX getWndClassEx() const { return wndClass_; }
};


}
