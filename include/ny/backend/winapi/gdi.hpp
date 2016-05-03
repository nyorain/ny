#pragma once

#include <ny/backend/winapi/include.hpp>
#include <ny/backend/winapi/windowContext.hpp>
#include <ny/draw/gdi.hpp>

#include <memory>

namespace ny
{

///Winapi WindowContext using gdi to draw.
class GdiWinapiWindowContext : public WinapiWindowContext
{
protected:
	std::unique_ptr<GdiDrawContext> drawContext_;

public:
	GdiWinapiWindowContext(WinapiAppContext& ctx, const WinapiWindowSettings& settings = {});
	virtual DrawGuard draw() override;
};

}
