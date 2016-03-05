#include <ny/backend/winapi/windowContext.hpp>
#include <ny/backend/winapi/appContext.hpp>
#include <ny/draw/drawContext.hpp>

#include <tchar.h>
#include <stdexcept>

namespace ny
{

//windowContext
WinapiWindowContext::WinapiWindowContext(WinapiAppContext& appContext,
	const WinapiWindowSettings& settings)
{
    //init check
	appContext_ = &appContext;

    if(!appContext.hinstance())
	{
		throw std::runtime_error("winapiWC::create: uninitialized appContext");
	}

	initWindowClass(settings);

	if (!RegisterClassEx(&wndClass_))
	{
		throw std::runtime_error("winapiWC::create: could not register window class");
		return;
	}

	initWindow(settings);
}

WinapiWindowContext::~WinapiWindowContext()
{
    if(handle_)
	{
		CloseWindow(handle_);
		appContext().unregisterContext(handle_);
	}
}

void WinapiWindowContext::initWindowClass(const WinapiWindowSettings& settings)
{
	static unsigned int highestID = 0;

	highestID++;
	std::string name = "ny::WinapiWindowClass" + std::to_string(highestID);

	wndClass_.hInstance = appContext().hinstance();
	wndClass_.lpszClassName = _T(name.c_str());
	wndClass_.lpfnWndProc = &WinapiAppContext::wndProcCallback;
	wndClass_.style = CS_DBLCLKS;
	wndClass_.cbSize = sizeof(WNDCLASSEX);
	wndClass_.hIcon = LoadIcon (nullptr, IDI_APPLICATION);
	wndClass_.hIconSm = LoadIcon (nullptr, IDI_APPLICATION);
	wndClass_.hCursor = LoadCursor (nullptr, IDC_ARROW);
	wndClass_.lpszMenuName = nullptr;
	wndClass_.cbClsExtra = 0;
	wndClass_.cbWndExtra = 0;
	wndClass_.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
}

void WinapiWindowContext::initWindow(const WinapiWindowSettings& settings)
{
	auto size = settings.size;
	auto position = settings.position;

	if(position.x == -1) position.x = CW_USEDEFAULT;
	if(position.y == -1) position.y = CW_USEDEFAULT;

	if(size.x == -1) size.x = CW_USEDEFAULT;
	if(size.y == -1) size.y = CW_USEDEFAULT;

	auto parent = static_cast<HWND>(settings.parent.pointer());
	auto hinstance = appContext().hinstance();
	auto style = WS_OVERLAPPEDWINDOW;

	handle_ = CreateWindowEx(0, wndClass_.lpszClassName, _T(settings.title.c_str()), style,
		position.x, position.y, size.x, size.y, parent, nullptr, hinstance, this);

	appContext().registerContext(handle_, *this);

	if(settings.initShown)
	{
		if(settings.initState == ToplevelState::maximized) ShowWindow(handle_, SW_SHOWMAXIMIZED);
		else if(settings.initState == ToplevelState::minimized) ShowWindow(handle_, SW_SHOWMINIMIZED);
		else ShowWindow(handle_, SW_SHOWDEFAULT);

		UpdateWindow(handle_);
	}
}

void WinapiWindowContext::refresh()
{
    RedrawWindow(handle_, nullptr, nullptr, RDW_INVALIDATE | RDW_NOERASE);
}

DrawGuard WinapiWindowContext::draw()
{
	throw std::logic_error("ny::WinapiWC: called draw() on draw-less windowContext");

/*
	PAINTSTRUCT ps;
	BeginPaint(handle_, &ps);
	EndPaint(handle_, &ps);
*/
}

void WinapiWindowContext::show()
{
	ShowWindow(handle_, SW_SHOWDEFAULT);
}
void WinapiWindowContext::hide()
{
	ShowWindow(handle_, SW_HIDE);
}

void WinapiWindowContext::addWindowHints(WindowHints hints)
{
}
void WinapiWindowContext::removeWindowHints(WindowHints hints)
{
}

bool WinapiWindowContext::handleEvent(const Event& e)
{
}

void WinapiWindowContext::size(const Vec2ui& size)
{
	SetWindowPos(handle_, HWND_TOP, 0, 0, size.x, size.y, SWP_NOMOVE);

}
void WinapiWindowContext::position(const Vec2i& position)
{
	SetWindowPos(handle_, HWND_TOP, position.x, position.y, 0, 0, SWP_NOSIZE);
}

void WinapiWindowContext::cursor(const Cursor& c)
{
}

void WinapiWindowContext::fullscreen()
{
}

void WinapiWindowContext::maximize()
{
	ShowWindow(handle_, SW_MAXIMIZE);
}

void WinapiWindowContext::minimize()
{
	ShowWindow(handle_, SW_MINIMIZE);
}

void WinapiWindowContext::normalState()
{
	ShowWindow(handle_, SW_RESTORE);
}

NativeWindowHandle WinapiWindowContext::nativeHandle() const
{
	return handle_;
}

//specific
Rect2i WinapiWindowContext::extents() const
{
	RECT ext;
	GetWindowRect(handle_, &ext);
	return {ext.left, ext.top, ext.right - ext.left, ext.bottom - ext.top};
}

}
