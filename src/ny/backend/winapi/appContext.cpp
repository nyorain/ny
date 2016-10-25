#include <ny/backend/winapi/appContext.hpp>
#include <ny/backend/winapi/windowContext.hpp>
#include <ny/backend/winapi/util.hpp>
#include <ny/backend/winapi/com.hpp>

#include <ny/backend/mouseContext.hpp>
#include <ny/backend/keyboardContext.hpp>
#include <ny/backend/events.hpp>

#include <ny/base/log.hpp>
#include <ny/base/event.hpp>
#include <ny/base/loopControl.hpp>
#include <ny/base/eventDispatcher.hpp>

#ifdef NY_WithGL
 #include <ny/backend/winapi/wgl.hpp>
#endif //Gl

#ifdef NY_WithVulkan
 #define VK_USE_PLATFORM_WIN32_KHR
 #include <ny/backend/winapi/vulkan.hpp>
 #include <vulkan/vulkan.h>
#endif //Vulkan

#include <nytl/utf.hpp>
#include <nytl/scope.hpp>

#include <ole2.h>

#include <stdexcept>
#include <cstring>

namespace ny
{

namespace
{

//LoopControl
class WinapiLoopControlImpl : public ny::LoopControlImpl
{
public:
	DWORD threadHandle;
	std::atomic<bool>* run;

public:
	WinapiLoopControlImpl(std::atomic<bool>& prun) : run(&prun)
	{
		threadHandle = ::GetCurrentThreadId();
	}

	virtual void stop() override
	{
		run->store(0);
		::PostThreadMessage(threadHandle, WM_USER, 0, 0);
	};
};

}

struct WinapiAppContext::Impl
{
#ifdef NY_WithGL
	WglSetup wglSetup;
#endif //GL
};

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

	//start gdiplus since some GdiDrawContext functions need it
	GetStartupInfo(&startupInfo_);
	GdiplusStartup(&gdiplusToken_, &gdiplusStartupInput_, nullptr);

	//needed for dnd and clipboard
	auto res = ::OleInitialize(nullptr);
	if(res != S_OK) warning("WinapiWC: OleInitialize failed with code ", res);

	//init dummy window (needed as clipboard viewer)
	//TODO: use this window also for dummy wgl context init (see wgl.cpp)
	dummyWindow_ = ::CreateWindow("STATIC", "", WS_DISABLED, 0, 0, 10, 10, nullptr, nullptr,
		hinstance(), nullptr);

	//we must load AddClipboardFormatListener dynamically since it does not link correctly
	//with mingw and is not supported for windows versions before vista
	//It is not really needed to call this function since we check for new clipboard content
	//anyways everytime clipboard() is called.
	//But this can free the old clipboard DataOffer object as soon as the clipboard
	//changes and create the new one.
	auto lib = ::LoadLibrary("User32.dll");
	if(lib)
	{
		auto func = ::GetProcAddress(lib, "AddClipboardFormatListener");
		if(!func) warning("ny::WinapiAC: Failed to retrieve AddClipboardFormatListener");
		else (reinterpret_cast<BOOL(*)(HWND)>(func))(dummyWindow_);
		::FreeLibrary(lib);
	}
}

WinapiAppContext::~WinapiAppContext()
{
	if(dummyWindow_) ::DestroyWindow(dummyWindow_);
	if(gdiplusToken_) Gdiplus::GdiplusShutdown(gdiplusToken_);
	::OleUninitialize();
}

