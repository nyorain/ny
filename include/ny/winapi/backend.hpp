#pragma once

#include <ny/winapi/include.hpp>
#include <ny/backend.hpp>

namespace ny
{

///Winapi backend implementation.
class WinapiBackend : public Backend
{
public:
	static WinapiBackend& instance(){ return instance_; }

public:
	bool available() const override { return true; } //TODO: sth to check here?
	AppContextPtr createAppContext() override;
	const char* name() const override { return "winapi"; }

protected:
	static WinapiBackend instance_;
	WinapiBackend() = default;
};

}
