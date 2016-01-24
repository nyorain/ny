#pragma once

#include <ny/backend/x11/include.hpp>
#include <ny/backend/backend.hpp>

namespace ny
{

class X11Backend : public Backend
{
protected:
    static X11Backend instance_;
    X11Backend();

public:
	static X11Backend& instance(){ return instance_; }

public:
    virtual bool available() const override;

    virtual AppContextPtr createAppContext() override;
	virtual WindowContextPtr 
		createWindowContext(Window& win, const WindowSettings& s = {}) override;

	virtual std::string name() const override { return "x11"; }
};

}
