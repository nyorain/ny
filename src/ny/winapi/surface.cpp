#include <ny/backend/winapi/surface.hpp>
#include <nytl/vecOps.hpp>
#include <windows.h>

namespace ny
{

//backend/integration/surface.cpp - private interface
using SurfaceIntegrateFunc = std::function<Surface(WindowContext&)>;
unsigned int registerSurfaceIntegrateFunc(const SurfaceIntegrateFunc& func);

namespace
{
	Surface winapiSurfaceIntegrateFunc(WindowContext& windowContext)
	{
		auto* xwc = dynamic_cast<WinapiWindowContext*>(&windowContext);
		if(!xwc) return {};

		Surface surface;
		xwc->surface(surface);
		return surface;
	}

	static int registered = registerSurfaceIntegrateFunc(winapiSurfaceIntegrateFunc);
}

WinapiBufferSurface::WinapiBufferSurface(WinapiWindowContext& wc) : WinapiDrawIntegration(wc)
{
}
WinapiBufferSurface::~WinapiBufferSurface()
{
}

MutableImageData WinapiBufferSurface::init()
{
	auto currSize = windowContext_.clientExtents().size;
	auto currTotal = nytl::multiply(currSize);

	if(currTotal > nytl::multiply(size_))
		data_ = std::make_unique<std::uint8_t[]>(currTotal * 4);

	size_ = currSize;
	return {data_.get(), size_, ImageDataFormat::bgra8888};
}
void WinapiBufferSurface::apply(MutableImageData&)
{
	auto bitmap = ::CreateBitmap(size_.x, size_.y, 1, 32, data_.get());
	auto whdc = ::GetDC(windowContext_.handle());
	auto bhdc = ::CreateCompatibleDC(whdc);

	auto prev = ::SelectObject(bhdc, bitmap);
	::BitBlt(whdc, 0, 0, size_.x, size_.y, bhdc, 0, 0, SRCCOPY);
	::SelectObject(bhdc, prev);

	::DeleteDC(bhdc);
	::ReleaseDC(windowContext_.handle(), whdc);
	::DeleteObject(bitmap);
}

}
