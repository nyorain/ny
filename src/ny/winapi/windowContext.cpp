// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/winapi/windowContext.hpp>
#include <ny/winapi/util.hpp>
#include <ny/winapi/appContext.hpp>
#include <ny/winapi/com.hpp>

#include <ny/log.hpp>
#include <ny/cursor.hpp>
#include <ny/image.hpp>
#include <ny/surface.hpp>

#include <nytl/scope.hpp>
#include <nytl/utf.hpp>

#include <Dwmapi.h>
#include <tchar.h>
#include <stdexcept>

namespace ny {

// windowContext
WinapiWindowContext::WinapiWindowContext(WinapiAppContext& appContext,
	const WinapiWindowSettings& settings)
{
	appContext_ = &appContext;
	initWindowClass(settings);
	setStyle(settings);
	initWindow(settings);
	showWindow(settings);
}

WinapiWindowContext::~WinapiWindowContext()
{
	if(dropTarget_) {
		dropTarget_->Release();
		dropTarget_ = nullptr;
	}

	if(ownedCursor_ && cursor_) {
		::SetCursor(nullptr);
		::DestroyCursor(cursor_);
		cursor_ = nullptr;
	}

	if(icon_) {
		PostMessage(handle(), WM_SETICON, ICON_BIG, (LPARAM) nullptr);
		PostMessage(handle(), WM_SETICON, ICON_SMALL, (LPARAM) nullptr);
		::DestroyIcon(icon_);
		icon_ = nullptr;
	}

	if(handle_) {
		::SetWindowLongPtr(handle_, GWLP_USERDATA, (LONG_PTR)nullptr);
		::DestroyWindow(handle_);
		handle_ = nullptr;
	}
}

void WinapiWindowContext::initWindowClass(const WinapiWindowSettings& settings)
{
	auto wndClass = windowClass(settings);
	if(!::RegisterClassEx(&wndClass))
		throw winapi::lastErrorException("ny::WinapiWindowContext: RegisterClassEx failed");
}

WNDCLASSEX WinapiWindowContext::windowClass(const WinapiWindowSettings&)
{
	// TODO: does every window needs it own class (sa setCursor function)?
	static unsigned int highestID = 0;
	highestID++;

	wndClassName_ = widen("ny::WinapiWindowClass" + std::to_string(highestID));

	WNDCLASSEX ret;
	ret.hInstance = appContext().hinstance();
	ret.lpszClassName = wndClassName_.c_str();
	ret.lpfnWndProc = &WinapiAppContext::wndProcCallback;
	ret.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	ret.cbSize = sizeof(WNDCLASSEX);
	ret.hIcon = ::LoadIcon (nullptr, IDI_APPLICATION);
	ret.hIconSm = ::LoadIcon (nullptr, IDI_APPLICATION);
	ret.hCursor = ::LoadCursor (nullptr, IDC_ARROW);
	ret.lpszMenuName = nullptr;
	ret.cbClsExtra = 0;
	ret.cbWndExtra = 0;
	ret.hbrBackground = nullptr;

	return ret;
}

void WinapiWindowContext::setStyle(const WinapiWindowSettings&)
{
	style_ = WS_OVERLAPPEDWINDOW | WS_OVERLAPPED | WS_VISIBLE | WS_CLIPSIBLINGS;
}

void WinapiWindowContext::initWindow(const WinapiWindowSettings& settings)
{
	auto parent = static_cast<HWND>(settings.parent.pointer());
	auto hinstance = appContext().hinstance();
	auto titleW = widen(settings.title);

	// parse position and size
	auto size = settings.size;
	auto position = settings.position;

	if(position == defaultPosition) position[0] = position[1] = CW_USEDEFAULT;
	if(size == defaultSize) size[0] = size[1] = CW_USEDEFAULT;

	// set the listener
	if(settings.listener) listener(*settings.listener);

	// NOTE on transparency and layered windows
	// The window has to be layered to enable transparent drawing on it
	// On newer windows versions it is not enough to call DwmEnableBlueBehinWindow, only
	// layered windows are considered really transparent and contents beneath it
	// are rerendered.
	//
	// Note that we even set this flag here for e.g. opengl windows which have CS_OWNDC set
	// which is not allowed per msdn. But windows applications do it themselves, it
	// is the only way to get the possibility for transparent windows and it works.
	//
	// Setting this flag can also really hit performance (e.g. resizing can lag) so
	// this should probably only be set if really needed

	auto exstyle = WS_EX_APPWINDOW |  WS_EX_OVERLAPPEDWINDOW;
	if(settings.transparent) exstyle |= WS_EX_LAYERED;

	handle_ = ::CreateWindowEx(
		exstyle,
		wndClassName_.c_str(),
		titleW.c_str(),
		style_,
		position[0], position[1],
		size[0], size[1],
		parent, nullptr, hinstance, this);

	if(!handle_)
		throw winapi::lastErrorException("ny::WinapiWindowContext: CreateWindowEx failed");

	// Set the userdata
	std::uintptr_t ptr = reinterpret_cast<std::uintptr_t>(this);
	::SetWindowLongPtr(handle_, GWLP_USERDATA, ptr);

	if(settings.transparent) {
		// TODO: check for windows version > xp here.
		// Otherwise ny will no compile/run on xp

		// This will simply cause windows to respect the alpha bits in the content of the window
		// and not actually blur anything. Windows is stupid af.
		DWM_BLURBEHIND bb {};
		bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
		bb.hRgnBlur = CreateRectRgn(0, 0, -1, -1);  // makes the window transparent
		bb.fEnable = TRUE;
		::DwmEnableBlurBehindWindow(handle(), &bb);

		// This is not what makes the window transparent.
		// We simply have to do this so the window contents are shown.
		// We only have to set the layered flag to make DwmEnableBlueBehinWindow function
		// correctly and this causes the flag to have no further effect.
		::SetLayeredWindowAttributes(handle(), 0x1, 0, LWA_COLORKEY);
	}

	// register a drop target
	if(settings.droppable) {
		dropTarget_ = new winapi::com::DropTargetImpl(*this);
		dropTarget_->AddRef();
		::RegisterDragDrop(handle(), dropTarget_);
	}
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
	if(!settings.show) return;

	if(settings.initState == ToplevelState::maximized) {
		::ShowWindowAsync(handle_, SW_SHOWMAXIMIZED);
	} else if(settings.initState == ToplevelState::minimized) {
		::ShowWindowAsync(handle_, SW_SHOWMINIMIZED);
	} else {
		::ShowWindowAsync(handle_, SW_SHOWDEFAULT);
	}

	refresh();
}

HINSTANCE WinapiWindowContext::hinstance() const
{
	return appContext().hinstance();
}

void WinapiWindowContext::refresh()
{
	::RedrawWindow(handle_, nullptr, nullptr, RDW_INVALIDATE | RDW_NOERASE | RDW_FRAME);
}

void WinapiWindowContext::show()
{
	::ShowWindowAsync(handle_, SW_SHOW);
}
void WinapiWindowContext::hide()
{
	::ShowWindowAsync(handle_, SW_HIDE);
}

void WinapiWindowContext::customDecorated(bool set)
{
	// /*

	auto style = ::GetWindowLong(handle(), GWL_STYLE);
	auto exStyle = ::GetWindowLong(handle(), GWL_EXSTYLE);

	if(set) {
		style &= ~WS_OVERLAPPEDWINDOW;
		style |= WS_POPUP;
		::SetWindowLong(handle(), GWL_STYLE, style);

		exStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
		::SetWindowLong(handle(), GWL_EXSTYLE, exStyle);
	} else {
		style |= WS_OVERLAPPEDWINDOW;
		style &= ~WS_POPUP;
		::SetWindowLong(handle(), GWL_STYLE, style);

		exStyle |= (WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
		::SetWindowLong(handle(), GWL_EXSTYLE, exStyle);
	}

	customDecorated_ = set;
	::SetWindowPos(handle(), HWND_TOP, 0, 0, 0, 0,
		SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_ASYNCWINDOWPOS);
}

void WinapiWindowContext::size(nytl::Vec2ui size)
{
	::SetWindowPos(handle_, HWND_TOP, 0, 0,
		size[0], size[1], SWP_NOMOVE | SWP_ASYNCWINDOWPOS | SWP_NOZORDER);
}
void WinapiWindowContext::position(nytl::Vec2i position)
{
	::SetWindowPos(handle_, HWND_TOP, position[0], position[1],
		0, 0, SWP_NOSIZE | SWP_ASYNCWINDOWPOS | SWP_NOZORDER);
}

void WinapiWindowContext::cursor(const Cursor& cursor)
{
	dlg_source("::winwc::cursor"_src);
	using winapi::errorMessage;

	if(cursor.type() == CursorType::image) {
		auto img = cursor.image();
		auto bitmap = winapi::toBitmap(img);
		auto mask = ::CreateBitmap(img.size[0], img.size[1], 1, 1, nullptr);

		auto bitmapsGuard = nytl::ScopeGuard([&]{
			if(bitmap) ::DeleteObject(bitmap);
			if(mask) ::DeleteObject(mask);
		});

		if(!bitmap || !mask) {
			ny_warn(errorMessage("CreateBitmap"));
			return;
		}

		const auto& hs = cursor.imageHotspot();
		ICONINFO iconinfo;
		iconinfo.fIcon = false;
		iconinfo.xHotspot = hs[0];
		iconinfo.yHotspot = hs[1];
		iconinfo.hbmMask = mask;
		iconinfo.hbmColor = bitmap;

		cursor_ = reinterpret_cast<HCURSOR>(::CreateIconIndirect(&iconinfo));
		if(!cursor_) {
			ny_warn(errorMessage("CreateIconIndirect"));
			return;
		}

		ownedCursor_ = true;
	} else if(cursor.type() == CursorType::none) {
		cursor_ = nullptr;
		ownedCursor_ = false;
	} else {
		auto cursorName = cursorToWinapi(cursor.type());
		if(!cursorName) {
			ny_warn("invalid native cursor type");
			return;
		}

		ownedCursor_ = false;
		cursor_ = ::LoadCursor(nullptr, cursorName);

		if(!cursor_) {
			ny_warn(errorMessage("LoadCursor"));
			return;
		}
	}

	// Some better method for doing this?
	// maybe just respond to the WM_SETCURSOR?
	// http://stackoverflow.com/questions/169155/setcursor-reverts-after-a-mouse-move
	::SetCursor(cursor_);

	// -12 == GCL_HCURSOR
	// number used since it is not defined in windows.h sometimes (tested with MinGW headers)
	::SetClassLongPtr(handle(), -12, reinterpret_cast<LONG_PTR>(cursor_));
	// ::SetClassLongPtr(handle(), -12, (LONG_PTR) nullptr);
}

void WinapiWindowContext::fullscreen()
{
	if(fullscreen_) return;

	// TODO: maybe add possibilty for display modes?
	// games *might* want to set a different resolution (low prio)
	// discussion needed, ny could also generally discourage/not implement it since it sucks
	// might be useful for dealing with 4k (some games might only wanna support 1080p, sucks too)

	// store current state since winapi does not do it for fullscreen
	savedState_.style = ::GetWindowLong(handle(), GWL_STYLE);
	savedState_.exstyle = ::GetWindowLong(handle(), GWL_EXSTYLE);
	savedState_.extents = extents();
	if(::IsZoomed(handle())) savedState_.state = 1;

	MONITORINFO monitorinfo;
	monitorinfo.cbSize = sizeof(monitorinfo);
	::GetMonitorInfo(::MonitorFromWindow(handle(), MONITOR_DEFAULTTONEAREST), &monitorinfo);
	auto& rect = monitorinfo.rcMonitor;
	rect.right -= rect.left;
	rect.bottom -= rect.top;

	::SetWindowLong(handle(), GWL_STYLE, (savedState_.style | WS_POPUP) & ~(WS_OVERLAPPEDWINDOW));
	::SetWindowLong(handle(), GWL_EXSTYLE, savedState_.exstyle & ~(WS_EX_DLGMODALFRAME |
		WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));

	// the rect.bottom + 1 is needed here since some (buggy?) winapi implementations
	// go automatically in real fullscreen mode when the window is a popup and the size
	// the same as the monitor (observed behaviour).
	// ny does not handle/support real fullscreen mode (consideren bad) since then
	// the window has to take care about correct alt-tab/minimize handling which becomes
	// easily buggy
	::SetWindowPos(handle(), HWND_TOP, rect.left, rect.top, rect.right, rect.bottom + 1,
		SWP_NOOWNERZORDER |	SWP_ASYNCWINDOWPOS | SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE);

	fullscreen_ = true;
}

void WinapiWindowContext::unsetFullscreen()
{
	if(fullscreen_) {
		auto& rect = savedState_.extents;
		SetWindowLong(handle(), GWL_STYLE, savedState_.style);
		SetWindowLong(handle(), GWL_EXSTYLE, savedState_.exstyle);
		SetWindowPos(handle(), nullptr, rect.position[0], rect.position[1],
			rect.size[0], rect.size[1], SWP_ASYNCWINDOWPOS | SWP_FRAMECHANGED);
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
	// We do intentionally not unset fullscreen here, since when a fullscreen window
	// is minimized it does usually keep its fullscreen state when it will be un-minimized again
	::ShowWindowAsync(handle_, SW_MINIMIZE);
}

void WinapiWindowContext::normalState()
{
	unsetFullscreen();
	::ShowWindowAsync(handle_, SW_RESTORE);
}

void WinapiWindowContext::minSize(nytl::Vec2ui size)
{
	minSize_ = size;
}

void WinapiWindowContext::maxSize(nytl::Vec2ui size)
{
	maxSize_ = size;
}

void WinapiWindowContext::beginMove(const EventData*)
{
	constexpr auto SC_DRAGMOVE = 0xf012;
	::PostMessage(handle_, WM_SYSCOMMAND, SC_DRAGMOVE, 0);
}

void WinapiWindowContext::beginResize(const EventData*, WindowEdges edges)
{
	// note that WinapiAppContext does explicitly handle this message and sets
	// the cursor.
	auto winapiEdges = edgesToWinapi(edges);
	::PostMessage(handle_, WM_SYSCOMMAND, SC_SIZE + winapiEdges, 0);
}

void WinapiWindowContext::icon(const Image& img)
{
	// if nullptr passed set no icon
	if(!img.data) {
		::PostMessage(handle(), WM_SETICON, ICON_BIG, (LPARAM) nullptr);
		::PostMessage(handle(), WM_SETICON, ICON_SMALL, (LPARAM) nullptr);
		return;
	}

	constexpr static auto reqFormat = ImageFormat::argb8888;
	auto uniqueImage = convertFormat(img, reqFormat);

	premultiply(uniqueImage);

	auto pixelsData = data(uniqueImage);
	icon_ = ::CreateIcon(hinstance(), img.size[0], img.size[1], 1, 32, nullptr, pixelsData);
	if(!icon_) {
		ny_warn("::winwc::icon"_src, winapi::errorMessage("CreateIcon"));
		return;
	}

	::PostMessage(handle(), WM_SETICON, ICON_BIG, (LPARAM) icon_);
	::PostMessage(handle(), WM_SETICON, ICON_SMALL, (LPARAM) icon_);
}

void WinapiWindowContext::title(std::string_view title)
{
	SetWindowText(handle(), widen(title.data()).c_str());
}

NativeHandle WinapiWindowContext::nativeHandle() const
{
	return handle_;
}

WindowCapabilities WinapiWindowContext::capabilities() const
{
	return WindowCapability::size |
		WindowCapability::fullscreen |
		WindowCapability::minimize |
		WindowCapability::maximize |
		WindowCapability::fullscreen |
		WindowCapability::position |
		WindowCapability::sizeLimits |
		WindowCapability::icon |
		WindowCapability::cursor |
		WindowCapability::title |
		WindowCapability::beginMove |
		WindowCapability::beginResize |
		WindowCapability::visibility |
		WindowCapability::customDecoration |
		WindowCapability::serverDecoration;
}

// win32 specific
Surface WinapiWindowContext::surface()
{
	return {};
}

nytl::Rect2i WinapiWindowContext::extents() const
{
	::RECT ext;
	GetWindowRect(handle_, &ext);
	return {{ext.left, ext.top}, {ext.right - ext.left, ext.bottom - ext.top}};
}

nytl::Rect2i WinapiWindowContext::clientExtents() const
{
	::RECT ext;
	GetClientRect(handle_, &ext);
	return {{ext.left, ext.top}, {ext.right - ext.left, ext.bottom - ext.top}};
}

} // namespace ny
