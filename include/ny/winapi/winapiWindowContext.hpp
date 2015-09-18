#pragma once

#include <ny/winapi/winapiInclude.hpp>
#include <ny/windowContext.hpp>
#include <ny/winapi/gdiDrawContext.hpp>

#include <winsock2.h>
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
class winapiWindowContext : public windowContext
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
         wglDrawContext* wgl_;
        #endif //GL
    };

public:
    winapiWindowContext(window& win, const winapiWindowContextSettings& settings = winapiWindowContextSettings());
    virtual ~winapiWindowContext();

    virtual void refresh() override;

    virtual drawContext* beginDraw() override;
    virtual void finishDraw() override;

    virtual void show() override;
    virtual void hide() override;

    //virtual void setWindowHints(const unsigned long hints) override;
    virtual void addWindowHints(const unsigned long hint) override;
    virtual void removeWindowHints(const unsigned long hint) override;

    //virtual void setContextHints(const unsigned long hints);
    virtual void addContextHints(const unsigned long hints) override;
    virtual void removeContextHints(const unsigned long hints) override;

    //virtual void setSettings(const windowContextSettings& s);

    virtual void setSize(vec2ui size, bool change = 1) override;
    virtual void setPosition(vec2i position, bool change = 1) override;

    virtual void setCursor(const cursor& c) override;
    virtual void updateCursor(const mouseCrossEvent* ev) override {};

    virtual bool hasGL() const override { return 0; };

    //toplevel
    virtual void setMaximized() override {};
    virtual void setMinimized() override {};
    virtual void setFullscreen() override {};
    virtual void setNormal() override {};

    virtual void setMinSize(vec2ui size) override {};
    virtual void setMaxSize(vec2ui size) override {};

    virtual void beginMove(const mouseButtonEvent* ev) override {};
    virtual void beginResize(const mouseButtonEvent* ev, windowEdge edges) override {};

    virtual void setIcon(const image* img) override {};
    virtual void setTitle(const std::string& title) override {};

    /////////////////////////////////////////
    //winapi specific

    HWND getHandle() const { return handle_; }
    HINSTANCE getInstance() const { return instance_; }
    WNDCLASSEX getWndClassEx() const { return wndClass_; }
};


}
