// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/winapi/appContext.hpp>
#include <ny/winapi/windowContext.hpp>
#include <ny/winapi/util.hpp>
#include <ny/winapi/com.hpp>
#include <ny/winapi/bufferSurface.hpp>

#include <ny/mouseContext.hpp>
#include <ny/keyboardContext.hpp>

#include <ny/log.hpp>
#include <ny/loopControl.hpp>

#ifdef NY_WithGl
#include <ny/winapi/wgl.hpp>
#endif //Gl

#ifdef NY_WithVulkan
#define VK_USE_PLATFORM_WIN32_KHR
#include <ny/winapi/vulkan.hpp>
#include <vulkan/vulkan.h>
#endif //Vulkan

#include <nytl/utf.hpp>
#include <nytl/scope.hpp>

#include <ole2.h>

#include <stdexcept>
#include <cstring>
#include <mutex>
#include <condition_variable>
#include <queue>

// NOTE: we never actually call TranslateMessage since we translate keycodes manually
// and calling this function will interfer with our ToUnicode calls.

namespace ny {
namespace {

// LoopInterface implementation
class WinapiLoopImpl : public ny::LoopInterface {
public:
	DWORD threadHandle;

	std::atomic<bool> run {true};
	std::queue<std::function<void()>> functions;
	std::mutex mutex;

public:
	WinapiLoopImpl(LoopControl& lc) : LoopInterface(lc)
	{
		threadHandle = ::GetCurrentThreadId();
	}

	bool stop() override
	{
		run.store(false);
		wakeup();
		return true;
	};

	bool call(std::function<void()> function) override
	{
		if(!function) return false;

		{
			std::lock_guard<std::mutex> lock(mutex);
			functions.push(std::move(function));
		}

		wakeup();
		return true;
	}

	void wakeup()
	{
		::PostThreadMessage(threadHandle, WM_USER, 0, 0);
	}

