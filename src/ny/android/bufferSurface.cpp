// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/android/bufferSurface.hpp>
#include <ny/log.hpp>

namespace ny {

// AndroidBufferSurface
AndroidBufferSurface::AndroidBufferSurface(ANativeWindow& nativeWindow)
	: nativeWindow_(nativeWindow)
{
	ANativeWindow_setBuffersGeometry(&nativeWindow, 0, 0, WINDOW_FORMAT_RGBA_8888);
}

AndroidBufferSurface::~AndroidBufferSurface()
{
	if(buffer_.bits)
		warning("ny::~AndroidBufferSurface: There is still an active buffer guard!");
}

BufferGuard AndroidBufferSurface::buffer()
{
	static const std::string funcName = "ny::AndroidBufferSurface::buffer: ";

	if(buffer_.bits)
		throw std::logic_error(funcName + "there is already a BufferGuard");

	// try to lock it
	auto ret = ANativeWindow_lock(&nativeWindow_, &buffer_, nullptr);
	if(ret != 0) {
		auto msg = "lock failed with code " + std::to_string(ret);
		throw std::runtime_error(funcName + msg);
	}

	// TODO: correct format query!
	// util func: androidToFormat?

	constexpr auto format = imageFormats::rgba8888;
	auto size = nytl::Vec2ui(buffer_.width, buffer_.height);
	auto data = reinterpret_cast<unsigned char*>(buffer_.bits);
	auto stride = static_cast<unsigned int>(buffer_.stride) * 32u; // pixel size : 32 bits
	return {*this, {data, size, format, stride}};
}

void AndroidBufferSurface::apply(const BufferGuard&) noexcept
{
	static const std::string funcName = "ny::AndroidBufferSurface::apply: ";
	if(!buffer_.bits)
		throw std::logic_error(funcName + "no active BufferGuard");

	buffer_ = {};

	int ret = ANativeWindow_unlockAndPost(&nativeWindow_);
	if(ret != 0)
		warning(funcName + "unlockAndPost failed with error code ", ret);
}

// AndroidBuffereWindowContext
AndroidBufferWindowContext::AndroidBufferWindowContext(AndroidAppContext& ac,
	const AndroidWindowSettings& settings) : AndroidWindowContext(ac, settings)
{
	if(!nativeWindow()) {
		warning("ny::AndroidBufferWindowContext: no native window");
		if(settings.buffer.storeSurface) *settings.buffer.storeSurface = nullptr;
		return;
	}

	bufferSurface_ = std::make_unique<AndroidBufferSurface>(*nativeWindow());
	if(settings.buffer.storeSurface)
		*settings.buffer.storeSurface = bufferSurface_.get();
}

Surface AndroidBufferWindowContext::surface() noexcept
{
	return {*bufferSurface_};
}

void AndroidBufferWindowContext::nativeWindow(ANativeWindow* window)
{
	AndroidWindowContext::nativeWindow(window);
	if(bufferSurface_) {
		SurfaceDestroyedEvent sde;
		listener().surfaceDestroyed(sde);
		bufferSurface_.reset();
	}

	if(window) {
		bufferSurface_ = std::make_unique<AndroidBufferSurface>(*window);
		SurfaceCreatedEvent sce;
		listener().surfaceCreated(sce);
	}
}

} // namespace ny
