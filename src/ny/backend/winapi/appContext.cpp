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

#include <windowsx.h>
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
		threadHandle = GetCurrentThreadId();
	}

	virtual void stop() override
	{
		run->store(0);
		PostThreadMessage(threadHandle, WM_USER, 0, 0);
	};
};

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
    instance_ = GetModuleHandle(nullptr);

	//XXX: is this check needed?
    if(instance_ == nullptr)
    {
        throw std::runtime_error("winapiAppContext: could not get hInstance");
        return;
    }

	//start gdiplus since some GdiDrawContext functions need it
    GetStartupInfo(&startupInfo_);
    GdiplusStartup(&gdiplusToken_, &gdiplusStartupInput_, nullptr);

	//needed for dnd and clipboard
	auto res = ::OleInitialize(nullptr);
	// auto res = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if(res != S_OK) warning("WinapiWC: OleInitialize failed with code ", res);
}

WinapiAppContext::~WinapiAppContext()
{
	// Font::defaultFont().resetCache("ny::GdiFontHandle");
    if(gdiplusToken_) Gdiplus::GdiplusShutdown(gdiplusToken_);
	OleUninitialize();
	// ::CoUninitialize();
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
		 return std::make_unique<WglWindowContext>(*this, winapiSettings);
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

    while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
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

		auto ret = GetMessage(&msg, nullptr, 0, 0);
		if(ret == -1)
		{
			warning(errorMessage("WinapiAC::dispatchLoop"));
			return false;
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
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
	auto threadHandle = GetCurrentThreadId();
	eventDispatcher_ = &dispatcher;

	//register a callback that is called everytime the dispatcher gets an event
	//if the event comes from this thread, this thread is not waiting, otherwise
	//wait this thread with a message.
	nytl::CbConnGuard conn = dispatcher.onDispatch.add([&] {
		if(std::this_thread::get_id() != threadid)
			PostThreadMessage(threadHandle, WM_USER, 0, 0);
	});

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
		
		auto ret = GetMessage(&msg, nullptr, 0, 0);
		if(ret == -1)
		{
			warning(errorMessage("WinapiAC::dispatchLoop"));
			return false;
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
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

std::unique_ptr<DataOffer> WinapiAppContext::clipboard()
{
	IDataObject* obj;
	::OleGetClipboard(&obj);
	if(!obj) return nullptr;
	return std::make_unique<winapi::DataOfferImpl>(*obj);
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
			result = DefWindowProc(window, message, wparam, lparam);
			break;
        }

        case WM_MOUSEMOVE:
        {
			Vec2i position{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};

			if(!mouseOver_ || mouseOver_->handle() != window)
			{
				mouseOver_ = windowContext(window);
				if(handler)
				{
					MouseCrossEvent ev(handler);
					ev.entered = true;
					ev.position = position;
					dispatch(ev);
				}
			}

			if(handler)
			{
				MouseMoveEvent ev(handler);
				ev.position = position;
				dispatch(ev);
			}

			break;
        }

		case WM_MOUSELEAVE:
		{
			mouseOver_ = nullptr;

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

		case WM_KEYDOWN:
		{
			if(handler)
			{
				KeyEvent ev(handler);
				ev.key = winapiToKey(wparam);
				ev.unicode = keyboardContext_.unicode(wparam);
				ev.pressed = true;
				dispatch(ev);
			}

			break;
		}

		case WM_KEYUP:
		{
			if(handler)
			{
				KeyEvent ev(handler);
				ev.key = winapiToKey(wparam);
				ev.unicode = keyboardContext_.unicode(wparam);
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

            result = DefWindowProc(window, message, wparam, lparam); //to validate the window
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

			result = DefWindowProc(window, message, wparam, lparam);
			break;
		}

        case WM_ERASEBKGND:
        {
			result = 1;
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
			result = DefWindowProc(window, message, wparam, lparam);
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
