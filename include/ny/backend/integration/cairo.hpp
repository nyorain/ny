#pragma once

#include <ny/include.hpp>
#include <memory>

typedef struct _cairo_surface cairo_surface_t;

namespace ny
{

class CairoIntegration;
class CairoSurfaceGuard;

class CairoSurfaceGuard
{
public:
	CairoSurfaceGuard(CairoIntegration&);
	~CairoSurfaceGuard();

	cairo_surface_t& surface() const;

protected:
	cairo_surface_t* surface_;
	CairoIntegration* integration_;
};


///Virtual base class that represents cairo integration for a WindowContext.
class CairoIntegration
{
public:
	CairoIntegration() = default;
	virtual ~CairoIntegration() = default;

	CairoSurfaceGuard get() { return CairoSurfaceGuard(*this); }

protected:
	virtual cairo_surface_t& init() = 0;
	virtual void apply(cairo_surface_t& surf) = 0;

	friend class CairoSurfaceGuard;
};

std::unique_ptr<CairoIntegration> cairoIntegration(WindowContext& context);


}

#ifndef NY_WithCairo
	#error ny was built without cairo. Do not include this header.
#endif
