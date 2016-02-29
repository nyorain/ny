#pragma once

#include <ny/include.hpp>
#include <ny/window/window.hpp>
#include <ny/window/defs.hpp>

namespace ny
{

class ToplevelWindow : public Window
{
protected:
    ToplevelState state_{};
    std::string title_{};
	
	bool maximizeHint_ {0};
	bool minimizeHint_ {0};
	bool resizeHint_ {0};
	bool moveHint_ {0};
	bool closeHint_ {0};
	bool customDecorated_ {0};

protected:
	virtual void mouseMoveEvent(const MouseMoveEvent& event) override;
	virtual void mouseButtonEvent(const MouseButtonEvent& event) override;

	ToplevelWindow() = default;
	void create(App& app, const Vec2ui& size, const std::string& name = "", 
			const WindowSettings& = {});

public:
	ToplevelWindow(App& app, const Vec2ui& size, const std::string& name = "",
			const WindowSettings& settings = {});

	virtual ~ToplevelWindow();

    //hints
    bool hasMaximizeHint() const { return (maximizeHint_); }
    bool hasMinimizeHint() const { return (minimizeHint_); }
    bool hasResizeHint() const { return (resizeHint_); }
    bool hasMoveHint() const { return (moveHint_); }
    bool hasCloseHint() const { return (closeHint_); }
    bool customDecorated() const {  return (customDecorated_); }

    void maximizeHint(bool set);
    void minimizeHint(bool set);
    void resizeHint(bool set);
    void moveHint(bool set);
    void closeHint(bool set);
    bool customDecorated(bool set);

	void icon(const Image& icon);

	void title(const std::string& ptitle);
	const std::string& title() const { return title_; } 

	ToplevelState state() const { return state_; }
    bool maximized() const { return (state_ == ToplevelState::maximized); };
    bool minimized() const { return (state_ == ToplevelState::minimized); };
    bool fullscreen() const { return (state_ == ToplevelState::fullscreen); };
};

}