	std::function<void()> popFunction()
	{
		std::lock_guard<std::mutex> lock(mutex);
		if(functions.empty()) return {};
		auto ret = std::move(functions.front());
		functions.pop();
		return ret;
	}
};

} // anonymous util namespace

struct WinapiAppContext::Impl {
#ifdef NY_WithGl
	WglSetup wglSetup;
	bool wglFailed;
#endif //GL
};

// TODO: for extra dnd thread
// std::thread dndThread;
// std::mutex dndMutex;
// std::condition_variable dndCond;
// std::unique_ptr<DataSource> dndSource;

//winapi callbacks
LRESULT CALLBACK WinapiAppContext::wndProcCallback(HWND a, UINT b, WPARAM c, LPARAM d)
{
	auto wc = reinterpret_cast<WinapiWindowContext*>(::GetWindowLongPtr(a, GWLP_USERDATA));
	if(!wc) return ::DefWindowProc(a, b, c, d);
	return wc->appContext().eventProc(a, b, c, d);
}

LRESULT CALLBACK WinapiAppContext::dlgProcCallback(HWND a, UINT b, WPARAM c, LPARAM d)
{
	auto wc = reinterpret_cast<WinapiWindowContext*>(::GetWindowLongPtr(a, GWLP_USERDATA));
	if(!wc) return ::DefWindowProc(a, b, c, d);
	return wc->appContext().eventProc(a, b, c, d);
}

//WinapiAC
WinapiAppContext::WinapiAppContext() : mouseContext_(*this), keyboardContext_(*this)
{
	impl_ = std::make_unique<Impl>();
	instance_ = ::GetModuleHandle(nullptr);

	// needed for dnd and clipboard
	auto res = ::OleInitialize(nullptr);
	if(res != S_OK) warning("ny::WinapiAppContext: OleInitialize failed with code ", res);

	// init dummy window (needed as clipboard viewer and opengl dummy window)
	dummyWindow_ = ::CreateWindow(L"STATIC", L"", WS_DISABLED, 0, 0, 10, 10, nullptr, nullptr,
		hinstance(), nullptr);

	// TODO:
	// we must load AddClipboardFormatListener dynamically since it does not link correctly
	// with mingw and is not supported for windows versions before vista
	// It is not really needed to call this function since we check for new clipboard content
	// anyways everytime clipboard() is called.
	// But this can free the old clipboard DataOffer object as soon as the clipboard
	// changes and create the new one.

	// auto lib = ::LoadLibrary(L"User32.dll");
	// if(lib) {
	// 	auto func = ::GetProcAddress(lib, "AddClipboardFormatListener");
	// 	if(!func) warning("ny::WinapiAppContext: Failed to retrieve AddClipboardFormatListener");
	// 	else (reinterpret_cast<BOOL(*)(HWND)>(func))(dummyWindow_);
	// 	::FreeLibrary(lib);
	// }

	// TODO: correctly implement this for non-modal dnd
	// worth it?

	// auto t1 = ::GetCurrentThreadId();
	// dndThread = std::thread([=](){
	//     ::OleInitialize(nullptr);
	//     auto t2 = ::GetCurrentThreadId();
	//     ::AttachThreadInput(t2, t1, true);
	//
	//     std::unique_lock<std::mutex> lock(dndMutex);
	//     while(true) {
	//         dndCond.wait(lock);
	//
	//         if(dndSource) {
	//         	auto dataObj = new winapi::com::DataObjectImpl(std::move(dndSource));
	//         	auto dropSource = new winapi::com::DropSourceImpl();
	//
	//         	DWORD effect;
	//             log("DoDragDrop start");
	//         	auto ret = ::DoDragDrop(dataObj, dropSource, DROPEFFECT_COPY, &effect);
	//             log("DoDragDrop: ", (unsigned int)ret);
	//         }
	//     }
	// });
}

WinapiAppContext::~WinapiAppContext()
{
	if(dummyWindow_) ::DestroyWindow(dummyWindow_);
	::OleUninitialize();
}

std::unique_ptr<WindowContext> WinapiAppContext::createWindowContext(const WindowSettings& settings)
{
	WinapiWindowSettings winapiSettings;
	const WinapiWindowSettings* ws = dynamic_cast<const WinapiWindowSettings*>(&settings);

	if(ws) winapiSettings = *ws;
	else winapiSettings.WindowSettings::operator=(settings);

	if(settings.surface == SurfaceType::vulkan) {
		#ifdef NY_WithVulkan
			return std::make_unique<WinapiVulkanWindowContext>(*this, winapiSettings);
		#else
			static constexpr auto noVulkan = "ny::WinapiAppContext::createWindowContext: "
				"ny was built without vulkan support and can not create a Vulkan surface";

		throw std::logic_error(noVulkan);
		#endif //vulkan
	} else if(settings.surface == SurfaceType::gl) {
		#ifdef NY_WithGl
			static constexpr auto wglFailed = "ny::WinapiAppContext::createWindowContext: "
				"initializing wgl failed, therefore no gl surfaces can be created";

		if(!wglSetup()) throw std::runtime_error(wglFailed);
		return std::make_unique<WglWindowContext>(*this, *wglSetup(), winapiSettings);
		#else
			static constexpr auto noWgl = "ny::WinapiAppContext::createWindowContext: "
				"ny was built without gl/wgl support and can therefore not create a gl Surface";

			throw std::logic_error(noWgl);
		#endif //gl
	} else if(settings.surface == SurfaceType::buffer) {
		return std::make_unique<WinapiBufferWindowContext>(*this, winapiSettings);
	}

	return std::make_unique<WinapiWindowContext>(*this, winapiSettings);
}

MouseContext* WinapiAppContext::mouseContext()
{
	return &mouseContext_;
}

KeyboardContext* WinapiAppContext::keyboardContext()
{
	return &keyboardContext_;
}

bool WinapiAppContext::dispatchEvents()
{
	receivedQuit_ = false;
	MSG msg;

	while(::PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) ::DispatchMessage(&msg);
	return !receivedQuit_;
}

bool WinapiAppContext::dispatchLoop(LoopControl& control)
{
	receivedQuit_ = false;
	WinapiLoopImpl loopImpl(control);

	MSG msg;
	while(!receivedQuit_ && loopImpl.run.load()) {
		while(auto func = loopImpl.popFunction()) func();

		auto ret = ::GetMessage(&msg, nullptr, 0, 0);
		if(ret == -1) {
			warning(winapi::errorMessage("ny::WinapiAppContext::dispatchLoop: GetMessage error"));
			return false;
		} else {
			::DispatchMessage(&msg);
		}
	}

	return !receivedQuit_;
}

// TODO: error handling (warnings)
bool WinapiAppContext::clipboard(std::unique_ptr<DataSource>&& source)
{
	winapi::com::DataObjectImpl* dataObj {};
	try {
		dataObj = new winapi::com::DataObjectImpl(std::move(source));
	} catch(const std::exception& err) {
		warning("ny::WinapiAppContext::clipboard(set): DataObject failed: ", err.what());
		return false;
	}

	auto ret = ::OleSetClipboard(dataObj);
	if(ret == S_OK) return true;

	warning("ny::WinapiAppContext::clipboard(set): OleSetClipboard failed with code ", ret);
	return false;
}

DataOffer* WinapiAppContext::clipboard()
{
	auto seq = ::GetClipboardSequenceNumber();
	if(!clipboardOffer_ || seq > clipboardSequenceNumber_) {
		clipboardOffer_ = nullptr;
		clipboardSequenceNumber_ = seq;

		IDataObject* obj;
		::OleGetClipboard(&obj);
		if(obj) clipboardOffer_ = std::make_unique<WinapiDataOffer>(*obj);
	}

	return clipboardOffer_.get();
}

bool WinapiAppContext::startDragDrop(std::unique_ptr<DataSource>&& source)
{
	//TODO: make non blocking
	//TODO: catch dataObject constructor exception

	// TODO
	// dndCond.notify_one();
	// dndSource = std::move(source);
	// return true;

	auto dataObj = new winapi::com::DataObjectImpl(std::move(source));
	auto dropSource = new winapi::com::DropSourceImpl();

	DWORD effect;
	auto ret = ::DoDragDrop(dataObj, dropSource, DROPEFFECT_COPY, &effect);
	return (ret == S_OK || ret == DRAGDROP_S_DROP || ret == DRAGDROP_S_CANCEL);
}

WinapiWindowContext* WinapiAppContext::windowContext(HWND w)
{
	auto ptr = ::GetWindowLongPtr(w, GWLP_USERDATA);
	return ptr ? reinterpret_cast<WinapiWindowContext*>(ptr) : nullptr;
}

//wndProc
LRESULT WinapiAppContext::eventProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	auto wc = windowContext(window);

