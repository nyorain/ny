// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/winapi/windowContext.hpp>
#include <ny/winapi/util.hpp>
#include <ny/winapi/appContext.hpp>
#include <ny/winapi/com.hpp>

#include <ny/log.hpp>
#include <ny/cursor.hpp>
#include <ny/imageData.hpp>
#include <ny/events.hpp>
#include <ny/surface.hpp>

#include <nytl/scope.hpp>

#include <Dwmapi.h>
#include <tchar.h>
#include <stdexcept>

namespace ny
{

//static
const char* WinapiWindowContext::nativeWidgetClassName(NativeWidgetType type)
{
	// switch(type)
	// {
	// 	case NativeWidgetType::button: return "Button";
	// 	case NativeWidgetType::textfield: return "Edit";
	// 	case NativeWidgetType::checkbox: return "Combobox";
	// 	default: return nullptr;
	// }

	return nullptr;
}

//windowContext
WinapiWindowContext::WinapiWindowContext(WinapiAppContext& appContext,
	const WinapiWindowSettings& settings)
{
	//init check
	appContext_ = &appContext;
	if(!hinstance()) throw std::runtime_error("ny::WinapiWC::create: uninitialized appContext");

	initWindowClass(settings);
	setStyle(settings);
	initWindow(settings);
	showWindow(settings);
}

WinapiWindowContext::~WinapiWindowContext()
{
	if(dropTarget_)
	{
		dropTarget_->Release();
		dropTarget_ = nullptr;
	}

	if(ownedCursor_ && cursor_)
	{
		::SetCursor(nullptr);
		::DestroyCursor(cursor_);
		cursor_ = nullptr;
	}

	if(icon_)
	{
		PostMessage(handle(), WM_SETICON, ICON_BIG, (LPARAM) nullptr);
		PostMessage(handle(), WM_SETICON, ICON_SMALL, (LPARAM) nullptr);
		::DestroyIcon(icon_);
		icon_ = nullptr;
	}

	if(handle_)
	{
		::SetWindowLongPtr(handle_, GWLP_USERDATA, (LONG_PTR)nullptr);
		::DestroyWindow(handle_);
		handle_ = nullptr;
	}
}

void WinapiWindowContext::initWindowClass(const WinapiWindowSettings& settings)
{
	// if(settings.nativeWidgetType != NativeWidgetType::none)
	// {
	// 	if(settings.nativeWidgetType == NativeWidgetType::dialog) return;
	//
	// 	auto name = nativeWidgetClassName(settings.nativeWidgetType);
	// 	if(!name) throw std::logic_error("ny::WinapiWC: invalid native widget type");
	// 	wndClassName_ = name;
	//
	// 	return;
	// }

	auto wndClass = windowClass(settings);
	if(!::RegisterClassEx(&wndClass))
		throw winapi::EC::exception("ny::WinapiWC: RegisterClassEx failed");
}

WNDCLASSEX WinapiWindowContext::windowClass(const WinapiWindowSettings& settings)
{
	//TODO: does every window needs it own class (sa setCursor function)?
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
	ret.hbrBackground = nullptr;

	return ret;
}

void WinapiWindowContext::setStyle(const WinapiWindowSettings& settings)
{
	style_ = WS_OVERLAPPEDWINDOW | WS_OVERLAPPED | WS_VISIBLE | WS_CLIPSIBLINGS;
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

	// if(settings.nativeWidgetType == NativeWidgetType::dialog)
	// {
	// 	initDialog(settings);
	// }
	// else
	// {
		//NOTE
		//The window has to be layered to enable transparent drawing on it
		//On newer windows versions it is not enough to call DwmEnableBlueBehinWindow, only
		//layered windows are considered really transparent and contents beneath it
		//are rerendered.
		//
		//Note that we even set this flag here for e.g. opengl windows which have CS_OWNDC set
		//which is not allowed per msdn. But windows applications do it themselves, it
		//is the only way to get the possibility for transparent windows and it works.
		//
		//Settings this flag can also really hit performance (e.g. resizing can lag) so
		//this should probably only be set if really needed
		//TODO: make this optional using WinapiWindowSettings
		auto exstyle = WS_EX_APPWINDOW | WS_EX_LAYERED | WS_EX_OVERLAPPEDWINDOW;
		handle_ = ::CreateWindowEx(exstyle, _T(wndClassName_.c_str()), _T(settings.title.c_str()),
			style_, position.x, position.y, size.x, size.y, parent, nullptr, hinstance, this);

		if(!handle_) throw winapi::EC::exception("ny::WinapiWC: CreateWindowEx failed");
	// }

	{
		//TODO: check for windows version > xp here.
		//Otherwise ny will no compile/run on xp
		//This will simply cause windows to respect the alpha bits in the content of the window
		//and not actually blur anything. Windows is stupid af.
		DWM_BLURBEHIND bb = { 0 };
		bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
		bb.hRgnBlur = CreateRectRgn(0, 0, -1, -1);  // makes the window transparent
		bb.fEnable = TRUE;
		::DwmEnableBlurBehindWindow(handle(), &bb);

		//This is not what makes the window transparent.
		//We simply have to do this so the window contents are shown.
		//We only have to set the layered flag to make DwmEnableBlueBehinWindow function
		//correctly and this causes the flag to have no further effect.
		::SetLayeredWindowAttributes(handle(), 0x1, 0, LWA_COLORKEY);
	}

	//Set the userdata
	std::uintptr_t ptr = reinterpret_cast<std::uintptr_t>(this);
	::SetWindowLongPtr(handle_, GWLP_USERDATA, ptr);
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

	::UpdateWindow(handle_);
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
	::ShowWindowAsync(handle_, SW_SHOWDEFAULT);
}
void WinapiWindowContext::hide()
{
	::ShowWindowAsync(handle_, SW_HIDE);
}

void WinapiWindowContext::droppable(const DataTypes& types)
{
	if(dropTarget_)
	{
		if(types.types.empty())
		{
			dropTarget_->Release();
			::RevokeDragDrop(handle());
			dropTarget_ = nullptr;
		}
		else
		{
			dropTarget_->dataTypes = types;
		}
	}
	else if(!types.types.empty())
	{
		dropTarget_ = new winapi::com::DropTargetImpl(*this, types);
		dropTarget_->AddRef();
		::RegisterDragDrop(handle(), dropTarget_);
	}
}

void WinapiWindowContext::addWindowHints(WindowHints hints)
{
	if(hints & WindowHint::customDecorated)
	{
		auto style = ::GetWindowLong(handle(), GWL_STYLE);
		style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
		::SetWindowLong(handle(), GWL_STYLE, style);

		auto exStyle = ::GetWindowLong(handle(), GWL_EXSTYLE);
		exStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
		::SetWindowLong(handle(), GWL_EXSTYLE, exStyle);
	}
}
void WinapiWindowContext::removeWindowHints(WindowHints hints)
{
	if(hints & WindowHint::customDecorated)
	{
		auto style = ::GetWindowLong(handle(), GWL_STYLE);
		style |= (WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
		::SetWindowLong(handle(), GWL_STYLE, style);

		auto exStyle = ::GetWindowLong(handle(), GWL_EXSTYLE);
		exStyle |= (WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
		::SetWindowLong(handle(), GWL_EXSTYLE, exStyle);
	}
}

void WinapiWindowContext::sizeEvent(nytl::Vec2ui size)
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
	if(c.type() == CursorType::image)
	{
		constexpr static auto reqFormat = ImageDataFormat::bgra8888; //TODO: endianess?

		const auto& imgdata = *c.image();
		std::unique_ptr<std::uint8_t[]> ownedData;
		auto packedStride = imgdata.size.x * imageDataFormatSize(imgdata.format);
		auto* pixelsData = imgdata.data;

		//usually an extra conversion/copy is required
		if((imgdata.stride != packedStride && imgdata.stride != 0) || imgdata.format != reqFormat)
		{
			ownedData = convertFormat(imgdata, reqFormat);
			pixelsData = ownedData.get();
		}

		auto bitmap = ::CreateBitmap(imgdata.size.x, imgdata.size.y, 1, 32, pixelsData);
		auto dummyBitmap = ::CreateBitmap(imgdata.size.x, imgdata.size.y, 1, 1, NULL);

		auto bitmapGuard = nytl::makeScopeGuard([&]{
			if(bitmap) ::DeleteObject(bitmap);
			if(dummyBitmap) ::DeleteObject(dummyBitmap);
		});

		if(!bitmap || !dummyBitmap)
		{
			warning(errorMessage("ny::WinapiWC::cursor: failed to create bitmap"));
			return;
		}

		const auto& hs = c.imageHotspot();
		ICONINFO iconinfo;
		iconinfo.fIcon = false;
		iconinfo.xHotspot = hs.x;
		iconinfo.yHotspot = hs.y;
		iconinfo.hbmMask = dummyBitmap;
		iconinfo.hbmColor = bitmap;

		cursor_ = reinterpret_cast<HCURSOR>(::CreateIconIndirect(&iconinfo));
		if(!cursor_)
		{
			warning(errorMessage("ny::WinapiWC::cursor: failed to create icon"));
			return;
		}

		ownedCursor_ = true;
	}
	else if(c.type() == CursorType::none)
	{
		cursor_ = nullptr;
		ownedCursor_ = false;
	}
	else
	{
		auto cursorName = cursorToWinapi(c.type());
		if(!cursorName)
		{
			warning("WinapiWC::cursor: invalid native cursor type");
			return;
		}

		ownedCursor_ = false;
		cursor_ = ::LoadCursor(nullptr, cursorName);

		if(!cursor_)
		{
			warning(errorMessage("WinapiWC::cursor: failed to load native cursor"));
			return;
		}
	}

	//Some better method for doing this?
	//maybe just respond to the WM_SETCURSOR?
	//http://stackoverflow.com/questions/169155/setcursor-reverts-after-a-mouse-move
	::SetCursor(cursor_);

	//-12 == GCL_HCURSOR
	//number used since it is not defined in windows.h sometimes (tested with MinGW headers)
	::SetClassLongPtr(handle(), -12, (LONG_PTR)cursor_);
}

void WinapiWindowContext::fullscreen()
{
	if(fullscreen_) return;

	//TODO: maybe add possibilty for display modes?
	//games *might* want to set a different resolution (low prio)
	//discussion needed, ny could also generally discourage/not implement it since it sucks
	//might be useful for dealing with 4k (some games might only wanna support 1080p, sucks too)

	//store current state since winapi does not do it for fullscreen
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
		SetWindowPos(handle(), nullptr, rect.left(), rect.top(), rect.width(), rect.height(),
			SWP_ASYNCWINDOWPOS | SWP_FRAMECHANGED);
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
	//We do intentionally not unset fullscreen here, since when a fullscreen window
	//is minimized it does usually keep its fullscreen state when it will be un-minimized again
	::ShowWindowAsync(handle_, SW_MINIMIZE);
}

void WinapiWindowContext::normalState()
{
	unsetFullscreen();
	::ShowWindowAsync(handle_, SW_RESTORE);
}

void WinapiWindowContext::minSize(const nytl::Vec2ui& size)
{
	minSize_ = size;
}

void WinapiWindowContext::maxSize(const nytl::Vec2ui& size)
{
	maxSize_ = size;
}

void WinapiWindowContext::beginMove(const MouseButtonEvent*)
{
	// auto currentCursor = ::GetClassLongPtr(handle_, -12);
	// this->cursor(CursorType::crosshair);
	// ::PostMessage(handle_, WM_SYSCOMMAND, SC_MOVE, 0);
	constexpr auto SC_DRAGMOVE = 0xf012; //or: 0xf009. Difference?
	::PostMessage(handle_, WM_SYSCOMMAND, SC_DRAGMOVE, 0);
	// ::SetClassLongPtr(handle_, -12, currentCursor);
}

void WinapiWindowContext::beginResize(const MouseButtonEvent*, WindowEdges edges)
{
	auto cursor = sizeCursorFromEdge(static_cast<WindowEdge>(edges.value()));
	auto currentCursor = ::GetClassLongPtr(handle_, -12);
	this->cursor(cursor);

	auto winapiEdges = edgesToWinapi(edges);
	::PostMessage(handle_, WM_SYSCOMMAND, SC_SIZE + winapiEdges, 0);

	// ::SetClassLongPtr(handle_, -12, currentCursor);
}

void WinapiWindowContext::icon(const ImageData& imgdata)
{
	//if nullptr passed set no icon
	if(!imgdata.data)
	{
		::PostMessage(handle(), WM_SETICON, ICON_BIG, (LPARAM) nullptr);
		::PostMessage(handle(), WM_SETICON, ICON_SMALL, (LPARAM) nullptr);
		return;
	}

	//windows wants the data in bgra format
	constexpr static auto reqFormat = ImageDataFormat::bgra8888; //TODO: endianess?

	std::unique_ptr<std::uint8_t[]> ownedData;
	auto packedStride = imgdata.size.x * imageDataFormatSize(imgdata.format);
	auto* pixelsData = imgdata.data;

	//usually an extra conversion/copy is required
	if((imgdata.stride != packedStride && imgdata.stride != 0) || imgdata.format != reqFormat)
	{
		ownedData = convertFormat(imgdata, reqFormat);
		pixelsData = ownedData.get();
	}

	icon_ = ::CreateIcon(hinstance(), imgdata.size.x, imgdata.size.y, 1, 32, nullptr, pixelsData);
	   if(!icon_)
	{
		warning("WinapiWindowContext::icon: Failed to create winapi icon handle");
		return;
	}

	PostMessage(handle(), WM_SETICON, ICON_BIG, (LPARAM) icon_);
	PostMessage(handle(), WM_SETICON, ICON_SMALL, (LPARAM) icon_);
}

void WinapiWindowContext::title(const std::string& title)
{
	SetWindowText(handle(), _T(title.c_str()));
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
		WindowCapability::position |
		WindowCapability::sizeLimits;
}

//specific
Surface WinapiWindowContext::surface()
{
	return {};
}

Rect2i WinapiWindowContext::extents() const
{
	RECT ext;
	GetWindowRect(handle_, &ext);
	return {ext.left, ext.top, ext.right - ext.left, ext.bottom - ext.top};
}

Rect2i WinapiWindowContext::clientExtents() const
{
	RECT ext;
	GetClientRect(handle_, &ext);
	return {ext.left, ext.top, ext.right - ext.left, ext.bottom - ext.top};
}

}

// COLORREF custcolors[16];
//
// ::CHOOSECOLOR cc {};
// cc.hwndOwner = static_cast<ny::WinapiWindowContext&>(*wc).handle();
// cc.lStructSize = sizeof(cc);
// cc.lpCustColors = custcolors;
// cc.Flags = CC_ANYCOLOR | CC_FULLOPEN;
// // cc.Flags = CC_ANYCOLOR | CC_FULLOPEN | CC_ENABLEHOOK;
// // cc.lpfnHook = reinterpret_cast<LPCCHOOKPROC>(&::DefWindowProc);
// ::ChooseColor(&cc);
