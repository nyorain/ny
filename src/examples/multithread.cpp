#include <ny/ny.hpp>
#include <thread>

//XXX: this example shows some basic useful multithread features of ny

class MyEventHandler : public ny::EventHandler
{
public:
	MyEventHandler(ny::LoopControl& mainLoop, ny::WindowContext& wc) : lc_(mainLoop), wc_(wc) {}
	bool handleEvent(const ny::Event& ev) override;

	ny::BufferSurface* surface;

protected:
	ny::LoopControl& lc_;
	ny::WindowContext& wc_;
};

int main()
{
	auto& backend = ny::Backend::choose();
	auto ac = backend.createAppContext();

	ny::BufferSurface* bufferSurface {};

	ny::WindowSettings settings;
	settings.surface = ny::SurfaceType::buffer;
	settings.buffer.storeSurface = &bufferSurface;
	auto wc = ac->createWindowContext(settings);

	ny::LoopControl control;
	MyEventHandler handler(control, *wc);
	handler.surface = bufferSurface;

	wc->eventHandler(handler);
	wc->refresh();

	//Here we create a second thread that will sleep 2 seconds, execute a function from the main
	//thread, sleeps again 5 seconds and after that stops the dispatch loop in the gui thread.
	//We also use a flag to signal the main thread when the thread finished (only used to
	//check if joining it might take a longer time);
	std::atomic<bool> finished {false};
	auto t = std::thread([&]{
		std::this_thread::sleep_for(std::chrono::seconds(2));

		//Execute the function in the main thread.
		//Useful for synchronization.
		//See below when this might return false.
		if(!control.call([]{ ny::log("Hello from the main thread"); }))
			ny::log("calling from the main thread failed (has the loop already finished?)");

		std::this_thread::sleep_for(std::chrono::seconds(5));

		//Stop the dispatch loop in the main thread.
		//This might return false e.g. if the loop was already stopped.
		//Try to quit the window before this thread triggers the stop call (i.e. in the first
		//seconds) and the application should output the message.
		if(!control.stop()) ny::log("stopping the main loop failed (has it already finished?)");

		//Signal the main thread that we finished
		finished.store(true);
	});

	ny::log("Entering main dispatch loop");

	//We just normally run the main dispatch loop.
	//Note that we here pass a reference to control that allows the AppCotnext to implement it.
	ac->dispatchLoop(control);
	ny::log("Finished main dispatch loop");

	//We call this to make sure there is no unresponsive window when we wait for
	//the second thread to join.
	wc.reset();
	ac.reset();

	//This must be called to avoid std::terminate to be called from the threads destructor.
	//If the dispatch loop is exited before the thread has finished (i.e. if the window was
	//manually closed) this will keep the application alive.
	if(!finished.load()) ny::log("Waiting for the second thread to finish...");
	if(t.joinable()) t.join();
}

bool MyEventHandler::handleEvent(const ny::Event& ev)
{
	ny::log("Received event with type ", ev.type());

	if(ev.type() == ny::eventType::close)
	{
		ny::log("Window closed from server side. Exiting.");
		lc_.stop();
		return true;
	}
	else if(ev.type() == ny::eventType::key)
	{
		const auto& kev = static_cast<const ny::KeyEvent&>(ev);
		if(!kev.pressed) return false;

		if(kev.keycode == ny::Keycode::escape)
		{
			ny::log("Esc key pressed. Exiting.");
			lc_.stop();
			return true;
		}

		return false;
	}
	else if(ev.type() == ny::eventType::draw)
	{
		auto bufferGuard = surface->buffer();
		auto buffer = bufferGuard.get();

		auto size = buffer.stride * buffer.size.y;
		std::memset(buffer.data, 0xCC, size);

		return true;
	}

	return false;
};
