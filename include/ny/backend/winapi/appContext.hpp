#pragma once

#include <ny/backend/winapi/include.hpp>
#include <ny/backend/winapi/input.hpp>
#include <ny/backend/appContext.hpp>

#include <windows.h>
#include <gdiplus.h>

#include <map>
#include <thread>

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
	KeyboardContext* keyboardContext() override;
	MouseContext* mouseContext() override;
	WindowContextPtr createWindowContext(const WindowSettings& settings) override;

	bool dispatchEvents() override;
	bool dispatchLoop(LoopControl& control) override;
	bool threadedDispatchLoop(EventDispatcher& disp, LoopControl& ctrl) override;

	bool clipboard(std::unique_ptr<DataSource>&& source) override;
	std::unique_ptr<DataOffer> clipboard() override;
	bool startDragDrop(std::unique_ptr<DataSource>&& source) override;

	//custom winapi stuff
    LONG_PTR eventProc(HWND, UINT, WPARAM, LPARAM);
	//INT_PTR dlgEventProc(HWND, UINT, WPARAM, LPARAM); //needed?

	EventDispatcher* eventDispatcher() const { return eventDispatcher_; }
    WinapiWindowContext* windowContext(HWND win);

    HINSTANCE hinstance() const { return instance_; };
    const STARTUPINFO& startupInfo() const { return startupInfo_; };

protected:
    HINSTANCE instance_ = nullptr;
    STARTUPINFO startupInfo_;

    Gdiplus::GdiplusStartupInput gdiplusStartupInput_;
    ULONG_PTR gdiplusToken_;

    std::map<HWND, WinapiWindowContext*> contexts_;
	std::unique_ptr<VulkanContext> vulkanContext_;

	LoopControl* dispatcherLoopControl_ = nullptr;
	EventDispatcher* eventDispatcher_ = nullptr;
	bool receivedQuit_ = false;

	WinapiWindowContext* focus_ = nullptr;
	WinapiWindowContext* mouseOver_ = nullptr;

	WinapiMouseContext mouseContext_;
	WinapiKeyboardContext keyboardContext_;
};

}
