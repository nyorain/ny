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

protected:
	ToplevelWindow() = default;
	void create(const vec2ui& size, const std::string& name = "", const WindowSettings& = {});

public:
	ToplevelWindow(const vec2ui& size, const std::string& name = "",
			const WindowSettings& settings = {});

	virtual ~ToplevelWindow();

    //hints
    bool hasMaximizeHint() const { return (hints_ & windowHints::maximize); }
    bool hasMinimizeHint() const { return (hints_ & windowHints::minimize); }
    bool hasResizeHint() const { return (hints_ & windowHints::resize); }
    bool hasMoveHint() const { return (hints_ & windowHints::move); }
    bool hasCloseHint() const { return (hints_ & windowHints::close); }
    bool customDecorated() const {  return (hints_ & windowHints::customDecorated); }

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
