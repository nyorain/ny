#include <ny/backend/winapi/windowContext.hpp>
#include <ny/backend/winapi/util.hpp>
#include <ny/backend/winapi/appContext.hpp>
#include <ny/backend/winapi/com.hpp>

#include <ny/base/log.hpp>
#include <ny/base/cursor.hpp>

#include <ny/base/image.hpp>
#include <ny/draw/drawContext.hpp>

#include <tchar.h>
#include <stdexcept>

namespace ny
{

//static
const char* WinapiWindowContext::nativeWidgetClassName(NativeWidgetType type)
{
	switch(type)
	{
		case NativeWidgetType::button: return "Button";
		case NativeWidgetType::textfield: return "Edit";
		case NativeWidgetType::checkbox: return "Combobox";
		default: return nullptr;
	}
}

//windowContext
WinapiWindowContext::WinapiWindowContext(WinapiAppContext& appContext,
	const WinapiWindowSettings& settings)
{
    //init check
	appContext_ = &appContext;
    if(!hinstance()) throw std::runtime_error("winapiWC::create: uninitialized appContext");

	initWindowClass(settings);
	setStyle(settings);
	initWindow(settings);
}

WinapiWindowContext::~WinapiWindowContext()
{
    if(handle_)
	{
		::DestroyWindow(handle_);
		appContext().unregisterContext(handle_);

		handle_ = nullptr;
	}

	if(dropTarget_)
	{
		dropTarget_->Release();
		dropTarget_ = nullptr;
	}

	///XXX: correct cursor deletion?
}

void WinapiWindowContext::initWindowClass(const WinapiWindowSettings& settings)
{
	if(settings.nativeWidgetType != NativeWidgetType::none)
	{
		if(settings.nativeWidgetType == NativeWidgetType::dialog) return;

		auto name = nativeWidgetClassName(settings.nativeWidgetType);
		if(!name) throw std::logic_error("WinapiWC: invalid native widget type");
		wndClassName_ = name;

		return;
	}

	auto wndClass = windowClass(settings);
	if(!::RegisterClassEx(&wndClass))
	{
		throw std::runtime_error("winapiWC::create: could not register window class");
		return;
	}
}

WNDCLASSEX WinapiWindowContext::windowClass(const WinapiWindowSettings& settings)
{
	///XXX: threadsafety?
	static unsigned int highestID = 0;
	highestID++;

	wndClassName_ = "ny::WinapiWindowClass" + std::to_string(highestID);

	WNDCLASSEX ret;
	ret.hInstance = appContext().hinstance();
	ret.lpszClassName = _T(wndClassName_.c_str());
	ret.lpfnWndProc = &WinapiAppContext::wndProcCallback;
	ret.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
	ret.cbSize = sizeof(WNDCLASSEX);
	ret.hIcon = ::LoadIcon (nullptr, IDI_APPLICATION);
	ret.hIconSm = ::LoadIcon (nullptr, IDI_APPLICATION);
	ret.hCursor = ::LoadCursor (nullptr, IDC_ARROW);
	ret.lpszMenuName = nullptr;
	ret.cbClsExtra = 0;
	ret.cbWndExtra = 0;
	//ret.hbrBackground = (HBRUSH) ::GetStockObject(WHITE_BRUSH);
	ret.hbrBackground = nullptr;

	return ret;
}

void WinapiWindowContext::setStyle(const WinapiWindowSettings& settings)
{
	style_ = WS_OVERLAPPEDWINDOW;
}

void WinapiWindowContext::initWindow(const WinapiWindowSettings& settings)
{
	auto parent = static_cast<HWND>(settings.parent.pointer());
	auto hinstance = appContext().hinstance();

	auto size = settings.size;
	auto position = settings.position;

	if(position.x == -1) position.x = CW_USEDEFAULT;
	if(position.y == -1) position.y = CW_USEDEFAULT;

	if(size.x == -1) size.x = CW_USEDEFAULT;
	if(size.y == -1) size.y = CW_USEDEFAULT;

	if(settings.nativeWidgetType == NativeWidgetType::dialog)
	{
		initDialog(settings);
	}
	else
	{
		handle_ = ::CreateWindowEx(0, _T(wndClassName_.c_str()), _T(settings.title.c_str()), style_,
			position.x, position.y, size.x, size.y, parent, nullptr, hinstance, this);
	}

	appContext().registerContext(handle_, *this);
}

void WinapiWindowContext::initDialog(const WinapiWindowSettings& settings)
{
	auto parent = static_cast<HWND>(settings.parent.pointer());
	auto dialogProc = &WinapiAppContext::dlgProcCallback;

	DLGTEMPLATE dtemp {};
	handle_ = ::CreateDialogIndirect(hinstance(), &dtemp, parent, dialogProc);
}

void WinapiWindowContext::showWindow(const WinapiWindowSettings& settings)
{
	if(!settings.initShown) return;

	if(settings.initState == ToplevelState::maximized)
	{
		::ShowWindowAsync(handle_, SW_SHOWMAXIMIZED);
	}
	else if(settings.initState == ToplevelState::minimized)
	{
		::ShowWindowAsync(handle_, SW_SHOWMINIMIZED);
	}
	else
	{
		::ShowWindowAsync(handle_, SW_SHOWDEFAULT);
	}

	::UpdateWindow(handle_);
}

HINSTANCE WinapiWindowContext::hinstance() const
{
	return appContext().hinstance();
}

void WinapiWindowContext::refresh()
{
    ::RedrawWindow(handle_, nullptr, nullptr, RDW_INVALIDATE | RDW_NOERASE);
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
	if(hints & WindowHints::customDecorated)
	{
		auto style = ::GetWindowLong(handle(), GWL_STYLE);
		style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
		::SetWindowLong(handle(), GWL_STYLE, style);

		auto exStyle = ::GetWindowLong(handle(), GWL_EXSTYLE);
		exStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
		::SetWindowLong(handle(), GWL_EXSTYLE, exStyle);
	}
	if(hints & WindowHints::acceptDrop)
	{
		if(!dropTarget_)
		{
			dropTarget_ = new winapi::com::DropTargetImpl(*this);
			dropTarget_->AddRef();
			::RegisterDragDrop(handle(), dropTarget_);
		}
	}
	if(hints & WindowHints::alwaysOnTop)
	{
		::SetWindowPos(handle(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	}
}
void WinapiWindowContext::removeWindowHints(WindowHints hints)
{
	if(hints & WindowHints::customDecorated)
	{
		auto style = ::GetWindowLong(handle(), GWL_STYLE);
		style |= (WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
		::SetWindowLong(handle(), GWL_STYLE, style);

		auto exStyle = ::GetWindowLong(handle(), GWL_EXSTYLE);
		exStyle |= (WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
		::SetWindowLong(handle(), GWL_EXSTYLE, exStyle);
	}
	if(hints & WindowHints::acceptDrop)
	{
		::RevokeDragDrop(handle());
	}
	if(hints & WindowHints::alwaysOnTop)
	{
		::SetWindowPos(handle(), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	}
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
	//TODO: here and icon: system metrics
	if(c.type() == Cursor::Type::image)
	{
		auto cpy = *c.image();
		cpy.format(Image::Format::bgra8888);

		auto hs = c.imageHotspot();
		auto size = c.image()->size();

		auto bitmap = ::CreateBitmap(size.x, size.y, 1, 32, cpy.data());

		ICONINFO iconinfo;
		iconinfo.fIcon = false;
		iconinfo.xHotspot = hs.x;
		iconinfo.yHotspot = hs.y;
		iconinfo.hbmMask = nullptr;
		iconinfo.hbmColor = bitmap;

		auto icon = ::CreateIconIndirect(&iconinfo);
		cursor_ = reinterpret_cast<HCURSOR>(icon);
		::SetCursor(cursor_);

		DeleteObject(bitmap);
	}
	else if(c.type() == Cursor::Type::none)
	{
		cursor_ = nullptr;
		::SetCursor(cursor_);
	}
	else
	{
		auto cursorName = cursorToWinapi(c.type());
		if(!cursorName)
		{
			warning("WinapiWC::cursor: invalid native cursor type");
			return;
		}

		cursor_ = ::LoadCursor(hinstance(), cursorName);
		if(!cursor_)
		{
			warning("WinapiWC::cursor: failed to load native cursor ", cursorName);
			return;
		}

		::SetCursor(cursor_);
	}
}

void WinapiWindowContext::fullscreen()
{
	if(fullscreen_) return;

	//TODO: maybe add possibilty for display modes?
	//games *might* want to set a different resolution (low prio)
	//discussion needed

	//store current state
	savedState_.style = ::GetWindowLong(handle(), GWL_STYLE);
	savedState_.exstyle = ::GetWindowLong(handle(), GWL_EXSTYLE);
	savedState_.extents = extents();
	savedState_.maximized = ::IsZoomed(handle());

 	MONITORINFO monitorinfo;
  	monitorinfo.cbSize = sizeof(monitorinfo);
    ::GetMonitorInfo(::MonitorFromWindow(handle(), MONITOR_DEFAULTTONEAREST), &monitorinfo);
	auto& rect = monitorinfo.rcMonitor;
	rect.right -= rect.left;
	rect.bottom -= rect.top;

	::SetWindowLong(handle(), GWL_STYLE, (savedState_.style | WS_POPUP) & ~(WS_OVERLAPPEDWINDOW));
	::SetWindowLong(handle(), GWL_EXSTYLE, savedState_.exstyle & ~(WS_EX_DLGMODALFRAME |
		WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));

	//the rect.bottom + 1 is needed here since some (buggy?) winapi implementations
	//go automatically in real fullscreen mode when the window is a popup and the size
	//the same as the monitor (observed behaviour).
	//ny does not handle/support real fullscreen mode (consideren bad) since then
	//the window has to take care about correct alt-tab/minimize handling which becomes
	//easily buggy
	::SetWindowPos(handle(), HWND_TOP, rect.left, rect.top, rect.right, rect.bottom + 1,
		SWP_NOOWNERZORDER |	SWP_ASYNCWINDOWPOS | SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE);

	fullscreen_ = true;
}

void WinapiWindowContext::unsetFullscreen()
{
	if(fullscreen_)
	{
		auto& rect = savedState_.extents;
		SetWindowLong(handle(), GWL_STYLE, savedState_.style);
		SetWindowLong(handle(), GWL_EXSTYLE, savedState_.exstyle);
		fullscreen_ = false;
	}
}

void WinapiWindowContext::maximize()
{
	unsetFullscreen();
	::ShowWindowAsync(handle_, SW_MAXIMIZE);
}

void WinapiWindowContext::minimize()
{
	::ShowWindowAsync(handle_, SW_MINIMIZE);
}

void WinapiWindowContext::normalState()
{
	if(fullscreen_)
	{
		auto& rect = savedState_.extents;
		SetWindowLong(handle(), GWL_STYLE, savedState_.style);
		SetWindowLong(handle(), GWL_EXSTYLE, savedState_.exstyle);
		SetWindowPos(handle(), nullptr, rect.left(), rect.top(), rect.width(), rect.height(),
			SWP_ASYNCWINDOWPOS | SWP_FRAMECHANGED);

		fullscreen_ = false;
	}

	ShowWindowAsync(handle_, SW_RESTORE);
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
