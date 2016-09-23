#include <ny/base.hpp>
#include <ny/backend.hpp>
#include <ny/app/events.hpp>
#include <ny/backend/integration/cairo.hpp>

#include <evg/image.hpp>
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

		if(ev.type() == ny::eventType::windowClose)
		{
			ny::debug("Window closed from server side. Exiting.");
			loopControl_.stop();
			return true;
		}
		else if(ev.type() == ny::eventType::windowDraw)
		{
			auto surfGuard = cairo->get();
			auto& surf = surfGuard.surface();

			auto cr = cairo_create(&surf);
			cairo_set_source_rgb(cr, 0.543, 1.0, 1.0);
			cairo_paint(cr);
			cairo_destroy(cr);

			return true;
		}
		else if(ev.type() == ny::eventType::mouseButton)
		{
			wc_.icon({});
			wc_.cursor(ny::CursorType::leftPtr);

			return true;
		}
		else if(ev.type() == ny::eventType::key)
		{
			ny::debug("Key pressed or released. Exiting.");
			loopControl_.stop();
			return true;
		}

		return false;
	};

	ny::CairoIntegration* cairo;

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

	///Set the cursor to a native grab cursor
	evg::Image cursorImage("cursor.png");
	// ny::Cursor cursor(ny::CursorType::grab);
	ny::Cursor cursor({*cursorImage.data(), cursorImage.size(), ny::ImageDataFormat::rgba8888});
	wc->cursor(cursor);

	///Set the icon to a loaded icon
	evg::Image iconImage("icon.png");
	wc->icon({*iconImage.data(), iconImage.size(), ny::ImageDataFormat::rgba8888});

	//integrate with cairo
	auto cairo = ny::cairoIntegration(*wc);
	if(!cairo)
	{
		ny::warning("Failed to create cairo integration");
		return EXIT_FAILURE;
	}

	handler.cairo = cairo.get();

	ny::debug("Entering main loop");
	ac->dispatchLoop(control);
}
