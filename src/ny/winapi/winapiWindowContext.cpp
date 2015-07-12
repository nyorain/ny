#include <ny/winapi/windowContext.hpp>
#include <ny/winapi/appContext.hpp"
#include <ny/winapi/gdiDrawContext.hpp"

#include <ny/app.hpp"
#include <ny/error.hpp"
#include <ny/window.hpp"

#include <tchar.h>

namespace ny
{

unsigned int winapiWindowContext::highestID = 0;

winapiWindowContext::winapiWindowContext(window* win, const winapiWindowContextSettings& settings) : windowContext(win, settings), m_instance(0)
{
    winapiAppContext* ac = dynamic_cast<winapiAppContext*> (getMainApp()->getAppContext());

    if(!ac)
    {
        throw std::runtime_error("winapiWindowContext::create: winapiAppContext not correctly set");
        return;
    }

    m_instance = ac->getInstance();

    if(!m_instance)
    {
        throw std::runtime_error("winapiWindowContext::create: hInstance invalid");
        return;
    }

    highestID++;

    std::string name = "regName" + std::to_string(highestID);

    m_wndClass.hInstance = m_instance;
    m_wndClass.lpszClassName = _T(name.c_str());
    m_wndClass.lpfnWndProc = &dummyWndProc;
    m_wndClass.style = CS_DBLCLKS;
    m_wndClass.cbSize = sizeof (WNDCLASSEX);
    m_wndClass.hIcon = LoadIcon (NULL, IDI_APPLICATION);
    m_wndClass.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
    m_wndClass.hCursor = LoadCursor (NULL, IDC_ARROW);
    m_wndClass.lpszMenuName = NULL;
    m_wndClass.cbClsExtra = 0;
    m_wndClass.cbWndExtra = 0;
    m_wndClass.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);

    m_drawContext = new gdiDrawContext(this);
}

winapiWindowContext::~winapiWindowContext()
{
    CloseWindow(m_handle);
}

void winapiWindowContext::refresh()
{
    //beginDraw();
    //m_window->draw(m_drawContext);
    //finishDraw();

    //RECT rect;
    //HRGN rgn;
    //GetWindowRect(m_handle, &rect);
    //GetWindowRgn(m_handle, rgn);
    //RedrawWindow(m_handle, &rect, rgn, 0);

    RedrawWindow(m_handle, NULL, NULL, RDW_INVALIDATE | RDW_NOERASE);
}

drawContext& winapiWindowContext::beginDraw()
{
    dynamic_cast<gdiDrawContext*>(m_drawContext)->beginDraw();
    return *m_drawContext;
}
void winapiWindowContext::finishDraw()
{
    dynamic_cast<gdiDrawContext*>(m_drawContext)->finishDraw();
}

void winapiWindowContext::show()
{
}
void winapiWindowContext::hide()
{
}

void winapiWindowContext::raise()
{
}
void winapiWindowContext::lower()
{
}

void winapiWindowContext::requestFocus()
{
}

void winapiWindowContext::setWindowHints(const unsigned long hints)
{
}
void winapiWindowContext::addWindowHints(const unsigned long hint)
{
}
void winapiWindowContext::removeWindowHints(const unsigned long hint)
{
}

void winapiWindowContext::setContextHints(const unsigned long hints)
{
}
void winapiWindowContext::addContextHints(const unsigned long hints)
{
}
void winapiWindowContext::removeContextHints(const unsigned long hints)
{
}

void winapiWindowContext::setSettings(const windowContextSettings& s)
{
}

void winapiWindowContext::setSize(vec2ui size, bool change)
{
}
void winapiWindowContext::setPosition(vec2i position, bool change)
{
}

void winapiWindowContext::setWindowCursor(const cursor& c)
{
}

//////////////////////////////////////////////////
//
winapiToplevelWindowContext::winapiToplevelWindowContext(toplevelWindow* win, const winapiWindowContextSettings& settings) : windowContext(win, settings), toplevelWindowContext(win, settings), winapiWindowContext(win, settings)
{
    winapiAppContext* ac = dynamic_cast<winapiAppContext*> (getMainApp()->getAppContext());

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

}