std::unique_ptr<WindowContext> WinapiAppContext::createWindowContext(const WindowSettings& settings)
{
	WinapiWindowSettings winapiSettings;
	const WinapiWindowSettings* ws = dynamic_cast<const WinapiWindowSettings*>(&settings);

	if(ws) winapiSettings = *ws;
	else winapiSettings.WindowSettings::operator=(settings);

	auto contextType = settings.context;
	if(contextType == ContextType::gl)
	{
		#ifdef NY_WithGL
		 if(!impl_->wglSetup.valid()) impl_->wglSetup = {dummyWindow_};
		 return std::make_unique<WglWindowContext>(*this, impl_->wglSetup, winapiSettings);
		#else
		 throw std::logic_error("ny::WinapiAC::createWC: ny was built without gl support");
		#endif //gl
	}

	else if(contextType == ContextType::vulkan)
	{
		#ifdef NY_WithVulkan
		 return std::make_unique<WinapiVulkanWindowContext>(*this, winapiSettings);
		#else
		 throw std::logic_error("ny::WinapiAC::createWC: ny was built without vulkan support");
		#endif //vulkan
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
	for(auto& e : pendingEvents_) if(e->handler) e->handler->handleEvent(*e);
	pendingEvents_.clear();

	receivedQuit_ = false;
	MSG msg;

	while(::PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}

	return !receivedQuit_;
}

bool WinapiAppContext::dispatchLoop(LoopControl& control)
{
	//TODO: we have to be really careful of exceptions inside this high functions
	//if everything during event handling throws, the end of this function will
	//not be reached
	receivedQuit_ = false;
	std::atomic<bool> run {1};
	control.impl_ = std::make_unique<WinapiLoopControlImpl>(run);
	dispatcherLoopControl_ = &control;

	auto scopeGuard = nytl::makeScopeGuard([&]{
		dispatcherLoopControl_ = nullptr;
		control.impl_.reset();
	});

	MSG msg;
	while(!receivedQuit_ && run.load())
	{
		for(auto& e : pendingEvents_) if(e->handler) e->handler->handleEvent(*e);
		pendingEvents_.clear();

		auto ret = ::GetMessage(&msg, nullptr, 0, 0);
		if(ret == -1)
		{
			warning(errorMessage("WinapiAC::dispatchLoop"));
			return false;
		}
		else
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}

	return !receivedQuit_;
}

bool WinapiAppContext::threadedDispatchLoop(EventDispatcher& dispatcher,
	LoopControl& control)
{
	receivedQuit_ = false;
	std::atomic<bool> run {1};
	control.impl_ = std::make_unique<WinapiLoopControlImpl>(run);
	receivedQuit_ = false;

	auto threadid = std::this_thread::get_id();
	auto threadHandle = ::GetCurrentThreadId();
	eventDispatcher_ = &dispatcher;

	//register a callback that is called everytime the dispatcher gets an event
	//if the event comes from this thread, this thread is not waiting, otherwise
	//wait this thread with a message.
	auto conn = nytl::makeConnection(dispatcher.onDispatch, dispatcher.onDispatch.add([&] {
		if(std::this_thread::get_id() != threadid)
			PostThreadMessage(threadHandle, WM_USER, 0, 0);
	}));

	//exception safety
	auto scopeGuard = nytl::makeScopeGuard([&]{
		eventDispatcher_ = nullptr;
		dispatcherLoopControl_ = nullptr;
		control.impl_.reset();
	});

	MSG msg;
	while(!receivedQuit_ && run.load())
	{
		for(auto& e : pendingEvents_) if(e->handler) e->handler->handleEvent(*e);
		pendingEvents_.clear();

		auto ret = ::GetMessage(&msg, nullptr, 0, 0);
		if(ret == -1)
		{
			warning(errorMessage("WinapiAC::dispatchLoop"));
			return false;
		}
		else
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}

		dispatcher.processEvents();
	}

	return !receivedQuit_;
}

//TODO: error handling (warnings)
bool WinapiAppContext::clipboard(std::unique_ptr<DataSource>&& source)
{
	//OleSetClipboard
	auto dataObj = new winapi::com::DataObjectImpl(std::move(source));
	return(::OleSetClipboard(dataObj) == S_OK);
}

DataOffer* WinapiAppContext::clipboard()
{
	auto seq = ::GetClipboardSequenceNumber();
	if(!clipboardOffer_ || seq > clipboardSequenceNumber_)
	{
		clipboardOffer_ = nullptr;
		clipboardSequenceNumber_ = seq;

		IDataObject* obj;
		::OleGetClipboard(&obj);
		if(obj) clipboardOffer_ = std::make_unique<winapi::DataOfferImpl>(*obj);
	}

	return clipboardOffer_.get();
}

bool WinapiAppContext::startDragDrop(std::unique_ptr<DataSource>&& source)
{
	// modalThreads_.emplace_back([source = std::move(source)]() mutable {
	// 	::OleInitialize(nullptr);
	// 	// auto res = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	// 	auto dataObj = new winapi::com::DataObjectImpl(std::move(source));
	// 	auto dropSource = new winapi::com::DropSourceImpl();
	//
	// 	DWORD effect;
	// 	::DoDragDrop(dataObj, dropSource, DROPEFFECT_COPY, &effect);
	// 	// ::CoUninitialize();
	// 	::OleUninitialize();
	// });
	// modalThreads_.back().join();
	//

	auto dataObj = new winapi::com::DataObjectImpl(std::move(source));
	auto dropSource = new winapi::com::DropSourceImpl();

	DWORD effect;
	return(::DoDragDrop(dataObj, dropSource, DROPEFFECT_COPY, &effect) == S_OK);
}

