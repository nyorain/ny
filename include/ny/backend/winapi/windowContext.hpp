#pragma once

#include <ny/backend/winapi/include.hpp>
#include <ny/backend/winapi/windows.hpp>
#include <ny/backend/windowContext.hpp>
#include <ny/backend/windowSettings.hpp>
#include <nytl/rect.hpp>

#include <ole2.h>

namespace ny
{

///Extents the WindowSettings class with extra winapi-specific settings.
class WinapiWindowSettings : public WindowSettings {};

///The base class for drawing integrations.
class WinapiDrawIntegration
{
public:
	WinapiDrawIntegration(WinapiWindowContext&);
	virtual ~WinapiDrawIntegration();
	virtual void resize(const nytl::Vec2ui&) {}

protected:
	WinapiWindowContext& context_; //TODO: rename windowContext_ for all backend DrawIntegrations
};

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
	WindowCapabilities capabilities() const override { return {}; }

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

    void icon(const ImageData& img) override;
    void title(const std::string& title) override;

    //winapi specific
	WinapiAppContext& appContext() const { return *appContext_; }

	HINSTANCE hinstance() const;
    HWND handle() const { return handle_; }

	///Returns the current (async) extents of the whole window, i.e. with server side
	///decorations.
	Rect2i extents() const;

	///Returns the extents of just the client area of the window.
	Rect2i clientExtents() const;

	//TODO:
	//Sets the integration to the given one.
	///Will return false if there is already such an integration or this implementation
	///does not support them (e.g. vulkan/opengl WindowContext).
	// virtual bool integration(WinapiDrawIntegration& integration);

	///Creates and returns a surface integration for this WindowContext, or an empty
	///surface (with type = none) if it could not be constructed.
	///This could be the case if the WindowContext already has another integration.
	// virtual Surface surface();

protected:
	struct State
	{
		std::uint64_t style {};
		std::uint64_t exstyle {};
		Rect2i extents {};
		unsigned int state {}; //0: normal, 1: maximized, 2: minimized, 3: fullscreen
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

	bool ownedCursor_ = false;
	HCURSOR cursor_ = nullptr;
	HICON icon_ = nullptr;

	bool fullscreen_ = false;
	std::uint64_t style_ = 0;
	State savedState_; //used e.g. for resetState

	WinapiDrawIntegration* drawIntegration_ = nullptr;
	friend class WinapiDrawIntegration;
};


}
