#pragma once

#include <ny/backend/winapi/include.hpp>
#include <ny/backend/winapi/windows.hpp>
#include <ny/backend/winapi/input.hpp>
#include <ny/backend/appContext.hpp>

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
	DataOffer* clipboard() override;
	bool startDragDrop(std::unique_ptr<DataSource>&& source) override;

	std::vector<const char*> vulkanExtensions() const override;

	//custom winapi stuff
	LONG_PTR eventProc(HWND, UINT, WPARAM, LPARAM);
	//INT_PTR dlgEventProc(HWND, UINT, WPARAM, LPARAM); //needed?

	void dispatch(Event&& event);
	WinapiWindowContext* windowContext(HWND win);

	HINSTANCE hinstance() const { return instance_; };
	const STARTUPINFO& startupInfo() const { return startupInfo_; };

protected:
	HINSTANCE instance_ = nullptr;
	STARTUPINFO startupInfo_;

	Gdiplus::GdiplusStartupInput gdiplusStartupInput_;
	ULONG_PTR gdiplusToken_;

	std::map<HWND, WinapiWindowContext*> contexts_;
	std::vector<std::unique_ptr<Event>> pendingEvents_;

	std::unique_ptr<DataOffer> clipboardOffer_;
	unsigned int clipboardSequenceNumber_ {};
	HWND dummyWindow_ {};

	LoopControl* dispatcherLoopControl_ = nullptr;
	EventDispatcher* eventDispatcher_ = nullptr;
	bool receivedQuit_ = false;

	WinapiMouseContext mouseContext_;
	WinapiKeyboardContext keyboardContext_;

	struct Impl;
	std::unique_ptr<Impl> impl_;
};

}
