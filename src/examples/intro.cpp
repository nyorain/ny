#include <ny/ny.hpp>

//XXX: This is the first ny example.
//It tries to cover the most important parts a ny application has or might use.
//In the following examples only the new parts will be documented so make sure to (roughly)
//understand the documented code here.
//If you have questions, just ask them on github (github.com/nyorain/ny, post an issue) and 
//we will document unclear aspects.

///Custom event handler for the low-level backend api.
///See intro-app for a higher level example if you think this is too complex.
class MyEventHandler : public ny::EventHandler
{
public:
	MyEventHandler(ny::LoopControl& mainLoop, ny::WindowContext& wc) : lc_(mainLoop), wc_(wc) {}

	//Virtual function derived from ny::EventHandler.
	//This function will receive the events for the created window.
	//It should return true if the event was processed and false otherwise (although it does not
	//make a difference in this case).
	bool handleEvent(const ny::Event& ev) override;

protected:
	ny::LoopControl& lc_;
	ny::WindowContext& wc_;
};

//Main function that just chooses a backend, creates Window- and AppContext from it, registers
//a custom EventHandler and then runs the mainLoop.
//There is no special main function (like WinMain or sth.) needed for different backends.
int main()
{
	//We let ny choose a backend.
	//If no backend is available, this function will simply throw an exception.
	//The backend will determine the type of display manager used, usually there is only
	//one available, but for corner cases (like e.g. wayland and XWayland) it will choose the
	//better/native one (in this case wayland).
	auto& backend = ny::Backend::choose();

	//Here we let the backend create an AppContext implementation.
	//This represents the connection to the display, our method of creating windows and
	//receiving events.
	auto ac = backend.createAppContext(); //decltype(ac): std::unique_ptr<ny::AppContext>
	
	//Now we let the AppContext create a WindowContext implementation.
	//Just use the defaulted WindowSettings.
	//This can later be used to change various aspects of the created window.
	ny::WindowSettings settings;
	auto wc = ac->createWindowContext(settings); //decltype(wc): std::unique_ptr<ny::WindowContext>

	//Now we create a LoopControl object.
	//With this object we can stop the dispatchLoop (which is called below) from the inside
	//or even from different threads (see ny-multithread).
	//We construct the EventHandler with a reference to it and when it receives an event that
	//the WindowContext was closed, it will stop the dispatchLoop, which will end this
	//program.
	ny::LoopControl control;
	MyEventHandler handler(control, *wc);

	//This call registers our EventHandler to receive the WindowContext related events from
	//the dispatchLoop.
	wc->eventHandler(handler);
	wc->refresh();

	//ny::debug can be used to easily output debug information.
	//The call will have no cost/effect when not compiled in debug mode.
	ny::debug("Entering main loop");
	ac->dispatchLoop(control);
}

bool MyEventHandler::handleEvent(const ny::Event& ev)
{
	ny::debug("Received event with type ", ev.type());

	//We want to application to exit when the window is closed.
	if(ev.type() == ny::eventType::close)
	{
		ny::debug("Window closed. Exiting.");
		lc_.stop();
		return true;
	}

	//We want the application also to exit when a key is pressed.
	//Some backends (wayland) may not have a possibilty to really close the window.
	else if(ev.type() == ny::eventType::key)
	{
		//Check if the KeyEvent was sent because a key was pressed.
		//If it was sent because it was released, dont handle the event.
		if(!static_cast<const ny::KeyEvent&>(ev).pressed) return false;

		ny::debug("Key pressed. Exiting.");
		lc_.stop();
		return true;
	}

	return false;
}
