#include <ny/surface.hpp>

#include <functional>
#include <vector>
#include <cstring>

namespace ny
{

//backend/integration/surface.cpp - private interface
using SurfaceIntegrateFunc = std::function<Surface(WindowContext&)>;
unsigned int registerSurfaceIntegrateFunc(const SurfaceIntegrateFunc& func);


BufferGuard::BufferGuard(BufferSurface& surface) : data_(surface.init()), surface_(surface)
{
}

BufferGuard::~BufferGuard()
{
	surface_.apply(data_);
}

Surface::Surface() : gl()
{
}

Surface::~Surface()
{
	if(type == Type::buffer) buffer.~unique_ptr<BufferSurface>();
}

//TODO: std-conform implementation of this
Surface::Surface(Surface&& other) noexcept
{
	std::memcpy(this, &other, sizeof(other));
	std::memset(&other, 0, sizeof(other));
}

Surface& Surface::operator=(Surface&& other) noexcept
{
	std::memcpy(this, &other, sizeof(other));
	std::memset(&other, 0, sizeof(other));
	return *this;
}


std::vector<SurfaceIntegrateFunc>& surfaceIntegrateFuncs()
{
	static std::vector<SurfaceIntegrateFunc> funcs;
	return funcs;
}

Surface surface(WindowContext& context)
{
	for(auto& f : surfaceIntegrateFuncs())
	{
		auto ret = f(context);
		if(ret.type != SurfaceType::none) return ret;
	}

	return {};
}

unsigned int registerSurfaceIntegrateFunc(const SurfaceIntegrateFunc& func)
{
	surfaceIntegrateFuncs().push_back(func);
	return surfaceIntegrateFuncs().size();
}

}
