#include <ny/winapi/winapiWindowContext.hpp>
#include <ny/winapi/winapiAppContext.hpp>
#include <ny/winapi/gdiDrawContext.hpp>

#include <ny/app.hpp>
#include <ny/error.hpp>
#include <ny/window.hpp>

#include <tchar.h>

namespace ny
{

unsigned int winapiWindowContext::highestID = 0;

winapiWindowContext::winapiWindowContext(window& win, const winapiWindowContextSettings& settings) : windowContext(win, settings)
{
    winapiAppContext* ac = dynamic_cast<winapiAppContext*> (nyMainApp()->getAppContext());

    if(!ac)
    {
        throw std::runtime_error("winapiWindowContext::create: winapiAppContext not correctly set");
        return;
    }

    instance_ = ac->getInstance();

    if(!instance_)
    {
        throw std::runtime_error("winapiWindowContext::create: hInstance invalid");
        return;
    }

    highestID++;

    std::string name = "regName" + std::to_string(highestID);

    wndClass_.hInstance = instance_;
    wndClass_.lpszClassName = _T(name.c_str());
    wndClass_.lpfnWndProc = &dummyWndProc;
    wndClass_.style = CS_DBLCLKS;
    wndClass_.cbSize = sizeof (WNDCLASSEX);
    wndClass_.hIcon = LoadIcon (nullptr, IDI_APPLICATION);
    wndClass_.hIconSm = LoadIcon (nullptr, IDI_APPLICATION);
    wndClass_.hCursor = LoadCursor (nullptr, IDC_ARROW);
    wndClass_.lpszMenuName = nullptr;
    wndClass_.cbClsExtra = 0;
    wndClass_.cbWndExtra = 0;
    wndClass_.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);

    if (!RegisterClassEx(&wndClass_))
    {
        throw std::runtime_error("winapiWindowContext::create: could not register window class");
        return;
    }

    handle_ = CreateWindowEx(0, wndClass_.lpszClassName, _T("title here"), WS_OVERLAPPEDWINDOW, win.getPosition().x, win.getPosition().y, win.getSize().x, win.getSize().y, HWND_DESKTOP, NULL, instance_, NULL);

    if(!handle_)
    {
        throw std::runtime_error("winapiWindowContext::create: could not create window");
        return;
    }

    ShowWindow(handle_, 10); //why 10? more generic?
    UpdateWindow(handle_);

    ac->registerContext(handle_, this);

    gdi_ = new gdiDrawContext(*this);
}

winapiWindowContext::~winapiWindowContext()
{
    CloseWindow(handle_);
}

void winapiWindowContext::refresh()
{
    RedrawWindow(handle_, nullptr, nullptr, RDW_INVALIDATE | RDW_NOERASE);
}

drawContext* winapiWindowContext::beginDraw()
{
    gdi_->beginDraw();
    return gdi_;
}
void winapiWindowContext::finishDraw()
{
    gdi_->finishDraw();
}

void winapiWindowContext::show()
{
}
void winapiWindowContext::hide()
{
}

/*
void winapiWindowContext::setWindowHints(const unsigned long hints)
{
}
*/
void winapiWindowContext::addWindowHints(const unsigned long hint)
{
}
void winapiWindowContext::removeWindowHints(const unsigned long hint)
{
}

/*
void winapiWindowContext::setContextHints(const unsigned long hints)
{
}
*/
void winapiWindowContext::addContextHints(const unsigned long hints)
{
}
void winapiWindowContext::removeContextHints(const unsigned long hints)
{
}

/*
void winapiWindowContext::setSettings(const windowContextSettings& s)
{
}
*/

void winapiWindowContext::setSize(vec2ui size, bool change)
{
}
void winapiWindowContext::setPosition(vec2i position, bool change)
{
}

void winapiWindowContext::setCursor(const cursor& c)
{
}

//////////////////////////////////////////////////
//
/*
winapiToplevelWindowContext::winapiToplevelWindowContext(toplevelWindow* win, const winapiWindowContextSettings& settings) : windowContext(win, settings), toplevelWindowContext(win, settings), winapiWindowContext(win, settings)
{
    winapiAppContext* ac = dynamic_cast<winapiAppContext*> (nyMainApp()->getAppContext());

    if (!RegisterClassEx(&m_wndClass))
    {
        throw std::runtime_error("winapiWindowContext::create: could not register window class");
        return;
    }

    m_handle = CreateWindowEx(0, m_wndClass.lpszClassName, _T(win->getName().c_str()), WS_OVERLAPPEDWINDOW, win->getPosition().x, win->getPosition().y, win->getSize().x, win->getSize().y, HWND_DESKTOP, NULL, m_instance, NULL);


    if(!m_handle)
    {
        throw std::runtime_error("winapiWindowContext::create: could not create window");
        return;
    }

    ShowWindow(m_handle, 10);
    UpdateWindow(m_handle);

    ac->registerContext(m_handle, this);
}


void winapiToplevelWindowContext::setMaximized()
{
}
void winapiToplevelWindowContext::setMinimized()
{
}
void winapiToplevelWindowContext::setFullscreen()
{
}

void winapiToplevelWindowContext::setNormal()
{
}
void winapiToplevelWindowContext::setMinSize()
{
}
void winapiToplevelWindowContext::setMaxSize()
{
}

void winapiToplevelWindowContext::beginMove(mouseButtonEvent* ev)
{
}
void winapiToplevelWindowContext::beginResize(mouseButtonEvent* ev, windowEdge edges)
{
}


//////////////////////////////////////////////////////
//
winapiChildWindowContext::winapiChildWindowContext(childWindow* win, const winapiWindowContextSettings& settings) : windowContext(win, settings), childWindowContext(win, settings), winapiWindowContext(win, settings)
{
}
*/
}
