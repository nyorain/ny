#include <ny/android/egl.hpp>
#include <ny/common/egl.hpp>
#include <ny/surface.hpp>

namespace ny
{

AndroidEglWindowContext::AndroidEglWindowContext(AndroidAppContext& ac, EglSetup& setup,
	const AndroidWindowSettings& ws) : AndroidWindowContext(ac, ws)
{
	auto androidnwindow = static_cast<void*>(nativeWindow());
	surface_ = std::make_unique<EglSurface>(setup, androidnwindow, ws.gl.config);

	//store surface if requested so
	if(ws.gl.storeSurface) *ws.gl.storeSurface = surface_.get();
}

bool AndroidEglWindowContext::surface(Surface& surface)
{
	surface.type = SurfaceType::gl;
	surface.gl = surface_.get();
	return true;
}

}
