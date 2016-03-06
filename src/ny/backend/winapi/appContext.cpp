#include <ny/backend/winapi/appContext.hpp>
#include <ny/backend/winapi/windowContext.hpp>

#include <ny/base/log.hpp>
#include <ny/base/event.hpp>
#include <ny/base/loopControl.hpp>
#include <ny/window/events.hpp>
#include <ny/app/mouse.hpp>
#include <ny/app/keyboard.hpp>
#include <ny/app/eventDispatcher.hpp>

#include <stdexcept>

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

//WinapiAC
WinapiAppContext::WinapiAppContext()
{
    instance_ = GetModuleHandle(nullptr);

    if(instance_ == nullptr)
    {
        throw std::runtime_error("winapiAppContext: could not get hInstance");
        return;
    }

    GetStartupInfo(&startupInfo_);
    GdiplusStartup(&gdiplusToken_, &gdiplusStartupInput_, nullptr);

	gAC = this;
}

WinapiAppContext::~WinapiAppContext()
{
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
			//error
			sendWarning("WinapiAC::dispatchLoop: error code ", GetLastError());
			return false;
		}
		else if(ret == 0)
		{
			//quit
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
	auto context = windowContext(window);
	auto handler = context ? context->eventHandler() : nullptr;

	bool handlerEvents = (handler && eventDispatcher_);
	bool contextEvents = (context && eventDispatcher_);

    switch(message)
    {
        case WM_CREATE:
        {
            return 0;
        }

        case WM_MOUSEMOVE:
        {
			if(handlerEvents)
			{
				auto ev = std::make_unique<MouseMoveEvent>(handler);
				eventDispatcher_->dispatch(std::move(ev));
			}

			return 0;
        }

        case WM_MBUTTONDOWN:
        {

        }

        case WM_MBUTTONUP:
        {

        }

        case WM_PAINT:
        {
			if(handlerEvents)
			{
				auto ev = std::make_unique<DrawEvent>(handler);
				eventDispatcher_->dispatch(std::move(ev));
			}

            return DefWindowProc(window, message, wparam, lparam); //to validate the rgn
        }

		case WM_DESTROY:
		{
			if(handlerEvents)
			{
				auto ev = std::make_unique<CloseEvent>(handler);
				eventDispatcher_->dispatch(std::move(ev));
			}

			return 0;
		}

        case WM_SIZE:
        {
			if(handlerEvents)
			{
				auto ev = std::make_unique<SizeEvent>(handler);
				eventDispatcher_->dispatch(std::move(ev));
			}

			return 0;
        }

        case WM_MOVE:
        {
			if(handlerEvents)
			{
				auto ev = std::make_unique<PositionEvent>(handler);
				eventDispatcher_->dispatch(std::move(ev));
			}

			return 0;
        }

        case WM_ERASEBKGND:
        {
            return 1;
        }

		case WM_QUIT:
		{
			receivedQuit_ = 1;
			if(dispatcherLoopControl_) dispatcherLoopControl_->stop();
			return 0;
		}
    }

	return DefWindowProc (window, message, wparam, lparam);
}


LRESULT CALLBACK WinapiAppContext::wndProcCallback(HWND a, UINT b, WPARAM c, LPARAM d)
{
    return gAC->eventProc(a,b,c,d);
}

}
