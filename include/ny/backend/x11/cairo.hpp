#pragma once

#include <ny/backend/x11/include.hpp>
#include <ny/backend/x11/windowContext.hpp>

namespace ny
{

///WindowContext implementation on a x11 backend with cairo used for drawing.
class X11CairoWindowContext : public X11WindowContext
{
protected:
	std::unique_ptr<CairoDrawContext> drawContext_;
	xcb_visualtype_t* visualType_ = nullptr;

	virtual void initVisual() override;
	void resizeCairo(const Vec2ui& size);

public:
	X11CairoWindowContext(X11AppContext& ctx, const X11WindowSettings& settings = {});
	~X11CairoWindowContext();
	
	virtual DrawGuard draw() override;
    virtual void size(const Vec2ui& size) override;
	virtual bool handleEvent(const Event& e) override;
};

}
