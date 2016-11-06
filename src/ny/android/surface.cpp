#include <ny/android/surface.hpp>
#include <ny/log.hpp>

namespace ny
{

//backend/integration/surface.cpp - private interface
using SurfaceIntegrateFunc = std::function<Surface(WindowContext&)>;
unsigned int registerSurfaceIntegrateFunc(const SurfaceIntegrateFunc& func);

namespace
{
	Surface androidSurfaceIntegrateFunc(WindowContext& windowContext)
	{
		auto* awc = dynamic_cast<AndroidWindowContext*>(&windowContext);
		if(!awc) return {};

		Surface surface;
		awc->surface(surface);
		return surface;
	}

	static int registered = registerSurfaceIntegrateFunc(androidSurfaceIntegrateFunc);
}

AndroidBufferSurface::AndroidBufferSurface(AndroidWindowContext& wc) : AndroidDrawIntegration(wc)
{
}

MutableImageData AndroidBufferSurface::init()
{
	if(!ANativeWindow_lock(windowContext_.nativeWindow(), &buffer_, nullptr))
	{
		warning("ny::AndroidBufferSurface::init failed");
		return {};
	}

	constexpr auto format = ImageDataFormat::rgba8888;
	return {buffer_.bits, {buffer_.width, buffer_.height}, format, buffer_.stride};
}

void AndroidBufferSurface::apply(MutableImageData&)
{
	ANativeWindow_unlockAndPost(windowContext_.nativeWindow());
}

}
