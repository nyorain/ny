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
	if(ANativeWindow_lock(&windowContext_.nativeWindow(), &buffer_, nullptr) != 0)
		throw std::runtime_error("ny::AndroidBufferSurface::buffer failed");

	constexpr auto format = imageFormats::rgba8888;
	auto size = nytl::Vec2ui(buffer_.width, buffer_.height);
	auto data = reinterpret_cast<unsigned char*>(buffer_.bits);
	auto stride = static_cast<unsigned int>(buffer_.stride) * 32u; // pixel size : 32 bits
	return {*this, {data, size, format, stride}};
}

void AndroidBufferSurface::apply(const BufferGuard&) noexcept
{
	ANativeWindow_unlockAndPost(&windowContext_.nativeWindow());
}

// AndroidBuffereWindowContext
AndroidBufferWindowContext::AndroidBufferWindowContext(AndroidAppContext& ac,
	const AndroidWindowSettings& settings) : AndroidWindowContext(ac, settings),
		bufferSurface_(*this)
{
	if(settings.buffer.storeSurface) *settings.buffer.storeSurface = &bufferSurface_;
}

Surface AndroidBufferWindowContext::surface() noexcept
{
	return {bufferSurface_};
}

} // namespace ny
