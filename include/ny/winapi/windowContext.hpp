// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/winapi/include.hpp>
#include <ny/winapi/windows.hpp>
#include <ny/windowContext.hpp>
#include <ny/windowSettings.hpp>

#include <nytl/rect.hpp>
#include <nytl/vec.hpp>

namespace ny {

/// Extents the WindowSettings class with extra winapi-specific settings.
class WinapiWindowSettings : public WindowSettings {};

/// WindowContext for winapi windows using the winapi backend on a windows OS.
class WinapiWindowContext : public WindowContext {
public:
	WinapiWindowContext(WinapiAppContext& ctx, const WinapiWindowSettings& settings = {});
	~WinapiWindowContext();

	void refresh() override;
	void show() override;
	void hide() override;

	void addWindowHints(WindowHints hints) override;
	void removeWindowHints(WindowHints hints) override;

	void size(nytl::Vec2ui size) override;
	void position(nytl::Vec2i position) override;

	void cursor(const Cursor& c) override;

	NativeHandle nativeHandle() const override;
	WindowCapabilities capabilities() const override;
	Surface surface() override;

	//toplevel
	void maximize() override;
	void minimize() override;
	void fullscreen() override;
	void normalState() override;

	void minSize(nytl::Vec2ui size) override;
	void maxSize(nytl::Vec2ui size) override;

	void beginMove(const EventData* ev) override;
	void beginResize(const EventData* ev, WindowEdges edges) override;

	bool customDecorated() const override { return false; };

	void icon(const Image& img) override;
	void title(nytl::StringParam title) override;

	WinapiAppContext& appContext() const { return *appContext_; } ///The associated AppContext
	HINSTANCE hinstance() const; ///The associated HINSTANCE
	HWND handle() const { return handle_; } ///The managed window handle

	nytl::Rect2i extents() const; ///Current (async) window extents with server decorations
	nytl::Rect2i clientExtents() const; ///Current (async) client area window extents

	nytl::Vec2ui minSize() const { return minSize_; }
	nytl::Vec2ui maxSize() const { return maxSize_; }

protected:
	struct State {
		std::uint64_t style {};
		std::uint64_t exstyle {};
		nytl::Rect2i extents {};
		unsigned int state {}; // 0: normal, 1: maximized, 2: minimized, 3: fullscreen
	};

protected:
	WinapiWindowContext() = default;

	virtual void initWindowClass(const WinapiWindowSettings& settings);
	virtual WNDCLASSEX windowClass(const WinapiWindowSettings& settings);

	virtual void initWindow(const WinapiWindowSettings& settings);
	virtual void initDialog(const WinapiWindowSettings& settings);
	virtual void showWindow(const WinapiWindowSettings& settings);

	virtual void setStyle(const WinapiWindowSettings& settings);

	void unsetFullscreen();

protected:
	WinapiAppContext* appContext_ = nullptr;
	std::wstring wndClassName_;

	HWND handle_ = nullptr;
	winapi::com::DropTargetImpl* dropTarget_ = nullptr; // referenced-counted (shared owned here)

	// If ownedCursor_ is true, cursor_ was created, otherwise it was loaded (and must not
	// be destroyed). icon_ is always owned.
	bool ownedCursor_ = false;
	HCURSOR cursor_ = nullptr;
	HICON icon_ = nullptr;

	bool fullscreen_ = false;
	std::uint64_t style_ = 0;
	State savedState_; // used e.g. for resetState

	nytl::Vec2ui minSize_ {};
	nytl::Vec2ui maxSize_ {9999, 9999};
};

} // namespace ny
