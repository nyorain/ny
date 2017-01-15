#include <ny/ny.hpp>

// This is the first ny example.
// It tries to cover the most important parts a ny application has or might use.
// In the following examples only the new parts will be documented so make sure to (roughly)
// understand the documented code here.
// If you have questions, just ask them on github (post an issue on github.com/nyorain/ny) and
// we will try to document unclear aspects.

// Create a custom Windowlistener that will handle events for windows.
// We only the handle the close event in this simple case.
// The class also holds an additional ny::LoopControl to which we will come in the main function.
// The close function is implemented at the bottom of this file.
class MyWindowListener : public ny::WindowListener {
public:
	ny::LoopControl* lc {};
	void close(const ny::CloseEvent& ev) override;
};

// Main function that just chooses a backend, creates Window- and AppContext from it, registers
// a custom EventHandler and then runs the mainLoop. All classes are only roughly described
// here, usually just looking at the ny header files and reading their documentation
// will really help you.
// The most important classes (interaces) of ny are ny::AppContext and ny::WindowContext.
int main()
{
	// We let ny choose a backend.
	// If no backend is available, this function will simply throw an exception.
	// The backend will determine the type of display manager used.
	// This function will try to choose the best available backend, but it can be
	// controlled by setting the NY_BACKEND environment variable.
	// After all, applications can also choose their backend in other ways.
	// Note how this is not done behind the scenes like other toolkits do.
	// The application has the full explicit control over what is done.
	// There is no global or implicit state in ny.
	auto& backend = ny::Backend::choose();

	// Here we let the backend create an AppContext implementation.
	// This represents the connection to the display, our method of creating windows and
	// receiving events from a display manager.
	auto ac = backend.createAppContext(); // decltype(ac): std::unique_ptr<ny::AppContext>

	// Create an object of our WindowListener class.
	auto listener = MyWindowListener {};

	// Now we let the AppContext create a WindowContext implementation.
	// This can then be used to receive input like e.g. key, button or mouse move events.
	// In later tutorials we will create windows in which we can render (using various methods),
	// but here we will create a renderless window which will usually result in flickering
	// or undefined window contents (might look ugly depending on the backend).
	// On some backends (e.g. wayland) this might output no window at all.
	// Here we therefore just use the default constructed WindowSettings, the only
	// thing we set is our own listener that should receive events about this window.
	// This listener could also be changed later on.
	// This can later be used to change various aspects of the created window.
	auto ws = ny::WindowSettings {};
	ws.listener = &listener;
	auto wc = ac->createWindowContext(ws); // decltype(wc): std::unique_ptr<ny::WindowContext>

	// Now we create a LoopControl object.
	// With this object we can stop the loop called below from the inside (in callbacks/listeners)
	// or even from different threads (see the ny-multithread example).
	// We also pass the loopControl to our listener, since it will use it to stop the main
	// loop when the window is closed.
	ny::LoopControl control {};
	listener.lc = &control;

	// ny::log can be used to easily output application information.
	// There are also other output methods, see ny/log.hpp.
	ny::log("Entering main loop");

	// We call the main dispatch loop, which will just wait for new events and process them
	// until a critical error occurs or we stop the loop using the passed LoopControl.
	// The LoopControl idiom can be imagined like this: We pass the looping function
	// an interface object and this function will set its implementation before starting
	// the loop that allows us in callbacks and listener functions triggered during the dispatch
	// loop to stop it (we do it below, when the window is closed).
	ac->dispatchLoop(control);

	// Clean up is done automatically, everything follows the RAII idiom.
}

void MyWindowListener::close(const ny::CloseEvent&)
{
	// We output that we received the close event and then call the stop function
	// on the loop control that controls the main dispatch loop on the AppContext.
	// This will cause the main loop to end and our program to exit gracefully.
	ny::log("Received an closed event - exiting");
	lc->stop();
}
