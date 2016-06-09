#include <ny/backend/winapi/windowContext.hpp>
#include <ny/backend/winapi/appContext.hpp>
#include <ny/base/log.hpp>

#include <ny/draw/image.hpp>
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

    if(!appContext_->hinstance())
	{
		throw std::runtime_error("winapiWC::create: uninitialized appContext");
	}

	setStyle(settings);
	initWindowClass(settings);

	if(!::RegisterClassEx(&wndClass_))
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
		::CloseWindow(handle_);
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
	wndClass_.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
	wndClass_.cbSize = sizeof(WNDCLASSEX);
	wndClass_.hIcon = ::LoadIcon (nullptr, IDI_APPLICATION);
	wndClass_.hIconSm = ::LoadIcon (nullptr, IDI_APPLICATION);
	wndClass_.hCursor = ::LoadCursor (nullptr, IDC_ARROW);
	wndClass_.lpszMenuName = nullptr;
	wndClass_.cbClsExtra = 0;
	wndClass_.cbWndExtra = 0;
	//wndClass_.hbrBackground = (HBRUSH) ::GetStockObject(WHITE_BRUSH);
	wndClass_.hbrBackground = nullptr;
}

void WinapiWindowContext::setStyle(const WinapiWindowSettings& settings)
{
	style_ = WS_OVERLAPPEDWINDOW;
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

	handle_ = ::CreateWindowEx(0, wndClass_.lpszClassName, _T(settings.title.c_str()), style_,
		position.x, position.y, size.x, size.y, parent, nullptr, hinstance, this);

	appContext().registerContext(handle_, *this);

	if(settings.initShown)
	{
		if(settings.initState == ToplevelState::maximized) ::ShowWindowAsync(handle_, SW_SHOWMAXIMIZED);
		else if(settings.initState == ToplevelState::minimized) ::ShowWindowAsync(handle_, SW_SHOWMINIMIZED);
		else ::ShowWindowAsync(handle_, SW_SHOWDEFAULT);

		::UpdateWindow(handle_);
	}
}

void WinapiWindowContext::refresh()
{
    ::RedrawWindow(handle_, nullptr, nullptr, RDW_INVALIDATE | RDW_NOERASE);
}

DrawGuard WinapiWindowContext::draw()
{
	throw std::logic_error("ny::WinapiWC: called draw() on draw-less windowContext");
}

void WinapiWindowContext::show()
{
	::ShowWindowAsync(handle_, SW_SHOWDEFAULT);
}
void WinapiWindowContext::hide()
{
	::ShowWindowAsync(handle_, SW_HIDE);
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
	::SetWindowPos(handle_, HWND_TOP, 0, 0, size.x, size.y, SWP_NOMOVE | SWP_ASYNCWINDOWPOS);
}
void WinapiWindowContext::position(const Vec2i& position)
{
	::SetWindowPos(handle_, HWND_TOP, position.x, position.y, 0, 0, SWP_NOSIZE | SWP_ASYNCWINDOWPOS);
}

void WinapiWindowContext::cursor(const Cursor& c)
{
}

void WinapiWindowContext::fullscreen()
{
	//TODO: maybe add possibilty for display modes?
	//games *might* want to set a different resolution (low prio)

	//store current state
	savedState_.style = GetWindowLong(handle(), GWL_STYLE);
	savedState_.exstyle = GetWindowLong(handle(), GWL_EXSTYLE);
	savedState_.extensts = extents();

 	MONITORINFO monitorinfo;
  	monitorinfo.cbSize = sizeof(monitorinfo);
    GetMonitorInfo(MonitorFromWindow(handle(), MONITOR_DEFAULTTONEAREST), &monitorinfo);
	auto& rect = monitorinfo.rcMonitor;

	::SetWindowLong(handle(), GWL_STYLE, WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
	::SetWindowLong(handle(), GWL_EXSTYLE, WS_EX_APPWINDOW  | WS_EX_TOPMOST);

	::SetWindowPos(handle(), HWND_TOP, rect.left, rect.top, rect.right, rect.bottom,
		SWP_ASYNCWINDOWPOS | SWP_FRAMECHANGED);
	::ShowWindowAsync(handle(), SW_SHOW);

	fullscreen_ = true;
}

void WinapiWindowContext::maximize()
{
	ShowWindowAsync(handle_, SW_MAXIMIZE);
}

void WinapiWindowContext::minimize()
{
	ShowWindowAsync(handle_, SW_MINIMIZE);
}

void WinapiWindowContext::normalState()
{
	if(fullscreen_)
	{
		auto& rect = savedState_.extents;
		SetWindowLong(handle(), GWL_STYLE, savedState_.style);
		SetWindowLong(handle(), GWL_EXSTYLE, savedState_.exstyle);
		SetWindowPos(handle(), nullptr, rect.left(), rect.top(), rect.width(), rect.height(),
			SWP_FRAMECHANGED);

		fullscreen_ = false;
	}

	ShowWindow(handle_, SW_RESTORE);
}

void WinapiWindowContext::icon(const Image* img)
{
	//if nullptr passed set no icon
	if(!img)
	{
		PostMessage(handle(), WM_SETICON, ICON_BIG, (LPARAM) nullptr);
        PostMessage(handle(), WM_SETICON, ICON_SMALL, (LPARAM) nullptr);
		return;
	}

	//windows wants the data in bgra format
	auto cpy = *img;
	cpy.format(Image::Format::bgra8888);

	auto module = GetModuleHandle(nullptr);
	auto icon = CreateIcon(module, cpy.size().x, cpy.size().y, 1, 32, nullptr, cpy.data());

   if(!icon) warning("WinapiWindowContext::icon: Failed to create winapi icon handle");

   PostMessageW(handle(), WM_SETICON, ICON_BIG, (LPARAM) icon);
   PostMessageW(handle(), WM_SETICON, ICON_SMALL, (LPARAM) icon);
}

void WinapiWindowContext::title(const std::string& title)
{
	SetWindowText(handle(), _T(title.c_str()));
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
