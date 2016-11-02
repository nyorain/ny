#pragma once

#include <ny/include.hpp>
#include <nytl/vec.hpp>
#include <memory>

typedef struct _cairo_surface cairo_surface_t;

namespace ny
{

class CairoIntegration;
class CairoSurfaceGuard;

class CairoSurfaceGuard
{
public:
	CairoSurfaceGuard(CairoIntegration&, cairo_surface_t&, Vec2ui size);
	~CairoSurfaceGuard();

	cairo_surface_t& surface() const { return *surface_; }
	Vec2ui size() const { return size_; }

protected:
	CairoIntegration* integration_;
	cairo_surface_t* surface_;
	nytl::Vec2ui size_;
};


///Virtual base class that represents cairo integration for a WindowContext.
class CairoIntegration
{
public:
	CairoIntegration() = default;
	virtual ~CairoIntegration() = default;
	virtual CairoSurfaceGuard get() = 0;

protected:
	virtual void apply(cairo_surface_t& surf) = 0;
	friend class CairoSurfaceGuard;
};

std::unique_ptr<CairoIntegration> cairoIntegration(WindowContext& context);


}

#ifndef NY_WithCairo
	#error ny was built without cairo. Do not include this header.
#endif
