#include "backends/winapi/appContext.hpp"

#include "backends/winapi/windowContext.hpp"

#include "app/error.hpp"
#include "app/app.hpp"
#include "app/event.hpp"
#include "window/windowEvents.hpp"

#include <iostream>

namespace ny
{

winapiAppContext::winapiAppContext() : appContext(), m_instance(0)
{
    m_instance = GetModuleHandle(NULL);

    if(m_instance == NULL)
    {
        throw error(error::Critical, "winapiAppContext: could not get hInstance");
        return;
    }

    GetStartupInfo(&m_startupInfo);

    GdiplusStartup(&m_gdiplusToken, &m_gdiplusStartupInput, NULL);
}

winapiAppContext::~winapiAppContext()
{
    GdiplusShutdown(m_gdiplusToken);
}

bool winapiAppContext::mainLoopCall()
{
    MSG msg;
    BOOL ret = GetMessage(&msg,NULL,0,0);

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
    m_contexts[w] = c;
}

void winapiAppContext::unregisterContext(HWND w)
{
    m_contexts.erase(w);
}

void winapiAppContext::unregisterContext(winapiWindowContext* c)
{
    //todo: implement
}

winapiWindowContext* winapiAppContext::getWindowContext(HWND w)
{
    if(m_contexts.find(w) != m_contexts.end())
        return m_contexts[w];

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

            mouseMoveEvent* e = new mouseMoveEvent();
            e->handler = w->getWindow();
            //e->position = ;
            //e->delta = ;
            getMainApp()->mouseMove(e);
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

            drawEvent* e = new drawEvent();
            e->handler = w->getWindow();
            e->backend = Winapi;
            getMainApp()->windowDraw(e);
            break;
        }

        case WM_CLOSE:
        {
            winapiWindowContext* w = getWindowContext(handler);
            if(!w) break;

            closeEvent* e = new closeEvent();
            e->handler = w->getWindow();
            e->backend = Winapi;
            getMainApp()->windowClose(e);
            break;
        }

        case WM_SIZE:
        {
            winapiWindowContext* w = getWindowContext(handler);
            if(!w) break;

            sizeEvent* e = new sizeEvent();
            e->handler = w->getWindow();
            e->backend = Winapi;
            //e->size = ;
            getMainApp()->windowSize(e);
            break;
        }

        case WM_MOVE:
        {
            winapiWindowContext* w = getWindowContext(handler);
            if(!w) break;

            positionEvent* e = new positionEvent();
            e->handler = w->getWindow();
            e->backend = Winapi;
            //e->position = ;
            getMainApp()->windowPosition(e);
            break;
        }

        case WM_QUIT:
        {
            getMainApp()->exitApp();
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
    winapiAppContext* aco = (winapiAppContext*) getMainApp()->getAppContext();
    //return DefWindowProc(a,b,c,d);
    return aco->eventProc(a,b,c,d);
}

}
