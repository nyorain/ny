#include <ny/base.hpp>
#include <ny/backend.hpp>
#include <ny/backend/integration/cairo.hpp>

#include <cairo/cairo.h>

//This example shows how to draw on a ny::WindowContext using cairo in a platform-neutral
//manner.

class MyEventHandler : public ny::EventHandler
{
public:
	ny::CairoIntegration* cairo;

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

	//XXX: interesting part here
	//we create a CairoIntegration for the created WindowContext.
	//this will trigger a backend-specific function to create the CairoIntegration object
	//which can then (in the event handler function) be used to retrieve a temporary cairo
	//surface on which can be drawn.
	//Note that if ny is built without cairo support (in which case including the header
	//should already trigger an error) or if the backend does not support cairo, this function
	//might return a nullptr.
	auto cairo = ny::cairoIntegration(*wc); //decltype(cairo): unique_ptr<ny::CairoIntegration>
	if(!cairo)
	{
		ny::error("Failed to create cairo integration");
		return EXIT_FAILURE;
	}

	//store the pointer in our event handler
	handler.cairo = cairo.get();

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
		//XXX: using cairo to draw onto the window
		//Here, get the surface guard which wraps a cairo_surface_t
		auto surfGuard = cairo->get();
		auto& surf = surfGuard.surface();

		//Then, create a cairo context for the returned surface and use it to draw
		auto cr = cairo_create(&surf);
		cairo_set_source_rgb(cr, 0.543, 1.0, 1.0);
		cairo_paint(cr);

		//always remember to destroy/recreate the cairo context on every draw call and dont
		//store it since the cairo surface might change from call to call
		cairo_destroy(cr);

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
