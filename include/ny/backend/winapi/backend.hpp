#pragma once

#include <ny/backend/winapi/include.hpp>
#include <ny/backend/backend.hpp>

namespace ny
{

class WinapiBackend : public Backend
{
protected:
    static WinapiBackend instance_;
	WinapiBackend() = default;

public:
	static WinapiBackend& instance(){ return instance_; }

public:
    virtual bool available() const { return 1; } //TODO: sth to check here?

    virtual AppContextPtr createAppContext() override;
	virtual WindowContextPtr createWindowContext(AppContext& app,
			const WindowSettings& s = {}) override;

	virtual std::string name() const override { return "winapi"; }
};

}
