#include <ny/backend/winapi/appContext.hpp>
#include <ny/backend/winapi/windowContext.hpp>
#include <ny/backend/winapi/util.hpp>

#include <ny/base/log.hpp>
#include <ny/base/event.hpp>
#include <ny/base/loopControl.hpp>
#include <ny/window/events.hpp>
#include <ny/app/mouse.hpp>
#include <ny/app/keyboard.hpp>
#include <ny/app/eventDispatcher.hpp>
#include <ny/draw/font.hpp>

#include <windowsx.h>
#include <ole2.h>

#include <stdexcept>
#include <cstring>

namespace ny
{

//todo - kinda hacky atm
namespace
{
	WinapiAppContext* gAC;
};

//LoopControl
class WinapiAppContext::LoopControlImpl : public ny::LoopControlImpl
{
public:
	DWORD threadHandle;
	std::atomic<bool>* run;

public:
	LoopControlImpl(std::atomic<bool>& prun) : run(&prun)
	{
		threadHandle = GetCurrentThreadId();
	}

	virtual void stop() override
	{
		run->store(0);
		PostThreadMessage(threadHandle, WM_USER, 0, 0);
	};
};

//winapi callbacks
LRESULT CALLBACK WinapiAppContext::wndProcCallback(HWND a, UINT b, WPARAM c, LPARAM d)
{
    return gAC->eventProc(a,b,c,d);
}

LRESULT CALLBACK WinapiAppContext::dlgProcCallback(HWND a, UINT b, WPARAM c, LPARAM d)
{
    return gAC->eventProc(a,b,c,d);
}

//WinapiAC
WinapiAppContext::WinapiAppContext()
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
	auto res = OleInitialize(nullptr);
	if(res != S_OK) warning("WinapiWC: OleInitialize failed with code ", res);

	gAC = this;
}

WinapiAppContext::~WinapiAppContext()
{
	Font::defaultFont().resetCache("ny::GdiFontHandle");
    if(gdiplusToken_) GdiplusShutdown(gdiplusToken_);
}

bool WinapiAppContext::dispatchEvents(EventDispatcher& dispatcher)
{
	receivedQuit_ = 0;
	eventDispatcher_ = &dispatcher;

    MSG msg;
    while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return !receivedQuit_;
}

