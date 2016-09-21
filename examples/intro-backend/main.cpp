#include <ny/base.hpp>
#include <ny/backend.hpp>
#include <ny/app/events.hpp>
#include <evg/drawContext.hpp>

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
			ny::debug("Window closed. Exiting.");
			loopControl_.stop();
			return true;
		}
		else if(ev.type() == ny::eventType::windowDraw)
		{
			auto guard = wc_.draw();
			auto& dc = guard.dc();
			dc.clear(evg::Color::white);
			return true;
		}

		return false;
	};

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

	ny::debug("Entering main loop");
	ac->dispatchLoop(control);
}
