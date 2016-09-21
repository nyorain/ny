#include <ny/backend/wayland/cairo.hpp>
#include <ny/backend/wayland/appContext.hpp>
#include <ny/backend/wayland/interfaces.hpp>

#include <ny/base/log.hpp>
#include <ny/base/imageData.hpp>

#include <nytl/rect.hpp>

#include <cairo/cairo.h>
#include <wayland-client-protocol.h>


namespace ny
{

using namespace wayland;

WaylandCairoDrawContext::WaylandCairoDrawContext(WaylandCairoWindowContext& wc, const Vec2ui& size)
	: windowContext_(&wc), buffer_(wc.appContext(), size)
{
	resize(size);
}

void WaylandCairoDrawContext::init()
{
	CairoDrawContext::init();
}

void WaylandCairoDrawContext::apply()
{
	if(buffer_.format() != WL_SHM_FORMAT_ARGB8888 && buffer_.format() != WL_SHM_FORMAT_XRGB8888)
	{
		auto fromData = cairo_image_surface_get_data(cairoSurface());
		auto width = cairo_image_surface_get_width(cairoSurface());
		auto height = cairo_image_surface_get_height(cairoSurface());

		auto to = waylandToImageFormat(buffer_.format());
		auto& toData = buffer_.data();
		convertFormat(ImageDataFormat::argb8888, to, *fromData, toData, {width, height});
	}

	CairoDrawContext::apply();
	windowContext_->commit(buffer_.wlBuffer());
}

void WaylandCairoDrawContext::resize(const Vec2ui& size)
{
	buffer_.size(size);
	cairo_surface_t* surf = nullptr;
	if(buffer_.format() == WL_SHM_FORMAT_ARGB8888 || buffer_.format() == WL_SHM_FORMAT_XRGB8888)
	{
		surf = cairo_image_surface_create_for_data(&buffer_.data(), CAIRO_FORMAT_ARGB32, 
			size.x, size.y, size.x * 4);
	}
	else
	{
		surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size.x, size.y);
	}

	CairoDrawContext::operator=({*surf});
}

//WaylandCairoDrawContext
WaylandCairoWindowContext::WaylandCairoWindowContext(WaylandAppContext& ac, 
	const WaylandWindowSettings& settings) : WaylandWindowContext(ac, settings)
{
	size_ = settings.size;
}

evg::DrawGuard WaylandCairoWindowContext::draw()
{
	if(frameCallback_) warning("WaylandCairoWC::draw still waiting for frame event");
	for(auto& b : buffers_)
		if(!b.shmBuffer().used()) return b;

	buffers_.emplace_back(*this, size_);
	return buffers_.back();
}

void WaylandCairoWindowContext::size(const nytl::Vec2ui& size)
{
	size_ = size;
	for(auto& b : buffers_) b.resize(size);
}

void WaylandCairoWindowContext::commit(wl_buffer& buffer)
{
	frameCallback_ = wl_surface_frame(wlSurface_);
	wl_callback_add_listener(frameCallback_, &frameListener, this);

	wl_surface_damage(wlSurface_, 0, 0, size_.x, size_.y);
	wl_surface_attach(wlSurface_, &buffer, 0, 0);
	wl_surface_commit(wlSurface_);
}

}

// //cairo/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// waylandCairoDrawContext::waylandCairoDrawContext(const waylandWindowContext& wc) : cairoDrawContext(wc.getWindow()), wc_(wc)
// {
//     Vec2ui size = wc.getWindow().getSize();
//     buffer_[0] = new wayland::shmBuffer(size, bufferFormat::argb8888); //front buffer
//     buffer_[1] = new wayland::shmBuffer(size, bufferFormat::argb8888);
// 
//     //todo: corRect dynamic format
//     cairoSurface_ = cairo_image_surface_create_for_data((unsigned char*) frontBuffer()->getData(), CAIRO_FORMAT_ARGB32, size.x, size.y, size.x * 4);
//     cairoBackSurface_ = cairo_image_surface_create_for_data((unsigned char*) backBuffer()->getData(), CAIRO_FORMAT_ARGB32, size.x, size.y, size.x * 4);
// 
//     cairoCR_ = cairo_create(cairoSurface_);
//     cairoBackCR_ = cairo_create(cairoBackSurface_);
// }
// 
// waylandCairoDrawContext::~waylandCairoDrawContext()
// {
//     if(cairoCR_) cairo_destroy(cairoCR_);
//     if(cairoSurface_) cairo_surface_destroy(cairoSurface_);
// 
//     if(cairoBackCR_) cairo_destroy(cairoBackCR_);
//     if(cairoBackSurface_) cairo_surface_destroy(cairoBackSurface_);
// 
//     if(frontBuffer()) delete frontBuffer();
//     if(backBuffer()) delete backBuffer();
// }
// 
// void waylandCairoDrawContext::updateSize(const Vec2ui& size)
// {
//     if(size != frontBuffer()->getSize())
//     {
//         if(cairoCR_) cairo_destroy(cairoCR_);
//         if(cairoSurface_) cairo_surface_destroy(cairoSurface_);
// 
//         frontBuffer()->setSize(size);
// 
//         cairoSurface_ = cairo_image_surface_create_for_data((unsigned char*) frontBuffer()->getData(), CAIRO_FORMAT_ARGB32, size.x, size.y, size.x * 4);
//         cairoCR_ = cairo_create(cairoSurface_);
//     }
// }
// 
// void waylandCairoDrawContext::swapBuffers()
// {
//     frontID_ ^= 1;
// 
//     std::swap(cairoSurface_, cairoBackSurface_);
//     std::swap(cairoCR_, cairoBackCR_);
// }
// 
// void waylandCairoDrawContext::attach(const Vec2i& pos)
// {
//     wl_surface_attach(wc_.getWlSurface(), frontBuffer()->getWlBuffer(), pos.x, pos.y);
//     frontBuffer()->wasAttached();
// }
// 
// bool waylandCairoDrawContext::frontBufferUsed() const
// {
//     return frontBuffer()->used();
// }


