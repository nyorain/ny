#pragma once

#include <ny/backend/winapi/include.hpp>
#include <ny/backend/appContext.hpp>

#include <windows.h>
#include <gdiplus.h>
#include <map>

using namespace Gdiplus;

namespace ny
{

class WinapiAppContext : public AppContext
{
public:
	class LoopControlImpl;
	static LRESULT CALLBACK wndProcCallback(HWND a, UINT b, WPARAM c, LPARAM d);

protected:
    HINSTANCE instance_ = nullptr;
    STARTUPINFO startupInfo_;

    std::map<HWND, WinapiWindowContext*> contexts_;

    GdiplusStartupInput gdiplusStartupInput_;
    ULONG_PTR gdiplusToken_;

	LoopControl* dispatcherLoopControl_ = nullptr;
	EventDispatcher* eventDispatcher_ = nullptr;

	bool receivedQuit_ = 0;

public:
    WinapiAppContext();
    ~WinapiAppContext();

	virtual bool dispatchEvents(EventDispatcher& dispatcher) override;
	virtual bool dispatchLoop(EventDispatcher& dispatcher, LoopControl& control) override;

    //data specifications
    LRESULT eventProc(HWND, UINT, WPARAM, LPARAM);

    void registerContext(HWND w, WinapiWindowContext& c);
    void unregisterContext(HWND w);
    WinapiWindowContext* windowContext(HWND win);

    void setCursor(unsigned int cursorID);
    void setCursor(Image* img);

    HINSTANCE hinstance() const { return instance_; };
    const STARTUPINFO& startupInfo() const { return startupInfo_; };
};

}
