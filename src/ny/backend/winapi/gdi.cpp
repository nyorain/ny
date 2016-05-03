#include <ny/backend/winapi/gdi.hpp>

namespace ny
{

GdiWinapiWindowContext::GdiWinapiWindowContext(WinapiAppContext& ctx,
	const WinapiWindowSettings& settings) : WinapiWindowContext(ctx, settings)
{
	// drawContext_.reset(new GdiDrawContext(handle()));
}

DrawGuard GdiWinapiWindowContext::draw()
{
    auto hdc = GetDC(handle());
	drawContext_.reset(new GdiDrawContext(hdc));
	return DrawGuard(*drawContext_);
}

}