WinapiWindowContext* WinapiAppContext::windowContext(HWND w)
{
	auto ptr = ::GetWindowLongPtr(w, GWLP_USERDATA);
	return ptr ? reinterpret_cast<WinapiWindowContext*>(ptr) : nullptr;
}

void WinapiAppContext::dispatch(Event&& event)
{
	pendingEvents_.push_back(nytl::cloneMove(event));
}

//wndProc
LRESULT WinapiAppContext::eventProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	//todo: implement all events correctly, look em up
	auto context = windowContext(window);
	auto handler = context ? context->eventHandler() : nullptr;

	//utilty function used to dispatch event
	auto dispatch = [&](Event& ev) {
		if(eventDispatcher_) eventDispatcher_->dispatch(std::move(ev));
		else if(ev.handler) ev.handler->handleEvent(ev);
	};

	//to be returned
	LRESULT result = 0;

	switch(message)
	{
		case WM_CREATE:
		{
			result = ::DefWindowProc(window, message, wparam, lparam);
			break;
		}

		case WM_MOUSEMOVE:
		{
			Vec2i pos(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
			POINT point {pos.x, pos.y};
			::ClientToScreen(window, &point);
			auto delta = mouseContext_.move(pos);

			if(!mouseContext_.over() || mouseContext_.over()->handle() != window)
			{
				if(handler)
				{
					MouseCrossEvent ev(handler);
					ev.entered = true;
					ev.position = pos;
					dispatch(ev);
				}

				if(context) mouseContext_.over(context);
				else mouseContext_.over(nullptr);
			}

			if(handler)
			{
				MouseMoveEvent ev(handler);
				ev.position = pos;
				ev.screenPosition = nytl::Vec2i(point.x, point.y);
				ev.delta = delta;
				dispatch(ev);
			}

			break;
		}

		case WM_MOUSELEAVE:
		{
			mouseContext_.over(nullptr);

			if(handler)
			{
				MouseCrossEvent ev(handler);
				ev.entered = false;
				dispatch(ev);
			}

			break;
		}

		case WM_LBUTTONDOWN:
		{
			mouseContext_.onButton(mouseContext_, MouseButton::left, true);

			if(handler)
			{
				MouseButtonEvent ev(handler);
				ev.position = {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
				ev.pressed = true;
				ev.button = MouseButton::left;
				dispatch(ev);
			}

			break;
		}

		case WM_LBUTTONUP:
		{
			mouseContext_.onButton(mouseContext_, MouseButton::left, false);

			if(handler)
			{
				MouseButtonEvent ev(handler);
				ev.position = {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
				ev.pressed = false;
				ev.button = MouseButton::left;
				dispatch(ev);
			}

			break;
		}

		case WM_RBUTTONDOWN:
		{
			mouseContext_.onButton(mouseContext_, MouseButton::right, true);

			if(handler)
			{
				MouseButtonEvent ev(handler);
				ev.position = {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
				ev.pressed = true;
				ev.button = MouseButton::right;
				dispatch(ev);
			}

			break;
		}

		case WM_RBUTTONUP:
		{
			mouseContext_.onButton(mouseContext_, MouseButton::right, false);

			if(handler)
			{
				MouseButtonEvent ev(handler);
				ev.position = {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
				ev.pressed = false;
				ev.button = MouseButton::right;
				dispatch(ev);
			}

			break;
		}

		case WM_MBUTTONDOWN:
		{
			mouseContext_.onButton(mouseContext_, MouseButton::middle, true);

			if(handler)
			{
				MouseButtonEvent ev(handler);
				ev.position = {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
				ev.pressed = true;
				ev.button = MouseButton::middle;
				dispatch(ev);
			}

			break;
		}

		case WM_MBUTTONUP:
		{
			mouseContext_.onButton(mouseContext_, MouseButton::middle, false);

			if(handler)
			{
				MouseButtonEvent ev(handler);
				ev.position = {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
				ev.pressed = false;
				ev.button = MouseButton::middle;
				dispatch(ev);
			}

			break;
		}

		case WM_XBUTTONDOWN:
		{
			auto button = (HIWORD(wparam) == 1) ? MouseButton::custom1 : MouseButton::custom2;
			mouseContext_.onButton(mouseContext_, button, true);

			if(handler)
			{
				MouseButtonEvent ev(handler);
				ev.position = {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
				ev.pressed = true;
				ev.button = button;
				dispatch(ev);
			}

			break;
		}

		case WM_XBUTTONUP:
		{
			auto button = (HIWORD(wparam) == 1) ? MouseButton::custom1 : MouseButton::custom2;
			mouseContext_.onButton(mouseContext_, button, false);

			if(handler)
			{
				MouseButtonEvent ev(handler);
				ev.position = {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
				ev.pressed = false;
				ev.button = button;
				dispatch(ev);
			}

			break;
		}

		case WM_MOUSEWHEEL:
		{
			float value = GET_WHEEL_DELTA_WPARAM(wparam);
			mouseContext_.onWheel(mouseContext_, value);

			if(handler)
			{
				MouseWheelEvent ev(handler);
				ev.value = value;
				dispatch(ev);
			}

			break;
		}

		case WM_SETFOCUS:
		{
			if(context || keyboardContext_.focus()) keyboardContext_.focus(context);

			if(handler)
			{
				FocusEvent ev(handler);
				ev.focus = true;
				dispatch(ev);
			}

			break;
		}

		case WM_KILLFOCUS:
		{
			if(keyboardContext_.focus() == context) keyboardContext_.focus(nullptr);

			if(handler)
			{
				FocusEvent ev(handler);
				ev.focus = false;
				dispatch(ev);
			}

			break;
		}

		case WM_KEYDOWN:
		{
			auto keycode = winapiToKeycode(wparam);
			auto utf8 = keyboardContext_.utf8(keycode, true);
			keyboardContext_.onKey(keyboardContext_, keycode, utf8, true);

			if(handler)
			{
				KeyEvent ev(handler);
				ev.keycode = keycode;
				ev.unicode = utf8;
				ev.pressed = true;
				dispatch(ev);
			}

			break;
		}

		case WM_KEYUP:
		{
			auto keycode = winapiToKeycode(wparam);
			auto utf8 = keyboardContext_.utf8(keycode, true);
			keyboardContext_.onKey(keyboardContext_, keycode, utf8, false);

			if(handler)
			{
				KeyEvent ev(handler);
				ev.keycode = keycode;
				ev.unicode = utf8;
				ev.pressed = false;
				dispatch(ev);
			}

			break;
		}

		case WM_PAINT:
		{
			if(handler)
			{
				DrawEvent ev(handler);
				dispatch(ev);
			}

			result = ::DefWindowProc(window, message, wparam, lparam); //to validate the window
			break;
		}

		case WM_DESTROY:
		{
			if(handler)
			{
				CloseEvent ev(handler);
				dispatch(ev);
			}

			break;
		}

		case WM_SIZE:
		{
			if(handler)
			{
				SizeEvent ev(handler);
				ev.size.x = LOWORD(lparam);
				ev.size.y = HIWORD(lparam);
				dispatch(ev);
			}

			break;
		}

		case WM_MOVE:
		{
			if(handler)
			{
				PositionEvent ev(handler);
				ev.position.x = LOWORD(lparam);
				ev.position.y = HIWORD(lparam);
				dispatch(ev);
			}

			break;
		}

		case WM_SYSCOMMAND:
		{
			if(handler)
			{
				ShowEvent ev(handler);
				ev.show = true;

				if(wparam == SC_MAXIMIZE)
				{
					ev.state = ToplevelState::maximized;
					dispatch(ev);
				}
				else if(wparam == SC_MINIMIZE)
				{
					ev.state = ToplevelState::minimized;
					dispatch(ev);
				}
				else if(wparam == SC_RESTORE)
				{
					ev.state = ToplevelState::normal;
					dispatch(ev);
				}
			}

			result = ::DefWindowProc(window, message, wparam, lparam);
			break;
		}

		case WM_ERASEBKGND:
		{
			result = 1;
			break;
		}

		case WM_CLIPBOARDUPDATE:
		{
			clipboardSequenceNumber_ = ::GetClipboardSequenceNumber();
			clipboardOffer_.reset();

			IDataObject* obj;
			::OleGetClipboard(&obj);
			if(obj) clipboardOffer_ = std::make_unique<winapi::DataOfferImpl>(*obj);
			break;
		}

		case WM_QUIT:
		{
			receivedQuit_ = 1;
			if(dispatcherLoopControl_) dispatcherLoopControl_->stop();
			break;
		}

		default:
		{
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
	#endif
}

}
