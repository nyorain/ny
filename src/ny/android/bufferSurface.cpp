// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/android/bufferSurface.hpp>
#include <ny/log.hpp>

namespace ny {

// AndroidBufferSurface
AndroidBufferSurface::AndroidBufferSurface(AndroidWindowContext& wc) : windowContext_(wc)
{
}

BufferGuard AndroidBufferSurface::buffer()
{
	static const std::string funcName = "ny::AndroidBufferSurface::buffer: ";
	auto nativeWindow = windowContext_.nativeWindow();

	if(!nativeWindow)
		throw std::runtime_error(funcName + "there is currently no valid native window");

	if(buffer_.bits)
		throw std::logic_error(funcName + "there is already a BufferGuard");

	// make sure window has needed format
	if(nativeWindow != formatApplied_) {
		ANativeWindow_setBuffersGeometry(nativeWindow, 0, 0, WINDOW_FORMAT_RGBA_8888);
		formatApplied_ = nativeWindow;
	}

	// try to lock it
	auto ret = ANativeWindow_lock(nativeWindow, &buffer_, nullptr);
	if(ret != 0) {
		auto msg = "lock failed with code " + std::to_string(ret);
		throw std::runtime_error(funcName + msg);
	}

	constexpr auto format = imageFormats::rgba8888;
	auto size = nytl::Vec2ui(buffer_.width, buffer_.height);
	auto data = reinterpret_cast<unsigned char*>(buffer_.bits);
	auto stride = static_cast<unsigned int>(buffer_.stride) * 32u; // pixel size : 32 bits
	return {*this, {data, size, format, stride}};
}

void AndroidBufferSurface::apply(const BufferGuard&) noexcept
{
	static const std::string funcName = "ny::AndroidBufferSurface::apply: ";
	if(!windowContext_.nativeWindow())
		throw std::runtime_error(funcName + "there is currently no valid native window");

	if(!buffer_.bits)
		throw std::logic_error(funcName + "no active BufferGuard");

	buffer_ = {};

	int ret = ANativeWindow_unlockAndPost(windowContext_.nativeWindow());
	if(ret != 0)
		warning(funcName + "unlockAndPost failed with error code ", ret);
}

// AndroidBuffereWindowContext
AndroidBufferWindowContext::AndroidBufferWindowContext(AndroidAppContext& ac,
	const AndroidWindowSettings& settings) : AndroidWindowContext(ac, settings),
		bufferSurface_(*this)
{
	if(settings.buffer.storeSurface)
		*settings.buffer.storeSurface = &bufferSurface_;
}

Surface AndroidBufferWindowContext::surface() noexcept
{
	return {bufferSurface_};
}

} // namespace ny
