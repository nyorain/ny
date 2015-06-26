#pragma once

#include "backends/appContext.hpp"

#include <map>
#include <windows.h>
#include <gdiplus.h>

using namespace Gdiplus;

namespace ny
{

class winapiAppContext;
class winapiWindowContext;
typedef winapiAppContext winapiAC;

class winapiAppContext : public appContext
{
public:
    typedef LRESULT CALLBACK (*wndProc)(HWND, UINT, WPARAM, LPARAM);

protected:
    HINSTANCE m_instance;
    STARTUPINFO m_startupInfo;

    std::map<HWND, winapiWindowContext*> m_contexts;

    GdiplusStartupInput m_gdiplusStartupInput;
    ULONG_PTR m_gdiplusToken;

public:
    winapiAppContext();
    ~winapiAppContext();

    virtual bool mainLoopCall();
    virtual LRESULT eventProc(HWND, UINT, WPARAM, LPARAM);

    void registerContext(HWND w, winapiWindowContext* c);
    void unregisterContext(HWND w);
    void unregisterContext(winapiWindowContext* c);
    winapiWindowContext* getWindowContext(HWND win);

    void setCursor(unsigned int cursorID);
    void setCursor(image* img);

    const HINSTANCE& getInstance() const { return m_instance; };
    const STARTUPINFO& getStartInfo() const { return m_startupInfo; };
};

LRESULT CALLBACK dummyWndProc(HWND a, UINT b, WPARAM c, LPARAM d);

}
