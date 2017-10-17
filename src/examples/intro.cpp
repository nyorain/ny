#include <ny/ny.hpp> // all common ny headers
#include <dlg/dlg.hpp> // logging

// This is the first ny example.
// It tries to cover the most important parts a ny application has or might use.
// In the following examples only the new parts will be documented so make sure to (roughly)
// understand the documented code here.
// If you have questions, just ask them on github (post an issue on github.com/nyorain/ny) and
// we will try to document unclear aspects.
// Note that we don't render anything in this example so the window contents
// are undefined - expect graphical glitches.

// A custom Windowlistener that will handle events for our window.
// We only the handle the close event in this simple case.
// The class also holds an additional bool to communiate with the main loop.
// The close function is implemented at the bottom of this file.
class MyWindowListener : public ny::WindowListener {
public:
	bool* run;
	void close(const ny::CloseEvent& ev) override;
};

// Main function that just chooses a backend, creates Window- and AppContext from it, registers
// a custom EventHandler and then runs the mainLoop. All classes are only roughly described
// here, usually just looking at the ny header files and reading their documentation
// will really help you.
// The most important classes (interaces) of ny are ny::AppContext, ny::WindowContext
// and ny::WindowListener.
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

	// We (and ny) use dlg to output information. You could easily filter
	// out logs (e.g. from ny) you don't want.
	// There are also other output methods, see dlg for details
	dlg_info("Entering main loop");

	// The main loop: ny actually has no built-in main loop you have
	// to it yourself. We keep things simple and just wait/dispatch
	// events as long as the window was not closed. We use a bool
	// variable to communicate between window listener and our main loop
	bool run;
	listener.run = &run;
	while(run && ac->waitEvents());

	// Clean up is done automatically, everything follows the RAII idiom.
}

void MyWindowListener::close(const ny::CloseEvent&)
{
	// We output that we received the close event and then set the run
	// pointer to false, which will end the main loop.
	dlg_info("Received an closed event - exiting");
	*run = false;
}
