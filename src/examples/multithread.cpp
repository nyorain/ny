#include <ny/ny.hpp>
#include <nytl/time.hpp>
#include <thread>

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

	//XXX: The interesting part.
	//First we create a ny::EventDispatcher object.
	//This can be used to sent events manually to the event/backend/ui-thread
	ny::EventDispatcher evdispatcher;

	//Here we create a second thread that will sleep 5 seconds and after that send a pressed
	//escape KeyEvent to our event handler. This will trigger the application to exit.
	//Note that this is not undefined behaviour or a data race since the EventDispatcher assures
	//us synchronization.
	//We also use an atomic flag here to check if the thread has finished execution
	std::atomic<bool> threadFinished {false};
	auto t = std::thread([&]{
		std::this_thread::sleep_for(nytl::Seconds(5));
		auto e = ny::KeyEvent(&handler);
		e.keycode = ny::Keycode::escape;
		e.pressed = true;
		evdispatcher.dispatch(std::move(e));
		threadFinished = true;
	});

	ny::debug("Entering main loop");

	//Here we run a threaded dispatch loop instead of the normal one and pass the EventDispatcher.
	//The backend will then simply mix its own events with the events the EventDispatcher receives
	//from other threads.
	ac->threadedDispatchLoop(evdispatcher, control);

	//This must be called to avoid std::terminate to be called from the threads destructor.
	//If the application is exited before the thread has finished sleeping (i.e. by a real
	//keypress or by closing the window) this will keep the application (and potentially the
	//unresponsive window) alive.
	if(!threadFinished) ny::debug("Waiting for the second thread to finish...");
	if(t.joinable()) t.join();
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
	else if(ev.type() == ny::eventType::key)
	{
		const auto& kev = static_cast<const ny::KeyEvent&>(ev);
		if(!kev.pressed) return false;

		if(kev.keycode == ny::Keycode::escape)
		{
			ny::debug("Esc key pressed. Exiting.");
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
