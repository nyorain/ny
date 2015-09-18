#pragma once

#include <ny/appContext.hpp>
#include <nyutil/eventLoop.hpp>

#include <winsock2.h>
#include <windows.h>
#include <gdiplus.h>

#include <memory>
#include <map>

using namespace Gdiplus;

namespace ny
{

class winapiAppContext;
class winapiWindowContext;
typedef winapiAppContext winapiAC;

class winapiAppContext : public appContext
{
public:
    typedef LRESULT CALLBACK (*wndProc)(HWND, UINT, WPARAM, LPARAM);

protected:
    HINSTANCE instance_ = nullptr;
    STARTUPINFO startupInfo_;

    std::map<HWND, winapiWindowContext*> contexts_;

    GdiplusStartupInput gdiplusStartupInput_;
    ULONG_PTR gdiplusToken_;

    std::unique_ptr<idleEventSource> eventSource_;

public:
    winapiAppContext();
    ~winapiAppContext();

    virtual bool mainLoop();

    virtual void startDataOffer(dataSource& source, const image& img, const window& w, const event* ev){}
    virtual bool isOffering() const { return 0; }
    virtual void endDataOffer(){}

    virtual dataOffer* getClipboard(){ return nullptr; }
    virtual void setClipboard(dataSource& source, const event* ev){}

    //data specifications
    void setClipboard(const std::string& str){}
    void setClipboard(const image& str){}


    virtual LRESULT eventProc(HWND, UINT, WPARAM, LPARAM);

    void registerContext(HWND w, winapiWindowContext* c);
    void unregisterContext(HWND w);
    void unregisterContext(winapiWindowContext* c);
    winapiWindowContext* getWindowContext(HWND win);

    void setCursor(unsigned int cursorID);
    void setCursor(image* img);

    const HINSTANCE& getInstance() const { return instance_; };
    const STARTUPINFO& getStartInfo() const { return startupInfo_; };
};

LRESULT CALLBACK dummyWndProc(HWND a, UINT b, WPARAM c, LPARAM d);

}
