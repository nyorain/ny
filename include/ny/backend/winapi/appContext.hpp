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
	static LONG_PTR CALLBACK wndProcCallback(HWND a, UINT b, WPARAM c, LPARAM d);
	static INT_PTR CALLBACK dlgProcCallback(HWND a, UINT b, WPARAM c, LPARAM d);

public:
    WinapiAppContext();
    ~WinapiAppContext();

	//interface implementation
	virtual bool dispatchEvents(EventDispatcher& disp) override;
	virtual bool dispatchLoop(EventDispatcher& disp, LoopControl& control) override;
	virtual bool threadedDispatchLoop(ThreadedEventDispatcher& disp, LoopControl& ctrl) override;

    //data specifications
    LRESULT eventProc(HWND, UINT, WPARAM, LPARAM);

    void registerContext(HWND w, WinapiWindowContext& c);
    void unregisterContext(HWND w);
    WinapiWindowContext* windowContext(HWND win);

    void setCursor(unsigned int cursorID);
    void setCursor(Image* img);

    HINSTANCE hinstance() const { return instance_; };
    const STARTUPINFO& startupInfo() const { return startupInfo_; };

protected:
	class LoopControlImpl;

protected:
    HINSTANCE instance_ = nullptr;
    STARTUPINFO startupInfo_;

    GdiplusStartupInput gdiplusStartupInput_;
    ULONG_PTR gdiplusToken_;

    std::map<HWND, WinapiWindowContext*> contexts_;
	HWND mouseOver_ = nullptr; //used to generate mouse enter events

	LoopControl* dispatcherLoopControl_ = nullptr;
	EventDispatcher* eventDispatcher_ = nullptr;

	bool receivedQuit_ = false;
	bool threadsafe_ = false;
};

}
