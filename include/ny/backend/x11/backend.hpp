#pragma once

#include <ny/backend/x11/include.hpp>
#include <ny/backend/backend.hpp>

namespace ny 
{

///X11 backend implementation.
class X11Backend : public Backend
{
public:
	static X11Backend& instance(){ return instance_; }

public:
    bool available() const override;
    AppContextPtr createAppContext() override;
	WindowContextPtr createWindowContext(AppContext& ctx, const WindowSettings& = {}) override;
	std::string name() const override { return "x11"; }

protected:
    static X11Backend instance_;
    X11Backend();
};

}

