#pragma once

#include <ny/backend/wayland/include.hpp>
#include <ny/backend/backend.hpp>

namespace ny
{

///Wayland backend implementation.
class WaylandBackend : public Backend
{
public:
	static WaylandBackend& instance(){ return instance_; }

public:
    bool available() const override;
    AppContextPtr createAppContext() override;
	const char* name() const override { return "wayland"; } 

protected:
    static WaylandBackend instance_;
    WaylandBackend();
};

}
