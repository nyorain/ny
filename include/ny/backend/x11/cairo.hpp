#pragma once

#include <ny/backend/x11/include.hpp>
#include <ny/backend/x11/windowContext.hpp>
#include <nytl/vec.hpp>

//Prototype to not include xcb.h
typedef struct xcb_visualtype_t xcb_visualtype_t;

namespace ny
{

///WindowContext implementation on a x11 backend with cairo used for drawing.
class X11CairoWindowContext : public X11WindowContext
{
protected:
	std::unique_ptr<CairoDrawContext> drawContext_;
	xcb_visualtype_t* visualType_ = nullptr;

protected:
	///Overrides the X11WindowContext function to query a visual.
	///Instead of just getting the id it stores a visualtype structure needed for cairo-xcb 
	///surface creation.
	virtual void initVisual() override;

	///Resized the cairo surface of the owned DrawContext.
	void resizeCairo(const Vec2ui& size);

public:
	X11CairoWindowContext(X11AppContext& ctx, const X11WindowSettings& settings = {});
	~X11CairoWindowContext();
	
	///Returns a reference to the internal cairo draw context.
	virtual DrawGuard draw() override;

	///Overrides the size function to automatically resize the cairo surface.
    virtual void size(const Vec2ui& size) override;

	///Handles now received size events to resize the cairo surface.
	virtual bool handleEvent(const Event& e) override;
};

}
