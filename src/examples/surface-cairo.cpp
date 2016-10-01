#include <ny/base.hpp>
#include <ny/backend.hpp>
#include <ny/backend/integration/surface.hpp>
#include <cairo/cairo.h>
#include <nytl/time.hpp>

//XXX: This example shows how to use cairo manually integrated using the surface/BufferSurface
//integration possibility. The performance should usually not be much worse, but one should
//nontheless prefer the normal cairo integration.
//Try to compare the performance of this example (ny-surface-cairo) with the native cairo 
//integration (ny-cairo) and on some backends you may notive a real difference (mainly for x11).

class MyEventHandler : public ny::EventHandler
{
public:
	ny::BufferSurface* surface;

public:
	MyEventHandler(ny::LoopControl& mainLoop, ny::WindowContext& wc) : lc_(mainLoop), wc_(wc) {}
	bool handleEvent(const ny::Event& ev) override;

protected:
	ny::LoopControl& lc_;
	ny::WindowContext& wc_;
};

int main()
{
	auto& backend = ny::Backend::choose();
	auto ac = backend.createAppContext();

	ny::WindowSettings settings;
	auto wc = ac->createWindowContext(settings);

	ny::LoopControl control;
	MyEventHandler handler(control, *wc);

	wc->eventHandler(handler);
	wc->refresh();

	//nothing special here, just create a surface integration
	//see examples/surface.cpp or ny/backend/integration/surface.hpp for more details on it
	auto surface = ny::surface(*wc);
	if(surface.type == ny::SurfaceType::none)
	{
		ny::error("Failed to create surface integration");
		return EXIT_FAILURE;
	}

	handler.surface = surface.buffer.get();

	ny::debug("Entering main loop");
	ac->dispatchLoop(control);
}

bool MyEventHandler::handleEvent(const ny::Event& ev)
{
	ny::debug("Received event with type ", ev.type());

	if(ev.type() == ny::eventType::close)
	{
		ny::debug("Window closed from server side. Exiting.");
		lc_.stop();
		return true;
	}
	else if(ev.type() == ny::eventType::draw)
	{
		//XXX: the interesting part
		//we just get a buffer from the BufferSurface and then create a cairo surface for it
		//note that (if the backend supports it) we can also use alpha to make the window
		//transparent
		auto buffGuard = surface->get();
		auto buff = buffGuard.get();

		//note that this only works if the returned buffer has argb8888 format!
		auto surf = cairo_image_surface_create_for_data(buff.data, CAIRO_FORMAT_ARGB32, 
			buff.size.x, buff.size.y, buff.stride);
		auto cr = cairo_create(surf);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_set_source_rgba(cr, 0.543, 0.4, 0.8, 0.5);
		cairo_paint(cr);

		//always remember that the data of the returned buffer may change between draw calls.
		//therefore we always have to create/destroy a new surface and context.
		//These both are actually not that expensive operations in cairo so it should be no
		//problem.
		cairo_destroy(cr);
		cairo_surface_destroy(surf);

		return true;
	}
	else if(ev.type() == ny::eventType::key)
	{
		if(!static_cast<const ny::KeyEvent&>(ev).pressed) return false;

		ny::debug("Key pressed. Exiting.");
		lc_.stop();
		return true;
	}

	return false;
};