	WinapiEventData eventData;
	eventData.windowContext = wc;
	eventData.window = window;
	eventData.message = message;
	eventData.wparam = wparam;
	eventData.lparam = lparam;

	LRESULT result = 0;

	if(!wc) return ::DefWindowProc(window, message, wparam, lparam);

	switch(message) {
		case WM_PAINT: {
			DrawEvent de;
			de.eventData = &eventData;
			wc->listener().draw(de);
			result = ::DefWindowProc(window, message, wparam, lparam); // to validate the window
			break;
		}

		case WM_DESTROY: {
			CloseEvent ce;
			ce.eventData = &eventData;
			wc->listener().close(ce);
			break;
		}

		case WM_SIZE: {
			SizeEvent se;
			se.eventData = &eventData;
			se.size = nytl::Vec2ui(LOWORD(lparam), HIWORD(lparam));
			se.edges = WindowEdge::none;
			wc->listener().resize(se);
			break;
		}

		case WM_SYSCOMMAND: {
			if(wc) {
				ToplevelState state;

				if(wparam == SC_MAXIMIZE) state = ToplevelState::maximized;
				else if(wparam == SC_MINIMIZE) state = ToplevelState::minimized;
				else if(wparam == SC_RESTORE) state = ToplevelState::normal;
				else if(wparam >= SC_SIZE && wparam <= SC_SIZE + 8) {
					auto currentCursor = ::GetClassLongPtr(wc->handle(), -12);

					auto edge = winapiToEdges(wparam - SC_SIZE);
					auto cursor = sizeCursorFromEdge(edge);
					wc->cursor(cursor);

				result = ::DefWindowProc(window, message, wparam, lparam);
					::SetClassLongPtr(wc->handle(), -12, currentCursor);

					break;
				}

				StateEvent se;
				se.eventData = &eventData;
				se.state = state;
				se.shown = IsWindowVisible(window);
				wc->listener().state(se);
			}

			result = ::DefWindowProc(window, message, wparam, lparam);
			break;
		}

		case WM_GETMINMAXINFO: {
			if(wc) {
				::MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(lparam);
				mmi->ptMaxTrackSize.x = wc->maxSize()[0];
				mmi->ptMaxTrackSize.y = wc->maxSize()[1];
				mmi->ptMinTrackSize.x = wc->minSize()[0];
				mmi->ptMinTrackSize.y = wc->minSize()[1];
			}

			break;
		}

		case WM_ERASEBKGND: {
			// prevent the background erase
			result = 1;
			break;
		}

		// TODO: needed?
		// case WM_CLIPBOARDUPDATE: {
		// 	clipboardSequenceNumber_ = ::GetClipboardSequenceNumber();
		// 	clipboardOffer_.reset();
		//
		// 	IDataObject* obj;
		// 	::OleGetClipboard(&obj);
		// 	if(obj) clipboardOffer_ = std::make_unique<winapi::DataOfferImpl>(*obj);
		// 	break;
		// }

		// case WM_MOVE: {
		// 	nytl::Vec2i position(LOWORD(lparam), HIWORD(lparam));
		// 	if(wc) wc->listener().position(position, &eventData);
		// 	break;
		// }

		case WM_QUIT: {
			receivedQuit_ = 1;
			break;
		}

		default: {
			if(keyboardContext_.processEvent(eventData, result)) break;
			if(mouseContext_.processEvent(eventData, result)) break;

			result = ::DefWindowProc(window, message, wparam, lparam);
			break;
		}
	}

	return result;
}

std::vector<const char*> WinapiAppContext::vulkanExtensions() const
{
	#ifdef NY_WithVulkan
		return {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
	#else
		return {};
	#endif // WithVulkan
}

GlSetup* WinapiAppContext::glSetup() const
{
	#ifdef NY_WithGl
		return wglSetup();
	#else
		return nullptr;
	#endif // WithGl
}

WglSetup* WinapiAppContext::wglSetup() const
{
	#ifdef NY_WithGl
		if(impl_->wglFailed) return nullptr;

		if(!impl_->wglSetup.valid()) {
			try {
				impl_->wglSetup = {dummyWindow_};
			} catch(const std::exception& error) {
				warning("ny::WinapiAppContext::wglSetup: init failed: ", error.what());
				impl_->wglFailed = true;
				impl_->wglSetup = {};
				return nullptr;
			}
		}

		return &impl_->wglSetup;

	#else
		return nullptr;

	#endif // WithGl
}

} // namespace ny
