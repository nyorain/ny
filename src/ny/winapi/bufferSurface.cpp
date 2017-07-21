// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/winapi/bufferSurface.hpp>
#include <ny/log.hpp>
#include <windows.h>

namespace ny {

WinapiBufferSurface::WinapiBufferSurface(WinapiWindowContext& wc) : windowContext_(&wc)
{
}

WinapiBufferSurface::~WinapiBufferSurface()
{
	if(active_)
		ny_warn("::winbs::~winbs"_src, "there is still an active BufferGuard");
}

BufferGuard WinapiBufferSurface::buffer()
{
	if(active_)
		throw std::logic_error("ny::WinapiBufferSurface::get: has already an active BufferGuard");

	auto currSize = nytl::Vec2ui(windowContext().clientExtents().size);
	auto currTotal = currSize[0] * currSize[1] * 4;

	if(currTotal > dataSize_) {
		dataSize_ = currTotal * 4; // allocate more storage than needed
		data_ = std::make_unique<std::uint8_t[]>(dataSize_);
	}

	size_ = currSize;
	active_ = true;

	return {*this, {data_.get(), size_, ImageFormat::argb8888, size_[0] * 32}};
}
void WinapiBufferSurface::apply(const BufferGuard& bufferGuard) noexcept
{
	if(!active_ || bufferGuard.get().data != data_.get()) {
		ny_warn("ny::winbs::apply: invalid bufferGuard");
		return;
	}

	active_ = false;
	auto bitmap = ::CreateBitmap(size_[0], size_[1], 1, 32, data_.get());
	auto whdc = ::GetDC(windowContext().handle());
	auto bhdc = ::CreateCompatibleDC(whdc);

	auto prev = ::SelectObject(bhdc, bitmap);
	::BitBlt(whdc, 0, 0, size_[0], size_[1], bhdc, 0, 0, SRCCOPY);
	::SelectObject(bhdc, prev);

	::DeleteDC(bhdc);
	::ReleaseDC(windowContext().handle(), whdc);
	::DeleteObject(bitmap);
}

// WinapiBufferWindowContext
WinapiBufferWindowContext::WinapiBufferWindowContext(WinapiAppContext& ac,
	const WinapiWindowSettings& ws) : WinapiWindowContext(ac, ws), bufferSurface_(*this)
{
	if(ws.buffer.storeSurface) *ws.buffer.storeSurface = &bufferSurface_;
}

Surface WinapiBufferWindowContext::surface()
{
	return {bufferSurface_};
}

} // namespace ny
