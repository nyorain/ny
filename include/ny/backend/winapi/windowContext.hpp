#pragma once

#include <ny/backend/winapi/include.hpp>
#include <ny/backend/windowContext.hpp>
#include <nytl/rect.hpp>

#include <windows.h>
#include <ole2.h>

namespace ny
{

///Extents the WindowSettings class with extra winapi-specific settings.
class WinapiWindowSettings : public WindowSettings {};

///WindowContext for winapi windows using the winapi backend on a windows OS.
class WinapiWindowContext : public WindowContext
{
public:
	static const char* nativeWidgetClassName(NativeWidgetType type);

public:
    WinapiWindowContext(WinapiAppContext& ctx, const WinapiWindowSettings& settings = {});
    ~WinapiWindowContext();

    void refresh() override;

    void show() override;
    void hide() override;

	void droppable(const DataTypes&) override {}

    void addWindowHints(WindowHints hints) override;
    void removeWindowHints(WindowHints hints) override;

    void size(const Vec2ui& size) override;
    void position(const Vec2i& position) override;

    void cursor(const Cursor& c) override;
    bool handleEvent(const Event& e) override;

	NativeWindowHandle nativeHandle() const override;

    //toplevel
    void maximize() override;
    void minimize() override;
    void fullscreen() override;
    void normalState() override;

    void minSize(const Vec2ui& size) override {};
    void maxSize(const Vec2ui& size) override {};

    void beginMove(const MouseButtonEvent* ev) override {};
    void beginResize(const MouseButtonEvent* ev, WindowEdges edges) override {};

	bool customDecorated() const override { return 0; };

    void icon(const Image* img) override;
    void title(const std::string& title) override;

    //winapi specific
	WinapiAppContext& appContext() const { return *appContext_; }

	HINSTANCE hinstance() const;
    HWND handle() const { return handle_; }

	Rect2i extents() const;

protected:
	struct State
	{
		std::uint64_t style {};
		std::uint64_t exstyle {};
		Rect2i extents {};
		bool maximized {};
		bool minimized {};
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

	std::string wndClassName_;

    HWND handle_ = nullptr;
	IDropTarget* dropTarget_ = nullptr;
	HCURSOR cursor_ = nullptr;

	bool fullscreen_ = false;
	std::uint64_t style_ = 0;
	State savedState_;
};


}
