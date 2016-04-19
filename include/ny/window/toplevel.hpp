#pragma once

#include <ny/include.hpp>
#include <ny/window/window.hpp>
#include <ny/window/defs.hpp>

namespace ny
{

class ToplevelWindow : public Window
{
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

public:
	ToplevelWindow(App& app, const Vec2ui& size, const std::string& title = "",
			const WindowSettings& settings = {});

	virtual ~ToplevelWindow();

    //hints
    bool maximizeHint() const { return nytl::bitsSet(hints_, WindowHints::maximize); }
    bool minimizeHint() const { return nytl::bitsSet(hints_, WindowHints::minimize); }
    bool resizeHint() const { return nytl::bitsSet(hints_, WindowHints::resize); }
    bool closeHint() const { return nytl::bitsSet(hints_, WindowHints::close); }
    bool customDecorated() const {  return nytl::bitsSet(hints_, WindowHints::customDecorated); }

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
};

}
