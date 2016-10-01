#include <ny/base.hpp>
#include <ny/backend.hpp>
#include <ny/backend/integration/surface.hpp>
#include <cairo/cairo.h>

///Custom event handler for the low-level backend api.
///See intro-app for a higher level example if you think this is too complex.
class MyEventHandler : public ny::EventHandler
{
public:
	MyEventHandler(ny::LoopControl& mainLoop, ny::WindowContext& wc)
		: loopControl_(mainLoop), wc_(wc) {}

	///Virtual function from ny::EventHandler
	bool handleEvent(const ny::Event& ev) override
	{
		ny::debug("Received event with type ", ev.type());

		if(ev.type() == ny::eventType::close)
		{
			ny::debug("Window closed from server side. Exiting.");
			loopControl_.stop();
			return true;
		}
		else if(ev.type() == ny::eventType::draw)
		{
			auto buffGuard = surface->get();
			auto buff = buffGuard.get();

			//note that this only works if the returned buffer has argb8888 format!
			auto surf = cairo_image_surface_create_for_data(buff.data, CAIRO_FORMAT_ARGB32, 
				buff.size.x, buff.size.y, buff.stride);
			auto cr = cairo_create(surf);
			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
			cairo_set_source_rgba(cr, 0.543, 0.4, 0.8, 0.5);
			cairo_paint(cr);

			cairo_destroy(cr);
			cairo_surface_destroy(surf);

			return true;
		}
		else if(ev.type() == ny::eventType::key)
		{
			if(!static_cast<const ny::KeyEvent&>(ev).pressed) return false;

			ny::debug("Key pressed. Exiting.");
			loopControl_.stop();
			return true;
		}

		return false;
	};

	ny::BufferSurface* surface;

protected:
	ny::LoopControl& loopControl_;
	ny::WindowContext& wc_;
};


///Main function that just chooses a backend, creates Window- and AppContext from it, registers
///a custom EventHandler and then runs the mainLoop.
int main()
{
	///We let ny choose a backend.
	///If no backend is available, this function will simply throw.
	auto& backend = ny::Backend::choose();
	auto ac = backend.createAppContext();

	///Default WindowSettings.
	ny::WindowSettings settings;
	auto wc = ac->createWindowContext(settings);

	///With this object we can stop the dispatchLoop called below from inside.
	///We construct the EventHandler with a reference to it and when it receives an event that
	///the WindowContext was closed, it will stop the dispatchLoop, which will end this
	///program.
	ny::LoopControl control;
	MyEventHandler handler(control, *wc);

	///This call registers our EventHandler to receive the WindowContext related events from
	///the dispatchLoop.
	wc->eventHandler(handler);
	wc->refresh();

	//integrate with cairo
	auto surface = ny::surface(*wc);
	if(surface.type == ny::SurfaceType::none)
	{
		ny::warning("Failed to create surface integration");
		return EXIT_FAILURE;
	}

	handler.surface = surface.buffer.get();

	ny::debug("Entering main loop");
	ac->dispatchLoop(control);
}
