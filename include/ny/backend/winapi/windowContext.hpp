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

//enum
enum class winapiDrawType : unsigned char
{
    none,

    wgl,
    gdi
};


//windowContext
class winapiWindowContext : public windowContext
{
protected:
    static unsigned int highestID; //just for window class registration

protected:
    HWND handle_;
    WNDCLASSEX wndClass_;
    PAINTSTRUCT tmpPS_;

    winapiDrawType drawType_ = winapiDrawType::none;
    union
    {
        std::unique_ptr<gdiDrawContext> gdi_ {nullptr};

        #ifdef NY_WithGL
         std::unique_ptr<wglDrawContext> wgl_;
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

    virtual void setSize(Vec2ui size, bool change = 1) override;
    virtual void setPosition(Vec2i position, bool change = 1) override;

    virtual void setCursor(const cursor& c) override;
    virtual void updateCursor(const mouseCrossEvent* ev) override {};

    virtual bool hasGL() const override { return 0; };
    virtual void processEvent(const contextEvent& e) override;

    //toplevel
    virtual void setMaximized() override {};
    virtual void setMinimized() override {};
    virtual void setFullscreen() override {};
    virtual void setNormal() override {};

    virtual void setMinSize(Vec2ui size) override {};
    virtual void setMaxSize(Vec2ui size) override {};

    virtual void beginMove(const mouseButtonEvent* ev) override {};
    virtual void beginResize(const mouseButtonEvent* ev, windowEdge edges) override {};

    virtual void setIcon(const image* img) override {};
    virtual void setTitle(const std::string& title) override {};

    /////////////////////////////////////////
    //winapi specific
    HWND getHandle() const { return handle_; }
    WNDCLASSEX getWndClassEx() const { return wndClass_; }

    gdiDrawContext* getGDI() const { if(drawType_ == winapiDrawType::gdi) return gdi_.get(); return nullptr; }
    wglDrawContext* getWGL() const { if(drawType_ == winapiDrawType::wgl) return wgl_.get(); return nullptr; }
    winapiDrawType getDrawType() const { return drawType_;}
};


}
