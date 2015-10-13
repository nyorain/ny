#include <ny/winapi/winapiWindowContext.hpp>
#include <ny/winapi/winapiAppContext.hpp>
#include <ny/winapi/gdiDrawContext.hpp>
#include <ny/winapi/wgl.hpp>

#include <ny/app.hpp>
#include <ny/error.hpp>
#include <ny/window.hpp>

#include <tchar.h>

namespace ny
{

unsigned int winapiWindowContext::highestID = 0;

bool usingGLWin(preference glPref)
{
    //WithGL
    #if (!defined NY_WithGL)
     if(glPref == preference::Must) throw std::runtime_error("winapiWC::winapiWC: no gl renderer available, glPreference was set to MUST");
     else return 0;

    #else
     if(glPref == preference::Must || glPref == preference::Should) return 1;
     else return 0;

    #endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//windowContext///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
winapiWindowContext::winapiWindowContext(window& win, const winapiWindowContextSettings& settings) : windowContext(win, settings)
{
    //init check
    winapiAppContext* ac = nyWinapiAppContext();

    if(!ac) throw std::runtime_error("winapiWindowContext::create: winapiAppContext not correctly set");
    if(!ac->getInstance()) throw std::runtime_error("winapiWindowContext::create: hInstance invalid");

    //toplevel or child, gl or gdi
    bool gl = usingGLWin(settings.glPref);
    nyDebug("pref: ", (unsigned int)settings.glPref);
    nyDebug("gl: ", gl);

    auto* toplvlw = dynamic_cast<toplevelWindow*>(&win);
    auto* childw = dynamic_cast<childWindow*>(&win);

    HWND parentHandle = nullptr;
    std::string windowName;

    if((!toplvlw && !childw) || (toplvlw && childw)) throw std::runtime_error("winapiWC::winapiWC: window must be either of childWindow or of toplevelWindow type");
    else if(toplvlw)
    {
        parentHandle = HWND_DESKTOP;
        windowName = toplvlw->getTitle();
    }
    else if(childw)
    {
        winapiWC* parentWC = dynamic_cast<winapiWindowContext*>(childw->getParent()->getWC());
        if(!parentWC || !(parentHandle = parentWC->getHandle())) throw std::runtime_error("winapiWC::winapiWC: could not find xParent");

        windowName = "test: childWindowName";
    }

    //register window class
    highestID++;
    std::string name = "regName" + std::to_string(highestID);

    wndClass_.hInstance = ac->getInstance();
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

    if(gl)
    {
        wndClass_.style |= CS_OWNDC;
        wndClass_.hbrBackground = (HBRUSH) GetStockObject(COLOR_BACKGROUND);
    }
    else
    {
        wndClass_.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
    }

    if (!RegisterClassEx(&wndClass_))
    {
        throw std::runtime_error("winapiWindowContext::create: could not register window class");
        return;
    }

    //create and check
    handle_ = CreateWindowEx(0, wndClass_.lpszClassName, _T(windowName.c_str()), WS_OVERLAPPEDWINDOW, win.getPosition().x, win.getPosition().y, win.getSize().x,
                             win.getSize().y, parentHandle, nullptr, ac->getInstance(), this);

    if(!handle_)
    {
        auto code = GetLastError();
        throw std::runtime_error("winapiWindowContext::create: CreateWindowEx failed with error " + std::to_string(code));
    }

    //register this wc
    //todo: unregister if creation fails?
    ac->registerContext(handle_, this);

    //dc
    if(gl)
    {
        drawType_ = winapiDrawType::wgl;
        wgl_.reset(new wglDrawContext(*this));
    }
    else
    {
        drawType_ = winapiDrawType::gdi;
        gdi_.reset(new gdiDrawContext(*this));
    }

    //here?
    ShowWindow(handle_, 10); //why 10? more generic?
    UpdateWindow(handle_);
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
    if(getGDI())
    {
        gdi_->beginDraw();
        return gdi_.get();
    }
    else if(getWGL())
    {
        wgl_->makeCurrent();
        return wgl_.get();
    }
    else
    {
        nyWarning("winapiWC::beginDraw: no valid drawContext.");
        return nullptr;
    }
}
void winapiWindowContext::finishDraw()
{
    if(getGDI())
    {
        gdi_->finishDraw();
    }
    else if(getWGL())
    {
        wgl_->swapBuffers(); //todo check
        wgl_->makeNotCurrent();
    }
    else
    {
        //todo
    }
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

void winapiWindowContext::processEvent(const contextEvent& e)
{
    if(e.contextType() == eventType::contextCreate)
    {
        if(getWGL())
            if(!getWGL()->setupContext())
                nyError("winapiWC::failed to init wgl context.");
    }
}

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
