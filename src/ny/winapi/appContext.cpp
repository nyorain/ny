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

#ifdef NY_WithGl
	#include <ny/winapi/wgl.hpp>
#endif //Gl

#ifdef NY_WithVulkan
	#define VK_USE_PLATFORM_WIN32_KHR
	#include <ny/winapi/vulkan.hpp>
	#include <vulkan/vulkan.h>
#endif //Vulkan

#include <dlg/dlg.hpp>
#include <nytl/utf.hpp>
#include <nytl/scope.hpp>

#include <ole2.h>

#include <stdexcept>
#include <cstring>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>

// NOTE: we never actually call TranslateMessage since we translate keycodes manually
// and calling this function will interfer with our ToUnicode calls.
namespace ny {

struct WinapiAppContext::Impl {
#ifdef NY_WithGl
	WglSetup wglSetup;
	bool wglFailed;
#endif //GL
};

// winapi callbacks
LRESULT CALLBACK WinapiAppContext::wndProcCallback(HWND a, UINT b, WPARAM c, LPARAM d)
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
	if(res != S_OK) {
		dlg_warn("OleInitialize failed with code ", res);
	}

	// init dummy window (needed as clipboard viewer and opengl dummy window)
	dummyWindow_ = ::CreateWindow(L"STATIC", L"", WS_DISABLED, 0, 0, 10, 10, nullptr, nullptr,
		hinstance(), nullptr);

	// needed for wakeup
	mainThread_ = ::GetCurrentThreadId();
}

WinapiAppContext::~WinapiAppContext()
{
	impl_.reset();
	if(dummyWindow_) {
		::DestroyWindow(dummyWindow_);
	}
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
		#endif // vulkan
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
		#endif // gl
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

void WinapiAppContext::pollEvents()
{
	MSG msg;
	while(::PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
		::DispatchMessage(&msg);
	}
}

void WinapiAppContext::waitEvents()
{
	// wait for first msg
	MSG msg;
	auto ret = ::GetMessage(&msg, nullptr, 0, 0);
	if(ret == -1) {
		// msdn does not state if there are any additional reasons for error
		// but suggests that they should be treated as critical errors
		auto msg = winapi::errorMessage("WinapiAppContext::waitEvents: GetMessage");
		dlg_error(msg);
		throw std::runtime_error(msg);
	} else {
		::DispatchMessage(&msg);
	}

	// dispatch all events that are still pending
	pollEvents();
}

void WinapiAppContext::wakeupWait()
{
	::PostThreadMessage(mainThread_, WM_USER, 0, 0);
}

// TODO: error handling (warnings)
bool WinapiAppContext::clipboard(std::unique_ptr<DataSource>&& source)
{
	winapi::com::DataObjectImpl* dataObj {};
	try {
		dataObj = new winapi::com::DataObjectImpl(std::move(source));
	} catch(const std::exception& err) {
		dlg_warn("DataObject constructor failed: ", err.what());
		return false;
	}

	auto ret = ::OleSetClipboard(dataObj);
	if(ret == S_OK) {
		return true;
	}

	dlg_warn("OleSetClipboard failed with code {}", ret);
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
	// TODO: catch dataObject constructor exception
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
			if(wc->clientExtents().size == nytl::Vec2i {0, 0}) {
				break;
			}

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
			auto size = nytl::Vec2ui{LOWORD(lparam), HIWORD(lparam)};
			if(size == nytl::Vec2ui{0, 0}) {
				break;
			}

			SizeEvent se;
			se.eventData = &eventData;
			se.size = size;
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

		default: {
			if(keyboardContext_.processEvent(eventData, result)) {
				break;
			} else if(mouseContext_.processEvent(eventData, result)) {
				break;
			}

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
				dlg_warn("wgl init failed: {}", error.what());
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
