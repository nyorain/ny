#pragma once

#include <ny/backend/wayland/include.hpp>
#include <ny/backend/wayland/windowContext.hpp>
#include <ny/backend/wayland/util.hpp>

#include <nytl/nonCopyable.hpp>
#include <nytl/vec.hpp>
#include <evg/cairo.hpp>

namespace ny
{

class WaylandCairoDrawContext : public evg::CairoDrawContext
{
public:
	WaylandCairoDrawContext(WaylandCairoWindowContext& wc, const Vec2ui& size);

	void init() override;
	void apply() override;

	void resize(const Vec2ui& size);
	const wayland::ShmBuffer& shmBuffer() const { return buffer_; }

protected:
	WaylandCairoWindowContext* windowContext_;
	wayland::ShmBuffer buffer_;
};

class WaylandCairoWindowContext: public WaylandWindowContext
{
public:
    WaylandCairoWindowContext(WaylandAppContext& ac, const WaylandWindowSettings& settings);

	DrawGuard draw() override;
	void size(const Vec2ui& size) override;

	///Attached the given buffer to the surface, damages the full surface and commits it.
	///Does also add a frame callback.
	void commit(wl_buffer& buffer);

protected:
	std::vector<WaylandCairoDrawContext> buffers_;
	nytl::Vec2ui size_;
};


}
