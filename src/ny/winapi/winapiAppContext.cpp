#include <ny/winapi/winapiAppContext.hpp>

#include <ny/winapi/winapiWindowContext.hpp>

#include <ny/error.hpp>
#include <ny/app.hpp>
#include <ny/event.hpp>
#include <ny/window.hpp>

#include <iostream>

namespace ny
{

winapiAppContext::winapiAppContext()
{
    instance_ = GetModuleHandle(nullptr);

    if(instance_ == nullptr)
    {
        throw std::runtime_error("winapiAppContext: could not get hInstance");
        return;
    }

    GetStartupInfo(&startupInfo_);

    GdiplusStartup(&gdiplusToken_, &gdiplusStartupInput_, nullptr);
}

winapiAppContext::~winapiAppContext()
{
    if(gdiplusToken_) GdiplusShutdown(gdiplusToken_);
}

bool winapiAppContext::mainLoop()
{
    MSG msg;
    BOOL ret = GetMessage(&msg, nullptr, 0, 0);

    if(ret == -1)
    {
        //error
        return 0;
    }

    else if(ret == 0)
    {
        //exit
        return 0;
    }

    else
    {
        //eventProc(msg.hwnd, msg.message, msg.wParam, msg.lParam);

        //ok
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        return 1;
    }
}

void winapiAppContext::registerContext(HWND w, winapiWindowContext* c)
{
    contexts_[w] = c;
}

void winapiAppContext::unregisterContext(HWND w)
{
    contexts_.erase(w);
}

void winapiAppContext::unregisterContext(winapiWindowContext* c)
{
    //todo: implement
}

winapiWindowContext* winapiAppContext::getWindowContext(HWND w)
{
    if(contexts_.find(w) != contexts_.end())
        return contexts_[w];

    return nullptr;
}

//wndProc
LRESULT winapiAppContext::eventProc(HWND handler, UINT message, WPARAM wparam, LPARAM lparam)
{
    //todo: implement all events correctly, look em up

    switch(message)
    {
        case WM_CREATE:
        {
            break;
        }

        case WM_MOUSEMOVE:
        {
            winapiWindowContext* w = getWindowContext(handler);
            if(!w) break;

            mouseMoveEvent ev;
            ev.handler = &w->getWindow();
            //e->position = ;
            //e->delta = ;
            nyMainApp()->mouseMove(ev);
            break;
        }

        case WM_MBUTTONDOWN:
        {

        }

        case WM_MBUTTONUP:
        {

        }

        case WM_PAINT:
        {
            winapiWindowContext* w = getWindowContext(handler);
            if(!w) break;

            drawEvent e;
            e.handler = &w->getWindow();
            e.backend = Winapi;
            nyMainApp()->windowDraw(e);
            break;
        }

        case WM_CLOSE:
        {
            winapiWindowContext* w = getWindowContext(handler);
            if(!w) break;

            destroyEvent e;
            e.handler = &w->getWindow();
            e.backend = Winapi;
            nyMainApp()->destroyHandler(e);
            break;
        }

        case WM_SIZE:
        {
            winapiWindowContext* w = getWindowContext(handler);
            if(!w) break;

            sizeEvent e;
            e.handler = &w->getWindow();
            e.backend = Winapi;
            //e->size = ;
            nyMainApp()->windowSize(e);
            break;
        }

        case WM_MOVE:
        {
            winapiWindowContext* w = getWindowContext(handler);
            if(!w) break;

            positionEvent e;
            e.handler = &w->getWindow();
            e.backend = Winapi;
            //e->position = ;
            nyMainApp()->windowPosition(e);
            break;
        }

        case WM_QUIT:
        {
            nyMainApp()->exitApp();
            break;
        }

        default:
            return DefWindowProc (handler, message, wparam, lparam);

    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK dummyWndProc(HWND a, UINT b, WPARAM c, LPARAM d)
{
    winapiAppContext* aco = (winapiAppContext*) nyMainApp()->getAppContext();
    //return DefWindowProc(a,b,c,d);
    return aco->eventProc(a,b,c,d);
}

}
