// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/winapi/include.hpp>
#include <ny/winapi/windows.hpp>
#include <ny/winapi/input.hpp>
#include <ny/appContext.hpp>
#include <ny/deferred.hpp>

#include <map>
#include <thread>

namespace ny {

///Winapi AppContext implementation.
///Implements the main event loop and callbacks and deals with com/ole implementations.
class WinapiAppContext : public AppContext {
public:
	static LONG_PTR CALLBACK wndProcCallback(HWND, UINT, WPARAM, LPARAM);
	static INT_PTR CALLBACK dlgProcCallback(HWND, UINT, WPARAM, LPARAM);

	DeferredOperator<void(), WindowContext*> deferred;

public:
	WinapiAppContext();
	~WinapiAppContext();

	//AppContext interface implementation
	KeyboardContext* keyboardContext() override;
	MouseContext* mouseContext() override;
	WindowContextPtr createWindowContext(const WindowSettings& settings) override;

	bool pollEvents() override;
	bool waitEvents() override;
	void wakeupWait() override;

	bool clipboard(std::unique_ptr<DataSource>&& source) override;
	DataOffer* clipboard() override;
	bool startDragDrop(std::unique_ptr<DataSource>&& source) override;

	std::vector<const char*> vulkanExtensions() const override;
	GlSetup* glSetup() const override;

	//- winapi specific -
	WglSetup* wglSetup() const;
	LONG_PTR eventProc(HWND, UINT, WPARAM, LPARAM);
	WinapiWindowContext* windowContext(HWND win);

	HINSTANCE hinstance() const { return instance_; };

protected:
	HINSTANCE instance_ = nullptr; // TODO: really needed?

	std::unique_ptr<DataOffer> clipboardOffer_;
	unsigned int clipboardSequenceNumber_ {};

	HWND dummyWindow_ {};
	WinapiMouseContext mouseContext_;
	WinapiKeyboardContext keyboardContext_;

	struct Impl;
	std::unique_ptr<Impl> impl_;
	DWORD mainThread_;
};

} // namespace ny
