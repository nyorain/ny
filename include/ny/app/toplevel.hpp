#pragma once

#include <ny/include.hpp>
#include <ny/app/window.hpp>

namespace ny
{

///ToplevelWindows are all windows that have to parent window.
///Some examples for further derived ToplevelWindow classes are Dialog and Frame.
class ToplevelWindow : public Window
{
public:
	ToplevelWindow(App& app, const WindowSettings& settings = {});
	ToplevelWindow(App& app, const Vec2ui& size, const std::string& title = "",
			const WindowSettings& settings = {});

	virtual ~ToplevelWindow();

    //hints
    bool maximizeHint() const { return hints_ & WindowHint::maximize; }
    bool minimizeHint() const { return hints_ & WindowHint::minimize; }
    bool resizeHint() const { return hints_ & WindowHint::resize; }
    bool closeHint() const { return hints_ & WindowHint::close; }
    bool customDecorated() const {  return hints_ & WindowHint::customDecorated; }

    void maximizeHint(bool set);
    void minimizeHint(bool set);
    void resizeHint(bool set);
    void closeHint(bool set);
    bool customDecorated(bool set);

	void icon(const Image& icon);

	void title(const std::string& ptitle);
	const std::string& title() const { return title_; }

	ToplevelState state() const { return state_; }
    bool maximized() const { return (state_ == ToplevelState::maximized); };
    bool minimized() const { return (state_ == ToplevelState::minimized); };
    bool isFullscreen() const { return (state_ == ToplevelState::fullscreen); };
	bool normalState() const { return (state_ == ToplevelState::normal); }

	void maximize();
	void minimize();
	void fullscreen();
	void reset();

protected:
    ToplevelState state_ {};
    std::string title_ {};
	WindowHints hints_ {};

protected:
	virtual void mouseMoveEvent(const MouseMoveEvent& event) override;
	virtual void mouseButtonEvent(const MouseButtonEvent& event) override;

	ToplevelWindow() = default;
	void create(App& app, const Vec2ui& size, const std::string& title = "",
			const WindowSettings& = {});
};

}
