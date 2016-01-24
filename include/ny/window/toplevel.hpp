#pragma once

#include <ny/include.hpp>
#include <ny/window/window.hpp>

namespace ny
{

class ToplevelWindow : public Window
{
protected:
    unsigned char handlingHints_{};
    ToplevelState state_{};
    std::string title_{};
    unsigned int borderSize_{};

    //draw window
    virtual void draw(DrawContext& dc) override;

	ToplevelWindow() = default;
	void create(const vec2ui& size, const std::string& name = "", const WindowSettings& = {});

public:
	ToplevelWindow(const vec2ui& size, const std::string& name = "",
			const WindowSettings& settings = {});

	virtual ~ToplevelWindow();

    //hints
    bool hasMaximizeHint() const { return (hints_ & windowHints::Maximize); }
    bool hasMinimizeHint() const { return (hints_ & windowHints::Minimize); }
    bool hasResizeHint() const { return (hints_ & windowHints::Resize); }
    bool hasMoveHint() const { return (hints_ & windowHints::Move); }
    bool hasCloseHint() const { return (hints_ & windowHints::Close); }

	const std::string& title() const { return title_; }
/*
    void setMaximizeHint(bool hint = 1);
    void setMinimizeHint(bool hint = 1);
    void setResizeHint(bool hint = 1);
    void setMoveHint(bool hint = 1);
    void setCloseHint(bool hint = 1);
*/
    //virtual const ToplevelWindow& toplevelParent() const override { return *this; };
    //virtual ToplevelWindow& toplevelParent() override { return *this; };

/*
    bool isCustomDecorated() const {  return (hints_ & windowHints::CustomDecorated); }
    bool isCustomMoved() const { return (hints_ & windowHints::CustomMoved); }
    bool isCustomResized() const { return (hints_ & windowHints::CustomResized); }

    //return if successful
    bool setCustomDecorated(bool set = 1);
    bool setCustomMoved(bool set = 1);
    bool setCustomResized(bool set = 1);
    ////
    std::string getTitle() const { return title_; }
    void setTitle(const std::string& n);

    void setIcon(const image* icon);


    window* getParent() const { return nullptr; };

    bool isMaximized() const { return (state_ == toplevelState::Maximized); };
    bool isMinimized() const { return (state_ == toplevelState::Minimized); };
    bool isFullscreen() const { return (state_ == toplevelState::Fullscreen); };

    virtual bool isVirtual() const final { return 0; }
*/
};

}