bool WinapiAppContext::dispatchLoop(EventDispatcher& dispatcher, LoopControl& control)
{
	std::atomic<bool> run {1};
	control.impl_ = std::make_unique<LoopControlImpl>(run);

	dispatcherLoopControl_ = &control;
	eventDispatcher_ = &dispatcher;

	MSG msg;
	while(run.load())
	{
		auto ret = GetMessage(&msg, nullptr, 0, 0);
		if(ret == -1)
		{
			sendWarning(errorMessage("WinapiAC::dispatchLoop"));
			return false;
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	dispatcherLoopControl_ = nullptr;
	control.impl_.reset();
	return true;
}

bool WinapiAppContext::threadedDispatchLoop(ThreadedEventDispatcher& dispatcher,
	LoopControl& control)
{
	auto threadid = std::this_thread::get_id();
	auto threadHandle = GetCurrentThreadId();

	threadsafe_ = true;

	//register a callback that is called everytime the dispatcher gets an event
	//if the event comes from this thread, this thread is not waiting, otherwise
	//wait this thread with a message.
	auto conn = dispatcher.onDispatch.add([&] {
			if(std::this_thread::get_id() != threadid)
				PostThreadMessage(threadHandle, WM_USER, 0, 0);
		});


	//just call the default dispatch loop
	dispatchLoop(dispatcher, control);

	//unregistert the dispatcher callback
	conn.destroy();
	threadsafe_ = false;

	return true;
}

void WinapiAppContext::clipboard(const std::string& text) const
{
	if(!::OpenClipboard(nullptr)) return;
	if(!::EmptyClipboard()) return;

	auto handle = ::GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
	if(!handle)
	{
		::CloseClipboard();
		return;
	}

	auto ptr = ::GlobalLock(handle);
	if(!ptr)
	{
		::CloseClipboard();
		return;
	}

	std::memcpy(ptr, text.c_str(), text.size() + 1);
	::GlobalUnlock(handle);

	::SetClipboardData(CF_TEXT, handle);

	::CloseClipboard();
	//::GlobalFree(handle); //XXX: do this here? doc states no. memory leak?
}

std::string WinapiAppContext::clipboard() const
{
	std::string ret;
	if(!::OpenClipboard(nullptr)) return ret;

	auto handle = ::GetClipboardData(CF_TEXT);
	if(!handle)
	{
		::CloseClipboard();
		return ret;
	}

	auto ptr = ::GlobalLock(handle);
	if(!ptr)
	{
		::CloseClipboard();
		return ret;
	}

	ret = reinterpret_cast<const char*>(ptr);
	::CloseClipboard();
	return ret;
}

void WinapiAppContext::registerContext(HWND w, WinapiWindowContext& c)
{
    contexts_[w] = &c;
}

void WinapiAppContext::unregisterContext(HWND w)
{
    contexts_.erase(w);
}

WinapiWindowContext* WinapiAppContext::windowContext(HWND w)
{
    if(contexts_.find(w) != contexts_.end())
        return contexts_[w];

    return nullptr;
}

//wndProc
LRESULT WinapiAppContext::eventProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    //todo: implement all events correctly, look em up
	auto context = eventDispatcher_ ? windowContext(window) : nullptr;
	auto handler = context ? context->eventHandler() : nullptr;

	bool handlerEvents = (handler && eventDispatcher_);
	bool contextEvents = (context && eventDispatcher_);

	auto threadedDispatcher = dynamic_cast<ThreadedEventDispatcher*>(eventDispatcher_);

	//utilty function used to dispatch event
	auto dispatch = [&](auto&& ev) {
			if(threadsafe_) eventDispatcher_->send(std::move(ev));
			else eventDispatcher_->dispatch(std::move(ev));
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

			if(mouseOver_ != window)
			{
				mouseOver_ = window;
				if(handlerEvents)
				{
					MouseCrossEvent ev(handler);
					ev.entered = true;
					ev.position = position;
					dispatch(ev);
				}
			}

			if(handlerEvents)
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

			if(handlerEvents)
			{
				MouseCrossEvent ev(handler);
				ev.entered = false;
				dispatch(ev);
			}
			break;
		}

		case WM_LBUTTONDOWN:
        {
			if(handlerEvents)
			{
				MouseButtonEvent ev(handler);
				ev.position = {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
				ev.pressed = true;
				ev.button = Mouse::Button::left;
				dispatch(ev);
			}
			break;
        }

		case WM_LBUTTONUP:
        {
			if(handlerEvents)
			{
				MouseButtonEvent ev(handler);
				ev.position = {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
				ev.pressed = false;
				ev.button = Mouse::Button::left;
				dispatch(ev);
			}

			break;
        }

		case WM_KEYDOWN:
		{
			if(handlerEvents)
			{
				KeyEvent ev(handler);
				ev.key = winapiToKey(wparam);
				ev.pressed = true;
				eventDispatcher_->dispatch(std::move(ev));
			}

			break;
		}

		case WM_KEYUP:
		{
			if(handlerEvents)
			{
				KeyEvent ev(handler);
				ev.key = winapiToKey(wparam);
				ev.pressed = false;
				eventDispatcher_->dispatch(std::move(ev));
			}

			break;
		}

        case WM_PAINT:
        {
			if(handlerEvents)
			{
				DrawEvent ev(handler);
				dispatch(ev);
			}

            result = DefWindowProc(window, message, wparam, lparam); //to validate the window
			break;
        }

		case WM_DESTROY:
		{
			if(handlerEvents)
			{
				CloseEvent ev(handler);
				dispatch(ev);
			}

			break;
		}

        case WM_SIZE:
        {
			if(handlerEvents)
			{
				SizeEvent ev(handler);
				dispatch(ev);
			}

			break;
        }

        case WM_MOVE:
        {
			if(handlerEvents)
			{
				PositionEvent ev(handler);
				dispatch(ev);
			}

			break;
        }

		case WM_SYSCOMMAND:
		{
			if(handlerEvents)
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

	if(threadsafe_ && threadedDispatcher) threadedDispatcher->processEvents();
	return result;
}


}
